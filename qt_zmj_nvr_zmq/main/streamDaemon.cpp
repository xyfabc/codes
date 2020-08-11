#include "streamDaemon.h"
#include <QDebug>
#include "nvrSysConfigMgr.h"
#include "hisi_vdec.h"
#include "rtspclient.h"
#include "camera.h"
#include <QProcess>
#include "pinger.h"

StreamDaemon::StreamDaemon()
{

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

void StreamDaemon::run()
{
    while(1){
        sleep(5);
        restartStream();
    }

}
