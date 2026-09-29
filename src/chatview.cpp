// The conversation pane.
//
// Split out of MainWindow: it owns the card layout, the batched lazy history
// render, scroll following and in-chat search, and needs nothing from the
// window except the message list it renders and the three card actions.

#include "chatview.h"

#include "apiclient.h"
#include "chatwidgets.h"
#include "theme.h"

#include <QDateTime>
#include <QElapsedTimer>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QScrollBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#include <cmath>

namespace {
constexpr int kFirstBatchSize = 8;
constexpr int kLaterBatchSize = 24;
constexpr int kFirstChunkBudgetMs = 12;
constexpr int kLaterChunkBudgetMs = 24;
}

ChatView::ChatView(QWidget *scrollArea,
                   QWidget *container,
                   QWidget *welcomeWidget,
                   ChatSearchBar *searchBar,
                   QList<ChatMessage> *messages,
                   QObject *parent)
    : QObject(parent)
    , m_scrollArea(scrollArea)
    , m_chatContainer(container)
    , m_welcomeWidget(welcomeWidget)
    , m_searchBar(searchBar)
    , m_messages(messages)
    , m_thinkingTimer(new QTimer(this))
    , m_streamRenderTimer(new QTimer(this))
    , m_scrollFollowTimer(new QTimer(this))
{
    m_chatLayout = qobject_cast<QVBoxLayout *>(container->layout());
    if (!m_chatLayout) {
        m_chatLayout = new QVBoxLayout(container);
    }

    // Slightly slower updates keep the UI smoother during long streams.
    m_streamRenderTimer->setInterval(50);
    m_streamRenderTimer->setSingleShot(false);
    connect(m_streamRenderTimer, &QTimer::timeout, this, &ChatView::flushStreamingChunks);

    connect(m_thinkingTimer, &QTimer::timeout, this, [this]() {
        if (!m_thinkingIndicator) return;
        m_thinkingDots = (m_thinkingDots + 1) % 4;
        m_thinkingIndicator->setText("Thinking" + QString(m_thinkingDots, '.'));
    });

    // Follow the live layout target without restarting an animation per chunk,
    // easing in so the view glides instead of jumping frame to frame.
    m_scrollFollowTimer->setInterval(16);
    m_scrollFollowTimer->setTimerType(Qt::PreciseTimer);
    connect(m_scrollFollowTimer, &QTimer::timeout, this, [this, clock = QElapsedTimer()]() mutable {
        auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea);
        if (!m_autoScroll || !m_stickToBottom || !scroll) {
            m_scrollFollowTimer->stop();
            clock.invalidate();
            return;
        }
        QScrollBar *bar = scroll->verticalScrollBar();
        if (!bar || bar->isSliderDown()) {
            m_scrollFollowTimer->stop();
            clock.invalidate();
            return;
        }
        const qint64 elapsed = clock.isValid() ? clock.restart() : 16;
        if (!clock.isValid()) clock.start();
        const int distance = bar->maximum() - bar->value();
        const double blend = 1.0 - std::exp(-qMin(elapsed, qint64(50)) / 85.0);
        const int step = qMax(1, int(std::ceil(distance * blend)));
        bar->setValue(bar->value() + qMin(distance, step));
        if (bar->value() == bar->maximum()) {
            m_scrollFollowTimer->stop();
            clock.invalidate();
        }
    });

    if (auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea)) {
        connect(scroll->verticalScrollBar(), &QScrollBar::valueChanged,
                this, [this](int) { saveScrollPosition(); });
        // Only user actions change follow intent; animation and layout changes
        // do not.
        connect(scroll->verticalScrollBar(), &QScrollBar::actionTriggered,
                this, [this, scroll](int action) {
            QScrollBar *bar = scroll->verticalScrollBar();
            const bool movingUp = action == QAbstractSlider::SliderSingleStepSub ||
                action == QAbstractSlider::SliderPageStepSub ||
                action == QAbstractSlider::SliderToMinimum || bar->sliderPosition() < bar->value();
            m_stickToBottom = !movingUp && bar->maximum() - bar->sliderPosition() <= 48;
            if (!m_stickToBottom) m_scrollFollowTimer->stop();
            else scrollToBottom(false);
        });
        connect(scroll->verticalScrollBar(), &QScrollBar::sliderPressed, this, [this]() {
            m_stickToBottom = false;
            m_scrollFollowTimer->stop();
        });
        connect(scroll->verticalScrollBar(), &QScrollBar::sliderReleased, this, [this, scroll]() {
            QScrollBar *bar = scroll->verticalScrollBar();
            m_stickToBottom = bar->maximum() - bar->sliderPosition() <= 48;
            scrollToBottom(false);
        });
        // Re-stick to the bottom whenever the content grows after layout
        // settles. Scrolling to maximum() directly from a chunk handler races
        // with the deferred word-wrap relayout and makes the view jump back and
        // forth.
        connect(scroll->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this, scroll](int, int) {
            if (m_autoScroll && m_stickToBottom && !scroll->verticalScrollBar()->isSliderDown()
                && !m_scrollFollowTimer->isActive()) {
                m_scrollFollowTimer->start();
            }
        });
    }
}

