#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "apicomponent.h"
#include "mediacomponent.h"
#include "playerwidget.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QMessageBox>
#include <QStyle>

const QString APP_ID = "4809611";
const QString PERMISSIONS = "audio,offline";
const QString REDIRECT_URI = "https://oauth.vk.com/blank.html";
const QString DISPLAY = "page";
const QString API_VERSION = "5.28";
const QString REVOKE = "1";

void setWidgetOnCenterScreen(QWidget *widget)
{
    Q_ASSERT(widget);

    widget->setGeometry(
                QStyle::alignedRect(
                    Qt::LeftToRight,
                    Qt::AlignCenter,
                    widget->size(),
                    qApp->desktop()->availableGeometry()
                    ));
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow), authWeb_(new QWebEngineView()), api_(new ApiComponent(this)), media_(new MediaComponent(this)),
    player_(new PlayerWidget(media_, api_))
{
    ui->setupUi(this);

    authWeb_->setAttribute(Qt::WA_DeleteOnClose);

    connect(api_, &ApiComponent::authorizeFinished, this, &MainWindow::processAuthResult);
    connect(api_, &ApiComponent::playlistReceived, player_, &PlayerWidget::setPlaylist);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_signInButton_clicked()
{
    hide();
    authWeb_->setWindowTitle("Flow Application Signing In");
    authWeb_->setWindowIcon(QIcon(":/icons/vkontakte.png"));

    connect(authWeb_, &QWebEngineView::urlChanged, api_, &ApiComponent::getTokensFromUrl);
    authWeb_->load(QUrl("https://oauth.vk.com/authorize?client_id=" + APP_ID + "&scope=" + PERMISSIONS +
                        "&redirect_uri=" + REDIRECT_URI + "&display=" + DISPLAY + "&v=" + API_VERSION +
                        "&revoke=" + REVOKE + "&response_type=token"));
    authWeb_->show();
    setWidgetOnCenterScreen(authWeb_);
}

void MainWindow::processAuthResult(bool result, const QString &error)
{
    authWeb_->close();

    if (result)
    {
        api_->requestAuthUserPlaylist();
        setWidgetOnCenterScreen(player_);
        player_->show();
    }
    else
    {
        QMessageBox::critical(this, "Flow Signing In Error", error, QMessageBox::Ok);
        qApp->exit();
    }
}
