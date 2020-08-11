/*
 * Filename: H264FramedLiveSource.hh
 * Auther: chenxin
 * Create date: 2015/ 10/28
 */


#include "H264LiveVideoServerMediaSubssion.hh"

#include "H264VideoStreamFramer.hh"
#include "H265VideoStreamFramer.hh"
#include "ByteStreamMemoryBufferSource.hh"
#include <algorithm>
using namespace std;

H264LiveVideoServerMediaSubssion* H264LiveVideoServerMediaSubssion::createNew(UsageEnvironment& env, Boolean reuseFirstSource ,table *table1)
{
    return new H264LiveVideoServerMediaSubssion(env, reuseFirstSource,table1);
}
void H264LiveVideoServerMediaSubssion::pRegister(table *table1)
{
     pthread_mutex_lock(&table1->mutex);
     if(!table1->clientNumEmpty){
         table1->clientNumEmpty = true;
     }
     pthread_mutex_unlock(&table1->mutex);

}

void H264LiveVideoServerMediaSubssion::pUnRegister(table* table1)
{
    if(table1->clientNumEmpty){
        pthread_mutex_lock(&table1->mutex);
        table1->clientNumEmpty = false;
        vector<unsigned char*>().swap(table1->dataQueue);
        pthread_mutex_unlock(&table1->mutex);
    }
}

static FILE *fp;
static void saveFrame11(void *frameBuf, long frameLength,u_int8_t cameraId)
{
   // unsigned char *pu8Buf = NULL;
    //int size = (1920 *1080 * 3)>>1;
    char h264Header[4] = {0x00, 0x00, 0x00, 0x01};
    int i = 0;
     
    printf("--func:%s-%%%%%%%-line:%d--\n",__func__,__LINE__);
    static int count = 0;
    if(count == 0){
        fp = fopen("savedsubssion.h265", "a+");
    }
    
    if(count == 10000){
        fclose(fp);
        return;
    }
    count++;
    if(0 != cameraId){

        return;
    }

     printf("-%s-func:%s--line:%d--\n",__FILE__,__func__,__LINE__);
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
void H264LiveVideoServerMediaSubssion::getBuffer(table* table1,unsigned char* buf,int& len)
{
    len = 0;
    pthread_mutex_lock(&table1->mutex);

    if(!table1->dataQueue.empty()){
        unsigned char *tmpbuf = table1->dataQueue.front();



        memcpy(&len,tmpbuf,sizeof(int));

        memcpy(buf,tmpbuf+2*sizeof(int),len);
        saveFrame(tmpbuf+2*sizeof(int),len,0);

        table1->dataQueue.erase(table1->dataQueue.begin());
        delete[] tmpbuf;
        tmpbuf = NULL;
        vector<unsigned char*>(table1->dataQueue).swap(table1->dataQueue);
    }

    pthread_mutex_unlock(&table1->mutex);
    
    return ;
}

H264LiveVideoServerMediaSubssion::H264LiveVideoServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource, table *table1)
: H264VideoFileServerMediaSubsession(env, fFileName, reuseFirstSource)//H264VideoFileServerMediaSubsession不是我们需要修改的文件，
																	  //但是我们又要用它来初始化我们的函数，
																	  //所以给个空数组进去即可
{

    m_table = table1;
    m_fb = new funcCallBack;
    m_fb->ptr = m_table;
    m_fb->getBuffer = getBuffer;
    m_fb->pRegister = pRegister;
    m_fb->pUnRegister = pUnRegister;

}


H264LiveVideoServerMediaSubssion::~H264LiveVideoServerMediaSubssion()
{
    delete m_fb;
    m_fb = NULL;
}

FramedSource* H264LiveVideoServerMediaSubssion::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    
    estBitrate = 1000; // kbps, estimate

    m_fb->pRegister(m_table);


    //创建视频源
    H264FramedLiveSource* liveSource = H264FramedLiveSource::createNew(envir(), m_table,m_fb);
    //    ByteStreamMemoryBufferSource* liveSource = ByteStreamMemoryBufferSource::createNew(envir(), (u_int8_t*)Server_databuf, *Server_datasize,False);
    if (liveSource == NULL)
    {
      
       return NULL;
    }

        FramedSource* videoES = liveSource;
        //return H264VideoStreamFramer::createNew(envir(), videoES,False);
      //  return H265VideoStreamFramer::createNew(envir(), videoES);
    
	// Create a framer for the Video Elementary Stream:
	//return H264VideoStreamFramer::createNew(envir(), liveSource);
}

