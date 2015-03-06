#include "apicomponent.h"

#include <QDomDocument>

ApiComponent::ApiComponent(QObject *parent) : QObject(parent)
{
    initializeGenresMap();
}

void ApiComponent::setOAuthTokens(const ApiComponent::OAuthTokensMap &tokens)
{
    tokens_ = tokens;
}

const ApiComponent::OAuthTokensMap& ApiComponent::tokens() const
{
    return tokens_;
}

void ApiComponent::downloadPlaylistItemByUrl(const QUrl &url)
{
    QNetworkAccessManager * manager = new QNetworkAccessManager(this);
    connect(manager, &QNetworkAccessManager::finished, this, &ApiComponent::downloadCompleted);
    manager->get(QNetworkRequest(url));
}


void ApiComponent::getTokensFromUrl(const QUrl& url)
{
    QString const urlString = url.toString();

    if (urlString.contains("error"))
        emit authorizeFinished(false, urlString.section("error_description=", 1));
    else if(urlString.contains("access_token"))
    {
        //!TODO =([^&]*)&?
        // ([\w]*)=([^&]*)&? <value, key>
        int const s_access = urlString.indexOf('=', 0) + 1;
        int const s_expires_in = urlString.indexOf('=', s_access) + 1;
        int const s_userid = urlString.indexOf('=', s_expires_in) + 1;

        int const e_access = urlString.indexOf('&', 0) + 1;
        int const e_expires_in = urlString.indexOf('&', e_access) + 1;
        int const e_userid = -1;

        tokens_[AccessToken] = urlString.mid(s_access, e_access - s_access - 1);
        tokens_[ExpiresIn] = urlString.mid(s_expires_in, e_expires_in - s_expires_in - 1);
        tokens_[UserId] = urlString.mid(s_userid, e_userid);

        emit authorizeFinished(true, QString());
    }
}

void ApiComponent::getPlaylistFromReply(QNetworkReply *reply)
{
    QDomDocument domDocument;
    domDocument.setContent(reply->readAll());

    QDomElement responseElement = domDocument.firstChildElement(); //! <response list = true>
    QDomNode itemNode = responseElement.firstChildElement(); //! <audio>

    Playlist playlist;

    while(!itemNode.isNull())
    {
        PlaylistItem data;
        QString const artist = itemNode.toElement().elementsByTagName("artist").item(0).toElement().text();
        QString const title = itemNode.toElement().elementsByTagName("title").item(0).toElement().text();
        QString const duration = itemNode.toElement().elementsByTagName("duration").item(0).toElement().text();
        QString const url = itemNode.toElement().elementsByTagName("url").item(0).toElement().text();

        if (!artist.isEmpty() && !title.isEmpty() && !duration.isEmpty() && !url.isEmpty())
        {
            data[Artist] = artist;
            data[Title] = title;
            data[Duration] = duration;
            data[Url] = url;

            playlist.push_back(data);
        }

        itemNode = itemNode.nextSibling();
    }

    reply->deleteLater();

    emit playlistReceived(playlist);
}

void ApiComponent::initializeGenresMap()
{
    genres["Rock"] = Rock;
    genres["Pop"] = Pop;
    genres["Rap & Hip-hop"] = RapAndHipHop;
    genres["Easy Listening"] = EasyListening;
    genres["Dance & House"] = DanceAndHouse;
    genres["Instrumental"] = Instrumental;
    genres["Metal"] = Metal;
    genres["Alternative"] = Alternative;
    genres["Dubstep"] = Dubstep;
    genres["Jazz & Blues"] = JazzAndBlues;
    genres["Drum & Bass"] = DrumAndBass;
    genres["Trance"] = Trance;
    genres["Chanson"] = Chanson;
    genres["Ethnic"] = Ethnic;
    genres["Acoustic & Vocal"] = AcousticAndVocal;
    genres["Reggae"] = Reggae;
    genres["Classical"] = Classical;
    genres["Indie Pop"] = IndiePop;
    genres["Speech"] = Speech;
    genres["Electropop & Disco"] = ElectropopAndDisco;
    genres["Other"] = Other;
}

void ApiComponent::requestAuthUserPlaylist()
{
    Q_ASSERT(tokens_.contains(UserId));

    QNetworkAccessManager * networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiComponent::getPlaylistFromReply);
    QNetworkRequest request("https://api.vk.com/method/audio.get.xml?uid=" +
                            tokens_[UserId] + "&access_token=" + tokens_[AccessToken]);

    QNetworkReply *reply = networkManager->get(request);
}

void ApiComponent::requestSuggestedPlaylist()
{
    QNetworkAccessManager * networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiComponent::getPlaylistFromReply);
    QNetworkRequest request("https://api.vk.com/method/audio.getRecommendations.xml?uid=" +
                            tokens_[UserId] + "&access_token=" + tokens_[AccessToken] + "&count=500");

    QNetworkReply *reply = networkManager->get(request);
}

void ApiComponent::requestPopularPlaylistByGenre(const QString &genre)
{
    QNetworkAccessManager * networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiComponent::getPlaylistFromReply);
    QNetworkRequest request("https://api.vk.com/method/audio.getPopular.xml?uid=" +
                            tokens_[UserId] + "&access_token=" + tokens_[AccessToken] +
                            + "&genre_id=" + QString::number(genres[genre]) + "&count=500");

    QNetworkReply *reply = networkManager->get(request);
}

void ApiComponent::requestPlaylistBySearchQuery(const ApiComponent::SearchQuery &query)
{
    QNetworkAccessManager * networkManager = new QNetworkAccessManager(this);
    connect(networkManager, &QNetworkAccessManager::finished, this, &ApiComponent::getPlaylistFromReply);
    QNetworkRequest request("https://api.vk.com/method/audio.search.xml?uid=" +
                            tokens_[UserId] + "&access_token=" + tokens_[AccessToken] +
                             "&performer_only=" + QString::number(query.artist) +
                            "&q=" + query.text + "&count=300");

    QNetworkReply *reply = networkManager->get(request);
}
