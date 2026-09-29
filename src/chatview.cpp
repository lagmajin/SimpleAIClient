#include "mainwindow.h"
#include "appicons.h"
#include "chatwidgets.h"


// Chat view: message cards, the batched lazy history render, scroll
// following, the welcome and thinking rows, and in-chat search with its
// highlighting. Split out of mainwindow.cpp because it is the largest
// cohesive block of the window (23 methods) and changes for its own reasons.

#include <QElapsedTimer>



#include <QScrollBar>





void MainWindow::refreshChatViewport()
{
    m_chatLayout->invalidate();
    m_chatContainer->adjustSize();
    m_scrollArea->widget()->updateGeometry();
    m_scrollArea->viewport()->update();
}

void MainWindow::removeTrailingSpacer()
{
    if (!m_chatLayout || m_chatLayout->count() == 0) {
        return;
    }

    QLayoutItem *lastItem = m_chatLayout->itemAt(m_chatLayout->count() - 1);
    if (lastItem && !lastItem->widget() && !lastItem->layout() && lastItem->spacerItem()) {
        QLayoutItem *removed = m_chatLayout->takeAt(m_chatLayout->count() - 1);
        delete removed;
    }
}

void MainWindow::appendBottomSpacer()
{
    if (!m_chatLayout) {
        return;
    }

    QLayoutItem *lastItem = m_chatLayout->count() > 0 ? m_chatLayout->itemAt(m_chatLayout->count() - 1) : nullptr;
    if (!(lastItem && !lastItem->widget() && !lastItem->layout() && lastItem->spacerItem())) {
        m_chatLayout->addStretch();
    }
}

void MainWindow::rebuildCurrentChatView()
{
    if (!m_chatContainer || !m_scrollArea) {
        return;
    }

    ++m_chatRenderGeneration;
    m_scrollFollowTimer->stop();
    m_stickToBottom = false;

    // Suppress intermediate repaints/relayouts while the batched render
    // reconstructs the message cards; everything is painted once at the end.
    m_chatContainer->setUpdatesEnabled(false);

    clearChatDisplay(false);

    // Captured after the clear: clearChatDisplay() invalidates any render that
    // was still pending, so reading the generation before it would make this
    // one stale and the transcript would never be drawn.
    const int renderGeneration = m_chatRenderGeneration;

    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        showWelcomeScreen();
        refreshChatViewport();
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    const auto &chat = m_chatSessions[m_currentChatIndex];
    if (chat.messages.isEmpty()) {
        showWelcomeScreen();
        refreshChatViewport();
        m_chatContainer->setUpdatesEnabled(true);
    } else {
        hideWelcomeScreen();
        m_chatRenderCursor = chat.messages.size() - 1;
        continueChatHistoryRender(renderGeneration);
    }
}

void MainWindow::clearChatDisplay(bool refresh)
{
    m_thinkingTimer->stop();

    // Every card below is destroyed, so any pointer still cached for search
    // navigation would dangle.
    m_highlightedCards.clear();
    m_currentHighlightIndex = -1;

    // Invalidate a pending batched history render: the caller re-adds cards
    // itself, so resuming the old cursor would duplicate them.
    ++m_chatRenderGeneration;
    m_chatRenderCursor = -1;

    // rebuildCurrentChatView() disables updates for the duration of a batched
    // render and re-enables them when it finishes. A pending batch is
    // invalidated here, so whoever disabled them has to give them back.
    m_chatContainer->setUpdatesEnabled(true);

    // The 50 ms render timer would otherwise keep firing with no card to
    // append to, for as long as the request lasts.
    m_streamRenderTimer->stop();
    m_pendingStreamChunk.clear();

    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        // deleteLater, not delete: this can run from a card's own signal
        // handler (Regenerate / Branch / Edit), and Qt is still dispatching
        // that signal from the widget.
        if (item->widget() && item->widget() != m_welcomeWidget) {
            item->widget()->deleteLater();
        } else if (item->layout()) {
            delete item->layout();
        }
        delete item;
    }
    appendBottomSpacer();
    m_streamingCard = nullptr;
    m_thinkingRowWidget = nullptr;
    m_thinkingIndicator = nullptr;
    if (refresh) {
        refreshChatViewport();
    }
}

