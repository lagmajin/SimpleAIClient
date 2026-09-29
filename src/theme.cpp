#include "theme.h"

#include <QApplication>
#include <QHash>
#include <QPalette>
#include <QSet>
#include <QStyleFactory>
#include <QWidget>

static ThemeController *g_themeController = nullptr;

Theme Theme::dark()
{
    Theme t;
    t.window = "#111418";
    t.sidebar = "#151a1f";
    t.surface = "#16181c";
    t.surfaceAlt = "#1f242b";
    t.surfaceSunken = "#1a1a1a";
    t.inputBg = "#101418";
    t.chipBg = "#262c34";
    t.chipBorder = "#334155";

    t.border = "#2b3139";
    t.borderStrong = "#2f3742";

    t.textStrong = "#ececf1";
    t.textMuted = "#9aa3b0";
    t.textFaint = "#7d8794";

    t.accent = "#005c4b";
    t.accentHover = "#007a5e";
    t.accentPressed = "#004d3f";
    t.accentText = "#ffffff";

    t.userBubble = "#14532d";
    t.userBubbleBorder = "#15803d";
    t.userBubbleText = "#d1fae5";
    t.assistantBubble = "#20242b";
    t.assistantBubbleBorder = "#2f3742";
    t.errorBubble = "#4a1515";

    t.danger = "#f87171";
    t.success = "#4ade80";
    t.warning = "#fbbf24";
    t.link = "#60a5fa";
    t.selection = "#1d4ed8";

    t.codeBg = "#1a1a1a";
    t.codeText = "#e6e6e6";
    t.codeHeaderBg = "#1a1a1a";
    t.codeLanguageText = "#90a4ae";

    t.scrollbar = "#404040";
    t.scrollbarHover = "#505050";
    t.isDarkTheme = true;
    return t;
}

Theme Theme::light()
{
    Theme t;
    t.window = "#ffffff";
    t.sidebar = "#f4f5f7";
    t.surface = "#ffffff";
    t.surfaceAlt = "#eef0f3";
    t.surfaceSunken = "#f6f7f9";
    t.inputBg = "#ffffff";
    t.chipBg = "#ffffff";
    t.chipBorder = "#c9ced6";

    t.border = "#d5dae0";
    t.borderStrong = "#b9c0c9";

    t.textStrong = "#1b1f24";
    t.textMuted = "#4d5563";
    t.textFaint = "#6b7280";

    // Kept dark enough to hold white text at AA on both themes.
    t.accent = "#0b6b52";
    t.accentHover = "#0d8062";
    t.accentPressed = "#085441";
    t.accentText = "#ffffff";

    t.userBubble = "#dff3ea";
    t.userBubbleBorder = "#a8d9c4";
    t.userBubbleText = "#14532d";
    t.assistantBubble = "#ffffff";
    t.assistantBubbleBorder = "#d5dae0";
    t.errorBubble = "#fde8e8";

    t.danger = "#c62828";
    t.success = "#1b7f4b";
    t.warning = "#a06000";
    t.link = "#1d4ed8";
    t.selection = "#2563eb";

    t.codeBg = "#f6f7f9";
    t.codeText = "#1f2328";
    t.codeHeaderBg = "#eceef1";
    t.codeLanguageText = "#5b6472";

    t.scrollbar = "#c3c8cf";
    t.scrollbarHover = "#a8aeb6";
    t.isDarkTheme = false;
    return t;
}

ThemeController::ThemeController(QObject *parent)
    : QObject(parent)
    , m_isDark(true)
    , m_tokens(Theme::dark())
    , m_builders(new QHash<QObject *, std::function<QString()>>())
    , m_watched(new QSet<QObject *>())
{
    g_themeController = this;
}

ThemeController::~ThemeController()
{
    if (g_themeController == this) {
        g_themeController = nullptr;
    }
    delete m_watched;
    delete m_builders;
}

void ThemeController::setDark(bool dark)
{
    if (m_isDark == dark) {
        return;
    }
    m_isDark = dark;
    m_tokens = dark ? Theme::dark() : Theme::light();
    applyPalette();
    refreshAll();
}

