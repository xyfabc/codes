#include"itcRtspClient.hpp"
#include<iostream>
#include<string.h>
#include<string>
#include<memory.h>
#include <arpa/inet.h>
#include <linux/if.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
using namespace std;
//#define REQUEST_STREAMING_OVER_TCP true
#define REQUEST_STREAMING_OVER_TCP false
void continueAfterDESCRIBE(RTSPClient* rtspClient, int resultCode, char* resultString){

    do{
        UsageEnvironment& env = rtspClient->envir();
        StreamClientState& scs = ((ItcRtspClient*)rtspClient)->scs;
        if (resultCode != 0) {
            cout<< "Failed to get a SDP description: " << resultString << "\n";
            delete[] resultString;
            break;
        }

        char* const sdpDescription = resultString;

        printf("[URL:\"%s\"]: Got a SDP description: \n%s",rtspClient->url(), sdpDescription);
        scs.session = MediaSession::createNew(env, sdpDescription);
        delete[] sdpDescription;
        if (scs.session == NULL){
            //printf("Failed to create a MediaSession object from the SDP description:%s", rtspClient->url(), env.getResultMsg());
            break;
        }else if(!scs.session->hasSubsessions()){
            printf("[URL:\"%s\"]: This session has no media subsessions (i.e., no \"m=\" lines)",
                   rtspClient->url());

            break;
        }

        scs.iter = new MediaSubsessionIterator(*scs.session);
        setupNextSubsession(rtspClient);
        return;
    }while(0);
}


void setupNextSubsession(RTSPClient* rtspClient) {
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ItcRtspClient*)rtspClient)->scs; // alias

    scs.subsession = scs.iter->next();
    if (scs.subsession != NULL) {
        if (!scs.subsession->initiate()) {
            //cout << "Failed to initiate the \"" << *scs.subsession << "\" subsession: " << env.getResultMsg() << "\n";
            setupNextSubsession(rtspClient); // give up on this subsession; go to the next one
        } else {
            // cout << "Initiated the \"" << *scs.subsession << "\" subsession (";
            if (scs.subsession->rtcpIsMuxed()) {
                // cout << "client port " << scs.subsession->clientPortNum();
            } else {
                // cout<< "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
            }
            // cout << ")\n";

            // Continue setting up this subsession, by sending a RTSP "SETUP" command:
            rtspClient->sendSetupCommand(*scs.subsession, continueAfterSETUP, false, REQUEST_STREAMING_OVER_TCP,false,&((ItcRtspClient*)rtspClient)->ourAuthenticator);
        }
        return;
    }

    // We've finished setting up all of the subsessions.  Now, send a RTSP "PLAY" command to start the streaming:
    if (scs.session->absStartTime() != NULL) {
        // Special case: The stream is indexed by 'absolute' time, so send an appropriate "PLAY" command:
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY, scs.session->absStartTime(), scs.session->absEndTime(),1.0f,&((ItcRtspClient*)rtspClient)->ourAuthenticator);
    } else {
        scs.duration = scs.session->playEndTime() - scs.session->playStartTime();
        rtspClient->sendPlayCommand(*scs.session, continueAfterPLAY,0.0f,-1.0f,1.0f,&((ItcRtspClient*)rtspClient)->ourAuthenticator);
    }
}


