#ifndef TCPSERVER_H
#define TCPSERVER_H

#include "laserx.h"
#include "CMDProcessor.h"

#define NVR_SERVER_IP   "192.168.8.37"
#define NVR_SERVER_PORT   8000


typedef void (*FtcpServerRun)(void *param);
class TcpServer
{
public:
    TcpServer();


public:
  //  int tcpServerInit();


private:
  //  int hw_net_send(int fd, void *data, int len);
  //  int hw_net_receive(int fd, void *buf, int len);
   // int cmd_register_callback(int type, cmd_callback_t *cb);
    //int cmd_ack(int fd, int cmd, int err, void *data, int len);
   // int hw_net_accept(int fd, char *client_addr);
    //void tcpServerRun(void *param);
   // int tcpServerCreate(char *ip_addr, unsigned short port1, int mode);

private:

};

//#define HW_DEBUG
#ifdef HW_DEBUG
#define DBG_PRINTF(s,...)			printf(s, ##__VA_ARGS__)
#else
#define DBG_PRINTF(s,...)
#endif



#define MAX_LEN  1024
#define DATA_BUF_LEN        1024*10240
#define RECV_BUF_LEN        1024*10240








int tcpServerCreate(char *ip_addr, unsigned short port1, int mode);
int CmdProcess(int connfd,laserx_frame_t &frame_info);

int tcpServerInit();



#endif // TCPSERVER_H