void ThemeController::applyPalette()
{
    const Theme &t = m_tokens;
    qApp->setStyle(QStyleFactory::create("Fusion"));

    QPalette palette;
    const QColor window(t.window);
    const QColor base(t.inputBg);
    const QColor surface(t.surfaceAlt);
    const QColor text(t.textStrong);
    const QColor muted(t.textMuted);

    palette.setColor(QPalette::Window, window);
    palette.setColor(QPalette::WindowText, text);
    palette.setColor(QPalette::Base, base);
    palette.setColor(QPalette::AlternateBase, surface);
    palette.setColor(QPalette::ToolTipBase, surface);
    palette.setColor(QPalette::ToolTipText, text);
    palette.setColor(QPalette::Text, text);
    palette.setColor(QPalette::Button, surface);
    palette.setColor(QPalette::ButtonText, text);
    palette.setColor(QPalette::BrightText, t.danger);
    palette.setColor(QPalette::Light, window.lighter(105));
    palette.setColor(QPalette::Midlight, window.lighter(110));
    palette.setColor(QPalette::Mid, window.darker(110));
    palette.setColor(QPalette::Dark, window.darker(150));
    palette.setColor(QPalette::Shadow, t.textFaint);

    palette.setColor(QPalette::Link, QColor(t.link));
    palette.setColor(QPalette::LinkVisited, QColor(t.link).darker(115));
    palette.setColor(QPalette::Highlight, QColor(t.accent));
    palette.setColor(QPalette::HighlightedText, QColor(t.accentText));
    palette.setColor(QPalette::PlaceholderText, muted);

    // Disabled text needs to stay legible in both themes.
    const QColor disabled(t.textFaint);
    palette.setColor(QPalette::Disabled, QPalette::WindowText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::Text, disabled);
    palette.setColor(QPalette::Disabled, QPalette::ButtonText, disabled);
    palette.setColor(QPalette::Disabled, QPalette::HighlightedText, disabled);

    qApp->setPalette(palette);
}

void ThemeController::registerStyle(QWidget *widget, const std::function<QString()> &builder)
{
    if (!widget) {
        return;
    }
    widget->setStyleSheet(builder());
    m_builders->insert(widget, builder);
    // Drop the builder if the widget goes away so a long session with chat
    // rebuilds does not accumulate entries. Re-registering a widget replaces
    // the stored builder, so the cleanup connection is only made once.
    if (!m_watched->isEmpty() && m_watched->contains(widget)) {
        return;
    }
    m_watched->insert(widget);
    connect(widget, &QObject::destroyed, this, [this, widget]() {
        m_builders->remove(widget);
        if (!m_watched->isEmpty()) {
            m_watched->remove(widget);
        }
    });
}

QString scrollBarStyle()
{
    const Theme &t = currentTheme();
    return
        "QScrollBar:vertical { "
        "  background-color: " + t.surface + "; "
        "  width: 10px; "
        "  border-radius: 5px; "
        "  margin: 0px; "
        "} "
        "QScrollBar::handle:vertical { "
        "  background-color: " + t.scrollbar + "; "
        "  border-radius: 5px; "
        "  min-height: 30px; "
        "} "
        "QScrollBar::handle:vertical:hover { "
        "  background-color: " + t.scrollbarHover + "; "
        "} "
        "QScrollBar:horizontal { "
        "  background-color: " + t.surface + "; "
        "  height: 10px; "
        "  border-radius: 5px; "
        "  margin: 0px; "
        "} "
        "QScrollBar::handle:horizontal { "
        "  background-color: " + t.scrollbar + "; "
        "  border-radius: 5px; "
        "  min-width: 30px; "
        "} "
        "QScrollBar::add-line, QScrollBar::sub-line, QScrollBar::add-page, QScrollBar::sub-page { "
        "  height: 0px; width: 0px; "
        "  background: none; "
        "}";
}

void ThemeController::refreshAll()
{
    const auto builders = *m_builders;
    for (auto it = builders.constBegin(); it != builders.constEnd(); ++it) {
        QWidget *widget = qobject_cast<QWidget *>(it.key());
        if (!widget) {
            continue;
        }
        widget->setStyleSheet(it.value()());
    }
}

ThemeController *themeController()
{
    return g_themeController;
}

const Theme &currentTheme()
{
    static const Theme fallback = Theme::dark();
    return g_themeController ? g_themeController->tokens() : fallback;
}
