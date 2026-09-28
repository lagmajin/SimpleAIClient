#ifndef APPICONS_H
#define APPICONS_H

#include <QColor>
#include <QIcon>

// Line icons drawn with QPainter rather than shipped as assets, so they stay
// crisp at any size and follow the active theme.
//
// The glyph palette was authored against the dark background; makeLineIcon()
// adapts any colour that would not clear a 3:1 contrast ratio so the icons
// stay legible in the light theme too.
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

QIcon makeLineIcon(AppIconGlyph glyph, int size = 24, const QColor &accent = QColor("#007acc"));

#endif // APPICONS_H