void MainWindow::addMessageCard(const QString &role, const QString &content, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs)
{
    addMessageCardWithCard(role, content, promptTokens, completionTokens, totalTokens, responseTimeMs);
}

ChatMessageCard* MainWindow::addMessageCardWithCard(const QString &role, const QString &content, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs, bool prepend)
{
    hideWelcomeScreen(false);

    removeTrailingSpacer();
    ChatMessageCard *card = new ChatMessageCard(role, content, m_chatContainer);
    card->setTimestamp(QDateTime::currentDateTime());
    card->showCopyButton(true);
    card->setContentFontSize(m_chatFontSize);

    int msgIndex = -1;
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
        msgIndex = m_chatSessions[m_currentChatIndex].messages.size();
    }
    card->setMessageIndex(msgIndex);

    if (role == "assistant") {
        card->showRegenerateButton(true);
        card->showBranchButton(true);
        connect(card, &ChatMessageCard::regenerateRequested, this, &MainWindow::onRegenerateResponse);
        connect(card, &ChatMessageCard::branchRequested, this, &MainWindow::onBranchConversation);
    } else if (role == "user") {
        card->setEditable(true);
        connect(card, &ChatMessageCard::editRequested, this, &MainWindow::onEditMessage);
    }

    if (totalTokens > 0) {
        card->setTokenInfo(promptTokens, completionTokens, totalTokens, responseTimeMs);
    }

    if (prepend) {
        m_chatLayout->insertWidget(0, card);
    } else {
        m_chatLayout->addWidget(card);
    }
    appendBottomSpacer();
    return card;
}

void MainWindow::continueChatHistoryRender(int generation)
{
    if (generation != m_chatRenderGeneration) {
        // A newer render owns the updates-disabled window; it will re-enable.
        return;
    }

    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    const auto &chat = m_chatSessions[m_currentChatIndex];
    if (chat.messages.isEmpty()) {
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    const bool firstChunk = (m_chatRenderCursor == chat.messages.size() - 1);
    constexpr int kFirstBatchSize = 8;
    constexpr int kLaterBatchSize = 24;
    constexpr int kFirstChunkBudgetMs = 12;
    constexpr int kLaterChunkBudgetMs = 24;
    const int maxCards = firstChunk ? kFirstBatchSize : kLaterBatchSize;
    const int maxBudgetMs = firstChunk ? kFirstChunkBudgetMs : kLaterChunkBudgetMs;
    QElapsedTimer timer;
    timer.start();

    int rendered = 0;
    while (m_chatRenderCursor >= 0 && rendered < maxCards) {
        const auto &msg = chat.messages[m_chatRenderCursor];
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        ChatMessageCard *card = addMessageCardWithCard(
            msg.role,
            displayText,
            msg.promptTokens,
            msg.completionTokens,
            msg.totalTokens,
            0,
            true
        );
        if (card) {
            card->setMessageIndex(m_chatRenderCursor);
            card->setTimestamp(QDateTime::currentDateTime());
        }
        --m_chatRenderCursor;
        ++rendered;
        if (timer.elapsed() >= maxBudgetMs) {
            break;
        }
    }

    if (m_chatRenderCursor >= 0) {
        // No intermediate refreshChatViewport(): with updates disabled it
        // would only burn O(n) layout work per batch (O(n^2) overall).
        QTimer::singleShot(0, this, [this, generation]() {
            continueChatHistoryRender(generation);
        });
    } else {
        m_chatContainer->setUpdatesEnabled(true);
        refreshChatViewport();
        restoreCurrentChatScrollPosition();
    }
}

void MainWindow::saveCurrentChatScrollPosition()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size() || !m_scrollArea) {
        return;
    }

    if (QScrollBar *bar = m_scrollArea->verticalScrollBar()) {
        m_chatSessions[m_currentChatIndex].scrollPosition = bar->value();
    }
}

