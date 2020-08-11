#ifndef _H265_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#define _H265_LIVE_VIDEO_SERVER_MEDIA_SUBSESSION_HH
#include "H265VideoFileServerMediaSubsession.hh"
#include "H264FramedLiveSource.hh"


using namespace std;
#define ERRO_BUF_LEN 1000000


class H265LiveVideoServerMediaSubssion : public H265VideoFileServerMediaSubsession 
{

public:
    static H265LiveVideoServerMediaSubssion* createNew(UsageEnvironment& env, Boolean reuseFirstSource, table *table1);
    static void pRegister(table*);
    static void pUnRegister(table*);
    static void getBuffer(table*,unsigned char*,int&);
protected: // we're a virtual base class
    H265LiveVideoServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource, table *table1);
	~H265LiveVideoServerMediaSubssion();

protected: // redefined virtual functions
	FramedSource* createNewStreamSource(unsigned clientSessionId,unsigned& estBitrate);
public:
    char fFileName[4];
    table *m_table;
    funcCallBack* m_fb;
};


#endif


