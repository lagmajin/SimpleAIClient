// Persistence for the chat transcripts: the session index, the per-chat
// message arrays, the drafts, and the recovery snapshot.
//
// Extracted from MainWindow because it is a self-contained concern that only
// touches QSettings, the session list and SecretStore. Everything here is
// sealed with DPAPI, so the registry and the backup file hold no readable
// conversation text.

#include "chatstore.h"

#include "chatsession.h"
#include "secretstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QSaveFile>
#include <QSettings>
#include <QStandardPaths>
#include <QTimer>

ChatStore::ChatStore(QSettings *settings, QList<ChatSession> *sessions)
    : m_settings(settings)
    , m_sessions(sessions)
    , m_backupTimer(nullptr)
{
}

ChatStore::~ChatStore()
{
    delete m_backupTimer;
}

QString ChatStore::backupFilePath()
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.simpleaiclient";
    }
    return QDir(baseDir).filePath("backups/chat-backup.json");
}

QByteArray ChatStore::readMessages(const QString &chatId) const
{
    const QByteArray stored = QByteArray::fromBase64(
        m_settings->value(QString("chatMessages/%1").arg(chatId)).toString().toLatin1());
    return SecretStore::unprotectBytes(stored);
}

QJsonObject ChatStore::previousSnapshotSession(const QString &chatId) const
{
    QJsonObject snapshot;
    if (!loadBackup(&snapshot)) {
        return QJsonObject();
    }
    for (const auto &val : snapshot["sessions"].toArray()) {
        const QJsonObject session = val.toObject();
        if (session["id"].toString() == chatId && !session["messages"].toArray().isEmpty()) {
            return session;
        }
    }
    return QJsonObject();
}

