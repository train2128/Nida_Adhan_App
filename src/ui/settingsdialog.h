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
class QStackedWidget;
class QTimer;
class QAction;

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
    void onSearchTextChanged(const QString &text);
    void onSearchTimeout();
    void onSearchFinished(const QList<LocationResult> &results);
    void onSearchError(const QString &error);
    void onResultSelected(QListWidgetItem *item);
    void onDetectClicked();
    void onDetectFinished(const LocationResult &result);
    void onDetectError(const QString &error);

private:
    struct SoundRow {
        QString sound;          // built-in name or absolute file path
        QWidget *widget = nullptr;
        QLabel *check = nullptr;
        QPushButton *play = nullptr;
    };

    void setupUi();
    QWidget *buildLocationPage();
    QWidget *buildTimesPage();
    QWidget *buildAdhanPage();
    QWidget *buildGeneralPage();
    QWidget *section(const QString &caption, QLayout *content);

    void populateSounds();
    int addSoundRow(const QString &sound, const QString &title);
    void selectSound(const QString &sound);
    void updateSoundRows();
    void togglePreview(int row);
    void updateSavedLabel();
    void setPending(const LocationResult &result);
    void applyManualLocation();
    void setDark(bool dark);
    QString selectedTheme() const;

    StorageService *m_storage;
    LocationService *m_location = nullptr;
    NidaSettings m_settings;
    bool m_dark = true;

    QListWidget *m_nav = nullptr;
    QStackedWidget *m_pages = nullptr;
    QList<QPair<QListWidgetItem *, QString>> m_navIcons;

    // Location
    QLabel *m_savedIcon = nullptr;
    QLabel *m_savedLabel = nullptr;
    QLabel *m_coordsLabel = nullptr;
    QLineEdit *m_searchEdit = nullptr;
    QAction *m_searchIcon = nullptr;
    QListWidget *m_resultsList = nullptr;
    QPushButton *m_detectBtn = nullptr;
    QLabel *m_searchStatus = nullptr;
    QTimer *m_searchTimer = nullptr;
    QList<LocationResult> m_searchResults;
    LocationResult m_pending;
    bool m_hasPending = false;

    // Prayer times
    QComboBox *m_methodCombo = nullptr;

    // Adhan
    QListWidget *m_soundList = nullptr;
    QList<SoundRow> m_soundRows;
    QPushButton *m_browseBtn = nullptr;
    QSlider *m_volumeSlider = nullptr;
    QLabel *m_volumeIcon = nullptr;
    QLabel *m_volumeValue = nullptr;
    int m_previewRow = -1;

    // General
    QCheckBox *m_startupCheck = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QComboBox *m_langCombo = nullptr;

    QMediaPlayer *m_player;
    QAudioOutput *m_audio;
};

#endif
