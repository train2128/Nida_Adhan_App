#include "settingsdialog.h"
#include "storageservice.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCheckBox>
#include <QSlider>
#include <QListWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QTimer>
#include <QUrl>
#include <QtGlobal>
#include <QDebug>

SettingsDialog::SettingsDialog(StorageService *storage, QWidget *parent)
    : QDialog(parent)
    , m_storage(storage)
    , m_location(new LocationService(this))
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
{
    qRegisterMetaType<QList<LocationResult>>("QList<LocationResult>");
    m_player->setAudioOutput(m_audio);
    setWindowTitle("Nida Settings");
    setMinimumWidth(420);
    setupUi();
    loadSettings();
    populateMethods();
    populateSounds();

    connect(m_location, &LocationService::searchFinished,
            this, &SettingsDialog::onSearchFinished);
    connect(m_location, &LocationService::searchError,
            this, &SettingsDialog::onSearchError);
    connect(m_location, &LocationService::detectFinished,
            this, &SettingsDialog::onDetectFinished);
    connect(m_location, &LocationService::detectError,
            this, &SettingsDialog::onDetectError);
}

SettingsDialog::~SettingsDialog()
{
    m_player->stop();
}

void SettingsDialog::setupUi()
{
    auto *root = new QVBoxLayout(this);

    // Location: online Photon search + auto-detect, saved offline in SQLite.
    auto *locGroup = new QGroupBox("Location");
    auto *locLayout = new QVBoxLayout(locGroup);
    m_savedLabel = new QLabel;
    m_savedLabel->setWordWrap(true);
    m_savedLabel->setObjectName("savedLocation");
    locLayout->addWidget(m_savedLabel);

    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText("Search any city... (e.g. Sousse)");
    m_searchEdit->setClearButtonEnabled(true);
    locLayout->addWidget(m_searchEdit);

    m_resultsList = new QListWidget;
    m_resultsList->setMaximumHeight(110);
    locLayout->addWidget(m_resultsList);

    auto *locBtnRow = new QHBoxLayout;
    m_detectBtn = new QPushButton("Detect my location");
    m_detectBtn->setToolTip("IP-based lookup (ipapi.co, fallback ip-api.com); needs internet once, then saved offline");
    m_detectBtn->setCursor(Qt::PointingHandCursor);
    m_searchStatus = new QLabel;
    m_searchStatus->setWordWrap(true);
    locBtnRow->addWidget(m_detectBtn);
    locBtnRow->addWidget(m_searchStatus, 1);
    locLayout->addLayout(locBtnRow);
    root->addWidget(locGroup);

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(350);
    connect(m_searchEdit, &QLineEdit::textChanged,
            this, &SettingsDialog::onSearchTextChanged);
    connect(m_searchTimer, &QTimer::timeout, this, &SettingsDialog::onSearchTimeout);
    connect(m_resultsList, &QListWidget::itemClicked,
            this, &SettingsDialog::onResultSelected);
    connect(m_detectBtn, &QPushButton::clicked, this, &SettingsDialog::onDetectClicked);

    // Calculation Method
    auto *methodGroup = new QGroupBox("Calculation Method");
    auto *methodLayout = new QVBoxLayout(methodGroup);
    m_methodCombo = new QComboBox;
    methodLayout->addWidget(m_methodCombo);
    root->addWidget(methodGroup);

    // Adhan Sound
    auto *soundGroup = new QGroupBox("Adhan Sound");
    auto *soundLayout = new QVBoxLayout(soundGroup);
    m_soundList = new QListWidget;
    m_soundList->setMaximumHeight(120);
    soundLayout->addWidget(m_soundList);

    auto *soundBtnRow = new QHBoxLayout;
    m_playBtn = new QPushButton("▶ Play");
    m_playBtn->setObjectName("playBtn");
    m_playBtn->setCursor(Qt::PointingHandCursor);
    auto *browseBtn = new QPushButton("Browse custom...");
    browseBtn->setObjectName("browseSoundBtn");
    soundBtnRow->addWidget(m_playBtn);
    soundBtnRow->addWidget(browseBtn);
    soundBtnRow->addStretch();
    soundLayout->addLayout(soundBtnRow);

    connect(m_playBtn, &QPushButton::clicked, this, &SettingsDialog::onPlaySound);
    connect(browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseSound);

    // Volume
    auto *volLayout = new QHBoxLayout;
    volLayout->addWidget(new QLabel("Volume:"));
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    volLayout->addWidget(m_volumeSlider);
    soundLayout->addLayout(volLayout);
    root->addWidget(soundGroup);

    // Preferences
    auto *prefGroup = new QGroupBox("Preferences");
    auto *prefLayout = new QVBoxLayout(prefGroup);
    m_startupCheck = new QCheckBox("Start with system");
    m_darkThemeCheck = new QCheckBox("Dark theme");
    prefLayout->addWidget(m_startupCheck);
    prefLayout->addWidget(m_darkThemeCheck);
    root->addWidget(prefGroup);

    // Language
    auto *langGroup = new QGroupBox("Language");
    auto *langLayout = new QVBoxLayout(langGroup);
    m_langCombo = new QComboBox;
    m_langCombo->addItem("English", "en");
    m_langCombo->addItem("العربية", "ar");
    langLayout->addWidget(m_langCombo);
    root->addWidget(langGroup);

    // Buttons
    auto *btnLayout = new QHBoxLayout;
    auto *cancelBtn = new QPushButton("Cancel");
    auto *saveBtn = new QPushButton("Save");
    saveBtn->setObjectName("saveBtn");
    cancelBtn->setObjectName("cancelBtn");
    btnLayout->addStretch();
    btnLayout->addWidget(cancelBtn);
    btnLayout->addWidget(saveBtn);
    root->addLayout(btnLayout);

    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState s) {
        if (s == QMediaPlayer::StoppedState) {
            m_playBtn->setText("▶ Play");
            m_playing = false;
        }
    });
}

