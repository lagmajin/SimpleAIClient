#include "apiclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QThread>
#include <QDebug>
#include <QDateTime>
#include <QPointer>

namespace {
constexpr int kConnectTimeoutSeconds = 15;
constexpr int kWriteTimeoutSeconds = 30;
constexpr int kReadTimeoutSeconds = 120;
constexpr int kMaxErrorBodyBytes = 64 * 1024;
}

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_model("venice-uncensored")
    , m_streaming(false)
    , m_temperature(0.7)
    , m_maxTokens(0)
    , m_webSearch(false)
    , m_activeRequestThread(nullptr)
    , m_currentRequest(nullptr)
{
}

ApiClient::~ApiClient()
{
    shutdownActiveRequest();
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
    shutdownActiveRequest();

    QThread *thread = new QThread();
    ChatRequestWorker *worker = new ChatRequestWorker(m_apiKey, m_model, messages, m_streaming, m_systemPrompt, m_temperature, m_maxTokens, m_webSearch);
    worker->moveToThread(thread);
    m_activeRequestThread = thread;

    // Stale workers can still be draining their socket: drop everything they
    // emit instead of letting it reach the UI and the chat state.
    //
    // The pointer is captured by value and re-read under a connection to the
    // worker's destroyed() signal rather than compared against
    // m_activeRequestWorker: the worker is deleteLater'd as soon as its thread
    // finishes, so the member would be left dangling and a later
    // shutdownActiveRequest() would call cancel() on freed memory.
    const auto isCurrent = [this, worker]() { return m_currentRequest == worker; };
    m_currentRequest = worker;

    connect(thread, &QThread::started, worker, &ChatRequestWorker::execute);
    connect(worker, &ChatRequestWorker::responseReceived, this, [this, isCurrent](const QString &response, int p, int c, int t, int ms) {
        if (isCurrent()) emit responseReceived(response, p, c, t, ms);
    });
    connect(worker, &ChatRequestWorker::responseChunk, this, [this, isCurrent](const QString &chunk) {
        if (isCurrent()) emit responseChunk(chunk);
    });
    connect(worker, &ChatRequestWorker::responseFinished, this, [this, isCurrent](int ms) {
        if (isCurrent()) emit responseFinished(ms);
    });
    connect(worker, &ChatRequestWorker::requestCancelled, this, [this, isCurrent]() {
        if (isCurrent()) emit requestCancelled();
    });
    connect(worker, &ChatRequestWorker::errorOccurred, this, [this, isCurrent](const QString &error) {
        if (isCurrent()) emit errorOccurred(error);
    });
    connect(worker, &ChatRequestWorker::responseReceived, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::responseFinished, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::requestCancelled, thread, &QThread::quit);
    connect(worker, &ChatRequestWorker::errorOccurred, thread, &QThread::quit);
    connect(thread, &QThread::finished, worker, &QObject::deleteLater);
    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    // Cleared here, in the worker thread, as soon as it is destroyed, so the
    // stored pointer can never outlive the object it refers to.
    connect(worker, &QObject::destroyed, this, [this, worker]() {
        if (m_currentRequest == worker) {
            m_currentRequest = nullptr;
        }
    });

    thread->start();
}

void ApiClient::shutdownActiveRequest()
{
    ChatRequestWorker *worker = m_currentRequest;
    if (!worker) {
        return;
    }

    // cancel() only touches an atomic flag and httplib's own (thread-safe)
    // stop(), so it is called directly: a queued call would sit unprocessed in
    // the worker thread's event queue, which is blocked inside Post().
    //
    // m_currentRequest is cleared afterwards but not before, so the worker's
    // requestCancelled signal is still recognised as current and reaches the
    // UI. Nulling it first would silently drop the signal and leave the window
    // stuck showing an in-flight request forever.
    worker->cancel();
    m_currentRequest = nullptr;
    m_activeRequestThread = nullptr;
}

void ApiClient::cancelCurrentRequest()
{
    shutdownActiveRequest();
}

