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
#include "apiclient.h"

struct ChatSession {
    QString id;
    QString title;
    QList<ChatMessage> messages;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void onSendMessage();
    void onResponseReceived(const QString &response);
    void onErrorOccurred(const QString &error);
    void onSettings();
    void onModelsFetched(const QStringList &models);
    void onModelChanged(const QString &model);
    void onNewChat();
    void onChatSelected(QListWidgetItem *current, QListWidgetItem *previous);
    void onDeleteChat();

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

    QWidget *m_sidebar;
    QListWidget *m_chatList;
    QPushButton *m_newChatButton;
    QPushButton *m_deleteChatButton;
    QTextEdit *m_chatDisplay;
    QLineEdit *m_inputField;
    QPushButton *m_sendButton;
    QComboBox *m_modelCombo;
    QSplitter *m_splitter;
    ApiClient *m_apiClient;
    QList<ChatSession> m_chatSessions;
    int m_currentChatIndex;
    QSettings m_settings;
};

#endif // MAINWINDOW_H
