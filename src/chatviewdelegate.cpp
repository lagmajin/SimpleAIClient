// MainWindow's thin delegates to ChatView.
//
// The view owns the conversation pane, so the window asks it to do things and
// otherwise keeps the session and request state. These keep the ~40 call sites
// unchanged, and they live next to the view so everything that knows both
// halves exist is in one place.

#include "mainwindow.h"

#include "chatview.h"

void MainWindow::initChatView()
{
    m_view = new ChatView(m_scrollArea, m_chatContainer, m_welcomeWidget, m_searchBar, nullptr, this);
    connect(m_view, &ChatView::regenerateRequested, this, &MainWindow::onRegenerateResponse);
    connect(m_view, &ChatView::branchRequested, this, &MainWindow::onBranchConversation);
    connect(m_view, &ChatView::editRequested, this, &MainWindow::onEditMessage);
    connect(m_view, &ChatView::chunkRendered, this, &MainWindow::updateStreamingSpeed);
    m_view->setAutoScroll(m_autoScrollToggle->isChecked());
    m_view->setFontSize(m_chatFontSize);
}

// The view renders the chat that is currently open, so it is re-pointed at the
// message list whenever the current chat changes.
void MainWindow::pointViewAtCurrentChat()
{
    if (!m_view) {
        return;
    }
    m_view->setScrollPersistence(m_currentChatIndex);
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
        m_view->setMessages(&m_chatSessions[m_currentChatIndex].messages);
    } else {
        m_view->setMessages(nullptr);
    }
}

void MainWindow::rebuildCurrentChatView()
{
    m_view->rebuild();
}

void MainWindow::clearChatDisplay(bool refresh)
{
    m_view->clear(refresh);
}

void MainWindow::addMessageCard(const QString &role, const QString &content,
                                int promptTokens, int completionTokens,
                                int totalTokens, int responseTimeMs)
{
    m_view->addCard(role, content, promptTokens, completionTokens, totalTokens, responseTimeMs);
}

ChatMessageCard *MainWindow::addMessageCardWithCard(const QString &role, const QString &content,
                                                    int promptTokens, int completionTokens,
                                                    int totalTokens, int responseTimeMs, bool)
{
    return m_view->addCard(role, content, promptTokens, completionTokens, totalTokens, responseTimeMs);
}

void MainWindow::rebuildCardsForMessages(const QList<ChatMessage> &messages)
{
    m_view->rebuildCards(messages);
}

void MainWindow::scrollToBottom(bool force)
{
    m_view->scrollToBottom(force);
}

bool MainWindow::isNearBottom(int tolerance) const
{
    return m_view->isNearBottom(tolerance);
}

void MainWindow::saveCurrentChatScrollPosition()
{
    m_view->saveScrollPosition();
}

void MainWindow::restoreCurrentChatScrollPosition()
{
    m_view->restoreScrollPosition();
}

void MainWindow::refreshChatViewport()
{
    m_view->refreshViewport();
}

void MainWindow::flushStreamingChunks()
{
    m_view->endStream();
}

void MainWindow::showWelcomeScreen(bool refresh)
{
    m_view->showWelcomeScreen(refresh);
}

void MainWindow::hideWelcomeScreen(bool refresh)
{
    m_view->hideWelcomeScreen(refresh);
}

void MainWindow::showThinkingIndicator()
{
    m_view->showThinkingIndicator();
}

void MainWindow::hideThinkingIndicator(bool refresh)
{
    m_view->hideThinkingIndicator(refresh);
}

void MainWindow::restyleCards()
{
    m_view->applyTheme();
}

void MainWindow::showSearchBar()
{
    m_view->showSearchBar();
}

void MainWindow::hideSearchBar()
{
    m_view->hideSearchBar();
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    m_view->search(text);
}

void MainWindow::onFindNext()
{
    m_view->findNext();
}

void MainWindow::onFindPrevious()
{
    m_view->findPrevious();
}

void MainWindow::clearHighlights()
{
    m_view->clearHighlights();
}
