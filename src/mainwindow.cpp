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
#include <QtMath>

namespace {

enum class AppIconGlyph {
    App,
    File,
    Export,
    Settings,
    Profiles,
    ClearChat,
    Theme,
    Advanced,
    Quit,
    NewChat,
    Send,
    Stop,
    Folder,
    Chat,
    Trash,
    Search,
    ChevronUp,
    ChevronDown,
    Close,
    Copy,
    Refresh,
    Branch,
    Edit,
    Save,
    Cancel,
    Image,
    Code,
    Key,
    Model,
    Pin,
    Unpin,
    Warning,
    Info,
    Database,
    Terminal,
    Globe
};

QIcon makeLineIcon(AppIconGlyph glyph, int size = 24, const QColor &accent = QColor("#007acc"))
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal scale = size / 24.0;
    auto sx = [scale](qreal value) { return value * scale; };

    QPen pen(accent, sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen softPen(QColor("#8ab4f8"), sx(1.5), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    switch (glyph) {
    case AppIconGlyph::App: {
        QPainterPath mark;
        mark.moveTo(sx(5), sx(7));
        mark.lineTo(sx(10), sx(3.5));
        mark.lineTo(sx(19), sx(6));
        mark.lineTo(sx(19), sx(18));
        mark.lineTo(sx(10), sx(20.5));
        mark.lineTo(sx(5), sx(17));
        mark.lineTo(sx(11), sx(12));
        mark.closeSubpath();
        painter.setBrush(QColor("#007acc"));
        painter.setPen(Qt::NoPen);
        painter.drawPath(mark);
        painter.setPen(QPen(QColor("#e8f3ff"), sx(1.6), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(10.5), sx(7.5)), QPointF(sx(15.5), sx(12)));
        painter.drawLine(QPointF(sx(15.5), sx(12)), QPointF(sx(10.5), sx(16.5)));
        break;
    }
    case AppIconGlyph::File:
        painter.drawRoundedRect(QRectF(sx(6), sx(3.5), sx(11), sx(17)), sx(1.8), sx(1.8));
        painter.drawLine(QPointF(sx(13), sx(3.5)), QPointF(sx(18), sx(8.5)));
        painter.drawLine(QPointF(sx(13), sx(3.5)), QPointF(sx(13), sx(8.5)));
        painter.drawLine(QPointF(sx(13), sx(8.5)), QPointF(sx(18), sx(8.5)));
        break;
    case AppIconGlyph::Folder:
        painter.setPen(QPen(QColor("#42a5f5"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(QColor(66, 165, 245, 45));
        {
            QPainterPath folder;
            folder.moveTo(sx(3.5), sx(7.5));
            folder.lineTo(sx(9), sx(7.5));
            folder.lineTo(sx(10.8), sx(5.5));
            folder.lineTo(sx(20.5), sx(5.5));
            folder.lineTo(sx(20.5), sx(18.5));
            folder.lineTo(sx(3.5), sx(18.5));
            folder.closeSubpath();
            painter.drawPath(folder);
        }
        break;
    case AppIconGlyph::Export:
        painter.drawRoundedRect(QRectF(sx(5), sx(5), sx(14), sx(15)), sx(2), sx(2));
        painter.drawLine(QPointF(sx(12), sx(15)), QPointF(sx(12), sx(3)));
        painter.drawLine(QPointF(sx(8), sx(7)), QPointF(sx(12), sx(3)));
        painter.drawLine(QPointF(sx(16), sx(7)), QPointF(sx(12), sx(3)));
        break;
    case AppIconGlyph::Settings:
    case AppIconGlyph::Advanced: {
        const QPointF center(sx(12), sx(12));
        painter.drawEllipse(center, sx(3), sx(3));
        for (int i = 0; i < 8; ++i) {
            const qreal angle = qDegreesToRadians(i * 45.0);
            QPointF inner(center.x() + qCos(angle) * sx(6), center.y() + qSin(angle) * sx(6));
            QPointF outer(center.x() + qCos(angle) * sx(8.5), center.y() + qSin(angle) * sx(8.5));
            painter.drawLine(inner, outer);
        }
        if (glyph == AppIconGlyph::Advanced) {
            painter.setPen(softPen);
            painter.drawLine(QPointF(sx(6), sx(19.5)), QPointF(sx(18), sx(19.5)));
        }
        break;
    }
    case AppIconGlyph::Profiles:
        painter.drawEllipse(QPointF(sx(10), sx(8)), sx(3.2), sx(3.2));
        painter.drawArc(QRectF(sx(4.5), sx(12), sx(11), sx(8)), 20 * 16, 140 * 16);
        painter.setPen(softPen);
        painter.drawEllipse(QPointF(sx(16.5), sx(9.5)), sx(2.3), sx(2.3));
        painter.drawArc(QRectF(sx(12.5), sx(13.5), sx(7), sx(5.5)), 20 * 16, 140 * 16);
        break;
    case AppIconGlyph::ClearChat:
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(12)), sx(2.5), sx(2.5));
        painter.drawLine(QPointF(sx(8), sx(20)), QPointF(sx(11), sx(17)));
        painter.drawLine(QPointF(sx(9), sx(9)), QPointF(sx(15), sx(15)));
        painter.drawLine(QPointF(sx(15), sx(9)), QPointF(sx(9), sx(15)));
        break;
    case AppIconGlyph::Theme:
        painter.setBrush(QColor(0, 122, 204, 45));
        painter.drawEllipse(QPointF(sx(12), sx(12)), sx(7), sx(7));
        painter.setBrush(Qt::NoBrush);
        painter.setPen(softPen);
        painter.drawArc(QRectF(sx(8), sx(5), sx(11), sx(14)), 90 * 16, 180 * 16);
        break;
    case AppIconGlyph::Quit:
        painter.drawLine(QPointF(sx(12), sx(4)), QPointF(sx(12), sx(12)));
        painter.drawArc(QRectF(sx(5), sx(5), sx(14), sx(14)), 210 * 16, 300 * 16);
        break;
    case AppIconGlyph::NewChat:
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(12)), sx(2.5), sx(2.5));
        painter.drawLine(QPointF(sx(12), sx(8)), QPointF(sx(12), sx(14)));
        painter.drawLine(QPointF(sx(9), sx(11)), QPointF(sx(15), sx(11)));
        painter.drawLine(QPointF(sx(8), sx(20)), QPointF(sx(11), sx(17)));
        break;
    case AppIconGlyph::Send:
        painter.setBrush(accent);
        painter.setPen(Qt::NoPen);
        painter.drawPolygon(QPolygonF{
            QPointF(sx(5), sx(4)),
            QPointF(sx(20), sx(12)),
            QPointF(sx(5), sx(20)),
            QPointF(sx(8), sx(12))
        });
        break;
    case AppIconGlyph::Stop:
        painter.setBrush(QColor("#ef4444"));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(sx(7), sx(7), sx(10), sx(10)), sx(2), sx(2));
        break;
    case AppIconGlyph::Chat:
        painter.setPen(QPen(QColor("#4fc3f7"), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(12)), sx(3), sx(3));
        painter.drawLine(QPointF(sx(8), sx(20)), QPointF(sx(11), sx(17)));
        painter.drawLine(QPointF(sx(8), sx(9.5)), QPointF(sx(16), sx(9.5)));
        painter.drawLine(QPointF(sx(8), sx(13)), QPointF(sx(14), sx(13)));
        break;
    case AppIconGlyph::Trash:
        painter.setPen(QPen(QColor("#ef5350"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(7)), QPointF(sx(17), sx(7)));
        painter.drawLine(QPointF(sx(10), sx(5)), QPointF(sx(14), sx(5)));
        painter.drawRoundedRect(QRectF(sx(8), sx(8), sx(8), sx(11)), sx(1.5), sx(1.5));
        painter.drawLine(QPointF(sx(10.5), sx(10.5)), QPointF(sx(10.5), sx(16.5)));
        painter.drawLine(QPointF(sx(13.5), sx(10.5)), QPointF(sx(13.5), sx(16.5)));
        break;
    case AppIconGlyph::Search:
        painter.setPen(QPen(QColor("#9ca3af"), sx(1.9), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(10.5), sx(10.5)), sx(5), sx(5));
        painter.drawLine(QPointF(sx(14.5), sx(14.5)), QPointF(sx(19), sx(19)));
        break;
    case AppIconGlyph::ChevronUp:
        painter.setPen(QPen(QColor("#cbd5e1"), sx(2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(14)), QPointF(sx(12), sx(9)));
        painter.drawLine(QPointF(sx(12), sx(9)), QPointF(sx(17), sx(14)));
        break;
    case AppIconGlyph::ChevronDown:
        painter.setPen(QPen(QColor("#cbd5e1"), sx(2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(10)), QPointF(sx(12), sx(15)));
        painter.drawLine(QPointF(sx(12), sx(15)), QPointF(sx(17), sx(10)));
        break;
    case AppIconGlyph::Close:
    case AppIconGlyph::Cancel:
        painter.setPen(QPen(QColor("#ef5350"), sx(2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(8), sx(8)), QPointF(sx(16), sx(16)));
        painter.drawLine(QPointF(sx(16), sx(8)), QPointF(sx(8), sx(16)));
        break;
    case AppIconGlyph::Copy:
        painter.setPen(QPen(QColor("#90caf9"), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(8), sx(7), sx(10), sx(12)), sx(1.8), sx(1.8));
        painter.drawRoundedRect(QRectF(sx(5), sx(4), sx(10), sx(12)), sx(1.8), sx(1.8));
        break;
    case AppIconGlyph::Refresh:
        painter.setPen(QPen(QColor("#81c784"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawArc(QRectF(sx(5), sx(5), sx(14), sx(14)), 35 * 16, 250 * 16);
        painter.drawLine(QPointF(sx(16.8), sx(5.6)), QPointF(sx(18.8), sx(5.8)));
        painter.drawLine(QPointF(sx(18.8), sx(5.8)), QPointF(sx(18.1), sx(8.2)));
        break;
    case AppIconGlyph::Branch:
        painter.setPen(QPen(QColor("#ce93d8"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(8), sx(6)), QPointF(sx(8), sx(18)));
        painter.drawLine(QPointF(sx(8), sx(12)), QPointF(sx(16), sx(8)));
        painter.drawEllipse(QPointF(sx(8), sx(6)), sx(2), sx(2));
        painter.drawEllipse(QPointF(sx(8), sx(18)), sx(2), sx(2));
        painter.drawEllipse(QPointF(sx(16), sx(8)), sx(2), sx(2));
        break;
    case AppIconGlyph::Edit:
        painter.setPen(QPen(QColor("#ffb74d"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(17)), QPointF(sx(16.5), sx(7.5)));
        painter.drawLine(QPointF(sx(14), sx(5)), QPointF(sx(19), sx(10)));
        painter.drawLine(QPointF(sx(6), sx(18)), QPointF(sx(10), sx(17)));
        break;
    case AppIconGlyph::Save:
        painter.setPen(QPen(QColor("#4ade80"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(5), sx(4), sx(14), sx(16)), sx(2), sx(2));
        painter.drawLine(QPointF(sx(8), sx(4)), QPointF(sx(8), sx(9)));
        painter.drawLine(QPointF(sx(16), sx(4)), QPointF(sx(16), sx(9)));
        painter.drawRoundedRect(QRectF(sx(8), sx(13), sx(8), sx(5)), sx(1), sx(1));
        break;
    case AppIconGlyph::Image:
        painter.setPen(QPen(QColor("#64b5f6"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(14)), sx(2), sx(2));
        painter.drawEllipse(QPointF(sx(15.5), sx(9)), sx(1.5), sx(1.5));
        painter.drawLine(QPointF(sx(6.5), sx(16.5)), QPointF(sx(10), sx(12.5)));
        painter.drawLine(QPointF(sx(10), sx(12.5)), QPointF(sx(13), sx(15)));
        painter.drawLine(QPointF(sx(13), sx(15)), QPointF(sx(16), sx(11.5)));
        painter.drawLine(QPointF(sx(16), sx(11.5)), QPointF(sx(19), sx(16.5)));
        break;
    case AppIconGlyph::Code:
        painter.setPen(QPen(QColor("#4dd0e1"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(9), sx(8)), QPointF(sx(5), sx(12)));
        painter.drawLine(QPointF(sx(5), sx(12)), QPointF(sx(9), sx(16)));
        painter.drawLine(QPointF(sx(15), sx(8)), QPointF(sx(19), sx(12)));
        painter.drawLine(QPointF(sx(19), sx(12)), QPointF(sx(15), sx(16)));
        painter.drawLine(QPointF(sx(13), sx(7)), QPointF(sx(11), sx(17)));
        break;
    case AppIconGlyph::Key:
        painter.setPen(QPen(QColor("#ffd54f"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(8), sx(10)), sx(3), sx(3));
        painter.drawLine(QPointF(sx(11), sx(10)), QPointF(sx(20), sx(10)));
        painter.drawLine(QPointF(sx(16), sx(10)), QPointF(sx(16), sx(13)));
        painter.drawLine(QPointF(sx(19), sx(10)), QPointF(sx(19), sx(12)));
        break;
    case AppIconGlyph::Model:
        painter.setPen(QPen(QColor("#26c6da"), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(5), sx(5), sx(14), sx(14)), sx(3), sx(3));
        painter.drawLine(QPointF(sx(9), sx(9)), QPointF(sx(15), sx(9)));
        painter.drawLine(QPointF(sx(9), sx(12)), QPointF(sx(15), sx(12)));
        painter.drawLine(QPointF(sx(9), sx(15)), QPointF(sx(13), sx(15)));
        break;
    case AppIconGlyph::Pin:
    case AppIconGlyph::Unpin:
        painter.setPen(QPen(QColor("#f06292"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(12), sx(13)), QPointF(sx(12), sx(21)));
        painter.drawLine(QPointF(sx(8), sx(5)), QPointF(sx(16), sx(5)));
        painter.drawLine(QPointF(sx(10), sx(5)), QPointF(sx(9), sx(13)));
        painter.drawLine(QPointF(sx(14), sx(5)), QPointF(sx(15), sx(13)));
        painter.drawLine(QPointF(sx(9), sx(13)), QPointF(sx(15), sx(13)));
        if (glyph == AppIconGlyph::Unpin) {
            painter.drawLine(QPointF(sx(6), sx(18)), QPointF(sx(18), sx(6)));
        }
        break;
    case AppIconGlyph::Warning:
        painter.setPen(QPen(QColor("#ffb300"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPolygon(QPolygonF{QPointF(sx(12), sx(4)), QPointF(sx(21), sx(19)), QPointF(sx(3), sx(19))});
        painter.drawLine(QPointF(sx(12), sx(9)), QPointF(sx(12), sx(14)));
        painter.drawPoint(QPointF(sx(12), sx(16.5)));
        break;
    case AppIconGlyph::Info:
        painter.setPen(QPen(QColor("#42a5f5"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(12), sx(12)), sx(8), sx(8));
        painter.drawLine(QPointF(sx(12), sx(11)), QPointF(sx(12), sx(16)));
        painter.drawPoint(QPointF(sx(12), sx(8)));
        break;
    case AppIconGlyph::Database:
        painter.setPen(QPen(QColor("#fdd835"), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(12), sx(6.5)), sx(6.5), sx(2.5));
        painter.drawLine(QPointF(sx(5.5), sx(6.5)), QPointF(sx(5.5), sx(17)));
        painter.drawLine(QPointF(sx(18.5), sx(6.5)), QPointF(sx(18.5), sx(17)));
        painter.drawEllipse(QPointF(sx(12), sx(17)), sx(6.5), sx(2.5));
        painter.drawArc(QRectF(sx(5.5), sx(9.5), sx(13), sx(5)), 180 * 16, 180 * 16);
        break;
    case AppIconGlyph::Terminal:
        painter.setPen(QPen(QColor("#90a4ae"), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(14)), sx(2), sx(2));
        painter.drawLine(QPointF(sx(7), sx(9)), QPointF(sx(10), sx(12)));
        painter.drawLine(QPointF(sx(10), sx(12)), QPointF(sx(7), sx(15)));
        painter.drawLine(QPointF(sx(12), sx(15)), QPointF(sx(17), sx(15)));
        break;
    case AppIconGlyph::Globe:
        painter.setPen(QPen(QColor("#29b6f6"), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(12), sx(12)), sx(8), sx(8));
        painter.drawLine(QPointF(sx(4), sx(12)), QPointF(sx(20), sx(12)));
        painter.drawArc(QRectF(sx(8), sx(4), sx(8), sx(16)), 90 * 16, 180 * 16);
        painter.drawArc(QRectF(sx(8), sx(4), sx(8), sx(16)), -90 * 16, 180 * 16);
        break;
    }

    return QIcon(pixmap);
}

}

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
        painter.setBrush(QColor("#1d4ed8"));
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
    m_titleLabel->setStyleSheet("QLabel { color: #ececf1; font-size: 13px; font-weight: 600; }");
    m_titleLabel->setWordWrap(false);
    textLayout->addWidget(m_titleLabel);

    m_subtitleLabel = new QLabel(subtitle, textContainer);
    m_subtitleLabel->setStyleSheet("QLabel { color: #8e8e8e; font-size: 11px; }");
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

void ChatListItem::setActive(bool active)
{
    m_isActive = active;
    if (m_isActive) {
        setStyleSheet("QWidget { background-color: #1f3a2d; border: 1px solid #15803d; border-radius: 10px; }");
        if (m_subtitleLabel) {
            m_subtitleLabel->setStyleSheet("QLabel { color: #d1fae5; font-size: 11px; font-weight: 600; }");
        }
    } else {
        setStyleSheet("QWidget { background-color: transparent; border: 1px solid transparent; border-radius: 10px; }");
        if (m_subtitleLabel) {
            m_subtitleLabel->setStyleSheet("QLabel { color: #8e8e8e; font-size: 11px; }");
        }
    }
}

void ChatListItem::enterEvent(QEnterEvent *event)
{
    QWidget::enterEvent(event);
    m_deleteBtn->setVisible(true);
    if (!m_isActive) {
        setStyleSheet("QWidget { background-color: #262c34; border: 1px solid #334155; border-radius: 10px; }");
    }
    if (m_subtitleLabel && !m_isActive) {
        m_subtitleLabel->setStyleSheet("QLabel { color: #cbd5e1; font-size: 11px; }");
    }
}

void ChatListItem::leaveEvent(QEvent *event)
{
    QWidget::leaveEvent(event);
    m_deleteBtn->setVisible(false);
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
    QString bgColor = (role == "user" ? "#14532d" : (role == "error" ? "#4a1515" : "#20242b"));
    m_card->setStyleSheet(
        "QFrame { "
        "  background-color: " + bgColor + "; "
        "  border: 1px solid #2f3742; "
        "  border-radius: 18px; "
        "  padding: 12px 16px; "
        "}"
    );
    m_card->setMaximumWidth(760);

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
        "  color: #94a3b8; "
        "  font-size: 11px; "
        "  padding: 0 2px; "
        "}"
    );
    copyLayout->addWidget(m_tokenLabel, 0, Qt::AlignVCenter);
    copyLayout->addStretch();

    m_copyBtn = new QPushButton("Copy", m_copyContainer);
    m_copyBtn->setIcon(makeLineIcon(AppIconGlyph::Copy));
    m_copyBtn->setIconSize(QSize(14, 14));
    m_copyBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #334155; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 7px; "
        "  padding: 5px 10px; "
        "  font-size: 12px; "
        "} "
        "QPushButton:hover { background-color: #475569; } "
        "QPushButton:disabled { background-color: #1f2937; color: #4ade80; }"
    );
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
    m_regenerateBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #334155; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 7px; "
        "  padding: 5px 10px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #475569; }"
    );
    m_regenerateBtn->setMaximumWidth(112);
    m_regenerateBtn->setVisible(false);
    copyLayout->addWidget(m_regenerateBtn);
    connect(m_regenerateBtn, &QPushButton::clicked, this, [this]() {
        emit regenerateRequested(m_messageIndex);
    });

    m_branchBtn = new QPushButton("Branch", m_card);
    m_branchBtn->setIcon(makeLineIcon(AppIconGlyph::Branch));
    m_branchBtn->setIconSize(QSize(14, 14));
    m_branchBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #334155; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 7px; "
        "  padding: 5px 10px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #475569; }"
    );
    m_branchBtn->setMaximumWidth(76);
    m_branchBtn->setVisible(false);
    copyLayout->addWidget(m_branchBtn);
    connect(m_branchBtn, &QPushButton::clicked, [this]() {
        emit branchRequested(m_messageIndex);
    });

    m_editBtn = new QPushButton("Edit", m_card);
    m_editBtn->setIcon(makeLineIcon(AppIconGlyph::Edit));
    m_editBtn->setIconSize(QSize(14, 14));
    m_editBtn->setStyleSheet(
        "QPushButton { "
        "  background-color: #334155; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 7px; "
        "  padding: 5px 10px; "
        "  font-size: 11px; "
        "} "
        "QPushButton:hover { background-color: #475569; }"
    );
    m_editBtn->setMaximumWidth(62);
    m_editBtn->setVisible(false);
    copyLayout->addWidget(m_editBtn);
    connect(m_editBtn, &QPushButton::clicked, [this]() {
        startEditing();
    });

    m_layout->addWidget(m_copyContainer);

    m_outerLayout->addLayout(rowLayout);

    m_timestampLabel = new QLabel(this);
    m_timestampLabel->setStyleSheet("QLabel { color: #8e8e8e; font-size: 11px; padding: 2px 8px; }");
    m_timestampLabel->setVisible(false);
    m_timestampLabel->setAlignment(role == "user" ? Qt::AlignRight : Qt::AlignLeft);
    m_outerLayout->addWidget(m_timestampLabel);

    setMouseTracking(true);
    m_card->setMouseTracking(true);
}

void ChatMessageCard::setTimestamp(const QDateTime &timestamp)
{
    m_timestamp = timestamp;
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
            m_streamLabel->setStyleSheet(
                "QLabel { "
                "  color: #ececf1; "
                "  font-size: " + QString::number(m_contentFontSize) + "px; "
                "  line-height: 1.6; "
                "} "
                "QLabel a { color: #60a5fa; }"
            );
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
        "  font-size: " + QString::number(m_contentFontSize) + "px; "
        "}"
    );

    QHBoxLayout *editBtnLayout = new QHBoxLayout();
    editBtnLayout->setContentsMargins(0, 4, 0, 0);
    editBtnLayout->addStretch();

    QPushButton *saveBtn = new QPushButton("Save", m_card);
    saveBtn->setIcon(makeLineIcon(AppIconGlyph::Save));
    saveBtn->setIconSize(QSize(14, 14));
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
    cancelBtn->setIcon(makeLineIcon(AppIconGlyph::Cancel));
    cancelBtn->setIconSize(QSize(14, 14));
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
    result.replace(QRegularExpression("`(.+?)`"), "<code style='background-color:#1a1a1a; padding:2px 4px; border-radius:3px; font-family:monospace;'>\\1</code>");
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
        "  font-size: " + QString::number(m_contentFontSize) + "px; "
        "  line-height: 1.75; "
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
        "  font-size: " + QString::number(qMax(10, m_contentFontSize - 1)) + "px; "
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
        copyBtn->setIcon(makeLineIcon(AppIconGlyph::Copy));
        copyBtn->setIconSize(QSize(14, 14));
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
    setStyleSheet(
        "QFrame { "
        "  background-color: #2f2f2f; "
        "  border: 1px solid #4d4d4f; "
        "  border-radius: 6px; "
        "  padding: 4px; "
        "} "
        "QLineEdit { "
        "  background-color: #1a1a1a; "
        "  color: #ececf1; "
        "  border: 1px solid #4d4d4f; "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  font-size: 13px; "
        "} "
        "QPushButton { "
        "  background-color: #404040; "
        "  color: #ececf1; "
        "  border: none; "
        "  border-radius: 4px; "
        "  padding: 4px 8px; "
        "  font-size: 12px; "
        "} "
        "QPushButton:hover { background-color: #505050; }"
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
    m_matchCount->setStyleSheet("QLabel { color: #8e8e8e; font-size: 12px; padding: 0 4px; }");
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
{
    setWindowTitle("SimpleAIClient");
    setWindowIcon(makeLineIcon(AppIconGlyph::App, 256));
    resize(1200, 800);
    setAcceptDrops(true);

    setupUI();
    setupMenu();
    loadProfiles();
    loadSettings();
    loadChatSessions();

    m_inputField->installEventFilter(this);

    new QShortcut(QKeySequence("Ctrl+N"), this, SLOT(onNewChat()));
    new QShortcut(QKeySequence("Ctrl+E"), this, SLOT(onExportChat()));
    new QShortcut(QKeySequence("Ctrl+,"), this, SLOT(onSettings()));

    connect(m_sendButton, &QToolButton::clicked, this, &MainWindow::onSendMessage);
    connect(m_apiClient, &ApiClient::responseReceived, this, &MainWindow::onResponseReceived);
    connect(m_apiClient, &ApiClient::responseChunk, this, &MainWindow::onResponseChunk);
    connect(m_apiClient, &ApiClient::responseFinished, this, &MainWindow::onResponseFinished);
    connect(m_apiClient, &ApiClient::requestCancelled, this, &MainWindow::onRequestCancelled);
    connect(m_apiClient, &ApiClient::errorOccurred, this, &MainWindow::onApiErrorWithRetry);
    connect(m_apiClient, &ApiClient::modelsFetched, this, &MainWindow::onModelsFetched);
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
    m_inputField->setMinimumHeight(44);
    m_inputField->setStyleSheet(
        "QTextEdit { "
        "  background-color: transparent; "
        "  color: #ececf1; "
        "  border: none; "
        "  font-size: 15px; "
        "  selection-background-color: #404040; "
        "} " + scrollbarStyle
    );
    m_inputField->document()->setDocumentMargin(2);

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
    m_attachButton->setIcon(makeLineIcon(AppIconGlyph::Image));
    m_attachButton->setIconSize(QSize(20, 20));
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

    m_sendButton->setIcon(makeLineIcon(AppIconGlyph::Send));
    m_sendButton->setIconSize(QSize(20, 20));
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

    const QString panelSurface = "#16181c";
    const QString panelBorder = "#2b3139";
    const QString cardSurface = "#20242b";
    const QString cardSurfaceAlt = "#262c34";
    const QString accent = "#15803d";
    const QString accentSoft = "#163e2a";
    const QString textStrong = "#f3f4f6";
    const QString textMuted = "#9ca3af";

    m_sidebar = new QWidget(this);
    // Keep the navigation rail stable so the chat list does not "breathe"
    // as the main panel content changes or the window is resized.
    m_sidebar->setFixedWidth(300);
    m_sidebar->setStyleSheet(QString(
        "QWidget { background-color: %1; color: %2; border-right: 1px solid %3; }")
        .arg(panelSurface).arg(textStrong).arg(panelBorder));

    QVBoxLayout *sidebarLayout = new QVBoxLayout(m_sidebar);
    sidebarLayout->setContentsMargins(18, 20, 18, 16);
    sidebarLayout->setSpacing(12);

    m_appTitle = new QLabel("SimpleAIClient", m_sidebar);
    m_appTitle->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 22px; font-weight: 700; letter-spacing: 0.5px; padding: 0; }")
        .arg(textStrong));
    sidebarLayout->addWidget(m_appTitle);

    QLabel *sidebarSubtitle = new QLabel("Fast local chat, without the clutter.", m_sidebar);
    sidebarSubtitle->setWordWrap(true);
    sidebarSubtitle->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 12px; line-height: 1.4; padding-bottom: 4px; }")
        .arg(textMuted));
    sidebarLayout->addWidget(sidebarSubtitle);

    m_newChatButton->setStyleSheet(QString(
        "QPushButton { background-color: %1; color: white; border: none; border-radius: 12px; "
        "padding: 12px 14px; text-align: left; font-size: 14px; font-weight: 600; } "
        "QPushButton:hover { background-color: #16a34a; } "
        "QPushButton:pressed { background-color: #166534; }")
        .arg(accent));
    m_newChatButton->setText("New Chat");
    m_newChatButton->setIcon(makeLineIcon(AppIconGlyph::NewChat, 24, QColor("#ffffff")));
    m_newChatButton->setIconSize(QSize(18, 18));
    sidebarLayout->addWidget(m_newChatButton);

    m_searchField = new QLineEdit(m_sidebar);
    m_searchField->setPlaceholderText("Search chats");
    m_searchField->addAction(makeLineIcon(AppIconGlyph::Search), QLineEdit::LeadingPosition);
    m_searchField->setStyleSheet(QString(
        "QLineEdit { background-color: %1; color: %2; border: 1px solid %3; border-radius: 10px; "
        "padding: 9px 12px; font-size: 13px; } "
        "QLineEdit:focus { border: 1px solid %4; background-color: %5; }")
        .arg(cardSurface).arg(textStrong).arg(panelBorder).arg(accent).arg(cardSurfaceAlt));
    connect(m_searchField, &QLineEdit::textChanged, this, &MainWindow::filterChats);
    sidebarLayout->addWidget(m_searchField);

    QLabel *historyLabel = new QLabel("Recent Chats", m_sidebar);
    historyLabel->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 11px; font-weight: 700; letter-spacing: 1px; text-transform: uppercase; }")
        .arg(textMuted));
    sidebarLayout->addWidget(historyLabel);

    QFrame *historyFrame = new QFrame(m_sidebar);
    historyFrame->setStyleSheet(QString(
        "QFrame { background-color: %1; border: 1px solid %2; border-radius: 16px; }")
        .arg(cardSurface).arg(panelBorder));
    QVBoxLayout *historyLayout = new QVBoxLayout(historyFrame);
    historyLayout->setContentsMargins(8, 8, 8, 8);
    historyLayout->setSpacing(0);

    m_chatListContainer = new QWidget();
    m_chatListContainer->setStyleSheet("background-color: transparent;");
    QVBoxLayout *chatListLayout = new QVBoxLayout(m_chatListContainer);
    chatListLayout->setContentsMargins(0, 0, 0, 0);
    chatListLayout->setSpacing(4);
    chatListLayout->addStretch();

    m_chatListScroll = new QScrollArea(historyFrame);
    m_chatListScroll->setWidget(m_chatListContainer);
    m_chatListScroll->setWidgetResizable(true);
    m_chatListScroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_chatListScroll->setStyleSheet("QScrollArea { border: none; background: transparent; }" + scrollbarStyle);
    historyLayout->addWidget(m_chatListScroll);
    sidebarLayout->addWidget(historyFrame, 1);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setHandleWidth(1);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->addWidget(m_sidebar);

    QWidget *mainPanel = new QWidget(this);
    mainPanel->setStyleSheet(QString("background-color: #111418; color: %1;").arg(textStrong));
    QVBoxLayout *mainLayout = new QVBoxLayout(mainPanel);
    mainLayout->setContentsMargins(18, 18, 18, 14);
    mainLayout->setSpacing(12);

    QFrame *headerFrame = new QFrame(mainPanel);
    headerFrame->setStyleSheet(QString(
        "QFrame { background-color: #151a1f; border: 1px solid %1; border-radius: 18px; }")
        .arg(panelBorder));
    QVBoxLayout *headerOuter = new QVBoxLayout(headerFrame);
    headerOuter->setContentsMargins(18, 16, 18, 14);
    headerOuter->setSpacing(12);

    QHBoxLayout *headerTop = new QHBoxLayout();
    headerTop->setSpacing(12);

    QVBoxLayout *headerTextLayout = new QVBoxLayout();
    headerTextLayout->setSpacing(3);
    m_headerTitle = new QLabel("New Chat", headerFrame);
    m_headerTitle->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 22px; font-weight: 700; }").arg(textStrong));
    m_headerSubtitle = new QLabel("Start typing below. Model and profile stay available, but out of the way.", headerFrame);
    m_headerSubtitle->setWordWrap(true);
    m_headerSubtitle->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 12px; line-height: 1.4; }").arg(textMuted));
    headerTextLayout->addWidget(m_headerTitle);
    headerTextLayout->addWidget(m_headerSubtitle);
    headerTop->addLayout(headerTextLayout, 1);

    m_modelCombo->setMinimumWidth(220);
    m_modelCombo->setMaximumWidth(320);
    m_modelCombo->setStyleSheet(QString(
        "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 10px; "
        "padding: 8px 12px; font-size: 13px; } "
        "QComboBox::drop-down { border: none; padding-right: 10px; } "
        "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: %4; font-size: 13px; }")
        .arg(cardSurface).arg(textStrong).arg(panelBorder).arg(accentSoft));

    m_profileCombo = new QComboBox(this);
    m_profileCombo->setMinimumWidth(140);
    m_profileCombo->setMaximumWidth(180);
    m_profileCombo->setStyleSheet(QString(
        "QComboBox { background-color: %1; color: %2; border: 1px solid %3; border-radius: 10px; "
        "padding: 8px 12px; font-size: 13px; } "
        "QComboBox::drop-down { border: none; padding-right: 10px; } "
        "QComboBox QAbstractItemView { background-color: %1; color: %2; selection-background-color: %4; font-size: 13px; }")
        .arg(cardSurface).arg(textStrong).arg(panelBorder).arg(accentSoft));

    QVBoxLayout *selectorLayout = new QVBoxLayout();
    selectorLayout->setSpacing(8);
    selectorLayout->addWidget(m_profileCombo, 0, Qt::AlignRight);
    selectorLayout->addWidget(m_modelCombo, 0, Qt::AlignRight);
    headerTop->addLayout(selectorLayout);
    headerOuter->addLayout(headerTop);

    const QString toggleStyle = QString(
        "QCheckBox { color: %1; font-size: 12px; spacing: 6px; background-color: %2; "
        "border: 1px solid %3; border-radius: 999px; padding: 6px 10px; } "
        "QCheckBox::indicator { width: 12px; height: 12px; border-radius: 6px; border: 1px solid %4; background-color: transparent; } "
        "QCheckBox::indicator:checked { background-color: %4; border: 1px solid %4; }")
        .arg(textMuted).arg(cardSurface).arg(panelBorder).arg(accent);

    QHBoxLayout *toggleRow = new QHBoxLayout();
    toggleRow->setSpacing(8);

    m_uncensoredFilter = new QCheckBox("Uncensored", this);
    m_uncensoredFilter->setStyleSheet(toggleStyle);
    m_uncensoredFilter->setChecked(m_settings.value("uncensoredFilter", false).toBool());
    connect(m_uncensoredFilter, &QCheckBox::toggled, this, [this]() {
        m_settings.setValue("uncensoredFilter", m_uncensoredFilter->isChecked());
        applyModelFilter();
    });
    toggleRow->addWidget(m_uncensoredFilter);

    m_streamToggle = new QCheckBox("Streaming", this);
    m_streamToggle->setStyleSheet(toggleStyle);
    m_streamToggle->setChecked(m_settings.value("streamMode", true).toBool());
    m_apiClient->setStreaming(m_streamToggle->isChecked());
    connect(m_streamToggle, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.setValue("streamMode", checked);
        m_apiClient->setStreaming(checked);
    });
    toggleRow->addWidget(m_streamToggle);

    m_webSearchToggle = new QCheckBox("Web Search", this);
    m_webSearchToggle->setStyleSheet(toggleStyle);
    m_webSearchToggle->setChecked(m_settings.value("webSearch", false).toBool());
    m_apiClient->setWebSearch(m_webSearchToggle->isChecked());
    connect(m_webSearchToggle, &QCheckBox::toggled, this, [this](bool checked) {
        m_settings.setValue("webSearch", checked);
        m_apiClient->setWebSearch(checked);
        updateHeaderState();
    });
    toggleRow->addWidget(m_webSearchToggle);

    m_autoScrollToggle = new QCheckBox("Auto Scroll", this);
    m_autoScrollToggle->setStyleSheet(toggleStyle);
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
    m_searchBar->setStyleSheet(QString(
        "QFrame { background-color: %1; border: 1px solid %2; border-radius: 14px; padding: 6px; } "
        "QLineEdit { background-color: #101418; color: %3; border: 1px solid %2; border-radius: 8px; padding: 6px 8px; font-size: 13px; } "
        "QPushButton { background-color: %4; color: %3; border: none; border-radius: 7px; padding: 4px 8px; font-size: 12px; } "
        "QPushButton:hover { background-color: %1; }")
        .arg(cardSurface).arg(panelBorder).arg(textStrong).arg(cardSurfaceAlt));
    mainLayout->addWidget(m_searchBar);

    m_scrollArea = new QScrollArea(mainPanel);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_scrollArea->setStyleSheet("QScrollArea { border: none; background-color: transparent; }" + scrollbarStyle);

    m_chatContainer = new QWidget();
    m_chatContainer->setStyleSheet("background-color: transparent;");
    m_chatLayout = new QVBoxLayout(m_chatContainer);
    m_chatLayout->setContentsMargins(58, 18, 58, 22);
    m_chatLayout->setSpacing(8);

    m_welcomeWidget = new QWidget(m_chatContainer);
    m_welcomeWidget->setMaximumWidth(620);
    m_welcomeWidget->setStyleSheet(QString(
        "QWidget { background-color: %1; border: 1px solid %2; border-radius: 24px; }")
        .arg(cardSurface).arg(panelBorder));
    QVBoxLayout *welcomeLayout = new QVBoxLayout(m_welcomeWidget);
    welcomeLayout->setContentsMargins(28, 30, 28, 28);
    welcomeLayout->setAlignment(Qt::AlignCenter);
    welcomeLayout->setSpacing(14);

    QLabel *welcomeEyebrow = new QLabel("READY WHEN YOU ARE", m_welcomeWidget);
    welcomeEyebrow->setAlignment(Qt::AlignCenter);
    welcomeEyebrow->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 11px; font-weight: 700; letter-spacing: 2px; }")
        .arg(textMuted));
    welcomeLayout->addWidget(welcomeEyebrow);

    QLabel *welcomeTitle = new QLabel("Talk to the model, not the UI.", m_welcomeWidget);
    welcomeTitle->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 30px; font-weight: 700; }").arg(textStrong));
    welcomeTitle->setAlignment(Qt::AlignCenter);
    welcomeLayout->addWidget(welcomeTitle);

    QLabel *welcomeSubtitle = new QLabel("Pick a model if you need to, then just type. Everything else stays quiet until you want it.", m_welcomeWidget);
    welcomeSubtitle->setWordWrap(true);
    welcomeSubtitle->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 14px; line-height: 1.5; }").arg(textMuted));
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
    inputFrame->setStyleSheet(QString(
        "QFrame { background-color: #151a1f; border: 1px solid %1; border-radius: 20px; }")
        .arg(panelBorder));
    QVBoxLayout *inputWithCounter = new QVBoxLayout(inputFrame);
    inputWithCounter->setContentsMargins(18, 14, 18, 14);
    inputWithCounter->setSpacing(10);

    QLabel *composerHint = new QLabel("Enter to send, Shift+Enter for a new line.", inputFrame);
    composerHint->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 11px; }").arg(textMuted));
    inputWithCounter->addWidget(composerHint, 0, Qt::AlignLeft);