void continueAfterSETUP(RTSPClient* rtspClient, int resultCode, char* resultString) {
    do {
        UsageEnvironment& env = rtspClient->envir(); // alias
        StreamClientState& scs = ((ItcRtspClient*)rtspClient)->scs; // alias
        if (resultCode != 0) {
            //cout<< "Failed to set up the \"" << *scs.subsession << "\" subsession: " << resultString << "\n";
            cout<<"fail to setup the subsession "<<resultString<<endl;
            break;
        }

        //cout << "Set up the \"" << *scs.subsession << "\" subsession (";
        if (scs.subsession->rtcpIsMuxed()) {
            env << "client port " << scs.subsession->clientPortNum();
        } else {
            env << "client ports " << scs.subsession->clientPortNum() << "-" << scs.subsession->clientPortNum()+1;
        }
        env << ")\n";

        // Having successfully setup the subsession, create a data sink for it, and call "startPlaying()" on it.
        // (This will prepare the data sink to receive data; the actual flow of data from the client won't start happening until later,
        // after we've sent a RTSP "PLAY" command.)

        scs.subsession->sink = DummySink::createNew(env, *scs.subsession, 1024*1024,((ItcRtspClient*)rtspClient)->m_streamName,rtspClient->url());
        // perhaps use your own custom "MediaSink" subclass instead
        if (scs.subsession->sink == NULL) {
            //        cout<< "Failed to create a data sink for the \"" << *scs.subsession
            //        << "\" subsession: " << env.getResultMsg() << "\n";
            cout<<"scs.subsession->sink is null on continueAfterSETUP"<<endl;
            break;
        }else{
            cout<<"scs.subsession->sink is not null on continueAfterSETUP"<<endl;
        }

        cout<<"scs address"<<(int *)&scs<<endl;

        DummySink *sink=(DummySink *)scs.subsession->sink;
        sink->setRstpClient((ItcRtspClient*)rtspClient);
        //cout<< "Created a data sink for the \"" << *scs.subsession << "\" subsession\n";
        scs.subsession->miscPtr = rtspClient; // a hack to let subsession handler functions get the "RTSPClient" from the subsession
        scs.subsession->sink->startPlaying(*(scs.subsession->readSource()),
                                           subsessionAfterPlaying, scs.subsession);

        // Also set a handler to be called if a RTCP "BYE" arrives for this subsession:
        if (scs.subsession->rtcpInstance() != NULL) {
            scs.subsession->rtcpInstance()->setByeHandler(subsessionByeHandler, scs.subsession);
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
        StreamClientState& scs = ((ItcRtspClient*)rtspClient)->scs; // alias

        if (resultCode != 0) {
            cout<< "Failed to start playing session: " << resultString << "\n";
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

        cout<< "Started playing session";
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
    }else{
        ((ItcRtspClient *)rtspClient)->scheduleKeepAliveCommand();
    }
}

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

void subsessionByeHandler(void* clientData) {
    MediaSubsession* subsession = (MediaSubsession*)clientData;
    RTSPClient* rtspClient = (RTSPClient*)subsession->miscPtr;
    UsageEnvironment& env = rtspClient->envir(); // alias

    //cout << "Received RTCP \"BYE\" on \"" << *subsession << "\" subsession\n";

    // Now act as if the subsession had closed:
    subsessionAfterPlaying(subsession);
}

void streamTimerHandler(void* clientData) {
    ItcRtspClient* rtspClient = (ItcRtspClient*)clientData;
    StreamClientState& scs = rtspClient->scs; // alias

    scs.streamTimerTask = NULL;

    shutdownStream(rtspClient);
}


void shutdownStream(void* data) {
    RTSPClient *rtspClient=(RTSPClient*)data;
    UsageEnvironment& env = rtspClient->envir(); // alias
    StreamClientState& scs = ((ItcRtspClient*)rtspClient)->scs; // alias
    string url=string(rtspClient->url());
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
            rtspClient->sendTeardownCommand(*scs.session, NULL,&((ItcRtspClient*)rtspClient)->ourAuthenticator);
        }
    }

    cout << "Closing the stream:"<<url<<endl;
    Medium::close(rtspClient);

}


void continueAfterKeepAliveCommand(RTSPClient *rtspClient,int resultCode,char *resultString){
    string resultStr="";
    if(resultString!=NULL){
        resultStr+=resultString;
    }
    delete[] resultString;
    resultString=NULL;

    if(resultCode!=0){
        printf("[URL:\%s\"] : \"KeepAlive(OPTIONS) \" request failed:%s ",rtspClient->url(),resultStr.c_str());

    }else{
        ((ItcRtspClient*)rtspClient)->scheduleKeepAliveCommand();
    }
}


