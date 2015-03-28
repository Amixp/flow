#include "playerwidget.h"
#include "ui_playerwidget.h"

#include <QDesktopWidget>
#include <QFileDialog>
#include <QButtonGroup>
#include <QMenu>
#include <QMessageBox>
#include <QMediaPlaylist>

static const int SYSTEM_TRAY_MESSAGE_TIMEOUT_HINT = 3000;
static const QSize ALBUM_ART_SIZE(512, 512);

PlayerWidget::PlayerWidget(MediaComponent *media, ApiComponent *api, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::PlayerWidget),
    api_(api), media_(media),
    model_(new QStandardItemModel(this)), playlist_(new QMediaPlaylist(this)),
    trayIcon_(new QSystemTrayIcon(this)),stillCurrentPlaylist_(false)
{
    Q_ASSERT(media);
    Q_ASSERT(api);

    ui->setupUi(this);

    initSystemTrayMenu();

    ui->musicSubMenuListWidget->clear();

    ui->playlistTableView->setModel(model_);
    ui->playlistTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    ui->playlistTableView->horizontalHeader()->setVisible(false);

    connect(media_, &MediaComponent::albumArtExtracted, ui->albumArtLabel, &QLabel::setPixmap);
    connect(api_, &ApiComponent::playlistReceived, this, &PlayerWidget::setPlaylist);
    connect(this, &PlayerWidget::playlistCleared, media_, &MediaComponent::clearPlaylist);
    connect(this, &PlayerWidget::playlistItemAdded, media_, &MediaComponent::addItemToPlaylist);

    connect(ui->searchEdit, &QLineEdit::returnPressed, this, &PlayerWidget::search);
    connect(ui->searchButton, &QPushButton::clicked, this, &PlayerWidget::search);

    connect(this, &PlayerWidget::requestedPopularByGenre, api_, &ApiComponent::requestPopularPlaylistByGenre);

    connect(ui->playPauseButton, &QPushButton::clicked, this, &PlayerWidget::solvePlayPauseAction);
    connect(ui->rewindButton, &QPushButton::clicked, this, &PlayerWidget::rewind);
    connect(ui->forwardButton, &QPushButton::clicked, this, &PlayerWidget::forward);

    connect(ui->albumArtLabel, &ClickableLabel::clicked, this, &PlayerWidget::showFullSizeAlbumArt);

    connect(ui->shuffleButton, &QPushButton::clicked, this, &PlayerWidget::solvePlaybackMode);
    connect(ui->loopButton, &QPushButton::clicked, this, &PlayerWidget::solvePlaybackMode);

    connect(ui->volumeSlider, &QSlider::sliderMoved, this, &PlayerWidget::changeVolume);
    connect(ui->timeSlider, &QSlider::sliderMoved, this, &PlayerWidget::seek);

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

void PlayerWidget::closeEvent(QCloseEvent *event)
{
    hide();
    trayIcon_->show();
    event->ignore();
}

void PlayerWidget::showFromTray(QSystemTrayIcon::ActivationReason reason)
{
    if (reason == QSystemTrayIcon::DoubleClick)
        show();
}

void PlayerWidget::show()
{
    trayIcon_->hide();
    QWidget::show();
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

    model_->setItem(rowCount, ArtistAndTitle, new QStandardItem(item[ApiComponent::Artist] + " - " + item[ApiComponent::Title]));
    QStandardItem* durationItem = new QStandardItem(convertSecondsToTimeString(item[ApiComponent::Duration].toInt()));
    durationItem->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    model_->setItem(rowCount, Duration, durationItem);

    playlist_->addMedia(QUrl(item[ApiComponent::Url]));
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

void PlayerWidget::showCurrentPlayItemText(const QString& title)
{
    ui->titleLabel->setText(title);
    trayIcon_->setToolTip(title);
    trayIcon_->showMessage("Now playing", title, QSystemTrayIcon::Information, SYSTEM_TRAY_MESSAGE_TIMEOUT_HINT);
}

void PlayerWidget::currentPlayItemChanged(int position)
{
    if (position == -1)
    {
        media_->pause();
        return;
    }

    QString const title = media_->model()->item(position, ArtistAndTitle)->text();

    if (stillCurrentPlaylist_)
        ui->playlistTableView->selectRow(position);

    showCurrentPlayItemText(title);
}

void PlayerWidget::playbackModeChanged(QAction *action)
{
    QString const playbackMode = action->text();
    if (playbackMode == "Shuffle")
        media_->setPlaybackMode(QMediaPlaylist::Random);
    if (playbackMode == "Repeat Single")
        media_->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    else if (playbackMode == "Repeat All")
        media_->setPlaybackMode(QMediaPlaylist::Loop);
    else if (playbackMode == "Repeat Off")
        media_->setPlaybackMode(QMediaPlaylist::Sequential);
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
    int const estimatedDuration = media_->duration() - progress;
    QString const duration = convertSecondsToTimeString(estimatedDuration);

    ui->durationLabel->setText("-" + duration);
}

void PlayerWidget::seek(int seconds)
{
    media_->setPosition(seconds * 1000);
}

void PlayerWidget::volumeChanged(int value)
{
    if(!ui->volumeSlider->isSliderDown())
        ui->volumeSlider->setValue(value);
}

void PlayerWidget::changeVolume(int volume)
{
    media_->setVolume(volume);
}

void PlayerWidget::solvePlayPauseAction()
{
    QMediaPlayer::State const state = media_->state();
    if(state == QMediaPlayer::PlayingState)
        media_->pause();
    else if(state == QMediaPlayer::PausedState)
        media_->play();
    else
    {
        QModelIndexList const selectedIndexes = ui->playlistTableView->selectionModel()->selectedIndexes();
        if (!selectedIndexes.isEmpty())
            playIndex(selectedIndexes.at(0));
    }
}

void PlayerWidget::rewind()
{
    media_->previous();
}

void PlayerWidget::forward()
{
    media_->next();
}

void PlayerWidget::stateChanged(QMediaPlayer::State state)
{
    QAction * const playPauseAction = trayIcon_->contextMenu()->actions().at(SystemTrayControl::PlayPause);
    QString const currentTitle = ui->titleLabel->text();

    if(state == QMediaPlayer::PlayingState)
    {
        ui->playPauseButton->setIcon(QIcon(":/icons/pause.png"));
        ui->playPauseButton->setToolTip("Pause");
        playPauseAction->setIcon(QIcon(":/icons/pause.png"));
        playPauseAction->setText("Pause");
        trayIcon_->setToolTip(currentTitle);
    }
    else if (state == QMediaPlayer::PausedState || state ==QMediaPlayer::StoppedState)
    {
        ui->playPauseButton->setIcon(QIcon(":/icons/play.png"));
        ui->playPauseButton->setToolTip("Play");
        playPauseAction->setIcon(QIcon(":/icons/play.png"));
        playPauseAction->setText("Play");
        trayIcon_->setToolTip(currentTitle + " [Paused]");
    }
}

void PlayerWidget::search()
{   
    ApiComponent::SearchQuery query;
    query.artist = false;
    query.text = ui->searchEdit->text();
    api_->requestPlaylistBySearchQuery(query);
    ui->playlistTableView->setModel(model_);

    if (ui->musicMenuListWidget->count() != MusicMenu::Count)
        setSearchResultsMenuVisible(true);

    ui->musicSubMenuListWidget->clear();
}

void PlayerWidget::solvePlaybackMode()
{
    bool const shuffle = ui->shuffleButton->isChecked();
    bool const loop = ui->loopButton->isChecked();

    if (shuffle && loop)
        media_->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    else if (shuffle && !loop)
        media_->setPlaybackMode(QMediaPlaylist::Random);
    else if (!shuffle && loop)
        media_->setPlaybackMode(QMediaPlaylist::CurrentItemInLoop);
    else media_->setPlaybackMode(QMediaPlaylist::Loop);
}

void PlayerWidget::setSearchResultsMenuVisible(bool visible)
{
    if (visible)
    {
        ui->musicMenuListWidget->insertItem(SearchResults, "Search results");
        ui->musicMenuListWidget->setCurrentRow(SearchResults);
    }
    else
    {
        QListWidgetItem *item = ui->musicMenuListWidget->item(SearchResults);
        if (item)
            delete item;
    }

}

void PlayerWidget::showFullSizeAlbumArt()
{
    QPixmap const * albumArt = ui->albumArtLabel->pixmap();
    if (!albumArt->size().isNull())
    {
        QLabel *albumArtLabel = new QLabel();
        albumArtLabel->setWindowTitle("[Album Art] " + ui->titleLabel->text());
        albumArtLabel->setWindowIcon(QIcon(":icons/logo.png"));
        albumArtLabel->setWindowModality(Qt::ApplicationModal);
        albumArtLabel->setScaledContents(true);
        albumArtLabel->setPixmap(*albumArt);
        albumArtLabel->setFixedSize(ALBUM_ART_SIZE);
        albumArtLabel->move(QApplication::desktop()->screen()->rect().center() - albumArtLabel->rect().center());
        albumArtLabel->show();
    }
}

void PlayerWidget::on_musicMenuListWidget_clicked(const QModelIndex &index)
{
    const int row = index.row();

    if (row != SearchResults)
    {
        setSearchResultsMenuVisible(false);
        ui->searchEdit->clear();
    }

    if (row != PopularMusic)
    {
        ui->musicSubMenuListWidget->clear();
        ui->musicSubMenuListWidget->setCurrentRow(-1);
    }

    if (row != CurrentPlaylist)
        ui->playlistTableView->setModel(model_);

    switch(index.row())
    {
    case CurrentPlaylist:
    {
        stillCurrentPlaylist_ = true;
        ui->playlistTableView->setModel(media_->model());
        ui->playlistTableView->selectRow(media_->playlist()->currentIndex());
        break;
    }
    case MyMusic:
    {
        api_->requestAuthUserPlaylist();
        break;
    }
    case SuggestedMusic:
    {
        api_->requestSuggestedPlaylist();
        break;
    }
    case PopularMusic:
    {
        fillMusicSubMenuByGenres();
        ui->musicSubMenuListWidget->setCurrentRow(0);
        api_->requestPopularPlaylistByGenre(ui->musicSubMenuListWidget->currentItem()->text());
        break;
    }
    default: break;
    }
}

void PlayerWidget::on_musicSubMenuListWidget_clicked(const QModelIndex &index)
{
    ui->musicMenuListWidget->setCurrentRow(PopularMusic);
    api_->requestPopularPlaylistByGenre(ui->musicSubMenuListWidget->item(index.row())->text());
}

void PlayerWidget::fillMusicSubMenuByGenres()
{
    ui->musicSubMenuListWidget->clear();
    ui->musicSubMenuListWidget->addItems(api_->genres().keys());
}

void PlayerWidget::on_clearSearchTextButton_clicked()
{
    ui->searchEdit->clear();
    ui->searchEdit->setFocus();
}

void PlayerWidget::initSystemTrayMenu()
{
    trayIcon_->setIcon(QIcon(":icons/logo.png"));
    trayIcon_->hide();

    QMenu *trayMenu = new QMenu(this);

    connect(trayIcon_, &QSystemTrayIcon::activated, this, &PlayerWidget::showFromTray);

    QAction *showAction = new QAction("Show", this);
    showAction->setIcon(QIcon(":icons/show.png"));
    connect(showAction, &QAction::triggered, this, &PlayerWidget::show);
    trayMenu->addAction(showAction);

    trayMenu->addSeparator();

    QAction *playPauseAction = new QAction("Play", this);
    playPauseAction->setIcon(QIcon(":icons/play.png"));
    connect(playPauseAction, &QAction::triggered, this, &PlayerWidget::solvePlayPauseAction);
    trayMenu->addAction(playPauseAction);

    QAction *rewindAction = new QAction("Rewind", this);
    rewindAction->setIcon(QIcon(":icons/rewind.png"));
    connect(rewindAction, &QAction::triggered, this, &PlayerWidget::rewind);
    trayMenu->addAction(rewindAction);

    QAction *forwardAction = new QAction("Forward", this);
    forwardAction->setIcon(QIcon(":icons/forward.png"));
    connect(forwardAction, &QAction::triggered, this, &PlayerWidget::forward);
    trayMenu->addAction(forwardAction);

    trayMenu->addSeparator();

    QAction *exitAction = new QAction("Exit", this);
    exitAction->setIcon(QIcon(":icons/exit.png"));
    connect(exitAction, &QAction::triggered, qApp, &QCoreApplication::exit);
    trayMenu->addAction(exitAction);

    trayIcon_->setContextMenu(trayMenu);
}