    QWidget *inputWrapper = new QWidget(inputFrame);
    inputWrapper->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    inputWrapper->setStyleSheet(QString(
        "QWidget { background-color: %1; border: 1px solid %2; border-radius: 22px; }")
        .arg(cardSurfaceAlt).arg(panelBorder));
    QVBoxLayout *inputWrapperLayout = new QVBoxLayout(inputWrapper);
    inputWrapperLayout->setContentsMargins(12, 12, 12, 10);
    inputWrapperLayout->setSpacing(8);

    m_imagePreview = new QLabel(inputWrapper);
    m_imagePreview->setMaximumHeight(72);
    m_imagePreview->setMaximumWidth(220);
    m_imagePreview->setScaledContents(true);
    m_imagePreview->setStyleSheet(QString(
        "QLabel { background-color: %1; border: 1px solid %2; border-radius: 10px; padding: 4px; }")
        .arg(cardSurface).arg(panelBorder));
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
    dropHint->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; }").arg(textMuted));
    m_charCounter = new QLabel("0", inputFrame);
    m_charCounter->setStyleSheet(QString("QLabel { color: %1; font-size: 11px; }").arg(textMuted));
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
    m_statusBar->setStyleSheet(QString(
        "QStatusBar { background-color: %1; color: %2; font-size: 12px; border-top: 1px solid %3; }")
        .arg(panelSurface).arg(textMuted).arg(panelBorder));

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

    m_isDarkTheme = m_settings.value("darkTheme", true).toBool();
    applyTheme();
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
    QString apiKey = CredentialStore::unprotect(m_settings.value("apiKey").toString());

    m_apiClient->setApiKey(apiKey);
    restoreModelSelection();

    QString systemPrompt = m_settings.value("systemPrompt").toString();
    m_apiClient->setSystemPrompt(systemPrompt);

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
    if (!force && index == m_currentChatIndex && m_chatSessions[index].messagesLoaded) return;

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

