#include "mediacomponent.h"

MediaComponent::MediaComponent(QObject *parent) : QObject(parent), player_(new QMediaPlayer(this)),
    playlist_(new QMediaPlaylist(this)), duration_(0), model_(new QStandardItemModel(this))
{
    player_->setPlaylist(playlist_);
    setVolume(100);
    setPlaybackMode(QMediaPlaylist::Loop);

    connect(player_, &QMediaPlayer::durationChanged, this, &MediaComponent::setDuration);
}

void MediaComponent::setPlayer(QMediaPlayer *player)
{
    Q_ASSERT(player);
    player_ = player;
}

void MediaComponent::setPlaylist(QMediaPlaylist *playlist)
{
    Q_ASSERT(playlist);
    playlist_ = playlist;
    player_->setPlaylist(playlist);
}

void MediaComponent::copyModel(QStandardItemModel *model)
{
    Q_ASSERT(model);

    model_->removeRows(0, model_->rowCount());

    for (int i = 0 ; i < model->rowCount() ; i++)
        for(int j = 0; j < model->columnCount(); ++j)
            model_->setItem(i, j, model->item(i, j)->clone());
}

void MediaComponent::copyPlaylist(QMediaPlaylist *playlist)
{
    Q_ASSERT(playlist);

    playlist_->clear();

    for (int i = 0; i < playlist->mediaCount(); ++i)
        playlist_->addMedia(playlist->media(i));
}

QMediaPlayer*  MediaComponent::player() const
{
    return player_;
}

QMediaPlaylist* MediaComponent::playlist() const
{
    return playlist_;
}

QStandardItemModel *MediaComponent::model() const
{
    return model_;
}

qint64 MediaComponent::duration() const
{
    return duration_;
}

QUrl MediaComponent::url(int index) const
{
    return playlist_->media(index).canonicalUrl();
}

QMediaPlayer::State MediaComponent::state() const
{
    return player_->state();
}

MediaComponent::~MediaComponent()
{
}

void MediaComponent::play()
{
    player_->play();
}

void MediaComponent::playIndex(int index)
{
    playlist_->setCurrentIndex(index);
    play();
}

void MediaComponent::stop()
{
    player_->stop();
}

void MediaComponent::pause()
{
    player_->pause();
}

void MediaComponent::next()
{
    playlist_->next();
}

void MediaComponent::previous()
{
    playlist_->previous();
}

void MediaComponent::setVolume(int volume)
{
    player_->setVolume(volume);
}

void MediaComponent::setPosition(int position)
{
    player_->setPosition(position);
}

void MediaComponent::setPlaybackMode(QMediaPlaylist::PlaybackMode mode)
{
    playlist_->setPlaybackMode(mode);
}

void MediaComponent::addItemToPlaylist(const QUrl &url)
{
    playlist_->addMedia(url);
}

void MediaComponent::clearPlaylist()
{
    playlist_->clear();
}

void MediaComponent::setDuration(qint64 duration)
{
    duration_ = duration / 1000;
}
