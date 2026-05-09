#include "mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QScrollBar>
#include <QTextCursor>
#include <QMenu>
#include <QAction>
#include <QShortcut>
#include <QKeyEvent>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_chatDisplay(new QTextEdit)
    , m_inputField(new QLineEdit)
    , m_sendButton(new QPushButton("Send"))
    , m_modelCombo(new QComboBox)
    , m_newChatButton(new QPushButton("+ New Chat"))
    , m_deleteChatButton(new QPushButton("Delete"))
    , m_apiClient(new ApiClient(this))
    , m_currentChatIndex(-1)
    , m_settings("lagmajin", "SimpleAIClient")
{
    setWindowTitle("SimpleAIClient");
    resize(1200, 800);

    setupUI();
    setupMenu();
    loadSettings();
    loadChatSessions();

    if (m_chatSessions.isEmpty()) {
        createNewChat();
    }

    connect(m_sendButton, &QPushButton::clicked, this, &MainWindow::onSendMessage);
    connect(m_inputField, &QLineEdit::returnPressed, this, &MainWindow::onSendMessage);
    connect(m_apiClient, &ApiClient::responseReceived, this, &MainWindow::onResponseReceived);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_apiClient, &ApiClient::modelsFetched, this, &MainWindow::onModelsFetched);
    connect(m_modelCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &MainWindow::onModelChanged);
    connect(m_newChatButton, &QPushButton::clicked, this, &MainWindow::onNewChat);
    connect(m_chatList, &QListWidget::currentItemChanged, this, &MainWindow::onChatSelected);
    connect(m_deleteChatButton, &QPushButton::clicked, this, &MainWindow::onDeleteChat);

    updateChatList();
    fetchModels();
}

void MainWindow::setupUI()
{
    m_chatDisplay->setReadOnly(true);
    m_chatDisplay->setPlaceholderText("Send a message to start a new chat...");
    m_modelCombo->setEditable(true);
    m_modelCombo->setMinimumWidth(150);
    m_modelCombo->setMaximumWidth(200);
    m_inputField->setPlaceholderText("Message SimpleAIClient...");

    m_sidebar = new QWidget(this);
    m_sidebar->setMinimumWidth(220);
    m_sidebar->setMaximumWidth(300);
    m_sidebar->setStyleSheet("background-color: #202123; color: #ececf1;");

    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(8, 8, 8, 8);
    sidebarLayout->setSpacing(4);

    m_newChatButton->setStyleSheet(
        "QPushButton { "
        "  background-color: transparent; "
        "  color: #ececf1; "
        "  border: 1px solid #4d4d4f; "
        "  border-radius: 4px; "
        "  padding: 8px; "
        "  text-align: left; "
        "} "
        "QPushButton:hover { background-color: #2a2b32; }"
    );
    sidebarLayout->addWidget(m_newChatButton);

    m_chatList = new QListWidget(m_sidebar);
    m_chatList->setStyleSheet(
        "QListWidget { "
        "  background-color: transparent; "
        "  border: none; "
        "  color: #ececf1; "
        "} "
        "QListWidget::item { "
        "  padding: 8px; "
        "  border-radius: 4px; "
        "} "
        "QListWidget::item:selected { background-color: #2a2b32; } "
        "QListWidget::item:hover { background-color: #2a2b32; }"
    );
    sidebarLayout->addWidget(m_chatList, 1);

    m_deleteChatButton->setStyleSheet(
        "QPushButton { "
        "  background-color: transparent; "
        "  color: #ececf1; "
        "  border: 1px solid #4d4d4f; "
        "  border-radius: 4px; "
        "  padding: 6px; "
        "} "
        "QPushButton:hover { background-color: #2a2b32; }"
    );
    sidebarLayout->addWidget(m_deleteChatButton);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->addWidget(m_sidebar);

    QWidget *mainPanel = new QWidget(this);
    QVBoxLayout *mainLayout = new QVBoxLayout(mainPanel);
    mainLayout->setContentsMargins(12, 12, 12, 12);
    mainLayout->setSpacing(8);

    QHBoxLayout *topBar = new QHBoxLayout();
    topBar->addWidget(m_modelCombo);
    topBar->addStretch();
    mainLayout->addLayout(topBar);

    mainLayout->addWidget(m_chatDisplay, 1);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->addWidget(m_inputField, 1);
    inputLayout->addWidget(m_sendButton);
    mainLayout->addLayout(inputLayout);

    m_splitter->addWidget(mainPanel);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({260, 940});

    setCentralWidget(m_splitter);
}

