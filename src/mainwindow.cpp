#include "mainwindow.h"
#include <QInputDialog>
#include <QMessageBox>
#include <QMenuBar>
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
#include <QEvent>
#include <QDebug>
#include <QClipboard>
#include <QApplication>
#include <QTextEdit>
#include <QRegularExpression>
#include <QPainter>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QStyle>
#include <QLayout>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QPen>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QPalette>
#include <QDialog>
#include <QFormLayout>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPixmap>
#include <QRegularExpression>

AvatarLabel::AvatarLabel(const QString &role, QWidget *parent)
    : QLabel(parent)
{
    setFixedSize(32, 32);
    setAlignment(Qt::AlignCenter);

    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    if (role == "user") {
        painter.setBrush(QColor("#005c4b"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(2, 2, 28, 28);

        painter.setPen(QColor("#ffffff"));
        QFont font = painter.font();
        font.setPointSize(14);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "U");
    } else if (role == "error") {
        painter.setBrush(QColor("#8b0000"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(2, 2, 28, 28);

        painter.setPen(QColor("#ffffff"));
        QFont font = painter.font();
        font.setPointSize(16);
        painter.setFont(font);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "!");
    } else {
        painter.setBrush(QColor("#7c3aed"));
        painter.setPen(Qt::NoPen);
        painter.drawEllipse(2, 2, 28, 28);

        painter.setPen(QColor("#ffffff"));
        QFont font = painter.font();
        font.setPointSize(14);
        font.setBold(true);
        painter.setFont(font);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "AI");
    }

    setPixmap(pixmap);
}

ChatListItem::ChatListItem(const QString &title, int index, QWidget *parent)
    : QWidget(parent), m_index(index)
{
    setStyleSheet(
        "QWidget { "
        "  background-color: transparent; "
        "  border-radius: 6px; "
        "}"
    );

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(8, 6, 4, 6);
    layout->setSpacing(6);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(16, 16);
    QPixmap iconPixmap(16, 16);
    iconPixmap.fill(Qt::transparent);
    QPainter iconPainter(&iconPixmap);
    iconPainter.setRenderHint(QPainter::Antialiasing);
    iconPainter.setPen(QPen(QColor("#8e8e8e"), 1.5));
    iconPainter.setBrush(Qt::NoBrush);
    iconPainter.drawRoundedRect(2, 3, 12, 10, 2, 2);
    iconPainter.drawLine(6, 3, 6, 1);
    iconPainter.drawLine(10, 3, 10, 1);
    m_iconLabel->setPixmap(iconPixmap);
    layout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);

    m_titleLabel = new QLabel(title, this);
    m_titleLabel->setStyleSheet("QLabel { color: #ececf1; font-size: 14px; }");
    m_titleLabel->setWordWrap(false);
    layout->addWidget(m_titleLabel, 1);

    m_deleteBtn = new QToolButton(this);
    m_deleteBtn->setIcon(style()->standardIcon(QStyle::SP_TrashIcon));
    m_deleteBtn->setFixedSize(20, 20);
    m_deleteBtn->setStyleSheet(
        "QToolButton { "
        "  background-color: transparent; "
        "  border: none; "
        "  padding: 2px; "
        "} "
        "QToolButton:hover { background-color: #404040; border-radius: 4px; }"
    );
    m_deleteBtn->setVisible(false);
    layout->addWidget(m_deleteBtn);

    connect(m_deleteBtn, &QToolButton::clicked, [this]() {
        emit deleteClicked(m_index);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, [this](const QPoint &pos) {
        QMenu menu(this);
    QAction *renameAction = menu.addAction("Rename");
    QAction *pinAction = menu.addAction(m_chatSessions[row].pinned ? "Unpin" : "Pin");
    QAction *chosen = menu.exec(mapToGlobal(pos));
    if (chosen == renameAction) {
        emit renameRequested(m_index);
    } else if (chosen == pinAction) {
        emit pinRequested(m_index);
    }
}
    });
}

void ChatListItem::enterEvent(QEvent *event)
{
    QWidget::enterEvent(event);
    m_deleteBtn->setVisible(true);
    setStyleSheet("QWidget { background-color: #2a2b32; border-radius: 6px; }");
}

void ChatListItem::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_deleteBtn->setVisible(false);
    setStyleSheet("QWidget { background-color: transparent; border-radius: 6px; }");
}

void ChatListItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_index);
    }
    QWidget::mousePressEvent(event);
}

ChatMessageCard::ChatMessageCard(const QString &role, const QString &content, QWidget *parent)
    : QWidget(parent), m_role(role), m_fullContent(content), m_copyBtn(nullptr), m_streamLabel(nullptr), m_tokenLabel(nullptr), m_regenerateBtn(nullptr), m_branchBtn(nullptr), m_editBtn(nullptr), m_editField(nullptr), m_messageIndex(-1), m_isStreaming(false)
{
    m_outerLayout = new QVBoxLayout(this);
    m_outerLayout->setContentsMargins(0, 8, 0, 8);
    m_outerLayout->setSpacing(4);

    QHBoxLayout *rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    m_avatar = new AvatarLabel(role, this);

    if (role == "user") {
        rowLayout->addStretch();
        rowLayout->addWidget(m_avatar, 0, Qt::AlignTop | Qt::AlignRight);
    } else {
        rowLayout->addWidget(m_avatar, 0, Qt::AlignTop | Qt::AlignLeft);
    }

    m_card = new QFrame(this);
    QString bgColor = (role == "user" ? "#005c4b" : (role == "error" ? "#4a1515" : "#2f2f2f"));
    m_card->setStyleSheet(
        "QFrame { "
        "  background-color: " + bgColor + "; "
        "  border-radius: 12px; "
        "  padding: 12px 16px; "
        "}"
    );
    m_card->setMaximumWidth(700);

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(10);
    shadow->setXOffset(0);
    shadow->setYOffset(2);
    shadow->setColor(QColor(0, 0, 0, 40));
    m_card->setGraphicsEffect(shadow);

    m_layout = new QVBoxLayout(m_card);
    m_layout->setContentsMargins(12, 12, 12, 12);
    m_layout->setSpacing(8);

    if (!content.isEmpty()) {
        renderMarkdown(content);
    }

    m_copyContainer = new QWidget(m_card);
    m_copyContainer->setVisible(false);
    QHBoxLayout *copyLayout = new QHBoxLayout(m_copyContainer);
    copyLayout->setContentsMargins(0, 0, 0, 0);
    copyLayout->addStretch();

    m_copyBtn = new QPushButton("Copy", m_copyContainer);
    m_copyBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #404040; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 10px; "
        "  font-size: 12px; "
    );
    m_copyBtn->setMaximumWidth(50);
    copyLayout->addWidget(m_copyBtn);

    m_regenerateBtn = new QPushButton("Regenerate", m_card);
    m_regenerateBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #404040; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 10px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #505050; }"
    );
    m_regenerateBtn->setMaximumWidth(80);
    m_regenerateBtn->setVisible(false);
    copyLayout->addWidget(m_regenerateBtn);
    connect(m_regenerateBtn, &QPushButton::clicked, this, &ChatMessageCard::regenerateRequested);

    m_branchBtn = new QPushButton("Branch", m_card);
    m_branchBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #404040; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 10px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #505050; }"
    );
    m_branchBtn->setMaximumWidth(60);
    m_branchBtn->setVisible(false);
    copyLayout->addWidget(m_branchBtn);
    connect(m_branchBtn, &QPushButton::clicked, [this]() {
        emit branchRequested(m_messageIndex);
    });

    m_editBtn = new QPushButton("Edit", m_card);
    m_editBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #404040; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 10px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #505050; }"
    );
    m_editBtn->setMaximumWidth(50);
    m_editBtn->setVisible(false);
    copyLayout->addWidget(m_editBtn);
    connect(m_editBtn, &QPushButton::clicked, [this]() {
        startEditing();
    });

    m_layout->addWidget(m_copyContainer);

    rowLayout->addWidget(m_card, 0, role == "user" ? Qt::AlignRight : Qt::AlignLeft);

    if (role != "user") {
        rowLayout->addStretch();
    }

    m_outerLayout->addLayout(rowLayout);
}

