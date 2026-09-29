// Building the window.
//
// The layout lives here rather than in mainwindow.cpp because it is a single
// 500-line straight-line construction: it reads top to bottom, and splitting it
// by widget would only make that harder to follow. Everything it creates is
// stored as a member; nothing here has behaviour.

#include "mainwindow.h"
#include "appicons.h"
#include "chatwidgets.h"

#include <QAction>
#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollArea>
#include <QSlider>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QTextEdit>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>
#include <QScrollBar>

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
