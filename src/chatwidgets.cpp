#include "chatwidgets.h"

#include <QApplication>
#include <QClipboard>
#include <QGraphicsDropShadowEffect>
#include <QMenu>
#include <QPainter>
#include <QRegularExpression>
#include <QShortcut>
#include <QTextCursor>
#include <QTimer>
#include <QtMath>

AvatarLabel::AvatarLabel(const QString &role, QWidget *parent)
    : QLabel(parent)
    , m_role(role)
{
    setFixedSize(32, 32);
    setAlignment(Qt::AlignCenter);
    applyTheme();
}

void AvatarLabel::applyTheme()
{
    const Theme &t = currentTheme();

    QPixmap pixmap(32, 32);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    QColor brush = QColor(t.selection);
    QString glyph = "AI";
    int glyphSize = 14;
    bool bold = true;

    if (m_role == "user") {
        brush = QColor(t.accent);
        glyph = "U";
    } else if (m_role == "error") {
        brush = QColor(t.danger);
        glyph = "!";
        glyphSize = 16;
        bold = false;
    }

    painter.setBrush(brush);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(2, 2, 28, 28);

    painter.setPen(QColor(t.accentText));
    QFont font = painter.font();
    font.setPointSize(glyphSize);
    font.setBold(bold);
    painter.setFont(font);
    painter.drawText(pixmap.rect(), Qt::AlignCenter, glyph);

    setPixmap(pixmap);
}

ChatListItem::ChatListItem(const QString &title, const QString &subtitle, int index, bool isPinned, QWidget *parent)
    : QWidget(parent), m_isActive(false), m_index(index), m_isPinned(isPinned)
{
    setStyleSheet(
        "QWidget { "
        "  background-color: transparent; "
        "  border: 1px solid transparent; "
        "  border-radius: 10px; "
        "}"
    );

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(10, 8, 6, 8);
    layout->setSpacing(8);

    m_iconLabel = new QLabel(this);
    m_iconLabel->setFixedSize(16, 16);
    m_iconLabel->setPixmap(makeLineIcon(isPinned ? AppIconGlyph::Pin : AppIconGlyph::Chat).pixmap(16, 16));
    layout->addWidget(m_iconLabel, 0, Qt::AlignVCenter);

    QWidget *textContainer = new QWidget(this);
    QVBoxLayout *textLayout = new QVBoxLayout(textContainer);
    textLayout->setContentsMargins(0, 0, 0, 0);
    textLayout->setSpacing(2);

    m_titleLabel = new QLabel(title, textContainer);
    m_titleLabel->setStyleSheet("QLabel { color: " + currentTheme().textStrong + "; font-size: 13px; font-weight: 600; }");
    m_titleLabel->setWordWrap(false);
    textLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(subtitle, textContainer);
    m_subtitleLabel->setStyleSheet("QLabel { color: " + currentTheme().textMuted + "; font-size: 11px; }");
    m_subtitleLabel->setWordWrap(false);
    textLayout->addWidget(m_subtitleLabel);

    layout->addWidget(textContainer, 1);

    m_deleteBtn = new QToolButton(this);
    m_deleteBtn->setIcon(makeLineIcon(AppIconGlyph::Trash));
    m_deleteBtn->setIconSize(QSize(14, 14));
    m_deleteBtn->setFixedSize(20, 20);
    m_deleteBtn->setStyleSheet(
        "QToolButton { "
        "  background-color: transparent; "
        "  border: none; "
        "  padding: 2px; "
        "} "
        "QToolButton:hover { background-color: " + currentTheme().surfaceAlt + "; border-radius: 4px; }"
    );
    m_deleteBtn->setVisible(false);
    layout->addWidget(m_deleteBtn);

    if (ThemeController *theme = themeController()) {
        theme->registerStyle(this, [this]() {
            return "QWidget { "
                   "  background-color: transparent; "
                   "  border: 1px solid transparent; "
                   "  border-radius: 10px; "
                   "}";
        });
        theme->registerStyle(m_titleLabel, [this]() {
            return "QLabel { color: " + currentTheme().textStrong + "; font-size: 13px; font-weight: 600; }";
        });
        theme->registerStyle(m_subtitleLabel, [this]() {
            return m_isActive
                ? "QLabel { color: " + currentTheme().accentText + "; font-size: 11px; font-weight: 600; }"
                : "QLabel { color: " + currentTheme().textMuted + "; font-size: 11px; }";
        });
        theme->registerStyle(m_deleteBtn, [this]() {
            return "QToolButton { "
                   "  background-color: transparent; "
                   "  border: none; "
                   "  padding: 2px; "
                   "} "
                   "QToolButton:hover { background-color: " + currentTheme().surfaceAlt + "; border-radius: 4px; }";
        });
    }

    connect(m_deleteBtn, &QToolButton::clicked, [this]() {
        emit deleteClicked(m_index);
    });

    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, [this](const QPoint &pos) {
        QMenu menu(this);
        QAction *renameAction = menu.addAction(makeLineIcon(AppIconGlyph::Edit), "Rename");
        QAction *pinAction = menu.addAction(makeLineIcon(m_isPinned ? AppIconGlyph::Unpin : AppIconGlyph::Pin), m_isPinned ? "Unpin" : "Pin");
        QAction *chosen = menu.exec(mapToGlobal(pos));
        if (chosen == renameAction) {
            emit renameRequested(m_index);
        } else if (chosen == pinAction) {
            emit pinRequested(m_index);
        }
    });
}

