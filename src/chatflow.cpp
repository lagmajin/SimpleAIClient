#include "mainwindow.h"
#include "appicons.h"
#include "chatwidgets.h"


// Request flow: sending a turn and handling the response, streaming or
// not, plus regenerate, branch and edit. Kept apart because it is the
// only code that mutates m_requestChatIndex and must be read as a whole.


#include <QMessageBox>
#include <QScrollBar>






void MainWindow::beginRequest(int chatIndex)
{
    m_requestChatIndex = chatIndex;
    m_streamedContent.clear();
    m_view->clearStream();
    m_retryCount = 0;
    m_streamStartTime = QDateTime::currentMSecsSinceEpoch();

    m_inputField->setEnabled(false);
    setRequestInFlight(true);

    if (chatIndex == m_currentChatIndex) {
        showThinkingIndicator();
    }

    m_pendingMessages = m_chatSessions[chatIndex].messages;
    m_apiClient->sendMessage(m_pendingMessages);
}

bool MainWindow::persistAssistantMessage(int chatIndex, const QString &content, int promptTokens, int completionTokens, int totalTokens)
{
    if (chatIndex < 0 || chatIndex >= m_chatSessions.size()) {
        return false;
    }
    if (!m_chatSessions[chatIndex].messagesLoaded) {
        return false;
    }
    m_chatSessions[chatIndex].messages.append({"assistant", content, promptTokens, completionTokens, totalTokens});
    m_chatSessions[chatIndex].messageCount = m_chatSessions[chatIndex].messages.size();
    // Both the sidebar subtitle and the header show the count, and neither was
    // refreshed after the first turn, so they read "1 message" for the rest of
    // the conversation. A background chat is refreshed too, since the sidebar
    // is visible while the user reads elsewhere.
    updateChatList();
    if (chatIndex == m_currentChatIndex) {
        updateHeaderState();
    }
    return true;
}

void MainWindow::onSendMessage()
{
    if (m_requestInFlight) {
        m_apiClient->cancelCurrentRequest();
        return;
    }

    QString text = m_inputField->toPlainText().trimmed();
    if (text.isEmpty() && m_currentChatImage.isEmpty()) return;

    if (text.startsWith("/")) {
        if (handleQuickCommand(text)) {
            m_inputField->clear();
        } else {
            m_inputField->setPlainText(text);
        }
        return;
    }

    if (!checkApiKey()) return;

    if (text.length() > kMaxInputChars) {
        QMessageBox::warning(this, "Message Too Long",
            QString("This message is %1 characters. Trim it to %2 or fewer before sending.")
                .arg(text.length()).arg(kMaxInputChars));
        return;
    }

    clearDraft();

    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        createNewChat();
    }

    m_inputField->clear();

    QString displayText = text;
    if (!m_currentChatImage.isEmpty()) {
        displayText += "\n[Image attached]";
    }
    addMessageCard("user", displayText);

    ChatMessage userMsg{"user", text, 0, 0, 0, m_currentChatImage};
    m_chatSessions[m_currentChatIndex].messages.append(userMsg);
    m_chatSessions[m_currentChatIndex].messageCount = m_chatSessions[m_currentChatIndex].messages.size();

    m_currentChatImage.clear();
    m_imagePreview->setVisible(false);
    m_imagePreview->clear();

    if (m_chatSessions[m_currentChatIndex].messages.size() == 1) {
        m_chatSessions[m_currentChatIndex].title = generateChatTitle(text);
        updateChatList();
        updateHeaderState();
        m_chatStartTime = QDateTime::currentDateTime();
    }

    beginRequest(m_currentChatIndex);
    saveChatSessions();

    if (m_statusConnection) {
        m_statusConnection->setText("Sending...");
    }

    scrollToBottom();
}

