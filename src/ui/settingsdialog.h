#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>
#include <QMediaPlayer>
#include <QAudioOutput>
#include "models/prayertimes.h"

class StorageService;
class QComboBox;
class QCheckBox;
class QSlider;
class QListWidget;
class QPushButton;

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

private:
    void setupUi();
    void populateMethods();
    void populateSounds();
    void populateLocations();
    void updateCityCountryFromCombo();

    StorageService *m_storage;
    NidaSettings m_settings;

    QComboBox *m_locationCombo;
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
