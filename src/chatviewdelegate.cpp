// MainWindow's thin delegates to ChatView.
//
// The view owns the conversation pane, so the window asks it to do things and
// otherwise keeps the session and request state. These keep the ~40 call sites
// unchanged, and they live next to the view so everything that knows both
// halves exist is in one place.

#include "mainwindow.h"

#include "chatsessionlist.h"
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
// message list whenever the current chat changes. Every mutation of
// m_chatSessions that shifts an index has to call this: the list the view
// borrows moves underneath it otherwise.
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
    if (m_view) m_view->rebuild();
}

void MainWindow::clearChatDisplay(bool refresh)
{
    if (m_view) m_view->clear(refresh);
}

void MainWindow::addMessageCard(const QString &role, const QString &content,
                                int promptTokens, int completionTokens,
                                int totalTokens, int responseTimeMs)
{
    if (m_view) m_view->addCard(role, content, promptTokens, completionTokens, totalTokens, responseTimeMs);
}

ChatMessageCard *MainWindow::addMessageCardWithCard(const QString &role, const QString &content,
                                                    int promptTokens, int completionTokens,
                                                    int totalTokens, int responseTimeMs, bool)
{
    return m_view ? m_view->addCard(role, content, promptTokens, completionTokens, totalTokens, responseTimeMs)
                      : nullptr;
}

void MainWindow::rebuildCardsForMessages(const QList<ChatMessage> &messages)
{
    if (m_view) m_view->rebuildCards(messages);
}

void MainWindow::scrollToBottom(bool force)
{
    if (m_view) m_view->scrollToBottom(force);
}

bool MainWindow::isNearBottom(int tolerance) const
{
    return m_view ? m_view->isNearBottom(tolerance) : true;
}

void MainWindow::saveCurrentChatScrollPosition()
{
    if (m_view) m_view->saveScrollPosition();
}

void MainWindow::restoreCurrentChatScrollPosition()
{
    if (m_view) m_view->restoreScrollPosition();
}

void MainWindow::refreshChatViewport()
{
    if (m_view) m_view->refreshViewport();
}

void MainWindow::flushStreamingChunks()
{
    if (m_view) m_view->endStream();
}

void MainWindow::showWelcomeScreen(bool refresh)
{
    if (m_view) m_view->showWelcomeScreen(refresh);
}

void MainWindow::hideWelcomeScreen(bool refresh)
{
    if (m_view) m_view->hideWelcomeScreen(refresh);
}

void MainWindow::showThinkingIndicator()
{
    if (m_view) m_view->showThinkingIndicator();
}

void MainWindow::hideThinkingIndicator(bool refresh)
{
    if (m_view) m_view->hideThinkingIndicator(refresh);
}

void MainWindow::restyleCards()
{
    if (m_view) m_view->applyTheme();
}

void MainWindow::showSearchBar()
{
    if (m_view) m_view->showSearchBar();
}

void MainWindow::hideSearchBar()
{
    if (m_view) m_view->hideSearchBar();
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    if (m_view) m_view->search(text);
}

void MainWindow::onFindNext()
{
    if (m_view) m_view->findNext();
}

void MainWindow::onFindPrevious()
{
    if (m_view) m_view->findPrevious();
}

void MainWindow::clearHighlights()
{
    if (m_view) m_view->clearHighlights();
}
