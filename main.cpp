#include "mainwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QStyle>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    setWidgetOnCenterScreen(&w);
    w.show();

    return a.exec();
}
