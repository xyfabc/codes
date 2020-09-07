/**********
This library is free software; you can redistribute it and/or modify it under
the terms of the GNU Lesser General Public License as published by the
Free Software Foundation; either version 3 of the License, or (at your
option) any later version. (See <http://www.gnu.org/copyleft/lesser.html>.)

This library is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for
more details.

You should have received a copy of the GNU Lesser General Public License
along with this library; if not, write to the Free Software Foundation, Inc.,
51 Franklin Street, Fifth Floor, Boston, MA 02110-1301  USA
**********/
// Copyright (c) 1996-2019, Live Networks, Inc.  All rights reserved
// A demo application, showing how to create and run a RTSP client (that can potentially receive multiple streams concurrently).
//
// NOTE: This code - although it builds a running application - is intended only to illustrate how to develop your own RTSP
// client application.  For a full-featured RTSP client application - with much more functionality, and many options - see
// "openRTSP": http://www.live555.com/openRTSP/

#include "liveMedia.hh"
#include "BasicUsageEnvironment.hh"
#include "hisi_vdec.h"
#include <pthread.h>
#include "rtspclient.h"

#include "pinger.h"
#include <QDebug>


// Forward function definitions:

// RTSP 'response handlers':
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString);
void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString);

// Other event handler functions:
void subsessionAfterPlaying(void* clientData); // called when a stream's subsession (e.g., audio or video substream) ends
void subsessionByeHandler(void* clientData, char const* reason);
  // called when a RTCP "BYE" is received for a subsession
void streamTimerHandler(void* clientData);
  // called at the end of a stream's expected duration (if the stream has not already signaled its end using a RTCP "BYE")

// The main streaming routine (for each "rtsp://" URL):
void openURL(UsageEnvironment& env, char const* progName, u_int8_t cameraId, char const* rtspURL);

// Used to iterate through each stream's 'subsessions', setting up each one:
void setupNextSubsession(RTSPClient* rtspClient);

// Used to shut down and close a stream (including its "RTSPClient" object):
void shutdownStream(RTSPClient* rtspClient, int exitCode = 1);

// A function that outputs a string that identifies each stream (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const RTSPClient& rtspClient) {
  return env << "[URL:\"" << rtspClient.url() << "\"]: ";
}

// A function that outputs a string that identifies each subsession (for debugging output).  Modify this if you wish:
UsageEnvironment& operator<<(UsageEnvironment& env, const MediaSubsession& subsession) {
  return env << subsession.mediumName() << "/" << subsession.codecName();
}

void usage(UsageEnvironment& env, char const* progName) {
  env << "Usage: " << progName << " <rtsp-url-1> ... <rtsp-url-N>\n";
  env << "\t(where each <rtsp-url-i> is a \"rtsp://\" URL)\n";
}


//UsageEnvironment* my_env;

#define RPST_MAX_LEN                16      //最大接收16路

char eventLoopWatchVariable = 0;

CRTSPSession::CRTSPSession()
{
    m_rtspClient = NULL;
   // m_running = false;
    eventLoopWatchVariable = 0;
    isRunning = 0;
   // m_RTPDataLength = 0;
}

CRTSPSession::~CRTSPSession()
{

}

int CRTSPSession::startRTSPClient(char const *progName, NET_DVR_CAMERAINFO cameraInfo, int debugLevel)
{
    int ret = -1;
    memcpy(&(this->cameraInfo),&cameraInfo,sizeof(NET_DVR_CAMERAINFO));
    m_debugLevel = debugLevel;
    eventLoopWatchVariable = 0;
    isRunning = 1;
    ret = pthread_create(&videotid,NULL,rtsp_thread_fun,(void *)this);
    if (0 == ret){
        //pthread_detach(videotid);
        return 0;
    }else{
        return -1;
    }

    return 0;
}

int CRTSPSession::stopRTSPClient()
{
  // eventLoopWatchVariable = 1;

    return 0;
}

