#include "tcpServer.h"
#include "laserx.h"

#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <net/if.h>
#include <sys/select.h>
#include <unistd.h>     /*Unix 标准函数定义*/
#include <stdio.h>      /*标准输入输出定义*/
#include <errno.h>      /*错误号定义*/
#include <string.h>      /*字符串相关*/
#include <fcntl.h>      /*文件控制定义*/
#include <sys/types.h>
#include <sys/ioctl.h>
#include "hisi_vdec.h"
#include <pthread.h>

#include "worker.h"


TcpServer::TcpServer()
{

}



/*=============================================================
 文件名称：tcp_echo_srv.c
 功能描述：TCP Echo Server  循环接收TCP发送来的数据,并将接收到的数据回发
 维护记录：2008-12-25  V1.0
           2011-08-11  v1.1
           2013-02-19  v1.2
 使用方法: ./tcp_echo_srv 8001(或者 ./tcp_echo_srv)
 telnet客户端关闭方法: 第一步: ctrl+]  第二步:quit
===============================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <assert.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <zmq.h>
#include "czmq.h"

int zmq_cmd_ack(void *responder, zframe_t *identity, int cmd, int err, void *data, int len)
{
    int ret;
    laserx_frame_t frame;
    u8 send_data[4096] = {0};
    int size = 0;
    frame.cmd = (unsigned char)cmd;
    frame.data = (u8 *)data;
    frame.format = LX_FORMAT_BIN;
    frame.len = len;
    frame.ret = (unsigned char)err;

    //laserx_enframe(&frame,send_data,&size,0);

    // zframe_t *zidentity = zframe_new (data, 5+len);
    zframe_t *content = zframe_new (send_data, size);

    zframe_send (&identity, responder, ZFRAME_REUSE + ZFRAME_MORE);
    ret = zframe_send (&content, responder, ZFRAME_REUSE);

    zframe_destroy (&identity);
    zframe_destroy (&content);
    return ret;
}


static void server_worker (void *ctx);

void zmj_server_task ()
{
    printf("------%d-------\n",__LINE__);
    //  Frontend socket talks to clients over TCP
    void *ctx = zmq_ctx_new ();
    void *frontend = zmq_socket (ctx, ZMQ_ROUTER);
    char buf[1024] = {0};
    int i=0;
    zmq_bind (frontend, "tcp://*:5560");

    //  Backend socket talks to workers over inproc
    void *backend = zmq_socket (ctx, ZMQ_DEALER);
    zmq_bind (backend, "inproc://backend");

    //  Launch pool of worker threads, precise number is not critical
    int thread_nbr;
    for (thread_nbr = 0; thread_nbr < 1; thread_nbr++){
        // ;//zthread_fork (ctx, server_worker, NULL);
        //zmq_threadstart (server_worker, ctx);
        pthread_t		tcpServerWorkerId;
        //pthread_create(&tcpServerWorkerId,NULL,server_worker,ctx);

    }

    //  Connect backend to frontend via a proxy
    zmq_proxy (frontend, backend, NULL);

    zmq_ctx_destroy (&ctx);
    return ;
}

static void
server_worker (void *ctx)
{
    printf("------%d-------\n",__LINE__);
    void *worker = zmq_socket (ctx, ZMQ_DEALER);
    zmq_connect (worker, "inproc://backend");
    char buf[1024] = {0};
    laserx_frame_t frame_info;
    memset(&frame_info, 0, sizeof(laserx_frame_t));
    int i=0;

    while (true) {
        zmq_pollitem_t items [] = { { worker, 0, ZMQ_POLLIN, 0 } };
        int rc = zmq_poll (items, 1, 10 * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;              //  Interrupted

        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (worker);

            zframe_t *identity = zmsg_pop (msg);
           // zframe_print (identity, 0);
            //printf("------%d-------\n",__LINE__);
            zframe_t *content = zmsg_pop (msg);
           // zframe_print (content, 0);
            // memcpy(buf,zframe_data(content),zframe_size (content));
            laserx_deframe(zframe_data(content),zframe_size (content), 0, &frame_info);
            ZMQCmdProcess(worker,identity,frame_info);
            //zframe_destroy (&identity);
            zframe_destroy (&content);
        }
    }
}


static void server_ipc_worker (void *ctx);
void zmj_ipc_server_task ()
{
    printf("------%d-------\n",__LINE__);
    //  Frontend socket talks to clients over TCP
    void *ctx = zmq_ctx_new ();
    void *frontend = zmq_socket (ctx, ZMQ_ROUTER);
    char buf[1024] = {0};
    int i=0;
    zmq_bind (frontend, "tcp://127.0.0.1:5580");

    //  Backend socket talks to workers over inproc
    void *backend = zmq_socket (ctx, ZMQ_DEALER);
    zmq_bind (backend, "inproc://backend");

    //  Launch pool of worker threads, precise number is not critical
    int thread_nbr;
    for (thread_nbr = 0; thread_nbr < 1; thread_nbr++){
        // ;//zthread_fork (ctx, server_worker, NULL);
        //zmq_threadstart (server_worker, ctx);
        pthread_t		tcpServerWorkerId;
       // pthread_create(&tcpServerWorkerId,NULL,server_ipc_worker,ctx);
    }

    //  Connect backend to frontend via a proxy
    zmq_proxy (frontend, backend, NULL);

    zmq_ctx_destroy (&ctx);

}
static void server_ipc_worker (void *ctx)
{
    printf("------%d-------\n",__LINE__);
    void *worker = zmq_socket (ctx, ZMQ_DEALER);
    zmq_connect (worker, "inproc://backend");
    char buf[1024] = {0};
    laserx_frame_t frame_info;
    memset(&frame_info, 0, sizeof(laserx_frame_t));
    int i=0;

    while (true) {
        zmq_pollitem_t items [] = { { worker, 0, ZMQ_POLLIN, 0 } };
        int rc = zmq_poll (items, 1, 10 * ZMQ_POLL_MSEC);
        if (rc == -1)
            break;              //  Interrupted

        if (items [0].revents & ZMQ_POLLIN) {
            zmsg_t *msg = zmsg_recv (worker);

            zframe_t *identity = zmsg_pop (msg);
           // zframe_print (identity, 0);
            printf("------%d-------\n",__LINE__);
            zframe_t *content = zmsg_pop (msg);
            zframe_print (content, 0);
            // memcpy(buf,zframe_data(content),zframe_size (content));
            laserx_deframe(zframe_data(content),zframe_size (content), 0, &frame_info);
            ZMQIpcCmdProcess(worker,identity,frame_info);
            //zframe_destroy (&identity);
            zframe_destroy (&content);
        }
    }
}

static void *tcpServerRun(void *param)
{
    printf("-------------zmj_server_task---------------------\n");
    zmj_server_task();
    pthread_detach(pthread_self());
}
static void *tcpIpcServerRun(void *param)
{
    printf("-------------zmj_ipc_server_task----------------\n");
    zmj_ipc_server_task ();
    pthread_detach(pthread_self());
}

int setip(char *ip)
{
    struct ifreq temp;
    struct sockaddr_in *addr;
    int fd = 0;
    int ret = -1;
      printf("-----------set ip---------------\n");
    strcpy(temp.ifr_name, "eth1");
    if((fd=socket(AF_INET, SOCK_STREAM, 0))<0)
    {
         printf("------------err----------------\n");
      return -1;
    }
    addr = (struct sockaddr_in *)&(temp.ifr_addr);
    addr->sin_family = AF_INET;
    addr->sin_addr.s_addr = inet_addr(ip);
    ret = ioctl(fd, SIOCSIFADDR, &temp);
    close(fd);
    if(ret < 0)
       return -1;
     printf("------------set ip ok----------------\n");
    return 0;
}

int tcpServerInit()
{
   // setip("192.168.8.37");
    setZMQSendCallbackFuntion(zmq_cmd_ack);
    pthread_t		tcpServerId;
    pthread_create(&tcpServerId,NULL,tcpServerRun,NULL);
    pthread_t		tcpIpcServerId;
    pthread_create(&tcpIpcServerId,NULL,tcpIpcServerRun,NULL);
}
