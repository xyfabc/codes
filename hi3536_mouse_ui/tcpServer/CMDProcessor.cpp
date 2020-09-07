#include "CMDProcessor.h"
#include <stdio.h>      /*标准输入输出定义*/
#include <errno.h>      /*错误号定义*/
#include <string.h>      /*字符串相关*/
#include <fcntl.h>      /*文件控制定义*/
#include <unistd.h>     /*Unix 标准函数定义*/
#include "hisi_vdec.h"
#include  <sys/time.h>
#include  <QDebug>

pcmd_ack_send cmd_ack_send;
pzmq_cmd_ack_send zmq_cmd_ack_send;

MainWindow *mainWindow;

CMDProcessor::CMDProcessor()
{

}



void u32_u8_be(int nu, BYTE *buf)
{
    buf[0] = (BYTE)(nu >> 24);
    buf[1] = (BYTE)(nu >> 16);
    buf[2] = (BYTE)(nu >> 8);
    buf[3] = (BYTE)(nu >> 0);
}


int u8_u32_be(BYTE *buf)
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

int  buf2nvr(unsigned char *buf, NET_DVR_IPDEVINFO *pNvrInfo)
{
    unsigned char *ptr = buf;
    printf("pNvrInfo->cmaraChnNum = %d::sizeof(NET_DVR_IPDEVINFO) = %d\n",pNvrInfo->cmaraChnNum,sizeof(NET_DVR_IPDEVINFO));

   // memset(pNvrInfo,0,sizeof(NET_DVR_IPDEVINFO));


    pNvrInfo->cmaraChnNum =  u8_u32_be(ptr);
    ptr += 4;
//printf("pNvrInfo->cmaraChnNum = %d::sizeof(NET_DVR_IPDEVINFO) = %d\n",pNvrInfo->cmaraChnNum,sizeof(NET_DVR_IPDEVINFO));
    pNvrInfo->cameraNum =  u8_u32_be(ptr);
    ptr += 4;
    pNvrInfo->isLabelShow =  u8_u32_be(ptr);
    ptr += 4;
//printf("pNvrInfo->cameraNum = %d\n",pNvrInfo->cameraNum);
    memset(pNvrInfo->sPassword,0,NAME_LEN);
    memcpy(pNvrInfo->sUserName,ptr, NAME_LEN);
    ptr += NAME_LEN;
//printf("pNvrInfo->sUserName = %s\n",pNvrInfo->sUserName);
    memset(pNvrInfo->sPassword,0,PASSWD_LEN);
    memcpy( pNvrInfo->sPassword,ptr, PASSWD_LEN);
    ptr += PASSWD_LEN;
//printf("pNvrInfo->sPassword = %s\n",pNvrInfo->sPassword);
    memset(pNvrInfo->struIP.sIpV4,0,IP_ADDR_LEN);
    memcpy(pNvrInfo->struIP.sIpV4, ptr,IP_ADDR_LEN);
    ptr += IP_ADDR_LEN;
//printf("pNvrInfo->struIP.sIpV4 = %s\n",pNvrInfo->struIP.sIpV4);
    memset(pNvrInfo->sTitleName,0,TITEL_NAME_LEN);
    memcpy(pNvrInfo->sTitleName,ptr, TITEL_NAME_LEN);
    ptr += TITEL_NAME_LEN;
//printf("buf2nvr pNvrInfo->sTitleName = %s\n",pNvrInfo->sTitleName);
    //camera info
    for(int i=0;i<CAMERA_MAX_NUM;i++){
         NET_DVR_CAMERAINFO *pcamera  = pNvrInfo->cameraList+i;
         memset(pcamera,0,sizeof(NET_DVR_CAMERAINFO));
         pcamera->cameraID =  u8_u32_be(ptr);
         ptr += 4;

         pcamera->cmaraChnID =  u8_u32_be(ptr);
         ptr += 4;

         pcamera->isRun =  u8_u32_be(ptr);
         ptr += 4;

         memcpy(pcamera->sCameraName,ptr, NAME_LEN);
         ptr += NAME_LEN;

         memcpy(pcamera->sUserName,ptr, NAME_LEN);
         ptr += NAME_LEN;

         memcpy(pcamera->sPassword,ptr, PASSWD_LEN);
         ptr += PASSWD_LEN;

         memcpy(pcamera->struIP.sIpV4,ptr, IP_ADDR_LEN);
         ptr += IP_ADDR_LEN;
    }

    //return ((int)ptr - (int)buf);
}
int nvr_info2buf(NET_DVR_IPDEVINFO *pNvrInfo, unsigned char *buf)
{
    unsigned char *ptr = buf;

    //memcpy(ptr, pNvrInfo->cmaraChnNum, 4);
    u32_u8_be(pNvrInfo->cmaraChnNum,ptr);
    ptr += 4;
   // printf("pNvrInfo->cmaraChnNum = %d\n",pNvrInfo->cmaraChnNum);
    u32_u8_be(pNvrInfo->cameraNum,ptr);
    ptr += 4;
    u32_u8_be(pNvrInfo->isLabelShow,ptr);
    ptr += 4;
   // printf("pNvrInfo->cameraNum = %d\n",pNvrInfo->cameraNum);
    memcpy(ptr, pNvrInfo->sUserName, NAME_LEN);
    ptr += NAME_LEN;
 //printf("pNvrInfo->sUserName = %s\n",pNvrInfo->sUserName);
    memcpy(ptr, pNvrInfo->sPassword, PASSWD_LEN);
    ptr += PASSWD_LEN;
//printf("pNvrInfo->sPassword = %s\n",pNvrInfo->sPassword);
    memcpy(ptr, pNvrInfo->struIP.sIpV4, IP_ADDR_LEN);
    ptr += IP_ADDR_LEN;
//printf("pNvrInfo->struIP.sIpV4 = %s\n",pNvrInfo->struIP.sIpV4);
    memcpy(ptr, pNvrInfo->sTitleName, TITEL_NAME_LEN);
    ptr += TITEL_NAME_LEN;
printf("pNvrInfo->sTitleName = %s\n",pNvrInfo->sTitleName);
    //camera info
    for(int i=0;i<pNvrInfo->cameraNum;i++){
        //printf("-qq--line--%d--%d----NET_CMD_NVR_INFO\n",__LINE__,i);
        NET_DVR_CAMERAINFO *pcamera  = pNvrInfo->cameraList+i;
        u32_u8_be(pcamera->cameraID ,ptr);
        ptr += 4;

        u32_u8_be(pcamera->cmaraChnID,ptr);
        ptr += 4;

        u32_u8_be(pcamera->isRun,ptr);
        ptr += 4;

        memcpy(ptr, pcamera->sCameraName, NAME_LEN);
        ptr += NAME_LEN;

        memcpy(ptr, pcamera->sUserName, NAME_LEN);
        ptr += NAME_LEN;

        memcpy(ptr, pcamera->sPassword, PASSWD_LEN);
        ptr += PASSWD_LEN;

        memcpy(ptr, pcamera->struIP.sIpV4, 64);
        ptr += 64;
    }

//printf("-qq--line--%d------NET_CMD_NVR_INFO\n",__LINE__);
  //  return ((int)ptr - (int)buf);
}

