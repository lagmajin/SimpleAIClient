#ifndef CHATVIEW_H
#define CHATVIEW_H

#include <QHash>
#include <QList>
#include <QObject>
#include <QString>

class QLabel;
class QScrollArea;
class QTimer;
class QWidget;
class QScrollArea;
class QVBoxLayout;

// Struct, not class: the definition comes from apiclient.h, and mainwindow.h
// does too, so a mismatched tag here silently makes every ChatView signature
// differ from its definition.
struct ChatMessage;
class ChatMessageCard;
class ChatSearchBar;

// The conversation pane: the message cards, the batched lazy history render,
// scroll following, the welcome and thinking rows, and in-chat search.
//
// Extracted from MainWindow because it is the one part of the window that owns
// a coherent piece of UI state. Everything here used to be a MainWindow method
// reaching across ~15 members; now those members live here, and MainWindow
// only asks the view to do things.
//
// The view does not own the message data. It borrows the session list, because
// the render cursor, the scroll position and the card indices all have to stay
// consistent with the model that MainWindow is mutating as a turn streams in.
class ChatView : public QObject
{
    Q_OBJECT

public:
    ChatView(QWidget *scrollArea,
             QWidget *container,
             QWidget *welcomeWidget,
             ChatSearchBar *searchBar,
             QList<ChatMessage> *messages,
             QObject *parent = nullptr);

    // The view does not own the message data. It borrows the list of the chat
    // being shown, because the render cursor, the card indices and the stream
    // seed all have to stay consistent with what the window is mutating. The
    // pointer is re-pointed whenever the current chat changes.
    void setMessages(QList<ChatMessage> *messages);

    void setFontSize(int pixels);
    void setAutoScroll(bool enabled);
    void setScrollPersistence(int chatIndex);

signals:
    // A card asked for one of the three actions that change the conversation.
    void regenerateRequested(int messageIndex);
    void branchRequested(int messageIndex);
    void editRequested(int messageIndex, const QString &newContent);
    // A batch of the stream was painted, so the window can update its
    // throughput readout. The count is word-ish, matching the old heuristic.
    void chunkRendered(int wordish);

public:
    // --- rendering ---------------------------------------------------------
    void rebuild();
    void clear(bool refresh = true);
    ChatMessageCard *addCard(const QString &role, const QString &content,
                             int promptTokens = 0, int completionTokens = 0,
                             int totalTokens = 0, int responseTimeMs = 0);
    void rebuildCards(const QList<ChatMessage> &messages);
    void refreshViewport();
    void applyTheme();

    // --- streaming ---------------------------------------------------------
    void beginStream(const QString &seedText);
    void appendStreamChunk(const QString &chunk);
    void endStream();
    // Drops the live card and any pending chunk, so a retry or an error does
    // not append to the previous attempt.
    void clearStream();

    // --- scroll ------------------------------------------------------------
    void saveScrollPosition();
    void restoreScrollPosition();
    void scrollToBottom(bool force = true);
    bool isNearBottom(int tolerance = 48) const;
    bool stickToBottom() const { return m_stickToBottom; }
    void scrollFollowStarted();

    // --- placeholders ------------------------------------------------------
    void showWelcomeScreen(bool refresh = true);
    void hideWelcomeScreen(bool refresh = true);
    void showThinkingIndicator();
    void hideThinkingIndicator(bool refresh = true);

    // --- search ------------------------------------------------------------
    void showSearchBar();
    void hideSearchBar();
    void search(const QString &text);
    void findNext();
    void findPrevious();
    void clearHighlights();
    QString searchText() const { return m_highlightText; }

private:
    ChatMessageCard *addMessageCardRow(const QString &role, const QString &displayText,
                           int promptTokens, int completionTokens,
                           int totalTokens, int responseTimeMs, bool prepend);
    void continueChatHistoryRender(int generation);
    void flushStreamingChunks();
    void removeTrailingSpacer();
    void appendBottomSpacer();

    QWidget *m_scrollArea = nullptr;
    QWidget *m_chatContainer = nullptr;
    QWidget *m_welcomeWidget = nullptr;
    QVBoxLayout *m_chatLayout = nullptr;
    ChatSearchBar *m_searchBar = nullptr;
    // Borrowed: the window mutates the list while a turn streams in.
    QList<ChatMessage> *m_messages = nullptr;
    // Keyed by chat index. Scroll position is UI state, so it belongs to the
    // view, but it is written on every scroll event so copying it per chat
    // into the session struct would be churn.
    QHash<int, int> m_savedScrollPositions;

    ChatMessageCard *m_streamingCard = nullptr;
    QWidget *m_thinkingRowWidget = nullptr;
    QLabel *m_thinkingIndicator = nullptr;
    QTimer *m_thinkingTimer = nullptr;
    QTimer *m_streamRenderTimer = nullptr;
    QTimer *m_scrollFollowTimer = nullptr;

    QString m_pendingStreamChunk;
    QList<ChatMessageCard *> m_highlightedCards;
    QString m_highlightText;
    int m_currentHighlightIndex = -1;
    int m_currentChatIndex = -1;

    // Batched lazy render: cards are inserted from the end so a long
    // conversation does not block the UI on one pass.
    int m_chatRenderGeneration = 0;
    int m_chatRenderCursor = -1;

    bool m_stickToBottom = true;
    bool m_autoScroll = true;
    int m_fontSize = 15;
    int m_thinkingDots = 0;
};

#endif // CHATVIEW_H