void* CRTSPSession::rtsp_thread_fun(void* param)
{
    CRTSPSession *pThis = (CRTSPSession *)param;
    pthread_detach(pthread_self());
    pThis->rtsp_fun();
    return 0;
}
#include <QString>
#include <QProcess>
#include <QDebug>
bool ping_ip(QString ip)
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
void CRTSPSession::rtsp_fun()
{
    if(!pinger.ping_ip(cameraInfo.struIP.sIpV4,1)){
         qDebug()<< "--------" <<"ping err";
         return ;
    }else{
        qDebug()<<"--------"<<"ping ok";
    }
    TaskScheduler *scheduler = BasicTaskScheduler::createNew();
    UsageEnvironment *env = BasicUsageEnvironment::createNew(*scheduler);
    char rtspURL[128]= {0};
    sprintf(rtspURL,"rtsp://%s:%s@%s",cameraInfo.sUserName,cameraInfo.sPassword,cameraInfo.struIP.sIpV4);
    printf("----%s\n",rtspURL);
    if (openURL(*env, "qt_zmj_nvr", rtspURL, m_debugLevel) == 0)
    {
        env->taskScheduler().doEventLoop(&eventLoopWatchVariable);
        eventLoopWatchVariable = 0;
        if (m_rtspClient)
        {
            shutdownStream(m_rtspClient, 0);
        }
        m_rtspClient = NULL;
    }
    env->reclaim();
    env = NULL;
    delete scheduler;
    scheduler = NULL;
}

int CRTSPSession::openURL(UsageEnvironment& env, char const* progName, char const* rtspURL, int debugLevel)
{
   // CUrl = rtspURL;  debugLevel
    m_rtspClient = ourRTSPClient::createNew(env, cameraInfo.cmaraChnID,rtspURL, debugLevel, progName);
    if (m_rtspClient == NULL)
    {
        env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
        return -1;
    }
    //((ourRTSPClient *)m_rtspClient)->m_nID = 1;
    m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE);

    return 0;
}

CRTSPSession cRtspSession[RPST_MAX_LEN];

int RTSPClientPlayOneCamera(NET_DVR_CAMERAINFO *pCamera)
{
    int ret = -1;
    if(pCamera->isRun){
       int num  = pCamera->cmaraChnID;
       chnStartRecvStream(pCamera->cmaraChnID);
       ret = cRtspSession[num].startRTSPClient("qt_zmj_nvr",*pCamera,0);
       //printf("---------run--------11111-----%s---chn:%d-------\n",pCamera->sCameraName,pCamera->cmaraChnID);
    }else{
        enableUserPic(pCamera->cmaraChnID);
       // printf("-----not run--------11111-----%s----------\n",pCamera->sCameraName);
    }

  return 0;
}
int RTSPClientStopOneCamera(NET_DVR_CAMERAINFO *pCamera)
{
    if(pCamera->isRun){
       int num  = pCamera->cmaraChnID;
       cRtspSession[num].stopRTSPClient();
    }else{
       // printf("-----------delete-----ok--------11111---------------\n");
    }
    usleep(100*1000);
     return 0;
}

int RTSPClientPlay(NET_DVR_IPDEVINFO *pNvrInfo)
{
    int num = pNvrInfo->cmaraChnNum > pNvrInfo->cameraNum?pNvrInfo->cmaraChnNum:pNvrInfo->cameraNum;
    printf("----------num = %d------\n",num);
    for(int i=0;i<num;i++){
        NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+i;
         RTSPClientPlayOneCamera(pCamera);
    }
  return 0;
}
int RTSPClientStop(NET_DVR_IPDEVINFO *pNvrInfo)
{
    int num = 0;
    for(int i=0;i<pNvrInfo->cameraNum;i++){
        NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+i;
        RTSPClientStopOneCamera(pCamera);
    }
  return 0;
   // cRtspSession.stopRTSPClient();
    printf("eventLoopWatchVariable = %d \n",eventLoopWatchVariable);
   // shutdownStream(rtspClient);
}


