QT -= core
QT -= gui

CONFIG += c++11

TARGET = lcd_init
CONFIG += console
CONFIG -= app_bundle

TARGET = lcd_init

INCLUDEPATH += ./devices/include/ \
INCLUDEPATH += ./include\

SOURCES += main.cpp \
    devices/framebuffer.cpp \
    devices/sample_comm_vo.c \
    devices/sample_comm_sys.c \
    devices/loadbmp.c \
    devices/logo.cpp

HEADERS += \
    devices/include/framebuffer.h \
    devices/include/hi_comm_vo.h \
    include/zmj_nvr_cfg.h \
    devices/include/hi_comm_sys.h \
    devices/loadbmp.h \
    devices/logo.h

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

DESTDIR = /home/eric/nfs/hi3536
