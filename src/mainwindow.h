#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextEdit>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMenuBar>
#include <QSettings>
#include "apiclient.h"

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

private:
    void setupUI();
    void setupMenu();
    void loadSettings();
    void saveSettings();
    bool checkApiKey();

    QTextEdit *m_chatDisplay;
    QLineEdit *m_inputField;
    QPushButton *m_sendButton;
    ApiClient *m_apiClient;
    QList<ChatMessage> m_messages;
    QSettings m_settings;
};

#endif // MAINWINDOW_H