//int RTSPClientPlay(NET_DVR_IPDEVINFO *pNvrInfo)
//{
//    int num = pNvrInfo->cmaraChnNum > pNvrInfo->cmaraChnNum?pNvrInfo->cmaraChnNum:pNvrInfo->cmaraChnNum;
//    printf("----------num = %d------\n",num);
//    for(int i=0;i<num;i++){
//        NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+i;
//        printf("------%s----------\n",pCamera->sCameraName);
//        if(pCamera->isRun){
//            int num = pCamera->cmaraChnID;
//           chnStartRecvStream(pCamera->cmaraChnID);
//          // enableUserPic(pCamera->cmaraChnID);
//           cRtspSession[num].startRTSPClient("qt_zmj_nvr",*pCamera,0);
//          // num++;
//           printf("---------run--------11111-----%s----------\n",pCamera->sCameraName);
//        }else{
//            enableUserPic(pCamera->cmaraChnID);
//            printf("-----not run--------11111-----%s----------\n",pCamera->sCameraName);
//        }
//    }
//  return 0;
//}
//int RTSPClientStop(NET_DVR_IPDEVINFO *pNvrInfo)
//{
//    int num = 0;
//    for(int i=0;i<pNvrInfo->cameraNum;i++){
//        NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+i;
//        num = pCamera->cmaraChnID;
//        if(pCamera->isRun){
//           cRtspSession[num].stopRTSPClient();
//          // num++;
//        }else{
//            printf("-----------delete-----ok--------11111---------------\n");
//        }
//    }
//  return 0;
//   // cRtspSession.stopRTSPClient();
//    printf("eventLoopWatchVariable = %d \n",eventLoopWatchVariable);
//   // shutdownStream(rtspClient);
//}
//run err then restart
int RTSPClientRestartOneCamera(NET_DVR_CAMERAINFO *pCamera)
{
    //stop

   //cRtspSession[pCamera->cmaraChnID].stopRTSPClient();
   //enableUserPic(pCamera->cmaraChnID);

   //restart
   chnStartRecvStream(pCamera->cmaraChnID);
   cRtspSession[pCamera->cmaraChnID].startRTSPClient("qt_zmj_nvr",*pCamera,0);
   return 0;
}

// Define a data sink (a subclass of "MediaSink") to receive the data for each subsession (i.e., each audio or video 'substream').
// In practice, this might be a class (or a chain of classes) that decodes and then renders the incoming audio or video.
// Or it might be a "FileSink", for outputting the received data into a file (as is done by the "openRTSP" application).
// In this example code, however, we define a simple 'dummy' sink that receives incoming data, but does nothing with it.

#define RTSP_CLIENT_VERBOSITY_LEVEL 1 // by default, print verbose output from each "RTSPClient"

static unsigned rtspClientCount = 0; // Counts how many streams (i.e., "RTSPClient"s) are currently in use.

