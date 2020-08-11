#include "Workthread.h"
//#include "H264LiveVideoServerMediaSubssion.hh"
#include "H265LiveVideoServerMediaSubssion.hh"

#include <BasicUsageEnvironment.hh>
#include "DynamicRTSPServer.hh"
#include "ServerMediaSession.hh"
#include "version.hh"
#include <sys/time.h>
#include <pthread.h>
#include "itcRtspClient.hpp"
#include<string.h>
#include<string>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>


using namespace std;
char fileName[]="/home/eric/Works/src_libs/live-x86/mediaServer/rtspServer.265";

//#define  USING_FIFO   1


#define BUFSIZE 1024*200
using namespace std;


void WorkThread::putData(unsigned char *buf,int dataLength,int type,char const* streamName,void *param){
    if(param==NULL) return;
    WorkThread *p=(WorkThread*)param;
    p->putDataFunc(buf,dataLength,type,streamName);

    //获取哪一路摄像头的信息  加以判断
}

void *WorkThread::workthreadRTSPServer(void *args)
{
    //pthread_deteach(pthread_self());
    pthread_detach(pthread_self());
    WorkThread *work = static_cast<WorkThread *>(args);
    work->RTSPServerCreate();
    return NULL;

}
#if 1
void WorkThread::putDataFunc(unsigned char *buf,int dataLength,int type,char const* streamName){


    unsigned int i=0;

    for(i=0;i<tableDATA.size();i++){
        table *tmptable= tableDATA[i];

        //if(strcmp(tmptable->name,streamName) == 0){
        if(tmptable->clientNumEmpty){
            unsigned char *tmpBuf=new unsigned char[dataLength+2*sizeof(int)];
            memcpy(tmpBuf+2*sizeof(int),buf,dataLength);
            memcpy(tmpBuf,&dataLength,sizeof(int));
            memcpy(tmpBuf+sizeof(int),&dataLength,sizeof(int));
            tmptable->dataQueue.push_back(tmpBuf);
            //printf("--func:%s--line:%d--putDataFunc11   %d\n",__func__,__LINE__,dataLength);
        }
        // break;
        //}
    }

}
#else