void MainWindow::setupMenu()
{
    QMenuBar *menuBar = this->menuBar();
    QMenu *fileMenu = menuBar->addMenu("File");
    QAction *settingsAction = fileMenu->addAction("Settings");
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    QAction *clearAction = fileMenu->addAction("Clear Current Chat");
    connect(clearAction, &QAction::triggered, [this]() {
        if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
            m_chatSessions[m_currentChatIndex].messages.clear();
            m_chatDisplay->clear();
            saveChatSessions();
        }
    });
}

void MainWindow::loadSettings()
{
    QString apiKey = m_settings.value("apiKey").toString();
    QString model = m_settings.value("model", "venice-uncensored").toString();

    m_apiClient->setApiKey(apiKey);
    m_apiClient->setModel(model);
    m_modelCombo->addItem(model);
    m_modelCombo->setCurrentText(model);
}

void MainWindow::fetchModels()
{
    if (!checkApiKey()) return;
    m_modelCombo->clear();
    m_modelCombo->addItem("Loading models...");
    m_modelCombo->setEnabled(false);
    m_apiClient->fetchModels();
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

void MainWindow::createNewChat()
{
    ChatSession newChat;
    newChat.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    newChat.title = "New Chat";
    m_chatSessions.prepend(newChat);
    m_currentChatIndex = 0;
    m_chatDisplay->clear();
    updateChatList();
    saveChatSessions();
}

void MainWindow::switchToChat(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;

    m_currentChatIndex = index;
    m_chatDisplay->clear();

    const auto &chat = m_chatSessions[index];
    for (const auto &msg : chat.messages) {
        QString role = msg.role == "user" ? "You" : "Assistant";
        m_chatDisplay->append("<b>" + role + ":</b> " + msg.content.toHtmlEscaped());
    }

    QScrollBar *bar = m_chatDisplay->verticalScrollBar();
    bar->setValue(bar->maximum());
}

void MainWindow::updateChatList()
{
    m_chatList->clear();
    for (int i = 0; i < m_chatSessions.size(); ++i) {
        QListWidgetItem *item = new QListWidgetItem(m_chatSessions[i].title);
        item->setData(Qt::UserRole, i);
        m_chatList->addItem(item);
    }
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatList->count()) {
        m_chatList->setCurrentRow(m_currentChatIndex);
    }
}

void MainWindow::saveChatSessions()
{
    QJsonArray sessionsArray;
    for (const auto &chat : m_chatSessions) {
        QJsonObject sessionObj;
        sessionObj["id"] = chat.id;
        sessionObj["title"] = chat.title;

        QJsonArray messagesArray;
        for (const auto &msg : chat.messages) {
            QJsonObject msgObj;
            msgObj["role"] = msg.role;
            msgObj["content"] = msg.content;
            messagesArray.append(msgObj);
        }
        sessionObj["messages"] = messagesArray;
        sessionsArray.append(sessionObj);
    }

    QJsonDocument doc(sessionsArray);
    m_settings.setValue("chatSessions", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
}

void MainWindow::loadChatSessions()
{
    QString data = m_settings.value("chatSessions").toString();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isArray()) return;

    m_chatSessions.clear();
    for (const auto &val : doc.array()) {
        QJsonObject sessionObj = val.toObject();
        ChatSession chat;
        chat.id = sessionObj["id"].toString();
        chat.title = sessionObj["title"].toString();

        QJsonArray messagesArray = sessionObj["messages"].toArray();
        for (const auto &msgVal : messagesArray) {
            QJsonObject msgObj = msgVal.toObject();
            chat.messages.append({msgObj["role"].toString(), msgObj["content"].toString()});
        }
        m_chatSessions.append(chat);
    }

    if (!m_chatSessions.isEmpty()) {
        m_currentChatIndex = 0;
    }
}