void openURL(UsageEnvironment& env, char const* progName, u_int8_t cameraId,char const* rtspURL) {
  // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
  // to receive (even if more than stream uses the same "rtsp://" URL).
  RTSPClient* rtspClient = ourRTSPClient::createNew(env, cameraId,rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
  if (rtspClient == NULL) {
    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
    return;
  }

  ++rtspClientCount;

  // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
  // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
  // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
  rtspClient->sendDescribeCommand(continueAfterDESCRIBE); 
}

//void openURL1(UsageEnvironment& env, char const* progName, u_int8_t cameraId,char const* rtspURL,RTSPClient *pRtspClient) {
//  // Begin by creating a "RTSPClient" object.  Note that there is a separate "RTSPClient" object for each stream that we wish
//  // to receive (even if more than stream uses the same "rtsp://" URL).
//  m_rtspClient = ourRTSPClient::createNew(env, cameraId,rtspURL, RTSP_CLIENT_VERBOSITY_LEVEL, progName);
//  if (m_rtspClient == NULL) {
//    env << "Failed to create a RTSP client for URL \"" << rtspURL << "\": " << env.getResultMsg() << "\n";
//    pRtspClient = NULL;
//    return;
//  }
//    //*pRtspClient = rtspClient;
//  ++rtspClientCount;

//  // Next, send a RTSP "DESCRIBE" command, to get a SDP description for the stream.
//  // Note that this command - like all RTSP commands - is sent asynchronously; we do not block, waiting for a response.
//  // Instead, the following function call returns immediately, and we handle the RTSP response later, from within the event loop:
//  m_rtspClient->sendDescribeCommand(continueAfterDESCRIBE);
//}


// Implementation of the RTSP 'response handlers':

void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to get a SDP description: " << resultString << "\n";
      delete[] resultString;
      break;
    }

    char* const sdpDescription = resultString;
    env << *rtspClient << "Got a SDP description:\n" << sdpDescription << "\n";

    // Create a media session object from this SDP description:
    scs.session = MediaSession::createNew(env, sdpDescription);
    delete[] sdpDescription; // because we don't need it anymore
    if (scs.session == NULL) {
      env << *rtspClient << "Failed to create a MediaSession object from the SDP description: " << env.getResultMsg() << "\n";
      break;
    } else if (!scs.session->hasSubsessions()) {
      env << *rtspClient << "This session has no media subsessions (i.e., no \"m=\" lines)\n";
      break;
    }

    // Then, create and set up our data source objects for the session.  We do this by iterating over the session's 'subsessions',
    // calling "MediaSubsession::initiate()", and then sending a RTSP "SETUP" command, on each one.
    // (Each 'subsession' will have its own data source.)
    scs.iter = new MediaSubsessionIterator(*scs.session);
    setupNextSubsession(rtspClient);
    return;
  } while (0);

  // An unrecoverable error occurred with this stream.
  shutdownStream(rtspClient);
}

// By default, we request that the server stream its data using RTP/UDP.
// If, instead, you want to request that the server stream via RTP-over-TCP, change the following to True:
#define REQUEST_STREAMING_OVER_TCP False

void setupNextSubsession(RTSPClient* rtspClient) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
  
  scs.subsession = scs.iter->next();
  if (scs.subsession != NULL) {
    if (!scs.subsession->initiate()) {
      env << *rtspClient << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
      setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
    } else {
      env << *rtspClient << "Initiated the \"" << *scs.subsession << "\" subsession (";
      if (scs.subsession->rtcpIsMuxed()) {
	env << "client port " << scs.subsession->clientPortNum();
      } else {
	env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
      }
      env << ")\n";

      // Continue setting up this subsession, by sending a RTSP "SETUP" command:
      rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, False, REQUEST_STREAMING_OVER_TCP);
    }
    return;
  }

  // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
  if (scs.session->absStartTime() != NULL) {
    // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime());
  } else {
    scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
    rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY);
  }
}

