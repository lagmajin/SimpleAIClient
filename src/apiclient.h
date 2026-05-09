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

signals:
    void responseReceived(const QString &response);
    void errorOccurred(const QString &error);

private slots:
    void onReplyFinished(QNetworkReply *reply);

private:
    QNetworkAccessManager *m_networkManager;
    QString m_apiKey;
    QString m_model;
};

#endif // APICLIENT_H