void MainWindow::restoreCurrentChatScrollPosition()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size() || !m_scrollArea) {
        return;
    }

    const int savedPosition = m_chatSessions[m_currentChatIndex].scrollPosition;
    const int renderGeneration = m_chatRenderGeneration;
    m_scrollFollowTimer->stop();
    m_stickToBottom = false;
    QTimer::singleShot(0, this, [this, savedPosition, renderGeneration]() {
        if (renderGeneration != m_chatRenderGeneration ||
            m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size() || !m_scrollArea) {
            return;
        }

        if (QScrollBar *bar = m_scrollArea->verticalScrollBar()) {
            bar->setValue(qBound(0, savedPosition, bar->maximum()));
            m_stickToBottom = isNearBottom();
        }
    });
}

void MainWindow::flushStreamingChunks()
{
    if (!m_streamingCard || m_pendingStreamChunk.isEmpty()) {
        if (m_streamRenderTimer->isActive() && m_pendingStreamChunk.isEmpty()) {
            m_streamRenderTimer->stop();
        }
        return;
    }

    QString chunk = m_pendingStreamChunk;
    m_pendingStreamChunk.clear();

    if (m_streamingCard->content().isEmpty()) {
        chunk = chunk.trimmed();
    }

    if (chunk.isEmpty()) {
        if (m_streamRenderTimer->isActive()) {
            m_streamRenderTimer->stop();
        }
        return;
    }

    m_streamingCard->appendContent(chunk);
    m_streamTokenCount += qMax(1, chunk.count(' ') + chunk.count('\n'));
    updateStreamingSpeed(chunk);
    scrollToBottom(false);

    if (m_streamRenderTimer->isActive() && m_pendingStreamChunk.isEmpty()) {
        m_streamRenderTimer->stop();
    }
}

bool MainWindow::isNearBottom(int tolerance) const
{
    if (!m_scrollArea) {
        return true;
    }

    QScrollBar *bar = m_scrollArea->verticalScrollBar();
    if (!bar) {
        return true;
    }

    return (bar->maximum() - bar->value()) <= tolerance;
}

void MainWindow::scrollToBottom(bool force)
{
    if (!m_autoScroll) return;
    if (!force && !m_stickToBottom) return;

    m_stickToBottom = true;
    QScrollBar *bar = m_scrollArea ? m_scrollArea->verticalScrollBar() : nullptr;
    if (bar) {
        // Let word-wrap/layout settle and perform at most one movement per
        // frame. rangeChanged will request another frame only if needed.
        if (!m_scrollFollowTimer->isActive()) {
            m_scrollFollowTimer->start();
        }
    }
}

void MainWindow::showWelcomeScreen(bool refresh)
{
    if (m_welcomeWidget) {
        if (m_chatLayout->indexOf(m_welcomeWidget) == -1) {
            m_chatLayout->insertWidget(0, m_welcomeWidget, 0, Qt::AlignCenter);
        }
        m_welcomeWidget->setVisible(true);
        m_welcomeWidget->show();
        m_welcomeWidget->raise();
        if (refresh) {
            refreshChatViewport();
        }
    }
}

void MainWindow::hideWelcomeScreen(bool refresh)
{
    if (m_welcomeWidget) {
        m_welcomeWidget->setVisible(false);
        if (refresh) {
            refreshChatViewport();
        }
    }
}

