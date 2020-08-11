#ifndef CMDPROCESSOR_H
#define CMDPROCESSOR_H

#include "laserx.h"
#include "mainwindow.h"
#include "nvrSysConfigMgr.h"
//#include "hisi_vdec.h"
//#include <pthread.h>
#include "rtspclient.h"
#include "czmq.h"
#include <zmq.h>

#ifdef __cplusplus
extern "C" {
#endif


typedef  int (*pcmd_ack_send)(int fd, int cmd, int err, void *data, int len);
typedef  int (*pzmq_cmd_ack_send)(void *responder, zframe_t *identity,int cmd, int err, void *data, int len);

#define NET_CMD_TEST            0x00
#define NET_CMD_NVR_SET_INFO        0x01
#define NET_CMD_NVR_GET_INFO        0x02

#define NET_CMD_SET_MUX             0x03
#define NET_CMD_GET_MUX             0x04

#define NET_CMD_SET_TITLE           0x05
#define NET_CMD_GET_TITLE           0x06

#define NET_CMD_GET_TIME           0x07
#define NET_CMD_SET_TIME           0x08


#define NET_CMD_SYS_REBOOT          0x09

#define NET_CMD_SYS_RESET          0x0A

#define NET_CMD_SYS_UPDATE_HEAD          0x0B
#define NET_CMD_SYS_UPDATE_DATA          0x0C
#define NET_CMD_SYS_UPDATE_END          0x0D


#define SUB_CMD_1MUX_CHN 			0x01
#define SUB_CMD_4MUX_CHN 		    0x04
#define SUB_CMD_6MUX_CHN 		    0x06
#define SUB_CMD_9MUX_CHN 		    0x09
#define SUB_CMD_16MUX_CHN 		    0x10

extern int CmdProcess(int connfd, laserx_frame_t &frame_info);

//设置网络发送回调函数
extern  void setCallbackFuntion(pcmd_ack_send cmd_ack);
void setZMQSendCallbackFuntion(pzmq_cmd_ack_send zmq_cmd_ack);
int ZMQCmdProcess(void *responder, zframe_t *identity, laserx_frame_t &frame_info);
int ZMQIpcCmdProcess( void *responder, zframe_t *identity,laserx_frame_t &frame_info);
extern void setMainwindow(MainWindow *w);

class CMDProcessor
{
public:
    CMDProcessor();
};


#ifdef __cplusplus
}
#endif

#endif // CMDPROCESSOR_H
