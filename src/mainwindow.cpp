#include "mainwindow.h"
#include "appicons.h"
#include "chatstore.h"
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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_inputField(new QTextEdit)
    , m_sendButton(new QToolButton)
    , m_modelCombo(new QComboBox)
    , m_newChatButton(new QPushButton("+ New Chat"))
    , m_apiClient(new ApiClient(this))
    , m_currentChatIndex(-1)
    , m_settings("lagmajin", "SimpleAIClient")
    , m_currentChatImage("")
    , m_imagePreview(nullptr)
    , m_headerTitle(nullptr)
    , m_headerSubtitle(nullptr)
    , m_charCounter(nullptr)
    , m_searchBar(nullptr)
    , m_currentProfileName("")
    , m_chatListRenderGeneration(0)
    , m_chatListRenderCursor(0)
    , m_retryCount(0)
    , m_maxRetries(3)
    , m_retryTimer(new QTimer(this))
    , m_store(new ChatStore(&m_settings, &m_chatSessions))
    , m_notificationSound(nullptr)
    , m_soundInitialized(false)
    , m_chatStartTime()
    , m_statusDuration(nullptr)
    , m_statusSpeed(nullptr)
    , m_streamStartTime(0)
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
    // After setupUI, which creates the scroll area, container and welcome
    // widget the view takes over.
    initChatView();
    setupMenu();
    loadProfiles();
    loadSettings();
    loadChatSessions();
    pointViewAtCurrentChat();

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

void MainWindow::onAttachImage()
{
    const QString filePath = QFileDialog::getOpenFileName(
        this, "Select Image", "",
        "Images (*.png *.jpg *.jpeg *.gif *.bmp *.webp)");
    attachImageFromPath(filePath);
}

void MainWindow::onToggleTheme()
{
    m_theme->setDark(!m_theme->isDark());
    m_isDarkTheme = m_theme->isDark();
    m_settings.setValue("darkTheme", m_isDarkTheme);
    restyleCards();
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


void MainWindow::updateStreamingSpeed(int renderedWords)
{
    m_streamTokenCount += qMax(1, renderedWords);

    if (m_streamStartTime == 0 || !m_statusSpeed) return;

    const qint64 elapsed = QDateTime::currentMSecsSinceEpoch() - m_streamStartTime;
    if (elapsed > 0) {
        const double speed = static_cast<double>(m_streamTokenCount) / (elapsed / 1000.0);
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
    // The backup is debounced, so a quit has to force the pending write or the
    // last turn is only in the registry.
    m_store->flushBackup();
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
    const bool enabled = m_autoScrollToggle->isChecked();
    m_settings.setValue("autoScroll", enabled);
    if (m_view) {
        m_view->setAutoScroll(enabled);
    }
}

void MainWindow::adjustFontSize(int delta)
{
    applyChatFontSize();
}

void MainWindow::resetFontSize()
{
    applyChatFontSize();
}

void MainWindow::applyChatFontSize()
{
    if (m_view) {
    }
}
