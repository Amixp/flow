#ifndef PLAYER_H
#define PLAYER_H

#include "apicomponent.h"
#include "mediacomponent.h"

#include <QLabel>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QSlider>
#include <QStyle>
#include <QWidget>

namespace Ui {
class PlayerWidget;
}

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = 0) : QLabel(parent) {}

    ~ClickableLabel() {}

signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent */*event*/)
    {
        emit clicked();
    }
};

class MouseDirectJumpSlider : public QSlider
{
    Q_OBJECT

public:
    explicit MouseDirectJumpSlider (QWidget *parent = 0) : QSlider(parent) {}

    ~MouseDirectJumpSlider() {}

protected:
    void mousePressEvent (QMouseEvent * event)
    {
        if (event->button() == Qt::LeftButton)
        {
            setValue(QStyle::sliderValueFromPosition(minimum(), maximum(), event->x(), width()));
            emit sliderMoved(value());
            event->accept();
        }

        QSlider::mousePressEvent(event);
    }
};

class PlayerWidget : public QWidget
{
    Q_OBJECT

    enum MusicMenu
    {
        CurrentPlaylist,
        MyMusic,
        SuggestedMusic,
        PopularMusic,
        SearchResults,
        Count
    };

    enum PlaylistColumn
    {
        ArtistAndTitle,
        Duration
    };

public:
    explicit PlayerWidget(MediaComponent *media, ApiComponent *api, QWidget *parent = 0);
    ~PlayerWidget();

public slots:
    void setPlaylist(const ApiComponent::Playlist& playlist);

private slots:
    void durationChanged(qint64 duration);

    void positionChanged(qint64 progress);

    void volumeChanged(int value);

    void stateChanged(QMediaPlayer::State state);

    void currentPlayItemChanged(int position);

    void playbackModeChanged(QAction *action);


    void playIndex(const QModelIndex& index);

    void updatePositionInfo(qint64 progress);

    void seek(int seconds);

    void changeVolume(int volume);

    void search();

    void solvePlaybackMode();

    void setSearchResultsMenuVisible(bool visible);


    void on_forwardButton_clicked();

    void on_rewindButton_clicked();

    void on_playPauseButton_clicked();


    void on_musicMenuListWidget_clicked(const QModelIndex &index);

    void on_musicSubMenuListWidget_clicked(const QModelIndex &index);

    void on_clearSearchTextButton_clicked();

signals:
    void playlistCleared();

    void startedPlaying(int index);

    void playlistItemAdded(const QUrl& url);

    void requestedPopularByGenre(const QString& genre);

private:

    void fillMusicSubMenuByGenres();

    void setPlayItemTitleText(const QString& title);

    void clear();

    void addItem(const ApiComponent::PlaylistItem& item);


    QString convertSecondsToTimeString(int seconds);

    Ui::PlayerWidget *ui;
    ApiComponent *api_;
    MediaComponent *media_;
    QStandardItemModel *model_;
    QMediaPlaylist *playlist_;
    bool stillCurrentPlaylist_;
};

#endif // PLAYER_H