QString MainWindow::chatBackupFilePath() const
{
    QString baseDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    if (baseDir.isEmpty()) {
        baseDir = QDir::homePath() + "/.simpleaiclient";
    }
    return QDir(baseDir).filePath("backups/chat-backup.json");
}

QJsonObject MainWindow::buildChatBackupSnapshot() const
{
    QJsonArray sessionsArray;

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        const ChatSession &chat = m_chatSessions[i];
        QJsonArray messagesArray;

        if (chat.messagesLoaded) {
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
        } else {
            QString data = m_settings.value(QString("chatMessages/%1").arg(chat.id)).toString();
            if (!data.isEmpty()) {
                QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
                if (doc.isArray()) {
                    messagesArray = doc.array();
                }
            }
        }

        QJsonObject sessionObj;
        sessionObj["id"] = chat.id;
        sessionObj["title"] = chat.title;
        sessionObj["pinned"] = chat.pinned;
        sessionObj["scrollPosition"] = chat.scrollPosition;
        sessionObj["messageCount"] = messagesArray.size();
        sessionObj["messages"] = messagesArray;
        sessionsArray.append(sessionObj);
    }

    QJsonObject snapshot;
    snapshot["version"] = 1;
    snapshot["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    snapshot["currentChatId"] = (m_currentChatIndex >= 0 && m_currentChatIndex < m_chatSessions.size())
        ? m_chatSessions[m_currentChatIndex].id
        : QString();
    snapshot["sessions"] = sessionsArray;
    return snapshot;
}

