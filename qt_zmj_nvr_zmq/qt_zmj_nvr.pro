#-------------------------------------------------
#
# Project created by QtCreator 2019-11-19T13:19:47
#
#-------------------------------------------------

QT       += core gui xml

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = qt_zmj_nvr
TEMPLATE = app
CXXFLAGS = -fpermissive

#include dir
INCLUDEPATH += ./include\
INCLUDEPATH += ./devices \
INCLUDEPATH += ./live555/include \
INCLUDEPATH += ./live555/include/liveMedia/include \
INCLUDEPATH += ./live555/include/groupsock/include \
INCLUDEPATH += ./live555/include/BasicUsageEnvironment/include \
INCLUDEPATH += ./live555/include/UsageEnvironment/include \
INCLUDEPATH += ./devices/include/ \
INCLUDEPATH += ./tcpServer/include/ \
INCLUDEPATH += ./sysConfig/include/ \
INCLUDEPATH += ./main/include \

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main/main.cpp \
        main/mainwindow.cpp \
        devices/sample_comm_sys.c \
        devices/sample_comm_vo.c \
        #src/loadbmp.c \
        devices/sample_comm_vdec.c \
        devices/sample_comm_vpss.c \
        devices/sample_vdec.c \
        devices/camera.cpp \
        tcpServer/tcpServer.cpp \
        tcpServer/laserx.c \
        sysConfig/nvrSysConfigMgr.cpp \
        devices/framebuffer.cpp \
        tcpServer/CMDProcessor.cpp \
        live555/RTSPClient.cpp \
   #     common/loadbmp.c \
         main/streamDaemon.cpp \
  #  tcpServer/worker.c \
  #  tcpServer/zeroMQServer.c \
  #  tcpServer/asyncsrv.cpp
    devices/nvr_watchdog.cpp \
    devices/pinger.cpp


HEADERS += \
        main/include/mainwindow.h \
        devices/include/sample_comm.h \
        devices/include/hifb.h \
        devices/include/hi_tde_api.h \
        devices/include/hi_tde_type.h \
        devices/include/hi_type.h \
        devices/include/hisi_vdec.h \
        live555/include/rtspClient.h \
        live555/include/BasicUsageEnvironment/include/BasicUsageEnvironment.hh \
        live555/include/liveMedia/include/liveMedia.hh \
        devices/include/framebuffer.h \
        devices/include/camera.h \
        tcpServer/include/tcpServer.h \
        tcpServer/include/laserx.h \
        tcpServer/include/nvrserverparatype.h \
        sysConfig/include/nvrSysConfigMgr.h \
        include/zmj_nvr_cfg.h \
        tcpServer/include/CMDProcessor.h \
        main/include/streamDaemon.h \
    #tcpServer/include/worker.h \
    #tcpServer/asyncsrv.h
    devices/include/nvr_watchdog.h \
    devices/include/watchdog.h \
    devices/pinger.h


#QTPLUGIN += qlinuxfb

FORMS += \
        main/mainwindow.ui

LIBS+= -L./lib \
        -ldl \
        -lmpi \
        -lpthread \
        -ldnvqe \
        -lupvqe \
        -lVoiceEngine \
        -ltde \
        -lhdmi \
        -lliveMedia \
        -lgroupsock  \
        -lBasicUsageEnvironment \
        -lUsageEnvironment \
        -lzmq \
        -lczmq \
        -luuid \

RESOURCES += \
    res/zmj_nvr_res.qrc

DESTDIR = /home/eric/nfs/hi3536
