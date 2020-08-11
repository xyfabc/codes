#-------------------------------------------------
#
# Project created by QtCreator 2020-08-06T10:45:27
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = rtspServer
TEMPLATE = app


# imx6ull A7平台
contains(CONFIG, ubuntu) {
    PLATFORM = ubuntu
}

# hi3536 海思视频处理器平台
contains(CONFIG, hi3536) {
    PLATFORM = hi3536
}



INCLUDEPATH += ./live/include/BasicUsageEnvironment/include \
                ./live/include/groupsock/include \
                ./live/include/liveMedia/include \
                ./live/include/UsageEnvironment/include \



SOURCES += main.cpp\
    DynamicRTSPServer.cpp \
    H264FramedLiveSource.cpp \
    H265LiveVideoServerMediaSubssion.cpp \
    itcRtspClient.cpp \
    Workthread.cpp

HEADERS  += DynamicRTSPServer.hh \
    H264FramedLiveSource.hh \
    H265LiveVideoServerMediaSubssion.hh \
    itcRtspClient.hpp \
    version.hh \
    Workthread.h \



LIBS+= -L./live/$$PLATFORM/lib \
        -ldl \
        -lpthread \
        -lliveMedia \
        -lgroupsock  \
        -lBasicUsageEnvironment \
        -lUsageEnvironment \


#FORMS    += mainwindow.ui
