#-------------------------------------------------
#
# Project created by QtCreator 2015-03-03T11:28:43
#
#-------------------------------------------------

QT       += core gui webenginewidgets xml multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = flow
TEMPLATE = app

QMAKE_CXXFLAGS += -std=c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    mediacomponent.cpp \
    apicomponent.cpp \
    playerwidget.cpp

HEADERS  += mainwindow.h \
    mediacomponent.h \
    apicomponent.h \
    playerwidget.h

FORMS    += mainwindow.ui \
    playerwidget.ui

RESOURCES += \
    icons.qrc

DISTFILES += \
    README.md \
    CMakeLists.txt
