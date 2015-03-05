#ifndef ApiComponent_H
#define ApiComponent_H

#include <QObject>
#include <QNetworkReply>

class ApiComponent : public QObject
{
    Q_OBJECT

public:

    struct SearchQuery
    {
        bool artist;
        QString text;
    };

    enum OAuthTokens
    {
        AccessToken,
        ExpiresIn,
        UserId
    };

    enum PlaylistItemData
    {
        Artist,
        Title,
        Duration,
        Url
    };

    typedef QMap<OAuthTokens, QString> OAuthTokensMap;
    typedef QMap<PlaylistItemData, QString> PlaylistItem;
    typedef QList<PlaylistItem> Playlist;

    explicit ApiComponent(QObject *parent = 0);

    void setOAuthTokens(const OAuthTokensMap& tokens);
    const OAuthTokensMap& tokens() const;

    void downloadPlaylistItemByUrl(const QUrl& url);

signals:
    void authorizeFinished(bool successfully, const QString& error);
    void playlistReceived(const Playlist& playlist);
    void downloadCompleted(QNetworkReply *reply);

public slots:
    void getTokensFromUrl(const QUrl& url);
    void requestAuthUserPlaylist();
    void requestPlaylistBySearchQuery(const SearchQuery& query);

private slots:
    void getPlaylistFromReply(QNetworkReply *reply);

private:
    OAuthTokensMap tokens_;
};

#endif // ApiComponent_H
