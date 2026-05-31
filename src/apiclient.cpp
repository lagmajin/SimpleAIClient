#include "apiclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QDebug>
#include <QDateTime>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_model("venice-uncensored")
    , m_streaming(false)
    , m_temperature(0.7)
    , m_maxTokens(0)
    , m_webSearch(false)
    , m_activeRequestThread(nullptr)
    , m_activeRequestWorker(nullptr)
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

void ApiClient::setSystemPrompt(const QString &prompt)
{
    m_systemPrompt = prompt;
}

void ApiClient::setTemperature(double temp)
{
    m_temperature = temp;
}

void ApiClient::setMaxTokens(int maxTokens)
{
    m_maxTokens = maxTokens;
}

void ApiClient::setWebSearch(bool enabled)
{
    m_webSearch = enabled;
}

void ApiClient::sendMessage(const QList<ChatMessage> &messages)
{
    cancelCurrentRequest();

    QThread *thread = new QThread();
    ChatRequestWorker *worker = new ChatRequestWorker(m_apiKey, m_model, messages, m_streaming, m_systemPrompt, m_temperature, m_maxTokens, m_webSearch);
    worker->moveToThread(thread);
    m_activeRequestThread = thread;
    m_activeRequestWorker = worker;

    connect(thread, &QThread::started, worker, &ChatRequestWorker::execute);
    connect(worker, &ChatRequestWorker::responseReceived, this, &ApiClient::responseReceived);
    connect(worker, &ChatRequestWorker::responseChunk, this, &ApiClient::responseChunk);
    connect(worker, &ChatRequestWorker::responseFinished, this, &ApiClient::responseFinished);
    connect(worker, &ChatRequestWorker::requestCancelled, this, &ApiClient::requestCancelled);
    connect(worker, &ChatRequestWorker::errorOccurred, this, &ApiClient::errorOccurred);
    connect(worker, &ChatRequestWorker::responseReceived, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::responseFinished, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::requestCancelled, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::errorOccurred, thread, &QThread::quit);
    connect(thread, &QThread::finished, this, [this, thread, worker]() {
        if (m_activeRequestThread == thread) {
            m_activeRequestThread = nullptr;
        }
        if (m_activeRequestWorker == worker) {
            m_activeRequestWorker = nullptr;
        }
    });
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);

    thread->start();
}

void ApiClient::cancelCurrentRequest()
{
    if (m_activeRequestWorker) {
        QMetaObject::invokeMethod(m_activeRequestWorker, "cancel", Qt::QueuedConnection);
    }
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

ChatRequestWorker::ChatRequestWorker(const QString &apiKey, const QString &model, const QList<ChatMessage> &messages, bool streaming, const QString &systemPrompt, double temperature, int maxTokens, bool webSearch)
    : m_apiKey(apiKey), m_model(model), m_messages(messages), m_streaming(streaming), m_startTime(0), m_systemPrompt(systemPrompt), m_temperature(temperature), m_maxTokens(maxTokens), m_webSearch(webSearch), m_cancelRequested(false)
{
}

void ChatRequestWorker::cancel()
{
    m_cancelRequested.store(true);
    if (m_client) {
        m_client->stop();
    }
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
    m_startTime = QDateTime::currentMSecsSinceEpoch();

    m_cancelRequested.store(false);
    m_client = std::make_shared<httplib::Client>("https://api.venice.ai");
    m_client->set_follow_location(true);

    QJsonObject payload;
    payload["model"] = m_model;
    payload["stream"] = m_streaming;

    QJsonArray jsonMessages;
    if (!m_systemPrompt.isEmpty()) {
        QJsonObject systemMsg;
        systemMsg["role"] = "system";
        systemMsg["content"] = m_systemPrompt;
        jsonMessages.append(systemMsg);
    }

    for (const auto &msg : m_messages) {
        QJsonObject jsonMsg;
        jsonMsg["role"] = msg.role;

        if (!msg.imageUrl.isEmpty()) {
            QJsonArray contentArray;
            QJsonObject textPart;
            textPart["type"] = "text";
            textPart["text"] = msg.content;
            contentArray.append(textPart);

            QJsonObject imagePart;
            imagePart["type"] = "image_url";
            QJsonObject imageUrlObj;
            imageUrlObj["url"] = msg.imageUrl;
            imagePart["image_url"] = imageUrlObj;
            contentArray.append(imagePart);

            jsonMsg["content"] = contentArray;
        } else {
            jsonMsg["content"] = msg.content;
        }

        jsonMessages.append(jsonMsg);
    }

    if (m_temperature > 0) {
        payload["temperature"] = m_temperature;
    }

    if (m_maxTokens > 0) {
        payload["max_tokens"] = m_maxTokens;
    }

    if (m_webSearch) {
        QJsonObject veniceParams;
        veniceParams["enable_web_search"] = "auto";
        payload["venice_parameters"] = veniceParams;
    }

    payload["messages"] = jsonMessages;

    QJsonDocument doc(payload);
    std::string body = doc.toJson().toStdString();

    httplib::Headers headers = {
        {"Authorization", "Bearer " + m_apiKey.toStdString()}
    };

    if (m_streaming) {
        QString fullResponse;
        httplib::Result res = m_client->Post("/api/v1/chat/completions", headers, body, "application/json",
            [&](const char *data, size_t data_length) {
                if (m_cancelRequested.load()) {
                    return false;
                }
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

        if (m_cancelRequested.load()) {
            emit requestCancelled();
            m_client.reset();
            return;
        }

        if (!res) {
            QString errMsg = QString("Request failed: error code %1").arg(static_cast<int>(res.error()));
            qDebug() << "ChatRequestWorker Error:" << errMsg;
            emit errorOccurred(errMsg);
            m_client.reset();
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
            m_client.reset();
            return;
        }

        emit responseFinished(static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startTime));
    } else {
        auto res = m_client->Post("/api/v1/chat/completions", headers, body, "application/json");

        if (m_cancelRequested.load()) {
            emit requestCancelled();
            m_client.reset();
            return;
        }

        if (!res) {
            QString errMsg = QString("Request failed: error code %1").arg(static_cast<int>(res.error()));
            qDebug() << "ChatRequestWorker Error:" << errMsg;
            emit errorOccurred(errMsg);
            m_client.reset();
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
            m_client.reset();
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

                int responseTime = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startTime);
                emit responseReceived(content, promptTokens, completionTokens, totalTokens, responseTime);
            } else {
                emit errorOccurred("Empty choices in response");
            }
        } else {
            emit errorOccurred("Unexpected response format");
        }
    }

    m_client.reset();
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
