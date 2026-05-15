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
#include "apiclient.h"

struct ChatSession {
    QString id;
    QString title;
    QList<ChatMessage> messages;
    bool pinned;
};

class AvatarLabel : public QLabel {
public:
    AvatarLabel(const QString &role, QWidget *parent = nullptr);
};

class ChatListItem : public QWidget {
    Q_OBJECT
public:
    ChatListItem(const QString &title, int index, QWidget *parent = nullptr);
    int index() const { return m_index; }

signals:
    void clicked(int index);
    void deleteClicked(int index);
    void renameRequested(int index);
    void pinRequested(int index);

protected:
    void enterEvent(QEvent *event) override;
    void leaveEvent(QEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

private:
    int m_index;
    QLabel *m_iconLabel;
    QLabel *m_titleLabel;
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
    void setContent(const QString &content);
    void setContent(const QString &content);

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
    QPushButton *m_copyBtn;
    QPushButton *m_regenerateBtn;
    QPushButton *m_branchBtn;
    QPushButton *m_editBtn;
    QTextEdit *m_editField;
    int m_messageIndex;
    bool m_isStreaming;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onSendMessage();
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
    void onEditMessage(int messageIndex, const QString &newContent);
    void onAttachImage();
    void onModelsFetched(const QStringList &models);
    void onModelChanged(const QString &model);
    void onNewChat();
    void onChatSelected(QListWidgetItem *current, QListWidgetItem *previous);
    void onDeleteChat();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    void setupUI();
    void setupMenu();
    void loadSettings();
    void saveSettings();
    bool checkApiKey();
    void fetchModels();
    void createNewChat();
    void switchToChat(int index);
    void updateChatList();
    void saveChatSessions();
    void loadChatSessions();
    QString generateChatTitle(const QString &firstMessage);
    void clearChatDisplay();
    void addMessageCard(const QString &role, const QString &content, int promptTokens = 0, int completionTokens = 0, int totalTokens = 0, int responseTimeMs = 0);
    void scrollToBottom();
    void applyModelFilter();
    void deleteChatAtRow(int row);
    void renameChat(int row);
    void togglePinChat(int row);
    void showWelcomeScreen();
    void hideWelcomeScreen();
    void filterChats(const QString &query);
    void saveDraft();
    void loadDraft();
    void clearDraft();
    void applyTheme();

    QWidget *m_sidebar;
    QLabel *m_appTitle;
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
    QCheckBox *m_uncensoredFilter;
    QCheckBox *m_streamToggle;
    QCheckBox *m_webSearchToggle;
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
    bool m_isDarkTheme;
};

#endif // MAINWINDOW_H