void ChatView::setMessages(QList<ChatMessage> *messages)
{
    m_messages = messages;
}

void ChatView::setFontSize(int pixels)
{
    m_fontSize = pixels;
}

void ChatView::setAutoScroll(bool enabled)
{
    m_autoScroll = enabled;
    if (m_autoScroll) {
        scrollToBottom();
    } else {
        m_scrollFollowTimer->stop();
    }
}

void ChatView::setScrollPersistence(int chatIndex)
{
    m_currentChatIndex = chatIndex;
}

void ChatView::scrollFollowStarted()
{
    m_stickToBottom = false;
    m_scrollFollowTimer->stop();
}

void ChatView::refreshViewport()
{
    auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea);
    m_chatLayout->invalidate();
    m_chatContainer->adjustSize();
    if (scroll) {
        scroll->widget()->updateGeometry();
        scroll->viewport()->update();
    }
}

void ChatView::removeTrailingSpacer()
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

void ChatView::appendBottomSpacer()
{
    if (!m_chatLayout) {
        return;
    }

    QLayoutItem *lastItem = m_chatLayout->count() > 0 ? m_chatLayout->itemAt(m_chatLayout->count() - 1) : nullptr;
    if (!(lastItem && !lastItem->widget() && !lastItem->layout() && lastItem->spacerItem())) {
        m_chatLayout->addStretch();
    }
}

void ChatView::rebuild()
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

    clear(false);

    // Captured after the clear: clear() invalidates any render that was still
    // pending, so reading the generation before it would make this one stale
    // and the transcript would never be drawn.
    const int renderGeneration = m_chatRenderGeneration;

    if (!m_messages || m_messages->isEmpty()) {
        showWelcomeScreen();
        refreshViewport();
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    hideWelcomeScreen();
    m_chatRenderCursor = m_messages->size() - 1;
    continueChatHistoryRender(renderGeneration);
}

void ChatView::clear(bool refresh)
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

    // rebuild() disables updates for the duration of a batched render and
    // re-enables them when it finishes. A pending batch is invalidated here, so
    // whoever disabled them has to give them back.
    m_chatContainer->setUpdatesEnabled(true);

    // The 50 ms render timer would otherwise keep firing with no card to append
    // to, for as long as the request lasts.
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
        refreshViewport();
    }
}

ChatMessageCard *ChatView::addCard(const QString &role, const QString &content,
                                   int promptTokens, int completionTokens,
                                   int totalTokens, int responseTimeMs)
{
    return addMessageCardRow(role, content, promptTokens, completionTokens, totalTokens, responseTimeMs, false);
}

