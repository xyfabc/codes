#include "streamDaemon.h"
#include <QDebug>
#include "nvrSysConfigMgr.h"
#include "hisi_vdec.h"
#include "rtspclient.h"
#include "camera.h"
#include <QProcess>
#include "pinger.h"
#include <QDateTime>
#include "mainwindow.h"

#include <QImage>
#include <QRgb>
#include <QFileInfo>
#include <QDir>
#include "tools/TcYuvX.h"
#include "tools/write_bmp_func.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

#define IMG_W 1920
#define IMG_H 1080

static MainWindow *mainWindow;

StreamDaemon::StreamDaemon()
{
    pbuf=(unsigned char*)malloc(1920*1080*4);
    flag = 1;
    chnId = 0;
}

bool StreamDaemon::ping_ip(QString ip)
{
    int exitCode;

    QString strArg = "ping -s 1 -c 1 " + ip;        //linux平台下的格式

    exitCode = QProcess::execute(strArg);
    if(0 == exitCode){
        //it's alive
        qDebug() << "shell ping " + ip + " sucessed!";
        return true;
    }else{
        qDebug() << "shell ping " + ip + " failed!";
        return false;
    }
}


void StreamDaemon::restartStream()
{
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getCacheNvrInfo();

    qDebug()<<"---------restartStream----------";
    for(int i = 0;i<pNvrInfo->cameraNum;i++){

        NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+i;
        int chn = pCamera->cmaraChnID;
        int status = getChnStatus(pCamera->cmaraChnID);
        int isRun = pCamera->isRun;
        if((isRun==1)&&(status==0)){
            // qDebug()<<i<<"-----------die-----------"<<chn<<"isRun"<<isRun<<status;
            RTSPClientStopOneCamera(pCamera);
            // qDebug()<<i<<"-----------die---ok---1-----"<<chn<<"isRun"<<isRun<<status;
            enableUserPic(pCamera->cmaraChnID);
            //qDebug()<<i<<"-----------die---ok---11-----"<<chn<<"isRun"<<isRun<<status;
            QString ip = QString("%1").arg(pCamera->struIP.sIpV4);
            // if(pinger.ping(ip)){
            if(pinger.ping_ip(pCamera->struIP.sIpV4,1)){
                //if(ping_ip(ip)){
                qDebug()<<chn<<"ping ok";
                sleep(1);
                RTSPClientRestartOneCamera(pCamera);
            }else{
                qDebug()<<chn<<"ping err";
            }
            // qDebug()<<i<<"-----------die---ok--------"<<chn<<"isRun"<<isRun<<status;
        }else{

            if(isRun==1){
                clearChnStatus(pCamera->cmaraChnID);
                //qDebug()<<chn<<"live";
            }

        }

    }

}



bool StreamDaemon::CapturePicture(int cameraId)
{
    char file_name[64] ={0};
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);
    // qDebug()<<"signal------"<<__LINE__;
    if(pbuf)
    {
        if(GetDataForAlg(pbuf,cameraId,&flag))
        {
            //             sprintf(file_name,"./img/test--%02d_%02d_%02d_%02d.yuv",t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

            //             FILE *fp = fopen(file_name, "wb");
            //             if(fp==NULL) return (BMPERR_INTERNAL_ERROR);
            //             int j = 0;
            //             for(j=0;j<1920*1080*3/2;j++){
            //                 fwrite(pbuf+j, 1, 1, fp);
            //             //	printf("j: %d, p[%d]: %d\n", j, j, p[j]);
            //             }
            //             fclose(fp);
            flag = 0;
            // qDebug()<<"signal------"<<0;
            emit UpdateImgData(pbuf,1920,1080,&flag,cameraId);
            return true;
        }
    }
    return false;
}

bool StreamDaemon::CaptureRGBPicture(const QString& filename)
{
    char file_name[64] ={0};
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    qDebug()<<"pbuf signal-----------"<<chnId;
    if(pbuf)
    {
        if(GetGBADataForView(pbuf,chnId,&flag))
        {
            // sprintf(file_name,"./img/test--%02d_%02d_%02d_%02d.bgr",t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

            //            FILE *fp = fopen(file_name, "wb");
            //            if(fp==NULL) return (BMPERR_INTERNAL_ERROR);
            //            int j = 0;
            //            for(j=0;j<1920*1080*3/2;j++){
            //                fwrite(pbuf+j, 1, 1, fp);
            //            //	printf("j: %d, p[%d]: %d\n", j, j, p[j]);
            //            }
            //            fclose(fp);
            flag = 0;
            qDebug()<<"signal-----------"<<chnId;
            //  emit UpdateImgData(pbuf,1920,1080,&flag);
            return true;
        }
        qDebug()<<"else signal-----------"<<chnId;
    }
    return false;
}

bool StreamDaemon::getYuv()
{
    qDebug()<<"else signal-----------"<<chnId;
}

bool StreamDaemon::getAllCameraYuv()
{

}

bool StreamDaemon::CapturePictureAndSave(const QString& filename)
{
    char file_name[64] ={0};
    struct tm *t;
    time_t tt;
    time(&tt);
    t = localtime(&tt);

    unsigned char* pbuf=(unsigned char*)malloc(1920*1080*3);
    if(pbuf)
    {
        if(GetDataForAlg(pbuf,0,&flag))
        {
            sprintf(file_name,"./img/test--%02d_%02d_%02d_%02d.yuv420sp",t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);

            FILE *fp = fopen(file_name, "wb");
            if(fp==NULL) return (BMPERR_INTERNAL_ERROR);
            int j = 0;
            for(j=0;j<1920*1080*3/2;j++){
                fwrite(pbuf+j, 1, 1, fp);
                //	printf("j: %d, p[%d]: %d\n", j, j, p[j]);
            }
            fclose(fp);

            QImage image(pbuf,1920,1080,QImage::Format_Indexed8);
            QVector<QRgb> grayTable;
            unsigned int rgb=0;
            for(int i= 0; i< 256;i++)
            {
                grayTable.append(rgb);
                rgb+=0x010101;
            }
            image.setColorTable(grayTable);
            if(!QDir().exists(QFileInfo(filename).absolutePath()))
                QDir().mkpath(QFileInfo(filename).absolutePath());
            image.save(filename,"JPG");
            return true;
        }
        free(pbuf);
    }
    return false;
}

static int u8_u32_be(BYTE *buf)
{
    int nu;

    nu = buf[0];
    nu <<= 8;
    nu |= buf[1];
    nu <<= 8;
    nu |= buf[2];
    nu <<= 8;
    nu |= buf[3];

    return nu;
}

char read_fifo_name[]="/home/zmj/mouse_pos_fifo";

void StreamDaemon::run()
{
    int read_fd;
    read_fd = open(read_fifo_name,O_RDONLY);
    if(read_fd<0)
    {
        perror("open_r");
        exit(-1);
    }

    while(1){
        char bufr[64] = "";
        int ret;

        ret = read(read_fd, bufr, sizeof(bufr));
        if (ret <= 0){
            printf("\r--read err----\n");
        }
        if(ret>0){
            //printf("\r--read %d----\n",ret);
            int x = u8_u32_be((BYTE *)&bufr[1]);
            int y = u8_u32_be((BYTE *)&bufr[5]);
//            printf("\r %c x = %d:y = %d  %c\n",bufr[0],x,y,bufr[9]);
//            fflush(stdout);
            QCursor::setPos(x, y);
        }
    }

}

void StreamDaemon::setMainwindow(MainWindow *w)
{
    mainWindow = w;
}

void StreamDaemon::setChnId(int chnId)
{
    this->chnId = chnId;
}