void setCallbackFuntion(pcmd_ack_send cmd_ack)
{
  cmd_ack_send =  cmd_ack;
}
void setZMQSendCallbackFuntion(pzmq_cmd_ack_send zmq_cmd_ack)
{
  zmq_cmd_ack_send =  zmq_cmd_ack;
}
void set_mux_num(unsigned char num)
{
    if(mainWindow){
        mainWindow->setChnLine(num);
    }
    switch (num)
    {
    case SUB_CMD_1MUX_CHN:    //0x01
         printf("CMD_1MUX_CHN\n");
         ZMJ_COMM_VO_StartChn(0, VO_MODE_1MUX);
        break;
    case SUB_CMD_4MUX_CHN:    //0x01
         printf("CMD_4MUX_CHN\n");
         ZMJ_COMM_VO_StartChn(0, VO_MODE_4MUX);
        break;
    case SUB_CMD_6MUX_CHN:    //0x01
         printf("CMD_6MUX_CHN\n");
         ZMJ_COMM_VO_StartChn(0, VO_MODE_6MUX);
        break;
    case SUB_CMD_9MUX_CHN:    //0x01
         printf("SUB_CMD_9MUX_CHN\n");
         ZMJ_COMM_VO_StartChn(0, VO_MODE_9MUX);
        break;
    case SUB_CMD_16MUX_CHN:    //0x01
         printf("CMD_16MUX_CHN\n");
         ZMJ_COMM_VO_StartChn(0, VO_MODE_16MUX);
        break;
    }
}
int set_title_name(laserx_frame_t &frame_info)
{
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    char buf[128] = {0};
    memcpy(buf,(char*)frame_info.data,frame_info.len);
    std::string str = buf;
    //printf("--------%d-----%s\n",frame_info.len,frame_info.data);
    QString titleName = QString::fromStdString(str);
   // QString titleName = QString("%1").arg((char*)frame_info.data);
    mainWindow->setTitleName(titleName);
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getNvrInfo();
    memcpy(pNvrInfo->sTitleName,frame_info.data,frame_info.len);
    pNvrSysConfigMgr->setNvrLocalInfo();
    return 0;

}


