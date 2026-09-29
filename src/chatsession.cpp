#include "mainwindow.h"
#include "chatstore.h"
#include "appicons.h"
#include "chatwidgets.h"


// Chat sessions: creating, switching, deleting and listing chats in the
// sidebar, plus the per-chat draft and the header subtitle. The sidebar
// rows are ChatListItem widgets, so this is the only place that knows
// about them.

#include <QElapsedTimer>
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollBar>






int MainWindow::chatIndexForId(const QString &id) const
{
    if (id.isEmpty()) {
        return -1;
    }

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        if (m_chatSessions[i].id == id) {
            return i;
        }
    }

    return -1;
}

void MainWindow::restoreLastChatSelection()
{
    if (m_chatSessions.isEmpty()) {
        createNewChat();
        return;
    }

    const QString lastChatId = m_settings.value("lastChatId").toString();
    int index = chatIndexForId(lastChatId);
    if (index < 0) {
        index = 0;
    }

    switchToChat(index);
    updateChatList();
}

void MainWindow::createNewChat()
{
    ChatSession newChat;
    newChat.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    newChat.title = "New Chat";
    newChat.messageCount = 0;
    newChat.pinned = false;
    newChat.messagesLoaded = true;
    m_chatSessions.prepend(newChat);
    // prepending shifts every session index, so an in-flight request would end
    // up naming a different chat than the one it was sent from.
    if (m_requestChatIndex >= 0) {
        m_requestChatIndex++;
    }
    m_currentChatIndex = 0;
    pointViewAtCurrentChat();
    clearChatDisplay();
    showWelcomeScreen();
    m_inputField->clear();
    m_currentChatImage.clear();
    if (m_imagePreview) {
        m_imagePreview->clear();
        m_imagePreview->setVisible(false);
    }
    updateCharCounter();
    updateChatList();
    saveChatSessions();
    m_settings.setValue("lastChatId", newChat.id);
    updateHeaderState();
}

void MainWindow::switchToChat(int index, bool force)
{
    if (index < 0 || index >= m_chatSessions.size()) return;
    // A migrated session arrives with its messages already in memory and
    // m_currentChatIndex already pointing at it, so the early return would
    // leave the transcript unbuilt and the welcome screen on top of it.
    if (!force && index == m_currentChatIndex && m_chatSessions[index].messagesLoaded
        && !m_chatSessions[index].messages.isEmpty()) {
        return;
    }

    const QString currentDraft = m_inputField->toPlainText().trimmed();
    const bool discardDraft = index != m_currentChatIndex && !currentDraft.isEmpty();
    if (discardDraft) {
        QMessageBox::StandardButton result = QMessageBox::question(this, "Unsaved Draft",
            "You have an unsaved draft. Discard it?",
            QMessageBox::Discard | QMessageBox::Cancel);
        if (result == QMessageBox::Cancel) return;
    }

    if (discardDraft) {
        clearDraft();
        m_inputField->clear();
    } else {
        saveDraft();
    }
    // A pending attachment belongs to the chat it was added in.
    m_currentChatImage.clear();
    if (m_imagePreview) {
        m_imagePreview->clear();
        m_imagePreview->setVisible(false);
    }
    saveCurrentChatScrollPosition();
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size() &&
        m_currentChatIndex != index && m_chatSessions[m_currentChatIndex].messagesLoaded) {
        saveChatMessages(m_currentChatIndex);
        // The chat the in-flight request belongs to must stay loaded, otherwise
        // its messages list is cleared and the answer has nowhere to land.
        if (m_currentChatIndex != m_requestChatIndex) {
            unloadChatMessages(m_currentChatIndex);
        }
    }

    m_currentChatIndex = index;
    pointViewAtCurrentChat();
    loadChatMessages(index);
    rebuildCurrentChatView();
    m_settings.setValue("lastChatId", m_chatSessions[index].id);

    loadDraft();
    updateHeaderState();
    updateContextUsage();
    updateChatDuration();
    // Repaint the highlight on the open row without rebuilding the list, which
    // would reset its scroll position and re-run the batches.
    if (m_sessionList) {
        m_sessionList->setCurrentIndex(index);
    }
}

void MainWindow::updateChatList()
{
    if (m_sessionList) {
        m_sessionList->refresh();
    }
}

void MainWindow::filterChats(const QString &query)
{
    if (m_sessionList) {
        m_sessionList->filter(query);
    }
}

