#include "mainwindow.h"
#include "appicons.h"
#include "chatwidgets.h"
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
#include <QPainterPath>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QStyle>
#include <QLayout>
#include <QPen>
#include <QFileDialog>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QSaveFile>
#include <QPalette>
#include <QDialog>
#include <QFormLayout>
#include <QElapsedTimer>
#include <cmath>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPixmap>
#include <QEnterEvent>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QListWidget>
#include <QDialogButtonBox>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QUrl>
#include <QSoundEffect>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QStandardPaths>
#include <QDir>
#include <QFileInfo>
#include <QMap>
#include <QMimeDatabase>
#include <QtMath>

namespace {

// Sending an unbounded paste makes the API reject the turn with a 400 and the
// markdown renderer stall on the huge body, so the composer is capped.
constexpr int kMaxInputChars = 8000;

} // namespace

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
    , m_headerTitle(nullptr)
    , m_headerSubtitle(nullptr)
    , m_thinkingRowWidget(nullptr)
    , m_thinkingIndicator(nullptr)
    , m_thinkingTimer(new QTimer(this))
    , m_thinkingDots(0)
    , m_charCounter(nullptr)
    , m_searchBar(nullptr)
    , m_currentHighlightIndex(-1)
    , m_currentProfileName("")
    , m_autoScroll(true)
    , m_stickToBottom(true)
    , m_chatRenderGeneration(0)
    , m_chatRenderCursor(-1)
    , m_chatListRenderGeneration(0)
    , m_chatListRenderCursor(0)
    , m_chatFontSize(15)
    , m_retryCount(0)
    , m_maxRetries(3)
    , m_retryTimer(new QTimer(this))
    , m_backupTimer(nullptr)
    , m_notificationSound(nullptr)
    , m_soundInitialized(false)
    , m_chatStartTime()
    , m_statusDuration(nullptr)
    , m_statusSpeed(nullptr)
    , m_streamStartTime(0)
    , m_streamTokenCount(0)
    , m_streamRenderTimer(new QTimer(this))
    , m_scrollFollowTimer(new QTimer(this))
    , m_requestChatIndex(-1)
    , m_requestInFlight(false)
    , m_theme(new ThemeController(this))
{
    setWindowTitle("SimpleAIClient");
    setWindowIcon(makeLineIcon(AppIconGlyph::App, 256));
    resize(1200, 800);
    setAcceptDrops(true);

    // The stored preference has to be known before setupUI so every widget is
    // built with the correct tokens the first time.
    m_theme->setDark(m_settings.value("darkTheme", true).toBool());
    m_theme->applyPalette();
    m_isDarkTheme = m_theme->isDark();

    setupUI();
    setupMenu();
    loadProfiles();
    loadSettings();
    loadChatSessions();

    m_inputField->installEventFilter(this);

    // Ctrl+N, Ctrl+E and Ctrl+, are registered once, on the menu actions in
    // setupMenu(). Registering them here as well made the key press ambiguous
    // between two receivers in the same shortcut context.
    connect(m_sendButton, &QToolButton::clicked, this, &MainWindow::onSendMessage);
    connect(m_apiClient, &ApiClient::responseReceived, this, &MainWindow::onResponseReceived);
    connect(m_apiClient, &ApiClient::responseChunk, this, &MainWindow::onResponseChunk);
    connect(m_apiClient, &ApiClient::responseFinished, this, &MainWindow::onResponseFinished);
    connect(m_apiClient, &ApiClient::requestCancelled, this, &MainWindow::onRequestCancelled);
    // A failed model list is not a failed chat turn: routing it through the
    // chat retry handler would append an error card to the open conversation
    // and leave the model combo stuck on its placeholder.
    connect(m_apiClient, &ApiClient::errorOccurred, this, &MainWindow::onApiErrorWithRetry);
    connect(m_apiClient, &ApiClient::modelsFetched, this, &MainWindow::onModelsFetched);
    connect(m_apiClient, &ApiClient::modelsFetchFailed, this, [this](const QString &error) {
        if (m_modelCombo) {
            m_modelCombo->setEnabled(true);
        }
        if (m_statusConnection) {
            m_statusConnection->setText("Model list unavailable");
        }
        if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
            QMessageBox::warning(this, "Models", "Could not load the model list: " + error);
        }
    });
    connect(m_modelCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &MainWindow::onModelChanged);
    connect(m_newChatButton, &QPushButton::clicked, this, &MainWindow::onNewChat);
    connect(m_profileCombo, QOverload<const QString &>::of(&QComboBox::currentTextChanged), this, &MainWindow::onProfileChanged);
    connect(m_inputField, &QTextEdit::textChanged, this, &MainWindow::updateCharCounter);
    // Slightly slower updates keep the UI smoother during long streams.
    m_streamRenderTimer->setInterval(50);
    m_streamRenderTimer->setSingleShot(false);
    connect(m_streamRenderTimer, &QTimer::timeout, this, &MainWindow::flushStreamingChunks);

    // Connected once for the lifetime of the window: the indicator is created
    // and destroyed per request, so the slot has to re-check the pointer.
    connect(m_thinkingTimer, &QTimer::timeout, this, [this]() {
        if (!m_thinkingIndicator) return;
        m_thinkingDots = (m_thinkingDots + 1) % 4;
        m_thinkingIndicator->setText("Thinking" + QString(m_thinkingDots, '.'));
    });

    // Follow the live layout target without restarting an animation per chunk.
    m_scrollFollowTimer->setInterval(16);
    m_scrollFollowTimer->setTimerType(Qt::PreciseTimer);
    connect(m_scrollFollowTimer, &QTimer::timeout, this, [this, clock = QElapsedTimer()]() mutable {
        if (!m_autoScroll || !m_stickToBottom || !m_scrollArea) {
            m_scrollFollowTimer->stop();
            clock.invalidate();
            return;
        }
        QScrollBar *bar = m_scrollArea->verticalScrollBar();
        if (!bar || bar->isSliderDown()) {
            m_scrollFollowTimer->stop();
            clock.invalidate();
            return;
        }
        const qint64 elapsed = clock.isValid() ? clock.restart() : 16;
        if (!clock.isValid()) clock.start();
        const int distance = bar->maximum() - bar->value();
        const double blend = 1.0 - std::exp(-qMin(elapsed, qint64(50)) / 85.0);
        const int step = qMax(1, int(std::ceil(distance * blend)));
        bar->setValue(bar->value() + qMin(distance, step));
        if (bar->value() == bar->maximum()) {
            m_scrollFollowTimer->stop();
            clock.invalidate();
        }
    });

    new QShortcut(QKeySequence("Ctrl+F"), this, [this]() { showSearchBar(); });
    new QShortcut(QKeySequence("Ctrl+="), this, [this]() { adjustFontSize(1); });
    new QShortcut(QKeySequence("Ctrl+-"), this, [this]() { adjustFontSize(-1); });
    new QShortcut(QKeySequence("Ctrl+0"), this, [this]() { resetFontSize(); });
    new QShortcut(QKeySequence("Ctrl+?"), this, [this]() { showShortcutsDialog(); });

    QTimer::singleShot(0, this, [this]() {
        restoreLastChatSelection();
        fetchModels();
    });
}