void SettingsDialog::loadSettings()
{
    m_settings = m_storage->loadSettings();
    m_hasPending = false;
    m_searchResults.clear();
    m_resultsList->clear();
    m_searchEdit->clear();
    m_searchStatus->clear();
    m_detectBtn->setEnabled(true);
    updateSavedLabel();
    m_volumeSlider->setValue(m_settings.volume);
    m_startupCheck->setChecked(m_settings.startupEnabled);
    m_darkThemeCheck->setChecked(m_settings.darkTheme);
    m_langCombo->setCurrentIndex(m_langCombo->findData(m_settings.language));
}

void SettingsDialog::updateSavedLabel()
{
    QString loc = m_settings.displayLocation();
    if (m_settings.hasCoords()) {
        m_savedLabel->setText(QString("Saved: %1 (%2, %3)")
            .arg(loc)
            .arg(m_settings.latitude, 0, 'f', 4)
            .arg(m_settings.longitude, 0, 'f', 4));
    } else {
        m_savedLabel->setText(QString("Saved: %1").arg(loc));
    }
}

void SettingsDialog::onSearchTextChanged(const QString &text)
{
    m_searchTimer->stop();
    if (text.trimmed().length() < 2) {
        m_resultsList->clear();
        m_searchResults.clear();
        m_searchStatus->clear();
        return;
    }
    m_searchStatus->setText("Searching...");
    m_searchTimer->start();
}

void SettingsDialog::onSearchTimeout()
{
    const QString text = m_searchEdit->text();
    if (text.trimmed().length() < 2)
        return;
    m_location->searchLocations(text, m_settings.language);
}

void SettingsDialog::onSearchFinished(const QList<LocationResult> &results)
{
    m_searchResults = results;
    m_resultsList->clear();
    if (results.isEmpty()) {
        m_searchStatus->setText("No matches. You can still type \"City, Country\" and Save.");
        return;
    }
    m_searchStatus->setText(QString("%1 match(es) — click to select").arg(results.size()));
    for (int i = 0; i < results.size(); ++i) {
        const LocationResult &r = results.at(i);
        auto *item = new QListWidgetItem(
            QString("%1 (%2, %3)").arg(r.label)
                .arg(r.latitude, 0, 'f', 3).arg(r.longitude, 0, 'f', 3));
        item->setData(Qt::UserRole, i);
        m_resultsList->addItem(item);
    }
}

void SettingsDialog::onSearchError(const QString &error)
{
    m_searchStatus->setText(QString("Search failed: %1").arg(error));
}

void SettingsDialog::onResultSelected(QListWidgetItem *item)
{
    if (!item)
        return;
    const int idx = item->data(Qt::UserRole).toInt();
    if (idx < 0 || idx >= m_searchResults.size())
        return;
    setPending(m_searchResults.at(idx));
}

void SettingsDialog::setPending(const LocationResult &result)
{
    if (!result.valid)
        return;
    m_pending = result;
    m_hasPending = true;
    m_searchStatus->setText(QString("Selected: %1 — press Save").arg(result.label));
    m_searchEdit->setText(result.label);
}

void SettingsDialog::onDetectClicked()
{
    m_detectBtn->setEnabled(false);
    m_searchStatus->setText("Detecting your location...");
    m_location->detectCurrentLocation();
}

void SettingsDialog::onDetectFinished(const LocationResult &result)
{
    m_detectBtn->setEnabled(true);
    setPending(result);
}

void SettingsDialog::onDetectError(const QString &error)
{
    m_detectBtn->setEnabled(true);
    m_searchStatus->setText(QString("Detect failed: %1").arg(error));
}