void ChatListItem::applyStyle()
{
    const Theme &t = currentTheme();

    if (m_isActive) {
        setStyleSheet("QWidget { background-color: " + t.accent + "; border: 1px solid " + t.accentHover + "; border-radius: 10px; }");
    } else if (m_hovered) {
        setStyleSheet("QWidget { background-color: " + t.surfaceAlt + "; border: 1px solid " + t.chipBorder + "; border-radius: 10px; }");
    } else {
        setStyleSheet("QWidget { background-color: transparent; border: 1px solid transparent; border-radius: 10px; }");
    }

    if (m_subtitleLabel) {
        if (m_isActive) {
            m_subtitleLabel->setStyleSheet("QLabel { color: " + t.accentText + "; font-size: 11px; font-weight: 600; }");
        } else if (m_hovered) {
            m_subtitleLabel->setStyleSheet("QLabel { color: " + t.textStrong + "; font-size: 11px; }");
        } else {
            m_subtitleLabel->setStyleSheet("QLabel { color: " + t.textMuted + "; font-size: 11px; }");
        }
    }
}

void ChatListItem::setActive(bool active)
{
    m_isActive = active;
    applyStyle();
}

void ChatListItem::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_deleteBtn->setVisible(true);
    m_hovered = true;
    applyStyle();
}

void ChatListItem::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_deleteBtn->setVisible(false);
    m_hovered = false;
    setActive(m_isActive);
}

void ChatListItem::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        emit clicked(m_index);
    }
    QWidget::mousePressEvent(event);
}