int set_sys_time(laserx_frame_t &frame_info)
{
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    char buf[128] = {0};
    memcpy(buf,(char*)frame_info.data,frame_info.len);
    std::string str = buf;
    printf("-----sys time---%d-----%s\n",frame_info.len,buf);
    QString time = QString::fromStdString(str);
    struct tm tptr;
    struct timeval tv;

    tptr.tm_year = time.section(QRegExp("[-- ::]"), 0, 0).toInt()-1900;
    tptr.tm_mon = time.section(QRegExp("[-- ::]"), 1, 1).toInt()-1;
    tptr.tm_mday = time.section(QRegExp("[-- ::]"), 2, 2).toInt();
    tptr.tm_hour = time.section(QRegExp("[-- ::]"), 3, 3).toInt();
    tptr.tm_min = time.section(QRegExp("[-- ::]"), 4, 4).toInt();
    tptr.tm_sec = time.section(QRegExp("[-- ::]"), 5, 5).toInt();
        qDebug()<<  time;

    //2019-12-20 11:56:05

    qDebug()<<tptr.tm_year<<tptr.tm_mon<<tptr.tm_mday<<tptr.tm_hour<<tptr.tm_min<<tptr.tm_sec;
    tv.tv_sec = mktime(&tptr);
    tv.tv_usec = 0;
    settimeofday(&tv, NULL);
   // QString titleName = QString("%1").arg((char*)frame_info.data);
    return 0;

}
int get_title_name(unsigned char *send_buf)
{
    int len = 0;
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getNvrInfo();
    len = sizeof(pNvrInfo->sTitleName);
    memcpy(send_buf,pNvrInfo->sTitleName,sizeof(pNvrInfo->sTitleName));
   // pNvrSysConfigMgr->setNvrLocalInfo();
    return len;

}
//int set_chn_num(int chn,laserx_frame_t &frame_info)
//{
//    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
//    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getNvrInfo();
//    pNvrInfo->cmaraChnNum = (int)frame_info.data[0];
//    pNvrSysConfigMgr->setNvrLocalInfo();
//    set_mux_num(chn);
//    return 0;

