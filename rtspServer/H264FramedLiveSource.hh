#ifndef _H264FRAMEDLIVESOURCE_HH
#define _H264FRAMEDLIVESOURCE_HH

#include <FramedSource.hh>
#include <queue>
#include <list>
#include <sys/time.h>
#include <pthread.h>

#include <iostream>
#include <fstream>
#include <sys/time.h>
#include <stdio.h>
#include <unistd.h>

#define DATA_LEN 1000*1000

using namespace std;

struct table{

    vector<unsigned char *> dataQueue;

    bool clientNumEmpty;

    pthread_mutex_t mutex;

    char const* name;

};
struct funcCallBack{

    void (*getBuffer)(table*,unsigned char*,int&);
    void (*pRegister)(table*);
    void (*pUnRegister)(table*);
    table *ptr;
};

class H264FramedLiveSource : public FramedSource
{
public:
    static H264FramedLiveSource* createNew(UsageEnvironment& env, table * table1,funcCallBack* fb,unsigned preferredFrameSize = 0, unsigned playTimePerFrame = 0);

protected:
    H264FramedLiveSource(UsageEnvironment& env, table * table1,funcCallBack* fb,unsigned preferredFrameSize, unsigned playTimePerFrame);
	~H264FramedLiveSource();
    unsigned maxFrameSize() const { return 100*1024; }
private:
	virtual void doGetNextFrame();
    void ReadData();

protected:
    int *Framed_datasize;//数据区大小
    unsigned char *Framed_databuf;//数据区指针

    unsigned fPlayTimePerFrame;
    unsigned fPreferredFrameSize;
    unsigned fLastPlayTime;
    int readbufsize;//记录已读取数据区大小
    int bufsizel;//记录数据区大小

    funcCallBack* m_funcB;
    table * m_table;

    FILE *my_fp;

private:
     unsigned char H265data[DATA_LEN];

};

#endif