void MainWindow::onResponseReceived(const QString &response, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs)
{
    hideThinkingIndicator();

    m_view->clearStream();
    m_streamedContent = response;

    const int requestChat = m_requestChatIndex;
    const bool shownInCurrentChat = requestChat == m_currentChatIndex;

    if (shownInCurrentChat) {
        addMessageCard("assistant", response, promptTokens, completionTokens, totalTokens, responseTimeMs);
    }

    if (persistAssistantMessage(requestChat, response, promptTokens, completionTokens, totalTokens)) {
        saveChatSessions();
        if (shownInCurrentChat) {
            updateContextUsage();
        }
    }

    if (m_statusConnection) {
        m_statusConnection->setText("Ready");
    }
    if (m_statusTokens && totalTokens > 0) {
        m_statusTokens->setText(QString("%1 tokens").arg(totalTokens));
    }
    if (m_statusResponseTime && responseTimeMs > 0) {
        m_statusResponseTime->setText(QString("%.1fs").arg(responseTimeMs / 1000.0));
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);

    scrollToBottom();

    saveChatSessions();

    updateContextUsage();
    updateHeaderState();
    updateChatDuration();
    playNotificationSound();

    // Non-streaming requests never emit responseFinished, so the request state
    // has to be torn down here or it leaks into the next chat switch.
    m_retryCount = 0;
    m_pendingMessages.clear();
    m_requestChatIndex = -1;
}

void MainWindow::onResponseChunk(const QString &chunk)
{
    m_streamedContent += chunk;

    // The view can be rebuilt mid-stream (chat switch, regenerate), so the
    // live card is seeded from the full text received so far rather than from
    // the chunk, and the pending buffer is what coalesces the repaints.
    if (m_requestChatIndex == m_currentChatIndex) {
        m_view->appendStreamChunk(chunk);
    }

    if (m_statusConnection) {
        m_statusConnection->setText("Streaming...");
    }
}

void MainWindow::onResponseFinished(int responseTimeMs)
{
    hideThinkingIndicator();
    m_view->endStream();

    const int requestChat = m_requestChatIndex;
    const bool shownInCurrentChat = requestChat == m_currentChatIndex;

    if (persistAssistantMessage(requestChat, m_streamedContent, 0, 0, 0)) {
        saveChatSessions();
        if (shownInCurrentChat) {
            updateContextUsage();
        }
    }

    if (m_statusConnection) {
        m_statusConnection->setText("Ready");
    }
    if (m_statusResponseTime && responseTimeMs > 0) {
        m_statusResponseTime->setText(QString("%.1fs").arg(responseTimeMs / 1000.0));
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);
    m_retryCount = 0;
    m_pendingMessages.clear();

    if (m_statusSpeed) m_statusSpeed->clear();
    m_requestChatIndex = -1;
    m_streamedContent.clear();

    updateChatDuration();
    playNotificationSound();
}

void MainWindow::onErrorOccurred(const QString &error)
{
    qDebug() << "API Error:" << error;

    hideThinkingIndicator();

    m_retryCount = 0;
    m_pendingMessages.clear();

    // Drop the partial render: keeping it would prepend this attempt's tail to
    // the next response.
    m_view->clearStream();
    m_streamedContent.clear();
    if (m_statusSpeed) m_statusSpeed->clear();

    addMessageCard("error", "Error: " + error);

    if (m_statusConnection) {
        m_statusConnection->setText("Error");
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);
    m_requestChatIndex = -1;
}

void MainWindow::onRequestCancelled()
{
    hideThinkingIndicator();
    m_retryTimer->stop();
    m_view->endStream();

    const int requestChat = m_requestChatIndex;
    const bool shownInCurrentChat = requestChat == m_currentChatIndex;
    const QString partialContent = m_streamedContent;

    const bool keepPartial = !partialContent.trimmed().isEmpty();
    if (keepPartial && persistAssistantMessage(requestChat, partialContent, 0, 0, 0)) {
        saveChatSessions();
        if (shownInCurrentChat) {
            updateContextUsage();
        }
    } else {
        m_view->clearStream();
    }

    m_streamedContent.clear();
    m_requestChatIndex = -1;
    m_retryCount = 0;
    m_pendingMessages.clear();
    if (m_statusSpeed) m_statusSpeed->clear();
    if (m_statusConnection) {
        m_statusConnection->setText("Stopped");
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);
}