//}
//int get_chn_num(int *chn)
//{
//    int len = 0;
//    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
//    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getNvrInfo();
//    *chn = pNvrInfo->cmaraChnNum;
//    return len;
//}
int set_nvr_all_info(laserx_frame_t &frame_info)
{
    int size;
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getNvrInfo();
    RTSPClientStop(pNvrInfo);
    size = buf2nvr(frame_info.data,pNvrInfo);
    printf("-------size =--%d-pNvrInfo->cmaraChnNum-%d-\n",size,pNvrInfo->cmaraChnNum);
    pNvrSysConfigMgr->setNvrLocalInfo();

    set_mux_num(pNvrInfo->cmaraChnNum);

    usleep(100*1000);


    mainWindow->setTitleCharName((char*)pNvrInfo->sTitleName);
    mainWindow->setChnLabelText();

    RTSPClientPlay(pNvrInfo);

    return size;
}
int update_nvr_all_info()
{
    int size;
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getNvrInfo();
    RTSPClientStop(pNvrInfo);
    //size = buf2nvr(frame_info.data,pNvrInfo);
   // printf("-------size =--%d-pNvrInfo->cmaraChnNum-%d-\n",size,pNvrInfo->cmaraChnNum);
  //  pNvrSysConfigMgr->setNvrLocalInfo();

    set_mux_num(pNvrInfo->cmaraChnNum);

    usleep(100*1000);


    mainWindow->setTitleCharName((char*)pNvrInfo->sTitleName);
    mainWindow->setChnLabelText();

    RTSPClientPlay(pNvrInfo);

    return size;
}
int get_nvr_all_info(unsigned char *send_buf)
{
    int len = 0;
    int i = 0;
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
     printf("-qq--line--%d------NET_CMD_NVR_INFO\n",__LINE__);
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getNvrInfo();
     printf("-qq--line--%d------NET_CMD_NVR_INFO\n",__LINE__);
    len = nvr_info2buf(pNvrInfo,send_buf);
    //memcpy(send_buf,pNvrInfo->sTitleName,sizeof(pNvrInfo->sTitleName));
   // pNvrSysConfigMgr->setNvrLocalInfo();
     printf("-qq--line--%d------NET_CMD_NVR_INFO\n",__LINE__);
    printf("send:(len = %d) ",len);
    for(i=0;i<30;i++){
        printf("0x%x ",send_buf[i]);
    }
    printf("\n");
    return len;
}
//int CmdProcess(int connfd, laserx_frame_t &frame_info)
//{
//    int ret = 0, cmd_ret;
//    int len;
//    unsigned char cmd, type;
//    cmd = frame_info.cmd;
//    type = frame_info.data[0];
//    len = frame_info.len;
//    unsigned char send_buf[5000] = {0};
//    NvrSysConfigMgr *pNvrSysConfigMgr;

//    printf("cmd = 0x%x\n", cmd);
//    cmd_ret = ERR_OK;
//    //memset(dataBuf, 0, DATA_BUF_LEN);
//    switch (cmd)
//    {
//    case NET_CMD_TEST:    //0x01
//        printf("NET_CMD_TEST\n");
//        cmd_ack_send(connfd, NET_CMD_TEST, ret, NULL, 0);
//        break;
//    case NET_CMD_NVR_SET_INFO:    //0x01
//         printf("-----------NET_CMD_NVR_INFO\n");
//         if(len>0){
//            ret = set_nvr_all_info(frame_info);
//            cmd_ack_send(connfd, NET_CMD_NVR_SET_INFO, ret, NULL, 0);
//         }
//        break;
//    case NET_CMD_NVR_GET_INFO:    //0x01
//         printf("NET_CMD_NVR_GET_INFO\n");
//        // ret = get_nvr_all_info(send_buf);
//         printf("NET_CMD_NVR_GET_INFO len = %d\n",ret);
//          // cmd_ack_send(connfd, NET_CMD_NVR_GET_INFO, ret, send_buf, ret);
//        break;
////    case NET_CMD_SET_MUX:    //0x01
////        printf("set CMD_1MUX_CHN\n");
////        if(len>0){
////            send_buf[0] = type;
////            set_chn_num(type,frame_info);
////            cmd_ack_send(connfd, NET_CMD_SET_MUX, ret, send_buf, 1);
////        }
////        break;
////    case NET_CMD_GET_MUX:    //0x01
////        printf("get NET_CMD_GET_MUX\n");
////        int chn;
////        get_chn_num(&chn);
////        send_buf[0] = chn;
////        set_mux_num(type);
////        cmd_ack_send(connfd, NET_CMD_SET_MUX, ret, send_buf, 1);