void MainWindow::setupUI()
{
    // Registers a stylesheet builder so the widget follows theme switches.
    const auto themed = [this](QWidget *w, const std::function<QString()> &builder) {
        m_theme->registerStyle(w, builder);
    };

    themed(m_inputField, []() {
        const Theme &t = currentTheme();
        return
            "QTextEdit { "
            "  background-color: transparent; "
            "  color: " + t.textStrong + "; "
            "  border: none; "
            "  font-size: 15px; "
            "  selection-background-color: " + t.selection + "; "
            "  selection-color: " + t.accentText + "; "
            "} " + scrollBarStyle();
    });

    themed(m_modelCombo, []() {
        const Theme &t = currentTheme();
        return
            "QComboBox { "
            "  background-color: " + t.surfaceAlt + "; "
            "  color: " + t.textStrong + "; "
            "  border: 1px solid " + t.borderStrong + "; "
            "  border-radius: 8px; "
            "  padding: 8px 12px; "
            "  font-size: 14px; "
            "} "
            "QComboBox::drop-down { border: none; padding-right: 10px; } "
            "QComboBox QAbstractItemView { "
            "  background-color: " + t.surfaceAlt + "; "
            "  color: " + t.textStrong + "; "
            "  selection-background-color: " + t.accent + "; "
            "  selection-color: " + t.accentText + "; "
            "  border: 1px solid " + t.borderStrong + "; "
            "  font-size: 14px; "
            "}";
    });

    themed(m_attachButton, []() {
        const Theme &t = currentTheme();
        return
            "QToolButton { "
            "  background-color: transparent; "
            "  border: none; "
            "  padding: 6px; "
            "} "
            "QToolButton:hover { background-color: " + t.surfaceAlt + "; border-radius: 18px; }";
    });

    themed(m_sendButton, []() {
        const Theme &t = currentTheme();
        return
            "QToolButton { "
            "  background-color: " + t.accent + "; "
            "  border: none; "
            "  border-radius: 20px; "
            "  padding: 8px; "
            "} "
            "QToolButton:hover { background-color: " + t.accentHover + "; } "
            "QToolButton:pressed { background-color: " + t.accentPressed + "; } "
            "QToolButton:disabled { background-color: " + t.scrollbar + "; }";
    });

    m_inputField->setPlaceholderText("Message SimpleAIClient...");
    m_inputField->setMaximumHeight(150);
    m_inputField->setMinimumHeight(44);
    m_inputField->document()->setDocumentMargin(2);

    m_modelCombo->setEditable(true);
    m_modelCombo->setMinimumWidth(200);
    m_modelCombo->setMaximumWidth(350);

    m_attachButton = new QToolButton;
    m_attachButton->setIcon(makeLineIcon(AppIconGlyph::Image));
    m_attachButton->setIconSize(QSize(20, 20));
    m_attachButton->setFixedSize(36, 36);
    m_attachButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    connect(m_attachButton, &QToolButton::clicked, this, &MainWindow::onAttachImage);

    m_sendButton->setIcon(makeLineIcon(AppIconGlyph::Send));
    m_sendButton->setIconSize(QSize(20, 20));
    m_sendButton->setFixedSize(40, 40);

    m_sidebar = new QWidget(this);
    // Keep the navigation rail stable so the chat list does not "breathe"
    // as the main panel content changes or the window is resized.
    m_sidebar->setFixedWidth(300);
    themed(m_sidebar, []() {
        const Theme &s = currentTheme();
        return QString("QWidget { background-color: %1; color: %2; border-right: 1px solid %3; }")
            .arg(s.sidebar).arg(s.textStrong).arg(s.border);
    });

    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(18, 20, 18, 16);
    sidebarLayout->setSpacing(12);

    m_appTitle = new QLabel("SimpleAIClient", m_sidebar);
    themed(m_appTitle, []() {
        return QString("QLabel { color: %1; font-size: 22px; font-weight: 700; letter-spacing: 0.5px; padding: 0; }")
            .arg(currentTheme().textStrong);
    });
    sidebarLayout->addWidget(m_appTitle);

    QLabel *sidebarSubtitle = new QLabel("Fast local chat, without the clutter.", m_sidebar);
    sidebarSubtitle->setWordWrap(true);
    themed(sidebarSubtitle, []() {
        return QString("QLabel { color: %1; font-size: 12px; line-height: 1.4; padding-bottom: 4px; }")
            .arg(currentTheme().textMuted);
    });
    sidebarLayout->addWidget(sidebarSubtitle);

    themed(m_newChatButton, []() {
        const Theme &n = currentTheme();
        return QString(
            "QPushButton { background-color: %1; color: %5; border: none; border-radius: 12px; "
            "padding: 12px 14px; text-align: left; font-size: 14px; font-weight: 600; } "
            "QPushButton:hover { background-color: %2; } "
            "QPushButton:pressed { background-color: %3; }")
            .arg(n.accent).arg(n.accentHover).arg(n.accentPressed).arg(n.success).arg(n.accentText);
    });
    m_newChatButton->setText("New Chat");
    m_newChatButton->setIcon(makeLineIcon(AppIconGlyph::NewChat, 24, QColor(currentTheme().accentText)));
    m_newChatButton->setIconSize(QSize(18, 18));
    sidebarLayout->addWidget(m_newChatButton);

    m_searchField = new QLineEdit(m_sidebar);
    m_searchField->setPlaceholderText("Search chats");
    m_searchField->addAction(makeLineIcon(AppIconGlyph::Search), QLineEdit::LeadingPosition);
    themed(m_searchField, []() {
        const Theme &s = currentTheme();
        return QString(
            "QLineEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: 10px; "
            "padding: 9px 12px; font-size: 13px; } "
            "QLineEdit:focus { border: 1px solid %4; background-color: %5; }")
            .arg(s.surfaceAlt).arg(s.textStrong).arg(s.border).arg(s.accent).arg(s.surfaceAlt);
    });
    connect(m_searchField, &QLineEdit::textChanged, this, &MainWindow::filterChats);
    sidebarLayout->addWidget(m_searchField);

    QLabel *historyLabel = new QLabel("Recent Chats", m_sidebar);
    themed(historyLabel, []() {
        return QString("QLabel { color: %1; font-size: 11px; font-weight: 700; letter-spacing: 1px; text-transform: uppercase; }")
            .arg(currentTheme().textMuted);
    });
    sidebarLayout->addWidget(historyLabel);

    QFrame *historyFrame = new QFrame(m_sidebar);
    themed(historyFrame, []() {
        const Theme &h = currentTheme();
        return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 16px; }")
            .arg(h.surfaceAlt).arg(h.border);
    });
    QVBoxLayout *historyLayout = new QVBoxLayout(historyFrame);
    historyLayout->setContentsMargins(8, 8, 8, 8);
    historyLayout->setSpacing(0);

    m_chatListContainer = new QWidget();
    m_chatListContainer->setStyleSheet("background-color: transparent;");
    QVBoxLayout *chatListLayout = new QVBoxLayout(m_chatListContainer);
    chatListLayout->setContentsMargins(0, 0, 0, 0);
    chatListLayout->setSpacing(4);
    chatListLayout->addStretch();

    QScrollArea *chatListScroll = new QScrollArea(historyFrame);
    chatListScroll->setWidget(m_chatListContainer);
    chatListScroll->setWidgetResizable(true);
    chatListScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    themed(chatListScroll, []() {
        return "QScrollArea { border: none; background: transparent; }" + scrollBarStyle();
    });
    historyLayout->addWidget(chatListScroll);
    sidebarLayout->addWidget(historyFrame, 1);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->addWidget(m_sidebar);

    QWidget *mainPanel = new QWidget(this);
    themed(mainPanel, []() {
        return QString("background-color: %1; color: %2;").arg(currentTheme().window).arg(currentTheme().textStrong);
    });
    QVBoxLayout *mainLayout = new QVBoxLayout(mainPanel);
    mainLayout->setContentsMargins(18, 18, 18, 14);
    mainLayout->setSpacing(12);

    QFrame *headerFrame = new QFrame(mainPanel);
    themed(headerFrame, []() {
        const Theme &h = currentTheme();
        return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 18px; }")
            .arg(h.surface).arg(h.border);
    });
    QVBoxLayout *headerOuter = new QVBoxLayout(headerFrame);
    headerOuter->setContentsMargins(18, 16, 18, 14);
    headerOuter->setSpacing(12);

    QHBoxLayout *headerTop = new QHBoxLayout();
    headerTop->setSpacing(12);

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(3);
    m_headerTitle = new QLabel("New Chat", headerFrame);
    themed(m_headerTitle, []() {
        return QString("QLabel { color: %1; font-size: 22px; font-weight: 700; }").arg(currentTheme().textStrong);
    });
    m_headerSubtitle = new QLabel("Start typing below. Model and profile stay available, but out of the way.", headerFrame);
    m_headerSubtitle->setWordWrap(true);
    themed(m_headerSubtitle, []() {
        return QString("QLabel { color: %1; font-size: 12px; line-height: 1.4; }").arg(currentTheme().textMuted);
    });
    headerTextLayout->addWidget(m_headerTitle);
    headerTextLayout->addWidget(m_headerSubtitle);
    headerTop->addLayout(headerTextLayout, 1);

    m_modelCombo->setMinimumWidth(220);
    m_modelCombo->setMaximumWidth(320);
    themed(m_modelCombo, []() {
        const Theme &c = currentTheme();
        return QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 10px; "
            "padding: 8px 12px; font-size: 13px; } "
            "QComboBox::drop-down { border: none; padding-right: 10px; } "
            "QComboBox QAbstractItemView { background-color: %1; color: %2; "
            "selection-background-color: %4; selection-color: %5; border: 1px solid %3; font-size: 13px; }")
            .arg(c.surfaceAlt).arg(c.textStrong).arg(c.border).arg(c.accent).arg(c.accentText);
    });

    m_profileCombo = new QComboBox(this);
    m_profileCombo->setMinimumWidth(140);
    m_profileCombo->setMaximumWidth(180);
    themed(m_profileCombo, []() {
        const Theme &c = currentTheme();
        return QString(
            "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 10px; "
            "padding: 8px 12px; font-size: 13px; } "
            "QComboBox::drop-down { border: none; padding-right: 10px; } "
            "QComboBox QAbstractItemView { background-color: %1; color: %2; "
            "selection-background-color: %4; selection-color: %5; border: 1px solid %3; font-size: 13px; }")
            .arg(c.surfaceAlt).arg(c.textStrong).arg(c.border).arg(c.accent).arg(c.accentText);
    });

    QVBoxLayout *selectorLayout = new QVBoxLayout();
    selectorLayout->setSpacing(8);
    selectorLayout->addWidget(m_profileCombo, 0, Qt::AlignRight);
    selectorLayout->addWidget(m_modelCombo, 0, Qt::AlignRight);
    headerTop->addLayout(selectorLayout);
    headerOuter->addLayout(headerTop);

    const auto toggleStyle = []() {
        const Theme &c = currentTheme();
        return QString(
            "QCheckBox { color: %1; font-size: 12px; spacing: 6px; background-color: %2; "
            "border: 1px solid %3; border-radius: 999px; padding: 6px 10px; } "
            "QCheckBox:disabled { color: %5; } "
            "QCheckBox::indicator { width: 12px; height: 12px; border-radius: 6px; border: 1px solid %4; background-color: transparent; } "
            "QCheckBox::indicator:checked { background-color: %4; border: 1px solid %4; }")
            .arg(c.textMuted).arg(c.surfaceAlt).arg(c.border).arg(c.accent).arg(c.textFaint);
    };
    const auto themedToggle = [&](QCheckBox *box) {
        themed(box, toggleStyle);
    };

    QHBoxLayout *toggleRow = new QHBoxLayout();
    toggleRow->setSpacing(8);

    m_uncensoredFilter = new QCheckBox("Uncensored", this);
    themedToggle(m_uncensoredFilter);
    m_uncensoredFilter->setChecked(m_settings.value("uncensoredFilter", false).toBool());
    connect(m_uncensoredFilter, &QCheckBox::toggled, this, [this]() {
        m_settings.setValue("uncensoredFilter", m_uncensoredFilter->isChecked());
        applyModelFilter();
    });
    toggleRow->addWidget(m_uncensoredFilter);

    m_streamToggle = new QCheckBox("Streaming", this);
    themedToggle(m_streamToggle);
    m_streamToggle->setChecked(m_settings.value("streamMode", true).toBool());
    m_apiClient->setStreaming(m_streamToggle->isChecked());
    connect(m_streamToggle, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.setValue("streamMode", checked);
        m_apiClient->setStreaming(checked);
    });
    toggleRow->addWidget(m_streamToggle);

    m_webSearchToggle = new QCheckBox("Web Search", this);
    themedToggle(m_webSearchToggle);
    m_webSearchToggle->setChecked(m_settings.value("webSearch", false).toBool());
    m_apiClient->setWebSearch(m_webSearchToggle->isChecked());
    connect(m_webSearchToggle, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.setValue("webSearch", checked);
        m_apiClient->setWebSearch(checked);
        updateHeaderState();
    });
    toggleRow->addWidget(m_webSearchToggle);

    m_autoScrollToggle = new QCheckBox("Auto Scroll", this);
    themedToggle(m_autoScrollToggle);
    m_autoScrollToggle->setChecked(m_settings.value("autoScroll", true).toBool());
    m_autoScroll = m_autoScrollToggle->isChecked();
    connect(m_autoScrollToggle, &QCheckBox::toggled, this, &MainWindow::onToggleAutoScroll);
    toggleRow->addWidget(m_autoScrollToggle);

    toggleRow->addStretch();
    headerOuter->addLayout(toggleRow);
    mainLayout->addWidget(headerFrame);

    m_searchBar = new ChatSearchBar(mainPanel);
    m_searchBar->setVisible(false);
    connect(m_searchBar, &ChatSearchBar::searchTextChanged, this, &MainWindow::onSearchTextChanged);
    connect(m_searchBar, &ChatSearchBar::findNext, this, &MainWindow::onFindNext);
    connect(m_searchBar, &ChatSearchBar::findPrevious, this, &MainWindow::onFindPrevious);
    connect(m_searchBar, &ChatSearchBar::closed, this, &MainWindow::hideSearchBar);
    themed(m_searchBar, []() {
        const Theme &s = currentTheme();
        return QString(
            "QFrame { background-color: %1; border: 1px solid %2; border-radius: 14px; padding: 6px; } "
            "QLineEdit { background-color: %3; color: %4; border: 1px solid %2; border-radius: 8px; padding: 6px 8px; font-size: 13px; } "
            "QPushButton { background-color: %5; color: %4; border: 1px solid %2; border-radius: 7px; padding: 4px 8px; font-size: 12px; } "
            "QPushButton:hover { background-color: %5; }")
            .arg(s.surfaceAlt).arg(s.border).arg(s.inputBg).arg(s.textStrong).arg(s.surfaceAlt);
    });
    mainLayout->addWidget(m_searchBar);

    m_scrollArea = new QScrollArea(mainPanel);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    themed(m_scrollArea, []() {
        return "QScrollArea { border: none; background-color: transparent; }" + scrollBarStyle();
    });

    m_chatContainer = new QWidget();
    m_chatContainer->setStyleSheet("background-color: transparent;");
    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setContentsMargins(58, 18, 58, 22);
    m_chatLayout->setSpacing(8);

    m_welcomeWidget = new QWidget(m_chatContainer);
    m_welcomeWidget->setMaximumWidth(620);
    themed(m_welcomeWidget, []() {
        const Theme &w = currentTheme();
        return QString("QWidget { background-color: %1; border: 1px solid %2; border-radius: 24px; }")
            .arg(w.surfaceAlt).arg(w.border);
    });
    QVBoxLayout *welcomeLayout = new QVBoxLayout(m_welcomeWidget);
    welcomeLayout->setContentsMargins(28, 30, 28, 28);
    welcomeLayout->setAlignment(Qt::AlignCenter);
    welcomeLayout->setSpacing(14);

    QLabel *welcomeEyebrow = new QLabel("READY WHEN YOU ARE", m_welcomeWidget);
    welcomeEyebrow->setAlignment(Qt::AlignCenter);
    themed(welcomeEyebrow, []() {
        return QString("QLabel { color: %1; font-size: 11px; font-weight: 700; letter-spacing: 2px; }")
            .arg(currentTheme().textMuted);
    });
    welcomeLayout->addWidget(welcomeEyebrow);

    QLabel *welcomeTitle = new QLabel("Talk to the model, not the UI.", m_welcomeWidget);
    themed(welcomeTitle, []() {
        return QString("QLabel { color: %1; font-size: 30px; font-weight: 700; }").arg(currentTheme().textStrong);
    });
    welcomeTitle->setAlignment(Qt::AlignCenter);
    welcomeLayout->addWidget(welcomeTitle);

    QLabel *welcomeSubtitle = new QLabel("Pick a model if you need to, then just type. Everything else stays quiet until you want it.", m_welcomeWidget);
    welcomeSubtitle->setWordWrap(true);
    themed(welcomeSubtitle, []() {
        return QString("QLabel { color: %1; font-size: 14px; line-height: 1.5; }").arg(currentTheme().textMuted);
    });
    welcomeSubtitle->setAlignment(Qt::AlignCenter);
    welcomeLayout->addWidget(welcomeSubtitle);

    m_chatLayout->addWidget(m_welcomeWidget, 0, Qt::AlignCenter);
    m_chatLayout->addStretch();

    m_scrollArea->setWidget(m_chatContainer);
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::valueChanged, this, [this](int) {
        saveCurrentChatScrollPosition();
    });
    // Only user actions change follow intent; animation and layout changes do not.
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::actionTriggered, this, [this](int action) {
        QScrollBar *bar = m_scrollArea->verticalScrollBar();
        const bool movingUp = action == QAbstractSlider::SliderSingleStepSub ||
            action == QAbstractSlider::SliderPageStepSub ||
            action == QAbstractSlider::SliderToMinimum || bar->sliderPosition() < bar->value();
        m_stickToBottom = !movingUp && bar->maximum() - bar->sliderPosition() <= 48;
        if (!m_stickToBottom) m_scrollFollowTimer->stop();
        else scrollToBottom(false);
    });
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::sliderPressed, this, [this]() {
        m_stickToBottom = false;
        m_scrollFollowTimer->stop();
    });
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::sliderReleased, this, [this]() {
        QScrollBar *bar = m_scrollArea->verticalScrollBar();
        m_stickToBottom = bar->maximum() - bar->sliderPosition() <= 48;
        scrollToBottom(false);
    });
    // Re-stick to the bottom whenever the content grows after layout settles.
    // Scrolling to maximum() directly from a chunk handler races with the
    // deferred word-wrap relayout and makes the view jump back and forth.
    connect(m_scrollArea->verticalScrollBar(), &QScrollBar::rangeChanged, this, [this](int, int max) {
        Q_UNUSED(max);
        if (m_autoScroll && m_stickToBottom &&
            !m_scrollArea->verticalScrollBar()->isSliderDown() &&
            !m_scrollFollowTimer->isActive()) {
            m_scrollFollowTimer->start();
        }
    });
    mainLayout->addWidget(m_scrollArea, 1);

    QFrame *inputFrame = new QFrame(mainPanel);
    themed(inputFrame, []() {
        const Theme &i = currentTheme();
        return QString("QFrame { background-color: %1; border: 1px solid %2; border-radius: 20px; }")
            .arg(i.surface).arg(i.border);
    });
    QVBoxLayout *inputWithCounter = new QVBoxLayout(inputFrame);
    inputWithCounter->setContentsMargins(18, 14, 18, 14);
    inputWithCounter->setSpacing(10);

    QLabel *composerHint = new QLabel("Enter to send, Shift+Enter for a new line.", inputFrame);
    themed(composerHint, []() {
        return QString("QLabel { color: %1; font-size: 11px; }").arg(currentTheme().textMuted);
    });
    inputWithCounter->addWidget(composerHint, 0, Qt::AlignLeft);

    QWidget *inputWrapper = new QWidget(inputFrame);
    inputWrapper->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    themed(inputWrapper, []() {
        const Theme &i = currentTheme();
        return QString("QWidget { background-color: %1; border: 1px solid %2; border-radius: 22px; }")
            .arg(i.surfaceAlt).arg(i.border);
    });
    QVBoxLayout *inputWrapperLayout = new QVBoxLayout(inputWrapper);
    inputWrapperLayout->setContentsMargins(12, 12, 12, 10);
    inputWrapperLayout->setSpacing(8);

    m_imagePreview = new QLabel(inputWrapper);
    m_imagePreview->setMaximumHeight(72);
    m_imagePreview->setMaximumWidth(220);
    m_imagePreview->setScaledContents(true);
    themed(m_imagePreview, []() {
        const Theme &i = currentTheme();
        return QString("QLabel { background-color: %1; border: 1px solid %2; border-radius: 10px; padding: 4px; }")
            .arg(i.inputBg).arg(i.border);
    });
    m_imagePreview->setVisible(false);
    m_imagePreview->setAlignment(Qt::AlignCenter);
    inputWrapperLayout->addWidget(m_imagePreview, 0, Qt::AlignLeft);

    QHBoxLayout *inputInner = new QHBoxLayout();
    inputInner->setContentsMargins(0, 0, 0, 0);
    inputInner->setSpacing(8);
    inputInner->addWidget(m_attachButton, 0, Qt::AlignBottom);
    inputInner->addWidget(m_inputField, 1);
    inputInner->addWidget(m_sendButton, 0, Qt::AlignBottom);
    inputWrapperLayout->addLayout(inputInner);

    QHBoxLayout *inputFooter = new QHBoxLayout();
    inputFooter->setContentsMargins(2, 0, 2, 0);
    inputFooter->setSpacing(8);
    QLabel *dropHint = new QLabel("You can also drag an image into the window.", inputWrapper);
    themed(dropHint, []() {
        return QString("QLabel { color: %1; font-size: 11px; }").arg(currentTheme().textMuted);
    });
    m_charCounter = new QLabel("0", inputFrame);
    themed(m_charCounter, []() {
        return QString("QLabel { color: %1; font-size: 11px; }").arg(currentTheme().textMuted);
    });
    inputFooter->addWidget(dropHint);
    inputFooter->addStretch();
    inputFooter->addWidget(m_charCounter);
    inputWrapperLayout->addLayout(inputFooter);

    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLayout->setContentsMargins(18, 0, 18, 0);
    inputLayout->addWidget(inputWrapper, 1);
    inputWithCounter->addLayout(inputLayout);
    mainLayout->addWidget(inputFrame);

    m_splitter->addWidget(mainPanel);
    m_splitter->setStretchFactor(0, 0);
    m_splitter->setStretchFactor(1, 1);
    m_splitter->setSizes({300, 900});

    setCentralWidget(m_splitter);

    m_statusBar = statusBar();
    themed(m_statusBar, []() {
        const Theme &s = currentTheme();
        return QString("QStatusBar { background-color: %1; color: %2; font-size: 12px; border-top: 1px solid %3; } "
                       "QStatusBar QLabel { color: %2; } "
                       "QStatusBar::item { border: none; }")
            .arg(s.sidebar).arg(s.textMuted).arg(s.border);
    });

    m_statusConnection = new QLabel("Ready", m_statusBar);
    m_statusModel = new QLabel(m_statusBar);
    m_statusTokens = new QLabel(m_statusBar);
    m_statusResponseTime = new QLabel(m_statusBar);
    m_statusContextUsage = new QLabel(m_statusBar);
    m_statusDuration = new QLabel(m_statusBar);
    m_statusSpeed = new QLabel(m_statusBar);

    m_statusBar->addWidget(m_statusConnection, 1);
    m_statusBar->addPermanentWidget(m_statusSpeed);
    m_statusBar->addPermanentWidget(m_statusDuration);
    m_statusBar->addPermanentWidget(m_statusContextUsage);
    m_statusBar->addPermanentWidget(m_statusTokens);
    m_statusBar->addPermanentWidget(m_statusResponseTime);
    m_statusBar->addPermanentWidget(m_statusModel);
}

