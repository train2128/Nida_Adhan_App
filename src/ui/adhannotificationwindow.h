#ifndef ADHANNOTIFICATIONWINDOW_H
#define ADHANNOTIFICATIONWINDOW_H

#include <QTime>
#include <QWidget>

class QLabel;
class QPushButton;
class QProgressBar;

// Popup shown at prayer time: Arabic prayer name, reminder, adhan progress,
// "Stop adhan" (stops the sound) and "Close" (hides the popup only).
class AdhanNotificationWindow : public QWidget
{
    Q_OBJECT
public:
    explicit AdhanNotificationWindow(QWidget *parent = nullptr);

    // prayerName "TEST" = test adhan. withAdhan=false for a silent reminder.
    void showForPrayer(const QString &prayerName, const QTime &prayerTime,
                       bool withAdhan, bool mediaPaused);
    void setProgress(qint64 positionMs, qint64 durationMs);
    void setAdhanFinished(); // audio ended or was stopped
    void setDark(bool dark);

signals:
    void stopRequested();
    void closed();

private:
    void setupUi();

    bool m_dark = true;
    bool m_mediaPaused = false;
    QLabel *m_badge;
    QLabel *m_nameAr;
    QLabel *m_subtitle;
    QLabel *m_reminder;
    QLabel *m_reminderEn;
    QWidget *m_progressRow;
    QLabel *m_volumeIcon;
    QProgressBar *m_progress;
    QLabel *m_progressTime;
    QPushButton *m_stopBtn;
    QPushButton *m_closeBtn;
    QLabel *m_note;
};

#endif
