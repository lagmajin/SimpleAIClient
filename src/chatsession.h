#ifndef CHATSESSION_H
#define CHATSESSION_H

#include <QList>
#include <QString>

#include "apiclient.h"

// Sending an unbounded paste makes the API reject the turn with a 400 and the
// markdown renderer stall on the huge body, so the composer is capped.
constexpr int kMaxInputChars = 8000;

// One conversation. The messages are unloaded lazily: only the sessions that
// have been opened keep theirs in memory, and the rest live in QSettings.
struct ChatSession {
    QString id;
    QString title;
    QList<ChatMessage> messages;
    int messageCount = 0;
    bool pinned;
    int scrollPosition = 0;
    bool messagesLoaded = false;
};

// A named set of API credentials and generation parameters.
struct ApiProfile {
    QString name;
    QString apiKey;
    QString model;
    QString systemPrompt;
    double temperature;
    int maxTokens;
};

#endif // CHATSESSION_H