void MainWindow::setupMenu()
{
    QMenuBar *menuBar = this->menuBar();
    QMenu *fileMenu = menuBar->addMenu(makeLineIcon(AppIconGlyph::File), "File");

    QAction *newChatAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::NewChat), "New Chat");
    newChatAction->setShortcut(QKeySequence("Ctrl+N"));
    connect(newChatAction, &QAction::triggered, this, &MainWindow::onNewChat);

    QAction *settingsAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Settings), "Settings");
    settingsAction->setShortcut(QKeySequence("Ctrl+,"));
    connect(settingsAction, &QAction::triggered, this, &MainWindow::onSettings);

    QAction *profilesAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Profiles), "Manage Profiles...");
    connect(profilesAction, &QAction::triggered, this, &MainWindow::onManageProfiles);

    fileMenu->addSeparator();

    QAction *clearAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::ClearChat), "Clear Current Chat");
    connect(clearAction, &QAction::triggered, [this]() {
        if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
            return;
        }
        m_apiClient->cancelCurrentRequest();
        m_chatSessions[m_currentChatIndex].messages.clear();
        m_chatSessions[m_currentChatIndex].messageCount = 0;
        m_currentChatImage.clear();
        if (m_imagePreview) {
            m_imagePreview->clear();
            m_imagePreview->setVisible(false);
        }
        clearChatDisplay();
        saveChatSessions();
        showWelcomeScreen();
    });

    QAction *deleteAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Trash), "Delete Current Chat");
    connect(deleteAction, &QAction::triggered, this, &MainWindow::onDeleteChat);

    QAction *exportAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Export), "Export Chat...");
    exportAction->setShortcut(QKeySequence("Ctrl+E"));
    connect(exportAction, &QAction::triggered, this, &MainWindow::onExportChat);

    QAction *recoverAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Refresh), "Recover Lost Chats...");
    connect(recoverAction, &QAction::triggered, this, &MainWindow::onRecoverChats);

    QAction *backupExportAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Save), "Export Backup Snapshot...");
    connect(backupExportAction, &QAction::triggered, this, &MainWindow::onExportBackupSnapshot);

    fileMenu->addSeparator();

    QAction *themeAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Theme), "Toggle Theme");
    themeAction->setShortcut(QKeySequence("Ctrl+T"));
    connect(themeAction, &QAction::triggered, this, &MainWindow::onToggleTheme);

    QAction *advancedAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Advanced), "Advanced Settings...");
    advancedAction->setShortcut(QKeySequence("Ctrl+Shift+S"));
    connect(advancedAction, &QAction::triggered, this, &MainWindow::onAdvancedSettings);

    fileMenu->addSeparator();

    QAction *quitAction = fileMenu->addAction(makeLineIcon(AppIconGlyph::Quit), "Quit");
    quitAction->setShortcut(QKeySequence("Ctrl+Q"));
    connect(quitAction, &QAction::triggered, this, &QMainWindow::close);
}

void MainWindow::loadSettings()
{
    QString apiKey = SecretStore::unprotect(m_settings.value("apiKey").toString());

    m_apiClient->setApiKey(apiKey);
    restoreModelSelection();

    m_apiClient->setSystemPrompt(SecretStore::unprotect(m_settings.value("systemPrompt").toString()));

    double temperature = m_settings.value("temperature", 0.7).toDouble();
    m_apiClient->setTemperature(temperature);

    int maxTokens = m_settings.value("maxTokens", 0).toInt();
    m_apiClient->setMaxTokens(maxTokens);

    bool webSearch = m_settings.value("webSearch", false).toBool();
    m_apiClient->setWebSearch(webSearch);

    m_chatFontSize = qBound(10, m_settings.value("chatFontSize", 15).toInt(), 30);
}

int MainWindow::chatIndexForId(const QString &id) const
{
    if (id.isEmpty()) {
        return -1;
    }

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        if (m_chatSessions[i].id == id) {
            return i;
        }
    }

    return -1;
}

void MainWindow::restoreLastChatSelection()
{
    if (m_chatSessions.isEmpty()) {
        createNewChat();
        return;
    }

    const QString lastChatId = m_settings.value("lastChatId").toString();
    int index = chatIndexForId(lastChatId);
    if (index < 0) {
        index = 0;
    }

    switchToChat(index);
    updateChatList();
}

void MainWindow::fetchModels()
{
    if (!checkApiKey()) return;
    // Block combo signals while swapping in the placeholder: clear()/addItem()
    // would otherwise emit currentTextChanged and have onModelChanged persist
    // ""/"Loading models..." over the user's saved model selection.
    {
        QSignalBlocker blocker(m_modelCombo);
        m_modelCombo->clear();
        m_modelCombo->addItem("Loading models...");
    }
    m_modelCombo->setEnabled(false);
    m_apiClient->fetchModels();
}

void MainWindow::saveSettings()
{
    m_settings.setValue("chatFontSize", m_chatFontSize);
    m_settings.sync();
}

bool MainWindow::checkApiKey()
{
    if (m_apiClient->apiKey().trimmed().isEmpty()) {
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
    newChat.messageCount = 0;
    newChat.pinned = false;
    newChat.messagesLoaded = true;
    m_chatSessions.prepend(newChat);
    // prepending shifts every session index, so an in-flight request would end
    // up naming a different chat than the one it was sent from.
    if (m_requestChatIndex >= 0) {
        m_requestChatIndex++;
    }
    m_currentChatIndex = 0;
    clearChatDisplay();
    showWelcomeScreen();
    m_inputField->clear();
    m_currentChatImage.clear();
    if (m_imagePreview) {
        m_imagePreview->clear();
        m_imagePreview->setVisible(false);
    }
    updateCharCounter();
    updateChatList();
    saveChatSessions();
    m_settings.setValue("lastChatId", newChat.id);
    updateHeaderState();
}

void MainWindow::switchToChat(int index, bool force)
{
    if (index < 0 || index >= m_chatSessions.size()) return;
    // A migrated session arrives with its messages already in memory and
    // m_currentChatIndex already pointing at it, so the early return would
    // leave the transcript unbuilt and the welcome screen on top of it.
    if (!force && index == m_currentChatIndex && m_chatSessions[index].messagesLoaded
        && !m_chatSessions[index].messages.isEmpty()) {
        return;
    }

    const QString currentDraft = m_inputField->toPlainText().trimmed();
    const bool discardDraft = index != m_currentChatIndex && !currentDraft.isEmpty();
    if (discardDraft) {
        QMessageBox::StandardButton result = QMessageBox::question(this, "Unsaved Draft",
            "You have an unsaved draft. Discard it?",
            QMessageBox::Discard | QMessageBox::Cancel);
        if (result == QMessageBox::Cancel) return;
    }

    if (discardDraft) {
        clearDraft();
        m_inputField->clear();
    } else {
        saveDraft();
    }
    // A pending attachment belongs to the chat it was added in.
    m_currentChatImage.clear();
    if (m_imagePreview) {
        m_imagePreview->clear();
        m_imagePreview->setVisible(false);
    }
    saveCurrentChatScrollPosition();
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size() &&
        m_currentChatIndex != index && m_chatSessions[m_currentChatIndex].messagesLoaded) {
        saveChatMessages(m_currentChatIndex);
        // The chat the in-flight request belongs to must stay loaded, otherwise
        // its messages list is cleared and the answer has nowhere to land.
        if (m_currentChatIndex != m_requestChatIndex) {
            unloadChatMessages(m_currentChatIndex);
        }
    }

    m_currentChatIndex = index;
    loadChatMessages(index);
    rebuildCurrentChatView();
    m_settings.setValue("lastChatId", m_chatSessions[index].id);

    loadDraft();
    updateHeaderState();
    updateContextUsage();
    updateChatDuration();
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
            // The list can be rebuilt from a widget's own context-menu
            // callback.  Deleting that widget synchronously would destroy
            // the sender while Qt is still dispatching its event.
            item->widget()->deleteLater();
        }
        delete item;
    }

    QString lowerQuery = query.toLower();
    ++m_chatListRenderGeneration;
    const int renderGeneration = m_chatListRenderGeneration;
    m_chatListRenderIndices.clear();

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

    m_chatListRenderIndices = pinnedIndices + unpinnedIndices;
    m_chatListRenderCursor = 0;

    layout->addStretch();
    continueChatListRender(renderGeneration);
}

void MainWindow::continueChatListRender(int generation)
{
    if (generation != m_chatListRenderGeneration) {
        return;
    }

    QVBoxLayout *layout = qobject_cast<QVBoxLayout*>(m_chatListContainer->layout());
    if (!layout) return;

    const int total = m_chatListRenderIndices.size();
    if (m_chatListRenderCursor >= total) {
        return;
    }

    constexpr int kBatchSize = 14;
    int rendered = 0;
    QElapsedTimer timer;
    timer.start();

    while (m_chatListRenderCursor < total && rendered < kBatchSize) {
        int idx = m_chatListRenderIndices[m_chatListRenderCursor++];
        const ChatSession &chat = m_chatSessions[idx];
        QStringList metaParts;
        if (chat.messageCount == 0) {
            metaParts << "Empty";
        } else {
            metaParts << QString("%1 msg").arg(chat.messageCount);
        }
        if (chat.pinned) {
            metaParts << "Pinned";
        }

        ChatListItem *chatItem = new ChatListItem(
            chat.title,
            metaParts.join("  •  "),
            idx,
            chat.pinned,
            m_chatListContainer
        );
        if (idx == m_currentChatIndex) {
            chatItem->setActive(true);
        }
        layout->insertWidget(layout->count() - 1, chatItem);

        connect(chatItem, &ChatListItem::clicked, this, [this, idx]() {
            switchToChat(idx);
            updateChatList();
        }, Qt::QueuedConnection);
        connect(chatItem, &ChatListItem::deleteClicked, this, [this, idx]() {
            deleteChatAtRow(idx);
        }, Qt::QueuedConnection);
        connect(chatItem, &ChatListItem::renameRequested, this, [this, idx]() {
            renameChat(idx);
        }, Qt::QueuedConnection);
        connect(chatItem, &ChatListItem::pinRequested, this, [this, idx]() {
            togglePinChat(idx);
        }, Qt::QueuedConnection);

        ++rendered;
        if (timer.elapsed() >= 8) {
            break;
        }
    }

    if (m_chatListRenderCursor < total) {
        QTimer::singleShot(0, this, [this, generation]() {
            continueChatListRender(generation);
        });
    }
}

