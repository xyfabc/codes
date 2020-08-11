#ifndef WORKTHREAD_H
#define WORKTHREAD_H

#include "BasicUsageEnvironment.hh"
#include "DynamicRTSPServer.hh"
#include "version.hh"
#include "H264FramedLiveSource.hh"
#include <pthread.h>

using namespace  std;

class WorkThread
{

public:
    static void putData(unsigned char *buf,int dataLength,int type,char const* streamName,void *param);
    static void *workthread(void *args){
        //pthread_deteach(pthread_self());
        pthread_detach(pthread_self());
        WorkThread *work = static_cast<WorkThread *>(args);
        work->run();
        return NULL;
    }
    static void *workthreadRTSPServer(void *args);

    WorkThread(int port);
    ~WorkThread();
    RTSPServer* rtsp_svr;
    TaskScheduler* scheduler;
    UsageEnvironment* env;
    vector<table *> tableDATA;
    void run();
    void addTable(table *tableTmp);
    void RTSPServerCreate();
private:
    void putDataFunc(unsigned char *buf,int dataLength,int type,char const* streamName);
    char flag;
    int m_port;
    int fifo_fd;
    FILE *fp;
    RTSPServer *rtspServer;
};


#endif // WORKTHREAD_H