QJsonObject ChatStore::buildSnapshot(const QString &currentChatId) const
{
    QJsonArray sessionsArray;

    for (const ChatSession &chat : *m_sessions) {
        QJsonArray messagesArray;

        if (chat.messagesLoaded) {
            for (const ChatMessage &msg : chat.messages) {
                QJsonObject msgObj;
                msgObj["role"] = msg.role;
                msgObj["content"] = msg.content;
                msgObj["promptTokens"] = msg.promptTokens;
                msgObj["completionTokens"] = msg.completionTokens;
                msgObj["totalTokens"] = msg.totalTokens;
                msgObj["imageUrl"] = msg.imageUrl;
                messagesArray.append(msgObj);
            }
        } else {
            const QByteArray data = readMessages(chat.id);
            if (!data.isEmpty()) {
                const QJsonDocument doc = QJsonDocument::fromJson(data);
                if (doc.isArray()) {
                    messagesArray = doc.array();
                }
            }
            if (messagesArray.isEmpty() && chat.messageCount > 0) {
                // The transcript is known to exist but could not be read back:
                // a truncated registry value (a chat with an attached image
                // runs to megabytes) or a DPAPI failure. Writing an empty
                // array here would replace the last good copy in the snapshot
                // file, and recoverableChatIndices() would then never offer the
                // chat, so the previous snapshot's entry is kept instead.
                const QJsonObject previous = previousSnapshotSession(chat.id);
                if (!previous.isEmpty()) {
                    sessionsArray.append(previous);
                    continue;
                }
                qWarning("ChatStore: dropping unreadable chat %s from the backup",
                         qPrintable(chat.id));
            }
        }

        QJsonObject sessionObj;
        sessionObj["id"] = chat.id;
        sessionObj["title"] = chat.title;
        sessionObj["pinned"] = chat.pinned;
        sessionObj["scrollPosition"] = chat.scrollPosition;
        sessionObj["messageCount"] = messagesArray.size();
        sessionObj["messages"] = messagesArray;
        sessionsArray.append(sessionObj);
    }

    QJsonObject snapshot;
    snapshot["version"] = 1;
    snapshot["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    snapshot["currentChatId"] = currentChatId;
    snapshot["sessions"] = sessionsArray;
    return snapshot;
}

bool ChatStore::loadBackup(QJsonObject *snapshot) const
{
    if (!snapshot) {
        return false;
    }

    QFile file(backupFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QByteArray raw = file.readAll();
    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error == QJsonParseError::NoError && doc.isObject()) {
        // Legacy plain-text snapshot written before the backup was encrypted.
        *snapshot = doc.object();
        return true;
    }

    const QByteArray decoded = SecretStore::unprotectBytes(raw);
    if (decoded.isEmpty()) {
        return false;
    }
    const QJsonDocument decrypted = QJsonDocument::fromJson(decoded, &err);
    if (err.error != QJsonParseError::NoError || !decrypted.isObject()) {
        return false;
    }

    *snapshot = decrypted.object();
    return true;
}

void ChatStore::flushBackup()
{
    const QJsonDocument doc(buildSnapshot(m_currentChatId));

    const QString path = backupFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    // The snapshot is a full copy of every transcript, so it gets the same
    // protection as the registry entries. protectBytes() adds the "enc:v1:"
    // tag itself, so the payload must be the bare JSON; prefixing it here would
    // leave the sealed blob with a second tag inside and loadBackup() could
    // never parse it back.
    file.write(SecretStore::protectBytes(doc.toJson(QJsonDocument::Compact)));
    file.commit();
}

QJsonObject ChatStore::exportSnapshot(const QString &currentChatId) const
{
    return buildSnapshot(currentChatId);
}

void ChatStore::scheduleBackup(const QString &currentChatId)
{
    m_currentChatId = currentChatId;
    if (!m_backupTimer) {
        // The store is not a QObject, so the connection is made with an
        // explicit context object that owns the timer. Qualified because
        // httplib.h declares a WinSock connect() that would otherwise win
        // overload resolution.
        ChatStore *self = this;
        m_backupTimer = new QTimer;
        m_backupTimer->setSingleShot(true);
        m_backupTimer->setInterval(400);
        QObject::connect(m_backupTimer, &QTimer::timeout, m_backupTimer, [self]() { self->flushBackup(); });
    }
    m_backupTimer->start();
}

QList<int> ChatStore::recoverableChatIndices() const
{
    QList<int> indices;

    QJsonObject snapshot;
    if (!loadBackup(&snapshot)) {
        return indices;
    }

    QMap<QString, int> backupMessageCounts;
    for (const auto &val : snapshot["sessions"].toArray()) {
        const QJsonObject sessionObj = val.toObject();
        const QString id = sessionObj["id"].toString();
        const int messageCount = sessionObj["messageCount"].toInt();
        if (!id.isEmpty() && messageCount > 0) {
            backupMessageCounts[id] = messageCount;
        }
    }

    for (int i = 0; i < m_sessions->size(); ++i) {
        const ChatSession &chat = m_sessions->at(i);
        if (chat.messageCount > 0) {
            continue;
        }
        if (backupMessageCounts.contains(chat.id)) {
            indices.append(i);
        }
    }

    return indices;
}

bool ChatStore::restoreChat(const QString &chatId, int currentIndex)
{
    if (chatId.isEmpty()) {
        return false;
    }

    QJsonObject snapshot;
    if (!loadBackup(&snapshot)) {
        return false;
    }

    QJsonArray messagesArray;
    QString title;
    int scrollPosition = 0;
    bool pinned = false;
    bool found = false;

    for (const auto &val : snapshot["sessions"].toArray()) {
        const QJsonObject sessionObj = val.toObject();
        if (sessionObj["id"].toString() != chatId) {
            continue;
        }

        messagesArray = sessionObj["messages"].toArray();
        title = sessionObj["title"].toString();
        scrollPosition = sessionObj["scrollPosition"].toInt();
        pinned = sessionObj["pinned"].toBool();
        found = true;
        break;
    }

    if (!found || messagesArray.isEmpty()) {
        return false;
    }

    const QByteArray json = QJsonDocument(messagesArray).toJson(QJsonDocument::Compact);
    m_settings->setValue(QString("chatMessages/%1").arg(chatId),
                         SecretStore::protectBytes(json).toBase64());

    for (ChatSession &chat : *m_sessions) {
        if (chat.id != chatId) {
            continue;
        }

        chat.title = title.isEmpty() ? chat.title : title;
        chat.pinned = pinned;
        chat.scrollPosition = scrollPosition;
        chat.messageCount = messagesArray.size();
        chat.messagesLoaded = false;
        if (&chat == &m_sessions->at(currentIndex)) {
            loadMessages(&chat);
        }
        break;
    }

    return true;
}

void ChatStore::removeChatData(const QString &chatId)
{
    m_settings->remove("chatMessages/" + chatId);
    m_settings->remove("draft_" + chatId);
}

void ChatStore::saveSessions()
{
    QJsonArray sessionsArray;
    for (ChatSession &chat : *m_sessions) {
        if (chat.messagesLoaded) {
            chat.messageCount = chat.messages.size();
            saveMessages(chat);
        }

        QJsonObject sessionObj;
        sessionObj["id"] = chat.id;
        sessionObj["title"] = chat.title;
        sessionObj["pinned"] = chat.pinned;
        sessionObj["scrollPosition"] = chat.scrollPosition;
        sessionObj["messageCount"] = chat.messageCount;
        sessionsArray.append(sessionObj);
    }

    const QJsonDocument doc(sessionsArray);
    m_settings->setValue("chatSessions",
                          SecretStore::protect(QString::fromUtf8(doc.toJson(QJsonDocument::Compact))));

    // The backup re-serialises every chat, so coalesce the writes that a single
    // turn produces (send + finish, plus a retry) into one.
    scheduleBackup(m_currentChatId);
}

bool ChatStore::loadSessions()
{
    // The index holds chat titles, which are derived from the first user
    // message, so it is encrypted alongside the transcripts.
    const QString data = SecretStore::unprotect(m_settings->value("chatSessions").toString());
    if (data.isEmpty()) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isArray()) {
        return false;
    }

    m_sessions->clear();
    bool needsMigration = false;
    for (const auto &val : doc.array()) {
        const QJsonObject sessionObj = val.toObject();
        ChatSession chat;
        chat.id = sessionObj["id"].toString();
        chat.title = sessionObj["title"].toString();
        chat.pinned = sessionObj["pinned"].toBool();
        chat.scrollPosition = sessionObj["scrollPosition"].toInt();
        chat.messageCount = sessionObj["messageCount"].toInt();
        chat.messagesLoaded = false;

        const QJsonArray messagesArray = sessionObj["messages"].toArray();
        if (!messagesArray.isEmpty()) {
            needsMigration = true;
            for (const auto &msgVal : messagesArray) {
                const QJsonObject msgObj = msgVal.toObject();
                chat.messages.append({msgObj["role"].toString(), msgObj["content"].toString(), msgObj["promptTokens"].toInt(), msgObj["completionTokens"].toInt(), msgObj["totalTokens"].toInt(), msgObj["imageUrl"].toString()});
            }
            chat.messageCount = chat.messages.size();
            chat.messagesLoaded = true;
        }
        m_sessions->append(chat);
    }

    if (needsMigration) {
        saveSessions();
    }
    return needsMigration;
}
void ChatStore::saveMessages(const ChatSession &chat)
{
    QJsonArray messagesArray;
    for (const ChatMessage &msg : chat.messages) {
        QJsonObject msgObj;
        msgObj["role"] = msg.role;
        msgObj["content"] = msg.content;
        msgObj["promptTokens"] = msg.promptTokens;
        msgObj["completionTokens"] = msg.completionTokens;
        msgObj["totalTokens"] = msg.totalTokens;
        msgObj["imageUrl"] = msg.imageUrl;
        messagesArray.append(msgObj);
    }

    const QByteArray json = QJsonDocument(messagesArray).toJson(QJsonDocument::Compact);
    m_settings->setValue(QString("chatMessages/%1").arg(chat.id),
                         SecretStore::protectBytes(json).toBase64());
}

