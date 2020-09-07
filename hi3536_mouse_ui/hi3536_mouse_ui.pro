#-------------------------------------------------
#
# Project created by QtCreator 2019-11-19T13:19:47
#
#-------------------------------------------------

QT       += core gui xml opengl

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
INCLUDEPATH += ./main/3D \
INCLUDEPATH += ./main/ \
INCLUDEPATH += ./tools/ \

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
        main/streamDaemon.cpp \
        devices/nvr_watchdog.cpp \
        devices/pinger.cpp \
        main/widget.cpp \
        main/3D/glwidget.cpp \
        main/3D/objloader.cpp \
        main/3D/glmainwindow.cpp \
        main/zmjwindows.cpp \
        tools/TcYuvX.cpp \
        tools/write_bmp_func.c \
        main/lcdform.cpp \
        main/QVideoWidget.cpp \
        main/QVideoThread.cpp \
        main/CameraWidget.cpp \
    main/openglwindow.cpp \
    sample_hifb.c \
    devices/loadbmp.c


HEADERS += \
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
        devices/include/nvr_watchdog.h \
        devices/include/watchdog.h \
        devices/pinger.h \
        main/widget.h \
        main/3D/glwidget.h \
        main/3D/objloader.h \
        main/include/mainwindow.h \
        main/3D/glmainwindow.h \
        main/zmjwindows.h \
        tools/TcYuvX.h \
        tools/write_bmp_func.h \
        main/lcdform.h \
        main/QVideoWidget.h \
        main/QVideoThread.h \
        main/CameraWidget.h \
    main/openglwindow.h \
    devices/loadbmp.h


#QTPLUGIN += qlinuxfb

FORMS += \
        main/mainwindow.ui \
    main/lcdform.ui \
    main/QVideoWidget.ui \
    main/CameraWidget.ui

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
        -live \

RESOURCES += \
    res/zmj_nvr_res.qrc \
    main/3D/obj.qrc


DESTDIR = /home/eric/nfs/hi3536
