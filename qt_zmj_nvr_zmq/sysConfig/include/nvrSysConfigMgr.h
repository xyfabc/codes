#ifndef NVRSYSCONFIGMGR_H
#define NVRSYSCONFIGMGR_H

#include "camera.h"
#include <QDebug>

class NvrSysConfigMgr
{

public:
    NvrSysConfigMgr();
    ~NvrSysConfigMgr();
    static NvrSysConfigMgr *Inst();

public:
    void createSysConfig();
    void addSubCamara(int id, char *name, char *ip, char *account, char *passward, int chnId, int isRun);
    void GetSubCamara(int id);
    int getAllSubcamara(NET_DVR_CAMERAINFO *cameraInfo);
    int getNvrLocalInfo(NET_DVR_IPDEVINFO *nvrInfo);
    int setNvrLocalInfo(NET_DVR_IPDEVINFO *nvrInfo);
    int setNvrLocalInfo();

    int getNvrTitle(char *name , int *len);
    int setNvrTitle(char *name ,int size);
    NET_DVR_IPDEVINFO *getNvrInfo();
    NET_DVR_IPDEVINFO *getCacheNvrInfo();
    int setip(char *ip);
    char* getip(char *ip_buf);
     int resetSys();

private:
    NET_DVR_CAMERAINFO *cameraInfoList;
    NET_DVR_IPDEVINFO *pNvrInfo;
};

int xml_test();

#endif // NVRSYSCONFIGMGR_H
