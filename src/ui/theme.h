#ifndef THEME_H
#define THEME_H

#include <QColor>
#include <QIcon>
#include <QPixmap>
#include <QString>

struct NidaSettings;
class QWidget;

// Central palette + helpers so every window (and every icon) uses the same
// colors in dark and light mode.
struct ThemeColors {
    QColor bg;           // window background
    QColor card;         // raised sections
    QColor line;         // borders / separators
    QColor text;         // primary text
    QColor muted;        // secondary text, idle icons
    QColor gold;         // Hijri date, reminders
    QColor accent;       // next prayer, primary buttons
    QColor accentBg;     // highlighted row / selected item
    QColor accentBorder; // border of highlighted row
    QColor onAccent;     // text on accent-filled buttons
};

namespace Theme {

// Resolves the "dark" / "light" / "system" setting to dark or light.
bool isDark(const NidaSettings &settings);
const ThemeColors &colors(bool dark);

// Full application stylesheet for the given mode.
QString styleSheet(bool dark);
void apply(QWidget *widget, bool dark);

// Stroke icons (Tabler style) rendered from embedded SVG in any color.
// Names: bell, bell-off, sun, settings, refresh, map-pin, search, location,
// play, stop, circle, circle-check, moon, volume, clock, adjustments, x, folder
QPixmap pixmap(const QString &name, const QColor &color, int size = 18);
QIcon icon(const QString &name, const QColor &color, int size = 18);

// Re-evaluate [property] selectors after setProperty() on a styled widget.
void repolish(QWidget *widget);

} // namespace Theme

#endif
