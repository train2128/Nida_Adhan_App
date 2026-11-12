#include "theme.h"
#include "models/prayertimes.h"
#include <QApplication>
#include <QFile>
#include <QGuiApplication>
#include <QHash>
#include <QPainter>
#include <QStyle>
#include <QStyleHints>
#include <QSvgRenderer>
#include <QWidget>

namespace {

const ThemeColors kDark = {
    QColor("#141428"), QColor("#1c1c36"), QColor("#2a2f52"), QColor("#e6e8f0"),
    QColor("#8a93b0"), QColor("#d4b25c"), QColor("#4fc3f7"), QColor("#123a52"),
    QColor("#1f5f86"), QColor("#06283a"),
};

const ThemeColors kLight = {
    QColor("#f7f8fc"), QColor("#eceff7"), QColor("#d6dae8"), QColor("#1d2033"),
    QColor("#646c8a"), QColor("#9a7616"), QColor("#0b7fc0"), QColor("#dcefff"),
    QColor("#9fcdf0"), QColor("#ffffff"),
};

// 24x24 stroke paths; COLOR is replaced at render time.
const QHash<QString, QString> &iconBodies()
{
    static const QHash<QString, QString> icons = {
        {"bell", "<path d='M10 5a2 2 0 1 1 4 0a7 7 0 0 1 4 6v3a4 4 0 0 0 2 3h-16a4 4 0 0 0 2 -3v-3a7 7 0 0 1 4 -6'/><path d='M9 17v1a3 3 0 0 0 6 0v-1'/>"},
        {"bell-off", "<path d='M10 5a2 2 0 1 1 4 0a7 7 0 0 1 4 6v3a4 4 0 0 0 2 3h-16a4 4 0 0 0 2 -3v-3a7 7 0 0 1 4 -6'/><path d='M9 17v1a3 3 0 0 0 6 0v-1'/><path d='M3 3l18 18'/>"},
        {"sun", "<circle cx='12' cy='12' r='4'/><path d='M3 12h1m8 -9v1m8 8h1m-9 8v1m-6.4 -15.4l.7 .7m12.1 -.7l-.7 .7m0 11.4l.7 .7m-12.1 -.7l-.7 .7'/>"},
        {"settings", "<path d='M10.325 4.317c.426 -1.756 2.924 -1.756 3.35 0a1.724 1.724 0 0 0 2.573 1.066c1.543 -.94 3.31 .826 2.37 2.37a1.724 1.724 0 0 0 1.065 2.572c1.756 .426 1.756 2.924 0 3.35a1.724 1.724 0 0 0 -1.066 2.573c.94 1.543 -.826 3.31 -2.37 2.37a1.724 1.724 0 0 0 -2.572 1.065c-.426 1.756 -2.924 1.756 -3.35 0a1.724 1.724 0 0 0 -2.573 -1.066c-1.543 .94 -3.31 -.826 -2.37 -2.37a1.724 1.724 0 0 0 -1.065 -2.572c-1.756 -.426 -1.756 -2.924 0 -3.35a1.724 1.724 0 0 0 1.066 -2.573c-.94 -1.543 .826 -3.31 2.37 -2.37c1 .608 2.296 .07 2.572 -1.065z'/><circle cx='12' cy='12' r='3'/>"},
        {"refresh", "<path d='M20 11a8.1 8.1 0 0 0 -15.5 -2m-.5 -4v4h4'/><path d='M4 13a8.1 8.1 0 0 0 15.5 2m.5 4v-4h-4'/>"},
        {"map-pin", "<circle cx='12' cy='11' r='3'/><path d='M17.657 16.657l-4.243 4.243a2 2 0 0 1 -2.827 0l-4.244 -4.243a8 8 0 1 1 11.314 0z'/>"},
        {"search", "<circle cx='10' cy='10' r='7'/><path d='M21 21l-6 -6'/>"},
        {"location", "<circle cx='12' cy='12' r='3'/><circle cx='12' cy='12' r='8'/><path d='M12 2v2m0 16v2m-10 -10h2m16 0h2'/>"},
        {"play", "<path d='M7 4v16l13 -8z'/>"},
        {"stop", "<rect x='5' y='5' width='14' height='14' rx='2'/>"},
        {"circle", "<circle cx='12' cy='12' r='9'/>"},
        {"circle-check", "<circle cx='12' cy='12' r='9'/><path d='M9 12l2 2l4 -4'/>"},
        {"moon", "<path d='M12 3c.132 0 .263 0 .393 0a7.5 7.5 0 0 0 7.92 12.446a9 9 0 1 1 -8.313 -12.454z'/>"},
        {"volume", "<path d='M15 8a5 5 0 0 1 0 8'/><path d='M17.7 5a9 9 0 0 1 0 14'/><path d='M6 15h-2a1 1 0 0 1 -1 -1v-4a1 1 0 0 1 1 -1h2l3.5 -4.5a.8 .8 0 0 1 1.5 .5v14a.8 .8 0 0 1 -1.5 .5l-3.5 -4.5'/>"},
        {"clock", "<circle cx='12' cy='12' r='9'/><path d='M12 7v5l3 3'/>"},
        {"adjustments", "<circle cx='14' cy='6' r='2'/><path d='M4 6h8m4 0h4'/><circle cx='8' cy='12' r='2'/><path d='M4 12h2m4 0h10'/><circle cx='17' cy='18' r='2'/><path d='M4 18h11m4 0h1'/>"},
        {"x", "<path d='M18 6l-12 12'/><path d='M6 6l12 12'/>"},
        {"folder", "<path d='M5 4h4l3 3h7a2 2 0 0 1 2 2v8a2 2 0 0 1 -2 2h-14a2 2 0 0 1 -2 -2v-11a2 2 0 0 1 2 -2'/>"},
    };
    return icons;
}

} // namespace

