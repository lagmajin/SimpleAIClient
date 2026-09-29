#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QComboBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QListWidget>
#include <QSplitter>
#include <QSettings>
#include <QDateTime>
#include <QScrollArea>
#include <QFrame>
#include <QLabel>
#include <QCheckBox>
#include <QToolButton>
#include <QTimer>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QShortcut>
#include <QStatusBar>
#include <QPixmap>
#include <QEnterEvent>
#include <QDateTime>
#include <QCloseEvent>
#include <QSoundEffect>
#include "secretstore.h"
#include "theme.h"
#include "chatwidgets.h"
#include <QJsonObject>
#include "apiclient.h"

struct ChatSession {
    QString id;
    QString title;
    QList<ChatMessage> messages;
    int messageCount = 0;
    bool pinned;
    int scrollPosition = 0;
    bool messagesLoaded = false;
};

struct ApiProfile {
    QString name;
    QString apiKey;
    QString model;
    QString systemPrompt;
    double temperature;
    int maxTokens;
};

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
    void saveChatSessions();
    void loadChatSessions();
    void saveChatMessages(int index);
    void loadChatMessages(int index);
    void unloadChatMessages(int index);
    QString generateChatTitle(const QString &firstMessage);
    void refreshChatViewport();
    void removeTrailingSpacer();
    void appendBottomSpacer();
    void rebuildCurrentChatView();
    void clearChatDisplay(bool refresh = true);
    void addMessageCard(const QString &role, const QString &content, int promptTokens = 0, int completionTokens = 0, int totalTokens = 0, int responseTimeMs = 0);
    ChatMessageCard* addMessageCardWithCard(const QString &role, const QString &content, int promptTokens = 0, int completionTokens = 0, int totalTokens = 0, int responseTimeMs = 0, bool prepend = false);
    void continueChatHistoryRender(int generation);
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
    void continueChatListRender(int generation);
    void saveCurrentChatScrollPosition();
    void restoreCurrentChatScrollPosition();
    void flushStreamingChunks();
    void saveDraft();
    void loadDraft();
    void clearDraft();
    void restyleCards();
    void updateCharCounter();
    // Returns false when the input is not a recognised command and the text
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
    void updateStreamingSpeed(const QString &chunk);
    void setRequestInFlight(bool inFlight);
    void beginRequest(int chatIndex);
    void rebuildCardsForMessages(const QList<ChatMessage> &messages);
    bool persistAssistantMessage(int chatIndex, const QString &content, int promptTokens, int completionTokens, int totalTokens);
    QString chatBackupFilePath() const;
    QJsonObject buildChatBackupSnapshot() const;
    QJsonObject previousSnapshotSession(const QString &chatId) const;
    bool loadChatBackupSnapshot(QJsonObject *snapshot) const;
    bool saveChatBackup();
    bool restoreChatFromBackup(const QString &chatId);
    QList<int> recoverableChatIndices() const;

    // Every pointer defaults to null: setupUI() registers a stylesheet builder
    // on several of these before the widget is created, and an indeterminate
    // pointer would be dereferenced there.
    QWidget *m_sidebar = nullptr;
    QLabel *m_appTitle = nullptr;
    QLabel *m_headerTitle = nullptr;
    QLabel *m_headerSubtitle = nullptr;
    QLineEdit *m_searchField = nullptr;
    QWidget *m_chatListContainer = nullptr;
    QWidget *m_welcomeWidget = nullptr;
    QPushButton *m_newChatButton = nullptr;
    QScrollArea *m_scrollArea = nullptr;
    QWidget *m_chatContainer = nullptr;
    QVBoxLayout *m_chatLayout = nullptr;
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
    ChatMessageCard *m_streamingCard = nullptr;
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
    QWidget *m_thinkingRowWidget = nullptr;
    QLabel *m_thinkingIndicator = nullptr;
    QTimer *m_thinkingTimer = nullptr;
    int m_thinkingDots = 0;
    QLabel *m_charCounter = nullptr;
    ChatSearchBar *m_searchBar = nullptr;
    QList<ChatMessageCard*> m_highlightedCards;
    int m_currentHighlightIndex = -1;
    QList<ApiProfile> m_profiles;
    QString m_currentProfileName;
    bool m_autoScroll = true;
    bool m_stickToBottom = true;
    int m_chatRenderGeneration = 0;
    int m_chatRenderCursor = -1;
    int m_chatListRenderGeneration = 0;
    QList<int> m_chatListRenderIndices;
    int m_chatListRenderCursor = 0;
    int m_chatFontSize = 15;
    int m_retryCount = 0;
    int m_maxRetries = 3;
    QTimer *m_retryTimer = nullptr;
    QTimer *m_backupTimer = nullptr;
    QList<ChatMessage> m_pendingMessages;
    QSoundEffect *m_notificationSound = nullptr;
    bool m_soundInitialized = false;
    QDateTime m_chatStartTime;
    QLabel *m_statusDuration = nullptr;
    QLabel *m_statusSpeed = nullptr;
    qint64 m_streamStartTime = 0;
    int m_streamTokenCount = 0;
    QTimer *m_streamRenderTimer = nullptr;
    QTimer *m_scrollFollowTimer = nullptr;
    QString m_pendingStreamChunk;
    QString m_streamedContent;
    // Captured by the char counter's stylesheet builder, which is re-registered
    // on every keystroke, so the state it reads has to outlive the call.
    int m_charCounterStyleLength = 0;
    bool m_charCounterOverLimit = false;
    int m_requestChatIndex = -1;
    bool m_requestInFlight = false;
    class ThemeController *m_theme = nullptr;
};

#endif // MAINWINDOW_H
