#ifndef ITCRTSPCLIENT_HPP
#define ITCRTSPCLIENT_HPP
#include "liveMedia.hh"
#include<string>
#include "BasicUsageEnvironment.hh"
using namespace std;

//void continueAfterOPTIONS(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:

// called when a stream's subsession (e.g., audio or video substream) ends
void subsessionAfterPlaying(void* clientData);

// called when a RTCP "BYE" is received for a subsession

void subsessionByeHandler(void* clientData);

// called at the end of a stream's expected duration (if the stream
// has not already signaled its end using a RTCP "BYE")

void streamTimerHandler(void* clientData);

// Used to iterate through each stream's 'subsessions', setting up each one:

void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):

//void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);
void shutdownStream(void* data);
void continueAfterKeepAliveCommand(RTSPClient *rtspClient,int resultCode,char *resultString);
void sendKeepRtspAliveCommand(void * clientData);

class StreamClientState
{
    public:
        StreamClientState();
        virtual ~StreamClientState();

    public:
        MediaSubsessionIterator* iter;
        MediaSession* session;
        MediaSubsession* subsession;
        TaskToken streamTimerTask;
        double duration;
};

class ItcRtspClient:public RTSPClient{

public:
    void setCallBack(void(*p)(unsigned char*,int,int,char const*,void*),void *param);
    static ItcRtspClient* createNew(
                    UsageEnvironment& env,
                    char const* rtspURL,
                    char const* streamName,
                    int verbosityLevel = 0,
                    char const* applicationName = NULL,
                    portNumBits tunnelOverHTTPPortNum = 0);

    ItcRtspClient(UsageEnvironment& env,
                  char const* rtspURL,
                  char const* streamName,
                  int verbosityLevel = 0,
                  char const* applicationName = NULL,
                  portNumBits tunnelOverHTTPPortNum = 0);
    void scheduleKeepAliveCommand();

    virtual ~ItcRtspClient();

    Authenticator ourAuthenticator;
    StreamClientState scs;
    char const* m_streamName;

public:
    void(*callBack)(unsigned char*,int,int,char const*,void*);
    void *param;
    TaskToken keepAliveTimerTask;
};


class DummySink: public MediaSink{
public:
  static DummySink* createNew(UsageEnvironment& env,
                  MediaSubsession& subsession,int bufferSize, // identifies the kind of data that's being received
                  char const* streamName,
                  char const* streamId = NULL);

  void setRstpClient(ItcRtspClient *client);
  char const* m_StreamName;

private:
  DummySink(UsageEnvironment& env, MediaSubsession& subsession, int bufferSize,char const* streamName,char const* streamId);
  virtual ~DummySink();

  static void afterGettingFrame(void* clientData, unsigned frameSize,
                                unsigned numTruncatedBytes,
                struct timeval presentationTime,
                                unsigned durationInMicroseconds);


   void afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
             struct timeval presentationTime, unsigned durationInMicroseconds);

private:
  // redefined virtual functions:
  virtual Boolean continuePlaying();
   unsigned char* fReceiveBuffer;
   MediaSubsession& fSubsession;
   int bufferSize;
   unsigned char SliceStartCode[4];  // start code "0x00 0x00 0x00 0x01"
   ItcRtspClient *client;
   u_int8_t fSPS[1024];
   unsigned fSPSSize;
   u_int8_t fPPS[1024];
   unsigned fPPSSize;
   u_int8_t vps[64];
   u_int8_t sps[64];
   u_int8_t pps[64];
   u_int8_t sei[64];
   unsigned vpsSize;
   unsigned spsSize;
   unsigned ppsSize;
   unsigned seiSize;

   timeval recordTime;
   char* fStreamId;
};
#endif // ITCRTSPCLIENT_HPP

