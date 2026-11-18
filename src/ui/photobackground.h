#ifndef PHOTOBACKGROUND_H
#define PHOTOBACKGROUND_H

#include <QColor>
#include <QPixmap>
#include <QWidget>

// Paints a photo behind its child widgets, scaled to cover the area.
//  - Overlay: whole widget, rounded corners, dark gradient so white text
//    stays readable (adhan popup).
//  - Hero: top of a window, only top corners rounded, darkened at the top
//    for the header text and faded into `fadeColor` at the bottom.
class PhotoBackground : public QWidget
{
    Q_OBJECT
public:
    enum class Mode { Overlay, Hero };

    PhotoBackground(const QString &resource, Mode mode, QWidget *parent = nullptr);

    void setFadeColor(const QColor &color);   // Hero: window background
    void setRadius(qreal radius);
    void setFocus(qreal x, qreal y);          // 0..1, point kept in view

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QPixmap m_photo;
    Mode m_mode;
    QColor m_fadeColor = QColor("#1b1c1f");
    qreal m_radius = 18;
    qreal m_focusX = 0.5;
    qreal m_focusY = 0.5;
};

#endif