void sendKeepRtspAliveCommand(void * clientData){

    RTSPClient *rtspClient=(RTSPClient *)clientData;
    rtspClient->sendOptionsCommand(continueAfterKeepAliveCommand,&((ItcRtspClient*)rtspClient)->ourAuthenticator);
}


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

ItcRtspClient* ItcRtspClient::createNew(UsageEnvironment& env, char const* rtspURL,char const* streamName,
                                        int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum) {
    return new ItcRtspClient(env, rtspURL,streamName, verbosityLevel, applicationName, tunnelOverHTTPPortNum);
}


void ItcRtspClient::setCallBack(void(*p)(unsigned char*,int,int,char const*,void*),void *param){
    this->callBack=p;
    this->param=param;
}
ItcRtspClient::ItcRtspClient(UsageEnvironment& env, char const* rtspURL,char const* streamName,
                             int verbosityLevel, char const* applicationName, portNumBits tunnelOverHTTPPortNum)
    : RTSPClient(env,rtspURL, verbosityLevel, applicationName, tunnelOverHTTPPortNum, -1) {


    ourAuthenticator.setUsernameAndPassword("admin","itc123456");

    keepAliveTimerTask=NULL;
    m_streamName = streamName;


}

ItcRtspClient::~ItcRtspClient() {

    if(keepAliveTimerTask!=NULL){
        this->envir().taskScheduler().unscheduleDelayedTask(keepAliveTimerTask);
        keepAliveTimerTask=NULL;
    }

}


void ItcRtspClient::scheduleKeepAliveCommand(){
    unsigned uSecToDelay=(unsigned)(10*1000*1000);


    keepAliveTimerTask = this->envir().taskScheduler().scheduleDelayedTask(uSecToDelay,
                                                                           sendKeepRtspAliveCommand,
                                                                           this);
}




DummySink* DummySink::createNew(UsageEnvironment& env, MediaSubsession& subsession, int bufferSize,char const* streamName,char const* streamId) {
    return new DummySink(env, subsession, bufferSize,streamName,streamId);
}

DummySink::DummySink(UsageEnvironment& env, MediaSubsession& subsession, int bufferSize,char const* streamName,char const* streamId):
    MediaSink(env),
    fSubsession(subsession),bufferSize(bufferSize){
    fStreamId = strDup(streamId);
    fReceiveBuffer = new u_int8_t[bufferSize];
    m_StreamName = streamName;

    memset(this->fPPS,0,sizeof(this->fPPS));
    memset(this->fSPS,0,sizeof(this->fSPS));
    SliceStartCode[0] = 0x00;
    SliceStartCode[1] = 0x00;
    SliceStartCode[2] = 0x00;
    SliceStartCode[3] = 0x01;
    memcpy(fSPS, SliceStartCode, sizeof(SliceStartCode));
    memcpy(fPPS, SliceStartCode, sizeof(SliceStartCode));
    fSPSSize = 0;
    fPPSSize = 0;
    memset(vps, 0, sizeof(vps));
    memset(sps, 0, sizeof(sps));
    memset(pps, 0, sizeof(pps));
    memset(sei, 0, sizeof(sei));
    vpsSize = 0;
    spsSize = 0;
    ppsSize = 0;
    seiSize = 0;

}


DummySink::~DummySink(){
    delete[] fReceiveBuffer;
    delete[] fStreamId;
}

Boolean DummySink::continuePlaying() {
    if (fSource == NULL) return False; // sanity check (should not happen)

    // Request the next frame of data from our input source.  "afterGettingFrame()" will get called later, when it arrives:
    fSource->getNextFrame(fReceiveBuffer, bufferSize,
                          afterGettingFrame, this,
                          onSourceClosure, this);

    return True;
}

