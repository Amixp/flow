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
    model_(new QStandardItemModel(this)),
    repeatMode_(NULL)
{
    Q_ASSERT(media);
    Q_ASSERT(api);

    ui->setupUi(this);

    repeatMode_ = ui->repeatOffButton;

    ui->volumeSlider->setVisible(false);

    QButtonGroup *playlistGroup = new QButtonGroup(this);
    playlistGroup->addButton(ui->myMusicButton);
    playlistGroup->addButton(ui->suggestedButton);
    playlistGroup->addButton(ui->popularButton);
    setPlaylistGroupButtonsVisibility(true);
    connect(ui->playlistButton, &QPushButton::toggled, playlistGroup, &QButtonGroup::setExclusive);

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
    controlGroup->addButton(ui->searchButton);
    controlGroup->addButton(ui->volumeButton);
    controlGroup->addButton(ui->repeatButton);

    connect(repeatGroup, SIGNAL(buttonClicked(QAbstractButton*)), this, SLOT(repeatModeChanged(QAbstractButton*)));
    setRepeatGroupButtonsVisibility(false);

    initializePlaylistHeaders();
    ui->playlistTableView->setModel(model_);
    ui->playlistTableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    connect(api_, &ApiComponent::playlistReceived, this, &PlayerWidget::setPlaylist);
    connect(api_, &ApiComponent::downloadCompleted, this, &PlayerWidget::replyFinishedDownload);
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

    connect(ui->playlistButton, &QPushButton::toggled, this, &PlayerWidget::setPlaylistGroupButtonsVisibility);
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
    clearPlaylist();

    foreach (const ApiComponent::PlaylistItem& item, playlist)
        addItemToPlaylist(item);

    if (media_->playlist()->isEmpty())
        setPlayItemTitle("Playlist is empty");
    else
        setPlayItemTitle("");
}

void PlayerWidget::clearPlaylist()
{
    model_->clear();
    initializePlaylistHeaders();

    emit playlistCleared();
}

void PlayerWidget::addItemToPlaylist(const ApiComponent::PlaylistItem &item)
{
    int const rowCount = model_->rowCount();
    model_->insertRow(rowCount);

    model_->setItem(rowCount, ApiComponent::Artist, new QStandardItem(item[ApiComponent::Artist]));
    model_->setItem(rowCount, ApiComponent::Title, new QStandardItem(item[ApiComponent::Title]));

    int const duration = item[ApiComponent::Duration].toInt();
    model_->setItem(rowCount, ApiComponent::Duration, new QStandardItem(convertSecondsToTimeString(duration)));

    emit playlistItemAdded(QUrl(item[ApiComponent::Url]));
}

void PlayerWidget::resetPlaylistGroupCheckState()
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
    QString const artist = model_->item(position, ApiComponent::Artist)->text();
    QString const title = model_->item(position, ApiComponent::Title)->text();
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

void PlayerWidget::on_downloadButton_clicked()
{
    QModelIndexList selectedRows = ui->playlistTableView->selectionModel()->selectedRows();

    if(!selectedRows.isEmpty())
    {
        int const index = model_->itemFromIndex(selectedRows.at(0))->row();
        api_->downloadPlaylistItemByUrl(media_->url(index));
        ui->playlistTableView->setSelectionMode(QTableView::NoSelection);
    }
}

void PlayerWidget::replyFinishedDownload(QNetworkReply *reply)
{
    if (reply->error())
        QMessageBox::critical(this, "Flow Downloading Error", reply->errorString(), QMessageBox::Ok);
    else
    {
        QString const directory = QFileDialog::getExistingDirectory(this, "Save audio in", QDir::homePath());
        if (!directory.isEmpty())
        {
            int const currentRow = model_->itemFromIndex(ui->playlistTableView->selectionModel()->selectedRows().at(0))->row();
            QString const filename = directory + "/" + model_->item(currentRow, ApiComponent::Artist)->text() +
                    "-" + model_->item(currentRow, ApiComponent::Title)->text() + ".mp3";

            QFile file(filename);
            if (file.open(QIODevice::ReadWrite))
            {
                file.write(reply->readAll());
                file.flush();
                file.close();

                QMessageBox::information(this, "Flow Downloading Complete", "File " + filename + " was saved successfully!");
            }
            else
                QMessageBox::critical(this, "Flow Saving File Error", file.errorString(), QMessageBox::Ok);
        }
    }

    reply->deleteLater();
    ui->playlistTableView->setSelectionMode(QTableView::SingleSelection);
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

void PlayerWidget::setPlaylistGroupButtonsVisibility(bool visible)
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
    }
}

void PlayerWidget::search()
{
    resetPlaylistGroupCheckState();
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