bool MainWindow::loadChatBackupSnapshot(QJsonObject *snapshot) const
{
    if (!snapshot) {
        return false;
    }

    QFile file(chatBackupFilePath());
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return false;
    }

    const QByteArray raw = file.readAll();
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(raw, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return false;
    }

    *snapshot = doc.object();
    return true;
}

bool MainWindow::saveChatBackup()
{
    QJsonObject snapshot = buildChatBackupSnapshot();
    QJsonDocument doc(snapshot);

    const QString path = chatBackupFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    if (!file.commit()) {
        return false;
    }

    return true;
}

QList<int> MainWindow::recoverableChatIndices() const
{
    QList<int> indices;

    QJsonObject snapshot;
    if (!loadChatBackupSnapshot(&snapshot)) {
        return indices;
    }

    QMap<QString, int> backupMessageCounts;
    QJsonArray sessionsArray = snapshot["sessions"].toArray();
    for (const auto &val : sessionsArray) {
        QJsonObject sessionObj = val.toObject();
        const QString id = sessionObj["id"].toString();
        const int messageCount = sessionObj["messageCount"].toInt();
        if (!id.isEmpty() && messageCount > 0) {
            backupMessageCounts[id] = messageCount;
        }
    }

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        const ChatSession &chat = m_chatSessions[i];
        if (chat.messageCount > 0) {
            continue;
        }
        if (backupMessageCounts.contains(chat.id)) {
            indices.append(i);
        }
    }

    return indices;
}

