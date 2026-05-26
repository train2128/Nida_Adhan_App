#include "settingsdialog.h"
#include "storageservice.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QComboBox>
#include <QCompleter>
#include <QCheckBox>
#include <QSlider>
#include <QListWidget>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QFileDialog>
#include <QDir>
#include <QFileInfo>
#include <QUrl>
#include <QDebug>

SettingsDialog::SettingsDialog(StorageService *storage, QWidget *parent)
    : QDialog(parent)
    , m_storage(storage)
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
{
    m_player->setAudioOutput(m_audio);
    setWindowTitle("Nida Settings");
    setMinimumWidth(420);
    setupUi();
    loadSettings();
    populateMethods();
    populateSounds();
}

SettingsDialog::~SettingsDialog()
{
    m_player->stop();
}

void SettingsDialog::setupUi()
{
    auto *root = new QVBoxLayout(this);

    // Location (single searchable combo)
    auto *locGroup = new QGroupBox("Location (City / Country)");
    auto *locLayout = new QVBoxLayout(locGroup);
    m_locationCombo = new QComboBox;
    m_locationCombo->setEditable(true);
    m_locationCombo->setInsertPolicy(QComboBox::NoInsert);
    m_locationCombo->setPlaceholderText("Type city or country name...");
    m_locationCombo->setMinimumWidth(280);

    auto *completer = new QCompleter(this);
    completer->setCaseSensitivity(Qt::CaseInsensitive);
    completer->setFilterMode(Qt::MatchContains);
    m_locationCombo->setCompleter(completer);

    locLayout->addWidget(m_locationCombo);
    root->addWidget(locGroup);

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
    populateLocations();
    m_volumeSlider->setValue(m_settings.volume);
    m_startupCheck->setChecked(m_settings.startupEnabled);
    m_darkThemeCheck->setChecked(m_settings.darkTheme);
    m_langCombo->setCurrentIndex(m_langCombo->findData(m_settings.language));
}

void SettingsDialog::populateLocations()
{
    m_locationCombo->clear();
    QStringList locations = {
        "Mecca, Saudi Arabia", "Medina, Saudi Arabia", "Riyadh, Saudi Arabia",
        "Jeddah, Saudi Arabia", "Dammam, Saudi Arabia", "Tabuk, Saudi Arabia",
        "Cairo, Egypt", "Alexandria, Egypt", "Luxor, Egypt",
        "Casablanca, Morocco", "Rabat, Morocco", "Marrakech, Morocco",
        "Algiers, Algeria", "Oran, Algeria", "Constantine, Algeria",
        "Tunis, Tunisia", "Sfax, Tunisia",
        "Tripoli, Libya", "Benghazi, Libya",
        "Khartoum, Sudan", "Omdurman, Sudan",
        "Baghdad, Iraq", "Basra, Iraq", "Mosul, Iraq",
        "Damascus, Syria", "Aleppo, Syria",
        "Amman, Jordan", "Zarqa, Jordan",
        "Beirut, Lebanon", "Tripoli, Lebanon",
        "Jerusalem, Palestine", "Gaza, Palestine", "Ramallah, Palestine",
        "Kuwait City, Kuwait",
        "Doha, Qatar",
        "Manama, Bahrain",
        "Muscat, Oman", "Salalah, Oman",
        "Abu Dhabi, UAE", "Dubai, UAE", "Sharjah, UAE", "Ajman, UAE",
        "Sana'a, Yemen", "Aden, Yemen",
        "Riyadh, Saudi Arabia",
        "Tehran, Iran", "Mashhad, Iran", "Isfahan, Iran", "Shiraz, Iran",
        "Ankara, Turkey", "Istanbul, Turkey", "Izmir, Turkey", "Bursa, Turkey",
        "Kuala Lumpur, Malaysia", "Penang, Malaysia", "Johor Bahru, Malaysia",
        "Jakarta, Indonesia", "Surabaya, Indonesia", "Bandung, Indonesia", "Medan, Indonesia",
        "Islamabad, Pakistan", "Karachi, Pakistan", "Lahore, Pakistan",
        "Dhaka, Bangladesh", "Chittagong, Bangladesh",
        "Kabul, Afghanistan", "Herat, Afghanistan",
        "Mogadishu, Somalia",
        "Djibouti City, Djibouti",
        "Nouakchott, Mauritania",
        "Dakar, Senegal",
        "Bamako, Mali",
        "Niamey, Niger",
        "N'Djamena, Chad",
        "Mumbai, India", "Delhi, India", "Hyderabad, India",
        "Colombo, Sri Lanka",
        "Beijing, China", "Shanghai, China", "Guangzhou, China",
        "Moscow, Russia", "Kazan, Russia",
        "London, United Kingdom", "Birmingham, United Kingdom",
        "Paris, France", "Marseille, France",
        "Berlin, Germany", "Munich, Germany",
        "New York, United States", "Chicago, United States",
        "Los Angeles, United States", "Houston, United States",
        "Toronto, Canada", "Montreal, Canada",
        "Sydney, Australia", "Melbourne, Australia"
    };

    for (const auto &loc : locations) {
        m_locationCombo->addItem(loc);
    }

    // Set current selection
    QString current = m_settings.city + ", " + m_settings.country;
    int idx = m_locationCombo->findText(current, Qt::MatchFixedString);
    if (idx >= 0)
        m_locationCombo->setCurrentIndex(idx);
    else
        m_locationCombo->setCurrentText(current);

    // Connect completer
    auto *completer = m_locationCombo->completer();
    if (completer)
        completer->setModel(m_locationCombo->model());
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
    else if (sound.startsWith("qrc"))
        path = sound;
    else
        path = "qrc:/sounds/" + sound;

    m_player->setSource(QUrl(path));
    m_audio->setVolume(m_volumeSlider->value() / 100.0f);
    m_player->play();
    m_playBtn->setText("■ Stop");
    m_playing = true;
}

void SettingsDialog::updateCityCountryFromCombo()
{
    QString text = m_locationCombo->currentText().trimmed();
    int commaPos = text.lastIndexOf(", ");
    if (commaPos > 0) {
        m_settings.city = text.left(commaPos).trimmed();
        m_settings.country = text.mid(commaPos + 2).trimmed();
    } else {
        m_settings.city = text;
        m_settings.country = text;
    }
}

void SettingsDialog::onSave()
{
    updateCityCountryFromCombo();
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
