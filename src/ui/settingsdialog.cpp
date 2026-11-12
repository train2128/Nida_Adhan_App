#include "settingsdialog.h"
#include "storageservice.h"
#include "theme.h"
#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QStandardPaths>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QtGlobal>

namespace {
const QStringList kBuiltInSounds = {
    "Adhan-Makkah", "Adhan-Madinah", "Adhan-Alaqsa", "Abdul-Basit", "Naghshbandi"
};

QUrl resolveAdhanPreviewSource(const QString &sound)
{
    QString path = sound;
    const bool isResourceOrUrl = path.startsWith(QStringLiteral("qrc:"))
        || path.startsWith(QStringLiteral(":/"))
        || path.startsWith(QStringLiteral("file://"));
    if (!isResourceOrUrl && !QDir::isAbsolutePath(path))
        path = QStringLiteral("qrc:/sounds/%1").arg(path);
    if (path.startsWith(QStringLiteral("qrc:")) || path.startsWith(QStringLiteral(":/"))
        || path.startsWith(QStringLiteral("file://")))
        return QUrl(path);
    return QUrl::fromLocalFile(path);
}

bool sameSound(const QString &a, const QString &b)
{
#ifdef Q_OS_WINDOWS
    return a.compare(b, Qt::CaseInsensitive) == 0; // case-insensitive FS
#else
    return a == b;
#endif
}

QLabel *pageTitle(const QString &text)
{
    auto *l = new QLabel(text);
    l->setObjectName("pageTitle");
    return l;
}

QLabel *hint(const QString &text)
{
    auto *l = new QLabel(text);
    l->setObjectName("hint");
    l->setWordWrap(true);
    return l;
}
}

SettingsDialog::SettingsDialog(StorageService *storage, QWidget *parent)
    : QDialog(parent)
    , m_storage(storage)
    , m_location(new LocationService(this))
    , m_player(new QMediaPlayer(this))
    , m_audio(new QAudioOutput(this))
{
    qRegisterMetaType<QList<LocationResult>>("QList<LocationResult>");
    m_player->setAudioOutput(m_audio);
    setObjectName("settingsDialog");
    setWindowTitle("Nida Settings");
    resize(660, 500);
    setMinimumSize(600, 460);
    setupUi();
    loadSettings();

    connect(m_location, &LocationService::searchFinished, this, &SettingsDialog::onSearchFinished);
    connect(m_location, &LocationService::searchError, this, &SettingsDialog::onSearchError);
    connect(m_location, &LocationService::detectFinished, this, &SettingsDialog::onDetectFinished);
    connect(m_location, &LocationService::detectError, this, &SettingsDialog::onDetectError);
    connect(m_player, &QMediaPlayer::playbackStateChanged, this, [this](QMediaPlayer::PlaybackState s) {
        if (s == QMediaPlayer::StoppedState) {
            m_previewRow = -1;
            updateSoundRows();
        }
    });
}

SettingsDialog::~SettingsDialog()
{
    m_player->stop();
}

// ── Layout ────────────────────────────────────────────────────────────────

