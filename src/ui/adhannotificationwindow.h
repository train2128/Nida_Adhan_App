#ifndef ADHANNOTIFICATIONWINDOW_H
#define ADHANNOTIFICATIONWINDOW_H

#include <QWidget>

class QLabel;
class QPushButton;

class AdhanNotificationWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AdhanNotificationWindow(QWidget *parent = nullptr);

    void showForPrayer(const QString &prayerName);

signals:
    void dismissed();

private:
    void setupUi();

    QLabel *m_prayerNameLabel;
    QLabel *m_reminderLabel;
    QLabel *m_timeLabel;
    QPushButton *m_closeBtn;
};

#endif
