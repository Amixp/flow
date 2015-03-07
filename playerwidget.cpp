#include "playerwidget.h"
#include "ui_playerwidget.h"

#include <QFileDialog>
#include <QButtonGroup>
#include <QMessageBox>
#include <QMediaPlaylist>

PlayerWidget::PlayerWidget(MediaComponent *media, ApiComponent *api, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerWidget),
    api_(api), media_(media),
    model_(new QStandardItemModel(this)), playlist_(new QMediaPlaylist(this)),
    repeatMode_(NULL), stillCurrentPlaylist_(false)
{
    Q_ASSERT(media);
    Q_ASSERT(api);

    ui->setupUi(this);

    repeatMode_ = ui->repeatOffButton;

    ui->volumeSlider->setVisible(false);

    QButtonGroup *musicGroup = new QButtonGroup(this);
    musicGroup->addButton(ui->myMusicButton);
    musicGroup->addButton(ui->suggestedButton);
    musicGroup->addButton(ui->popularButton);
    setMusicGroupButtonsVisibility(true);
    connect(ui->musicButton, &QPushButton::toggled, musicGroup, &QButtonGroup::setExclusive);

    QButtonGroup *repeatGroup = new QButtonGroup(this);
    repeatGroup->addButton(ui->repeatSingleButton);
    repeatGroup->addButton(ui->repeatAllButton);
    repeatGroup->addButton(ui->repeatOffButton);

    QButtonGroup *searchGroup = new QButtonGroup(this);
    searchGroup->addButton(ui->byTitleButton);
    searchGroup->addButton(ui->byArtistButton);
    connect(searchGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(trySearch(QAbstractButton*)));
    setSearchFormVisible(false);

    QButtonGroup *controlGroup = new QButtonGroup(this);
    controlGroup->addButton(ui->playlistButton);
    controlGroup->addButton(ui->musicButton);
    controlGroup->addButton(ui->searchButton);
    controlGroup->addButton(ui->volumeButton);
    controlGroup->addButton(ui->repeatButton);

    connect(repeatGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(repeatModeChanged(QAbstractButton*)));
    setRepeatGroupButtonsVisibility(false);

    initializePlaylistHeaders();
    ui->playlistTableView->setModel(model_);
    ui->playlistTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(api_, &ApiComponent::playlistReceived, this, &PlayerWidget::setPlaylist);
    connect(this, &PlayerWidget::playlistCleared, media_, &MediaComponent::clearPlaylist);
    connect(this, &PlayerWidget::playlistItemAdded, media_, &MediaComponent::addItemToPlaylist);

    connect(ui->startSearchingButton, &QPushButton::clicked, this, &PlayerWidget::search);
    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &PlayerWidget::search);

    connect(ui->myMusicButton, &QPushButton::clicked, api_, &ApiComponent::requestAuthUserPlaylist);
    connect(ui->suggestedButton, &QPushButton::clicked, api_, &ApiComponent::requestSuggestedPlaylist);
    connect(ui->popularButton, &QPushButton::clicked, this, &PlayerWidget::requestPopular);
    connect(ui->popularButton, &QPushButton::toggled, ui->genreComboBox, &QComboBox::setVisible);
    connect(ui->genreComboBox, &QComboBox::currentTextChanged, this, &PlayerWidget::requestedPopularByGenre);
    connect(this, &PlayerWidget::requestedPopularByGenre, api_, &ApiComponent::requestPopularPlaylistByGenre);

    connect(ui->volumeButton, &QPushButton::toggled, ui->volumeSlider, &QSlider::setVisible);
    connect(ui->volumeSlider, &QSlider::sliderMoved, this, &PlayerWidget::changeVolume);
    connect(ui->timeSlider, &QSlider::sliderMoved, this, &PlayerWidget::seek);

    connect(ui->musicButton, &QPushButton::toggled, this, &PlayerWidget::setMusicGroupButtonsVisibility);
    connect(ui->repeatButton, &QPushButton::toggled, this, &PlayerWidget::setRepeatGroupButtonsVisibility);
    connect(ui->searchButton, &QPushButton::toggled, this, &PlayerWidget::setSearchFormVisible);
    connect(ui->titleButton, &QPushButton::clicked, this, &PlayerWidget::selectCurrentPlayItem);

    connect(ui->playlistTableView, &QTableView::doubleClicked, this, &PlayerWidget::playIndex);
    connect(this, &PlayerWidget::startedPlaying, media_, &MediaComponent::playIndex);
    connect(media_->playlist(), &QMediaPlaylist::currentIndexChanged, this, &PlayerWidget::currentPlayItemChanged);
    connect(media_->player(), &QMediaPlayer::durationChanged, this, &PlayerWidget::durationChanged);
    connect(media_->player(), &QMediaPlayer::positionChanged, this, &PlayerWidget::positionChanged);
    connect(media_->player(), &QMediaPlayer::volumeChanged, this, &PlayerWidget::volumeChanged);
    connect(media_->player(), &QMediaPlayer::stateChanged, this, &PlayerWidget::stateChanged);
}