void SettingsDialog::setupUi()
{
    auto *root = new QHBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);

    // Sidebar
    auto *sidebar = new QWidget;
    sidebar->setObjectName("sidebar");
    sidebar->setAttribute(Qt::WA_StyledBackground);
    sidebar->setFixedWidth(176);
    auto *side = new QVBoxLayout(sidebar);
    side->setContentsMargins(10, 16, 10, 12);
    side->setSpacing(8);
    auto *appTitle = new QLabel(QStringLiteral("Nida  نداء"));
    appTitle->setObjectName("sidebarTitle");
    side->addWidget(appTitle);
    side->addSpacing(6);
    m_nav = new QListWidget;
    m_nav->setObjectName("navList");
    m_nav->setIconSize(QSize(17, 17));
    m_nav->setFrameShape(QFrame::NoFrame);
    m_nav->setFocusPolicy(Qt::NoFocus);
    const QList<QPair<QString, QString>> nav = {
        {"Location", "map-pin"}, {"Prayer times", "clock"},
        {"Adhan", "volume"}, {"General", "adjustments"},
    };
    for (const auto &n : nav) {
        auto *item = new QListWidgetItem(n.first);
        item->setSizeHint(QSize(0, 36));
        m_nav->addItem(item);
        m_navIcons.append({item, n.second});
    }
    side->addWidget(m_nav, 1);
    auto *version = new QLabel(QStringLiteral("Nida %1").arg(QApplication::applicationVersion()));
    version->setObjectName("hint");
    side->addWidget(version);
    root->addWidget(sidebar);

    // Content
    auto *content = new QVBoxLayout;
    content->setContentsMargins(20, 18, 20, 16);
    content->setSpacing(12);
    m_pages = new QStackedWidget;
    m_pages->addWidget(buildLocationPage());
    m_pages->addWidget(buildTimesPage());
    m_pages->addWidget(buildAdhanPage());
    m_pages->addWidget(buildGeneralPage());
    content->addWidget(m_pages, 1);

    auto *buttons = new QHBoxLayout;
    auto *cancelBtn = new QPushButton(QStringLiteral("Cancel"));
    cancelBtn->setObjectName("secondaryBtn");
    auto *saveBtn = new QPushButton(QStringLiteral("Save"));
    saveBtn->setObjectName("primaryBtn");
    saveBtn->setDefault(true);
    for (QPushButton *b : {cancelBtn, saveBtn})
        b->setCursor(Qt::PointingHandCursor);
    buttons->addStretch();
    buttons->addWidget(cancelBtn);
    buttons->addWidget(saveBtn);
    content->addLayout(buttons);
    root->addLayout(content, 1);

    connect(m_nav, &QListWidget::currentRowChanged, m_pages, &QStackedWidget::setCurrentIndex);
    connect(saveBtn, &QPushButton::clicked, this, &SettingsDialog::onSave);
    connect(cancelBtn, &QPushButton::clicked, this, &SettingsDialog::onCancel);
}

QWidget *SettingsDialog::section(const QString &caption, QLayout *content)
{
    auto *box = new QWidget;
    box->setObjectName("section");
    box->setAttribute(Qt::WA_StyledBackground);
    auto *v = new QVBoxLayout(box);
    v->setContentsMargins(14, 12, 14, 14);
    v->setSpacing(8);
    auto *cap = new QLabel(caption);
    cap->setObjectName("sectionCaption");
    v->addWidget(cap);
    v->addLayout(content);
    return box;
}

QWidget *SettingsDialog::buildLocationPage()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);
    v->addWidget(pageTitle(QStringLiteral("Location")));

    auto *current = new QHBoxLayout;
    current->setSpacing(8);
    m_savedIcon = new QLabel;
    m_savedLabel = new QLabel;
    m_savedLabel->setObjectName("savedLocation");
    m_savedLabel->setWordWrap(true);
    m_coordsLabel = new QLabel;
    m_coordsLabel->setObjectName("hint");
    current->addWidget(m_savedIcon);
    current->addWidget(m_savedLabel, 1);
    current->addWidget(m_coordsLabel);
    v->addWidget(section(QStringLiteral("Current location"), current));

    auto *change = new QVBoxLayout;
    change->setSpacing(8);
    m_searchEdit = new QLineEdit;
    m_searchEdit->setPlaceholderText(QStringLiteral("Search for a city, like Sousse"));
    m_searchEdit->setClearButtonEnabled(true);
    m_searchIcon = m_searchEdit->addAction(QIcon(), QLineEdit::LeadingPosition);
    m_resultsList = new QListWidget;
    m_resultsList->setObjectName("resultsList");
    m_resultsList->setMaximumHeight(120);
    m_resultsList->setVisible(false); // shown once there are results
    auto *detectRow = new QHBoxLayout;
    m_detectBtn = new QPushButton(QStringLiteral("Detect my location"));
    m_detectBtn->setObjectName("secondaryBtn");
    m_detectBtn->setCursor(Qt::PointingHandCursor);
    m_detectBtn->setToolTip(QStringLiteral("IP-based lookup (ipapi.co, fallback ip-api.com). "
                                           "Needs internet once, then saved offline."));
    m_searchStatus = new QLabel;
    m_searchStatus->setObjectName("hint");
    m_searchStatus->setWordWrap(true);
    detectRow->addWidget(m_detectBtn);
    detectRow->addWidget(m_searchStatus, 1);
    change->addWidget(m_searchEdit);
    change->addWidget(m_resultsList);
    change->addLayout(detectRow);
    v->addWidget(section(QStringLiteral("Change location"), change));
    v->addStretch();

    m_searchTimer = new QTimer(this);
    m_searchTimer->setSingleShot(true);
    m_searchTimer->setInterval(350);
    connect(m_searchEdit, &QLineEdit::textChanged, this, &SettingsDialog::onSearchTextChanged);
    connect(m_searchTimer, &QTimer::timeout, this, &SettingsDialog::onSearchTimeout);
    connect(m_resultsList, &QListWidget::itemClicked, this, &SettingsDialog::onResultSelected);
    connect(m_detectBtn, &QPushButton::clicked, this, &SettingsDialog::onDetectClicked);
    return page;
}

