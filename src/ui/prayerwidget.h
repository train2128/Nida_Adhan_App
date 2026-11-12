#ifndef PRAYERWIDGET_H
#define PRAYERWIDGET_H

#include <QWidget>

class QLabel;
class QPushButton;

// One row of the main window: "Asr  العصر ........ 16:04  [bell]".
class PrayerWidget : public QWidget
{
    Q_OBJECT
public:
    enum class State { Upcoming, Passed, Next };

    explicit PrayerWidget(const QString &name, const QString &nameAr,
                          QWidget *parent = nullptr);

    QString name() const { return m_name; }
    void setTime(const QString &time);
    void setAdhanEnabled(bool enabled);
    void setAdhanVisible(bool visible); // false for Sunrise (sun icon instead)
    void setState(State state);
    void setDark(bool dark);

signals:
    void adhanToggled(const QString &name, bool enabled);

private:
    void updateIcon();

    QString m_name;
    bool m_adhanEnabled = true;
    bool m_adhanVisible = true;
    bool m_dark = true;
    State m_state = State::Upcoming;

    QLabel *m_nameLabel;
    QLabel *m_nameArLabel;
    QLabel *m_timeLabel;
    QPushButton *m_adhanBtn;
};

#endif
