// Persistence for the chat transcripts: the session index, the per-chat
// message arrays, the drafts, and the recovery snapshot.
//
// Split out of mainwindow.cpp because it is a self-contained concern that
// only touches QSettings, SecretStore and the session list. Everything here
// is sealed with DPAPI, so the registry and the backup file hold no readable
// conversation text.

#include "mainwindow.h"
#include "secretstore.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSaveFile>
#include <QStandardPaths>
QString MainWindow::chatBackupFilePath() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.simpleaiclient";
    }
    return QDir(baseDir).filePath("backups/chat-backup.json");
}

QJsonObject MainWindow::buildChatBackupSnapshot() const
{
    QJsonArray sessionsArray;

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        const ChatSession &chat = m_chatSessions[i];
        QJsonArray messagesArray;

        if (chat.messagesLoaded) {
            for (const auto &msg : chat.messages) {
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
            const QByteArray stored = QByteArray::fromBase64(
                m_settings.value(QString("chatMessages/%1").arg(chat.id)).toString().toLatin1());
            const QByteArray data = SecretStore::unprotectBytes(stored);
            if (!data.isEmpty()) {
                QJsonDocument doc = QJsonDocument::fromJson(data);
                if (doc.isArray()) {
                    messagesArray = doc.array();
                }
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
    snapshot["currentChatId"] = (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size())
        ? m_chatSessions[m_currentChatIndex].id
        : QString();
    snapshot["sessions"] = sessionsArray;
    return snapshot;
}

bool MainWindow::loadChatBackupSnapshot(QJsonObject *snapshot) const
{
    if (!snapshot) {
        return false;
    }

    QFile file(chatBackupFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QByteArray raw = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
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

bool MainWindow::saveChatBackup()
{
    QJsonObject snapshot = buildChatBackupSnapshot();
    QJsonDocument doc(snapshot);

    const QString path = chatBackupFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    // The snapshot is a full copy of every transcript, so it gets the same
    // protection as the registry entries. An unwrapped file from an older
    // build is still accepted by loadChatBackupSnapshot().
    // protectBytes() adds the "enc:v1:" tag itself, so the payload must be the
    // bare JSON. Prefixing it here would leave the sealed blob with a second
    // tag inside, and loadChatBackupSnapshot() could never parse it back.
    file.write(SecretStore::protectBytes(doc.toJson(QJsonDocument::Compact)));
    if (!file.commit()) {
        return false;
    }

    return true;
}

QList<int> MainWindow::recoverableChatIndices() const
{
    QList<int> indices;

    QJsonObject snapshot;
    if (!loadChatBackupSnapshot(&snapshot)) {
        return indices;
    }

    QMap<QString, int> backupMessageCounts;
    QJsonArray sessionsArray = snapshot["sessions"].toArray();
    for (const auto &val : sessionsArray) {
        QJsonObject sessionObj = val.toObject();
        const QString id = sessionObj["id"].toString();
        const int messageCount = sessionObj["messageCount"].toInt();
        if (!id.isEmpty() && messageCount > 0) {
            backupMessageCounts[id] = messageCount;
        }
    }

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        const ChatSession &chat = m_chatSessions[i];
        if (chat.messageCount > 0) {
            continue;
        }
        if (backupMessageCounts.contains(chat.id)) {
            indices.append(i);
        }
    }

    return indices;
}

bool MainWindow::restoreChatFromBackup(const QString &chatId)
{
    if (chatId.isEmpty()) {
        return false;
    }

    QJsonObject snapshot;
    if (!loadChatBackupSnapshot(&snapshot)) {
        return false;
    }

    QJsonArray sessionsArray = snapshot["sessions"].toArray();
    QJsonArray messagesArray;
    QString title;
    int scrollPosition = 0;
    bool pinned = false;
    bool found = false;

    for (const auto &val : sessionsArray) {
        QJsonObject sessionObj = val.toObject();
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

    const QString messagesKey = QString("chatMessages/%1").arg(chatId);
    const QByteArray json = QJsonDocument(messagesArray).toJson(QJsonDocument::Compact);
    m_settings.setValue(messagesKey, SecretStore::protectBytes(json).toBase64());

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        if (m_chatSessions[i].id != chatId) {
            continue;
        }

        m_chatSessions[i].title = title.isEmpty() ? m_chatSessions[i].title : title;
        m_chatSessions[i].pinned = pinned;
        m_chatSessions[i].scrollPosition = scrollPosition;
        m_chatSessions[i].messageCount = messagesArray.size();
        m_chatSessions[i].messagesLoaded = false;
        if (i == m_currentChatIndex) {
            loadChatMessages(i);
            rebuildCurrentChatView();
            updateHeaderState();
            updateContextUsage();
            updateChatDuration();
        }
        break;
    }

    saveChatSessions();
    updateChatList();
    return true;
}

void MainWindow::saveChatSessions()
{
    QJsonArray sessionsArray;
    for (int i = 0; i < m_chatSessions.size(); ++i) {
        auto &chat = m_chatSessions[i];
        if (chat.messagesLoaded) {
            chat.messageCount = chat.messages.size();
            saveChatMessages(i);
        }

        QJsonObject sessionObj;
        sessionObj["id"] = chat.id;
        sessionObj["title"] = chat.title;
        sessionObj["pinned"] = chat.pinned;
        sessionObj["scrollPosition"] = chat.scrollPosition;
        sessionObj["messageCount"] = chat.messageCount;
        sessionsArray.append(sessionObj);
    }

    QJsonDocument doc(sessionsArray);
    m_settings.setValue("chatSessions",
                        SecretStore::protect(QString::fromUtf8(doc.toJson(QJsonDocument::Compact))));

    // The backup re-serialises every chat, so coalesce the writes that a single
    // turn produces (send + finish, plus a retry) into one.
    if (!m_backupTimer) {
        m_backupTimer = new QTimer(this);
        m_backupTimer->setSingleShot(true);
        m_backupTimer->setInterval(400);
        connect(m_backupTimer, &QTimer::timeout, this, [this]() { saveChatBackup(); });
    }
    m_backupTimer->start();
}

void MainWindow::loadChatSessions()
{
    // The index holds chat titles, which are derived from the first user
    // message, so it is encrypted alongside the transcripts.
    QString data = SecretStore::unprotect(m_settings.value("chatSessions").toString());
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isArray()) return;

    m_chatSessions.clear();
    bool needsMigration = false;
    for (const auto &val : doc.array()) {
        QJsonObject sessionObj = val.toObject();
        ChatSession chat;
        chat.id = sessionObj["id"].toString();
        chat.title = sessionObj["title"].toString();
        chat.pinned = sessionObj["pinned"].toBool();
        chat.scrollPosition = sessionObj["scrollPosition"].toInt();
        chat.messageCount = sessionObj["messageCount"].toInt();
        chat.messagesLoaded = false;

        QJsonArray messagesArray = sessionObj["messages"].toArray();
        if (!messagesArray.isEmpty()) {
            needsMigration = true;
            for (const auto &msgVal : messagesArray) {
                QJsonObject msgObj = msgVal.toObject();
                chat.messages.append({msgObj["role"].toString(), msgObj["content"].toString(), msgObj["promptTokens"].toInt(), msgObj["completionTokens"].toInt(), msgObj["totalTokens"].toInt(), msgObj["imageUrl"].toString()});
            }
            chat.messageCount = chat.messages.size();
            chat.messagesLoaded = true;
        }
        m_chatSessions.append(chat);
    }

    if (!m_chatSessions.isEmpty()) {
        m_currentChatIndex = 0;
    }

    if (needsMigration) {
        saveChatSessions();
    }
}

void MainWindow::saveChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;

    const auto &chat = m_chatSessions[index];
    QJsonArray messagesArray;
    for (const auto &msg : chat.messages) {
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
    m_settings.setValue(QString("chatMessages/%1").arg(chat.id),
                        SecretStore::protectBytes(json).toBase64());
}

void MainWindow::loadChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;

    auto &chat = m_chatSessions[index];
    if (chat.messagesLoaded) return;

    chat.messages.clear();
    const QByteArray stored = QByteArray::fromBase64(
        m_settings.value(QString("chatMessages/%1").arg(chat.id)).toString().toLatin1());
    const QByteArray data = SecretStore::unprotectBytes(stored);
    if (!data.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(data);
        if (doc.isArray()) {
            for (const auto &msgVal : doc.array()) {
                QJsonObject msgObj = msgVal.toObject();
                chat.messages.append({msgObj["role"].toString(), msgObj["content"].toString(), msgObj["promptTokens"].toInt(), msgObj["completionTokens"].toInt(), msgObj["totalTokens"].toInt(), msgObj["imageUrl"].toString()});
            }
        }
    }
    chat.messageCount = chat.messages.size();
    chat.messagesLoaded = true;
}

void MainWindow::unloadChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;
    auto &chat = m_chatSessions[index];
    if (!chat.messagesLoaded) return;

    chat.messageCount = chat.messages.size();
    chat.messages.clear();
    chat.messagesLoaded = false;
}