QWidget *SettingsDialog::buildTimesPage()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);
    v->addWidget(pageTitle(QStringLiteral("Prayer times")));

    auto *method = new QVBoxLayout;
    m_methodCombo = new QComboBox;
    for (const auto &m : calculationMethods())
        m_methodCombo->addItem(m.second, m.first);
    method->addWidget(m_methodCombo);
    method->addWidget(hint(QStringLiteral("Pick the method used by your local mosque or "
                                          "religious authority. Times come from aladhan.com "
                                          "and are kept for offline use.")));
    v->addWidget(section(QStringLiteral("Calculation method"), method));
    v->addStretch();
    return page;
}

QWidget *SettingsDialog::buildAdhanPage()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);
    v->addWidget(pageTitle(QStringLiteral("Adhan")));

    auto *sound = new QVBoxLayout;
    m_soundList = new QListWidget;
    m_soundList->setObjectName("soundList");
    m_soundList->setMinimumHeight(5 * 38 + 12);
    m_browseBtn = new QPushButton(QStringLiteral("Add your own sound..."));
    m_browseBtn->setObjectName("secondaryBtn");
    m_browseBtn->setCursor(Qt::PointingHandCursor);
    auto *browseRow = new QHBoxLayout;
    browseRow->addWidget(m_browseBtn);
    browseRow->addStretch();
    sound->addWidget(m_soundList);
    sound->addLayout(browseRow);
    v->addWidget(section(QStringLiteral("Adhan sound"), sound));

    auto *vol = new QHBoxLayout;
    vol->setSpacing(10);
    m_volumeIcon = new QLabel;
    m_volumeSlider = new QSlider(Qt::Horizontal);
    m_volumeSlider->setRange(0, 100);
    m_volumeValue = new QLabel(QStringLiteral("80%"));
    m_volumeValue->setObjectName("hint");
    m_volumeValue->setMinimumWidth(36);
    vol->addWidget(m_volumeIcon);
    vol->addWidget(m_volumeSlider, 1);
    vol->addWidget(m_volumeValue);
    v->addWidget(section(QStringLiteral("Volume"), vol));
    v->addStretch();

    connect(m_soundList, &QListWidget::currentRowChanged, this, [this](int) { updateSoundRows(); });
    connect(m_browseBtn, &QPushButton::clicked, this, &SettingsDialog::onBrowseSound);
    connect(m_volumeSlider, &QSlider::valueChanged, this, [this](int value) {
        m_volumeValue->setText(QStringLiteral("%1%").arg(value));
        m_audio->setVolume(value / 100.0f);
    });
    return page;
}

