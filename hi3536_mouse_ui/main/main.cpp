#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QFontDatabase>
#include <QObject>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <pthread.h>
#include "nvrSysConfigMgr.h"
#include "camera.h"
#include "watchdog.h"
#include "framebuffer.h"
#include "hifb.h"
#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vb.h"


#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_hdmi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_region.h"
#include "sample_comm.h"
#include "rtspclient.h"

#include "tcpServer.h"
#include <QTextCodec>
#include "zmj_nvr_cfg.h"
#include <QDir>
#include <QtPlugin>
#include <QPluginLoader>
#include "nvr_watchdog.h"

#include "../PluginApp/Main/interfaceplugin.h"

#include "widget.h"
#include  "glmainwindow.h"
#include "zmjwindows.h"

#include "QVideoWidget.h"
#include <QApplication>
#include <QGridLayout>
#include "QVideoThread.h"
#include <QList>
#include <QDebug>



int main(int argc, char *argv[])
{
    // QApplication a(argc, argv);



    //必须添加，支持中文 for Qt 5.5.1
    //  QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    // QTextCodec::setCodecForLocale(codec);

    //必须添加，支持中文 for Qt 4.8.3
    //QTextCodec::setCodecForCStrings(QTextCodec::codecForName("UTF-8"));
    //QTextCodec::setCodecForLocale(QTextCodec::codecForName("UTF-8"));
    // QTextCodec::setCodecForTr(QTextCodec::codecForName("UTF-8"));

    //get local configs
    NET_DVR_IPDEVINFO *pNvrInfo;

    //启动看门狗
    // watchDogInit();
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();

    // pNvrSysConfigMgr->resetSys();

    //get local config
    pNvrInfo = pNvrSysConfigMgr->getNvrInfo();

    char buf[16]="";
    // pNvrSysConfigMgr->setip(pNvrInfo->struIP.sIpV4);
    printf("ipaddr===%s\n",pNvrSysConfigMgr->getip(buf));
    char ip_addr[128] = {0};
    //    sprintf(ip_addr, "ifconfig eth1 %s netmask 255.255.255.0 up", pNvrInfo->struIP.sIpV4);
    //    system(ip_addr);


    //hisi framebuffer init
//    hifb_init();
//    SAMPLE_HIFB_StandardMode();
//    sleep(10);
//    while(1){
//        sleep(1);
//    }

#if 1
    //hisi video init
   // vdec_init(ZMJ_VDEC_MAX_CHN,(SAMPLE_VO_MODE_E)pNvrInfo->cmaraChnNum);

   // RTSPClientPlay(pNvrInfo);

    // TCP server
    //tcpServerInit();

    // next is Qt UI
    printf("---------------------1111-------------mainWindow show-\n");
    QApplication a(argc, argv);
    int id = QFontDatabase::addApplicationFont("/mnt/nfs/qt-hisi/lib/fonts/DroidSansFallback.ttf");
    QString msyh = QFontDatabase::applicationFontFamilies (id).at(0);
    QFont font(msyh,10);
    qDebug()<<msyh;
    font.setPointSize(20);
    a.setFont(font);
QApplication::setOverrideCursor(Qt::BlankCursor);
    //    QSurfaceFormat fmt;
    //    fmt.setDepthBufferSize(24);

    //    if (QOpenGLContext::openGLModuleType() == QOpenGLContext::LibGL) {
    //        qDebug("Requesting 3.3 compatibility context");
    //        fmt.setVersion(3, 3);
    //        fmt.setProfile(QSurfaceFormat::CompatibilityProfile);
    //    } else {
    //        qDebug() << fmt.majorVersion();
    //        qDebug() << fmt.minorVersion();
    //        qDebug("Requesting 3.0 context");
    //        fmt.setVersion(2, 0);
    //    }



    MainWindow w;
    std::string str = (char*)pNvrInfo->sTitleName;
    QString titleName = QString::fromStdString(str);
    w.setTitleName(titleName);
    setMainwindow(&w);
    w.show();
    w.resize(1920, 1080);
    w.setChnLabelText();

  //  SAMPLE_HIFB_StandardMode();
    return a.exec();

#else




#endif




}
//./configure -prefix /home/eric/Works/drivers/hi3536/qtlib/qt-5.12-install -opensource -confirm-license -release -strip -linuxfb -qt-zlib -no-gif -qt-libpng -qt-libjpeg -qt-freetype -no-rpath -no-pch -no-avx -no-openssl -no-cups -no-dbus -no-pkg-config -xplatform linux-hi3536-g++ -make libs -eglfs -opengl es2 -no-glib -nomake examples -nomake tools  -qt-sqlite
//./configure -prefix  /opt/hi3536/qt-5.12 -opensource -confirm-license -release -strip -linuxfb -qt-zlib -no-gif -qt-libpng -qt-libjpeg -qt-freetype -no-rpath -no-pch -no-avx -no-openssl -no-cups -no-dbus -no-pkg-config -xplatform linux-hi3536-g++ -make libs -eglfs -opengl es2 -no-glib -nomake examples -nomake tools  -qt-sqlite  -skip qtserialbus  -skip qtwebview -skip qtwebengine -skip qtlocation -skip qtwebsockets -skip qtwebchannel-skip qtgamepad -skip qtandroidextras
//-skip qtmacextras \
//-skip qtx11extras \
//-skip qtsensors \
//-skip qtserialbus \
//-skip qtserialport \
//-skip qtwebengine \
//-skip qtwebchannel \
//-skip qtwebsockets \
//-skip qtlocation \
//-skip qtquickcontrols \
//-skip qtpurchasing \
//-skip qtconnectivity \
//-skip qtscxml \
//-skip qtxmlpatterns \
//-skip qtnetworkauth \
//-skip qtspeech \
//-skip qtscript \
//-skip qtremoteobjects \
//-skip qtcharts \
//-skip qtdatavis3d \
//-skip qtwebview