void DummySink::afterGettingFrame(void* clientData, unsigned frameSize, unsigned numTruncatedBytes,
                                  struct timeval presentationTime, unsigned durationInMicroseconds) {
    DummySink* sink = (DummySink*)clientData;
    sink->afterGettingFrame(frameSize, numTruncatedBytes, presentationTime, durationInMicroseconds);
}

FILE *fp;
static void saveFrame1(void *frameBuf, long frameLength,u_int8_t cameraId)
{
    // unsigned char *pu8Buf = NULL;
    //int size = (1920 *1080 * 3)>>1;
    char h264Header[4] = {0x00, 0x00, 0x00, 0x01};
    int i = 0;

    printf("--func:%s--line:%d--\n",__func__,__LINE__);
    static int count = 0;
    if(count == 0){
        fp = fopen("saved1.h265", "a+");
    }
    
    if(count == 10000){
        fclose(fp);
        return;
    }
    count++;
    if(0 != cameraId){

        return;
    }

    printf("--func:%s--line:%d--\n",__func__,__LINE__);
#if 0
    printf("data:");
    for(i=0;i<20;i++){
        printf("0x%x ",pu8Buf[i]);
    }
    printf("\n");
    printf("\tsaved %d bytes\n", frameLength);
#endif	

    fwrite(frameBuf, 1, frameLength, fp);
    printf("--func:%s--line:%d--\n",__func__,__LINE__);
    //fwrite(h264Header, 1, 4, fp);
    fflush(fp);

    // fflush(stdout);


    printf("--func:%s--line:%d--\n",__func__,__LINE__);

}

#include <QDateTime>
#include <QDebug>

