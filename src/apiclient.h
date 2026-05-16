#ifndef APICLIENT_H
#define APICLIENT_H

#include <QObject>
#include <QString>
#include <QList>
#include <QThread>
#include <httplib.h>

struct ChatMessage {
    QString role;
    QString content;
    int promptTokens;
    int completionTokens;
    int totalTokens;
    QString imageUrl;
};

class ApiClient : public QObject
{
    Q_OBJECT

public:
    explicit ApiClient(QObject *parent = nullptr);
    void setApiKey(const QString &key);
    void setModel(const QString &model);
    void setStreaming(bool enabled);
    void setSystemPrompt(const QString &prompt);
    void setTemperature(double temp);
    void setMaxTokens(int maxTokens);
    void setWebSearch(bool enabled);
    void sendMessage(const QList<ChatMessage> &messages);
    void fetchModels();

signals:
    void responseReceived(const QString &response, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs);
    void responseChunk(const QString &chunk);
    void responseFinished(int responseTimeMs);
    void errorOccurred(const QString &error);
    void modelsFetched(const QStringList &models);

private:
    QString m_apiKey;
    QString m_model;
    bool m_streaming;
    QString m_systemPrompt;
    double m_temperature;
    int m_maxTokens;
    bool m_webSearch;
};

class ChatRequestWorker : public QObject
{
    Q_OBJECT
public:
    ChatRequestWorker(const QString &apiKey, const QString &model, const QList<ChatMessage> &messages, bool streaming, const QString &systemPrompt, double temperature, int maxTokens, bool webSearch);

public slots:
    void execute();

signals:
    void responseReceived(const QString &response, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs);
    void responseChunk(const QString &chunk);
    void responseFinished(int responseTimeMs);
    void errorOccurred(const QString &error);

private:
    QString parseSSELine(const QString &line);

    QString m_apiKey;
    QString m_model;
    QList<ChatMessage> m_messages;
    bool m_streaming;
    qint64 m_startTime;
    QString m_systemPrompt;
    double m_temperature;
    int m_maxTokens;
    bool m_webSearch;
};

class ModelsRequestWorker : public QObject
{
    Q_OBJECT
public:
    ModelsRequestWorker(const QString &apiKey);

public slots:
    void execute();

signals:
    void modelsFetched(const QStringList &models);
    void errorOccurred(const QString &error);

private:
    QString m_apiKey;
};

#endif // APICLIENT_H