void MainWindow::showThinkingIndicator()
{
    if (m_thinkingIndicator) return;

    hideWelcomeScreen();

    removeTrailingSpacer();

    m_thinkingRowWidget = new QWidget(m_chatContainer);
    QHBoxLayout *thinkingRow = new QHBoxLayout(m_thinkingRowWidget);
    thinkingRow->setContentsMargins(32, 0, 32, 0);
    thinkingRow->setSpacing(12);

    m_thinkingIndicator = new QLabel(m_thinkingRowWidget);
    m_thinkingIndicator->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_thinkingIndicator->setStyleSheet(
        "QLabel { "
        "  color: " + currentTheme().textMuted + "; "
        "  font-size: 15px; "
        "  font-style: italic; "
        "  padding: 12px 16px; "
        "}"
    );
    m_theme->registerStyle(m_thinkingIndicator, []() {
        return QString(
            "QLabel { "
            "  color: " + currentTheme().textMuted + "; "
            "  font-size: 15px; "
            "  font-style: italic; "
            "  padding: 12px 16px; "
            "}");
    });

    AvatarLabel *aiAvatar = new AvatarLabel("assistant", m_thinkingRowWidget);
    aiAvatar->setFixedSize(28, 28);
    thinkingRow->addWidget(aiAvatar, 0, Qt::AlignTop);

    thinkingRow->addWidget(m_thinkingIndicator, 1);
    thinkingRow->addStretch();

    m_chatLayout->addWidget(m_thinkingRowWidget);
    appendBottomSpacer();

    m_thinkingDots = 0;
    m_thinkingIndicator->setText("Thinking");
    m_thinkingTimer->start(500);

    scrollToBottom();
}

void MainWindow::hideThinkingIndicator(bool refresh)
{
    m_thinkingTimer->stop();
    if (!m_thinkingIndicator) return;

    delete m_thinkingRowWidget;
    m_thinkingRowWidget = nullptr;
    m_thinkingIndicator = nullptr;
    if (refresh) {
        refreshChatViewport();
    }
}

void MainWindow::rebuildCardsForMessages(const QList<ChatMessage> &messages)
{
    clearChatDisplay();
    for (int i = 0; i < messages.size(); ++i) {
        const auto &msg = messages.at(i);
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        ChatMessageCard *card = addMessageCardWithCard(msg.role, displayText, msg.promptTokens, msg.completionTokens, msg.totalTokens);
        if (card) {
            card->setMessageIndex(i);
        }
    }
}

void MainWindow::showSearchBar()
{
    if (m_searchBar) {
        m_searchBar->setVisible(true);
        m_searchBar->m_searchInput->setFocus();
    }
}

void MainWindow::hideSearchBar()
{
    if (m_searchBar) {
        m_searchBar->setVisible(false);
        clearHighlights();
    }
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    // One pass over the visible cards: each card re-renders at most once, and
    // clearHighlight() is a no-op for cards that carry no highlight.
    m_highlightedCards.clear();
    for (int i = 0; i < m_chatLayout->count(); ++i) {
        QLayoutItem *item = m_chatLayout->itemAt(i);
        if (auto *card = qobject_cast<ChatMessageCard*>(item->widget())) {
            if (!text.isEmpty() && card->content().contains(text, Qt::CaseInsensitive)) {
                card->highlightText(text);
                m_highlightedCards.append(card);
            } else {
                card->clearHighlight();
            }
        }
    }

    m_currentHighlightIndex = m_highlightedCards.isEmpty() ? -1 : 0;
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(
            m_highlightedCards.isEmpty() ? "0/0" :
            QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size())
        );
    }
}

void MainWindow::onFindNext()
{
    if (m_highlightedCards.isEmpty()) return;
    m_currentHighlightIndex = (m_currentHighlightIndex + 1) % m_highlightedCards.size();
    m_scrollArea->ensureWidgetVisible(m_highlightedCards[m_currentHighlightIndex], 0, 120);
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size()));
    }
}

void MainWindow::onFindPrevious()
{
    if (m_highlightedCards.isEmpty()) return;
    m_currentHighlightIndex = (m_currentHighlightIndex - 1 + m_highlightedCards.size()) % m_highlightedCards.size();
    m_scrollArea->ensureWidgetVisible(m_highlightedCards[m_currentHighlightIndex], 0, 120);
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size()));
    }
}

void MainWindow::clearHighlights()
{
    for (auto *card : m_highlightedCards) {
        card->clearHighlight();
    }
    m_highlightedCards.clear();
    m_currentHighlightIndex = -1;
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText("0/0");
    }
}