QString MainWindow::generateChatTitle(const QString &firstMessage)
{
    QString title = firstMessage.simplified();
    if (title.isEmpty()) {
        title = "New Chat";
    }
    if (title.length() > 30) {
        title = title.left(30) + "...";
    }
    return title;
}

void MainWindow::onNewChat()
{
    saveDraft();
    saveCurrentChatScrollPosition();
    createNewChat();
    m_inputField->setFocus();
}

void MainWindow::deleteChatAtRow(int row)
{
    if (row < 0 || row >= m_chatSessions.size()) return;

    if (QMessageBox::question(this, "Delete Chat",
            QString("Delete \"%1\" and its messages? This cannot be undone.").arg(m_chatSessions[row].title),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // The chat an in-flight request belongs to cannot disappear: cancel first,
    // otherwise the answer would be written into an unrelated chat.
    if (m_requestInFlight && m_requestChatIndex == row) {
        m_apiClient->cancelCurrentRequest();
        m_requestChatIndex = -1;
    } else if (m_requestChatIndex > row) {
        m_requestChatIndex--;
    }

    const QString removedId = m_chatSessions[row].id;
    m_store->removeChatData(removedId);

    if (m_chatSessions.size() == 1) {
        m_chatSessions.clear();
        m_currentChatIndex = -1;
        clearChatDisplay();
        createNewChat();
    } else {
        // The scroll position has to be flushed while m_currentChatIndex still
        // refers to the chat being removed, otherwise the chat that slides
        // into this index inherits its position.
        if (m_currentChatIndex == row) {
            saveCurrentChatScrollPosition();
        }
        m_chatSessions.removeAt(row);
        if (m_currentChatIndex == row) {
            if (m_currentChatIndex >= m_chatSessions.size()) {
                m_currentChatIndex = m_chatSessions.size() - 1;
            }
            // The index now refers to a different chat, so the "same chat"
            // early return must not skip the rebuild.
            switchToChat(m_currentChatIndex, true);
        } else if (m_currentChatIndex > row) {
            m_currentChatIndex--;
        }
    }

    updateChatList();
    saveChatSessions();
}

void MainWindow::renameChat(int row)
{
    if (row < 0 || row >= m_chatSessions.size()) return;

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Chat", "New name:", QLineEdit::Normal, m_chatSessions[row].title, &ok);
    if (ok && !newName.isEmpty()) {
        m_chatSessions[row].title = newName;
        updateChatList();
        saveChatSessions();
        if (row == m_currentChatIndex) {
            updateHeaderState();
        }
    }
}

void MainWindow::togglePinChat(int row)
{
    if (row < 0 || row >= m_chatSessions.size()) return;

    m_chatSessions[row].pinned = !m_chatSessions[row].pinned;
    updateChatList();
    saveChatSessions();
}

void MainWindow::onDeleteChat()
{
    if (m_currentChatIndex < 0 || m_chatSessions.isEmpty()) return;
    deleteChatAtRow(m_currentChatIndex);
}

void MainWindow::saveDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    storeDraft(m_chatSessions[m_currentChatIndex].id, m_inputField->toPlainText());
}

void MainWindow::loadDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    const QString draft = storedDraft(m_chatSessions[m_currentChatIndex].id);
    m_inputField->blockSignals(true);
    m_inputField->setPlainText(draft);
    m_inputField->blockSignals(false);
    updateCharCounter();
}

void MainWindow::clearDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    removeDraft(m_chatSessions[m_currentChatIndex].id);
    m_inputField->blockSignals(true);
    m_inputField->clear();
    m_inputField->blockSignals(false);
    updateCharCounter();
}

void MainWindow::updateHeaderState()
{
    if (!m_headerTitle || !m_headerSubtitle) {
        return;
    }

    QString title = "New Chat";
    int messageCount = 0;
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
        const ChatSession &chat = m_chatSessions[m_currentChatIndex];
        title = chat.title.isEmpty() ? "New Chat" : chat.title;
        messageCount = chat.messageCount;
    }

    QString profile = m_profileCombo ? m_profileCombo->currentText() : QString();
    QString model = m_modelCombo ? m_modelCombo->currentText() : QString();

    QStringList details;
    details << (messageCount == 0 ? "Ready to start" : QString("%1 messages").arg(messageCount));
    if (!profile.isEmpty()) {
        details << profile;
    }
    if (!model.isEmpty()) {
        details << model;
    }
    if (m_webSearchToggle && m_webSearchToggle->isChecked()) {
        details << "Web search";
    }

    m_headerTitle->setText(title);
    m_headerSubtitle->setText(details.join("  •  "));
}
