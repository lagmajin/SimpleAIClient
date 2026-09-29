#ifndef CHATSTORE_H
#define CHATSTORE_H

#include <QList>
#include <QObject>
#include <QString>

class QJsonObject;
class QObject;
class QSettings;
class QTimer;

struct ChatSession;
struct ChatMessage;

// Persistence for the chat transcripts: the session index, the per-chat
// message arrays, the drafts, and the recovery snapshot.
//
// Extracted from MainWindow because it is a self-contained concern that only
// touches QSettings, the session list and the encrypted-at-rest store. Every
// value it writes is sealed with DPAPI, so neither the registry nor the backup
// file holds readable conversation text.
//
// The store does not own the session list: the window keeps it, and the store
// reads and writes it in place, so a chat that is open in the UI and the copy
// on disk can never drift apart.
class ChatStore
{
public:
    explicit ChatStore(QSettings *settings, QList<ChatSession> *sessions);
    ~ChatStore();

    // Writes the session index and coalesces the backup rewrite, which
    // re-serialises every transcript and is far too slow to run per turn.
    void saveSessions();
    // Replaces the index in place. Returns true when legacy inline messages
    // were migrated out of the index into the per-chat keys.
    bool loadSessions();

    void saveMessages(const ChatSession &chat);
    // Loads the chat's messages in place. No-op when already loaded.
    void loadMessages(ChatSession *chat);
    // Drops the in-memory copy so a long session does not keep every
    // conversation resident.
    void unloadMessages(ChatSession *chat);

    // Drafts are per chat, so switching away and back does not lose text.
    void saveDraft(const QString &chatId, const QString &draft);
    QString loadDraft(const QString &chatId) const;
    void clearDraft(const QString &chatId);

    // Recovery snapshot. flushBackup() forces a pending write, which the
    // window does on quit.
    void scheduleBackup(const QString &currentChatId);
    void flushBackup();
    bool loadBackup(QJsonObject *snapshot) const;
    // The snapshot as plain JSON, for the explicit "Export Backup Snapshot",
    // which is deliberately readable rather than sealed.
    QJsonObject exportSnapshot(const QString &currentChatId) const;
    // Indices of sessions the snapshot can restore into.
    QList<int> recoverableChatIndices() const;
    // Overwrites one chat from the snapshot. Returns false if the id is not in
    // the snapshot. `currentIndex` selects which session is refreshed in the UI.
    bool restoreChat(const QString &chatId, int currentIndex);

    void removeChatData(const QString &chatId);

private:
    static QString backupFilePath();
    QJsonObject buildSnapshot(const QString &currentChatId) const;
    QJsonObject previousSnapshotSession(const QString &chatId) const;
    QByteArray readMessages(const QString &chatId) const;

    QSettings *m_settings;
    QList<ChatSession> *m_sessions;
    QTimer *m_backupTimer;
    QString m_currentChatId;
};

#endif // CHATSTORE_H