void ChatMessageCard::appendContent(const QString &content)
{
    m_fullContent += content;

    if (m_isStreaming) {
        if (m_streamLabel) {
            m_streamLabel->setText(escapeHtml(m_fullContent));
        }
    } else {
        rebuildContent();
    }
}

void ChatMessageCard::setContent(const QString &content)
{
    m_fullContent = content;
    rebuildContent();
}

void ChatMessageCard::setStreaming(bool streaming)
{
    m_isStreaming = streaming;
    if (streaming) {
        if (!m_streamLabel) {
            while (m_layout->count() > 1) {
                QLayoutItem *item = m_layout->takeAt(0);
                delete item->widget();
                delete item;
            }
            m_streamLabel = new QLabel(m_card);
            m_streamLabel->setWordWrap(true);
            m_streamLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);
            m_streamLabel->setTextFormat(Qt::RichText);
            m_streamLabel->setStyleSheet(
                "QLabel { "
                "  color: #ececf1; "
                "  font-size: 15px; "
                "  line-height: 1.6; "
                "} "
                "QLabel a { color: #60a5fa; }"
            );
            m_layout->insertWidget(0, m_streamLabel);
        }
        m_streamLabel->setText(escapeHtml(m_fullContent));
    } else {
        if (m_streamLabel) {
            delete m_streamLabel;
            m_streamLabel = nullptr;
        }
        rebuildContent();
    }
}

void ChatMessageCard::showCopyButton(bool show)
{
    if (m_copyContainer) {
        m_copyContainer->setVisible(show);
    }
}

void ChatMessageCard::setTokenInfo(int promptTokens, int completionTokens, int totalTokens, int responseTimeMs)
{
    if (totalTokens > 0 && m_tokenLabel) {
        QString text = QString("Tokens: %1 prompt, %2 completion, %3 total").arg(promptTokens).arg(completionTokens).arg(totalTokens);
        if (responseTimeMs > 0) {
            text += QString(" | %.1fs").arg(responseTimeMs / 1000.0);
        }
        m_tokenLabel->setText(text);
        m_tokenLabel->setVisible(true);
    }
}

void ChatMessageCard::setMessageIndex(int index)
{
    m_messageIndex = index;
}

void ChatMessageCard::showRegenerateButton(bool show)
{
    if (m_regenerateBtn) {
        m_regenerateBtn->setVisible(show);
    }
}

void ChatMessageCard::showBranchButton(bool show)
{
    if (m_branchBtn) {
        m_branchBtn->setVisible(show);
    }
}

void ChatMessageCard::setEditable(bool editable)
{
    if (m_editBtn) {
        m_editBtn->setVisible(editable);
    }
}

void ChatMessageCard::startEditing()
{
    if (m_editField) return;

    m_editField = new QTextEdit(m_card);
    m_editField->setPlainText(m_fullContent);
    m_editField->setMaximumHeight(150);
    m_editField->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1a1a1a; "
        "  color: #ececf1; "
        "  border: 1px solid #4d4d4f; "
        "  border-radius: 6px; "
        "  padding: 8px; "
        "  font-size: 14px; "
        "}"
    );

    QHBoxLayout *editBtnLayout = new QHBoxLayout();
    editBtnLayout->setContentsMargins(0, 4, 0, 0);
    editBtnLayout->addStretch();

    QPushButton *saveBtn = new QPushButton("Save", m_card);
    saveBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #005c4b; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 12px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #007a5e; }"
    );
    QPushButton *cancelBtn = new QPushButton("Cancel", m_card);
    cancelBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #404040; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 12px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #505050; }"
    );
    editBtnLayout->addWidget(cancelBtn);
    editBtnLayout->addWidget(saveBtn);

    m_layout->insertWidget(0, m_editField);
    m_layout->insertLayout(1, editBtnLayout);

    connect(saveBtn, &QPushButton::clicked, [this]() {
        QString newContent = m_editField->toPlainText().trimmed();
        if (!newContent.isEmpty()) {
            emit editRequested(m_messageIndex, newContent);
        }
    });
    connect(cancelBtn, &QPushButton::clicked, [this]() {
        m_layout->removeWidget(m_editField);
        delete m_editField;
        m_editField = nullptr;
        QLayoutItem *item = m_layout->takeAt(1);
        if (item) {
            delete item->layout();
            delete item;
        }
    });
}

void ChatMessageCard::rebuildContent()
{
    while (m_layout->count() > 1) {
        QLayoutItem *item = m_layout->takeAt(0);
        delete item->widget();
        delete item;
    }

    renderMarkdown(m_fullContent);
    m_layout->addWidget(m_copyContainer);
}

QString ChatMessageCard::escapeHtml(const QString &text)
{
    QString escaped = text.toHtmlEscaped();
    escaped.replace("\n", "<br>");
    return escaped;
}

