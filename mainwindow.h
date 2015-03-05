#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "apicomponent.h"
#include "playerwidget.h"
#include "mediacomponent.h"

#include <QMainWindow>
#include <QWebEngineView>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_signInButton_clicked();
    void processAuthResult(bool result, const QString& error);

private:

    Ui::MainWindow *ui;
    QWebEngineView *authWeb_;
    ApiComponent *api_;
    MediaComponent *media_;
    PlayerWidget *player_;
};

#endif // MAINWINDOW_H