QWidget *SettingsDialog::buildGeneralPage()
{
    auto *page = new QWidget;
    auto *v = new QVBoxLayout(page);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(10);
    v->addWidget(pageTitle(QStringLiteral("General")));

    auto *startup = new QVBoxLayout;
    m_startupCheck = new QCheckBox(QStringLiteral("Start Nida when you sign in"));
    startup->addWidget(m_startupCheck);
    v->addWidget(section(QStringLiteral("Startup"), startup));

    auto *look = new QVBoxLayout;
    m_themeCombo = new QComboBox;
    m_themeCombo->addItem(QStringLiteral("Dark"), QStringLiteral("dark"));
    m_themeCombo->addItem(QStringLiteral("Light"), QStringLiteral("light"));
    m_themeCombo->addItem(QStringLiteral("Match system"), QStringLiteral("system"));
    look->addWidget(m_themeCombo);
    v->addWidget(section(QStringLiteral("Theme"), look));

    auto *lang = new QVBoxLayout;
    m_langCombo = new QComboBox;
    m_langCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));
    m_langCombo->addItem(QStringLiteral("العربية"), QStringLiteral("ar"));
    lang->addWidget(m_langCombo);
    lang->addWidget(hint(QStringLiteral("Used for city names in location search.")));
    v->addWidget(section(QStringLiteral("Language"), lang));
    v->addStretch();

    // Live preview of the theme inside this window.
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this]() {
        NidaSettings preview = m_settings;
        preview.theme = selectedTheme();
        setDark(Theme::isDark(preview));
    });
    return page;
}

// ── Theme ─────────────────────────────────────────────────────────────────

QString SettingsDialog::selectedTheme() const
{
    return m_themeCombo->currentData().toString();
}

void SettingsDialog::setDark(bool dark)
{
    m_dark = dark;
    Theme::apply(this, dark);
    const ThemeColors &c = Theme::colors(dark);
    for (const auto &n : m_navIcons) {
        QIcon icon;
        icon.addPixmap(Theme::pixmap(n.second, c.muted, 17), QIcon::Normal);
        icon.addPixmap(Theme::pixmap(n.second, c.accent, 17), QIcon::Selected);
        n.first->setIcon(icon);
    }
    m_savedIcon->setPixmap(Theme::pixmap(QStringLiteral("map-pin"), c.gold, 16));
    m_searchIcon->setIcon(Theme::icon(QStringLiteral("search"), c.muted, 16));
    m_detectBtn->setIcon(Theme::icon(QStringLiteral("location"), c.text, 15));
    m_browseBtn->setIcon(Theme::icon(QStringLiteral("folder"), c.text, 15));
    m_volumeIcon->setPixmap(Theme::pixmap(QStringLiteral("volume"), c.muted, 16));
    updateSoundRows();
}

// ── Load / save ───────────────────────────────────────────────────────────

void SettingsDialog::loadSettings()
{
    m_player->stop();
    m_settings = m_storage->loadSettings();
    m_hasPending = false;
    m_searchResults.clear();
    m_resultsList->clear();
    m_resultsList->setVisible(false);
    m_searchEdit->clear();
    m_searchStatus->clear();
    m_detectBtn->setEnabled(true);
    updateSavedLabel();

    const int methodIdx = m_methodCombo->findData(m_settings.method);
    m_methodCombo->setCurrentIndex(methodIdx >= 0 ? methodIdx : 0);
    populateSounds();
    m_volumeSlider->setValue(m_settings.volume);
    m_volumeValue->setText(QStringLiteral("%1%").arg(m_settings.volume));
    m_startupCheck->setChecked(m_settings.startupEnabled);
    const int themeIdx = m_themeCombo->findData(m_settings.theme);
    m_themeCombo->blockSignals(true);
    m_themeCombo->setCurrentIndex(themeIdx >= 0 ? themeIdx : 0);
    m_themeCombo->blockSignals(false);
    const int langIdx = m_langCombo->findData(m_settings.language);
    m_langCombo->setCurrentIndex(langIdx >= 0 ? langIdx : 0);

    m_nav->setCurrentRow(0);
    setDark(Theme::isDark(m_settings));
}

