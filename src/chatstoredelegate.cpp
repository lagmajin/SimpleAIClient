// MainWindow's thin delegates to ChatStore.
//
// Kept next to the store rather than in mainwindow.cpp so that everything
// which knows the store exists is in one place; the window itself only ever
// calls these.

#include "mainwindow.h"

#include "chatstore.h"

QString MainWindow::currentChatId() const
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        return QString();
    }
    return m_chatSessions[m_currentChatIndex].id;
}

void MainWindow::saveChatSessions()
{
    m_store->saveSessions();
    m_store->scheduleBackup(currentChatId());
}

void MainWindow::loadChatSessions()
{
    m_store->loadSessions();
    if (!m_chatSessions.isEmpty()) {
        m_currentChatIndex = 0;
    }
}

void MainWindow::saveChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;
    m_store->saveMessages(m_chatSessions[index]);
}

void MainWindow::loadChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;
    m_store->loadMessages(&m_chatSessions[index]);
}

void MainWindow::unloadChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;
    m_store->unloadMessages(&m_chatSessions[index]);
}

void MainWindow::storeDraft(const QString &chatId, const QString &draft)
{
    m_store->saveDraft(chatId, draft);
}

QString MainWindow::storedDraft(const QString &chatId) const
{
    return m_store->loadDraft(chatId);
}

void MainWindow::removeDraft(const QString &chatId)
{
    m_store->clearDraft(chatId);
}

QList<int> MainWindow::recoverableChatIndices() const
{
    return m_store->recoverableChatIndices();
}

bool MainWindow::restoreChatFromBackup(const QString &chatId)
{
    if (!m_store->restoreChat(chatId, m_currentChatIndex)) {
        return false;
    }
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
        rebuildCurrentChatView();
        updateHeaderState();
        updateContextUsage();
        updateChatDuration();
    }
    updateChatList();
    saveChatSessions();
    return true;
}
