#-------------------------------------------------
#
# Project created by QtCreator 2013-07-18T12:52:00
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = aprs
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui

INCLUDEPATH += /usr/include/QtMultimediaKit /usr/include/QtMobility

LIBS += -lQtMultimediaKit