bool MainWindow::restoreChatFromBackup(const QString &chatId)
{
    if (chatId.isEmpty()) {
        return false;
    }

    QJsonObject snapshot;
    if (!loadChatBackupSnapshot(&snapshot)) {
        return false;
    }

    QJsonArray sessionsArray = snapshot["sessions"].toArray();
    QJsonArray messagesArray;
    QString title;
    int scrollPosition = 0;
    bool pinned = false;
    bool found = false;

    for (const auto &val : sessionsArray) {
        QJsonObject sessionObj = val.toObject();
        if (sessionObj["id"].toString() != chatId) {
            continue;
        }

        messagesArray = sessionObj["messages"].toArray();
        title = sessionObj["title"].toString();
        scrollPosition = sessionObj["scrollPosition"].toInt();
        pinned = sessionObj["pinned"].toBool();
        found = true;
        break;
    }

    if (!found || messagesArray.isEmpty()) {
        return false;
    }

    const QString messagesKey = QString("chatMessages/%1").arg(chatId);
    m_settings.setValue(messagesKey, QString::fromUtf8(QJsonDocument(messagesArray).toJson(QJsonDocument::Compact)));

    for (int i = 0; i < m_chatSessions.size(); ++i) {
        if (m_chatSessions[i].id != chatId) {
            continue;
        }

        m_chatSessions[i].title = title.isEmpty() ? m_chatSessions[i].title : title;
        m_chatSessions[i].pinned = pinned;
        m_chatSessions[i].scrollPosition = scrollPosition;
        m_chatSessions[i].messageCount = messagesArray.size();
        m_chatSessions[i].messagesLoaded = false;
        if (i == m_currentChatIndex) {
            loadChatMessages(i);
            rebuildCurrentChatView();
            updateHeaderState();
            updateContextUsage();
            updateChatDuration();
        }
        break;
    }

    saveChatSessions();
    updateChatList();
    return true;
}