PlayerWidget::~PlayerWidget()
{
    delete ui;
}

void PlayerWidget::setPlaylist(const ApiComponent::Playlist& playlist)
{
    stillCurrentPlaylist_ = false;

    clear();

    foreach (const ApiComponent::PlaylistItem& item, playlist)
        addItem(item);
}

void PlayerWidget::clear()
{
    model_->removeRows(0, model_->rowCount());
    playlist_->clear();
}

void PlayerWidget::addItem(const ApiComponent::PlaylistItem &item)
{
    int const rowCount = model_->rowCount();
    model_->insertRow(rowCount);

    model_->setItem(rowCount, ApiComponent::Artist, new QStandardItem(item[ApiComponent::Artist]));
    model_->setItem(rowCount, ApiComponent::Title, new QStandardItem(item[ApiComponent::Title]));
    model_->setItem(rowCount, ApiComponent::Duration,
                    new QStandardItem(convertSecondsToTimeString(item[ApiComponent::Duration].toInt())));

    playlist_->addMedia(QUrl(item[ApiComponent::Url]));
}

void PlayerWidget::resetMusicGroupCheckState()
{
    ui->myMusicButton->setChecked(false);
    ui->suggestedButton->setChecked(false);
    ui->popularButton->setChecked(false);
}

QString PlayerWidget::convertSecondsToTimeString(int seconds)
{
    QTime time(seconds/3600, seconds/60, seconds % 60);
    QString const format = seconds > 3600 ? "hh:mm:ss" :"mm:ss";
    return time.toString(format);
}

void PlayerWidget::playIndex(const QModelIndex &index)
{
    if (!stillCurrentPlaylist_)
    {
        media_->copyModel(model_);
        media_->copyPlaylist(playlist_);
        stillCurrentPlaylist_ = true;
    }

    int const currentIndex = index.row();
    emit startedPlaying(currentIndex);
}

void PlayerWidget::setPlayItemTitle(const QString& title)
{
    ui->titleButton->setText(title);
    setWindowTitle(title + " - Flow");
}

void PlayerWidget::currentPlayItemChanged(int position)
{
    if (position == -1)
    {
        media_->pause();
        return;
    }

    QString const artist = media_->model()->item(position, ApiComponent::Artist)->text();
    QString const title = media_->model()->item(position, ApiComponent::Title)->text();

    if (stillCurrentPlaylist_)
        ui->playlistTableView->selectRow(position);

    QString const fullTitle = title + " by " + artist;
    setPlayItemTitle(fullTitle);
    QString const searchText = ui->byArtistButton->isChecked() ? artist : title;
    ui->searchEdit->setText(searchText);
}

void PlayerWidget::durationChanged(qint64 duration)
{
    ui->timeSlider->setMaximum(duration / 1000);
}

void PlayerWidget::positionChanged(qint64 progress)
{
    if (!ui->timeSlider->isSliderDown())
        ui->timeSlider->setValue(progress / 1000);

    updatePositionInfo(progress / 1000);
}

void PlayerWidget::updatePositionInfo(qint64 progress)
{
    QString const position = convertSecondsToTimeString(progress);
    QString const duration = convertSecondsToTimeString(media_->duration());

    ui->timeLabel->setText(position + " of " + duration);
}

void PlayerWidget::seek(int seconds)
{
    media_->setPosition(seconds * 1000);
}

void PlayerWidget::volumeChanged(int value)
{
    ui->volumeButton->setIcon(getVolumeIcon(value));

    if(!ui->volumeSlider->isSliderDown())
        ui->volumeSlider->setValue(value);
}

void PlayerWidget::changeVolume(int volume)
{
    ui->volumeButton->setIcon(getVolumeIcon(volume));
    media_->setVolume(volume);
}

void PlayerWidget::trySearch(QAbstractButton */* button */)
{
    if (!ui->searchEdit->text().isEmpty())
        search();
}