QString MainWindow::generateChatTitle(const QString &firstMessage)
{
    QString title = firstMessage.simplified();
    if (title.isEmpty()) {
        title = "New Chat";
    }
    if (title.length() > 30) {
        title = title.left(30) + "...";
    }
    return title;
}

void MainWindow::refreshChatViewport()
{
    m_chatLayout->invalidate();
    m_chatContainer->adjustSize();
    m_scrollArea->widget()->updateGeometry();
    m_scrollArea->viewport()->update();
}

void MainWindow::removeTrailingSpacer()
{
    if (!m_chatLayout || m_chatLayout->count() == 0) {
        return;
    }

    QLayoutItem *lastItem = m_chatLayout->itemAt(m_chatLayout->count() - 1);
    if (lastItem && !lastItem->widget() && !lastItem->layout() && lastItem->spacerItem()) {
        QLayoutItem *removed = m_chatLayout->takeAt(m_chatLayout->count() - 1);
        delete removed;
    }
}

void MainWindow::appendBottomSpacer()
{
    if (!m_chatLayout) {
        return;
    }

    QLayoutItem *lastItem = m_chatLayout->count() > 0 ? m_chatLayout->itemAt(m_chatLayout->count() - 1) : nullptr;
    if (!(lastItem && !lastItem->widget() && !lastItem->layout() && lastItem->spacerItem())) {
        m_chatLayout->addStretch();
    }
}

void MainWindow::rebuildCurrentChatView()
{
    if (!m_chatContainer || !m_scrollArea) {
        return;
    }

    ++m_chatRenderGeneration;
    m_scrollFollowTimer->stop();
    m_stickToBottom = false;

    // Suppress intermediate repaints/relayouts while the batched render
    // reconstructs the message cards; everything is painted once at the end.
    m_chatContainer->setUpdatesEnabled(false);

    clearChatDisplay(false);

    // Captured after the clear: clearChatDisplay() invalidates any render that
    // was still pending, so reading the generation before it would make this
    // one stale and the transcript would never be drawn.
    const int renderGeneration = m_chatRenderGeneration;

    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        showWelcomeScreen();
        refreshChatViewport();
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    const auto &chat = m_chatSessions[m_currentChatIndex];
    if (chat.messages.isEmpty()) {
        showWelcomeScreen();
        refreshChatViewport();
        m_chatContainer->setUpdatesEnabled(true);
    } else {
        hideWelcomeScreen();
        m_chatRenderCursor = chat.messages.size() - 1;
        continueChatHistoryRender(renderGeneration);
    }
}

void MainWindow::clearChatDisplay(bool refresh)
{
    m_thinkingTimer->stop();

    // Every card below is destroyed, so any pointer still cached for search
    // navigation would dangle.
    m_highlightedCards.clear();
    m_currentHighlightIndex = -1;

    // Invalidate a pending batched history render: the caller re-adds cards
    // itself, so resuming the old cursor would duplicate them.
    ++m_chatRenderGeneration;
    m_chatRenderCursor = -1;

    // rebuildCurrentChatView() disables updates for the duration of a batched
    // render and re-enables them when it finishes. A pending batch is
    // invalidated here, so whoever disabled them has to give them back.
    m_chatContainer->setUpdatesEnabled(true);

    // The 50 ms render timer would otherwise keep firing with no card to
    // append to, for as long as the request lasts.
    m_streamRenderTimer->stop();
    m_pendingStreamChunk.clear();

    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        // deleteLater, not delete: this can run from a card's own signal
        // handler (Regenerate / Branch / Edit), and Qt is still dispatching
        // that signal from the widget.
        if (item->widget() && item->widget() != m_welcomeWidget) {
            item->widget()->deleteLater();
        } else if (item->layout()) {
            delete item->layout();
        }
        delete item;
    }
    appendBottomSpacer();
    m_streamingCard = nullptr;
    m_thinkingRowWidget = nullptr;
    m_thinkingIndicator = nullptr;
    if (refresh) {
        refreshChatViewport();
    }
}

void MainWindow::addMessageCard(const QString &role, const QString &content, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs)
{
    addMessageCardWithCard(role, content, promptTokens, completionTokens, totalTokens, responseTimeMs);
}

ChatMessageCard* MainWindow::addMessageCardWithCard(const QString &role, const QString &content, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs, bool prepend)
{
    hideWelcomeScreen(false);

    removeTrailingSpacer();
    ChatMessageCard *card = new ChatMessageCard(role, content, m_chatContainer);
    card->setTimestamp(QDateTime::currentDateTime());
    card->showCopyButton(true);
    card->setContentFontSize(m_chatFontSize);

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

    if (prepend) {
        m_chatLayout->insertWidget(0, card);
    } else {
        m_chatLayout->addWidget(card);
    }
    appendBottomSpacer();
    return card;
}

void MainWindow::continueChatHistoryRender(int generation)
{
    if (generation != m_chatRenderGeneration) {
        // A newer render owns the updates-disabled window; it will re-enable.
        return;
    }

    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    const auto &chat = m_chatSessions[m_currentChatIndex];
    if (chat.messages.isEmpty()) {
        m_chatContainer->setUpdatesEnabled(true);
        return;
    }

    const bool firstChunk = (m_chatRenderCursor == chat.messages.size() - 1);
    constexpr int kFirstBatchSize = 8;
    constexpr int kLaterBatchSize = 24;
    constexpr int kFirstChunkBudgetMs = 12;
    constexpr int kLaterChunkBudgetMs = 24;
    const int maxCards = firstChunk ? kFirstBatchSize : kLaterBatchSize;
    const int maxBudgetMs = firstChunk ? kFirstChunkBudgetMs : kLaterChunkBudgetMs;
    QElapsedTimer timer;
    timer.start();

    int rendered = 0;
    while (m_chatRenderCursor >= 0 && rendered < maxCards) {
        const auto &msg = chat.messages[m_chatRenderCursor];
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        ChatMessageCard *card = addMessageCardWithCard(
            msg.role,
            displayText,
            msg.promptTokens,
            msg.completionTokens,
            msg.totalTokens,
            0,
            true
        );
        if (card) {
            card->setMessageIndex(m_chatRenderCursor);
            card->setTimestamp(QDateTime::currentDateTime());
        }
        --m_chatRenderCursor;
        ++rendered;
        if (timer.elapsed() >= maxBudgetMs) {
            break;
        }
    }

    if (m_chatRenderCursor >= 0) {
        // No intermediate refreshChatViewport(): with updates disabled it
        // would only burn O(n) layout work per batch (O(n^2) overall).
        QTimer::singleShot(0, this, [this, generation]() {
            continueChatHistoryRender(generation);
        });
    } else {
        m_chatContainer->setUpdatesEnabled(true);
        refreshChatViewport();
        restoreCurrentChatScrollPosition();
    }
}

void MainWindow::saveCurrentChatScrollPosition()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size() || !m_scrollArea) {
        return;
    }

    if (QScrollBar *bar = m_scrollArea->verticalScrollBar()) {
        m_chatSessions[m_currentChatIndex].scrollPosition = bar->value();
    }
}

void MainWindow::restoreCurrentChatScrollPosition()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size() || !m_scrollArea) {
        return;
    }

    const int savedPosition = m_chatSessions[m_currentChatIndex].scrollPosition;
    const int renderGeneration = m_chatRenderGeneration;
    m_scrollFollowTimer->stop();
    m_stickToBottom = false;
    QTimer::singleShot(0, this, [this, savedPosition, renderGeneration]() {
        if (renderGeneration != m_chatRenderGeneration ||
            m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size() || !m_scrollArea) {
            return;
        }

        if (QScrollBar *bar = m_scrollArea->verticalScrollBar()) {
            bar->setValue(qBound(0, savedPosition, bar->maximum()));
            m_stickToBottom = isNearBottom();
        }
    });
}

void MainWindow::flushStreamingChunks()
{
    if (!m_streamingCard || m_pendingStreamChunk.isEmpty()) {
        if (m_streamRenderTimer->isActive() && m_pendingStreamChunk.isEmpty()) {
            m_streamRenderTimer->stop();
        }
        return;
    }

    QString chunk = m_pendingStreamChunk;
    m_pendingStreamChunk.clear();

    if (m_streamingCard->content().isEmpty()) {
        chunk = chunk.trimmed();
    }

    if (chunk.isEmpty()) {
        if (m_streamRenderTimer->isActive()) {
            m_streamRenderTimer->stop();
        }
        return;
    }

    m_streamingCard->appendContent(chunk);
    m_streamTokenCount += qMax(1, chunk.count(' ') + chunk.count('\n'));
    updateStreamingSpeed(chunk);
    scrollToBottom(false);

    if (m_streamRenderTimer->isActive() && m_pendingStreamChunk.isEmpty()) {
        m_streamRenderTimer->stop();
    }
}

bool MainWindow::isNearBottom(int tolerance) const
{
    if (!m_scrollArea) {
        return true;
    }

    QScrollBar *bar = m_scrollArea->verticalScrollBar();
    if (!bar) {
        return true;
    }

    return (bar->maximum() - bar->value()) <= tolerance;
}

void MainWindow::scrollToBottom(bool force)
{
    if (!m_autoScroll) return;
    if (!force && !m_stickToBottom) return;

    m_stickToBottom = true;
    QScrollBar *bar = m_scrollArea ? m_scrollArea->verticalScrollBar() : nullptr;
    if (bar) {
        // Let word-wrap/layout settle and perform at most one movement per
        // frame. rangeChanged will request another frame only if needed.
        if (!m_scrollFollowTimer->isActive()) {
            m_scrollFollowTimer->start();
        }
    }
}

void MainWindow::showWelcomeScreen(bool refresh)
{
    if (m_welcomeWidget) {
        if (m_chatLayout->indexOf(m_welcomeWidget) == -1) {
            m_chatLayout->insertWidget(0, m_welcomeWidget, 0, Qt::AlignCenter);
        }
        m_welcomeWidget->setVisible(true);
        m_welcomeWidget->show();
        m_welcomeWidget->raise();
        if (refresh) {
            refreshChatViewport();
        }
    }
}

void MainWindow::hideWelcomeScreen(bool refresh)
{
    if (m_welcomeWidget) {
        m_welcomeWidget->setVisible(false);
        if (refresh) {
            refreshChatViewport();
        }
    }
}

void MainWindow::showThinkingIndicator()
{
    if (m_thinkingIndicator) return;

    hideWelcomeScreen();

    removeTrailingSpacer();

    m_thinkingRowWidget = new QWidget(m_chatContainer);
    QHBoxLayout *thinkingRow = new QHBoxLayout(m_thinkingRowWidget);
    thinkingRow->setContentsMargins(32, 0, 32, 0);
    thinkingRow->setSpacing(12);

    m_thinkingIndicator = new QLabel(m_thinkingRowWidget);
    m_thinkingIndicator->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    m_thinkingIndicator->setStyleSheet(
        "QLabel { "
        "  color: " + currentTheme().textMuted + "; "
        "  font-size: 15px; "
        "  font-style: italic; "
        "  padding: 12px 16px; "
        "}"
    );
    m_theme->registerStyle(m_thinkingIndicator, []() {
        return QString(
            "QLabel { "
            "  color: " + currentTheme().textMuted + "; "
            "  font-size: 15px; "
            "  font-style: italic; "
            "  padding: 12px 16px; "
            "}");
    });

    AvatarLabel *aiAvatar = new AvatarLabel("assistant", m_thinkingRowWidget);
    aiAvatar->setFixedSize(28, 28);
    thinkingRow->addWidget(aiAvatar, 0, Qt::AlignTop);

    thinkingRow->addWidget(m_thinkingIndicator, 1);
    thinkingRow->addStretch();

    m_chatLayout->addWidget(m_thinkingRowWidget);
    appendBottomSpacer();

    m_thinkingDots = 0;
    m_thinkingIndicator->setText("Thinking");
    m_thinkingTimer->start(500);

    scrollToBottom();
}

void MainWindow::hideThinkingIndicator(bool refresh)
{
    m_thinkingTimer->stop();
    if (!m_thinkingIndicator) return;

    delete m_thinkingRowWidget;
    m_thinkingRowWidget = nullptr;
    m_thinkingIndicator = nullptr;
    if (refresh) {
        refreshChatViewport();
    }
}

void MainWindow::applyModelFilter()
{
    QString savedModel = m_settings.value("model", "venice-uncensored").toString();
    bool uncensoredOnly = m_uncensoredFilter->isChecked();

    QSignalBlocker blocker(m_modelCombo);
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
    } else if (!savedModel.isEmpty()) {
        m_modelCombo->setEditText(savedModel);
    }
    m_apiClient->setModel(m_modelCombo->currentText());
}

