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
#include <QSoundEffect>
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

class AvatarLabel : public QLabel {
public:
    AvatarLabel(const QString &role, QWidget *parent = nullptr);
};

class ChatListItem : public QWidget {
    Q_OBJECT
public:
    ChatListItem(const QString &title, const QString &subtitle, int index, bool isPinned, QWidget *parent = nullptr);
    int index() const { return m_index; }
    void setActive(bool active);

signals:
    void clicked(int index);
    void deleteClicked(int index);
    void renameRequested(int index);
    void pinRequested(int index);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    bool m_isActive;
    int m_index;
    bool m_isPinned;
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
    QLabel *m_subtitleLabel;
    QToolButton *m_deleteBtn;
};

class ChatMessageCard : public QWidget {
    Q_OBJECT
public:
    ChatMessageCard(const QString &role, const QString &content, QWidget *parent = nullptr);
    void appendContent(const QString &content);
    void setContent(const QString &content);
    void showCopyButton(bool show);
    void setStreaming(bool streaming);
    void setTokenInfo(int promptTokens, int completionTokens, int totalTokens, int responseTimeMs = 0);
    void setMessageIndex(int index);
    void showRegenerateButton(bool show);
    void showBranchButton(bool show);
    void setEditable(bool editable);
    void startEditing();
    QString content() const { return m_fullContent; }
    void highlightText(const QString &text);
    void clearHighlight();
    void setTimestamp(const QDateTime &timestamp);

protected:
    void enterEvent(QEnterEvent *event) override;
    void leaveEvent(QEvent *event) override;

signals:
    void copyRequested(const QString &text);
    void regenerateRequested();
    void branchRequested(int messageIndex);
    void editRequested(int messageIndex, const QString &newContent);

private:
    void renderMarkdown(const QString &text);
    QString escapeHtml(const QString &text);
    QString renderInlineMarkdown(const QString &text);
    void addCodeBlock(const QString &code, const QString &language);
    void addTextBlock(const QString &text);
    void rebuildContent();

    QString m_role;
    QString m_fullContent;
    QVBoxLayout *m_outerLayout;
    AvatarLabel *m_avatar;
    QFrame *m_card;
    QVBoxLayout *m_layout;
    QWidget *m_copyContainer;
    QPushButton *m_copyBtn;
    QLabel *m_streamLabel;
    QLabel *m_tokenLabel;
    QPushButton *m_regenerateBtn;
    QPushButton *m_branchBtn;
    QPushButton *m_editBtn;
    QTextEdit *m_editField;
    int m_messageIndex;
    bool m_isStreaming;
    QDateTime m_timestamp;
    QLabel *m_timestampLabel;
};

class ChatSearchBar : public QFrame {
    Q_OBJECT
public:
    ChatSearchBar(QWidget *parent = nullptr);
    QLineEdit *m_searchInput;
    QLabel *m_matchCount;

signals:
    void searchTextChanged(const QString &text);
    void findNext();
    void findPrevious();
    void closed();

private slots:
    void onClose();

private:
    QPushButton *m_prevBtn;
    QPushButton *m_nextBtn;
    QPushButton *m_closeBtn;
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
    void onRegenerateResponse();
    void onBranchConversation(int messageIndex);
    void onEditMessage(int messageIndex, const QString &newContent);
    void onAttachImage();
    void onModelsFetched(const QStringList &models);
    void onModelChanged(const QString &model);
    void onNewChat();
    void onChatSelected(QListWidgetItem *current, QListWidgetItem *previous);
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
    void createNewChat();
    void switchToChat(int index);
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
    void updateCharCounter();
    void handleQuickCommand(const QString &command);
    void loadProfiles();
    void saveProfiles();
    void applyProfile(const ApiProfile &profile);
    void updateHeaderState();
    void updateContextUsage();
    void showSearchBar();
    void hideSearchBar();
    void highlightInChat(const QString &text);
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
    bool m_scrollToBottomQueued;
    bool m_scrollToBottomForcePending;
    int m_chatRenderGeneration;
    int m_chatRenderCursor;
    int m_chatListRenderGeneration;
    QList<int> m_chatListRenderIndices;
    int m_chatListRenderCursor;
    int m_chatFontSize;
    int m_retryCount;
    int m_maxRetries;
    QTimer *m_retryTimer;
    QList<ChatMessage> m_pendingMessages;
    QSoundEffect *m_notificationSound;
    bool m_soundInitialized;
    QDateTime m_chatStartTime;
    QLabel *m_statusDuration;
    QLabel *m_statusSpeed;
    qint64 m_streamStartTime;
    int m_streamTokenCount;
    QTimer *m_streamRenderTimer;
    QString m_pendingStreamChunk;
    bool m_requestInFlight;
};

#endif // MAINWINDOW_H
