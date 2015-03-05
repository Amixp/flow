#include "mediacomponent.h"

MediaComponent::MediaComponent(QObject *parent) : QObject(parent), player_(new QMediaPlayer(this)),
    playlist_(new QMediaPlaylist(this)), duration_(0)
{
    player_->setPlaylist(playlist_);
    setVolume(100);
    setPlaybackMode(QMediaPlaylist::Sequential);

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

QMediaPlayer*  MediaComponent::player() const
{
    return player_;
}

QMediaPlaylist* MediaComponent::playlist() const
{
    return playlist_;
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