ChatMessageCard *ChatView::addMessageCardRow(const QString &role, const QString &displayText,
                                            int promptTokens, int completionTokens,
                                            int totalTokens, int responseTimeMs, bool prepend)
{
    hideWelcomeScreen(false);

    removeTrailingSpacer();
    ChatMessageCard *card = new ChatMessageCard(role, displayText, m_chatContainer);
    card->setTimestamp(QDateTime::currentDateTime());
    card->showCopyButton(true);
    card->setContentFontSize(m_fontSize);

    card->setMessageIndex(m_messages ? m_messages->size() : -1);

    if (role == "assistant") {
        card->showRegenerateButton(true);
        card->showBranchButton(true);
        connect(card, &ChatMessageCard::regenerateRequested,
                this, &ChatView::regenerateRequested);
        connect(card, &ChatMessageCard::branchRequested,
                this, &ChatView::branchRequested);
    } else if (role == "user") {
        card->setEditable(true);
        connect(card, &ChatMessageCard::editRequested,
                this, &ChatView::editRequested);
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

void ChatView::continueChatHistoryRender(int generation)
{
    if (generation != m_chatRenderGeneration) {
        // A newer render owns the updates-disabled window; it will re-enable.
        return;
    }

    if (!m_messages || m_messages->isEmpty()) {
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    const bool firstChunk = (m_chatRenderCursor == m_messages->size() - 1);
    const int maxCards = firstChunk ? kFirstBatchSize : kLaterBatchSize;
    const int maxBudgetMs = firstChunk ? kFirstChunkBudgetMs : kLaterChunkBudgetMs;
    QElapsedTimer timer;
    timer.start();

    int rendered = 0;
    while (m_chatRenderCursor >= 0 && rendered < maxCards) {
        const ChatMessage &msg = m_messages->at(m_chatRenderCursor);
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        ChatMessageCard *card = addMessageCardRow(
            msg.role, displayText, msg.promptTokens, msg.completionTokens,
            msg.totalTokens, 0, true);
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
        // No intermediate refreshViewport(): with updates disabled it would
        // only burn O(n) layout work per batch (O(n^2) overall).
        QTimer::singleShot(0, this, [this, generation]() {
            continueChatHistoryRender(generation);
        });
    } else {
        m_chatContainer->setUpdatesEnabled(true);
        refreshViewport();
        restoreScrollPosition();
    }
}

void ChatView::rebuildCards(const QList<ChatMessage> &messages)
{
    clear();
    for (int i = 0; i < messages.size(); ++i) {
        const ChatMessage &msg = messages.at(i);
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        ChatMessageCard *card = addMessageCardRow(msg.role, displayText, msg.promptTokens,
                                                  msg.completionTokens, msg.totalTokens, 0, false);
        if (card) {
            card->setMessageIndex(i);
        }
    }
}

void ChatView::saveScrollPosition()
{
    if (m_currentChatIndex < 0 || !m_messages || !m_scrollArea) {
        return;
    }
    if (auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea)) {
        if (QScrollBar *bar = scroll->verticalScrollBar()) {
            m_savedScrollPositions[m_currentChatIndex] = bar->value();
        }
    }
}

void ChatView::restoreScrollPosition()
{
    if (!m_scrollArea) {
        return;
    }
    const int savedPosition = m_savedScrollPositions.value(m_currentChatIndex, 0);
    const int renderGeneration = m_chatRenderGeneration;
    m_scrollFollowTimer->stop();
    m_stickToBottom = false;
    QTimer::singleShot(0, this, [this, savedPosition, renderGeneration]() {
        if (renderGeneration != m_chatRenderGeneration) {
            return;
        }
        auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea);
        if (!scroll) {
            return;
        }
        if (QScrollBar *bar = scroll->verticalScrollBar()) {
            bar->setValue(qBound(0, savedPosition, bar->maximum()));
        }
    });
}

void ChatView::beginStream(const QString &seedText)
{
    if (m_streamingCard) {
        return;
    }
    hideThinkingIndicator(false);
    removeTrailingSpacer();
    m_streamingCard = new ChatMessageCard("assistant", seedText, m_chatContainer);
    m_streamingCard->setContentFontSize(m_fontSize);
    m_streamingCard->setStreaming(true);
    m_streamingCard->setTimestamp(QDateTime::currentDateTime());
    m_chatLayout->addWidget(m_streamingCard);
    appendBottomSpacer();
    refreshViewport();
}

void ChatView::appendStreamChunk(const QString &chunk)
{
    if (!m_streamingCard || chunk.isEmpty()) {
        return;
    }
    m_pendingStreamChunk += chunk;
    if (!m_streamRenderTimer->isActive()) {
        m_streamRenderTimer->start();
    }
}

void ChatView::clearStream()
{
    m_streamRenderTimer->stop();
    m_pendingStreamChunk.clear();
    if (m_streamingCard) {
        m_chatLayout->removeWidget(m_streamingCard);
        delete m_streamingCard;
        m_streamingCard = nullptr;
    }
}

void ChatView::endStream()
{
    flushStreamingChunks();
    m_streamRenderTimer->stop();
    m_pendingStreamChunk.clear();
    if (m_streamingCard) {
        m_streamingCard->setStreaming(false);
        m_streamingCard->showCopyButton(true);
    }
}

void ChatView::flushStreamingChunks()
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
    emit chunkRendered(chunk.count(' ') + chunk.count('\n'));
    scrollToBottom(false);

    if (m_streamRenderTimer->isActive() && m_pendingStreamChunk.isEmpty()) {
        m_streamRenderTimer->stop();
    }
}