ChatMessageCard::ChatMessageCard(const QString &role, const QString &content, QWidget *parent)
    : QWidget(parent), m_role(role), m_fullContent(content), m_copyBtn(nullptr), m_streamLabel(nullptr), m_tokenLabel(nullptr), m_regenerateBtn(nullptr), m_branchBtn(nullptr), m_editBtn(nullptr), m_editField(nullptr), m_messageIndex(-1), m_isStreaming(false), m_timestampLabel(nullptr)
{
    m_outerLayout = new QVBoxLayout(this);
    m_outerLayout->setContentsMargins(0, 8, 0, 8);
    m_outerLayout->setSpacing(4);

    QHBoxLayout *rowLayout = new QHBoxLayout();
    rowLayout->setContentsMargins(0, 0, 0, 0);
    rowLayout->setSpacing(12);

    m_avatar = new AvatarLabel(role, this);

    m_card = new QFrame(this);
    m_card->setMaximumWidth(760);
    applyCardStyle();

    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
    shadow->setBlurRadius(18);
    shadow->setXOffset(0);
    shadow->setYOffset(6);
    shadow->setColor(QColor(0, 0, 0, 55));
    m_card->setGraphicsEffect(shadow);

    m_layout = new QVBoxLayout(m_card);
    m_layout->setContentsMargins(12, 12, 12, 12);
    m_layout->setSpacing(8);

    if (!content.isEmpty()) {
        renderMarkdown(content);
    }

    if (role == "user") {
        rowLayout->addStretch();
        rowLayout->addWidget(m_card, 0, Qt::AlignRight);
        rowLayout->addWidget(m_avatar, 0, Qt::AlignTop | Qt::AlignRight);
    } else {
        rowLayout->addWidget(m_avatar, 0, Qt::AlignTop | Qt::AlignLeft);
        rowLayout->addWidget(m_card, 0, Qt::AlignLeft);
        rowLayout->addStretch();
    }

    m_copyContainer = new QWidget(m_card);
    m_copyContainer->setVisible(false);
    QHBoxLayout *copyLayout = new QHBoxLayout(m_copyContainer);
    copyLayout->setContentsMargins(0, 0, 0, 0);
    copyLayout->setSpacing(8);

    m_tokenLabel = new QLabel(m_copyContainer);
    m_tokenLabel->setVisible(false);
    m_tokenLabel->setStyleSheet(
        "QLabel { "
        "  color: " + currentTheme().textMuted + "; "
        "  font-size: 11px; "
        "  padding: 0 2px; "
        "}"
    );
    copyLayout->addWidget(m_tokenLabel, 0, Qt::AlignVCenter);
    copyLayout->addStretch();

    m_copyBtn = new QPushButton("Copy", m_copyContainer);
    m_copyBtn->setIcon(makeLineIcon(AppIconGlyph::Copy));
    m_copyBtn->setIconSize(QSize(14, 14));
    m_copyBtn->setMaximumWidth(74);
    copyLayout->addWidget(m_copyBtn);

    connect(m_copyBtn, &QPushButton::clicked, [this]() {
        QClipboard *clipboard = QApplication::clipboard();
        const QString copiedText = m_fullContent;
        clipboard->setText(copiedText);

        // Keep copied drafts from being accidentally pasted into another app.
        // Only clear the clipboard if it still contains the text we placed there;
        // this avoids deleting a newer copy made by the user.
        QTimer::singleShot(30000, [copiedText]() {
            QClipboard *currentClipboard = QApplication::clipboard();
            if (currentClipboard && currentClipboard->text() == copiedText)
                currentClipboard->clear();
        });

        m_copyBtn->setText("Copied!");
        m_copyBtn->setEnabled(false);

        QTimer::singleShot(1000, [this]() {
            m_copyBtn->setText("Copy");
            m_copyBtn->setEnabled(true);
        });
    });

    m_regenerateBtn = new QPushButton("Regenerate", m_card);
    m_regenerateBtn->setIcon(makeLineIcon(AppIconGlyph::Refresh));
    m_regenerateBtn->setIconSize(QSize(14, 14));
    m_regenerateBtn->setMaximumWidth(112);
    m_regenerateBtn->setVisible(false);
    copyLayout->addWidget(m_regenerateBtn);
    connect(m_regenerateBtn, &QPushButton::clicked, this, [this]() {
        emit regenerateRequested(m_messageIndex);
    });

    m_branchBtn = new QPushButton("Branch", m_card);
    m_branchBtn->setIcon(makeLineIcon(AppIconGlyph::Branch));
    m_branchBtn->setIconSize(QSize(14, 14));
    m_branchBtn->setMaximumWidth(76);
    m_branchBtn->setVisible(false);
    copyLayout->addWidget(m_branchBtn);
    connect(m_branchBtn, &QPushButton::clicked, [this]() {
        emit branchRequested(m_messageIndex);
    });

    m_editBtn = new QPushButton("Edit", m_card);
    m_editBtn->setIcon(makeLineIcon(AppIconGlyph::Edit));
    m_editBtn->setIconSize(QSize(14, 14));
    m_editBtn->setMaximumWidth(62);
    m_editBtn->setVisible(false);
    copyLayout->addWidget(m_editBtn);
    connect(m_editBtn, &QPushButton::clicked, [this]() {
        startEditing();
    });

    m_layout->addWidget(m_copyContainer);

    m_outerLayout->addLayout(rowLayout);

    m_timestampLabel = new QLabel(this);
    m_timestampLabel->setVisible(false);
    m_timestampLabel->setAlignment(role == "user" ? Qt::AlignRight : Qt::AlignLeft);
    m_outerLayout->addWidget(m_timestampLabel);

    setMouseTracking(true);
    m_card->setMouseTracking(true);

    // Applies every card-owned style from the current tokens.
    applyTheme();
}

void ChatMessageCard::setTimestamp(const QDateTime &timestamp)
{
    m_timestamp = timestamp;
}

