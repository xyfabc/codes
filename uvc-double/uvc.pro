#-------------------------------------------------
#
# Project created by QtCreator 2017-04-13T09:17:52
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = uvc
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    v4l2/demo.cxx \
    v4l2/v4l2.cxx \
    CapTh.cpp \
    TcYuvX.cpp

HEADERS  += mainwindow.h \
    v4l2/v4l2.h \
    v4l2/image.hpp \
    v4l2/uncopyable.hpp \
    CapTh.h \
    TcYuvX.h

FORMS    += mainwindow.ui