void SettingsDialog::populateMethods()
{
    m_methodCombo->addItem("Muslim World League", 3);
    m_methodCombo->addItem("Islamic Society of North America", 2);
    m_methodCombo->addItem("Egyptian General Authority", 5);
    m_methodCombo->addItem("Umm Al-Qura (Makkah)", 4);
    m_methodCombo->addItem("University of Islamic Sciences (Karachi)", 1);
    m_methodCombo->addItem("Institute of Geophysics (Tehran)", 7);
    m_methodCombo->addItem("Gulf Region", 8);
    m_methodCombo->addItem("Kuwait", 9);
    m_methodCombo->addItem("Qatar", 10);
    m_methodCombo->addItem("Majlis Ugama (Singapore)", 11);
    m_methodCombo->addItem("Union Organization (France)", 12);
    m_methodCombo->addItem("Diyanet (Turkey)", 13);
    m_methodCombo->addItem("Spiritual Administration (Russia)", 14);
    m_methodCombo->addItem("Moonsighting Committee", 15);
    m_methodCombo->addItem("Dubai", 16);
    m_methodCombo->addItem("JAKIM (Malaysia)", 17);
    m_methodCombo->addItem("Tunisia", 18);
    m_methodCombo->addItem("Algeria", 19);
    m_methodCombo->addItem("Kemenag (Indonesia)", 20);
    m_methodCombo->addItem("Morocco", 21);
    m_methodCombo->addItem("Comunidade de Lisboa (Portugal)", 22);
    m_methodCombo->addItem("Ministry of Awqaf (Jordan)", 23);

    int idx = m_methodCombo->findData(m_settings.method);
    if (idx >= 0) m_methodCombo->setCurrentIndex(idx);
}

void SettingsDialog::populateSounds()
{
    QStringList builtIn = {
        "Abdul-Basit", "Adhan-Alaqsa", "Adhan-Madinah",
        "Adhan-Makkah", "Naghshbandi"
    };
    for (const auto &name : builtIn) {
        auto *item = new QListWidgetItem(name);
        item->setData(Qt::UserRole, name);
        m_soundList->addItem(item);
    }
    for (int i = 0; i < m_soundList->count(); ++i) {
        if (m_soundList->item(i)->data(Qt::UserRole).toString() == m_settings.selectedAdhan) {
            m_soundList->setCurrentRow(i);
            break;
        }
    }
}

void SettingsDialog::onBrowseSound()
{
    QString path = QFileDialog::getOpenFileName(this, "Select Adhan Sound",
        QString(), "Audio Files (*.mp3 *.wav *.ogg)");
    if (path.isEmpty()) return;

    QString fileName = QFileInfo(path).fileName();
    auto *item = new QListWidgetItem(fileName);
    item->setData(Qt::UserRole, path);
    m_soundList->addItem(item);
    m_soundList->setCurrentItem(item);
}

void SettingsDialog::onPlaySound()
{
    auto *cur = m_soundList->currentItem();
    if (!cur) return;

    if (m_playing) {
        m_player->stop();
        m_playBtn->setText("▶ Play");
        m_playing = false;
        return;
    }

    QString sound = cur->data(Qt::UserRole).toString();
    QString path;
    if (sound.startsWith('/') || sound.startsWith("file://"))
        path = sound;
    else if (sound.startsWith("qrc") || sound.startsWith(":/"))
        path = sound;
    else
        path = "qrc:/sounds/" + sound;

    QUrl source;
    if (path.startsWith("file://") || path.startsWith("qrc:/") || path.startsWith(":/"))
        source = QUrl(path);
    else
        source = QUrl::fromLocalFile(path);
    m_player->setSource(source);
    m_audio->setVolume(m_volumeSlider->value() / 100.0f);
    m_player->play();
    m_playBtn->setText("■ Stop");
    m_playing = true;
}

void SettingsDialog::applyManualLocation()
{
    // Offline fallback: user typed "City, Country" without picking a search
    // result. Coords stay unset -> legacy city/country timings are used.
    const QString text = m_searchEdit->text().trimmed();
    if (text.isEmpty())
        return; // keep previously saved location
    const int commaPos = text.lastIndexOf(", ");
    if (commaPos > 0) {
        m_settings.city = text.left(commaPos).trimmed();
        m_settings.country = text.mid(commaPos + 2).trimmed();
    } else {
        m_settings.city = text;
        m_settings.country = text;
    }
    m_settings.latitude = qQNaN();
    m_settings.longitude = qQNaN();
    m_settings.locationLabel = text;
}

void SettingsDialog::onSave()
{
    if (m_hasPending && m_pending.valid) {
        // Exact Photon/IP location: saved with coords for precise timings and
        // offline reuse via the coords-keyed prayer cache.
        m_settings.city = m_pending.city;
        m_settings.country = m_pending.country;
        m_settings.latitude = m_pending.latitude;
        m_settings.longitude = m_pending.longitude;
        m_settings.locationLabel = m_pending.label;
    } else {
        applyManualLocation();
    }
    m_settings.method = m_methodCombo->currentData().toInt();

    auto *cur = m_soundList->currentItem();
    if (cur)
        m_settings.selectedAdhan = cur->data(Qt::UserRole).toString();

    m_settings.volume = m_volumeSlider->value();
    m_settings.startupEnabled = m_startupCheck->isChecked();
    m_settings.darkTheme = m_darkThemeCheck->isChecked();
    m_settings.language = m_langCombo->currentData().toString();

    m_storage->saveSettings(m_settings);
    emit settingsChanged();
    accept();
}

void SettingsDialog::onCancel()
{
    m_player->stop();
    reject();
}