void MainWindow::onRegenerateResponse(int messageIndex)
{
    if (m_requestInFlight) return;
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    auto &messages = m_chatSessions[m_currentChatIndex].messages;
    if (messages.isEmpty()) return;

    if (messageIndex >= 0 && messageIndex < messages.size()) {
        // Regenerating an older answer drops that answer and everything after
        // it, instead of always dropping the newest one.
        messages = messages.mid(0, messageIndex);
    } else {
        // No usable index: fall back to dropping a trailing answer.
        if (messages.last().role == "assistant") {
            messages.removeLast();
        }
    }
    m_chatSessions[m_currentChatIndex].messageCount = messages.size();

    if (messages.isEmpty()) return;

    const QList<ChatMessage> requestMessages = messages;
    rebuildCardsForMessages(requestMessages);
    beginRequest(m_currentChatIndex);
    saveChatSessions();

    scrollToBottom();
}

void MainWindow::onBranchConversation(int messageIndex)
{
    // prepending shifts every session index, so an in-flight request would end
    // up pointing at a different chat than the one it was sent from.
    if (m_requestInFlight) return;
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    const auto &sourceChat = m_chatSessions[m_currentChatIndex];
    if (messageIndex < 0 || messageIndex >= sourceChat.messages.size()) return;

    ChatSession newChat;
    newChat.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    newChat.title = sourceChat.title + " (branch)";
    newChat.pinned = false;
    newChat.messagesLoaded = true;

    for (int i = 0; i <= messageIndex; ++i) {
        newChat.messages.append(sourceChat.messages[i]);
    }
    newChat.messageCount = newChat.messages.size();

    m_chatSessions.prepend(newChat);
    m_currentChatIndex = 0;

    rebuildCardsForMessages(newChat.messages);

    updateChatList();
    saveChatSessions();
    scrollToBottom();
}

void MainWindow::onEditMessage(int messageIndex, const QString &newContent)
{
    if (m_requestInFlight) return;
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    auto &messages = m_chatSessions[m_currentChatIndex].messages;
    if (messageIndex < 0 || messageIndex >= messages.size()) return;

    messages[messageIndex].content = newContent;

    for (int i = messages.size() - 1; i > messageIndex; --i) {
        if (messages[i].role == "assistant" || messages[i].role == "user") {
            messages.removeAt(i);
        }
    }
    m_chatSessions[m_currentChatIndex].messageCount = messages.size();

    const QList<ChatMessage> requestMessages = messages;
    rebuildCardsForMessages(requestMessages);
    beginRequest(m_currentChatIndex);
    saveChatSessions();

    scrollToBottom();
}

void MainWindow::onApiErrorWithRetry(const QString &error)
{
    m_retryCount++;
    if (m_retryCount <= m_maxRetries && !m_pendingMessages.isEmpty()) {
        int delayMs = (1 << (m_retryCount - 1)) * 1000;
        if (m_statusConnection) {
            m_statusConnection->setText(QString("Retrying (%1/%2)...").arg(m_retryCount).arg(m_maxRetries));
        }

        // Drop the failed attempt's partial card so the retry starts a fresh
        // one instead of appending to it.
        m_view->clearStream();
        m_streamedContent.clear();
        if (m_requestChatIndex == m_currentChatIndex) {
            showThinkingIndicator();
        }

        const QList<ChatMessage> retryMessages = m_pendingMessages;
        m_retryTimer->singleShot(delayMs, this, [this, retryMessages]() {
            if (!m_requestInFlight || retryMessages.isEmpty()) {
                return;
            }
            m_apiClient->sendMessage(retryMessages);
        });
    } else {
        m_retryCount = 0;
        onErrorOccurred(error);
    }
}
