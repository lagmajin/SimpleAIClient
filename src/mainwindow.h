#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCheckBox>
#include <QCloseEvent>
#include <QComboBox>
#include <QDateTime>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QList>
#include <QMainWindow>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSoundEffect>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "apiclient.h"
#include "chatsession.h"
#include "chatsessionlist.h"
#include "chatview.h"
#include "chatsessionlist.h"
#include "chatview.h"
#include "chatwidgets.h"
#include "secretstore.h"
#include "theme.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onSendMessage();
    void onRequestCancelled();
    void onResponseReceived(const QString &response, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs);
    void onResponseChunk(const QString &chunk);
    void onResponseFinished(int responseTimeMs);
    void onErrorOccurred(const QString &error);
    void onSettings();
    void onExportChat();
    void onAdvancedSettings();
    void onToggleTheme();
    void onRegenerateResponse(int messageIndex);
    void onBranchConversation(int messageIndex);
    void onEditMessage(int messageIndex, const QString &newContent);
    void onAttachImage();
    bool attachImageFromPath(const QString &filePath);
    void onRecoverChats();
    void onExportBackupSnapshot();
    void onModelsFetched(const QStringList &models);
    void onModelChanged(const QString &model);
    void onNewChat();
    void onDeleteChat();
    void onProfileChanged();
    void onManageProfiles();
    void onSearchTextChanged(const QString &text);
    void onFindNext();
    void onFindPrevious();
    void onToggleAutoScroll();
    void onApiErrorWithRetry(const QString &error);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    void setupUI();
    void setupMenu();
    void loadSettings();
    void saveSettings();
    bool checkApiKey();
    void fetchModels();
    void restoreModelSelection();
    int chatIndexForId(const QString &id) const;
    void restoreLastChatSelection();
    void createNewChat();
    void switchToChat(int index, bool force = false);
    void updateChatList();
    // Thin delegates to ChatStore, which owns everything that touches
    // QSettings. They exist so the call sites read as "save the chats" rather
    // than "reach into the persistence object".
    void saveChatSessions();
    void loadChatSessions();
    void saveChatMessages(int index);
    void loadChatMessages(int index);
    void unloadChatMessages(int index);
    // The id-taking forms are the store's; the no-argument ones above and
    // below operate on the current chat and are what the UI calls.
    void storeDraft(const QString &chatId, const QString &draft);
    QString storedDraft(const QString &chatId) const;
    void removeDraft(const QString &chatId);
    QList<int> recoverableChatIndices() const;
    bool restoreChatFromBackup(const QString &chatId);
    QString currentChatId() const;
    QString generateChatTitle(const QString &firstMessage);
    // Thin delegates to ChatView, which owns the conversation pane. Same
    // names and signatures as before, so the call sites are untouched.
    void initChatView();
    void pointViewAtCurrentChat();
    void rebuildCurrentChatView();
    void clearChatDisplay(bool refresh = true);
    void addMessageCard(const QString &role, const QString &content, int promptTokens = 0, int completionTokens = 0, int totalTokens = 0, int responseTimeMs = 0);
    ChatMessageCard* addMessageCardWithCard(const QString &role, const QString &content, int promptTokens = 0, int completionTokens = 0, int totalTokens = 0, int responseTimeMs = 0, bool prepend = false);
    void scrollToBottom(bool force = true);
    bool isNearBottom(int tolerance = 48) const;
    void applyModelFilter();
    void deleteChatAtRow(int row);
    void renameChat(int row);
    void togglePinChat(int row);
    void showWelcomeScreen(bool refresh = true);
    void hideWelcomeScreen(bool refresh = true);
    void showThinkingIndicator();
    void hideThinkingIndicator(bool refresh = true);
    void filterChats(const QString &query);
    void saveCurrentChatScrollPosition();
    void restoreCurrentChatScrollPosition();
    void flushStreamingChunks();
    void refreshChatViewport();
    void saveDraft();
    void loadDraft();
    void clearDraft();
    void restyleCards();
    void updateCharCounter();    // Returns false when the input is not a recognised command and the text
    // was put back into the input field.
    bool handleQuickCommand(const QString &command);
    void loadProfiles();
    void saveProfiles();
    void applyProfile(const ApiProfile &profile);
    void updateHeaderState();
    void updateContextUsage();
    void showSearchBar();
    void hideSearchBar();
    void clearHighlights();
    void adjustFontSize(int delta);
    void resetFontSize();
    void applyChatFontSize();
    void playNotificationSound();
    void updateChatDuration();
    void showShortcutsDialog();
    void updateStreamingSpeed(int renderedWords);
    void setRequestInFlight(bool inFlight);
    void beginRequest(int chatIndex);
    void rebuildCardsForMessages(const QList<ChatMessage> &messages);
    bool persistAssistantMessage(int chatIndex, const QString &content, int promptTokens, int completionTokens, int totalTokens);

    // Every pointer defaults to null: setupUI() registers a stylesheet builder
    // on several of these before the widget is created, and an indeterminate
    // pointer would be dereferenced there.
    QWidget *m_sidebar = nullptr;
    QLabel *m_appTitle = nullptr;
    QLabel *m_headerTitle = nullptr;
    QLabel *m_headerSubtitle = nullptr;
    QLineEdit *m_searchField = nullptr;
    QWidget *m_chatListContainer = nullptr;
    // The view drives these; the window builds them and passes them over.
    QWidget *m_welcomeWidget = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_chatContainer = nullptr;
    QVBoxLayout *m_chatLayout = nullptr;
    QPushButton *m_newChatButton = nullptr;
    QTextEdit *m_inputField = nullptr;
    QToolButton *m_sendButton = nullptr;
    QToolButton *m_attachButton = nullptr;
    QComboBox *m_modelCombo = nullptr;
    QComboBox *m_profileCombo = nullptr;
    QCheckBox *m_uncensoredFilter = nullptr;
    QCheckBox *m_streamToggle = nullptr;
    QCheckBox *m_webSearchToggle = nullptr;
    QCheckBox *m_autoScrollToggle = nullptr;
    QSplitter *m_splitter = nullptr;
    ApiClient *m_apiClient = nullptr;
    QList<ChatSession> m_chatSessions;
    int m_currentChatIndex = -1;
    QSettings m_settings;
    QStringList m_allModels;
    QString m_currentChatImage;
    QLabel *m_imagePreview = nullptr;
    QStatusBar *m_statusBar = nullptr;
    QLabel *m_statusModel = nullptr;
    QLabel *m_statusTokens = nullptr;
    QLabel *m_statusConnection = nullptr;
    QLabel *m_statusResponseTime = nullptr;
    QLabel *m_statusContextUsage = nullptr;
    bool m_isDarkTheme = true;
    QLabel *m_charCounter = nullptr;
    ChatSearchBar *m_searchBar = nullptr;
    QList<ApiProfile> m_profiles;
    QString m_currentProfileName;
    int m_chatFontSize = 15;
    // Full text of the answer being streamed, kept so a view rebuild mid-stream
    // can restore the card instead of losing what already arrived.
    QString m_streamedContent;
    int m_retryCount = 0;
    int m_maxRetries = 3;
    QTimer *m_retryTimer = nullptr;
    // Owns every QSettings read and write, including the debounced backup.
    // Owns the conversation pane: cards, lazy render, scroll, search.
    class ChatView *m_view = nullptr;
    class ChatSessionList *m_sessionList = nullptr;
    // Owns every QSettings read and write, including the debounced backup.
    class ChatStore *m_store = nullptr;
    QList<ChatMessage> m_pendingMessages;
    QSoundEffect *m_notificationSound = nullptr;
    bool m_soundInitialized = false;
    QDateTime m_chatStartTime;
    QLabel *m_statusDuration = nullptr;
    QLabel *m_statusSpeed = nullptr;
    qint64 m_streamStartTime = 0;
    int m_streamTokenCount = 0;
    // Captured by the char counter's stylesheet builder, which is re-registered
    // on every keystroke, so the state it reads has to outlive the call.
    int m_charCounterStyleLength = 0;
    bool m_charCounterOverLimit = false;
    int m_requestChatIndex = -1;
    bool m_requestInFlight = false;
    class ThemeController *m_theme = nullptr;
};

#endif // MAINWINDOW_H