void ChatStore::loadMessages(ChatSession *chat)
{
    if (!chat || chat->messagesLoaded) {
        return;
    }

    chat->messages.clear();
    const QByteArray data = readMessages(chat->id);
    if (!data.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            for (const auto &msgVal : doc.array()) {
                const QJsonObject msgObj = msgVal.toObject();
                chat->messages.append({msgObj["role"].toString(), msgObj["content"].toString(), msgObj["promptTokens"].toInt(), msgObj["completionTokens"].toInt(), msgObj["totalTokens"].toInt(), msgObj["imageUrl"].toString()});
            }
        }
    }
    chat->messageCount = chat->messages.size();
    chat->messagesLoaded = true;
}

void ChatStore::unloadMessages(ChatSession *chat)
{
    if (!chat || !chat->messagesLoaded) {
        return;
    }
    chat->messageCount = chat->messages.size();
    chat->messages.clear();
    chat->messagesLoaded = false;
}

void ChatStore::saveDraft(const QString &chatId, const QString &draft)
{
    m_settings->setValue(QString("draft_%1").arg(chatId), SecretStore::protect(draft));
}

QString ChatStore::loadDraft(const QString &chatId) const
{
    return SecretStore::unprotect(m_settings->value(QString("draft_%1").arg(chatId)).toString());
}

void ChatStore::clearDraft(const QString &chatId)
{
    m_settings->remove(QString("draft_%1").arg(chatId));
}
