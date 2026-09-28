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
    void applyTheme();
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
    void showDraftWarning();
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
    bool loadChatBackupSnapshot(QJsonObject *snapshot) const;
    bool saveChatBackup();
    bool restoreChatFromBackup(const QString &chatId);
    QList<int> recoverableChatIndices() const;

    QWidget *m_sidebar;
    QLabel *m_appTitle;
    QLabel *m_headerTitle;
    QLabel *m_headerSubtitle;
    QLineEdit *m_searchField;
    QScrollArea *m_chatListScroll;
    QWidget *m_chatListContainer;
    QWidget *m_welcomeWidget;
    QPushButton *m_newChatButton;
    QScrollArea *m_scrollArea;
    QWidget *m_chatContainer;
    QVBoxLayout *m_chatLayout;
    QTextEdit *m_inputField;
    QToolButton *m_sendButton;
    QToolButton *m_attachButton;
    QComboBox *m_modelCombo;
    QComboBox *m_profileCombo;
    QCheckBox *m_uncensoredFilter;
    QCheckBox *m_streamToggle;
    QCheckBox *m_webSearchToggle;
    QCheckBox *m_autoScrollToggle;
    QSplitter *m_splitter;
    ApiClient *m_apiClient;
    QList<ChatSession> m_chatSessions;
    int m_currentChatIndex;
    QSettings m_settings;
    ChatMessageCard *m_streamingCard;
    QStringList m_allModels;
    QString m_currentChatImage;
    QLabel *m_imagePreview;
    QStatusBar *m_statusBar;
    QLabel *m_statusModel;
    QLabel *m_statusTokens;
    QLabel *m_statusConnection;
    QLabel *m_statusResponseTime;
    QLabel *m_statusContextUsage;
    bool m_isDarkTheme;
    QWidget *m_thinkingRowWidget;
    QLabel *m_thinkingIndicator;
    QTimer *m_thinkingTimer;
    int m_thinkingDots;
    QLabel *m_charCounter;
    ChatSearchBar *m_searchBar;
    QList<ChatMessageCard*> m_highlightedCards;
    int m_currentHighlightIndex;
    QList<ApiProfile> m_profiles;
    QString m_currentProfileName;
    bool m_autoScroll;
    bool m_stickToBottom;
    int m_chatRenderGeneration;
    int m_chatRenderCursor;
    int m_chatListRenderGeneration;
    QList<int> m_chatListRenderIndices;
    int m_chatListRenderCursor;
    int m_chatFontSize;
    int m_retryCount;
    int m_maxRetries;
    QTimer *m_retryTimer;
    QTimer *m_backupTimer;
    QList<ChatMessage> m_pendingMessages;
    QSoundEffect *m_notificationSound;
    bool m_soundInitialized;
    QDateTime m_chatStartTime;
    QLabel *m_statusDuration;
    QLabel *m_statusSpeed;
    qint64 m_streamStartTime;
    int m_streamTokenCount;
    QTimer *m_streamRenderTimer;
    QTimer *m_scrollFollowTimer;
    QString m_pendingStreamChunk;
    QString m_streamedContent;
    int m_requestChatIndex;
    bool m_requestInFlight;
    class ThemeController *m_theme;
};

#endif // MAINWINDOW_H