void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
  do {
  		printf("---------------continueAfterSETUP------------------------\n");
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias
    u_int8_t cameraId = 0;

    if (resultCode != 0) {
      env << *rtspClient << "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
      break;
    }
	//zmj eric.xi add for video only
	if(strcmp(scs.subsession->mediumName(), "video") != 0){
		printf("----------not video-----Discard it--------\n");
		break;
	}
		
    env << *rtspClient << "Set up the \"" << *scs.subsession << "\" subsession (";
    if (scs.subsession->rtcpIsMuxed()) {
      env << "client port " << scs.subsession->clientPortNum();
    } else {
      env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
    }
    env << ")\n";

    // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
    // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
    // after we've sent a RTSP "PLAY" command.)
	cameraId = scs.cameraId;
//	subsession.mediumName() << "/" << subsession.codecName()
	printf("------resultCode=%d----------cameraId------------------%d-------------%s----%s--------\n",resultCode,cameraId,scs.subsession->mediumName(),scs.subsession->codecName());
    scs.subsession->sink = DummySink::createNew(env, *scs.subsession,HiSiFrameDataCallBack, cameraId,rtspClient->url());
      // perhaps use your own custom "MediaSink" subclass instead
    if (scs.subsession->sink == NULL) {
      env << *rtspClient << "Failed to create a data sink for the \"" << *scs.subsession
	  << "\" subsession: " << env.getResultMsg() << "\n";
      break;
    }
	//cameraId++; //add by eric.xi@zmj 20191113
	//zmj eric.xi add at 20191111
	//scs.subsession->sink->m_FrameDataCallBack =  HiSiFrameDataCallBack;
    env << *rtspClient << "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
    scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession 
    scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
				       subsessionAfterPlaying, scs.subsession);
    // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
    if (scs.subsession->rtcpInstance() != NULL) {
      scs.subsession->rtcpInstance()->setByeWithReasonHandler(subsessionByeHandler, scs.subsession);
    }
  } while (0);
  delete[] resultString;

  // Set up the next subsession, if any:
  setupNextSubsession(rtspClient);
}

void continueAfterPLAY(RTSPClient* rtspClient, int resultCode, char* resultString) {
  Boolean success = False;

  do {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

    if (resultCode != 0) {
      env << *rtspClient << "Failed to start playing session: " << resultString << "\n";
      break;
    }

    // Set a timer to be handled at the end of the stream's expected duration (if the stream does not already signal its end
    // using a RTCP "BYE").  This is optional.  If, instead, you want to keep the stream active - e.g., so you can later
    // 'seek' back within it and do another RTSP "PLAY" - then you can omit this code.
    // (Alternatively, if you don't want to receive the entire stream, you could set this timer for some shorter value.)
    if (scs.duration > 0) {
      unsigned const delaySlop = 2; // number of seconds extra to delay, after the stream's expected duration.  (This is optional.)
      scs.duration += delaySlop;
      unsigned uSecsToDelay = (unsigned)(scs.duration*1000000);
      scs.streamTimerTask = env.taskScheduler().scheduleDelayedTask(uSecsToDelay, (TaskFunc*)streamTimerHandler, rtspClient);
    }

    env << *rtspClient << "Started playing session";
    if (scs.duration > 0) {
      env << " (for up to " << scs.duration << " seconds)";
    }
    env << "...\n";

    success = True;
  } while (0);
  delete[] resultString;

  if (!success) {
    // An unrecoverable error occurred with this stream.
    shutdownStream(rtspClient);
  }
}


// Implementation of the other event handlers:

void subsessionAfterPlaying(void* clientData) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)(subsession->miscPtr);

  // Begin by closing this subsession's stream:
  Medium::close(subsession->sink);
  subsession->sink = NULL;

  // Next, check whether *all* subsessions' streams have now been closed:
  MediaSession& session = subsession->parentSession();
  MediaSubsessionIterator iter(session);
  while ((subsession = iter.next()) != NULL) {
    if (subsession->sink != NULL) return; // this subsession is still active
  }

  // All subsessions' streams have now been closed, so shutdown the client:
  shutdownStream(rtspClient);
}

void subsessionByeHandler(void* clientData, char const* reason) {
  MediaSubsession* subsession = (MediaSubsession*)clientData;
  RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
  UsageEnvironment& env = rtspClient->envir(); // alias

  env << *rtspClient << "Received RTCP \"BYE\"";
  if (reason != NULL) {
    env << " (reason:\"" << reason << "\")";
    delete[] (char*)reason;
  }
  env << " on \"" << *subsession << "\" subsession\n";

  // Now act as if the subsession had closed:
  subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
  ourRTSPClient* rtspClient = (ourRTSPClient*)clientData;
  StreamClientState& scs = rtspClient->scs; // alias

  scs.streamTimerTask = NULL;

  // Shut down the stream:
  shutdownStream(rtspClient);
}