void WorkThread::putDataFunc(unsigned char *buf,int dataLength,int type,char const* streamName){

    // printf("--func:%s--line:%d--putDataFunc\n",__func__,__LINE__);
    if(tableDATA.empty()){

        table *table1=new table;
        Boolean const reuseSource = False;
        pthread_mutex_init(&table1->mutex,NULL);
        table1->dataQueue.clear();
        table1->clientNumEmpty = false;

        unsigned char *tmpBuf=new unsigned char[dataLength+2*sizeof(int)];
        memcpy(tmpBuf+2*sizeof(int),buf,dataLength);
        memcpy(tmpBuf,&dataLength,sizeof(int));
        memcpy(tmpBuf+sizeof(int),&type,sizeof(int));
        table1->dataQueue.push_back(tmpBuf);


        table1->name = streamName;
        tableDATA.push_back(table1);

        Boolean reuseFirstSource = true;
        char const* descriptionString= "Session streamed by \"testOnDemandRTSPServer\"";
        ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName,descriptionString);
#ifdef USING_FIFO
        printf("--func:%s--line:%d--putDataFunc\n",__func__,__LINE__);
        // sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(*env, fileName, reuseSource));
#else
        //sms->addSubsession(H265LiveVideoServerMediaSubssion::createNew(*env, reuseFirstSource,table1));//修改为自己实现的H264LiveVideoServerMediaSubssion
#endif
        OutPacketBuffer::maxSize = 800000;
        // rtsp_svr->addServerMediaSession(sms);
        printf("----------func:%s--line:%d--putDataFunc\n",__func__,__LINE__);
    }else{

        unsigned int i=0;

        for(i=0;i<tableDATA.size();i++){
            table *tmptable= tableDATA[i];

            if(strcmp(tmptable->name,streamName) == 0){
                if(tmptable->clientNumEmpty){
                    unsigned char *tmpBuf=new unsigned char[dataLength+2*sizeof(int)];
                    memcpy(tmpBuf+2*sizeof(int),buf,dataLength);
                    memcpy(tmpBuf,&dataLength,sizeof(int));
                    memcpy(tmpBuf+sizeof(int),&type,sizeof(int));
                    tmptable->dataQueue.push_back(tmpBuf);
                    // printf("--func:%s--line:%d--putDataFunc11   %d\n",__func__,__LINE__,dataLength);
                    // write(fifo_fd, tmpBuf, dataLength);
                    //fwrite(tmpBuf, 1, dataLength, fp);
                    //printf("--func:%s--line:%d--putDataFunc11   %d\n",__func__,__LINE__,dataLength);
                }
                break;
            }
        }
        if(i == tableDATA.size()){

            table *table1 = new table;
            Boolean const reuseSource = False;

            pthread_mutex_init(&table1->mutex,NULL);

            table1->dataQueue.clear();
            table1->clientNumEmpty = false;
            unsigned char *tmpBuf=new unsigned char[dataLength+2*sizeof(int)];
            memcpy(tmpBuf+2*sizeof(int),buf,dataLength);
            memcpy(tmpBuf,&dataLength,sizeof(int));
            memcpy(tmpBuf+sizeof(int),&type,sizeof(int));
            table1->dataQueue.push_back(tmpBuf);
            table1->name = streamName;
            tableDATA.push_back(table1);

            Boolean reuseFirstSource = true;
            char const* descriptionString= "Session streamed by \"testOnDemandRTSPServer\"";


            ServerMediaSession* sms = ServerMediaSession::createNew(*env, streamName, streamName,descriptionString);
#ifdef USING_FIFO
            printf("--func:%s--line:%d--putDataFunc\n",__func__,__LINE__);
            // sms->addSubsession(H265VideoFileServerMediaSubsession::createNew(*env, fileName, reuseSource));
#else
            printf("--func:%s--line:%d--putDataFunc\n",__func__,__LINE__);
            sms->addSubsession(H265LiveVideoServerMediaSubssion::createNew(*env, reuseFirstSource,table1));//修改为自己实现的H264LiveVideoServerMediaSubssion
#endif

            OutPacketBuffer::maxSize = 800000;
            //rtsp_svr->addServerMediaSession(sms);
            // printf("--func:%s--line:%d--putDataFunc\n",__func__,__LINE__);

        }
        // printf("--func:%s--line:%d-%d-%d-putDataFunc\n",__func__,__LINE__,tableDATA.size(),i);
    }
}
#endif


WorkThread::WorkThread(int port)
{
    m_port = port;
    rtsp_svr = NULL;
    tableDATA.clear();
    scheduler = BasicTaskScheduler::createNew();
    env = BasicUsageEnvironment::createNew(*scheduler);

}
WorkThread::~WorkThread()
{
    flag=1;
    //requestInterruption();
    terminate();
    wait();
}


void WorkThread::run()
{

    UserAuthenticationDatabase* authDB = NULL;

    if(rtsp_svr== NULL ){
        rtsp_svr = RTSPServer::createNew(*env, m_port, authDB);

    }
#define RTSP_CLIENT_VERBOSITY_LEVEL 1
#define PROG_NAME "ITC_CLIENT_MANGER"


    //摄像头的IP 用户名和密码 修改在这里
    ItcRtspClient * clientPtr=ItcRtspClient::createNew(*env,"rtsp://192.168.8.131:554/","h264ESVideoTest9000",RTSP_CLIENT_VERBOSITY_LEVEL,PROG_NAME);
    clientPtr->ourAuthenticator.setUsernameAndPassword("admin","zmj123456");

    //clientPtr->sendDescribeCommand(continueAfterDESCRIBE,clientPtr->ourAuthenticator);
    clientPtr->setCallBack(putData,this);
    clientPtr->sendDescribeCommand(continueAfterDESCRIBE,&clientPtr->ourAuthenticator);

    ItcRtspClient * clientPtr1=ItcRtspClient::createNew(*env,"rtsp://192.168.1.97:554/","h264ESVideoTest9001",RTSP_CLIENT_VERBOSITY_LEVEL,PROG_NAME);
    //clientPtr->sendDescribeCommand(continueAfterDESCRIBE,clientPtr->ourAuthenticator);
    clientPtr1->ourAuthenticator.setUsernameAndPassword("admin","itc123456");
    clientPtr1->setCallBack(putData,this);
    clientPtr1->sendDescribeCommand(continueAfterDESCRIBE,&clientPtr1->ourAuthenticator);

    flag=0;
    env->taskScheduler().doEventLoop(&flag);
    //free(databuf);//释放掉内存
}