QString ChatMessageCard::renderInlineMarkdown(const QString &text)
{
    QString result = text;

    result = QRegularExpression("\\*\\*(.+?)\\*\\*").replace(result, "<b>\\1</b>");
    result = QRegularExpression("\\*(.+?)\\*").replace(result, "<i>\\1</i>");
    result = QRegularExpression("__(.+?)__").replace(result, "<b>\\1</b>");
    result = QRegularExpression("_(.+?)_").replace(result, "<i>\\1</i>");
    result = QRegularExpression("`(.+?)`").replace(result, "<code style='background-color:#1a1a1a; padding:2px 4px; border-radius:3px; font-family:monospace;'>\\1</code>");
    result = QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)").replace(result, "<a href='\\2' style='color:#60a5fa;'>\\1</a>");

    return result;
}

void ChatMessageCard::addTextBlock(const QString &text)
{
    if (text.trimmed().isEmpty()) return;

    QStringList lines = text.split("\n");
    QString html;

    bool inList = false;
    bool inOrderedList = false;

    for (const auto &line : lines) {
        QString trimmed = line.trimmed();

        if (trimmed.startsWith("### ")) {
            if (inList) { html += "</ul>"; inList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += "<h3 style='color:#ececf1; font-size:16px; margin:8px 0 4px 0;'>" + renderInlineMarkdown(trimmed.mid(4)) + "</h3>";
        } else if (trimmed.startsWith("## ")) {
            if (inList) { html += "</ul>"; inList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += "<h2 style='color:#ececf1; font-size:18px; margin:10px 0 6px 0;'>" + renderInlineMarkdown(trimmed.mid(3)) + "</h2>";
        } else if (trimmed.startsWith("# ")) {
            if (inList) { html += "</ul>"; inList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += "<h1 style='color:#ececf1; font-size:20px; margin:12px 0 8px 0;'>" + renderInlineMarkdown(trimmed.mid(2)) + "</h1>";
        } else if (trimmed.startsWith("- ") || trimmed.startsWith("* ")) {
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            if (!inList) { html += "<ul style='margin:4px 0; padding-left:20px;'>"; inList = true; }
            html += "<li style='margin:2px 0;'>" + renderInlineMarkdown(trimmed.mid(2)) + "</li>";
        } else if (QRegularExpression("^\\d+\\.\\s").match(trimmed).hasMatch()) {
            if (inList) { html += "</ul>"; inList = false; }
            if (!inOrderedList) { html += "<ol style='margin:4px 0; padding-left:20px;'>"; inOrderedList = true; }
            int dotPos = trimmed.indexOf(".");
            html += "<li style='margin:2px 0;'>" + renderInlineMarkdown(trimmed.mid(dotPos + 1).trimmed()) + "</li>";
        } else if (trimmed.isEmpty()) {
            if (inList) { html += "</ul>"; inList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += "<br>";
        } else {
            if (inList) { html += "</ul>"; inList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += renderInlineMarkdown(trimmed) + "<br>";
        }
    }

    if (inList) html += "</ul>";
    if (inOrderedList) html += "</ol>";

    QLabel *label = new QLabel(m_card);
    label->setText(html);
    label->setWordWrap(true);
    label->setStyleSheet(
        "QLabel { "
        "  color: #ececf1; "
        "  font-size: 15px; "
        "  line-height: 1.6; "
        "} "
        "QLabel a { color: #60a5fa; }"
    );
    label->setTextInteractionFlags(Qt::TextSelectableByMouse | Qt::LinksAccessibleByMouse);
    label->setOpenExternalLinks(true);
    m_layout->insertWidget(m_layout->count() - 1, label);
}

void ChatMessageCard::addCodeBlock(const QString &code, const QString &language)
{
    QWidget *codeContainer = new QWidget(m_card);
    QVBoxLayout *codeLayout = new QVBoxLayout(codeContainer);
    codeLayout->setContentsMargins(0, 0, 0, 0);
    codeLayout->setSpacing(0);

    QTextEdit *codeEdit = new QTextEdit(codeContainer);
    codeEdit->setReadOnly(true);
    codeEdit->setPlainText(code);
    codeEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: #1a1a1a; "
        "  color: #e6e6e6; "
        "  font-family: 'Cascadia Code', 'Consolas', 'Courier New', monospace; "
        "  font-size: 14px; "
        "  border: none; "
        "  border-bottom-left-radius: 8px; "
        "  border-bottom-right-radius: 8px; "
        "  padding: 12px; "
        "}"
    );
    codeEdit->setMaximumHeight(300);

    if (!language.isEmpty()) {
        QLabel *langLabel = new QLabel(language, codeContainer);
        langLabel->setStyleSheet(
            "QLabel { "
            "  color: #8e8e8e; "
            "  font-size: 12px; "
            "  padding: 4px 8px; "
            "}"
        );

        QPushButton *copyBtn = new QPushButton("Copy", codeContainer);
        copyBtn->setStyleSheet(
            "QPushButton { "
            "  background-color: #404040; "
            "  color: #ececf1; "
            "  border: none; "
            "  border-radius: 4px; "
            "  padding: 4px 12px; "
            "  font-size: 12px; "
            "} "
            "QPushButton:hover { background-color: #505050; }"
        );
        copyBtn->setMaximumWidth(60);

        QHBoxLayout *headerLayout = new QHBoxLayout();
        headerLayout->addWidget(langLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(copyBtn);
        headerLayout->setContentsMargins(8, 4, 8, 4);

        QFrame *headerFrame = new QFrame(codeContainer);
        headerFrame->setStyleSheet("QFrame { background-color: #1a1a1a; border-top-left-radius: 8px; border-top-right-radius: 8px; }");
        headerFrame->setLayout(headerLayout);

        codeLayout->addWidget(headerFrame);
        connect(copyBtn, &QPushButton::clicked, [codeEdit]() {
            QClipboard *clipboard = QApplication::clipboard();
            clipboard->setText(codeEdit->toPlainText());
        });
    }

    codeLayout->addWidget(codeEdit);
    m_layout->insertWidget(m_layout->count() - 1, codeContainer);
}

void ChatMessageCard::renderMarkdown(const QString &text)
{
    QRegularExpression codeBlockRegex("```(\\w*)\\n([\\s\\S]*?)```");
    int lastPos = 0;
    QRegularExpressionMatchIterator it = codeBlockRegex.globalMatch(text);

    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        QString before = text.mid(lastPos, match.capturedStart() - lastPos);
        if (!before.isEmpty()) {
            addTextBlock(before);
        }
        addCodeBlock(match.captured(2).trimmed(), match.captured(1));
        lastPos = match.capturedEnd();
    }

    if (lastPos < text.length()) {
        addTextBlock(text.mid(lastPos));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_inputField(new QTextEdit)
    , m_sendButton(new QToolButton)
    , m_modelCombo(new QComboBox)
    , m_newChatButton(new QPushButton("+ New Chat"))
    , m_apiClient(new ApiClient(this))
    , m_currentChatIndex(-1)
    , m_settings("lagmajin", "SimpleAIClient")
    , m_streamingCard(nullptr)
    , m_currentChatImage("")
    , m_imagePreview(nullptr)
{
    setWindowTitle("SimpleAIClient");
    resize(1200, 800);

    setupUI();
    setupMenu();
    loadSettings();
    loadChatSessions();

    if (m_chatSessions.isEmpty()) {
        createNewChat();
    } else {
        if (!m_chatSessions.isEmpty() && m_chatSessions[0].messages.isEmpty()) {
            showWelcomeScreen();
        }
        loadDraft();
    }

    m_inputField->installEventFilter(this);

    new QShortcut(QKeySequence("Ctrl+N"), this, SLOT(onNewChat()));
    new QShortcut(QKeySequence("Ctrl+E"), this, SLOT(onExportChat()));
    new QShortcut(QKeySequence("Ctrl+,"), this, SLOT(onSettings()));

    connect(m_attachButton, &QToolButton::clicked, this, &MainWindow::onAttachImage);
    connect(m_sendButton, &QToolButton::clicked, this, &MainWindow::onSendMessage);
    connect(m_apiClient, &ApiClient::responseReceived, this, &MainWindow::onResponseReceived);
    connect(m_apiClient, &ApiClient::responseChunk, this, &MainWindow::onResponseChunk);
    connect(m_apiClient, &ApiClient::responseFinished, this, &MainWindow::onResponseFinished);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &MainWindow::onErrorOccurred);
    connect(m_apiClient, &ApiClient::modelsFetched, this, &MainWindow::onModelsFetched);
    connect(m_modelCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &MainWindow::onModelChanged);
    connect(m_newChatButton, &QPushButton::clicked, this, &MainWindow::onNewChat);

    updateChatList();
    fetchModels();
}

void MainWindow::setupUI()
{
    QString scrollbarStyle =
        "QScrollBar:vertical { "
        "  background-color: #2f2f2f; "
        "  width: 10px; "
        "  border-radius: 5px; "
        "  margin: 0px; "
        "} "
        "QScrollBar::handle:vertical { "
        "  background-color: #555555; "
        "  border-radius: 5px; "
        "  min-height: 30px; "
        "} "
        "QScrollBar::handle:vertical:hover { "
        "  background-color: #666666; "
        "} "
        "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical, "
        "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical { "
        "  height: 0px; "
        "  background: none; "
        "}";

    m_inputField->setPlaceholderText("Message SimpleAIClient...");
    m_inputField->setMaximumHeight(150);
    m_inputField->setStyleSheet(
        "QTextEdit { "
        "  background-color: transparent; "
        "  color: #ececf1; "
        "  border: none; "
        "  font-size: 15px; "
        "  selection-background-color: #404040; "
        "} " + scrollbarStyle
    );

    m_modelCombo->setEditable(true);
    m_modelCombo->setMinimumWidth(200);
    m_modelCombo->setMaximumWidth(350);
    m_modelCombo->setStyleSheet(
        "QComboBox { "
        "  background-color: #2f2f2f; "
        "  color: #ececf1; "
        "  border: 1px solid #4d4d4f; "
        "  border-radius: 8px; "
        "  padding: 8px 12px; "
        "  font-size: 14px; "
        "} "
        "QComboBox::drop-down { "
        "  border: none; "
        "  padding-right: 10px; "
        "} "
        "QComboBox QAbstractItemView { "
        "  background-color: #2f2f2f; "
        "  color: #ececf1; "
        "  selection-background-color: #404040; "
        "  font-size: 14px; "
        "}"
    );

    m_attachButton = new QToolButton;
    m_attachButton->setIcon(style()->standardIcon(QStyle::SP_FileIcon));
    m_attachButton->setFixedSize(36, 36);
    m_attachButton->setStyleSheet(
        "QToolButton { "
        "  background-color: transparent; "
        "  border: none; "
        "  padding: 6px; "
        "} "
        "QToolButton:hover { background-color: #404040; border-radius: 18px; }"
    );
    m_attachButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    connect(m_attachButton, &QToolButton::clicked, this, &MainWindow::onAttachImage);

    m_sendButton->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
    m_sendButton->setFixedSize(40, 40);
    m_sendButton->setStyleSheet(
        "QToolButton { "
        "  background-color: #005c4b; "
        "  border: none; "
        "  border-radius: 20px; "
        "  padding: 8px; "
        "} "
        "QToolButton:hover { background-color: #007a5e; } "
        "QToolButton:pressed { background-color: #004d3f; } "
        "QToolButton:disabled { background-color: #404040; }"
    );

    inputInner->addWidget(m_attachButton, 0, Qt::AlignBottom);
    inputInner->addWidget(m_inputField, 1);
    inputInner->addWidget(m_sendButton, 0, Qt::AlignBottom);

    m_imagePreview = new QLabel(inputFrame);
    m_imagePreview->setMaximumHeight(60);
    m_imagePreview->setMaximumWidth(200);
    m_imagePreview->setScaledContents(true);
    m_imagePreview->setStyleSheet("QLabel { border: 1px solid #4d4d4f; border-radius: 8px; padding: 4px; }");
    m_imagePreview->setVisible(false);
    m_imagePreview->setAlignment(Qt::AlignCenter);

    QVBoxLayout *inputWrapperLayout = new QVBoxLayout(inputWrapper);
    inputWrapperLayout->setContentsMargins(0, 0, 0, 0);
    inputWrapperLayout->setSpacing(4);
    inputWrapperLayout->addWidget(m_imagePreview, 0, Qt::AlignLeft);
    inputWrapperLayout->addLayout(inputInner);

    inputLayout->addStretch();
    inputLayout->addWidget(inputWrapper);
    inputLayout->addStretch();

    mainLayout->addWidget(inputFrame);

    m_splitter->addWidget(mainPanel);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({280, 920});

    setCentralWidget(m_splitter);

    m_statusBar = statusBar();
    m_statusBar->setStyleSheet(
        "QStatusBar { "
        "  background-color: #171717; "
        "  color: #8e8e8e; "
        "  font-size: 12px; "
        "  border-top: 1px solid #3a3a3a; "
        "}"
    );

    m_statusConnection = new QLabel("Ready", m_statusBar);
    m_statusModel = new QLabel(m_statusBar);
    m_statusTokens = new QLabel(m_statusBar);
    m_statusResponseTime = new QLabel(m_statusBar);

    m_statusBar->addWidget(m_statusConnection, 1);
    m_statusBar->addPermanentWidget(m_statusResponseTime);
    m_statusBar->addPermanentWidget(m_statusTokens);
    m_statusBar->addPermanentWidget(m_statusModel);

    m_isDarkTheme = m_settings.value("darkTheme", true).toBool();
    applyTheme();
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
            clearChatDisplay();
            saveChatSessions();
        }
    });

    QAction *exportAction = fileMenu->addAction("Export Chat...");
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportChat);

    QAction *themeAction = fileMenu->addAction("Toggle Theme");
    themeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);

    QAction *advancedAction = fileMenu->addAction("Advanced Settings...");
    advancedAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(advancedAction, &QAction::triggered, this, &MainWindow::onAdvancedSettings);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction("Quit");
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::loadSettings()
{
    QString apiKey = m_settings.value("apiKey").toString();
    QString model = m_settings.value("model", "venice-uncensored").toString();

    m_apiClient->setApiKey(apiKey);
    m_apiClient->setModel(model);
    m_modelCombo->addItem(model);
    m_modelCombo->setCurrentText(model);

    if (m_statusModel) {
        m_statusModel->setText(model);
    }

    QString systemPrompt = m_settings.value("systemPrompt").toString();
    m_apiClient->setSystemPrompt(systemPrompt);

    double temperature = m_settings.value("temperature", 0.7).toDouble();
    m_apiClient->setTemperature(temperature);

    int maxTokens = m_settings.value("maxTokens", 0).toInt();
    m_apiClient->setMaxTokens(maxTokens);

    bool webSearch = m_settings.value("webSearch", false).toBool();
    m_apiClient->setWebSearch(webSearch);
}

void MainWindow::fetchModels()
{
    if (!checkApiKey()) return;
    QString savedModel = m_modelCombo->currentText();
    m_modelCombo->clear();
    m_modelCombo->addItem("Loading models...");
    m_modelCombo->setEnabled(false);
    m_apiClient->fetchModels();
    m_modelCombo->setProperty("savedModel", savedModel);
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
    newChat.pinned = false;
    m_chatSessions.prepend(newChat);
    m_currentChatIndex = 0;
    clearChatDisplay();
    showWelcomeScreen();
    updateChatList();
    saveChatSessions();
}

void MainWindow::switchToChat(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;

    saveDraft();

    m_currentChatIndex = index;
    clearChatDisplay();

    const auto &chat = m_chatSessions[index];
    if (chat.messages.isEmpty()) {
        showWelcomeScreen();
    } else {
        hideWelcomeScreen();
    for (const auto &msg : chat.messages) {
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        addMessageCard(msg.role, displayText, msg.promptTokens, msg.completionTokens, msg.totalTokens);
    }
    }

    loadDraft();
    scrollToBottom();
}

void MainWindow::updateChatList()
{
    filterChats(m_searchField->text());
}

void MainWindow::filterChats(const QString &query)
{
    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(m_chatListContainer->layout());
    if (!layout) return;

    QLayoutItem *item;
    while (layout->count() > 0) {
        item = layout->takeAt(0);
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    QString lowerQuery = query.toLower();

    QList<int> pinnedIndices;
    QList<int> unpinnedIndices;
    for (int i = 0; i < m_chatSessions.size(); ++i) {
        if (!lowerQuery.isEmpty() && !m_chatSessions[i].title.toLower().contains(lowerQuery)) {
            continue;
        }
        if (m_chatSessions[i].pinned) {
            pinnedIndices.append(i);
        } else {
            unpinnedIndices.append(i);
        }
    }

    QList<int> orderedIndices = pinnedIndices + unpinnedIndices;

    for (int idx : orderedIndices) {
        ChatListItem *chatItem = new ChatListItem(m_chatSessions[idx].title, idx, m_chatListContainer);
        if (idx == m_currentChatIndex) {
            chatItem->setStyleSheet("QWidget { background-color: #2a2b32; border-radius: 6px; }");
        }
        layout->insertWidget(layout->count(), chatItem);

        connect(chatItem, &ChatListItem::clicked, this, [this, idx]() {
            switchToChat(idx);
            updateChatList();
        });
        connect(chatItem, &ChatListItem::deleteClicked, this, [this, idx]() {
            deleteChatAtRow(idx);
        });
        connect(chatItem, &ChatListItem::renameRequested, this, [this, idx]() {
            renameChat(idx);
        });
        connect(chatItem, &ChatListItem::pinRequested, this, [this, idx]() {
            togglePinChat(idx);
        });
    }

    layout->addStretch();
}

void MainWindow::saveChatSessions()
{
    QJsonArray sessionsArray;
    for (const auto &chat : m_chatSessions) {
        QJsonObject sessionObj;
        sessionObj["id"] = chat.id;
        sessionObj["title"] = chat.title;
        sessionObj["pinned"] = chat.pinned;

        QJsonArray messagesArray;
        for (const auto &msg : chat.messages) {
            QJsonObject msgObj;
            msgObj["role"] = msg.role;
            msgObj["content"] = msg.content;
            msgObj["promptTokens"] = msg.promptTokens;
            msgObj["completionTokens"] = msg.completionTokens;
            msgObj["totalTokens"] = msg.totalTokens;
            msgObj["imageUrl"] = msg.imageUrl;
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
        chat.pinned = sessionObj["pinned"].toBool();

        QJsonArray messagesArray = sessionObj["messages"].toArray();
        for (const auto &msgVal : messagesArray) {
            QJsonObject msgObj = msgVal.toObject();
            chat.messages.append({msgObj["role"].toString(), msgObj["content"].toString(), msgObj["promptTokens"].toInt(), msgObj["completionTokens"].toInt(), msgObj["totalTokens"].toInt(), msgObj["imageUrl"].toString()});
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

void MainWindow::clearChatDisplay()
{
    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        if (item->widget() && item->widget() != m_welcomeWidget) {
            delete item->widget();
        }
        delete item;
    }
    m_chatLayout->addStretch();
    m_streamingCard = nullptr;
}

void MainWindow::addMessageCard(const QString &role, const QString &content, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs)
{
    hideWelcomeScreen();

    m_chatLayout->removeItem(m_chatLayout->itemAt(m_chatLayout->count() - 1));
    ChatMessageCard *card = new ChatMessageCard(role, content, m_chatContainer);
    card->showCopyButton(true);

    int msgIndex = -1;
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
        msgIndex = m_chatSessions[m_currentChatIndex].messages.size();
    }
    card->setMessageIndex(msgIndex);

    if (role == "assistant") {
        card->showRegenerateButton(true);
        card->showBranchButton(true);
        connect(card, &ChatMessageCard::regenerateRequested, this, &MainWindow::onRegenerateResponse);
        connect(card, &ChatMessageCard::branchRequested, this, &MainWindow::onBranchConversation);
    } else if (role == "user") {
        card->setEditable(true);
        connect(card, &ChatMessageCard::editRequested, this, &MainWindow::onEditMessage);
    }

    if (totalTokens > 0) {
        card->setTokenInfo(promptTokens, completionTokens, totalTokens, responseTimeMs);
    }

    QGraphicsOpacityEffect *opacityEffect = new QGraphicsOpacityEffect(card);
    opacityEffect->setOpacity(0);
    card->setGraphicsEffect(opacityEffect);

    QPropertyAnimation *fadeIn = new QPropertyAnimation(opacityEffect, "opacity");
    fadeIn->setDuration(200);
    fadeIn->setStartValue(0);
    fadeIn->setEndValue(1);
    fadeIn->start(QAbstractAnimation::DeleteWhenStopped);

    m_chatLayout->addWidget(card);
    m_chatLayout->addStretch();
}

void MainWindow::scrollToBottom()
{
    QScrollBar *bar = m_scrollArea->verticalScrollBar();
    QMetaObject::invokeMethod(bar, [bar]() {
        bar->setValue(bar->maximum());
    }, Qt::QueuedConnection);
}

void MainWindow::showWelcomeScreen()
{
    if (m_welcomeWidget) {
        m_welcomeWidget->setVisible(true);
    }
}

void MainWindow::hideWelcomeScreen()
{
    if (m_welcomeWidget) {
        m_welcomeWidget->setVisible(false);
    }
}

void MainWindow::applyModelFilter()
{
    QString savedModel = m_modelCombo->currentText();
    bool uncensoredOnly = m_uncensoredFilter->isChecked();

    m_modelCombo->clear();
    for (const auto &model : m_allModels) {
        if (uncensoredOnly && !model.contains("uncensored", Qt::CaseInsensitive)) {
            continue;
        }
        m_modelCombo->addItem(model);
    }

    int index = m_modelCombo->findText(savedModel);
    if (index >= 0) {
        m_modelCombo->setCurrentIndex(index);
    }
    m_apiClient->setModel(m_modelCombo->currentText());
}

void MainWindow::onAttachImage()
{
    QString filePath = QFileDialog::getOpenFileName(this, "Select Image", "", "Images (*.png *.jpg *.jpeg *.gif *.bmp *.webp)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open image file.");
        return;
    }

    QByteArray data = file.readAll();
    QString base64 = QString("data:image/jpeg;base64,") + data.toBase64();
    m_currentChatImage = base64;

    QPixmap pixmap;
    pixmap.load(filePath);
    if (!pixmap.isNull()) {
        m_imagePreview->setPixmap(pixmap.scaled(200, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imagePreview->setVisible(true);
    }
}

void MainWindow::onSendMessage()
{
    QString text = m_inputField->toPlainText().trimmed();
    if (text.isEmpty() && m_currentChatImage.isEmpty()) return;
    if (!checkApiKey()) return;

    clearDraft();

    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        createNewChat();
    }

    m_inputField->clear();

    QString displayText = text;
    if (!m_currentChatImage.isEmpty()) {
        displayText += "\n[Image attached]";
    }
    addMessageCard("user", displayText);

    ChatMessage userMsg{"user", text, 0, 0, 0, m_currentChatImage};
    m_chatSessions[m_currentChatIndex].messages.append(userMsg);

    m_currentChatImage.clear();
    m_imagePreview->setVisible(false);
    m_imagePreview->clear();

    if (m_chatSessions[m_currentChatIndex].messages.size() == 1) {
        m_chatSessions[m_currentChatIndex].title = generateChatTitle(text);
        updateChatList();
    }

    m_sendButton->setEnabled(false);
    m_inputField->setEnabled(false);

    if (m_streamToggle->isChecked()) {
        m_streamingCard = new ChatMessageCard("assistant", "", m_chatContainer);
        m_streamingCard->setStreaming(true);
        m_chatLayout->removeItem(m_chatLayout->itemAt(m_chatLayout->count() - 1));
        m_chatLayout->addWidget(m_streamingCard);
        m_chatLayout->addStretch();
    } else {
        m_chatLayout->removeItem(m_chatLayout->itemAt(m_chatLayout->count() - 1));
        ChatMessageCard *thinkingCard = new ChatMessageCard("assistant", "_Thinking..._", m_chatContainer);
        m_chatLayout->addWidget(thinkingCard);
        m_chatLayout->addStretch();
        m_streamingCard = thinkingCard;
    }

    m_apiClient->sendMessage(m_chatSessions[m_currentChatIndex].messages);
    saveChatSessions();

    if (m_statusConnection) {
        m_statusConnection->setText("Sending...");
    }

    scrollToBottom();
}

void MainWindow::onResponseReceived(const QString &response, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs)
{
    if (m_streamingCard) {
        m_chatLayout->removeWidget(m_streamingCard);
        delete m_streamingCard;
        m_streamingCard = nullptr;
    }

    addMessageCard("assistant", response, promptTokens, completionTokens, totalTokens, responseTimeMs);

    ChatMessage assistantMsg{"assistant", response, promptTokens, completionTokens, totalTokens};
    m_chatSessions[m_currentChatIndex].messages.append(assistantMsg);

    if (m_statusConnection) {
        m_statusConnection->setText("Ready");
    }
    if (m_statusTokens && totalTokens > 0) {
        m_statusTokens->setText(QString("%1 tokens").arg(totalTokens));
    }
    if (m_statusResponseTime && responseTimeMs > 0) {
        m_statusResponseTime->setText(QString("%.1fs").arg(responseTimeMs / 1000.0));
    }

    m_sendButton->setEnabled(true);
    m_inputField->setEnabled(true);
    m_inputField->setFocus();

    scrollToBottom();

    saveChatSessions();
}

void MainWindow::onResponseChunk(const QString &chunk)
{
    if (m_streamingCard) {
        m_streamingCard->appendContent(chunk);
        scrollToBottom();
    }
    if (m_statusConnection) {
        m_statusConnection->setText("Streaming...");
    }
}

void MainWindow::onResponseFinished(int responseTimeMs)
{
    if (m_streamingCard) {
        m_streamingCard->setStreaming(false);
        m_streamingCard->showCopyButton(true);

        QString fullContent = m_streamingCard->content();
        ChatMessage assistantMsg{"assistant", fullContent, 0, 0, 0};
        m_chatSessions[m_currentChatIndex].messages.append(assistantMsg);
        saveChatSessions();
    }

    if (m_statusConnection) {
        m_statusConnection->setText("Ready");
    }
    if (m_statusResponseTime && responseTimeMs > 0) {
        m_statusResponseTime->setText(QString("%.1fs").arg(responseTimeMs / 1000.0));
    }

    m_sendButton->setEnabled(true);
    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    m_streamingCard = nullptr;
}

void MainWindow::onErrorOccurred(const QString &error)
{
    qDebug() << "API Error:" << error;

    if (m_streamingCard) {
        m_chatLayout->removeWidget(m_streamingCard);
        delete m_streamingCard;
        m_streamingCard = nullptr;
    }

    addMessageCard("error", "Error: " + error);

    if (m_statusConnection) {
        m_statusConnection->setText("Error");
    }

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

void MainWindow::onExportChat()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        QMessageBox::information(this, "Export Chat", "No chat to export.");
        return;
    }

    const auto &chat = m_chatSessions[m_currentChatIndex];
    QString defaultName = chat.title.isEmpty() ? "chat" : chat.title;
    defaultName.replace(QRegularExpression("[^a-zA-Z0-9\\s]"), "");

    QString filePath = QFileDialog::getSaveFileName(this, "Export Chat", defaultName + ".md", "Markdown Files (*.md);;Text Files (*.txt);;All Files (*)");
    if (filePath.isEmpty()) return;

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not save file: " + filePath);
        return;
    }

    QTextStream out(&file);
    out.setCodec("UTF-8");

    out << "# " << chat.title << "\n\n";
    out << "Exported: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";
    out << "---\n\n";

    for (const auto &msg : chat.messages) {
        QString role = msg.role == "user" ? "You" : "Assistant";
        out << "## " << role << "\n\n";
        out << msg.content << "\n\n";
        if (msg.totalTokens > 0) {
            out << "*Tokens: " << msg.promptTokens << " prompt, " << msg.completionTokens << " completion, " << msg.totalTokens << " total*\n\n";
        }
        out << "---\n\n";
    }

    file.close();
    QMessageBox::information(this, "Export Complete", "Chat exported to:\n" + filePath);
}

void MainWindow::onToggleTheme()
{
    m_isDarkTheme = !m_isDarkTheme;
    m_settings.setValue("darkTheme", m_isDarkTheme);
    applyTheme();
}

void MainWindow::onRegenerateResponse()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    auto &messages = m_chatSessions[m_currentChatIndex].messages;
    if (messages.isEmpty()) return;

    if (messages.last().role == "assistant") {
        messages.removeLast();
    }

    if (messages.isEmpty()) return;

    clearChatDisplay();
    for (const auto &msg : messages) {
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        addMessageCard(msg.role, displayText, msg.promptTokens, msg.completionTokens, msg.totalTokens);
    }

    m_sendButton->setEnabled(false);
    m_inputField->setEnabled(false);

    m_streamingCard = new ChatMessageCard("assistant", "", m_chatContainer);
    m_streamingCard->setStreaming(true);
    m_chatLayout->removeItem(m_chatLayout->itemAt(m_chatLayout->count() - 1));
    m_chatLayout->addWidget(m_streamingCard);
    m_chatLayout->addStretch();

    m_apiClient->sendMessage(messages);
    saveChatSessions();

    scrollToBottom();
}

void MainWindow::onBranchConversation(int messageIndex)
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    const auto &sourceChat = m_chatSessions[m_currentChatIndex];
    if (messageIndex < 0 || messageIndex >= sourceChat.messages.size()) return;

    ChatSession newChat;
    newChat.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    newChat.title = sourceChat.title + " (branch)";
    newChat.pinned = false;

    for (int i = 0; i <= messageIndex; ++i) {
        newChat.messages.append(sourceChat.messages[i]);
    }

    m_chatSessions.prepend(newChat);
    m_currentChatIndex = 0;

    clearChatDisplay();
    for (const auto &msg : newChat.messages) {
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        addMessageCard(msg.role, displayText, msg.promptTokens, msg.completionTokens, msg.totalTokens);
    }

    updateChatList();
    saveChatSessions();
    scrollToBottom();
}

