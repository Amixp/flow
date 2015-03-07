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

    enum Genres
    {
        Rock = 1,
        Pop = 2,
        RapAndHipHop = 3,
        EasyListening = 4,
        DanceAndHouse = 5,
        Instrumental = 6,
        Metal = 7,
        Alternative = 21,
        Dubstep = 8,
        JazzAndBlues = 9,
        DrumAndBass = 10,
        Trance = 11,
        Chanson = 12,
        Ethnic = 13,
        AcousticAndVocal = 14,
        Reggae = 15,
        Classical = 16,
        IndiePop = 17,
        Speech = 19,
        ElectropopAndDisco = 22,
        Other = 18
    };

    typedef QMap<OAuthTokens, QString> OAuthTokensMap;
    typedef QMap<PlaylistItemData, QString> PlaylistItem;
    typedef QList<PlaylistItem> Playlist;

    typedef QMap<QString, Genres> GenresMap;

    explicit ApiComponent(QObject *parent = 0);

    void setOAuthTokens(const OAuthTokensMap& tokens);
    const OAuthTokensMap& tokens() const;

signals:
    void authorizeFinished(bool successfully, const QString& error);
    void playlistReceived(const Playlist& playlist);

public slots:
    void getTokensFromUrl(const QUrl& url);
    void requestAuthUserPlaylist();
    void requestSuggestedPlaylist();
    void requestPopularPlaylistByGenre(const QString& genre);
    void requestPlaylistBySearchQuery(const SearchQuery& query);

private slots:
    void getPlaylistFromReply(QNetworkReply *reply);

private:
    void initializeGenresMap();

    void sendPlaylistRequest(const QString& request);

    OAuthTokensMap tokens_;
    GenresMap genres;
};

#endif // ApiComponent_H