bool ChatView::isNearBottom(int tolerance) const
{
    auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea);
    if (!scroll) {
        return true;
    }
    QScrollBar *bar = scroll->verticalScrollBar();
    if (!bar) {
        return true;
    }
    return (bar->maximum() - bar->value()) <= tolerance;
}

void ChatView::scrollToBottom(bool force)
{
    if (!m_autoScroll) return;
    if (!force && !m_stickToBottom) return;

    m_stickToBottom = true;
    auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea);
    QScrollBar *bar = scroll ? scroll->verticalScrollBar() : nullptr;
    if (bar) {
        // Let word-wrap/layout settle and perform at most one movement per
        // frame. rangeChanged will request another frame only if needed.
        if (!m_scrollFollowTimer->isActive()) {
            m_scrollFollowTimer->start();
        }
    }
}

void ChatView::showWelcomeScreen(bool refresh)
{
    if (m_welcomeWidget) {
        if (m_chatLayout->indexOf(m_welcomeWidget) == -1) {
            m_chatLayout->insertWidget(0, m_welcomeWidget, 0, Qt::AlignCenter);
        }
        m_welcomeWidget->setVisible(true);
        m_welcomeWidget->show();
        m_welcomeWidget->raise();
        if (refresh) {
            refreshViewport();
        }
    }
}

void ChatView::hideWelcomeScreen(bool refresh)
{
    if (m_welcomeWidget) {
        m_welcomeWidget->setVisible(false);
        if (refresh) {
            refreshViewport();
        }
    }
}

void ChatView::showThinkingIndicator()
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
    if (ThemeController *theme = themeController()) {
        theme->registerStyle(m_thinkingIndicator, []() {
            return QString(
                "QLabel { "
                "  color: " + currentTheme().textMuted + "; "
                "  font-size: 15px; "
                "  font-style: italic; "
                "  padding: 12px 16px; "
                "}");
        });
    }

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

void ChatView::hideThinkingIndicator(bool refresh)
{
    m_thinkingTimer->stop();
    if (!m_thinkingIndicator) return;

    delete m_thinkingRowWidget;
    m_thinkingRowWidget = nullptr;
    m_thinkingIndicator = nullptr;
    if (refresh) {
        refreshViewport();
    }
}

void ChatView::applyTheme()
{
    // Message cards are created per message and own their own styles, so they
    // are not part of the controller's registry. The streaming card lives in
    // the layout too, so it is covered by the loop above.
    for (int i = 0; i < m_chatLayout->count(); ++i) {
        QLayoutItem *item = m_chatLayout->itemAt(i);
        if (auto *card = qobject_cast<ChatMessageCard *>(item->widget())) {
            card->applyTheme();
        }
    }
}

void ChatView::showSearchBar()
{
    if (m_searchBar) {
        m_searchBar->setVisible(true);
        m_searchBar->m_searchInput->setFocus();
    }
}

void ChatView::hideSearchBar()
{
    if (m_searchBar) {
        m_searchBar->setVisible(false);
        clearHighlights();
    }
}

void ChatView::search(const QString &text)
{
    // One pass over the visible cards: each card re-renders at most once, and
    // clearHighlight() is a no-op for cards that carry no highlight.
    m_highlightedCards.clear();
    m_highlightText = text;
    for (int i = 0; i < m_chatLayout->count(); ++i) {
        QLayoutItem *item = m_chatLayout->itemAt(i);
        if (auto *card = qobject_cast<ChatMessageCard *>(item->widget())) {
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
            m_highlightedCards.isEmpty() ? "0/0"
            : QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size()));
    }
}

void ChatView::findNext()
{
    if (m_highlightedCards.isEmpty()) return;
    m_currentHighlightIndex = (m_currentHighlightIndex + 1) % m_highlightedCards.size();
    if (auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea)) {
        scroll->ensureWidgetVisible(m_highlightedCards[m_currentHighlightIndex], 0, 120);
    }
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size()));
    }
}

void ChatView::findPrevious()
{
    if (m_highlightedCards.isEmpty()) return;
    m_currentHighlightIndex = (m_currentHighlightIndex - 1 + m_highlightedCards.size()) % m_highlightedCards.size();
    if (auto *scroll = qobject_cast<QScrollArea *>(m_scrollArea)) {
        scroll->ensureWidgetVisible(m_highlightedCards[m_currentHighlightIndex], 0, 120);
    }
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size()));
    }
}

void ChatView::clearHighlights()
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
