#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "models/prayertimes.h"
#include "services/locationservice.h"

class StorageService;
class QComboBox;
class QCheckBox;
class QSlider;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QLineEdit;
class QLabel;
class QTimer;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(StorageService *storage, QWidget *parent = nullptr);
    ~SettingsDialog();

signals:
    void settingsChanged();

public slots:
    void loadSettings();

private slots:
    void onSave();
    void onCancel();
    void onBrowseSound();
    void onPlaySound();
    void onSearchTextChanged(const QString &text);
    void onSearchTimeout();
    void onSearchFinished(const QList<LocationResult> &results);
    void onSearchError(const QString &error);
    void onResultSelected(QListWidgetItem *item);
    void onDetectClicked();
    void onDetectFinished(const LocationResult &result);
    void onDetectError(const QString &error);

private:
    void setupUi();
    void populateMethods();
    void populateSounds();
    void updateSavedLabel();
    void setPending(const LocationResult &result);
    void applyManualLocation();

    StorageService *m_storage;
    LocationService *m_location = nullptr;
    NidaSettings m_settings;

    QLineEdit *m_searchEdit = nullptr;
    QListWidget *m_resultsList = nullptr;
    QPushButton *m_detectBtn = nullptr;
    QLabel *m_savedLabel = nullptr;
    QLabel *m_searchStatus = nullptr;
    QTimer *m_searchTimer = nullptr;
    QList<LocationResult> m_searchResults;
    LocationResult m_pending;
    bool m_hasPending = false;

    QComboBox *m_methodCombo;
    QListWidget *m_soundList;
    QSlider *m_volumeSlider;
    QCheckBox *m_startupCheck;
    QCheckBox *m_darkThemeCheck;
    QComboBox *m_langCombo;
    QPushButton *m_playBtn;

    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
    bool m_playing = false;
};

#endif
