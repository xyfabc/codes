#ifndef _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define _H264_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#include "H264VideoFileServerMediaSubsession.hh"
#include "H264FramedLiveSource.hh"

using namespace std;


class H264LiveVideoServerMediaSubssion : public H264VideoFileServerMediaSubsession {

public:
    static H264LiveVideoServerMediaSubssion* createNew(UsageEnvironment& env, Boolean reuseFirstSource, table *table1);
    static void pRegister(table*);
    static void pUnRegister(table*);
    static void getBuffer(table*,unsigned char*,int&);
protected: // we're a virtual base class
    H264LiveVideoServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource, table *table1);
	~H264LiveVideoServerMediaSubssion();

protected: // redefined virtual functions
	FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
public:
    char fFileName[4];
    table *m_table;
    funcCallBack* m_fb;
};


#endif