void MainWindow::onAttachImage()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", "",
        "Images (*.png *.jpg *.jpeg *.gif *.bmp *.webp)");
    attachImageFromPath(filePath);
}

void MainWindow::beginRequest(int chatIndex)
{
    m_requestChatIndex = chatIndex;
    m_streamedContent.clear();
    m_pendingStreamChunk.clear();
    m_retryCount = 0;
    m_streamStartTime = QDateTime::currentMSecsSinceEpoch();
    m_streamTokenCount = 0;

    m_inputField->setEnabled(false);
    setRequestInFlight(true);

    if (chatIndex == m_currentChatIndex) {
        showThinkingIndicator();
    }

    m_pendingMessages = m_chatSessions[chatIndex].messages;
    m_apiClient->sendMessage(m_pendingMessages);
}

bool MainWindow::persistAssistantMessage(int chatIndex, const QString &content, int promptTokens, int completionTokens, int totalTokens)
{
    if (chatIndex < 0 || chatIndex >= m_chatSessions.size()) {
        return false;
    }
    if (!m_chatSessions[chatIndex].messagesLoaded) {
        return false;
    }
    m_chatSessions[chatIndex].messages.append({"assistant", content, promptTokens, completionTokens, totalTokens});
    m_chatSessions[chatIndex].messageCount = m_chatSessions[chatIndex].messages.size();
    // Both the sidebar subtitle and the header show the count, and neither was
    // refreshed after the first turn, so they read "1 message" for the rest of
    // the conversation. A background chat is refreshed too, since the sidebar
    // is visible while the user reads elsewhere.
    updateChatList();
    if (chatIndex == m_currentChatIndex) {
        updateHeaderState();
    }
    return true;
}

void MainWindow::rebuildCardsForMessages(const QList<ChatMessage> &messages)
{
    clearChatDisplay();
    for (int i = 0; i < messages.size(); ++i) {
        const auto &msg = messages.at(i);
        QString displayText = msg.content;
        if (!msg.imageUrl.isEmpty()) {
            displayText += "\n[Image attached]";
        }
        ChatMessageCard *card = addMessageCardWithCard(msg.role, displayText, msg.promptTokens, msg.completionTokens, msg.totalTokens);
        if (card) {
            card->setMessageIndex(i);
        }
    }
}

void MainWindow::onSendMessage()
{
    if (m_requestInFlight) {
        m_apiClient->cancelCurrentRequest();
        return;
    }

    QString text = m_inputField->toPlainText().trimmed();
    if (text.isEmpty() && m_currentChatImage.isEmpty()) return;

    if (text.startsWith("/")) {
        if (handleQuickCommand(text)) {
            m_inputField->clear();
        } else {
            m_inputField->setPlainText(text);
        }
        return;
    }

    if (!checkApiKey()) return;

    if (text.length() > kMaxInputChars) {
        QMessageBox::warning(this, "Message Too Long",
            QString("This message is %1 characters. Trim it to %2 or fewer before sending.")
                .arg(text.length()).arg(kMaxInputChars));
        return;
    }

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
    m_chatSessions[m_currentChatIndex].messageCount = m_chatSessions[m_currentChatIndex].messages.size();

    m_currentChatImage.clear();
    m_imagePreview->setVisible(false);
    m_imagePreview->clear();

    if (m_chatSessions[m_currentChatIndex].messages.size() == 1) {
        m_chatSessions[m_currentChatIndex].title = generateChatTitle(text);
        updateChatList();
        updateHeaderState();
        m_chatStartTime = QDateTime::currentDateTime();
    }

    beginRequest(m_currentChatIndex);
    saveChatSessions();

    if (m_statusConnection) {
        m_statusConnection->setText("Sending...");
    }

    scrollToBottom();
}

void MainWindow::onResponseReceived(const QString &response, int promptTokens, int completionTokens, int totalTokens, int responseTimeMs)
{
    hideThinkingIndicator();

    if (m_streamingCard) {
        m_chatLayout->removeWidget(m_streamingCard);
        delete m_streamingCard;
        m_streamingCard = nullptr;
    }
    m_streamRenderTimer->stop();
    m_pendingStreamChunk.clear();
    m_streamedContent = response;

    const int requestChat = m_requestChatIndex;
    const bool shownInCurrentChat = requestChat == m_currentChatIndex;

    if (shownInCurrentChat) {
        addMessageCard("assistant", response, promptTokens, completionTokens, totalTokens, responseTimeMs);
    }

    if (persistAssistantMessage(requestChat, response, promptTokens, completionTokens, totalTokens)) {
        saveChatSessions();
        if (shownInCurrentChat) {
            updateContextUsage();
        }
    }

    if (m_statusConnection) {
        m_statusConnection->setText("Ready");
    }
    if (m_statusTokens && totalTokens > 0) {
        m_statusTokens->setText(QString("%1 tokens").arg(totalTokens));
    }
    if (m_statusResponseTime && responseTimeMs > 0) {
        m_statusResponseTime->setText(QString("%.1fs").arg(responseTimeMs / 1000.0));
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);

    scrollToBottom();

    saveChatSessions();

    updateContextUsage();
    updateHeaderState();
    updateChatDuration();
    playNotificationSound();

    // Non-streaming requests never emit responseFinished, so the request state
    // has to be torn down here or it leaks into the next chat switch.
    m_retryCount = 0;
    m_pendingMessages.clear();
    m_requestChatIndex = -1;
}

void MainWindow::onResponseChunk(const QString &chunk)
{
    m_streamedContent += chunk;
    m_pendingStreamChunk += chunk;

    // The view can be rebuilt mid-stream (chat switch, regenerate): rebuild the
    // live card from the full text received so far instead of losing the part
    // that was rendered into the destroyed card.
    if (m_requestChatIndex == m_currentChatIndex) {
        if (!m_streamingCard) {
            hideThinkingIndicator(false);
            removeTrailingSpacer();
            m_streamingCard = new ChatMessageCard("assistant", m_streamedContent, m_chatContainer);
            m_streamingCard->setContentFontSize(m_chatFontSize);
            m_streamingCard->setStreaming(true);
            m_streamingCard->setTimestamp(QDateTime::currentDateTime());
            // The live card is built here rather than through
            // addMessageCardWithCard(), so it has to be given the same index
            // and the same actions the reloaded copy gets. Without this the
            // freshest answer had no Regenerate, Branch or hover timestamp
            // while the same message acquired them after a chat switch.
            m_streamingCard->setMessageIndex(m_chatSessions[m_currentChatIndex].messages.size());
            m_streamingCard->showRegenerateButton(true);
            m_streamingCard->showBranchButton(true);
            connect(m_streamingCard, &ChatMessageCard::regenerateRequested,
                    this, &MainWindow::onRegenerateResponse);
            connect(m_streamingCard, &ChatMessageCard::branchRequested,
                    this, &MainWindow::onBranchConversation);
            m_chatLayout->addWidget(m_streamingCard);
            appendBottomSpacer();
            // The card was seeded with everything received so far.
            m_pendingStreamChunk.clear();
            refreshChatViewport();
        }
        if (!chunk.isEmpty() && !m_streamRenderTimer->isActive()) {
            m_streamRenderTimer->start();
        }
    }

    if (m_statusConnection) {
        m_statusConnection->setText("Streaming...");
    }
}

void MainWindow::onResponseFinished(int responseTimeMs)
{
    hideThinkingIndicator();
    flushStreamingChunks();
    m_streamRenderTimer->stop();

    const int requestChat = m_requestChatIndex;
    const bool shownInCurrentChat = requestChat == m_currentChatIndex;

    if (m_streamingCard) {
        m_streamingCard->setStreaming(false);
        m_streamingCard->showCopyButton(true);
    }

    if (persistAssistantMessage(requestChat, m_streamedContent, 0, 0, 0)) {
        saveChatSessions();
        if (shownInCurrentChat) {
            updateContextUsage();
        }
    }

    if (m_statusConnection) {
        m_statusConnection->setText("Ready");
    }
    if (m_statusResponseTime && responseTimeMs > 0) {
        m_statusResponseTime->setText(QString("%.1fs").arg(responseTimeMs / 1000.0));
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);
    m_retryCount = 0;
    m_pendingMessages.clear();

    if (m_statusSpeed) m_statusSpeed->clear();
    m_streamTokenCount = 0;
    m_requestChatIndex = -1;
    m_pendingStreamChunk.clear();
    m_streamedContent.clear();
    m_streamingCard = nullptr;

    updateChatDuration();
    playNotificationSound();
}

void MainWindow::onErrorOccurred(const QString &error)
{
    qDebug() << "API Error:" << error;

    hideThinkingIndicator();

    m_retryCount = 0;
    m_pendingMessages.clear();

    // Drop the partial render: keeping it would prepend this attempt's tail to
    // the next response.
    m_streamRenderTimer->stop();
    m_pendingStreamChunk.clear();
    m_streamedContent.clear();
    if (m_statusSpeed) m_statusSpeed->clear();

    if (m_streamingCard) {
        m_chatLayout->removeWidget(m_streamingCard);
        delete m_streamingCard;
        m_streamingCard = nullptr;
    }

    addMessageCard("error", "Error: " + error);

    if (m_statusConnection) {
        m_statusConnection->setText("Error");
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);
    m_requestChatIndex = -1;
}

void MainWindow::onRequestCancelled()
{
    hideThinkingIndicator();
    m_retryTimer->stop();
    flushStreamingChunks();
    m_streamRenderTimer->stop();

    const int requestChat = m_requestChatIndex;
    const bool shownInCurrentChat = requestChat == m_currentChatIndex;
    const QString partialContent = m_streamedContent;

    if (m_streamingCard) {
        m_streamingCard->setStreaming(false);
        m_streamingCard->showCopyButton(true);
    }

    const bool keepPartial = !partialContent.trimmed().isEmpty();
    if (keepPartial && persistAssistantMessage(requestChat, partialContent, 0, 0, 0)) {
        saveChatSessions();
        if (shownInCurrentChat) {
            updateContextUsage();
        }
    } else if (m_streamingCard) {
        m_chatLayout->removeWidget(m_streamingCard);
        delete m_streamingCard;
    }

    m_streamingCard = nullptr;
    m_pendingStreamChunk.clear();
    m_streamedContent.clear();
    m_requestChatIndex = -1;
    m_retryCount = 0;
    m_pendingMessages.clear();
    m_streamTokenCount = 0;
    if (m_statusSpeed) m_statusSpeed->clear();
    if (m_statusConnection) {
        m_statusConnection->setText("Stopped");
    }

    m_inputField->setEnabled(true);
    m_inputField->setFocus();
    setRequestInFlight(false);
}

void MainWindow::onSettings()
{
    const QString currentKey = SecretStore::unprotect(m_settings.value("apiKey").toString());

    bool ok;
    QString apiKey = QInputDialog::getText(this, "Settings", "Venice.ai API Key:",
                                           QLineEdit::Password, currentKey, &ok);
    if (ok) {
        m_settings.setValue("apiKey", SecretStore::protect(apiKey));
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

    loadChatMessages(m_currentChatIndex);

    const auto &chat = m_chatSessions[m_currentChatIndex];
    QString defaultName = chat.title.isEmpty() ? "chat" : chat.title;
    defaultName.replace(QRegularExpression("[^a-zA-Z0-9\\s]"), "");

    QString filePath = QFileDialog::getSaveFileName(this, "Export Chat", defaultName + ".md", "Markdown Files (*.md);;Text Files (*.txt);;All Files (*)");
    if (filePath.isEmpty()) return;

    QString markdown;
    QTextStream out(&markdown);

    out << "# " << chat.title << "\n\n";
    out << "Exported: " << QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss") << "\n\n";
    out << "---\n\n";

    for (const auto &msg : chat.messages) {
        QString role = msg.role == "user" ? "You" : "Assistant";
        out << "## " << role << "\n\n";
        if (!msg.imageUrl.isEmpty()) {
            out << "*[Image attached]*\n\n";
        }
        out << msg.content << "\n\n";
        if (msg.totalTokens > 0) {
            out << "*Tokens: " << msg.promptTokens << " prompt, " << msg.completionTokens << " completion, " << msg.totalTokens << " total*\n\n";
        }
        out << "---\n\n";
    }
    out.flush();

    // QSaveFile so a crash mid-export cannot leave a truncated transcript.
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not save file: " + filePath);
        return;
    }
    file.write(markdown.toUtf8());
    if (!file.commit()) {
        QMessageBox::warning(this, "Export Failed", "Could not finalise file: " + filePath);
        return;
    }
    QMessageBox::information(this, "Export Complete", "Chat exported to:\n" + filePath);
}