void WorkThread::addTable(table *tableTmp)
{
    tableDATA.push_back(tableTmp);
}

void WorkThread::RTSPServerCreate()
{
    // Begin by setting up our usage environment:
    TaskScheduler* scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment* env = BasicUsageEnvironment::createNew(*scheduler);

    UserAuthenticationDatabase* authDB = NULL;
#ifdef ACCESS_CONTROL
    // To implement client access control to the RTSP server, do the following:
    authDB = new UserAuthenticationDatabase;
    authDB->addUserRecord("username1", "password1"); // replace these with real strings
    // Repeat the above with each <username>, <password> that you wish to allow
    // access to the server.
#endif

    // Create the RTSP server.  Try first with the default port number (554),
    // and then with the alternative port number (8554):

    portNumBits rtspServerPortNum = 8554;
    rtspServer = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB);
    if (rtspServer == NULL) {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
         exit(1);
        rtspServerPortNum = 8554;
        rtspServer = DynamicRTSPServer::createNew(*env, rtspServerPortNum, authDB);
    }
    if (rtspServer == NULL) {
        *env << "Failed to create RTSP server: " << env->getResultMsg() << "\n";
        exit(1);
    }

    *env << "LIVE555 Media Server\n";
    *env << "\tversion " << MEDIA_SERVER_VERSION_STRING
         << " (LIVE555 Streaming Media library version "
         << LIVEMEDIA_LIBRARY_VERSION_STRING << ").\n";

    char* urlPrefix = rtspServer->rtspURLPrefix();
    *env << "Play streams from this server using the URL\n\t"
         << urlPrefix << "<filename>\nwhere <filename> is a file present in the current directory.\n";
    *env << "Each file's type is inferred from its name suffix:\n";
    *env << "\t\".264\" => a H.264 Video Elementary Stream file\n";
    *env << "\t\".265\" => a H.265 Video Elementary Stream file\n";
    *env << "\t\".aac\" => an AAC Audio (ADTS format) file\n";
    *env << "\t\".ac3\" => an AC-3 Audio file\n";
    *env << "\t\".amr\" => an AMR Audio file\n";
    *env << "\t\".dv\" => a DV Video file\n";
    *env << "\t\".m4e\" => a MPEG-4 Video Elementary Stream file\n";
    *env << "\t\".mkv\" => a Matroska audio+video+(optional)subtitles file\n";
    *env << "\t\".mp3\" => a MPEG-1 or 2 Audio file\n";
    *env << "\t\".mpg\" => a MPEG-1 or 2 Program Stream (audio+video) file\n";
    *env << "\t\".ogg\" or \".ogv\" or \".opus\" => an Ogg audio and/or video file\n";
    *env << "\t\".ts\" => a MPEG Transport Stream file\n";
    *env << "\t\t(a \".tsx\" index file - if present - provides server 'trick play' support)\n";
    *env << "\t\".vob\" => a VOB (MPEG-2 video with AC-3 audio) file\n";
    *env << "\t\".wav\" => a WAV Audio file\n";
    *env << "\t\".webm\" => a WebM audio(Vorbis)+video(VP8) file\n";
    *env << "See http://www.live555.com/mediaServer/ for additional documentation.\n";

    // Also, attempt to create a HTTP server for RTSP-over-HTTP tunneling.
    // Try first with the default HTTP port (80), and then with the alternative HTTP
    // port numbers (8000 and 8080).

    if (rtspServer->setUpTunnelingOverHTTP(80) || rtspServer->setUpTunnelingOverHTTP(8000) || rtspServer->setUpTunnelingOverHTTP(8080)) {
        *env << "(We use port " << rtspServer->httpServerPortNum() << " for optional RTSP-over-HTTP tunneling, or for HTTP live streaming (for indexed Transport Stream files only).)\n";
    } else {
        *env << "(RTSP-over-HTTP tunneling is not available.)\n";
    }
    DynamicRTSPServer *server = (DynamicRTSPServer*)rtspServer;
    server->SetWorkThread(this);

    env->taskScheduler().doEventLoop(); // does not return
}



