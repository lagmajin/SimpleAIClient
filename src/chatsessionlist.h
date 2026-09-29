#ifndef CHATSESSIONLIST_H
#define CHATSESSIONLIST_H

#include <QList>
#include <QObject>
#include <QString>

class QLineEdit;
class QWidget;

// Struct, matching chatsession.h: a mismatched tag here silently makes every
// signature differ from the definition and only the linker notices.
struct ChatSession;

// The sidebar: the list of chats, filtered and rendered in batches.
//
// Extracted from MainWindow for the same reason as ChatView - it is one part of
// the UI with its own state (the filter query, the render generation and the
// rows still to draw), and it does not need the window for anything but the
// session list and a place to report what the user clicked.
//
// The list of sessions is borrowed, again because deleting, pinning and
// renaming all shift indices, and a second copy would be a second thing to keep
// in step.
class ChatSessionList : public QObject
{
    Q_OBJECT

public:
    ChatSessionList(QWidget *container, QLineEdit *searchField,
                    QList<ChatSession> *sessions, QObject *parent = nullptr);

    // Re-reads the search box and redraws.
    void refresh();
    // Draws only the sessions whose title contains `query`; an empty query
    // shows everything. Pinned chats sort first.
    void filter(const QString &query);
    // Marks one row as the open chat and clears the rest.
    void setCurrentIndex(int index);

signals:
    void chatSelected(int index);
    void deleteRequested(int index);
    void renameRequested(int index);
    void pinRequested(int index);

private:
    void continueRender(int generation);

    QWidget *m_container = nullptr;
    QLineEdit *m_searchField = nullptr;
    QList<ChatSession> *m_sessions = nullptr;

    QList<int> m_renderIndices;
    int m_renderGeneration = 0;
    int m_renderCursor = 0;
    int m_currentIndex = -1;
};

#endif // CHATSESSIONLIST_H