void MainWindow::onRecoverChats()
{
    QJsonObject snapshot;
    if (!loadChatBackupSnapshot(&snapshot)) {
        QMessageBox::information(this, "Recover Lost Chats", "No backup snapshot was found yet.");
        return;
    }

    QMap<QString, QJsonObject> backupById;
    QJsonArray sessionsArray = snapshot["sessions"].toArray();
    for (const auto &val : sessionsArray) {
        QJsonObject sessionObj = val.toObject();
        const QString id = sessionObj["id"].toString();
        if (!id.isEmpty()) {
            backupById[id] = sessionObj;
        }
    }

    QList<int> candidates = recoverableChatIndices();
    if (candidates.isEmpty()) {
        QMessageBox::information(this, "Recover Lost Chats", "No recoverable chats were found in the backup.");
        return;
    }

    QDialog dialog(this);
    dialog.setWindowTitle("Recover Lost Chats");
    dialog.setMinimumSize(760, 420);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QLabel *info = new QLabel(
        "These chats are currently empty in the app, but a backup still has message data for them.\n"
        "Select one or more rows and restore them back into the app.",
        &dialog
    );
    info->setWordWrap(true);
    layout->addWidget(info);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Title", "Current", "Backup", "Chat ID"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Stretch);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::ExtendedSelection);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setRowCount(candidates.size());

    for (int row = 0; row < candidates.size(); ++row) {
        int index = candidates[row];
        const ChatSession &chat = m_chatSessions[index];
        const QJsonObject sessionObj = backupById.value(chat.id);
        const int backupCount = sessionObj["messageCount"].toInt();

        auto *titleItem = new QTableWidgetItem(chat.title.isEmpty() ? "(untitled)" : chat.title);
        titleItem->setData(Qt::UserRole, chat.id);
        table->setItem(row, 0, titleItem);
        table->setItem(row, 1, new QTableWidgetItem(QString::number(chat.messageCount)));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(backupCount)));
        table->setItem(row, 3, new QTableWidgetItem(chat.id));
    }
    layout->addWidget(table);

    QDialogButtonBox *buttons = new QDialogButtonBox(&dialog);
    QPushButton *restoreSelectedBtn = buttons->addButton("Restore Selected", QDialogButtonBox::AcceptRole);
    QPushButton *restoreAllBtn = buttons->addButton("Restore All", QDialogButtonBox::ActionRole);
    buttons->addButton(QDialogButtonBox::Close);
    layout->addWidget(buttons);

    auto restoreRows = [this, table, &dialog](const QList<int> &rows) {
        int restored = 0;
        int failed = 0;
        for (int row : rows) {
            QTableWidgetItem *item = table->item(row, 0);
            if (!item) {
                ++failed;
                continue;
            }
            const QString chatId = item->data(Qt::UserRole).toString();
            if (restoreChatFromBackup(chatId)) {
                ++restored;
            } else {
                ++failed;
            }
        }

        if (restored > 0) {
            updateChatList();
            if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
                switchToChat(m_currentChatIndex);
            }
        }

        QMessageBox::information(
            &dialog,
            "Recover Lost Chats",
            QString("Restored %1 chat(s).%2")
                .arg(restored)
                .arg(failed > 0 ? QString("\n%1 chat(s) could not be restored.").arg(failed) : QString())
        );
    };

    connect(restoreSelectedBtn, &QPushButton::clicked, &dialog, [&]() {
        QList<int> rows;
        for (const auto &range : table->selectedRanges()) {
            for (int row = range.topRow(); row <= range.bottomRow(); ++row) {
                rows.append(row);
            }
        }
        if (rows.isEmpty()) {
            QMessageBox::information(&dialog, "Recover Lost Chats", "Select one or more rows first.");
            return;
        }
        restoreRows(rows);
    });

    connect(restoreAllBtn, &QPushButton::clicked, &dialog, [&]() {
        QList<int> rows;
        for (int row = 0; row < table->rowCount(); ++row) {
            rows.append(row);
        }
        restoreRows(rows);
    });

    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);

    dialog.exec();
}

void MainWindow::onExportBackupSnapshot()
{
    QString defaultName = QString("simpleaiclient-backup-%1.json")
        .arg(QDateTime::currentDateTime().toString("yyyyMMdd-HHmmss"));
    QString filePath = QFileDialog::getSaveFileName(
        this,
        "Export Backup Snapshot",
        defaultName,
        "JSON Files (*.json);;All Files (*)"
    );
    if (filePath.isEmpty()) {
        return;
    }

    // An explicit export stays readable on purpose: the user picked this path,
    // and loadChatBackupSnapshot() accepts both the encrypted and plain form.
    QJsonDocument doc(buildChatBackupSnapshot());
    QSaveFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::warning(this, "Export Failed", "Could not save file: " + filePath);
        return;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        QMessageBox::warning(this, "Export Failed", "Could not finalise file: " + filePath);
        return;
    }

    if (!recoverableChatIndices().isEmpty()) {
        QMessageBox::information(this, "Export Complete",
            "Backup snapshot exported to:\n" + filePath +
            "\n\nIt contains your chat transcripts in plain text. Store it accordingly.");
        return;
    }
    QMessageBox::information(this, "Export Complete", "Backup snapshot exported to:\n" + filePath);
}

void MainWindow::onToggleTheme()
{
    m_theme->setDark(!m_theme->isDark());
    m_isDarkTheme = m_theme->isDark();
    m_settings.setValue("darkTheme", m_isDarkTheme);
    restyleCards();
}

void MainWindow::onRegenerateResponse(int messageIndex)
{
    if (m_requestInFlight) return;
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    auto &messages = m_chatSessions[m_currentChatIndex].messages;
    if (messages.isEmpty()) return;

    if (messageIndex >= 0 && messageIndex < messages.size()) {
        // Regenerating an older answer drops that answer and everything after
        // it, instead of always dropping the newest one.
        messages = messages.mid(0, messageIndex);
    } else {
        // No usable index: fall back to dropping a trailing answer.
        if (messages.last().role == "assistant") {
            messages.removeLast();
        }
    }
    m_chatSessions[m_currentChatIndex].messageCount = messages.size();

    if (messages.isEmpty()) return;

    const QList<ChatMessage> requestMessages = messages;
    rebuildCardsForMessages(requestMessages);
    beginRequest(m_currentChatIndex);
    saveChatSessions();

    scrollToBottom();
}

void MainWindow::onBranchConversation(int messageIndex)
{
    // prepending shifts every session index, so an in-flight request would end
    // up pointing at a different chat than the one it was sent from.
    if (m_requestInFlight) return;
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    const auto &sourceChat = m_chatSessions[m_currentChatIndex];
    if (messageIndex < 0 || messageIndex >= sourceChat.messages.size()) return;

    ChatSession newChat;
    newChat.id = QString::number(QDateTime::currentMSecsSinceEpoch());
    newChat.title = sourceChat.title + " (branch)";
    newChat.pinned = false;
    newChat.messagesLoaded = true;

    for (int i = 0; i <= messageIndex; ++i) {
        newChat.messages.append(sourceChat.messages[i]);
    }
    newChat.messageCount = newChat.messages.size();

    m_chatSessions.prepend(newChat);
    m_currentChatIndex = 0;

    rebuildCardsForMessages(newChat.messages);

    updateChatList();
    saveChatSessions();
    scrollToBottom();
}

void MainWindow::onEditMessage(int messageIndex, const QString &newContent)
{
    if (m_requestInFlight) return;
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    auto &messages = m_chatSessions[m_currentChatIndex].messages;
    if (messageIndex < 0 || messageIndex >= messages.size()) return;

    messages[messageIndex].content = newContent;

    for (int i = messages.size() - 1; i > messageIndex; --i) {
        if (messages[i].role == "assistant" || messages[i].role == "user") {
            messages.removeAt(i);
        }
    }
    m_chatSessions[m_currentChatIndex].messageCount = messages.size();

    const QList<ChatMessage> requestMessages = messages;
    rebuildCardsForMessages(requestMessages);
    beginRequest(m_currentChatIndex);
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

    QString currentPrompt = SecretStore::unprotect(m_settings.value("systemPrompt").toString());
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
        m_settings.setValue("systemPrompt", SecretStore::protect(systemPromptEdit->toPlainText()));
        m_settings.setValue("temperature", tempSpin->value());
        m_settings.setValue("maxTokens", maxTokensSpin->value());

        m_apiClient->setSystemPrompt(systemPromptEdit->toPlainText());
        m_apiClient->setTemperature(tempSpin->value());
        m_apiClient->setMaxTokens(maxTokensSpin->value());
    }
}

void MainWindow::restyleCards()
{
    // Message cards are created per message and own their own styles, so they
    // are not part of the controller's registry. The streaming card lives in
    // m_chatLayout too, so it is covered by the loop above.
    for (int i = 0; i < m_chatLayout->count(); ++i) {
        QLayoutItem *item = m_chatLayout->itemAt(i);
        if (auto *card = qobject_cast<ChatMessageCard *>(item->widget())) {
            card->applyTheme();
        }
    }
}

void MainWindow::onModelsFetched(const QStringList &models)
{
    m_allModels = models;
    m_modelCombo->setEnabled(true);
    applyModelFilter();
    restoreModelSelection();
}

void MainWindow::onModelChanged(const QString &model)
{
    // Never persist transient combo states (cleared, placeholder text).
    if (model.isEmpty() || model == "Loading models...") {
        return;
    }
    m_apiClient->setModel(model);
    m_settings.setValue("model", model);
    m_settings.setValue("lastModel", model);
    if (m_statusModel) {
        m_statusModel->setText(model);
    }
    updateHeaderState();
}

void MainWindow::onNewChat()
{
    saveDraft();
    saveCurrentChatScrollPosition();
    createNewChat();
    m_inputField->setFocus();
}

