#include "apiclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QDebug>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_model("venice-uncensored")
    , m_streaming(false)
{
}

void ApiClient::setApiKey(const QString &key)
{
    m_apiKey = key;
}

void ApiClient::setModel(const QString &model)
{
    m_model = model;
}

void ApiClient::setStreaming(bool enabled)
{
    m_streaming = enabled;
}

void ApiClient::sendMessage(const QList<ChatMessage> &messages)
{
    QThread *thread = new QThread();
    ChatRequestWorker *worker = new ChatRequestWorker(m_apiKey, m_model, messages, m_streaming);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &ChatRequestWorker::execute);
    connect(worker, &ChatRequestWorker::responseReceived, this, &ApiClient::responseReceived);
    connect(worker, &ChatRequestWorker::responseChunk, this, &ApiClient::responseChunk);
    connect(worker, &ChatRequestWorker::responseFinished, this, &ApiClient::responseFinished);
    connect(worker, &ChatRequestWorker::errorOccurred, this, &ApiClient::errorOccurred);
    connect(worker, &ChatRequestWorker::responseReceived, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::responseFinished, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::errorOccurred, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void ApiClient::fetchModels()
{
    QThread *thread = new QThread();
    ModelsRequestWorker *worker = new ModelsRequestWorker(m_apiKey);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &ModelsRequestWorker::execute);
    connect(worker, &ModelsRequestWorker::modelsFetched, this, &ApiClient::modelsFetched);
    connect(worker, &ModelsRequestWorker::errorOccurred, this, &ApiClient::errorOccurred);
    connect(worker, &ModelsRequestWorker::modelsFetched, thread, &QThread::quit);
    connect(worker, &ModelsRequestWorker::errorOccurred, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

ChatRequestWorker::ChatRequestWorker(const QString &apiKey, const QString &model, const QList<ChatMessage> &messages, bool streaming)
    : m_apiKey(apiKey), m_model(model), m_messages(messages), m_streaming(streaming)
{
}

QString ChatRequestWorker::parseSSELine(const QString &line)
{
    if (line.isEmpty() || line == "data: [DONE]") {
        return QString();
    }

    QString data = line;
    if (data.startsWith("data: ")) {
        data = data.mid(6);
    }

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (doc.isObject()) {
        QJsonObject obj = doc.object();
        if (obj.contains("choices")) {
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject choice = choices[0].toObject();
                QJsonObject delta = choice["delta"].toObject();
                QString content = delta["content"].toString();
                QString finishReason = choice["finish_reason"].toString();
                if (finishReason == "stop") {
                    return QString();
                }
                return content;
            }
        }
    }
    return QString();
}

void ChatRequestWorker::execute()
{
    httplib::Client cli("https://api.venice.ai");
    cli.set_follow_location(true);

    QJsonObject payload;
    payload["model"] = m_model;
    payload["stream"] = m_streaming;

    QJsonArray jsonMessages;
    for (const auto &msg : m_messages) {
        QJsonObject jsonMsg;
        jsonMsg["role"] = msg.role;
        jsonMsg["content"] = msg.content;
        jsonMessages.append(jsonMsg);
    }
    payload["messages"] = jsonMessages;

    QJsonDocument doc(payload);
    std::string body = doc.toJson().toStdString();

    httplib::Headers headers = {
        {"Authorization", "Bearer " + m_apiKey.toStdString()}
    };

    if (m_streaming) {
        QString fullResponse;
        httplib::Result res = cli.Post("/api/v1/chat/completions", headers, body, "application/json",
            [&](const char *data, size_t data_length) {
                QString chunk = QString::fromUtf8(data, static_cast<int>(data_length));
                QStringList lines = chunk.split("\n");
                for (const auto &line : lines) {
                    QString content = parseSSELine(line.trimmed());
                    if (!content.isEmpty()) {
                        fullResponse += content;
                        emit responseChunk(content);
                    }
                }
                return true;
            });

        if (!res) {
            QString errMsg = QString("Request failed: error code %1").arg(static_cast<int>(res.error()));
            qDebug() << "ChatRequestWorker Error:" << errMsg;
            emit errorOccurred(errMsg);
            return;
        }

        if (res->status != 200) {
            QString errorMsg = QString("HTTP %1").arg(res->status);
            qDebug() << "ChatRequestWorker HTTP Error:" << res->status << QString::fromStdString(res->body);
            QJsonDocument errDoc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
            if (errDoc.isObject()) {
                QJsonValue detailsVal = errDoc.object()["details"];
                if (detailsVal.isObject()) {
                    QJsonValue errorsVal = detailsVal.toObject()["_errors"];
                    if (errorsVal.isArray() && !errorsVal.toArray().isEmpty()) {
                        errorMsg = errorsVal.toArray()[0].toString();
                    }
                }
                if (errorMsg.startsWith("HTTP")) {
                    QJsonValue errorVal = errDoc.object()["error"];
                    if (errorVal.isString()) {
                        errorMsg = errorVal.toString();
                    }
                }
            } else {
                errorMsg += ": " + QString::fromStdString(res->body);
            }
            emit errorOccurred(errorMsg);
            return;
        }

        emit responseFinished();
    } else {
        auto res = cli.Post("/api/v1/chat/completions", headers, body, "application/json");

        if (!res) {
            QString errMsg = QString("Request failed: error code %1").arg(static_cast<int>(res.error()));
            qDebug() << "ChatRequestWorker Error:" << errMsg;
            emit errorOccurred(errMsg);
            return;
        }

        if (res->status != 200) {
            QString errorMsg = QString("HTTP %1").arg(res->status);
            qDebug() << "ChatRequestWorker HTTP Error:" << res->status << QString::fromStdString(res->body);
            QJsonDocument errDoc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
            if (errDoc.isObject()) {
                QJsonValue detailsVal = errDoc.object()["details"];
                if (detailsVal.isObject()) {
                    QJsonValue errorsVal = detailsVal.toObject()["_errors"];
                    if (errorsVal.isArray() && !errorsVal.toArray().isEmpty()) {
                        errorMsg = errorsVal.toArray()[0].toString();
                    }
                }
                if (errorMsg.startsWith("HTTP")) {
                    QJsonValue errorVal = errDoc.object()["error"];
                    if (errorVal.isString()) {
                        errorMsg = errorVal.toString();
                    }
                }
            } else {
                errorMsg += ": " + QString::fromStdString(res->body);
            }
            emit errorOccurred(errorMsg);
            return;
        }

        QJsonDocument responseDoc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
        QJsonObject obj = responseDoc.object();

        if (obj.contains("choices")) {
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject firstChoice = choices[0].toObject();
                QJsonObject message = firstChoice["message"].toObject();
                QString content = message["content"].toString();

                int promptTokens = 0, completionTokens = 0, totalTokens = 0;
                if (obj.contains("usage")) {
                    QJsonObject usage = obj["usage"].toObject();
                    promptTokens = usage["prompt_tokens"].toInt();
                    completionTokens = usage["completion_tokens"].toInt();
                    totalTokens = usage["total_tokens"].toInt();
                }

                emit responseReceived(content, promptTokens, completionTokens, totalTokens);
            } else {
                emit errorOccurred("Empty choices in response");
            }
        } else {
            emit errorOccurred("Unexpected response format");
        }
    }
}