void shutdownStream(RTSPClient* rtspClient, int exitCode) {
  UsageEnvironment& env = rtspClient->envir(); // alias
  StreamClientState& scs = ((ourRTSPClient*)rtspClient)->scs; // alias

  // First, check whether any subsessions have still to be closed:
  if (scs.session != NULL) { 
    Boolean someSubsessionsWereActive = False;
    MediaSubsessionIterator iter(*scs.session);
    MediaSubsession* subsession;

    while ((subsession = iter.next()) != NULL) {
      if (subsession->sink != NULL) {
	Medium::close(subsession->sink);
	subsession->sink = NULL;

	if (subsession->rtcpInstance() != NULL) {
	  subsession->rtcpInstance()->setByeHandler(NULL, NULL); // in case the server sends a RTCP "BYE" while handling "TEARDOWN"
	}

	someSubsessionsWereActive = True;
      }
    }

    if (someSubsessionsWereActive) {
      // Send a RTSP "TEARDOWN" command, to tell the server to shutdown the stream.
      // Don't bother handling the response to the "TEARDOWN".
      rtspClient->sendTeardownCommand(*scs.session, NULL);
    }
  }

  env << *rtspClient << "Closing the stream.\n";
  Medium::close(rtspClient);
    // Note that this will also cause this stream's "StreamClientState" structure to get reclaimed.

  if (--rtspClientCount == 0) {
    // The final stream has ended, so exit the application now.
    // (Of course, if you're embedding this code into your own application, you might want to comment this out,
    // and replace it with "eventLoopWatchVariable = 1;", so that we leave the LIVE555 event loop, and continue running "main()".)
   // exit(exitCode);
  }
}


// Implementation of "ourRTSPClient":