void ChatMessageCard::applyTheme()
{
    const Theme &t = currentTheme();

    applyCardStyle();

    if (m_streamLabel) {
        m_streamLabel->setStyleSheet(streamLabelStyle());
    }
    if (m_editField) {
        m_editField->setStyleSheet(editFieldStyle());
    }

    if (m_tokenLabel) {
        m_tokenLabel->setStyleSheet(
            "QLabel { color: " + t.textMuted + "; font-size: 11px; padding: 0 2px; }");
    }

    const QString secondaryButton =
        "QPushButton { "
        "  background-color: " + t.chipBg + "; "
        "  color: " + t.textStrong + "; "
        "  border: 1px solid " + t.chipBorder + "; "
        "  border-radius: 7px; "
        "  padding: 5px 10px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: " + t.surfaceAlt + "; }";

    if (m_copyBtn) {
        m_copyBtn->setStyleSheet(
            "QPushButton { "
            "  background-color: " + t.chipBg + "; "
            "  color: " + t.textStrong + "; "
            "  border: 1px solid " + t.chipBorder + "; "
            "  border-radius: 7px; "
            "  padding: 5px 10px; "
            "  font-size: 12px; "
            "} "
            "QPushButton:hover { background-color: " + t.surfaceAlt + "; } "
            "QPushButton:disabled { background-color: " + t.surface + "; color: " + t.success + "; }");
    }
    if (m_regenerateBtn) m_regenerateBtn->setStyleSheet(secondaryButton);
    if (m_branchBtn) m_branchBtn->setStyleSheet(secondaryButton);
    if (m_editBtn) m_editBtn->setStyleSheet(secondaryButton);

    if (m_timestampLabel) {
        m_timestampLabel->setStyleSheet(
            "QLabel { color: " + t.textMuted + "; font-size: 11px; padding: 2px 8px; }");
    }

    if (m_avatar) {
        m_avatar->applyTheme();
    }

    // Re-renders the markdown so the block labels pick up the new tokens.
    setContentFontSize(m_contentFontSize);
}

QString ChatMessageCard::streamLabelStyle() const
{
    const Theme &t = currentTheme();
    return QString(
        "QLabel { "
        "  color: " + t.textStrong + "; "
        "  font-size: " + QString::number(m_contentFontSize) + "px; "
        "  line-height: 1.6; "
        "} "
        "QLabel a { color: " + t.link + "; }");
}

QString ChatMessageCard::editFieldStyle() const
{
    const Theme &t = currentTheme();
    return QString(
        "QTextEdit { "
        "  background-color: " + t.inputBg + "; "
        "  color: " + t.textStrong + "; "
        "  border: 1px solid " + t.borderStrong + "; "
        "  border-radius: 6px; "
        "  padding: 8px; "
        "  font-size: " + QString::number(m_contentFontSize) + "px; "
        "} " + scrollBarStyle());
}

void ChatMessageCard::applyCardStyle()
{
    const Theme &t = currentTheme();

    QString bgColor = t.assistantBubble;
    QString borderColor = t.assistantBubbleBorder;
    QString textColor = t.textStrong;
    if (m_role == "user") {
        bgColor = t.userBubble;
        borderColor = t.userBubbleBorder;
        textColor = t.userBubbleText;
    } else if (m_role == "error") {
        bgColor = t.errorBubble;
        borderColor = t.danger;
        textColor = t.textStrong;
    }

    m_card->setStyleSheet(
        "QFrame { "
        "  background-color: " + bgColor + "; "
        "  border: 1px solid " + borderColor + "; "
        "  border-radius: 18px; "
        "  padding: 12px 16px; "
        "  color: " + textColor + "; "
        "}"
    );

    // The shadow is invisible on a light background and too heavy on a dark one.
    if (auto *shadow = qobject_cast<QGraphicsDropShadowEffect *>(m_card->graphicsEffect())) {
        shadow->setColor(QColor(0, 0, 0, t.isDarkTheme ? 55 : 24));
    }
}

void ChatMessageCard::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    if (m_timestamp.isValid() && m_timestampLabel) {
        m_timestampLabel->setText(m_timestamp.toString("HH:mm"));
        m_timestampLabel->setVisible(true);
    }
}

void ChatMessageCard::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    if (m_timestampLabel) {
        m_timestampLabel->setVisible(false);
    }
}

