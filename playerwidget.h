#ifndef PLAYER_H
#define PLAYER_H

#include "apicomponent.h"
#include "mediacomponent.h"

#include <QLabel>
#include <QMouseEvent>
#include <QStandardItemModel>
#include <QSlider>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QWidget>

namespace Ui {
class PlayerWidget;
}

class QTreeWidgetItem;

class ClickableLabel : public QLabel
{
    Q_OBJECT

public:
    explicit ClickableLabel(QWidget *parent = 0) : QLabel(parent) {}

    ~ClickableLabel() {}

signals:
    void clicked(const QString&);

protected:
    void mousePressEvent(QMouseEvent */*event*/)
    {
        emit clicked(text());
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
        Artist,
        Title,
        Duration
    };

    enum SystemTrayControl
    {
        Show = 0,
        PlayPause = 2,
        Rewind = 3,
        Forward = 4,
        Exit = 6
    };

    enum SearchType
    {
        ByTitle,
        ByArtist
    };

public:
    explicit PlayerWidget(MediaComponent *media, ApiComponent *api, QWidget *parent = 0);
    ~PlayerWidget();

public slots:
    void show();

    void setPlaylist(const ApiComponent::Playlist& playlist);

protected:
    virtual void closeEvent(QCloseEvent *);

private slots:
    void showFromTray(QSystemTrayIcon::ActivationReason);

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

    void searchBySearch();

    void solvePlaybackMode();

    void solvePlayPauseAction();

    void rewind();

    void forward();


    void showFullSizeAlbumArt();

    void search(const QString& text, bool artist);

    void searchByArtist(const QString& artist = QString());

    void searchByTitle(const QString& title = QString());

    void changeSearchType(int type);

    void changePlaylistMenuMode();


    void on_clearSearchTextButton_clicked();

signals:
    void playlistCleared();

    void startedPlaying(int index);

    void playlistItemAdded(const QUrl& url);

    void requestedPopularByGenre(const QString& genre);

private:

    void initSystemTrayMenu();

    void showCurrentPlayItemText(const QString& artist, const QString& title);

    void clearPlaylist();

    void addItem(const ApiComponent::PlaylistItem& item);


    QString convertSecondsToTimeString(int seconds);

    Ui::PlayerWidget *ui;
    ApiComponent *api_;
    MediaComponent *media_;
    QStandardItemModel *model_;
    QMediaPlaylist *playlist_;
    QSystemTrayIcon *trayIcon_;
    bool stillCurrentPlaylist_;
};

#endif // PLAYER_H