void MainWindow::deleteChatAtRow(int row)
{
    if (row < 0 || row >= m_chatSessions.size()) return;

    if (QMessageBox::question(this, "Delete Chat",
            QString("Delete \"%1\" and its messages? This cannot be undone.").arg(m_chatSessions[row].title),
            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // The chat an in-flight request belongs to cannot disappear: cancel first,
    // otherwise the answer would be written into an unrelated chat.
    if (m_requestInFlight && m_requestChatIndex == row) {
        m_apiClient->cancelCurrentRequest();
        m_requestChatIndex = -1;
    } else if (m_requestChatIndex > row) {
        m_requestChatIndex--;
    }

    const QString removedId = m_chatSessions[row].id;
    m_settings.remove("chatMessages/" + removedId);
    m_settings.remove("draft_" + removedId);

    if (m_chatSessions.size() == 1) {
        m_chatSessions.clear();
        m_currentChatIndex = -1;
        clearChatDisplay();
        createNewChat();
    } else {
        // The scroll position has to be flushed while m_currentChatIndex still
        // refers to the chat being removed, otherwise the chat that slides
        // into this index inherits its position.
        if (m_currentChatIndex == row) {
            saveCurrentChatScrollPosition();
        }
        m_chatSessions.removeAt(row);
        if (m_currentChatIndex == row) {
            if (m_currentChatIndex >= m_chatSessions.size()) {
                m_currentChatIndex = m_chatSessions.size() - 1;
            }
            // The index now refers to a different chat, so the "same chat"
            // early return must not skip the rebuild.
            switchToChat(m_currentChatIndex, true);
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
        if (row == m_currentChatIndex) {
            updateHeaderState();
        }
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
    m_settings.setValue(key, SecretStore::protect(draft));
}

void MainWindow::loadDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    QString key = QString("draft_%1").arg(m_chatSessions[m_currentChatIndex].id);
    QString draft = SecretStore::unprotect(m_settings.value(key).toString());
    m_inputField->blockSignals(true);
    m_inputField->setPlainText(draft);
    m_inputField->blockSignals(false);
    updateCharCounter();
}

void MainWindow::clearDraft()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return;

    QString key = QString("draft_%1").arg(m_chatSessions[m_currentChatIndex].id);
    m_settings.remove(key);
    m_inputField->blockSignals(true);
    m_inputField->clear();
    m_inputField->blockSignals(false);
    updateCharCounter();
}

void MainWindow::updateCharCounter()
{
    if (!m_charCounter) return;
    const int len = m_inputField->toPlainText().length();
    const bool overLimit = len > kMaxInputChars;

    m_charCounter->setText(len == 0 ? "Start typing"
                                    : QString("%1 / %2").arg(len).arg(kMaxInputChars));

    // Registering the builder means a theme switch re-applies this, so the
    // length colour has to be produced inside it rather than frozen here.
    // updateCharCounter() runs on every keystroke, so the previous
    // registration is replaced rather than left to accumulate connections.
    m_charCounterStyleLength = len;
    m_charCounterOverLimit = overLimit;
    if (ThemeController *theme = themeController()) {
        theme->registerStyle(m_charCounter, [this]() {
            const Theme &t = currentTheme();
            return QString("QLabel { color: %1; font-size: 11px; font-weight: %2; }")
                .arg(m_charCounterOverLimit ? t.danger : t.textMuted)
                .arg(m_charCounterStyleLength > 0 ? "600" : "400");
        });
    }

    const int docHeight = qCeil(m_inputField->document()->size().height()) + 12;
    const int clampedHeight = qBound(44, docHeight, 150);
    m_inputField->setFixedHeight(clampedHeight);
}

bool MainWindow::handleQuickCommand(const QString &command)
{
    if (command == "/clear") {
        if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return true;
        m_apiClient->cancelCurrentRequest();
        m_chatSessions[m_currentChatIndex].messages.clear();
        m_chatSessions[m_currentChatIndex].messageCount = 0;
        m_currentChatImage.clear();
        if (m_imagePreview) {
            m_imagePreview->clear();
            m_imagePreview->setVisible(false);
        }
        clearChatDisplay();
        saveChatSessions();
        showWelcomeScreen();
        return true;
    }

    if (command == "/stats") {
        if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) return true;
        const auto &chat = m_chatSessions[m_currentChatIndex];
        int totalTokens = 0;
        int msgCount = chat.messages.size();
        for (const auto &msg : chat.messages) {
            totalTokens += msg.totalTokens;
        }
        QMessageBox::information(this, "Chat Statistics",
            QString("Chat: %1\nMessages: %2\nTotal Tokens: %3").arg(chat.title).arg(msgCount).arg(totalTokens));
        return true;
    }

    if (command.startsWith("/model ")) {
        QString modelName = command.mid(7).trimmed();
        if (modelName.isEmpty()) {
            QMessageBox::warning(this, "Quick Commands", "Usage: /model <name>");
            return true;
        }
        m_modelCombo->setCurrentText(modelName);
        onModelChanged(modelName);
        return true;
    }

    if (command == "/help") {
        QMessageBox::information(this, "Quick Commands",
            "Available commands:\n"
            "/clear - Clear current chat\n"
            "/stats - Show chat statistics\n"
            "/model <name> - Change model\n"
            "/search - Open chat search");
        return true;
    }

    if (command == "/search") {
        showSearchBar();
        return true;
    }

    // Unknown command: the caller puts the text back so a typo is not
    // silently swallowed.
    QTextCursor cursor = m_inputField->textCursor();
    cursor.setPosition(0);
    m_inputField->setTextCursor(cursor);
    QMessageBox::information(this, "Quick Commands",
        QString("Unknown command: %1\nUse /help to list the available commands.").arg(command));
    return false;
}

void MainWindow::loadProfiles()
{
    m_profiles.clear();
    m_profileCombo->clear();

    // Only the API keys inside are protected individually, so the profile
    // document itself stays plain and a corrupted key cannot hide the rest.
    QString data = m_settings.value("apiProfiles").toString();
    if (!data.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if (doc.isArray()) {
            for (const auto &val : doc.array()) {
                QJsonObject obj = val.toObject();
                ApiProfile profile;
                profile.name = obj["name"].toString();
                profile.apiKey = SecretStore::unprotect(obj["apiKey"].toString());
                profile.model = obj["model"].toString();
                // A system prompt can carry confidential context, so it is
                // protected too; an empty one is stored unprotected.
                profile.systemPrompt = obj["systemPrompt"].toString();
                if (SecretStore::isEncrypted(obj["systemPrompt"].toString())) {
                    profile.systemPrompt = SecretStore::unprotect(obj["systemPrompt"].toString());
                }
                profile.temperature = obj["temperature"].toDouble();
                profile.maxTokens = obj["maxTokens"].toInt();
                m_profiles.append(profile);
            }
        }
    }

    if (m_profiles.isEmpty()) {
        ApiProfile defaultProfile;
        defaultProfile.name = "Default";
        defaultProfile.apiKey = SecretStore::unprotect(m_settings.value("apiKey").toString());
        defaultProfile.model = m_settings.value("model", "venice-uncensored").toString();
        defaultProfile.systemPrompt = SecretStore::unprotect(m_settings.value("systemPrompt").toString());
        defaultProfile.temperature = m_settings.value("temperature", 0.7).toDouble();
        defaultProfile.maxTokens = m_settings.value("maxTokens", 0).toInt();
        m_profiles.append(defaultProfile);
    }

    for (const auto &profile : m_profiles) {
        m_profileCombo->addItem(profile.name);
    }

    m_currentProfileName = m_settings.value("currentProfile", m_profiles[0].name).toString();
    int idx = m_profileCombo->findText(m_currentProfileName);
    if (idx >= 0) {
        m_profileCombo->setCurrentIndex(idx);
    }
    applyProfile(m_profiles[idx >= 0 ? idx : 0]);
}

void MainWindow::saveProfiles()
{
    QJsonArray arr;
    for (const auto &profile : m_profiles) {
        QJsonObject obj;
        obj["name"] = profile.name;
        obj["apiKey"] = SecretStore::protect(profile.apiKey);
        obj["model"] = profile.model;
        obj["systemPrompt"] = SecretStore::protect(profile.systemPrompt);
        obj["temperature"] = profile.temperature;
        obj["maxTokens"] = profile.maxTokens;
        arr.append(obj);
    }
    m_settings.setValue("apiProfiles", QString::fromUtf8(QJsonDocument(arr).toJson(QJsonDocument::Compact)));
    m_settings.setValue("currentProfile", m_currentProfileName);
}

void MainWindow::applyProfile(const ApiProfile &profile)
{
    m_apiClient->setApiKey(profile.apiKey);
    m_apiClient->setModel(profile.model);
    m_apiClient->setSystemPrompt(profile.systemPrompt);
    m_apiClient->setTemperature(profile.temperature);
    m_apiClient->setMaxTokens(profile.maxTokens);

    QSignalBlocker blocker(m_modelCombo);
    m_modelCombo->setCurrentText(profile.model);
    if (m_statusModel) {
        m_statusModel->setText(profile.model);
    }
    updateHeaderState();
}

void MainWindow::updateHeaderState()
{
    if (!m_headerTitle || !m_headerSubtitle) {
        return;
    }

    QString title = "New Chat";
    int messageCount = 0;
    if (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
        const ChatSession &chat = m_chatSessions[m_currentChatIndex];
        title = chat.title.isEmpty() ? "New Chat" : chat.title;
        messageCount = chat.messageCount;
    }

    QString profile = m_profileCombo ? m_profileCombo->currentText() : QString();
    QString model = m_modelCombo ? m_modelCombo->currentText() : QString();

    QStringList details;
    details << (messageCount == 0 ? "Ready to start" : QString("%1 messages").arg(messageCount));
    if (!profile.isEmpty()) {
        details << profile;
    }
    if (!model.isEmpty()) {
        details << model;
    }
    if (m_webSearchToggle && m_webSearchToggle->isChecked()) {
        details << "Web search";
    }

    m_headerTitle->setText(title);
    m_headerSubtitle->setText(details.join("  •  "));
}

void MainWindow::restoreModelSelection()
{
    QString model = m_settings.value("lastModel", m_settings.value("model", "venice-uncensored")).toString();
    if (model.isEmpty()) {
        model = "venice-uncensored";
    }

    QSignalBlocker blocker(m_modelCombo);
    int index = m_modelCombo->findText(model);
    if (index >= 0) {
        m_modelCombo->setCurrentIndex(index);
    } else {
        m_modelCombo->setEditText(model);
    }

    m_apiClient->setModel(model);
    if (m_statusModel) {
        m_statusModel->setText(model);
    }
}

void MainWindow::onProfileChanged()
{
    QString name = m_profileCombo->currentText();
    for (const auto &profile : m_profiles) {
        if (profile.name == name) {
            m_currentProfileName = name;
            applyProfile(profile);
            m_settings.setValue("model", profile.model);
            m_settings.setValue("lastModel", profile.model);
            saveProfiles();
            updateHeaderState();
            break;
        }
    }
}

void MainWindow::onManageProfiles()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Manage API Profiles");
    dialog.setMinimumWidth(500);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QListWidget *profileList = new QListWidget(&dialog);
    for (const auto &profile : m_profiles) {
        profileList->addItem(profile.name);
    }
    layout->addWidget(profileList);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *addBtn = new QPushButton("Add", &dialog);
    addBtn->setIcon(makeLineIcon(AppIconGlyph::NewChat));
    addBtn->setIconSize(QSize(14, 14));
    QPushButton *editBtn = new QPushButton("Edit", &dialog);
    editBtn->setIcon(makeLineIcon(AppIconGlyph::Edit));
    editBtn->setIconSize(QSize(14, 14));
    QPushButton *deleteBtn = new QPushButton("Delete", &dialog);
    deleteBtn->setIcon(makeLineIcon(AppIconGlyph::Trash));
    deleteBtn->setIconSize(QSize(14, 14));
    btnLayout->addWidget(addBtn);
    btnLayout->addWidget(editBtn);
    btnLayout->addWidget(deleteBtn);
    layout->addLayout(btnLayout);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    auto openProfileDialog = [&](ApiProfile *profile, bool isNew) -> bool {
        QDialog dlg(&dialog);
        dlg.setWindowTitle(isNew ? "Add Profile" : "Edit Profile");
        dlg.setMinimumWidth(400);

        QVBoxLayout *dlgLayout = new QVBoxLayout(&dlg);
        QFormLayout *form = new QFormLayout();

        QLineEdit *nameEdit = new QLineEdit(&dlg);
        nameEdit->setText(profile->name);
        form->addRow("Name:", nameEdit);

        QLineEdit *keyEdit = new QLineEdit(&dlg);
        keyEdit->setText(profile->apiKey);
        keyEdit->setEchoMode(QLineEdit::Password);
        form->addRow("API Key:", keyEdit);

        QLineEdit *modelEdit = new QLineEdit(&dlg);
        modelEdit->setText(profile->model);
        form->addRow("Model:", modelEdit);

        QTextEdit *promptEdit = new QTextEdit(&dlg);
        promptEdit->setPlainText(profile->systemPrompt);
        promptEdit->setMaximumHeight(80);
        form->addRow("System Prompt:", promptEdit);

        QDoubleSpinBox *tempSpin = new QDoubleSpinBox(&dlg);
        tempSpin->setRange(0.0, 2.0);
        tempSpin->setSingleStep(0.1);
        tempSpin->setValue(profile->temperature);
        form->addRow("Temperature:", tempSpin);

        QSpinBox *maxSpin = new QSpinBox(&dlg);
        maxSpin->setRange(0, 32000);
        maxSpin->setSingleStep(256);
        maxSpin->setValue(profile->maxTokens);
        maxSpin->setSpecialValueText("Unlimited");
        form->addRow("Max Tokens:", maxSpin);

        dlgLayout->addLayout(form);

        QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dlg);
        connect(bb, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
        connect(bb, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
        dlgLayout->addWidget(bb);

        if (dlg.exec() == QDialog::Accepted) {
            profile->name = nameEdit->text().trimmed();
            profile->apiKey = keyEdit->text();
            profile->model = modelEdit->text().trimmed();
            profile->systemPrompt = promptEdit->toPlainText();
            profile->temperature = tempSpin->value();
            profile->maxTokens = maxSpin->value();
            return true;
        }
        return false;
    };

    connect(addBtn, &QPushButton::clicked, [&]() {
        ApiProfile newProfile;
        newProfile.name = "New Profile";
        newProfile.model = "venice-uncensored";
        newProfile.temperature = 0.7;
        newProfile.maxTokens = 0;
        if (openProfileDialog(&newProfile, true)) {
            if (newProfile.name.isEmpty()) newProfile.name = "Unnamed";
            m_profiles.append(newProfile);
            profileList->addItem(newProfile.name);
            m_profileCombo->addItem(newProfile.name);
            saveProfiles();
        }
    });

    connect(editBtn, &QPushButton::clicked, [&]() {
        int row = profileList->currentRow();
        if (row < 0 || row >= m_profiles.size()) return;
        ApiProfile profile = m_profiles[row];
        if (openProfileDialog(&profile, false)) {
            m_profiles[row] = profile;
            profileList->item(row)->setText(profile.name);
            m_profileCombo->setItemText(row, profile.name);
            if (m_currentProfileName == m_profiles[row].name || row == m_profileCombo->currentIndex()) {
                applyProfile(profile);
                m_settings.setValue("model", profile.model);
            }
            saveProfiles();
        }
    });

    connect(deleteBtn, &QPushButton::clicked, [&]() {
        int row = profileList->currentRow();
        if (row < 0 || row >= m_profiles.size()) return;
        if (m_profiles.size() <= 1) {
            QMessageBox::warning(&dialog, "Cannot Delete", "At least one profile must exist.");
            return;
        }
        QString name = m_profiles[row].name;
        if (QMessageBox::question(&dialog, "Delete Profile", QString("Delete profile '%1'?").arg(name)) == QMessageBox::Yes) {
            m_profiles.removeAt(row);
            delete profileList->takeItem(row);
            m_profileCombo->removeItem(row);
            if (m_currentProfileName == name) {
                m_currentProfileName = m_profiles[0].name;
                m_profileCombo->setCurrentIndex(0);
                applyProfile(m_profiles[0]);
                m_settings.setValue("model", m_profiles[0].model);
            }
            saveProfiles();
        }
    });

    dialog.exec();
}

void MainWindow::updateContextUsage()
{
    if (m_currentChatIndex < 0 || m_currentChatIndex >= m_chatSessions.size()) {
        if (m_statusContextUsage) m_statusContextUsage->clear();
        return;
    }

    int totalTokens = 0;
    for (const auto &msg : m_chatSessions[m_currentChatIndex].messages) {
        totalTokens += msg.totalTokens;
    }

    QString currentModel = m_modelCombo->currentText();
    int maxContext = 4096;
    if (currentModel.contains("uncensored", Qt::CaseInsensitive)) maxContext = 4096;
    else if (currentModel.contains("large", Qt::CaseInsensitive)) maxContext = 8192;
    else if (currentModel.contains("medium", Qt::CaseInsensitive)) maxContext = 4096;

    // Without usage data (streaming never reports it) an empty label is more
    // honest than "0/4096".
    if (totalTokens <= 0) {
        if (m_statusContextUsage) m_statusContextUsage->clear();
        return;
    }

    double usage = (double)totalTokens / maxContext * 100.0;
    QString color = usage < 50 ? "#4ade80" : (usage < 80 ? "#fbbf24" : "#f87171");

    if (m_statusContextUsage) {
        m_statusContextUsage->setText(QString("Context: %1/%2 (%3%)").arg(totalTokens).arg(maxContext).arg(QString::number(usage, 'f', 1)));
        m_statusContextUsage->setStyleSheet(QString("QLabel { color: %1; font-size: 12px; }").arg(color));
    }
}

void MainWindow::showSearchBar()
{
    if (m_searchBar) {
        m_searchBar->setVisible(true);
        m_searchBar->m_searchInput->setFocus();
    }
}

void MainWindow::hideSearchBar()
{
    if (m_searchBar) {
        m_searchBar->setVisible(false);
        clearHighlights();
    }
}

void MainWindow::onSearchTextChanged(const QString &text)
{
    // One pass over the visible cards: each card re-renders at most once, and
    // clearHighlight() is a no-op for cards that carry no highlight.
    m_highlightedCards.clear();
    for (int i = 0; i < m_chatLayout->count(); ++i) {
        QLayoutItem *item = m_chatLayout->itemAt(i);
        if (auto *card = qobject_cast<ChatMessageCard*>(item->widget())) {
            if (!text.isEmpty() && card->content().contains(text, Qt::CaseInsensitive)) {
                card->highlightText(text);
                m_highlightedCards.append(card);
            } else {
                card->clearHighlight();
            }
        }
    }

    m_currentHighlightIndex = m_highlightedCards.isEmpty() ? -1 : 0;
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(
            m_highlightedCards.isEmpty() ? "0/0" :
            QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size())
        );
    }
}