ourRTSPClient* ourRTSPClient::createNew(UsageEnvironment& env, u_int8_t cameraId, char const* rtspURL,
					int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
  return new ourRTSPClient(env,cameraId, rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}

ourRTSPClient::ourRTSPClient(UsageEnvironment& env,u_int8_t cameraId, char const* rtspURL,
			     int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
  : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {
  //this->cameraId = cameraId;
  this->scs.cameraId = cameraId;
}

ourRTSPClient::~ourRTSPClient() {
}


// Implementation of "StreamClientState":

StreamClientState::StreamClientState()
  : iter(NULL), session(NULL), subsession(NULL), streamTimerTask(NULL), duration(0.0) {
}

StreamClientState::~StreamClientState() {
  delete iter;
  if (session != NULL) {
    // We also need to delete "session", and unschedule "streamTimerTask" (if set)
    UsageEnvironment& env = session->envir(); // alias

    env.taskScheduler().unscheduleDelayedTask(streamTimerTask);
    Medium::close(session);
  }
}


// Implementation of "DummySink":

// Even though we're not going to be doing anything with the incoming data, we still need to receive it.
// Define the size of the buffer that we'll use:
#define DUMMY_SINK_RECEIVE_BUFFER_SIZE 500000

DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession ,FrameDataCallBack *m_Frame,u_int8_t cameraId,char const* streamId ) {

  printf("--------------------------------%d-----------------------------------------\n",cameraId);
  return new DummySink(env, subsession, streamId, m_Frame,cameraId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, char const* streamId ,FrameDataCallBack *m_Frame,u_int8_t cameraId)
  : MediaSink(env),
    fSubsession(subsession) {
  fStreamId = strDup(streamId);
  fReceiveBuffer = new u_int8_t[DUMMY_SINK_RECEIVE_BUFFER_SIZE];
  this->m_FrameDataCallBack =   m_Frame;
  this->cameraId = cameraId;
}

u_int8_t DummySink::getCameraId()
{
	return this->cameraId;
}


DummySink::~DummySink() {
  delete[] fReceiveBuffer;
  delete[] fStreamId;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned durationInMicroseconds) {
  DummySink* sink = (DummySink*)clientData;
  sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

// If you don't want to see debugging output for each received frame, then comment out the following line:
#define DEBUG_PRINT_EACH_RECEIVED_FRAME 1
#if 0
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
  // We've just received a frame of data.  (Optionally) print out information about it:
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
  
  // Then continue, to request the next frame of data:
  continuePlaying();
}
#else

#if 0
unsigned char *pH264 = NULL; // 数据buffer
int firstFrame = 0;
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
struct timeval presentationTime, unsigned /*durationInMicroseconds*/) 
{
	//printf("%s----------%d\n",__FILE__, __LINE__); 

	if(!strcmp(fSubsession.mediumName(), "video")){
		
	
 		if(firstFrame){
 			unsigned int num;
 			SPropRecord *sps = parseSPropParameterSets(fSubsession.fmtp_spropparametersets(), num);
 			// For H.264 video stream, we use a special sink that insert start_codes:
 			struct timeval tv= {0,0};
 			unsigned char start_code[4] = {0x00, 0x00, 0x00, 0x01};
			
			FILE *fp = fopen("test.h265", "a+b");
 			if(fp){
 				fwrite(start_code, 4, 1, fp);
 				fwrite(sps[0].sPropBytes, sps[0].sPropLength, 1, fp);	
 				fclose(fp);		
 				fp = NULL;		
 			}
		
 			delete [] sps;
 			firstFrame = 0;
 		}
 		char *pbuf = (char *)fReceiveBuffer;
 		char head[4] = {0x00, 0x00, 0x00, 0x01};
 		FILE *fp = fopen("test.h265", "a+b");
	
 		if(fp){
 			fwrite(head, 4, 1, fp);
 			fwrite(fReceiveBuffer, frameSize, 1, fp);
 			fclose(fp);
 			fp = NULL;
 		}
  }
	//printf("%s----------%d\n",__FILE__, __LINE__); 
// Then continue, to request the next frame of data:
continuePlaying();
}
#else
void DummySink::afterGettingFrame(unsigned frameSize, unsigned numTruncatedBytes,
				  struct timeval presentationTime, unsigned /*durationInMicroseconds*/) {
  // We've just received a frame of data.  (Optionally) print out information about it:
  #if 0
#ifdef DEBUG_PRINT_EACH_RECEIVED_FRAME
  if (fStreamId != NULL) envir() << "Stream \"" << fStreamId << "\"; ";
  envir() << fSubsession.mediumName() << "/" << fSubsession.codecName() << ":\tReceived " << frameSize << " bytes";
  if (numTruncatedBytes > 0) envir() << " (with " << numTruncatedBytes << " bytes truncated)";
  char uSecsStr[6+1]; // used to output the 'microseconds' part of the presentation time
  sprintf(uSecsStr, "%06u", (unsigned)presentationTime.tv_usec);
  envir() << ".\tPresentation time: " << (int)presentationTime.tv_sec << "." << uSecsStr;
  if (fSubsession.rtpSource() != NULL && !fSubsession.rtpSource()->hasBeenSynchronizedUsingRTCP()) {
    envir() << "!"; // mark the debugging output to indicate that this presentation time is not RTCP-synchronized
  }
#ifdef DEBUG_PRINT_NPT
  envir() << "\tNPT: " << fSubsession.getNormalPlayTime(presentationTime);
#endif
  envir() << "\n";
#endif
#endif

  unsigned long long  u64pts = (int)presentationTime.tv_sec*1000*1000+(unsigned)presentationTime.tv_usec;

  (*m_FrameDataCallBack)(fReceiveBuffer, frameSize,u64pts,getCameraId(),0);
  
  // Then continue, to request the next frame of data:
  continuePlaying();
}

#endif

#endif
Boolean DummySink::continuePlaying() {
  if (fSource == NULL) return False; // sanity check (should not happen)

  // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
  fSource->getNextFrame(fReceiveBuffer, DUMMY_SINK_RECEIVE_BUFFER_SIZE,
                        afterGettingFrame, this,
                        onSourceClosure, this);
  return True;
}