namespace Theme {

bool isDark(const NidaSettings &settings)
{
    if (settings.theme == QLatin1String("light"))
        return false;
    if (settings.theme == QLatin1String("system")) {
#if QT_VERSION >= QT_VERSION_CHECK(6, 5, 0)
        const Qt::ColorScheme scheme = QGuiApplication::styleHints()->colorScheme();
        if (scheme != Qt::ColorScheme::Unknown)
            return scheme == Qt::ColorScheme::Dark;
#endif
        return true;
    }
    return true;
}

const ThemeColors &colors(bool dark)
{
    return dark ? kDark : kLight;
}

QString styleSheet(bool dark)
{
    // One template, colors filled in from the palette: dark and light can
    // never drift apart.
    QFile f(QStringLiteral(":/styles/nida"));
    if (!f.open(QFile::ReadOnly))
        return {};
    QString qss = QString::fromUtf8(f.readAll());
    qss.replace(QStringLiteral("@mode"), dark ? QStringLiteral("dark") : QStringLiteral("light"));
    const ThemeColors &c = colors(dark);
    const QList<QPair<QString, QColor>> tokens = {
        {"@accentBorder", c.accentBorder}, {"@accentBg", c.accentBg},
        {"@onAccent", c.onAccent}, {"@accent", c.accent}, {"@card", c.card},
        {"@line", c.line}, {"@text", c.text}, {"@muted", c.muted},
        {"@gold", c.gold}, {"@bg", c.bg},
    };
    for (const auto &t : tokens)
        qss.replace(t.first, t.second.name());
    // Subtle hover wash that works on both backgrounds.
    qss.replace(QStringLiteral("@hover"),
                dark ? QStringLiteral("rgba(255,255,255,0.06)")
                     : QStringLiteral("rgba(0,0,0,0.05)"));
    return qss;
}

void apply(QWidget *widget, bool dark)
{
    if (!widget)
        return;
    widget->setStyleSheet(styleSheet(dark));
    // Switching sheets on an already-polished widget can leave children with
    // the old look; repolish the whole tree.
    repolish(widget);
}

QPixmap pixmap(const QString &name, const QColor &color, int size)
{
    const QString body = iconBodies().value(name);
    const qreal dpr = qApp ? qApp->devicePixelRatio() : 1.0;
    QPixmap pm(QSize(size, size) * dpr);
    pm.fill(Qt::transparent);
    pm.setDevicePixelRatio(dpr);
    if (body.isEmpty())
        return pm;
    const QString svg = QStringLiteral(
        "<svg xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24' fill='none' "
        "stroke='%1' stroke-width='2' stroke-linecap='round' stroke-linejoin='round'>%2</svg>")
        .arg(color.name(), body);
    QSvgRenderer renderer(svg.toUtf8());
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    renderer.render(&p, QRectF(0, 0, size, size));
    return pm;
}

QIcon icon(const QString &name, const QColor &color, int size)
{
    return QIcon(pixmap(name, color, size));
}

void repolish(QWidget *widget)
{
    if (!widget)
        return;
    widget->style()->unpolish(widget);
    widget->style()->polish(widget);
    const auto children = widget->findChildren<QWidget *>();
    for (QWidget *child : children) {
        child->style()->unpolish(child);
        child->style()->polish(child);
    }
    widget->update();
}

} // namespace Theme