void ChatMessageCard::appendContent(const QString &content)
{
    m_fullContent += content;

    if (m_isStreaming) {
        if (m_streamLabel) {
            // Stream in plain text to avoid reparsing rich text on every chunk.
            m_streamLabel->setText(m_fullContent);
        }
    } else {
        rebuildContent();
    }
}

void ChatMessageCard::setContentFontSize(int pixels)
{
    if (m_contentFontSize == pixels) {
        return;
    }
    m_contentFontSize = pixels;

    if (m_streamLabel) {
        m_streamLabel->setStyleSheet(
            "QLabel { "
            "  color: #ececf1; "
            "  font-size: " + QString::number(m_contentFontSize) + "px; "
            "  line-height: 1.6; "
            "} "
            "QLabel a { color: #60a5fa; }"
        );
    }
    if (m_editField) {
        m_editField->setStyleSheet(
            "QTextEdit { "
            "  background-color: #1a1a1a; "
            "  color: #ececf1; "
            "  border: 1px solid #4d4d4f; "
            "  border-radius: 6px; "
            "  padding: 8px; "
            "  font-size: " + QString::number(m_contentFontSize) + "px; "
            "}"
        );
    }
    if (!m_isStreaming) {
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
            m_streamLabel->setTextFormat(Qt::PlainText);
            m_streamLabel->setStyleSheet(streamLabelStyle());
            m_layout->insertWidget(0, m_streamLabel);
        }
        m_streamLabel->setText(m_fullContent);
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

    const Theme &t = currentTheme();

    m_editField = new QTextEdit(m_card);
    m_editField->setPlainText(m_fullContent);
    m_editField->setMaximumHeight(150);
    m_editField->setStyleSheet(editFieldStyle());

    QHBoxLayout *editBtnLayout = new QHBoxLayout();
    editBtnLayout->setContentsMargins(0, 4, 0, 4);
    editBtnLayout->addStretch();

    QPushButton *saveBtn = new QPushButton("Save", m_card);
    saveBtn->setIcon(makeLineIcon(AppIconGlyph::Save));
    saveBtn->setIconSize(QSize(14, 14));
    saveBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: " + t.accent + "; "
        "  color: " + t.accentText + "; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 12px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: " + t.accentHover + "; }"
    );
    QPushButton *cancelBtn = new QPushButton("Cancel", m_card);
    cancelBtn->setIcon(makeLineIcon(AppIconGlyph::Cancel));
    cancelBtn->setIconSize(QSize(14, 14));
    cancelBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: " + t.surfaceAlt + "; "
        "  color: " + t.textStrong + "; "
        "  border: 1px solid " + t.borderStrong + "; "
        "  border-radius: 4px; "
        "  padding: 4px 12px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: " + t.scrollbarHover + "; }"
    );
    editBtnLayout->addWidget(cancelBtn);
    editBtnLayout->addWidget(saveBtn);
    if (ThemeController *theme = themeController()) {
        theme->registerStyle(saveBtn, []() {
            const Theme &s = currentTheme();
            return QString(
                "QPushButton { background-color: " + s.accent + "; color: " + s.accentText + "; "
                "border: none; border-radius: 4px; padding: 4px 12px; font-size: 11px; } "
                "QPushButton:hover { background-color: " + s.accentHover + "; }");
        });
        theme->registerStyle(cancelBtn, []() {
            const Theme &s = currentTheme();
            return QString(
                "QPushButton { background-color: " + s.surfaceAlt + "; color: " + s.textStrong + "; "
                "border: 1px solid " + s.borderStrong + "; border-radius: 4px; padding: 4px 12px; font-size: 11px; } "
                "QPushButton:hover { background-color: " + s.scrollbarHover + "; }");
        });
    }

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
}