QIcon PlayerWidget::getVolumeIcon(int value)
{
    if (value == 0)
        return QIcon(":/icons/volumeoff.png");
    else if (value > 0 && value < 50)
        return QIcon(":/icons/volume25.png");
    else if (value >= 50 && value < 75)
        return QIcon(":/icons/volume50.png");
    else if (value >= 75)
        return QIcon(":/icons/volume75.png");
    else return QIcon(":/icons/volumeoff.png");
}

void PlayerWidget::initializePlaylistHeaders()
{
    QStringList headers;
    headers << "Artist" << "Title" << "Duration";
    model_->setHorizontalHeaderLabels(headers);
    ui->playlistTableView->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
}

void PlayerWidget::on_playPauseButton_clicked()
{
    QMediaPlayer::State const state = media_->state();
    if(state == QMediaPlayer::PlayingState)
        media_->pause();
    else if(state == QMediaPlayer::PausedState)
        media_->play();
    else media_->play();
}

void PlayerWidget::stateChanged(QMediaPlayer::State state)
{
    if(state == QMediaPlayer::PlayingState)
    {
        ui->playPauseButton->setIcon(QIcon(":/icons/pause.png"));
        ui->playPauseButton->setToolTip("Pause");
    }
    else if (state == QMediaPlayer::PausedState || state ==QMediaPlayer::StoppedState)
    {
        ui->playPauseButton->setIcon(QIcon(":/icons/play.png"));
        ui->playPauseButton->setToolTip("Play");
    }
}

void PlayerWidget::setRepeatGroupButtonsVisibility(bool visible)
{
    ui->repeatSingleButton->setVisible(visible);
    ui->repeatAllButton->setVisible(visible);
    ui->repeatOffButton->setVisible(visible);
}

void PlayerWidget::setMusicGroupButtonsVisibility(bool visible)
{
    ui->myMusicButton->setVisible(visible);
    ui->suggestedButton->setVisible(visible);
    ui->popularButton->setVisible(visible);

    if (visible && ui->popularButton->isChecked())
        ui->genreComboBox->setVisible(true);
    else
        ui->genreComboBox->setVisible(false);
}

void PlayerWidget::repeatModeChanged(QAbstractButton *button)
{
    if (button == ui->repeatSingleButton)
        media_->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    else if (button == ui->repeatAllButton)
        media_->setPlaybackMode(QMediaPlaylist::Loop);
    else if (button == ui->repeatOffButton)
        media_->setPlaybackMode(QMediaPlaylist::Sequential);
}

void PlayerWidget::on_shuffleButton_clicked(bool checked)
{
    if (checked)
        media_->setPlaybackMode(QMediaPlaylist::Random);
    else
        repeatModeChanged(repeatMode_);
}

void PlayerWidget::on_forwardButton_clicked()
{
    media_->next();
}

void PlayerWidget::on_rewindButton_clicked()
{
    media_->previous();
}

void PlayerWidget::selectCurrentPlayItem()
{
    ui->playlistButton->setChecked(true);
    ui->playlistTableView->clearSelection();
    int const currentIndex = media_->playlist()->currentIndex();
    if (currentIndex != -1)
        ui->playlistTableView->selectRow(currentIndex);
}

void PlayerWidget::setSearchFormVisible(bool visible)
{
    ui->byArtistButton->setVisible(visible);
    ui->byTitleButton->setVisible(visible);
    ui->searchEdit->setVisible(visible);
    ui->startSearchingButton->setVisible(visible);

    if(visible)
    {
        ui->searchEdit->selectAll();
        ui->searchEdit->setFocus();
        trySearch(NULL);
    }
}

void PlayerWidget::search()
{
    resetMusicGroupCheckState();
    ApiComponent::SearchQuery query;
    query.artist = ui->byArtistButton->isChecked() ? true : false;
    query.text = ui->searchEdit->text();
    api_->requestPlaylistBySearchQuery(query);
}

void PlayerWidget::requestPopular(bool checked)
{
    if(checked)
        emit requestedPopularByGenre(ui->genreComboBox->currentText());
}

void PlayerWidget::on_playlistButton_toggled(bool checked)
{
    if (checked)
    {
        stillCurrentPlaylist_ = true;
        ui->playlistTableView->setModel(media_->model());
        ui->playlistTableView->selectRow(media_->playlist()->currentIndex());
    }
    else
        ui->playlistTableView->setModel(model_);
}
