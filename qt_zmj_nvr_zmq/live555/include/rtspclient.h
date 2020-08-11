#ifndef RTSPCLIENT_H
#define RTSPCLIENT_H

#include "hisi_vdec.h"
#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"

#include "camera.h"
#include <pthread.h>
#include <pinger.h>

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

////定义接口
//class RTSP
//{
//public:
//    virtual int RTSPClientPlay(NET_DVR_IPDEVINFO *pNvrInfo) = 0;
//    virtual int RTSPClientStop(NET_DVR_IPDEVINFO *pNvrInfo) = 0;

//    virtual int RTSPClientStopOneCamera(NET_DVR_CAMERAINFO *pCamera) = 0;
//    virtual int RTSPClientPlayOneCamera(NET_DVR_CAMERAINFO *pCamera) = 0;


//    virtual  int RTSPClientRestartOneCamera(NET_DVR_CAMERAINFO *pCamera) = 0;
//};

////定义接口
//class NVR
//{
//public:
//    virtual int set_nvr_all_info(NET_DVR_IPDEVINFO *pNvrInfo) = 0;
//    virtual int get_nvr_all_info(NET_DVR_IPDEVINFO *pNvrInfo) = 0;

//    virtual int nvr_reboot() = 0;
//    virtual int nvr_reset() = 0;
//    virtual int nvr_set_time(char *time,int size) = 0;
//    virtual int nvr_get_time(char *time,int size) = 0;
//    virtual int nvr_set_time(char *time,int size) = 0;

//    virtual int nvr_get_version(NET_DVR_VERSION *version) = 0;
//    virtual int nvr_update(char *filename) = 0;

//    virtual int get_nvr_set_ip(NET_DVR_IPADDR *struIP) = 0;
//};


typedef	int (FrameDataCallBack)(void *frameBuf, long frameLength,unsigned long long u64pts,u_int8_t cameraId, void *args);



typedef struct hiCAMERA
{
    u_int8_t cameraId;
    char rtspURL[128];
}ZMJCAMERA;

extern int RTSPClientStart();
extern int RTSPClientInit(NET_DVR_IPDEVINFO *pNvrInfo);
extern void openURL(UsageEnvironment& env, char const* progName, u_int8_t cameraId,char const* rtspURL);
extern void openURL1(UsageEnvironment& env, char const* progName, u_int8_t cameraId,char const* rtspURL,RTSPClient *pRtspClient);
extern int RTSPClientPlay(NET_DVR_IPDEVINFO *pNvrInfo);
extern int RTSPClientStop(NET_DVR_IPDEVINFO *pNvrInfo);

extern int RTSPClientStopOneCamera(NET_DVR_CAMERAINFO *pCamera);
extern int RTSPClientPlayOneCamera(NET_DVR_CAMERAINFO *pCamera);


extern  int RTSPClientRestartOneCamera(NET_DVR_CAMERAINFO *pCamera);




class CRTSPSession
{
public:
    CRTSPSession();
    virtual ~CRTSPSession();
    int startRTSPClient(char const *progName, NET_DVR_CAMERAINFO cameraInfo, int debugLevel);
    int stopRTSPClient();
    int openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel);

  //  static UINT rtsp_thread_fun(LPVOID param);
    void rtsp_fun();
    static void *rtsp_thread_fun(void* param);
public:
    RTSPClient *m_rtspClient;
    char eventLoopWatchVariable;
    pthread_t		videotid;
    void *value_ptr;
    NET_DVR_CAMERAINFO cameraInfo;
    int m_debugLevel;

    int isRunning;
private:
    Pinger pinger;
};



// Define a class to hold per-stream state that we maintain throughout each stream's lifetime:

class StreamClientState {
public:
  StreamClientState();
  virtual ~StreamClientState();

public:
  MediaSubsessionIterator* iter;
  MediaSession* session;
  MediaSubsession* subsession;
  TaskToken streamTimerTask;
  double duration;
  u_int8_t cameraId;
};

// If you're streaming just a single stream (i.e., just from a single URL, once), then you can define and use just a single
// "StreamClientState" structure, as a global variable in your application.  However, because - in this demo application - we're
// showing how to play multiple streams, concurrently, we can't do that.  Instead, we have to have a separate "StreamClientState"
// structure for each "RTSPClient".  To do this, we subclass "RTSPClient", and add a "StreamClientState" field to the subclass:

class ourRTSPClient: public RTSPClient {
public:
  static ourRTSPClient* createNew(UsageEnvironment& env, u_int8_t cameraId, char const* rtspURL,
                  int verbosityLevel = 0,
                  char const* applicationName = NULL,
                  portNumBits tunnelOverHTTPPortNum = 0);

protected:
  ourRTSPClient(UsageEnvironment& env, u_int8_t cameraId, char const* rtspURL,
        int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum);
    // called only by createNew();
  virtual ~ourRTSPClient();

public:
  StreamClientState scs;
  //u_int8_t cameraId;
};



class DummySink: public MediaSink {
public:
  static DummySink* createNew(UsageEnvironment& env,
                  MediaSubsession& subsession, // identifies the kind of data that's being received
                  FrameDataCallBack *m_Frame,u_int8_t cameraId,char const* streamId = NULL); // identifies the stream itself (optional)

        u_int8_t getCameraId();


private:
  DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId,FrameDataCallBack *m_Frame,u_int8_t cameraId);
    // called only by "createNew()"
  virtual ~DummySink();



  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                struct timeval presentationTime,
                                unsigned durationInMicroseconds);
  void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
             struct timeval presentationTime, unsigned durationInMicroseconds);

             FrameDataCallBack *m_FrameDataCallBack;


private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();

private:
  u_int8_t* fReceiveBuffer;
  MediaSubsession& fSubsession;
  char* fStreamId;
  u_int8_t cameraId;
};


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */



#endif // RTSPCLIENT_H