void MainWindow::onEditMessage(int messageIndex, const QString &newContent)
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    auto &messages = m_chatSessions[m_currentChatIndex].messages;
    if (messageIndex < 0 || messageIndex >= messages.size()) return;

    messages[messageIndex].content = newContent;

    for (int i = messages.size() - 1; i > messageIndex; --i) {
        if (messages[i].role == "assistant" || messages[i].role == "user") {
            messages.removeAt(i);
        }
    }

    clearChatDisplay();
    for (const auto &msg : messages) {
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        addMessageCard(msg.role, displayText, msg.promptTokens, msg.completionTokens, msg.totalTokens);
    }

    m_sendButton->setEnabled(false);
    m_inputField->setEnabled(false);

    m_streamingCard = new ChatMessageCard("assistant", "", m_chatContainer);
    m_streamingCard->setStreaming(true);
    m_chatLayout->removeItem(m_chatLayout->itemAt(m_chatLayout->count() - 1));
    m_chatLayout->addWidget(m_streamingCard);
    m_chatLayout->addStretch();

    m_apiClient->sendMessage(messages);
    saveChatSessions();

    scrollToBottom();
}

void MainWindow::onAdvancedSettings()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Advanced Settings");
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QFormLayout *formLayout = new QFormLayout();
    formLayout->setSpacing(12);

    QString currentPrompt = m_settings.value("systemPrompt").toString();
    QTextEdit *systemPromptEdit = new QTextEdit(&dialog);
    systemPromptEdit->setPlainText(currentPrompt);
    systemPromptEdit->setMaximumHeight(100);
    systemPromptEdit->setPlaceholderText("e.g., You are a helpful assistant that speaks in a formal tone...");
    formLayout->addRow("System Prompt:", systemPromptEdit);

    double currentTemp = m_settings.value("temperature", 0.7).toDouble();
    QDoubleSpinBox *tempSpin = new QDoubleSpinBox(&dialog);
    tempSpin->setRange(0.0, 2.0);
    tempSpin->setSingleStep(0.1);
    tempSpin->setValue(currentTemp);
    tempSpin->setToolTip("Controls randomness: 0 = deterministic, 2 = very random");
    formLayout->addRow("Temperature:", tempSpin);

    int currentMaxTokens = m_settings.value("maxTokens", 0).toInt();
    QSpinBox *maxTokensSpin = new QSpinBox(&dialog);
    maxTokensSpin->setRange(0, 32000);
    maxTokensSpin->setSingleStep(256);
    maxTokensSpin->setValue(currentMaxTokens);
    maxTokensSpin->setSpecialValueText("Unlimited");
    maxTokensSpin->setToolTip("Maximum tokens in response (0 = unlimited)");
    formLayout->addRow("Max Tokens:", maxTokensSpin);

    layout->addLayout(formLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    connect(buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    if (dialog.exec() == QDialog::Accepted) {
        m_settings.setValue("systemPrompt", systemPromptEdit->toPlainText());
        m_settings.setValue("temperature", tempSpin->value());
        m_settings.setValue("maxTokens", maxTokensSpin->value());

        m_apiClient->setSystemPrompt(systemPromptEdit->toPlainText());
        m_apiClient->setTemperature(tempSpin->value());
        m_apiClient->setMaxTokens(maxTokensSpin->value());
    }
}

void MainWindow::applyTheme()
{
    if (m_isDarkTheme) {
        qApp->setStyle("Fusion");
        QPalette darkPalette;
        darkPalette.setColor(QPalette::Window, QColor(33, 33, 33));
        darkPalette.setColor(QPalette::WindowText, QColor(236, 236, 241));
        darkPalette.setColor(QPalette::Base, QColor(47, 47, 47));
        darkPalette.setColor(QPalette::AlternateBase, QColor(42, 43, 50));
        darkPalette.setColor(QPalette::ToolTipBase, QColor(47, 47, 47));
        darkPalette.setColor(QPalette::ToolTipText, QColor(236, 236, 241));
        darkPalette.setColor(QPalette::Text, QColor(236, 236, 241));
        darkPalette.setColor(QPalette::Button, QColor(47, 47, 47));
        darkPalette.setColor(QPalette::ButtonText, QColor(236, 236, 241));
        darkPalette.setColor(QPalette::BrightText, QColor(236, 236, 241));
        darkPalette.setColor(QPalette::Link, QColor(96, 165, 250));
        darkPalette.setColor(QPalette::Highlight, QColor(0, 92, 75));
        darkPalette.setColor(QPalette::HighlightedText, QColor(236, 236, 241));
        qApp->setPalette(darkPalette);
    } else {
        qApp->setStyle("Fusion");
        QPalette lightPalette;
        lightPalette.setColor(QPalette::Window, QColor(245, 245, 245));
        lightPalette.setColor(QPalette::WindowText, QColor(33, 33, 33));
        lightPalette.setColor(QPalette::Base, QColor(255, 255, 255));
        lightPalette.setColor(QPalette::AlternateBase, QColor(230, 230, 230));
        lightPalette.setColor(QPalette::ToolTipBase, QColor(255, 255, 255));
        lightPalette.setColor(QPalette::ToolTipText, QColor(33, 33, 33));
        lightPalette.setColor(QPalette::Text, QColor(33, 33, 33));
        lightPalette.setColor(QPalette::Button, QColor(255, 255, 255));
        lightPalette.setColor(QPalette::ButtonText, QColor(33, 33, 33));
        lightPalette.setColor(QPalette::BrightText, QColor(33, 33, 33));
        lightPalette.setColor(QPalette::Link, QColor(0, 92, 75));
        lightPalette.setColor(QPalette::Highlight, QColor(0, 92, 75));
        lightPalette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        qApp->setPalette(lightPalette);
    }
}

void MainWindow::onModelsFetched(const QStringList &models)
{
    m_allModels = models;
    m_modelCombo->setEnabled(true);
    applyModelFilter();

    if (m_statusModel) {
        m_statusModel->setText(m_modelCombo->currentText());
    }
}

void MainWindow::onModelChanged(const QString &model)
{
    m_apiClient->setModel(model);
    m_settings.setValue("model", model);
    if (m_statusModel) {
        m_statusModel->setText(model);
    }
}

void MainWindow::onNewChat()
{
    saveDraft();
    createNewChat();
    m_inputField->setFocus();
}

void MainWindow::onChatSelected(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void MainWindow::deleteChatAtRow(int row)
{
    if (row < 0 || row >= m_chatSessions.size()) return;

    if (m_chatSessions.size() == 1) {
        m_chatSessions.clear();
        m_currentChatIndex = -1;
        clearChatDisplay();
        createNewChat();
    } else {
        m_chatSessions.removeAt(row);
        if (m_currentChatIndex == row) {
            if (m_currentChatIndex >= m_chatSessions.size()) {
                m_currentChatIndex = m_chatSessions.size() - 1;
            }
            switchToChat(m_currentChatIndex);
        } else if (m_currentChatIndex > row) {
            m_currentChatIndex--;
        }
    }

    updateChatList();
    saveChatSessions();
}

void MainWindow::renameChat(int row)
{
    if (row < 0 || row >= m_chatSessions.size()) return;

    bool ok;
    QString newName = QInputDialog::getText(this, "Rename Chat", "New name:", QLineEdit::Normal, m_chatSessions[row].title, &ok);
    if (ok && !newName.isEmpty()) {
        m_chatSessions[row].title = newName;
        updateChatList();
        saveChatSessions();
    }
}

void MainWindow::togglePinChat(int row)
{
    if (row < 0 || row >= m_chatSessions.size()) return;

    m_chatSessions[row].pinned = !m_chatSessions[row].pinned;
    updateChatList();
    saveChatSessions();
}

void MainWindow::onDeleteChat()
{
    if (m_currentChatIndex < 0 || m_chatSessions.isEmpty()) return;
    deleteChatAtRow(m_currentChatIndex);
}

bool MainWindow::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == m_inputField && event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->key() == Qt::Key_Return && !(keyEvent->modifiers() & Qt::ShiftModifier)) {
            onSendMessage();
            return true;
        }
    }
    return QMainWindow::eventFilter(obj, event);
}

void MainWindow::saveDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    QString draft = m_inputField->toPlainText();
    QString key = QString("draft_%1").arg(m_chatSessions[m_currentChatIndex].id);
    m_settings.setValue(key, draft);
}

void MainWindow::loadDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    QString key = QString("draft_%1").arg(m_chatSessions[m_currentChatIndex].id);
    QString draft = m_settings.value(key).toString();
    m_inputField->blockSignals(true);
    m_inputField->setPlainText(draft);
    m_inputField->blockSignals(false);
}

void MainWindow::clearDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    QString key = QString("draft_%1").arg(m_chatSessions[m_currentChatIndex].id);
    m_settings.remove(key);
    m_inputField->blockSignals(true);
    m_inputField->clear();
    m_inputField->blockSignals(false);
}