void MainWindow::onFindNext()
{
    if (m_highlightedCards.isEmpty()) return;
    m_currentHighlightIndex = (m_currentHighlightIndex + 1) % m_highlightedCards.size();
    m_scrollArea->ensureWidgetVisible(m_highlightedCards[m_currentHighlightIndex], 0, 120);
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size()));
    }
}

void MainWindow::onFindPrevious()
{
    if (m_highlightedCards.isEmpty()) return;
    m_currentHighlightIndex = (m_currentHighlightIndex - 1 + m_highlightedCards.size()) % m_highlightedCards.size();
    m_scrollArea->ensureWidgetVisible(m_highlightedCards[m_currentHighlightIndex], 0, 120);
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText(QString("%1/%2").arg(m_currentHighlightIndex + 1).arg(m_highlightedCards.size()));
    }
}

void MainWindow::clearHighlights()
{
    for (auto *card : m_highlightedCards) {
        card->clearHighlight();
    }
    m_highlightedCards.clear();
    m_currentHighlightIndex = -1;
    if (m_searchBar) {
        m_searchBar->m_matchCount->setText("0/0");
    }
}

void MainWindow::updateChatDuration()
{
    if (!m_statusDuration) return;

    if (m_chatStartTime.isValid() && m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size()) {
        const auto &chat = m_chatSessions[m_currentChatIndex];
        if (!chat.messages.isEmpty()) {
            qint64 secs = m_chatStartTime.secsTo(QDateTime::currentDateTime());
            int mins = secs / 60;
            int remainingSecs = secs % 60;
            m_statusDuration->setText(QString("Duration: %1m %2s").arg(mins).arg(remainingSecs, 2, 10, QChar('0')));
            return;
        }
    }
    m_statusDuration->clear();
}

void MainWindow::playNotificationSound()
{
    if (!m_soundInitialized) {
        m_notificationSound = new QSoundEffect(this);

        // QSoundEffect is inert without a source, so the completion tone is
        // synthesised into a temp file on first use.
        static const char toneName[] = "simpleaiclient-notify.wav";
        QString cachePath = QStandardPaths::writableLocation(QStandardPaths::TempLocation) + QLatin1String(toneName);
        if (!QFileInfo::exists(cachePath) && !QFileInfo::exists(m_settings.value("notificationTonePath").toString())) {
            QFile toneFile(cachePath);
            if (toneFile.open(QIODevice::WriteOnly)) {
                QByteArray wav;
                const int sampleRate = 22050;
                const int samples = static_cast<int>(sampleRate * 0.18);
                QVector<qint16> pcm;
                pcm.reserve(samples);
                for (int i = 0; i < samples; ++i) {
                    const double t = static_cast<double>(i) / sampleRate;
                    // Simple two-note chime with an exponential decay envelope.
                    const double freq = t < 0.09 ? 880.0 : 1174.7;
                    const double envelope = std::exp(-6.0 * t);
                    const double value = std::sin(2.0 * M_PI * freq * t) * envelope * 0.28;
                    pcm.append(static_cast<qint16>(std::clamp(value, -1.0, 1.0) * 32767.0));
                }

                const quint32 dataSize = static_cast<quint32>(pcm.size() * 2);
                QByteArray header;
                auto append32 = [&header](quint32 v) {
                    for (int b = 0; b < 4; ++b) {
                        header.append(static_cast<char>((v >> (8 * b)) & 0xFF));
                    }
                };
                auto append16 = [&header](quint16 v) {
                    header.append(static_cast<char>(v & 0xFF));
                    header.append(static_cast<char>((v >> 8) & 0xFF));
                };
                header.append("RIFF");
                append32(36 + dataSize);
                header.append("WAVE");
                header.append("fmt ");
                append32(16);
                append16(1);                          // PCM
                append16(1);                          // mono
                append32(static_cast<quint32>(sampleRate));
                append32(static_cast<quint32>(sampleRate * 2));
                append16(2);                          // block align
                append16(16);                         // bits per sample
                header.append("data");
                append32(dataSize);

                wav = header;
                for (qint16 s : pcm) {
                    wav.append(static_cast<char>(s & 0xFF));
                    wav.append(static_cast<char>((s >> 8) & 0xFF));
                }
                toneFile.write(wav);
            }
        }

        const QString source = QFileInfo::exists(m_settings.value("notificationTonePath").toString())
            ? m_settings.value("notificationTonePath").toString()
            : cachePath;
        if (QFileInfo::exists(source)) {
            m_notificationSound->setSource(QUrl::fromLocalFile(source));
        }
        m_soundInitialized = true;
    }
    if (m_notificationSound && m_notificationSound->isLoaded()) {
        m_notificationSound->play();
    }
}

void MainWindow::showShortcutsDialog()
{
    QDialog dialog(this);
    dialog.setWindowTitle("Keyboard Shortcuts");
    dialog.setMinimumWidth(400);

    QVBoxLayout *layout = new QVBoxLayout(&dialog);

    QTableWidget *table = new QTableWidget(&dialog);
    table->setColumnCount(2);
    table->setHorizontalHeaderLabels({"Shortcut", "Action"});
    table->horizontalHeader()->setStretchLastSection(true);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    const Theme &t = currentTheme();
    table->setStyleSheet(
        "QTableWidget { "
        "  background-color: " + t.surfaceAlt + "; "
        "  color: " + t.textStrong + "; "
        "  gridline-color: " + t.border + "; "
        "  border: 1px solid " + t.borderStrong + "; "
        "  selection-background-color: " + t.accent + "; "
        "  selection-color: " + t.accentText + "; "
        "  font-size: 13px; "
        "} "
        "QHeaderView::section { "
        "  background-color: " + t.surfaceSunken + "; "
        "  color: " + t.textStrong + "; "
        "  padding: 4px; "
        "  border: 1px solid " + t.borderStrong + "; "
        "} "
        + scrollBarStyle()
    );

    QList<QPair<QString, QString>> shortcuts = {
        {"Ctrl+N", "New Chat"},
        {"Ctrl+E", "Export Chat"},
        {"Ctrl+,", "Settings"},
        {"Ctrl+Shift+S", "Advanced Settings"},
        {"Ctrl+Q", "Quit"},
        {"Ctrl+F", "Search in Chat"},
        {"Ctrl+T", "Toggle Theme"},
        {"Ctrl+=", "Increase Font Size"},
        {"Ctrl+-", "Decrease Font Size"},
        {"Ctrl+0", "Reset Font Size"},
        {"Ctrl+?", "Show Shortcuts"},
        {"Enter", "Send Message"},
        {"Shift+Enter", "New Line"},
    };

    table->setRowCount(shortcuts.size());
    for (int i = 0; i < shortcuts.size(); ++i) {
        table->setItem(i, 0, new QTableWidgetItem(shortcuts[i].first));
        table->setItem(i, 1, new QTableWidgetItem(shortcuts[i].second));
    }

    layout->addWidget(table);

    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Close, &dialog);
    connect(buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    layout->addWidget(buttonBox);

    dialog.exec();
}

void MainWindow::updateStreamingSpeed(const QString &chunk)
{
    if (m_streamStartTime == 0 || !m_statusSpeed) return;

    qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_streamStartTime;
    if (elapsed > 0) {
        double speed = (double)m_streamTokenCount / (elapsed / 1000.0);
        m_statusSpeed->setText(QString("%1 tok/s").arg(speed, 0, 'f', 1));
    }
}

void MainWindow::setRequestInFlight(bool inFlight)
{
    m_requestInFlight = inFlight;
    m_sendButton->setEnabled(true);
    m_sendButton->setIcon(makeLineIcon(inFlight ? AppIconGlyph::Stop : AppIconGlyph::Send));
    m_sendButton->setToolTip(inFlight ? "Stop generation" : "Send message");
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Draft, scroll position and the current turn were only persisted on chat
    // switches, so a quit lost the last input.
    saveDraft();
    saveCurrentChatScrollPosition();
    if (m_requestInFlight) {
        m_apiClient->cancelCurrentRequest();
    }
    saveChatSessions();
    if (m_backupTimer && m_backupTimer->isActive()) {
        m_backupTimer->stop();
        saveChatBackup();
    }
    saveSettings();
    QMainWindow::closeEvent(event);
}

void MainWindow::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        for (const QUrl &url : event->mimeData()->urls()) {
            QString suffix = url.toLocalFile().toLower();
            if (suffix.endsWith(".png") || suffix.endsWith(".jpg") || suffix.endsWith(".jpeg") ||
                suffix.endsWith(".gif") || suffix.endsWith(".bmp") || suffix.endsWith(".webp")) {
                event->acceptProposedAction();
                return;
            }
        }
    }
}

void MainWindow::dragMoveEvent(QDragMoveEvent *event)
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *event)
{
    if (attachImageFromPath(event->mimeData()->urls().first().toLocalFile())) {
        event->acceptProposedAction();
    }
}

bool MainWindow::attachImageFromPath(const QString &filePath)
{
    if (filePath.isEmpty()) {
        return false;
    }

    // base64 inflates by ~4/3 and the result is embedded in the request, the
    // QSettings value and every backup snapshot, so oversized files are refused
    // rather than silently degrading all three.
    constexpr qint64 kMaxImageBytes = 4 * 1024 * 1024;
    const qint64 size = QFileInfo(filePath).size();
    if (size > kMaxImageBytes) {
        QMessageBox::warning(this, "Image Too Large",
            QString("%1 is %2 MB. Images are limited to %3 MB.")
                .arg(QFileInfo(filePath).fileName())
                .arg(size / (1024 * 1024))
                .arg(kMaxImageBytes / (1024 * 1024)));
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this, "Error", "Could not open image file.");
        return false;
    }

    // The MIME type has to match the payload: the dialog and the drop filter
    // accept png/gif/bmp/webp too, and a mislabelled data URL is rejected by
    // the vision endpoint.
    const QMimeDatabase mimeDb;
    const QString mimeType = mimeDb.mimeTypeForFile(filePath).name();
    if (!mimeType.startsWith("image/")) {
        QMessageBox::warning(this, "Unsupported File", "That file is not an image.");
        return false;
    }

    m_currentChatImage = QString("data:%1;base64,%2")
                             .arg(mimeType)
                             .arg(QString::fromLatin1(file.readAll().toBase64()));

    QPixmap pixmap;
    pixmap.load(filePath);
    if (!pixmap.isNull() && m_imagePreview) {
        m_imagePreview->setPixmap(pixmap.scaled(200, 60, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        m_imagePreview->setVisible(true);
    }
    return true;
}

void MainWindow::onToggleAutoScroll()
{
    m_autoScroll = m_autoScrollToggle->isChecked();
    m_settings.setValue("autoScroll", m_autoScroll);
    if (m_autoScroll) scrollToBottom();
    else m_scrollFollowTimer->stop();
}

void MainWindow::adjustFontSize(int delta)
{
    m_chatFontSize = qBound(10, m_chatFontSize + delta, 30);
    applyChatFontSize();
}

void MainWindow::resetFontSize()
{
    m_chatFontSize = 15;
    applyChatFontSize();
}

void MainWindow::applyChatFontSize()
{
    for (int i = 0; i < m_chatLayout->count(); ++i) {
        QLayoutItem *item = m_chatLayout->itemAt(i);
        if (auto *card = qobject_cast<ChatMessageCard*>(item->widget())) {
            card->setContentFontSize(m_chatFontSize);
        }
    }
}

void MainWindow::onApiErrorWithRetry(const QString &error)
{
    m_retryCount++;
    if (m_retryCount <= m_maxRetries && !m_pendingMessages.isEmpty()) {
        int delayMs = (1 << (m_retryCount - 1)) * 1000;
        if (m_statusConnection) {
            m_statusConnection->setText(QString("Retrying (%1/%2)...").arg(m_retryCount).arg(m_maxRetries));
        }

        // Drop the failed attempt's partial card so the retry starts a fresh
        // one instead of appending to it.
        m_streamRenderTimer->stop();
        m_pendingStreamChunk.clear();
        m_streamedContent.clear();
        if (m_streamingCard) {
            m_chatLayout->removeWidget(m_streamingCard);
            delete m_streamingCard;
            m_streamingCard = nullptr;
        }
        if (m_requestChatIndex == m_currentChatIndex) {
            showThinkingIndicator();
        }

        const QList<ChatMessage> retryMessages = m_pendingMessages;
        m_retryTimer->singleShot(delayMs, this, [this, retryMessages]() {
            if (!m_requestInFlight || retryMessages.isEmpty()) {
                return;
            }
            m_apiClient->sendMessage(retryMessages);
        });
    } else {
        m_retryCount = 0;
        onErrorOccurred(error);
    }
}
