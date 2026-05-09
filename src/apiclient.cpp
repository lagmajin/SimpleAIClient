#include "apiclient.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkRequest>

ApiClient::ApiClient(QObject *parent)
    : QObject(parent)
    , m_networkManager(new QNetworkAccessManager(this))
    , m_model("venice-uncensored")
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

void ApiClient::fetchModels()
{
    if (m_apiKey.isEmpty()) return;

    QUrl url("https://api.venice.ai/api/v1/models");
    QNetworkRequest request(url);
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QNetworkReply *reply = m_networkManager->get(request);
    attachRequestType(reply, RequestType::Models);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void ApiClient::sendMessage(const QList<ChatMessage> &messages)
{
    QUrl url("https://api.venice.ai/api/v1/chat/completions");
    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", QString("Bearer %1").arg(m_apiKey).toUtf8());

    QJsonObject payload;
    payload["model"] = m_model;
    payload["no_filters"] = true;

    QJsonArray jsonMessages;
    for (const auto &msg : messages) {
        QJsonObject jsonMsg;
        jsonMsg["role"] = msg.role;
        jsonMsg["content"] = msg.content;
        jsonMessages.append(jsonMsg);
    }
    payload["messages"] = jsonMessages;

    QJsonDocument doc(payload);
    QNetworkReply *reply = m_networkManager->post(request, doc.toJson());
    attachRequestType(reply, RequestType::Chat);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void ApiClient::attachRequestType(QNetworkReply *reply, RequestType type)
{
    reply->setProperty("requestType", static_cast<int>(type));
}

ApiClient::RequestType ApiClient::getRequestType(QNetworkReply *reply) const
{
    return static_cast<RequestType>(reply->property("requestType").toInt());
}

void ApiClient::onReplyFinished(QNetworkReply *reply)
{
    RequestType reqType = getRequestType(reply);

    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

    if (reqType == RequestType::Models) {
        if (obj.contains("data")) {
            QJsonArray dataArray = obj["data"].toArray();
            QStringList modelList;
            for (const auto &item : dataArray) {
                modelList.append(item.toObject()["id"].toString());
            }
            emit modelsFetched(modelList);
        }
    } else if (reqType == RequestType::Chat) {
        if (obj.contains("choices")) {
            QJsonArray choices = obj["choices"].toArray();
            if (!choices.isEmpty()) {
                QJsonObject firstChoice = choices[0].toObject();
                QJsonObject message = firstChoice["message"].toObject();
                emit responseReceived(message["content"].toString());
            }
        } else if (obj.contains("error")) {
            emit errorOccurred(obj["error"].toObject()["message"].toString());
        } else {
            emit errorOccurred("Unexpected response format");
        }
    }

    reply->deleteLater();
}
