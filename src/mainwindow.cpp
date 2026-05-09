#include "mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollBar>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chatDisplay(new QTextEdit)
    , m_inputField(new QLineEdit)
    , m_sendButton(new QPushButton("Send"))
    , m_apiClient(new ApiClient(this))
    , m_settings("lagmajin", "lagmajinnoqt6")
{
    setWindowTitle("Venice.ai Client");
    resize(800, 600);

    setupUI();
    setupMenu();
    loadSettings();

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    connect(m_inputField, &QLineEdit::returnPressed, this, &MainWindow::onSendMessage);
    connect(m_apiClient, &ApiClient::responseReceived, this, &MainWindow::onResponseReceived);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &MainWindow::onErrorOccurred);
}

void MainWindow::setupUI()
{
    m_chatDisplay->setReadOnly(true);

    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->addWidget(m_chatDisplay);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_inputField);
    inputLayout->addWidget(m_sendButton);
    mainLayout->addLayout(inputLayout);
}

void MainWindow::setupMenu()
{
    QMenuBar *menuBar = this->menuBar();
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *settingsAction = fileMenu->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    QAction *clearAction = fileMenu->addAction("Clear Chat");
    connect(clearAction, &QAction::triggered, [this]() {
        m_chatDisplay->clear();
        m_messages.clear();
    });
}

void MainWindow::loadSettings()
{
    QString apiKey = m_settings.value("apiKey").toString();
    QString model = m_settings.value("model", "venice-uncensored").toString();

    m_apiClient->setApiKey(apiKey);
    m_apiClient->setModel(model);
}

void MainWindow::saveSettings()
{
    m_settings.sync();
}

bool MainWindow::checkApiKey()
{
    if (m_settings.value("apiKey").toString().isEmpty()) {
        QMessageBox::warning(this, "API Key Required", "Please set your Venice.ai API key in Settings.");
        return false;
    }
    return true;
}

void MainWindow::onSendMessage()
{
    QString text = m_inputField->text().trimmed();
    if (text.isEmpty() || !checkApiKey()) return;

    m_inputField->clear();
    m_chatDisplay->append("<b>You:</b> " + text.toHtmlEscaped());

    ChatMessage userMsg{"user", text};
    m_messages.append(userMsg);

    m_sendButton->setEnabled(false);
    m_inputField->setEnabled(false);
    m_chatDisplay->append("<i>Thinking...</i>");

    m_apiClient->sendMessage(m_messages);
}

void MainWindow::onResponseReceived(const QString &response)
{
    m_chatDisplay->document()->removeLastBlock();
    m_chatDisplay->append("<b>Assistant:</b> " + response.toHtmlEscaped());

    ChatMessage assistantMsg{"assistant", response};
    m_messages.append(assistantMsg);

    m_sendButton->setEnabled(true);
    m_inputField->setEnabled(true);
    m_inputField->setFocus();

    QScrollBar *bar = m_chatDisplay->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::onErrorOccurred(const QString &error)
{
    m_chatDisplay->document()->removeLastBlock();
    m_chatDisplay->append("<b>Error:</b> " + error.toHtmlEscaped());

    m_sendButton->setEnabled(true);
    m_inputField->setEnabled(true);
    m_inputField->setFocus();
}

void MainWindow::onSettings()
{
    bool ok;
    QString apiKey = QInputDialog::getText(this, "Settings", "Venice.ai API Key:", QLineEdit::Normal, m_settings.value("apiKey").toString(), &ok);
    if (ok) {
        m_settings.setValue("apiKey", apiKey);
        m_apiClient->setApiKey(apiKey);
    }

    QString model = QInputDialog::getText(this, "Settings", "Model:", QLineEdit::Normal, m_settings.value("model", "venice-uncensored").toString(), &ok);
    if (ok) {
        m_settings.setValue("model", model);
        m_apiClient->setModel(model);
    }

    saveSettings();
}
