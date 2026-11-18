#include "photobackground.h"
#include <QLinearGradient>
#include <QPainter>
#include <QPainterPath>

PhotoBackground::PhotoBackground(const QString &resource, Mode mode, QWidget *parent)
    : QWidget(parent)
    , m_photo(resource)
    , m_mode(mode)
{
    setAttribute(Qt::WA_TranslucentBackground);
}

void PhotoBackground::setFadeColor(const QColor &color)
{
    m_fadeColor = color;
    update();
}

void PhotoBackground::setRadius(qreal radius)
{
    m_radius = radius;
    update();
}

void PhotoBackground::setFocus(qreal x, qreal y)
{
    m_focusX = qBound(0.0, x, 1.0);
    m_focusY = qBound(0.0, y, 1.0);
    update();
}

void PhotoBackground::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);
    const QRectF r = rect();

    QPainterPath clip;
    if (m_mode == Mode::Hero) {
        // Rounded top corners only: extend a rounded rect below the bottom.
        clip.addRoundedRect(r.adjusted(0, 0, 0, m_radius), m_radius, m_radius);
        QPainterPath box;
        box.addRect(r);
        clip = clip.intersected(box);
    } else {
        clip.addRoundedRect(r, m_radius, m_radius);
    }
    p.setClipPath(clip);

    // "Cover" scaling around the focus point.
    if (!m_photo.isNull()) {
        const QSizeF src = m_photo.size();
        const qreal scale = qMax(r.width() / src.width(), r.height() / src.height());
        const QSizeF shown(r.width() / scale, r.height() / scale);
        const qreal sx = qBound(0.0, m_focusX * src.width() - shown.width() / 2,
                                src.width() - shown.width());
        const qreal sy = qBound(0.0, m_focusY * src.height() - shown.height() / 2,
                                src.height() - shown.height());
        p.drawPixmap(r, m_photo, QRectF(QPointF(sx, sy), shown));
    } else {
        p.fillRect(r, m_fadeColor);
    }

    QLinearGradient g(r.topLeft(), r.bottomLeft());
    if (m_mode == Mode::Overlay) {
        g.setColorAt(0.0, QColor(10, 10, 12, 140));
        g.setColorAt(0.40, QColor(10, 10, 12, 190));
        g.setColorAt(1.0, QColor(10, 10, 12, 240));
        p.fillRect(r, g);
        p.setClipping(false);
        p.setPen(QPen(QColor(255, 255, 255, 40), 1));
        p.setBrush(Qt::NoBrush);
        p.drawRoundedRect(r.adjusted(0.5, 0.5, -0.5, -0.5), m_radius, m_radius);
    } else {
        QColor clear = m_fadeColor;
        clear.setAlpha(0);
        QColor mid = m_fadeColor;
        mid.setAlpha(110);
        g.setColorAt(0.0, QColor(0, 0, 0, 115));
        g.setColorAt(0.35, QColor(0, 0, 0, 40));
        g.setColorAt(0.55, clear);
        g.setColorAt(0.78, mid);
        g.setColorAt(1.0, m_fadeColor);
        p.fillRect(r, g);
    }
}