void ApiClient::fetchModels()
{
    QThread *thread = new QThread();
    ModelsRequestWorker *worker = new ModelsRequestWorker(m_apiKey);
    worker->moveToThread(thread);

    connect(thread, &QThread::started, worker, &ModelsRequestWorker::execute);
    connect(worker, &ModelsRequestWorker::modelsFetched, this, &ApiClient::modelsFetched);
    connect(worker, &ModelsRequestWorker::errorOccurred, this, &ApiClient::modelsFetchFailed);
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

std::shared_ptr<httplib::Client> ChatRequestWorker::takeClientSnapshot()
{
    std::lock_guard<std::mutex> lock(m_clientMutex);
    return m_client;
}

void ChatRequestWorker::cancel()
{
    m_cancelRequested.store(true);
    if (auto client = takeClientSnapshot()) {
        client->stop();
    }
}

QString ChatRequestWorker::buildErrorMessage(int status, const QByteArray &body)
{
    QString errorMsg = QString("HTTP %1").arg(status);

    QJsonParseError parseError;
    const QJsonDocument errDoc = QJsonDocument::fromJson(body, &parseError);
    if (parseError.error == QJsonParseError::NoError && errDoc.isObject()) {
        const QJsonObject root = errDoc.object();

        const QJsonValue errorVal = root.value("error");
        if (errorVal.isString()) {
            return errorVal.toString();
        }
        if (errorVal.isObject()) {
            const QJsonObject errorObj = errorVal.toObject();
            const QString message = errorObj.value("message").toString();
            if (!message.isEmpty()) {
                return message;
            }
        }

        const QJsonValue detailsVal = root.value("details");
        if (detailsVal.isObject()) {
            const QJsonValue errorsVal = detailsVal.toObject().value("_errors");
            if (errorsVal.isArray() && !errorsVal.toArray().isEmpty()) {
                const QJsonValue first = errorsVal.toArray().at(0);
                const QString detail = first.isString() ? first.toString() : first.toObject().value("msg").toString();
                if (!detail.isEmpty()) {
                    return detail;
                }
            }
        }
    }

    if (!body.isEmpty()) {
        errorMsg += ": " + QString::fromUtf8(body.left(kMaxErrorBodyBytes));
    }
    return errorMsg;
}

QString ChatRequestWorker::parseSSELine(const QString &line)
{
    if (!line.startsWith("data:")) {
        return QString();
    }

    QString data = line.mid(5);
    if (data.startsWith(" ")) {
        data = data.mid(1);
    }
    if (data == "[DONE]") {
        return QString();
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (doc.isObject()) {
        const QJsonObject obj = doc.object();
        if (obj.contains("choices")) {
            const QJsonArray choices = obj.value("choices").toArray();
            if (!choices.isEmpty()) {
                const QJsonObject choice = choices.at(0).toObject();
                return choice.value("delta").toObject().value("content").toString();
            }
        }
    }
    return QString();
}

void ChatRequestWorker::execute()
{
    // httplib throws (e.g. std::invalid_argument for unsupported schemes when
    // built without SSL). An exception escaping this worker thread would
    // terminate the whole process; report it as an error instead.
    try {
        executeImpl();
    } catch (const std::exception &e) {
        emit errorOccurred(QString("Request failed: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Request failed: unknown exception");
    }
}

void ChatRequestWorker::executeImpl()
{
    m_startTime = QDateTime::currentMSecsSinceEpoch();

    m_cancelRequested.store(false);
    {
        std::lock_guard<std::mutex> lock(m_clientMutex);
        m_client = std::make_shared<httplib::Client>("https://api.venice.ai");
        m_client->set_follow_location(true);
        m_client->set_connection_timeout(kConnectTimeoutSeconds, 0);
        m_client->set_read_timeout(kReadTimeoutSeconds, 0);
        m_client->set_write_timeout(kWriteTimeoutSeconds, 0);
    }

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

    if (m_temperature >= 0) {
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

    auto client = takeClientSnapshot();
    if (!client) {
        emit errorOccurred("Request failed: no HTTP client");
        return;
    }

    if (m_streaming) {
        // httplib hands over raw socket-read boundaries, not SSE event
        // boundaries, and never fills Result::body when a content receiver is
        // supplied. Keep the unterminated tail across calls so a `data: {...}`
        // line split by a chunk boundary is not silently dropped, and keep any
        // non-SSE payload (the error document) for the error path.
        QString lineBuffer;
        QByteArray nonSseBody;
        bool sawSseData = false;

        const auto handleLine = [&](const QString &rawLine) {
            const QString line = rawLine.trimmed();
            if (line.isEmpty()) {
                return;
            }
            if (!line.startsWith("data:")) {
                if (!sawSseData && nonSseBody.size() < kMaxErrorBodyBytes) {
                    nonSseBody += rawLine.toUtf8();
                    nonSseBody += '\n';
                }
                return;
            }
            sawSseData = true;
            const QString content = parseSSELine(line);
            if (!content.isEmpty()) {
                emit responseChunk(content);
            }
        };

        httplib::Result res = client->Post("/api/v1/chat/completions", headers, body, "application/json",
            [&](const char *data, size_t data_length) {
                if (m_cancelRequested.load()) {
                    return false;
                }
                lineBuffer += QString::fromUtf8(data, static_cast<int>(data_length));
                int newline = lineBuffer.indexOf('\n');
                while (newline >= 0) {
                    handleLine(lineBuffer.left(newline));
                    lineBuffer.remove(0, newline + 1);
                    newline = lineBuffer.indexOf('\n');
                }
                return true;
            });

        if (m_cancelRequested.load()) {
            emit requestCancelled();
            return;
        }

        if (!res) {
            QString errMsg = QString("Request failed: error code %1").arg(static_cast<int>(res.error()));
            qDebug() << "ChatRequestWorker Error:" << errMsg;
            emit errorOccurred(errMsg);
            return;
        }

        if (res->status != 200) {
            if (!lineBuffer.isEmpty()) {
                handleLine(lineBuffer);
            }
            const QByteArray errorBody = nonSseBody.isEmpty() ? QByteArray::fromStdString(res->body) : nonSseBody;
            const QString errorMsg = buildErrorMessage(res->status, errorBody);
            qDebug() << "ChatRequestWorker HTTP Error:" << res->status << errorMsg;
            emit errorOccurred(errorMsg);
            return;
        }

        if (!sawSseData) {
            // A 200 that never produced an SSE event is not a response: a
            // proxy error page, a captive portal, or a JSON error document
            // with a success status. Reporting success would persist an empty
            // assistant message and the turn would be silently lost.
            const QByteArray body = nonSseBody.isEmpty() ? QByteArray::fromStdString(res->body)
                                                          : nonSseBody;
            const QString errorMsg = buildErrorMessage(res->status, body);
            qDebug() << "ChatRequestWorker: no SSE data in a 200 response" << errorMsg;
            emit errorOccurred(errorMsg.isEmpty() ? QString("The server returned no completions")
                                                  : errorMsg);
            return;
        }

        emit responseFinished(static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startTime));
    } else {
        auto res = client->Post("/api/v1/chat/completions", headers, body, "application/json");

        if (m_cancelRequested.load()) {
            emit requestCancelled();
            return;
        }

        if (!res) {
            QString errMsg = QString("Request failed: error code %1").arg(static_cast<int>(res.error()));
            qDebug() << "ChatRequestWorker Error:" << errMsg;
            emit errorOccurred(errMsg);
            return;
        }

        if (res->status != 200) {
            const QString errorMsg = buildErrorMessage(res->status, QByteArray::fromStdString(res->body));
            qDebug() << "ChatRequestWorker HTTP Error:" << res->status << errorMsg;
            emit errorOccurred(errorMsg);
            return;
        }

        QJsonDocument responseDoc = QJsonDocument::fromJson(QByteArray::fromStdString(res->body));
        QJsonObject obj = responseDoc.object();

        if (obj.contains("choices")) {
            QJsonArray choices = obj["choices"].toArray();
            if (choices.isEmpty()) {
                emit errorOccurred("Empty choices in response");
            } else if (obj.contains("error")) {
                // Some gateways answer 200 with an error document.
                emit errorOccurred(buildErrorMessage(res->status, QByteArray::fromStdString(res->body)));
            } else {
                QJsonObject firstChoice = choices[0].toObject();
                QJsonObject message = firstChoice["message"].toObject();
                const QString content = message["content"].toString();

                if (content.trimmed().isEmpty()) {
                    // A tool-call-only or reasoning-only turn has no text.
                    // Persisting an empty assistant message would send it back
                    // to the API on the next request and blank the transcript.
                    emit errorOccurred("The model returned an empty response");
                    return;
                }

                int promptTokens = 0, completionTokens = 0, totalTokens = 0;
                if (obj.contains("usage")) {
                    QJsonObject usage = obj["usage"].toObject();
                    promptTokens = usage["prompt_tokens"].toInt();
                    completionTokens = usage["completion_tokens"].toInt();
                    totalTokens = usage["total_tokens"].toInt();
                }

                int responseTime = static_cast<int>(QDateTime::currentMSecsSinceEpoch() - m_startTime);
                emit responseReceived(content, promptTokens, completionTokens, totalTokens, responseTime);
            }
        } else {
            emit errorOccurred(buildErrorMessage(res->status, QByteArray::fromStdString(res->body)));
        }
    }

    std::lock_guard<std::mutex> lock(m_clientMutex);
    m_client.reset();
}

ModelsRequestWorker::ModelsRequestWorker(const QString &apiKey)
    : m_apiKey(apiKey)
{
}

void ModelsRequestWorker::execute()
{
    // httplib throws (e.g. std::invalid_argument for unsupported schemes when
    // built without SSL). An exception escaping this worker thread would
    // terminate the whole process; report it as an error instead.
    try {
        executeImpl();
    } catch (const std::exception &e) {
        emit errorOccurred(QString("Models request failed: %1").arg(e.what()));
    } catch (...) {
        emit errorOccurred("Models request failed: unknown exception");
    }
}

void ModelsRequestWorker::executeImpl()
{
    httplib::Client cli("https://api.venice.ai");
    cli.set_follow_location(true);
    cli.set_connection_timeout(kConnectTimeoutSeconds, 0);
    cli.set_read_timeout(kReadTimeoutSeconds, 0);
    cli.set_write_timeout(kWriteTimeoutSeconds, 0);

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
