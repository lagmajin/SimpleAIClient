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
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        onReplyFinished(reply);
    });
}

void ApiClient::onReplyFinished(QNetworkReply *reply)
{
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }

    QByteArray data = reply->readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject obj = doc.object();

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

    reply->deleteLater();
}
