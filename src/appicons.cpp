#include "appicons.h"
#include "theme.h"

#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QtMath>

#include <cmath>

namespace {

double relativeLuminance(const QColor &c)
{
    auto ch = [](double v) {
        v /= 255.0;
        return v <= 0.03928 ? v / 12.92 : std::pow((v + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * ch(c.red()) + 0.7152 * ch(c.green()) + 0.0722 * ch(c.blue());
}

double contrastRatio(const QColor &a, const QColor &b)
{
    const double la = relativeLuminance(a);
    const double lb = relativeLuminance(b);
    return (qMax(la, lb) + 0.05) / (qMin(la, lb) + 0.05);
}

// The icon palette was authored for the dark background, so several glyphs are
// near-white and would disappear on light. Nudge a colour towards the opposite
// end until it clears a 3:1 ratio against both panel surfaces, keeping its hue.
QColor adaptIconColor(const QColor &color)
{
    const Theme &t = currentTheme();
    const QColor backgrounds[] = {QColor(t.surface), QColor(t.window)};

    const auto clears = [&backgrounds](const QColor &c) {
        for (const QColor &bg : backgrounds) {
            if (contrastRatio(c, bg) < 3.0) {
                return false;
            }
        }
        return true;
    };

    if (clears(color)) {
        return color;
    }

    QColor adjusted = color;
    for (int i = 0; i < 24 && !clears(adjusted); ++i) {
        adjusted = t.isDarkTheme ? adjusted.lighter(110) : adjusted.darker(110);
    }
    return adjusted;
}

} // namespace

QIcon makeLineIcon(AppIconGlyph glyph, int size, const QColor &accent)
{
    QPixmap pixmap(size, size);
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);

    const qreal scale = size / 24.0;
    auto sx = [scale](qreal value) { return value * scale; };
    const auto ink = [](const QColor &c) { return adaptIconColor(c); };

    QPen pen(ink(accent), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    QPen softPen(ink(QColor("#8ab4f8")), sx(1.5), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
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
        painter.setBrush(ink(QColor("#007acc")));
        painter.setPen(Qt::NoPen);
        painter.drawPath(mark);
        painter.setPen(QPen(ink(QColor("#e8f3ff")), sx(1.6), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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
        painter.setPen(QPen(ink(QColor("#42a5f5")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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
        painter.setBrush(ink(QColor("#ef4444")));
        painter.setPen(Qt::NoPen);
        painter.drawRoundedRect(QRectF(sx(7), sx(7), sx(10), sx(10)), sx(2), sx(2));
        break;
    case AppIconGlyph::Chat:
        painter.setPen(QPen(ink(QColor("#4fc3f7")), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(12)), sx(3), sx(3));
        painter.drawLine(QPointF(sx(8), sx(20)), QPointF(sx(11), sx(17)));
        painter.drawLine(QPointF(sx(8), sx(9.5)), QPointF(sx(16), sx(9.5)));
        painter.drawLine(QPointF(sx(8), sx(13)), QPointF(sx(14), sx(13)));
        break;
    case AppIconGlyph::Trash:
        painter.setPen(QPen(ink(QColor("#ef5350")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(7)), QPointF(sx(17), sx(7)));
        painter.drawLine(QPointF(sx(10), sx(5)), QPointF(sx(14), sx(5)));
        painter.drawRoundedRect(QRectF(sx(8), sx(8), sx(8), sx(11)), sx(1.5), sx(1.5));
        painter.drawLine(QPointF(sx(10.5), sx(10.5)), QPointF(sx(10.5), sx(16.5)));
        painter.drawLine(QPointF(sx(13.5), sx(10.5)), QPointF(sx(13.5), sx(16.5)));
        break;
    case AppIconGlyph::Search:
        painter.setPen(QPen(ink(QColor("#9ca3af")), sx(1.9), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(10.5), sx(10.5)), sx(5), sx(5));
        painter.drawLine(QPointF(sx(14.5), sx(14.5)), QPointF(sx(19), sx(19)));
        break;
    case AppIconGlyph::ChevronUp:
        painter.setPen(QPen(ink(QColor("#cbd5e1")), sx(2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(14)), QPointF(sx(12), sx(9)));
        painter.drawLine(QPointF(sx(12), sx(9)), QPointF(sx(17), sx(14)));
        break;
    case AppIconGlyph::ChevronDown:
        painter.setPen(QPen(ink(QColor("#cbd5e1")), sx(2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(10)), QPointF(sx(12), sx(15)));
        painter.drawLine(QPointF(sx(12), sx(15)), QPointF(sx(17), sx(10)));
        break;
    case AppIconGlyph::Close:
    case AppIconGlyph::Cancel:
        painter.setPen(QPen(ink(QColor("#ef5350")), sx(2), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(8), sx(8)), QPointF(sx(16), sx(16)));
        painter.drawLine(QPointF(sx(16), sx(8)), QPointF(sx(8), sx(16)));
        break;
    case AppIconGlyph::Copy:
        painter.setPen(QPen(ink(QColor("#90caf9")), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(8), sx(7), sx(10), sx(12)), sx(1.8), sx(1.8));
        painter.drawRoundedRect(QRectF(sx(5), sx(4), sx(10), sx(12)), sx(1.8), sx(1.8));
        break;
    case AppIconGlyph::Refresh:
        painter.setPen(QPen(ink(QColor("#81c784")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawArc(QRectF(sx(5), sx(5), sx(14), sx(14)), 35 * 16, 250 * 16);
        painter.drawLine(QPointF(sx(16.8), sx(5.6)), QPointF(sx(18.8), sx(5.8)));
        painter.drawLine(QPointF(sx(18.8), sx(5.8)), QPointF(sx(18.1), sx(8.2)));
        break;
    case AppIconGlyph::Branch:
        painter.setPen(QPen(ink(QColor("#ce93d8")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(8), sx(6)), QPointF(sx(8), sx(18)));
        painter.drawLine(QPointF(sx(8), sx(12)), QPointF(sx(16), sx(8)));
        painter.drawEllipse(QPointF(sx(8), sx(6)), sx(2), sx(2));
        painter.drawEllipse(QPointF(sx(8), sx(18)), sx(2), sx(2));
        painter.drawEllipse(QPointF(sx(16), sx(8)), sx(2), sx(2));
        break;
    case AppIconGlyph::Edit:
        painter.setPen(QPen(ink(QColor("#ffb74d")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(7), sx(17)), QPointF(sx(16.5), sx(7.5)));
        painter.drawLine(QPointF(sx(14), sx(5)), QPointF(sx(19), sx(10)));
        painter.drawLine(QPointF(sx(6), sx(18)), QPointF(sx(10), sx(17)));
        break;
    case AppIconGlyph::Save:
        painter.setPen(QPen(ink(QColor("#4ade80")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(5), sx(4), sx(14), sx(16)), sx(2), sx(2));
        painter.drawLine(QPointF(sx(8), sx(4)), QPointF(sx(8), sx(9)));
        painter.drawLine(QPointF(sx(16), sx(4)), QPointF(sx(16), sx(9)));
        painter.drawRoundedRect(QRectF(sx(8), sx(13), sx(8), sx(5)), sx(1), sx(1));
        break;
    case AppIconGlyph::Image:
        painter.setPen(QPen(ink(QColor("#64b5f6")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(14)), sx(2), sx(2));
        painter.drawEllipse(QPointF(sx(15.5), sx(9)), sx(1.5), sx(1.5));
        painter.drawLine(QPointF(sx(6.5), sx(16.5)), QPointF(sx(10), sx(12.5)));
        painter.drawLine(QPointF(sx(10), sx(12.5)), QPointF(sx(13), sx(15)));
        painter.drawLine(QPointF(sx(13), sx(15)), QPointF(sx(16), sx(11.5)));
        painter.drawLine(QPointF(sx(16), sx(11.5)), QPointF(sx(19), sx(16.5)));
        break;
    case AppIconGlyph::Code:
        painter.setPen(QPen(ink(QColor("#4dd0e1")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(sx(9), sx(8)), QPointF(sx(5), sx(12)));
        painter.drawLine(QPointF(sx(5), sx(12)), QPointF(sx(9), sx(16)));
        painter.drawLine(QPointF(sx(15), sx(8)), QPointF(sx(19), sx(12)));
        painter.drawLine(QPointF(sx(19), sx(12)), QPointF(sx(15), sx(16)));
        painter.drawLine(QPointF(sx(13), sx(7)), QPointF(sx(11), sx(17)));
        break;
    case AppIconGlyph::Key:
        painter.setPen(QPen(ink(QColor("#ffd54f")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(8), sx(10)), sx(3), sx(3));
        painter.drawLine(QPointF(sx(11), sx(10)), QPointF(sx(20), sx(10)));
        painter.drawLine(QPointF(sx(16), sx(10)), QPointF(sx(16), sx(13)));
        painter.drawLine(QPointF(sx(19), sx(10)), QPointF(sx(19), sx(12)));
        break;
    case AppIconGlyph::Model:
        painter.setPen(QPen(ink(QColor("#26c6da")), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(5), sx(5), sx(14), sx(14)), sx(3), sx(3));
        painter.drawLine(QPointF(sx(9), sx(9)), QPointF(sx(15), sx(9)));
        painter.drawLine(QPointF(sx(9), sx(12)), QPointF(sx(15), sx(12)));
        painter.drawLine(QPointF(sx(9), sx(15)), QPointF(sx(13), sx(15)));
        break;
    case AppIconGlyph::Pin:
    case AppIconGlyph::Unpin:
        painter.setPen(QPen(ink(QColor("#f06292")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
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
        painter.setPen(QPen(ink(QColor("#ffb300")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawPolygon(QPolygonF{QPointF(sx(12), sx(4)), QPointF(sx(21), sx(19)), QPointF(sx(3), sx(19))});
        painter.drawLine(QPointF(sx(12), sx(9)), QPointF(sx(12), sx(14)));
        painter.drawPoint(QPointF(sx(12), sx(16.5)));
        break;
    case AppIconGlyph::Info:
        painter.setPen(QPen(ink(QColor("#42a5f5")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(12), sx(12)), sx(8), sx(8));
        painter.drawLine(QPointF(sx(12), sx(11)), QPointF(sx(12), sx(16)));
        painter.drawPoint(QPointF(sx(12), sx(8)));
        break;
    case AppIconGlyph::Database:
        painter.setPen(QPen(ink(QColor("#fdd835")), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(12), sx(6.5)), sx(6.5), sx(2.5));
        painter.drawLine(QPointF(sx(5.5), sx(6.5)), QPointF(sx(5.5), sx(17)));
        painter.drawLine(QPointF(sx(18.5), sx(6.5)), QPointF(sx(18.5), sx(17)));
        painter.drawEllipse(QPointF(sx(12), sx(17)), sx(6.5), sx(2.5));
        painter.drawArc(QRectF(sx(5.5), sx(9.5), sx(13), sx(5)), 180 * 16, 180 * 16);
        break;
    case AppIconGlyph::Terminal:
        painter.setPen(QPen(ink(QColor("#90a4ae")), sx(1.8), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawRoundedRect(QRectF(sx(4), sx(5), sx(16), sx(14)), sx(2), sx(2));
        painter.drawLine(QPointF(sx(7), sx(9)), QPointF(sx(10), sx(12)));
        painter.drawLine(QPointF(sx(10), sx(12)), QPointF(sx(7), sx(15)));
        painter.drawLine(QPointF(sx(12), sx(15)), QPointF(sx(17), sx(15)));
        break;
    case AppIconGlyph::Globe:
        painter.setPen(QPen(ink(QColor("#29b6f6")), sx(1.7), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(sx(12), sx(12)), sx(8), sx(8));
        painter.drawLine(QPointF(sx(4), sx(12)), QPointF(sx(20), sx(12)));
        painter.drawArc(QRectF(sx(8), sx(4), sx(8), sx(16)), 90 * 16, 180 * 16);
        painter.drawArc(QRectF(sx(8), sx(4), sx(8), sx(16)), -90 * 16, 180 * 16);
        break;
    }

    return QIcon(pixmap);
}