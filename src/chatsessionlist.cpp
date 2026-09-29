// The sidebar chat list.
//
// Extracted from MainWindow: it owns the filter, the render generation and the
// rows still to draw, and needs nothing from the window except the session list
// and someone to tell what the user clicked.

#include "chatsessionlist.h"

#include "chatsession.h"
#include "chatwidgets.h"

#include <QElapsedTimer>
#include <QLineEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

ChatSessionList::ChatSessionList(QWidget *container, QLineEdit *searchField,
                                 QList<ChatSession> *sessions, QObject *parent)
    : QObject(parent)
    , m_container(container)
    , m_searchField(searchField)
    , m_sessions(sessions)
{
}

void ChatSessionList::refresh()
{
    filter(m_searchField ? m_searchField->text() : QString());
}

void ChatSessionList::setCurrentIndex(int index)
{
    m_currentIndex = index;
    // Updates the rows already drawn rather than rebuilding the list, so
    // switching chats does not lose the scroll position or re-run the batches.
    if (m_container) {
        const auto rows = m_container->findChildren<ChatListItem *>();
        for (ChatListItem *row : rows) {
            row->setActive(row->index() == index);
        }
    }
}

void ChatSessionList::filter(const QString &query)
{
    auto *layout = m_container ? qobject_cast<QVBoxLayout *>(m_container->layout()) : nullptr;
    if (!layout) {
        return;
    }

    QLayoutItem *item;
    while (layout->count() > 0) {
        item = layout->takeAt(0);
        if (item->widget()) {
            // The list can be rebuilt from a widget's own context-menu
            // callback. Deleting that widget synchronously would destroy the
            // sender while Qt is still dispatching its event.
            item->widget()->deleteLater();
        }
        delete item;
    }

    const QString lowerQuery = query.toLower();
    ++m_renderGeneration;
    const int renderGeneration = m_renderGeneration;
    m_renderIndices.clear();

    QList<int> pinnedIndices;
    QList<int> unpinnedIndices;
    for (int i = 0; i < m_sessions->size(); ++i) {
        if (!lowerQuery.isEmpty() && !m_sessions->at(i).title.toLower().contains(lowerQuery)) {
            continue;
        }
        if (m_sessions->at(i).pinned) {
            pinnedIndices.append(i);
        } else {
            unpinnedIndices.append(i);
        }
    }

    m_renderIndices = pinnedIndices + unpinnedIndices;
    m_renderCursor = 0;

    layout->addStretch();
    continueRender(renderGeneration);
}

void ChatSessionList::continueRender(int generation)
{
    if (generation != m_renderGeneration) {
        return;
    }

    auto *layout = m_container ? qobject_cast<QVBoxLayout *>(m_container->layout()) : nullptr;
    if (!layout) {
        return;
    }

    const int total = m_renderIndices.size();
    if (m_renderCursor >= total) {
        return;
    }

    // Batched so a long history does not block the UI in one pass; the 8 ms
    // budget is short enough that the first rows appear immediately.
    constexpr int kBatchSize = 14;
    int rendered = 0;
    QElapsedTimer timer;
    timer.start();

    while (m_renderCursor < total && rendered < kBatchSize) {
        const int idx = m_renderIndices.at(m_renderCursor++);
        const ChatSession &chat = m_sessions->at(idx);

        QStringList metaParts;
        if (chat.messageCount == 0) {
            metaParts << "Empty";
        } else {
            metaParts << QString("%1 msg").arg(chat.messageCount);
        }
        if (chat.pinned) {
            metaParts << "Pinned";
        }

        auto *chatItem = new ChatListItem(chat.title, metaParts.join("  •  "),
                                          idx, chat.pinned, m_container);
        if (idx == m_currentIndex) {
            chatItem->setActive(true);
        }
        layout->insertWidget(layout->count() - 1, chatItem);

        // Queued: a click can be delivered after the list was rebuilt, and the
        // captured index is a session index, not a row.
        connect(chatItem, &ChatListItem::clicked, this, [this, idx]() { emit chatSelected(idx); },
                Qt::QueuedConnection);
        connect(chatItem, &ChatListItem::deleteClicked, this, [this, idx]() { emit deleteRequested(idx); },
                Qt::QueuedConnection);
        connect(chatItem, &ChatListItem::renameRequested, this, [this, idx]() { emit renameRequested(idx); },
                Qt::QueuedConnection);
        connect(chatItem, &ChatListItem::pinRequested, this, [this, idx]() { emit pinRequested(idx); },
                Qt::QueuedConnection);

        ++rendered;
        if (timer.elapsed() >= 8) {
            break;
        }
    }

    if (m_renderCursor < total) {
        QTimer::singleShot(0, this, [this, generation]() { continueRender(generation); });
    }
}
