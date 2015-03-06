#ifndef MEDIACOMPONENT_H
#define MEDIACOMPONENT_H

#include <QObject>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QStandardItemModel>

class MediaComponent : public QObject
{
    Q_OBJECT
public:
    explicit MediaComponent(QObject *parent = 0);

    void setPlayer(QMediaPlayer *player);
    void setPlaylist(QMediaPlaylist *playlist);

    void copyModel(QStandardItemModel *model);
    void copyPlaylist(QMediaPlaylist *playlist);

    QMediaPlayer * player() const;
    QMediaPlaylist * playlist() const;

    qint64 duration() const;

    QUrl url(int index) const;

    QMediaPlayer::State state() const;

    ~MediaComponent();

public slots:

    void play();
    void playIndex(int index);
    void stop();
    void pause();

    void next();
    void previous();

    void setVolume(int volume);
    void setPosition(int position);
    void setPlaybackMode(QMediaPlaylist::PlaybackMode mode);

    void addItemToPlaylist(const QUrl& url);
    void clearPlaylist();

private slots:
    void setDuration(qint64 duration);

private:

    QMediaPlayer *player_;
    QMediaPlaylist *playlist_;
    qint64 duration_;
    QStandardItemModel *model_;
};

#endif // MEDIACOMPONENT_H
