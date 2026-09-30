#ifndef PRAYERWIDGET_H
#define PRAYERWIDGET_H

#include <QWidget>

class QLabel;
class QPushButton;

class PrayerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit PrayerWidget(const QString &name, const QString &nameAr,
                          const QString &time = "--:--", QWidget *parent = nullptr);

    void setTime(const QString &time);
    void setAdhanEnabled(bool enabled);
    void setAdhanVisible(bool visible);

signals:
    void adhanToggled(const QString &name, bool enabled);

private:
    void setupUi();

    QString m_name;
    QString m_nameAr;
    QString m_time;
    bool m_adhanEnabled = true;
    bool m_adhanVisible = true;

    QLabel *m_nameLabel;
    QLabel *m_timeLabel;
    QPushButton *m_adhanBtn;
    QWidget *m_spacer;
};

#endif