void DummySink::afterGettingFrame(
        unsigned frameSize,
        unsigned numTruncatedBytes,
        struct timeval presentationTime,
        unsigned durationInMicroseconds){
    //cout<<"afterGettingFrame"<<endl;
    // qint64 t1 = QDateTime::currentMSecsSinceEpoch();

    //qDebug() << "------1-------" << t2 - t1;
    if(strcmp(fSubsession.mediumName(), "video") == 0)
    {
        // cout<<"-------codes name is "<<fSubsession.codecName()<<endl;
        int bwrite=0;
        if(strcmp(fSubsession.codecName(), "H264") == 0)
        {
            //cout<<"has recivie h264 data"<<endl;
            int length=0;
            {
                memmove(fReceiveBuffer + sizeof(SliceStartCode),fReceiveBuffer,frameSize);
                fReceiveBuffer[0]=0x00;
                fReceiveBuffer[1]=0x00;
                fReceiveBuffer[2]=0x00;
                fReceiveBuffer[3]=0x01;
                bwrite=1;
                length=frameSize+sizeof(SliceStartCode);
            }
            if(bwrite){
                //TODO 放数据
                if(this->client->callBack!=NULL &&this->client->param!=NULL){

                    this->client->callBack(fReceiveBuffer,length,1,m_StreamName,this->client->param);
                }

            }
        }else  if(strcmp(fSubsession.codecName(), "H265") == 0)
        {
            // cout<<"has recivie h265 data"<<frameSize<<endl;
            
            int length=0;

            {
                memmove(fReceiveBuffer + sizeof(SliceStartCode),fReceiveBuffer,frameSize);
                fReceiveBuffer[0]=0x00;
                fReceiveBuffer[1]=0x00;
                fReceiveBuffer[2]=0x00;
                fReceiveBuffer[3]=0x01;

                bwrite=1;

                length=frameSize+sizeof(SliceStartCode);

            }


            if(bwrite){
                //TODO 放数据
                if(this->client->callBack!=NULL &&this->client->param!=NULL){

                    //saveFrame(fReceiveBuffer,length,0);
                    this->client->callBack(fReceiveBuffer,length,1,m_StreamName,this->client->param);
                    //saveFrame(fReceiveBuffer,length,0);
                }

            }
            //            qint64 t2 = QDateTime::currentMSecsSinceEpoch();

            //            qDebug() << "------time-------" << t2 - t1;
        }else if(strcmp(fSubsession.codecName(), "H266") == 0){
            int type = ((fReceiveBuffer[0]) & 0x7E)>>1;
            cout<<"has recivie h265 data  "<<type<<endl;
            switch (type) {
            case 32:
            {
                memcpy(vps+sizeof(SliceStartCode), fReceiveBuffer, frameSize);
                vps[0]=0x00;
                vps[1]=0x00;
                vps[2]=0x00;
                vps[3]=0x01;
                vpsSize = frameSize + sizeof(SliceStartCode);
                break;

            }
            case 33:
            {
                memcpy(sps+sizeof(SliceStartCode), fReceiveBuffer, frameSize);
                sps[0]=0x00;
                sps[1]=0x00;
                sps[2]=0x00;
                sps[3]=0x01;
                spsSize = frameSize + sizeof(SliceStartCode);
                break;
            }
            case 34:
            {
                memcpy(pps+sizeof(SliceStartCode), fReceiveBuffer, frameSize);
                pps[0]=0x00;
                pps[1]=0x00;
                pps[2]=0x00;
                pps[3]=0x01;
                ppsSize = frameSize + sizeof(SliceStartCode);
                break;
            }
            case 39:
            case 40:
            {
                memcpy(sei+sizeof(SliceStartCode), fReceiveBuffer, frameSize );
                sei[0]=0x00;
                sei[1]=0x00;
                sei[2]=0x00;
                sei[3]=0x01;
                seiSize = frameSize + sizeof(SliceStartCode);
                break;
            }
            case 1:
            {
                frameSize = frameSize + sizeof(SliceStartCode);
                bwrite =1;
                break;
            }
            case 19:
            case 20:
            {
                memmove(fReceiveBuffer + vpsSize + spsSize
                        + ppsSize + seiSize+sizeof(SliceStartCode),
                        fReceiveBuffer, frameSize);
                memcpy(fReceiveBuffer, vps, vpsSize);
                memcpy(fReceiveBuffer + vpsSize, sps, spsSize);
                memcpy(fReceiveBuffer + vpsSize + spsSize, pps, ppsSize);
                memcpy(fReceiveBuffer + vpsSize + spsSize + ppsSize, sei, seiSize);
                int index=vpsSize + spsSize + ppsSize+seiSize;
                fReceiveBuffer[index+0]=0x00;
                fReceiveBuffer[index+1]=0x00;
                fReceiveBuffer[index+2]=0x00;
                fReceiveBuffer[index+3]=0x01;

                frameSize = vpsSize + spsSize + ppsSize + seiSize + sizeof(SliceStartCode) + frameSize;
                bwrite = 1;
                break;
            }
            default:
            {
                frameSize = frameSize + sizeof(SliceStartCode);
                bwrite =1;
                break;
            }
            }
            //bwrite=0;
            if(bwrite)
            {
                if(this->client->callBack!=NULL &&this->client->param!=NULL){
                    //cout<<"has recivie h265 data callBack"<<type<<endl;
                    this->client->callBack(fReceiveBuffer,frameSize,1,m_StreamName,this->client->param);
                }
            }
        }
    }else if(strcmp(fSubsession.mediumName(), "audio") == 0){
        /*handler audio data*/
        // cout<<"has recivie audio data"<<endl;


    } else
    {
        printf("Stream type is unknown. neither video, nor audio!");
    }

    if(numTruncatedBytes > 0)
    {
        printf("Receive buffer size is too small, %d bytes is truncated",
               numTruncatedBytes);
    }

    continuePlaying();
}

void DummySink::setRstpClient(ItcRtspClient *client){
    this->client=client;
}



