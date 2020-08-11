#include "H264FramedLiveSource.hh"


using namespace std;
H264FramedLiveSource::H264FramedLiveSource(UsageEnvironment& env, table * table1,funcCallBack* fb, unsigned preferredFrameSize, unsigned playTimePerFrame)
    : FramedSource(env)
{
    Framed_datasize = 0;//数据区大小指针
    Framed_databuf = (unsigned char*)malloc(1024*1024);//数据区指针

    fPlayTimePerFrame = 0;
    fPreferredFrameSize = 0;
    fFrameSize = 0;
    m_funcB = fb;
    m_table = table1;
//    my_fp = fopen("saved.h265", "rb");
    memset(H265data,0,DATA_LEN);

}

H264FramedLiveSource* H264FramedLiveSource::createNew(UsageEnvironment& env, table * table1,funcCallBack* fb,unsigned preferredFrameSize, unsigned playTimePerFrame)
{
    H264FramedLiveSource* newSource = new H264FramedLiveSource(env,table1,fb, preferredFrameSize, playTimePerFrame);


    return newSource;
}

H264FramedLiveSource::~H264FramedLiveSource()
{
    m_funcB->pUnRegister(m_table);
     printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
    // free(Framed_databuf);

}

static FILE *fp;
static void saveFrame1(void *frameBuf, long frameLength,u_int8_t cameraId)
{
    // unsigned char *pu8Buf = NULL;
    //int size = (1920 *1080 * 3)>>1;
    char h264Header[4] = {0x00, 0x00, 0x00, 0x01};
    int i = 0;

    printf("--func:%s--line:%d--\n",__func__,__LINE__);
    static int count = 0;
    if(count == 0){
        fp = fopen("saved.h265", "a+");
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

#if 1
void H264FramedLiveSource::doGetNextFrame()
{
    int len = 0;
   // printf("--func:%s--line:%d-%d-putDataFunc\n",__func__,__LINE__,len);
    m_funcB->getBuffer(m_table,H265data,len);
   // printf("--func:%s--line:%d-%d-putDataFunc\n",__func__,__LINE__,len);
    if(!len){
        // FramedSource::afterGetting(this);
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                                 (TaskFunc*)FramedSource::afterGetting, this);
        return;
    }
    Framed_databuf = H265data;//数据区指针

    bufsizel = len;
    readbufsize = 0;
    if(len>0&&len <fMaxSize){
        fMaxSize = len;
    }else if(len == 0){
        return;
    }

    fFrameSize = fMaxSize;
   // printf("-------------------0x%x-0x%x-0x%x-0x%x----------MaxSize = %lu   %lu\n",Framed_databuf[0],Framed_databuf[1],Framed_databuf[2],Framed_databuf[3], fMaxSize,fFrameSize);
    memcpy(fTo, Framed_databuf, fFrameSize);
//printf("--func:%s--line:%d-%d-putDataFunc\n",__func__,__LINE__,len);

    nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                             (TaskFunc*)FramedSource::afterGetting, this);

    return;
}
#else
void H264FramedLiveSource::doGetNextFrame()
{
    int len = 0;
    unsigned char buf[1000*1000]={0};
    m_funcB->getBuffer(m_table,buf,len);
    len = 10000;
    printf("--func:%s--line:%d-%d-putDataFunc\n",__func__,__LINE__,len);
    if(!len){
        nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
                                                                 (TaskFunc*)FramedSource::afterGetting, this);
        return;
    }

    Framed_databuf = buf;//数据区指针

    bufsizel = len;
    readbufsize = 0;
    if(len>0&&len <fMaxSize){
        fMaxSize = len;
    }else if(len == 0){

        return;
    }

    fFrameSize = fMaxSize;
    int n = fread(Framed_databuf, 10000, 1, my_fp);
    //  printf("-------------------0x%x-0x%x-0x%x-0x%x----------MaxSize = %lu   %lu\n",Framed_databuf[0],Framed_databuf[1],Framed_databuf[2],Framed_databuf[3], fMaxSize,fFrameSize);
    memcpy(fTo, Framed_databuf, 10000);
    //saveFrame1(Framed_databuf, fFrameSize,0);

    FramedSource::afterGetting(this);

    //nextTask() = envir().taskScheduler().scheduleDelayedTask(0,
    //      (TaskFunc*)FramedSource::afterGetting, this);

    return;
}
#endif
void H264FramedLiveSource::ReadData()
{

}