QString ChatMessageCard::renderInlineMarkdown(const QString &text)
{
    // Escape before the markup substitutions: model and user text must never be
    // parsed as rich text (`a < b`, a literal `<b>`, `&nbsp;`, ...).
    QString result = text.toHtmlEscaped();

    result.replace(QRegularExpression("\\*\\*(.+?)\\*\\*"), "<b>\\1</b>");
    result.replace(QRegularExpression("\\*(.+?)\\*"), "<i>\\1</i>");
    result.replace(QRegularExpression("__(.+?)__"), "<b>\\1</b>");
    result.replace(QRegularExpression("_(.+?)_"), "<i>\\1</i>");
    result.replace(QRegularExpression("`(.+?)`"),
                   "<code style='background-color:" + currentTheme().surfaceSunken + "; padding:2px 4px; border-radius:3px; font-family:monospace;'>\\1</code>");
    result.replace(QRegularExpression("\\[([^\\]]+)\\]\\(([^)]+)\\)"), "<a href='\\2' style='color:#60a5fa;'>\\1</a>");

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
            html += "<h3 style='color:" + currentTheme().textStrong + "; font-size:16px; margin:8px 0 4px 0;'>" + renderInlineMarkdown(trimmed.mid(4)) + "</h3>";
        } else if (trimmed.startsWith("## ")) {
            if (inList) { html += "</ul>"; inList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += "<h2 style='color:" + currentTheme().textStrong + "; font-size:18px; margin:10px 0 6px 0;'>" + renderInlineMarkdown(trimmed.mid(3)) + "</h2>";
        } else if (trimmed.startsWith("# ")) {
            if (inList) { html += "</ul>"; inList = false; }
            if (inOrderedList) { html += "</ol>"; inOrderedList = false; }
            html += "<h1 style='color:" + currentTheme().textStrong + "; font-size:20px; margin:12px 0 8px 0;'>" + renderInlineMarkdown(trimmed.mid(2)) + "</h1>";
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
        "  color: " + currentTheme().textStrong + "; "
        "  font-size: " + QString::number(m_contentFontSize) + "px; "
        "  line-height: 1.75; "
        "} "
        "QLabel a { color: " + currentTheme().link + "; }"
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
    const Theme &t = currentTheme();

    codeEdit->setStyleSheet(
        "QTextEdit { "
        "  background-color: " + t.codeBg + "; "
        "  color: " + t.codeText + "; "
        "  selection-background-color: " + t.selection + "; "
        "  selection-color: " + t.accentText + "; "
        "  font-family: 'Cascadia Code', 'Consolas', 'Courier New', monospace; "
        "  font-size: " + QString::number(qMax(10, m_contentFontSize - 1)) + "px; "
        "  border: none; "
        "  border-bottom-left-radius: 8px; "
        "  border-bottom-right-radius: 8px; "
        "  padding: 12px; "
        "} " + scrollBarStyle()
    );
    codeEdit->setMaximumHeight(300);

    if (!language.isEmpty()) {
        QLabel *langLabel = new QLabel(language, codeContainer);
        langLabel->setStyleSheet(
            "QLabel { "
            "  color: " + t.codeLanguageText + "; "
            "  font-size: 12px; "
            "  padding: 4px 8px; "
            "}"
        );

        QPushButton *copyBtn = new QPushButton("Copy", codeContainer);
        copyBtn->setIcon(makeLineIcon(AppIconGlyph::Copy));
        copyBtn->setIconSize(QSize(14, 14));
        copyBtn->setStyleSheet(
            "QPushButton { "
            "  background-color: " + t.surfaceAlt + "; "
            "  color: " + t.textStrong + "; "
            "  border: 1px solid " + t.borderStrong + "; "
            "  border-radius: 4px; "
            "  padding: 4px 12px; "
            "  font-size: 12px; "
            "} "
            "QPushButton:hover { background-color: " + t.scrollbarHover + "; }"
        );
        copyBtn->setMaximumWidth(60);

        QHBoxLayout *headerLayout = new QHBoxLayout();
        headerLayout->addWidget(langLabel);
        headerLayout->addStretch();
        headerLayout->addWidget(copyBtn);
        headerLayout->setContentsMargins(8, 4, 8, 4);

        QFrame *headerFrame = new QFrame(codeContainer);
        headerFrame->setStyleSheet(
            "QFrame { background-color: " + t.codeHeaderBg + "; border-top-left-radius: 8px; border-top-right-radius: 8px; }");
        headerFrame->setLayout(headerLayout);

        codeLayout->addWidget(headerFrame);
        connect(copyBtn, &QPushButton::clicked, [codeEdit]() {
            QClipboard *clipboard = QApplication::clipboard();
            const QString copiedText = codeEdit->toPlainText();
            clipboard->setText(copiedText);
            QTimer::singleShot(30000, [copiedText]() {
                QClipboard *currentClipboard = QApplication::clipboard();
                if (currentClipboard && currentClipboard->text() == copiedText)
                    currentClipboard->clear();
            });
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

void ChatMessageCard::highlightText(const QString &text)
{
    // Keep streaming updates lightweight; the final rendered card will
    // receive full markdown/highlight treatment once generation completes.
    if (m_isStreaming) {
        return;
    }
    if (m_highlightText == text) {
        return;
    }

    m_highlightText = text;
    rebuildContent();
    if (text.isEmpty()) {
        return;
    }

    const QRegularExpression regex("(" + QRegularExpression::escape(text) + ")", QRegularExpression::CaseInsensitiveOption);
    const QString mark = "<mark style='background-color:#fbbf24;color:#000;'>\\1</mark>";
    const auto labels = m_card->findChildren<QLabel*>();
    for (QLabel *label : labels) {
        if (label == m_tokenLabel || label == m_timestampLabel) {
            continue;
        }
        QString labelText = label->text();
        if (labelText.isEmpty()) {
            continue;
        }
        labelText.replace(regex, mark);
        label->setText(labelText);
    }
}

void ChatMessageCard::clearHighlight()
{
    if (m_isStreaming || m_highlightText.isEmpty()) {
        return;
    }
    m_highlightText.clear();
    rebuildContent();
}

ChatSearchBar::ChatSearchBar(QWidget *parent)
    : QFrame(parent)
{
    setFrameStyle(QFrame::StyledPanel);
    const Theme &t = currentTheme();
    setStyleSheet(
        "QFrame { "
        "  background-color: " + t.surfaceAlt + "; "
        "  border: 1px solid " + t.borderStrong + "; "
        "  border-radius: 6px; "
        "  padding: 4px; "
        "} "
        "QLineEdit { "
        "  background-color: " + t.inputBg + "; "
        "  color: " + t.textStrong + "; "
        "  border: 1px solid " + t.borderStrong + "; "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  font-size: 13px; "
        "} "
        "QPushButton { "
        "  background-color: " + t.surfaceAlt + "; "
        "  color: " + t.textStrong + "; "
        "  border: 1px solid " + t.borderStrong + "; "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  font-size: 12px; "
        "} "
        "QPushButton:hover { background-color: " + t.scrollbarHover + "; }"
    );

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(6, 4, 6, 4);
    layout->setSpacing(4);

    m_searchInput = new QLineEdit(this);
    m_searchInput->setPlaceholderText("Search in chat...");
    m_searchInput->setMaximumWidth(250);
    m_searchInput->addAction(makeLineIcon(AppIconGlyph::Search), QLineEdit::LeadingPosition);
    connect(m_searchInput, &QLineEdit::textChanged, this, &ChatSearchBar::searchTextChanged);

    m_matchCount = new QLabel("0/0", this);
    m_matchCount->setStyleSheet("QLabel { color: " + t.textMuted + "; font-size: 12px; padding: 0 4px; }");
    m_matchCount->setMaximumWidth(50);

    m_prevBtn = new QPushButton(this);
    m_prevBtn->setIcon(makeLineIcon(AppIconGlyph::ChevronUp));
    m_prevBtn->setIconSize(QSize(14, 14));
    m_prevBtn->setFixedSize(26, 26);
    m_prevBtn->setToolTip("Previous match");
    connect(m_prevBtn, &QPushButton::clicked, this, &ChatSearchBar::findPrevious);

    m_nextBtn = new QPushButton(this);
    m_nextBtn->setIcon(makeLineIcon(AppIconGlyph::ChevronDown));
    m_nextBtn->setIconSize(QSize(14, 14));
    m_nextBtn->setFixedSize(26, 26);
    m_nextBtn->setToolTip("Next match");
    connect(m_nextBtn, &QPushButton::clicked, this, &ChatSearchBar::findNext);

    m_closeBtn = new QPushButton(this);
    m_closeBtn->setIcon(makeLineIcon(AppIconGlyph::Close));
    m_closeBtn->setIconSize(QSize(14, 14));
    m_closeBtn->setFixedSize(26, 26);
    m_closeBtn->setToolTip("Close search");
    connect(m_closeBtn, &QPushButton::clicked, this, &ChatSearchBar::onClose);

    layout->addWidget(m_searchInput);
    layout->addWidget(m_matchCount);
    layout->addWidget(m_prevBtn);
    layout->addWidget(m_nextBtn);
    layout->addWidget(m_closeBtn);
}

void ChatSearchBar::onClose()
{
    m_searchInput->clear();
    emit closed();
    hide();
}