////        break;
//    case NET_CMD_SET_TITLE:    //0x01
//        printf("set NET_CMD_TITLE\n");
//        if(len>0){
//            set_title_name(frame_info);

//            cmd_ack_send(connfd, NET_CMD_SET_TITLE, ret, NULL, 0);
//        }
//        break;
//    case NET_CMD_SET_TIME:    //0x01
//        printf("set NET_CMD_TITLE\n");
//        if(len>0){
//            set_sys_time(frame_info);

//            cmd_ack_send(connfd, NET_CMD_SET_TIME, ret, NULL, 0);
//        }
//        break;
//    case NET_CMD_SYS_REBOOT:    //0x01
//        printf("NET_CMD_SYS_REBOOT\n");
//        cmd_ack_send(connfd, NET_CMD_SYS_REBOOT, ret, NULL, 0);
//        sleep(1);
//        system("reboot");
//        break;
//    case NET_CMD_SYS_RESET:
//        printf("NET_CMD_SYS_RESET\n");
//        pNvrSysConfigMgr = NvrSysConfigMgr::Inst();

//        pNvrSysConfigMgr->resetSys();
//        cmd_ack_send(connfd, NET_CMD_SYS_RESET, ret, NULL, 0);

//        sleep(1);
//        system("reboot");
//        break;
//    case NET_CMD_GET_TITLE:    //0x01
//        printf("get  NET_CMD_TITLE\n");
//         len = get_title_name(send_buf);
//        printf("-------------%s\n",send_buf);
//        cmd_ack_send(connfd, NET_CMD_GET_TITLE, ret, send_buf, len);

//        break;
//    }
//}
int saveUpdateFile(char *filename,char *update_data,int size)
{
    if(NULL == update_data){
        printf("update_data is NULL\n");
        return -1;
    }
    FILE * outfile= fopen(filename, "wb" );
    if(outfile == NULL){
        printf("not exit\n");
        return -1;
    }
    fwrite( update_data, size, sizeof( unsigned char ), outfile );
    fclose(outfile);
printf("save file ok %d\n",size);
    return 0;
}

char *update_data = NULL;
int update_len = 0;
int update_count = 0;