QString MainWindow::generateChatTitle(const QString &firstMessage)
{
    QString title = firstMessage;
    if (title.length() > 30) {
        title = title.left(30) + "...";
    }
    return title;
}

void MainWindow::onSendMessage()
{
    QString text = m_inputField->text().trimmed();
    if (text.isEmpty() || !checkApiKey()) return;

    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        createNewChat();
    }

    m_inputField->clear();
    m_chatDisplay->append("<b>You:</b> " + text.toHtmlEscaped());

    ChatMessage userMsg{"user", text};
    m_chatSessions[m_currentChatIndex].messages.append(userMsg);

    if (m_chatSessions[m_currentChatIndex].messages.size() == 1) {
        m_chatSessions[m_currentChatIndex].title = generateChatTitle(text);
        updateChatList();
    }

    m_sendButton->setEnabled(false);
    m_inputField->setEnabled(false);
    m_chatDisplay->append("<i>Thinking...</i>");

    m_apiClient->sendMessage(m_chatSessions[m_currentChatIndex].messages);
    saveChatSessions();
}

void MainWindow::onResponseReceived(const QString &response)
{
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

    m_chatDisplay->append("<b>Assistant:</b> " + response.toHtmlEscaped());

    ChatMessage assistantMsg{"assistant", response};
    m_chatSessions[m_currentChatIndex].messages.append(assistantMsg);

    m_sendButton->setEnabled(true);
    m_inputField->setEnabled(true);
    m_inputField->setFocus();

    QScrollBar *bar = m_chatDisplay->verticalScrollBar();
    bar->setValue(bar->maximum());

    saveChatSessions();
}

void MainWindow::onErrorOccurred(const QString &error)
{
    QTextCursor cursor = m_chatDisplay->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
    cursor.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
    cursor.removeSelectedText();

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
        fetchModels();
    }

    saveSettings();
}

void MainWindow::onModelsFetched(const QStringList &models)
{
    m_modelCombo->clear();
    m_modelCombo->setEnabled(true);

    QString savedModel = m_settings.value("model", "venice-uncensored").toString();
    int defaultIndex = 0;

    for (int i = 0; i < models.size(); ++i) {
        m_modelCombo->addItem(models[i]);
        if (models[i] == savedModel) {
            defaultIndex = i;
        }
    }

    m_modelCombo->setCurrentIndex(defaultIndex);
    m_apiClient->setModel(m_modelCombo->currentText());
}

void MainWindow::onModelChanged(const QString &model)
{
    m_apiClient->setModel(model);
    m_settings.setValue("model", model);
}

void MainWindow::onNewChat()
{
    createNewChat();
    m_inputField->setFocus();
}

void MainWindow::onChatSelected(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    if (current) {
        int index = current->data(Qt::UserRole).toInt();
        switchToChat(index);
    }
}

void MainWindow::onDeleteChat()
{
    if (m_currentChatIndex < 0 || m_chatSessions.isEmpty()) return;

    if (m_chatSessions.size() == 1) {
        m_chatSessions.clear();
        m_currentChatIndex = -1;
        m_chatDisplay->clear();
        createNewChat();
    } else {
        m_chatSessions.removeAt(m_currentChatIndex);
        if (m_currentChatIndex >= m_chatSessions.size()) {
            m_currentChatIndex = m_chatSessions.size() - 1;
        }
        switchToChat(m_currentChatIndex);
    }

    updateChatList();
    saveChatSessions();
}