void SettingsDialog::updateSavedLabel()
{
    m_savedLabel->setText(m_settings.displayLocation());
    if (m_settings.hasCoords()) {
        m_coordsLabel->setText(QStringLiteral("%1, %2")
            .arg(m_settings.latitude, 0, 'f', 4)
            .arg(m_settings.longitude, 0, 'f', 4));
    } else {
        m_coordsLabel->setText(QStringLiteral("by city name"));
    }
}

void SettingsDialog::onSave()
{
    m_player->stop();
    if (m_hasPending && m_pending.valid) {
        // Exact Photon/IP location: saved with coords for precise timings.
        m_settings.city = m_pending.city;
        m_settings.country = m_pending.country;
        m_settings.latitude = m_pending.latitude;
        m_settings.longitude = m_pending.longitude;
        m_settings.locationLabel = m_pending.label;
    } else {
        applyManualLocation();
    }
    m_settings.method = m_methodCombo->currentData().toInt();
    const int row = m_soundList->currentRow();
    if (row >= 0 && row < m_soundRows.size())
        m_settings.selectedAdhan = m_soundRows.at(row).sound;
    m_settings.volume = m_volumeSlider->value();
    m_settings.startupEnabled = m_startupCheck->isChecked();
    m_settings.theme = selectedTheme();
    m_settings.darkTheme = Theme::isDark(m_settings);
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

// ── Location ──────────────────────────────────────────────────────────────

void SettingsDialog::onSearchTextChanged(const QString &text)
{
    m_searchTimer->stop();
    if (m_hasPending && text == m_pending.label)
        return; // text was filled in by selecting a result
    m_hasPending = false;
    if (text.trimmed().length() < 2) {
        m_resultsList->clear();
        m_resultsList->setVisible(false);
        m_searchResults.clear();
        m_searchStatus->clear();
        return;
    }
    m_searchStatus->setText(QStringLiteral("Searching..."));
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
    m_resultsList->setVisible(!results.isEmpty());
    if (results.isEmpty()) {
        m_searchStatus->setText(QStringLiteral("No matches. You can still type \"City, Country\" and save."));
        return;
    }
    m_searchStatus->setText(QStringLiteral("Pick a result, then save"));
    for (int i = 0; i < results.size(); ++i) {
        const LocationResult &r = results.at(i);
        auto *item = new QListWidgetItem(r.label);
        item->setToolTip(QStringLiteral("%1, %2").arg(r.latitude, 0, 'f', 4).arg(r.longitude, 0, 'f', 4));
        item->setData(Qt::UserRole, i);
        m_resultsList->addItem(item);
    }
}

void SettingsDialog::onSearchError(const QString &error)
{
    m_searchStatus->setText(QStringLiteral("Search failed: %1").arg(error));
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
    m_searchEdit->setText(result.label);
    m_searchStatus->setText(QStringLiteral("Selected %1 (%2, %3). Save to use it.")
        .arg(result.label)
        .arg(result.latitude, 0, 'f', 3)
        .arg(result.longitude, 0, 'f', 3));
}

void SettingsDialog::onDetectClicked()
{
    m_detectBtn->setEnabled(false);
    m_searchStatus->setText(QStringLiteral("Detecting your location..."));
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
    m_searchStatus->setText(QStringLiteral("Couldn't detect your location: %1").arg(error));
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

// ── Adhan sounds ──────────────────────────────────────────────────────────

void SettingsDialog::populateSounds()
{
    m_soundList->clear();
    m_soundRows.clear();
    m_previewRow = -1;
    for (const QString &name : kBuiltInSounds)
        addSoundRow(name, name);
    // A saved custom file is listed too, so saving again keeps it.
    bool known = false;
    for (const SoundRow &r : m_soundRows)
        known = known || sameSound(r.sound, m_settings.selectedAdhan);
    if (!known && !m_settings.selectedAdhan.isEmpty())
        addSoundRow(m_settings.selectedAdhan, QFileInfo(m_settings.selectedAdhan).fileName());
    selectSound(m_settings.selectedAdhan);
}

int SettingsDialog::addSoundRow(const QString &sound, const QString &title)
{
    SoundRow row;
    row.sound = sound;
    row.widget = new QWidget;
    row.widget->setObjectName("soundRow");
    auto *h = new QHBoxLayout(row.widget);
    h->setContentsMargins(10, 4, 6, 4);
    h->setSpacing(10);
    row.check = new QLabel;
    auto *name = new QLabel(title);
    name->setObjectName("soundName");
    if (QDir::isAbsolutePath(sound))
        name->setToolTip(QDir::toNativeSeparators(sound));
    row.play = new QPushButton;
    row.play->setObjectName("iconBtnFlat");
    row.play->setFixedSize(28, 28);
    row.play->setIconSize(QSize(15, 15));
    row.play->setCursor(Qt::PointingHandCursor);
    row.play->setToolTip(QStringLiteral("Preview"));
    h->addWidget(row.check);
    h->addWidget(name, 1);
    h->addWidget(row.play);

    auto *item = new QListWidgetItem;
    item->setSizeHint(QSize(0, 38));
    m_soundList->addItem(item);
    m_soundList->setItemWidget(item, row.widget);
    const int index = m_soundRows.size();
    m_soundRows.append(row);
    connect(row.play, &QPushButton::clicked, this, [this, index]() { togglePreview(index); });
    updateSoundRows();
    return index;
}

void SettingsDialog::selectSound(const QString &sound)
{
    for (int i = 0; i < m_soundRows.size(); ++i) {
        if (sameSound(m_soundRows.at(i).sound, sound)) {
            m_soundList->setCurrentRow(i);
            return;
        }
    }
    m_soundList->setCurrentRow(0);
}

void SettingsDialog::updateSoundRows()
{
    const ThemeColors &c = Theme::colors(m_dark);
    const int current = m_soundList ? m_soundList->currentRow() : -1;
    for (int i = 0; i < m_soundRows.size(); ++i) {
        const SoundRow &r = m_soundRows.at(i);
        const bool selected = i == current;
        r.check->setPixmap(Theme::pixmap(selected ? QStringLiteral("circle-check")
                                                  : QStringLiteral("circle"),
                                         selected ? c.accent : c.muted, 17));
        const bool playing = i == m_previewRow;
        r.play->setIcon(Theme::icon(playing ? QStringLiteral("stop") : QStringLiteral("play"),
                                    playing ? c.accent : c.muted, 15));
        r.play->setToolTip(playing ? QStringLiteral("Stop preview") : QStringLiteral("Preview"));
    }
}

void SettingsDialog::togglePreview(int row)
{
    if (row < 0 || row >= m_soundRows.size())
        return;
    if (row == m_previewRow) {
        m_player->stop();
        return;
    }
    m_player->stop();
    m_player->setSource(resolveAdhanPreviewSource(m_soundRows.at(row).sound));
    m_audio->setVolume(m_volumeSlider->value() / 100.0f);
    m_player->play();
    m_previewRow = row;
    updateSoundRows();
}

void SettingsDialog::onBrowseSound()
{
    const QString startDir = QStandardPaths::writableLocation(QStandardPaths::MusicLocation);
    const QString path = QFileDialog::getOpenFileName(this, QStringLiteral("Choose an adhan sound"),
        startDir, QStringLiteral("Audio files (*.mp3 *.wav *.ogg *.m4a)"));
    if (path.isEmpty())
        return;
    for (int i = 0; i < m_soundRows.size(); ++i) {
        if (sameSound(m_soundRows.at(i).sound, path)) {
            m_soundList->setCurrentRow(i);
            return;
        }
    }
    const int row = addSoundRow(path, QFileInfo(path).fileName());
    m_soundList->setCurrentRow(row);
}