void MainWindow::saveChatSessions()
{
    QJsonArray sessionsArray;
    for (int i = 0; i < m_chatSessions.size(); ++i) {
        auto &chat = m_chatSessions[i];
        if (chat.messagesLoaded) {
            chat.messageCount = chat.messages.size();
            saveChatMessages(i);
        }

        QJsonObject sessionObj;
        sessionObj["id"] = chat.id;
        sessionObj["title"] = chat.title;
        sessionObj["pinned"] = chat.pinned;
        sessionObj["scrollPosition"] = chat.scrollPosition;
        sessionObj["messageCount"] = chat.messageCount;
        sessionsArray.append(sessionObj);
    }

    QJsonDocument doc(sessionsArray);
    m_settings.setValue("chatSessions", QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));

    // The backup re-serialises every chat, so coalesce the writes that a single
    // turn produces (send + finish, plus a retry) into one.
    if (!m_backupTimer) {
        m_backupTimer = new QTimer(this);
        m_backupTimer->setSingleShot(true);
        m_backupTimer->setInterval(400);
        connect(m_backupTimer, &QTimer::timeout, this, [this]() { saveChatBackup(); });
    }
    m_backupTimer->start();
}

void MainWindow::loadChatSessions()
{
    QString data = m_settings.value("chatSessions").toString();
    if (data.isEmpty()) return;

    QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
    if (!doc.isArray()) return;

    m_chatSessions.clear();
    bool needsMigration = false;
    for (const auto &val : doc.array()) {
        QJsonObject sessionObj = val.toObject();
        ChatSession chat;
        chat.id = sessionObj["id"].toString();
        chat.title = sessionObj["title"].toString();
        chat.pinned = sessionObj["pinned"].toBool();
        chat.scrollPosition = sessionObj["scrollPosition"].toInt();
        chat.messageCount = sessionObj["messageCount"].toInt();
        chat.messagesLoaded = false;

        QJsonArray messagesArray = sessionObj["messages"].toArray();
        if (!messagesArray.isEmpty()) {
            needsMigration = true;
            for (const auto &msgVal : messagesArray) {
                QJsonObject msgObj = msgVal.toObject();
                chat.messages.append({msgObj["role"].toString(), msgObj["content"].toString(), msgObj["promptTokens"].toInt(), msgObj["completionTokens"].toInt(), msgObj["totalTokens"].toInt(), msgObj["imageUrl"].toString()});
            }
            chat.messageCount = chat.messages.size();
            chat.messagesLoaded = true;
        }
        m_chatSessions.append(chat);
    }

    if (!m_chatSessions.isEmpty()) {
        m_currentChatIndex = 0;
    }

    if (needsMigration) {
        saveChatSessions();
    }
}