ModelsRequestWorker::ModelsRequestWorker(const QString &apiKey)
    : m_apiKey(apiKey)
{
}

void ModelsRequestWorker::execute()
{
    httplib::Client cli("https://api.venice.ai");
    cli.set_follow_location(true);

    httplib::Headers headers = {
        {"Authorization", "Bearer " + m_apiKey.toStdString()}
    };

    auto res = cli.Get("/api/v1/models", headers);

    if (!res) {
        QString errMsg = QString("Request failed: error code %1").arg(static_cast<int>(res.error()));
        qDebug() << "ModelsRequestWorker Error:" << errMsg;
        emit errorOccurred(errMsg);
        return;
    }

    if (res->status != 200) {
        QString errMsg = QString("HTTP %1").arg(res->status);
        qDebug() << "ModelsRequestWorker HTTP Error:" << res->status << QString::fromStdString(res->body);
        emit errorOccurred(errMsg);
        return;
    }

    QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
    QJsonObject obj = doc.object();

    if (obj.contains("data")) {
        QJsonArray dataArray = obj["data"].toArray();
        QStringList modelList;
        for (const auto &item : dataArray) {
            modelList.append(item.toObject()["id"].toString());
        }
        emit modelsFetched(modelList);
    } else {
        emit errorOccurred("Unexpected response format");
    }
}
