#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonArray>

struct ChatMessage {
    QString role;
    QString content;
};

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);
    void setApiKey(const QString &key);
    void setModel(const QString &model);
    void sendMessage(const QList<ChatMessage> &messages);
    void fetchModels();

signals:
    void responseReceived(const QString &response);
    void errorOccurred(const QString &error);
    void modelsFetched(const QStringList &models);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_model;
    enum class RequestType { Chat, Models };
    void attachRequestType(QNetworkReply *reply, RequestType type);
    RequestType getRequestType(QNetworkReply *reply) const;
};

#endif // APICLIENT_H