void MainWindow::saveChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;

    const auto &chat = m_chatSessions[index];
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

    m_settings.setValue(QString("chatMessages/%1").arg(chat.id), QString::fromUtf8(QJsonDocument(messagesArray).toJson(QJsonDocument::Compact)));
}

void MainWindow::loadChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;

    auto &chat = m_chatSessions[index];
    if (chat.messagesLoaded) return;

    chat.messages.clear();
    QString data = m_settings.value(QString("chatMessages/%1").arg(chat.id)).toString();
    if (!data.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if (doc.isArray()) {
            for (const auto &msgVal : doc.array()) {
                QJsonObject msgObj = msgVal.toObject();
                chat.messages.append({msgObj["role"].toString(), msgObj["content"].toString(), msgObj["promptTokens"].toInt(), msgObj["completionTokens"].toInt(), msgObj["totalTokens"].toInt(), msgObj["imageUrl"].toString()});
            }
        }
    }
    chat.messageCount = chat.messages.size();
    chat.messagesLoaded = true;
}

void MainWindow::unloadChatMessages(int index)
{
    if (index < 0 || index >= m_chatSessions.size()) return;
    auto &chat = m_chatSessions[index];
    if (!chat.messagesLoaded) return;

    chat.messageCount = chat.messages.size();
    chat.messages.clear();
    chat.messagesLoaded = false;
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
    const int renderGeneration = m_chatRenderGeneration;
    m_scrollFollowTimer->stop();
    m_stickToBottom = false;

    // Suppress intermediate repaints/relayouts while the batched render
    // reconstructs the message cards; everything is painted once at the end.
    m_chatContainer->setUpdatesEnabled(false);

    clearChatDisplay(false);

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

    QLayoutItem *item;
    while ((item = m_chatLayout->takeAt(0)) != nullptr) {
        if (item->widget() && item->widget() != m_welcomeWidget) {
            delete item->widget();
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
        "  color: #8e8e8e; "
        "  font-size: 15px; "
        "  font-style: italic; "
        "  padding: 12px 16px; "
        "}"
    );

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
    const QString currentKey = CredentialStore::unprotect(m_settings.value("apiKey").toString());

    bool ok;
    QString apiKey = QInputDialog::getText(this, "Settings", "Venice.ai API Key:",
                                           QLineEdit::Password, currentKey, &ok);
    if (ok) {
        m_settings.setValue("apiKey", CredentialStore::protect(apiKey));
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
    QMessageBox::information(this, "Export Complete", "Backup snapshot exported to:\n" + filePath);
}

void MainWindow::onToggleTheme()
{
    m_isDarkTheme = !m_isDarkTheme;
    m_settings.setValue("darkTheme", m_isDarkTheme);
    applyTheme();
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
    int len = m_inputField->toPlainText().length();
    m_charCounter->setText(len == 0 ? "Start typing" : QString("%1 chars").arg(len));
    m_charCounter->setStyleSheet(QString(
        "QLabel { color: %1; font-size: 11px; font-weight: %2; }")
        .arg(len > 1200 ? "#fbbf24" : "#9ca3af")
        .arg(len > 0 ? "600" : "400"));

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

    QString data = m_settings.value("apiProfiles").toString();
    if (!data.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(data.toUtf8());
        if (doc.isArray()) {
            for (const auto &val : doc.array()) {
                QJsonObject obj = val.toObject();
                ApiProfile profile;
                profile.name = obj["name"].toString();
                profile.apiKey = CredentialStore::unprotect(obj["apiKey"].toString());
                profile.model = obj["model"].toString();
                profile.systemPrompt = obj["systemPrompt"].toString();
                profile.temperature = obj["temperature"].toDouble();
                profile.maxTokens = obj["maxTokens"].toInt();
                m_profiles.append(profile);
            }
        }
    }

    if (m_profiles.isEmpty()) {
        ApiProfile defaultProfile;
        defaultProfile.name = "Default";
        defaultProfile.apiKey = CredentialStore::unprotect(m_settings.value("apiKey").toString());
        defaultProfile.model = m_settings.value("model", "venice-uncensored").toString();
        defaultProfile.systemPrompt = m_settings.value("systemPrompt").toString();
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
        obj["apiKey"] = CredentialStore::protect(profile.apiKey);
        obj["model"] = profile.model;
        obj["systemPrompt"] = profile.systemPrompt;
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
    table->setStyleSheet(
        "QTableWidget { "
        "  background-color: #2f2f2f; "
        "  color: #ececf1; "
        "  gridline-color: #4d4d4f; "
        "  border: 1px solid #4d4d4f; "
        "  font-size: 13px; "
        "} "
        "QHeaderView::section { "
        "  background-color: #1a1a1a; "
        "  color: #ececf1; "
        "  padding: 4px; "
        "  border: 1px solid #4d4d4f; "
        "}"
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
    const QMimeData *mimeData = event->mimeData();
    if (mimeData->hasUrls()) {
        for (const QUrl &url : mimeData->urls()) {
            QString filePath = url.toLocalFile();
            QString lowerPath = filePath.toLower();
            if (lowerPath.endsWith(".png") || lowerPath.endsWith(".jpg") || lowerPath.endsWith(".jpeg") ||
                lowerPath.endsWith(".gif") || lowerPath.endsWith(".bmp") || lowerPath.endsWith(".webp")) {
                QFile file(filePath);
                if (file.open(QIODevice::ReadOnly)) {
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
                break;
            }
        }
    }
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