int ZMQCmdProcess( void *responder, zframe_t *identity,laserx_frame_t &frame_info)
{
    int ret = 0, cmd_ret;
    unsigned char cmd, type;
    int len = 0;


    cmd = frame_info.cmd;
    type = frame_info.data[0];
    len = frame_info.len;

    unsigned char send_buf[5000] = {0};
    NvrSysConfigMgr *pNvrSysConfigMgr;

    printf("video cmd = 0x%x\n", cmd);
    cmd_ret = ERR_OK;
    //memset(dataBuf, 0, DATA_BUF_LEN);
    switch (cmd)
    {
    case NET_CMD_TEST:    //0x01
        printf("NET_CMD_TEST\n");
         zmq_cmd_ack_send(responder,identity, NET_CMD_TEST, ret, send_buf, 0);
        break;
    case NET_CMD_NVR_SET_INFO:    //0x01
         printf("-----------NET_CMD_NVR_INFO\n");
         if(len>0){
            ret = set_nvr_all_info(frame_info);
            zmq_cmd_ack_send(responder,identity, NET_CMD_NVR_SET_INFO, ret, send_buf, 0);
         }
        break;
    case NET_CMD_NVR_GET_INFO:    //0x01
         printf("NET_CMD_NVR_GET_INFO\n");
         ret = get_nvr_all_info(send_buf);
         printf("NET_CMD_NVR_GET_INFO len = %d\n",ret);
         zmq_cmd_ack_send(responder,identity, NET_CMD_NVR_GET_INFO, ret, send_buf, ret);
        break;
    case NET_CMD_SET_TIME:    //0x01
        printf("set NET_CMD_TITLE\n");
        if(len>0){
            set_sys_time(frame_info);
            zmq_cmd_ack_send(responder,identity, NET_CMD_SET_TIME, ret, send_buf, 0);
        }
        break;
    case NET_CMD_SYS_REBOOT:    //0x01
        printf("NET_CMD_SYS_REBOOT\n");
        zmq_cmd_ack_send(responder,identity, NET_CMD_SYS_REBOOT, ret, send_buf, 0);
        sleep(1);
        system("reboot");
        break;
    case NET_CMD_SYS_RESET:
        printf("NET_CMD_SYS_RESET\n");
        pNvrSysConfigMgr = NvrSysConfigMgr::Inst();

        pNvrSysConfigMgr->resetSys();
        zmq_cmd_ack_send(responder,identity, NET_CMD_SYS_RESET, ret, send_buf, 0);
        sleep(1);
        system("reboot");
        break;
    case NET_CMD_SYS_UPDATE_HEAD:    //0x01
        printf("get  NET_CMD_SYS_UPDATE_HEAD\n");
         //len = get_title_name(send_buf);
        update_len = (frame_info.data[3]<<24)|(frame_info.data[2]<<16)|(frame_info.data[1]<<8)|frame_info.data[0];
        if(NULL==update_data){
            update_data = (char *)malloc(update_len);
            update_count = 0;
        }
        printf("--------%d-----%d::%d\n",len,(frame_info.data[0]<<8)|frame_info.data[1],(frame_info.data[3]<<24)|(frame_info.data[2]<<16)|(frame_info.data[1]<<8)|frame_info.data[0]);
        zmq_cmd_ack_send(responder,identity, NET_CMD_SYS_UPDATE_HEAD, ret, send_buf, 0);

        break;
    case NET_CMD_SYS_UPDATE_DATA:    //0x01
        //printf("get  NET_CMD_SYS_UPDATE_DATA\n");
        if(NULL!=update_data){
            memcpy(update_data+update_count,frame_info.data,len);
            update_count += len;
        }
        //printf("-----get---len = %d-----update_count = %d\n",len,update_count);
        zmq_cmd_ack_send(responder,identity, NET_CMD_SYS_UPDATE_DATA, ret, send_buf, 0);

        break;
    case NET_CMD_SYS_UPDATE_END:    //0x01
        printf("get  NET_CMD_SYS_UPDATE_END\n");
         //len = get_title_name(send_buf);
        printf("-----%d--%d------%s\n",update_count,update_len,send_buf);
        if(update_count == update_len){
            printf(" NET_CMD_SYS_UPDATE_END  recv all data\n");
        }
        saveUpdateFile("qt_zmj_nvr_net",update_data,update_count);
        if(NULL!=update_data){
            free(update_data);
        }
        zmq_cmd_ack_send(responder,identity, NET_CMD_SYS_UPDATE_END, ret, send_buf, 0);

        break;

    }

}


int ZMQIpcCmdProcess( void *responder, zframe_t *identity,laserx_frame_t &frame_info)
{
    int ret = 0, cmd_ret;
    unsigned char cmd, type;
    int len = 0;


    cmd = frame_info.cmd;
    type = frame_info.data[0];
    len = frame_info.len;

    unsigned char send_buf[5000] = {0};
    NvrSysConfigMgr *pNvrSysConfigMgr;

    printf("video cmd = 0x%x\n", cmd);
    update_nvr_all_info();
    cmd_ret = ERR_OK;
    //memset(dataBuf, 0, DATA_BUF_LEN);
    switch (cmd)
    {
    case NET_CMD_TEST:    //0x01
        printf("NET_CMD_TEST\n");
         zmq_cmd_ack_send(responder,identity, NET_CMD_TEST, ret, send_buf, 0);
        break;

    }

}


void setMainwindow(MainWindow *w)
{
   mainWindow = w;
}
