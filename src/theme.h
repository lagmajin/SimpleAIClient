#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QObject>
#include <QSet>
#include <QString>
#include <QWidget>

#include <functional>
#include <memory>

// Semantic colour tokens for the whole UI.
//
// Every stylesheet used to be written with literal hex values, which is why the
// light theme only changed the QPalette and left dark-on-dark widgets behind.
// Widgets now build their stylesheet from these tokens and register a builder
// so a theme switch can regenerate them.
struct Theme
{
    // Surfaces
    QString window;          // main chat panel background
    QString sidebar;         // chat list panel background
    QString surface;         // cards, panels
    QString surfaceAlt;      // inputs, hovered rows
    QString surfaceSunken;   // code blocks
    QString inputBg;
    QString chipBg;          // filter toggle background
    QString chipBorder;

    // Lines
    QString border;
    QString borderStrong;

    // Text
    QString textStrong;
    QString textMuted;
    QString textFaint;

    // Accent
    QString accent;
    QString accentHover;
    QString accentPressed;
    QString accentText;

    // Message bubbles
    QString userBubble;
    QString userBubbleBorder;
    QString userBubbleText;
    QString assistantBubble;
    QString assistantBubbleBorder;
    QString errorBubble;

    // Semantics
    QString danger;
    QString success;
    QString warning;
    QString link;
    QString selection;

    // Code
    QString codeBg;
    QString codeText;
    QString codeHeaderBg;
    QString codeLanguageText;

    // Scrollbars
    QString scrollbar;
    QString scrollbarHover;

    bool isDarkTheme = true;

    static Theme dark();
    static Theme light();
};

// Re-applies registered widget stylesheets when the theme changes.
class ThemeController : public QObject
{
    Q_OBJECT

public:
    explicit ThemeController(QObject *parent = nullptr);
    ~ThemeController() override;

    bool isDark() const { return m_isDark; }
    const Theme &tokens() const { return m_tokens; }

    void setDark(bool dark);
    void applyPalette();
    void registerStyle(QWidget *widget, const std::function<QString()> &builder);
    void refreshAll();

private:
    bool m_isDark;
    Theme m_tokens;
    class QHash<QObject *, std::function<QString()>> *m_builders;
    // Widgets that already have a destroyed() cleanup connection, so
    // re-registering one (the char counter does, on every keystroke) does not
    // add another.
    QSet<QObject *> *m_watched;
};

// Process-wide theme controller, owned by MainWindow.
ThemeController *themeController();
const Theme &currentTheme();

// Scrollbar styling shared by the composer, the code blocks and both scroll
// areas.
QString scrollBarStyle();

#endif // THEME_H
