/*
 * Filename: H264FramedLiveSource.hh
 * Auther: chenxin
 * Create date: 2015/ 10/28
 */


#include "H265LiveVideoServerMediaSubssion.hh"

#include "H264VideoStreamFramer.hh"
#include "H265VideoStreamFramer.hh"
#include "ByteStreamMemoryBufferSource.hh"
#include <algorithm>
using namespace std;

H265LiveVideoServerMediaSubssion* H265LiveVideoServerMediaSubssion::createNew(UsageEnvironment& env, Boolean reuseFirstSource ,table *table1)
{
    return new H265LiveVideoServerMediaSubssion(env, reuseFirstSource,table1);
}
void H265LiveVideoServerMediaSubssion::pRegister(table *table1)
{
    pthread_mutex_lock(&table1->mutex);
    if(!table1->clientNumEmpty){
        table1->clientNumEmpty = true;
    }
    pthread_mutex_unlock(&table1->mutex);
}

void H265LiveVideoServerMediaSubssion::pUnRegister(table* table1)
{
    if(table1->clientNumEmpty){
        pthread_mutex_lock(&table1->mutex);
        table1->clientNumEmpty = false;
        vector<unsigned char*>().swap(table1->dataQueue);
        pthread_mutex_unlock(&table1->mutex);
    }
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

void H265LiveVideoServerMediaSubssion::getBuffer(table* table1,unsigned char* buf,int& len)
{
    int lenTmp = 0;
    len = 0;
   // printf("--func:%s----------------line:%d--tid = %u\n",__func__,__LINE__,pthread_self());
    pthread_mutex_lock(&table1->mutex);


    if(NULL ==table1){
        printf("--func:%s--------NULL ==table1--------line:%d--\n",__func__,__LINE__);
    }
   // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
    if(!table1->dataQueue.empty()){
        // printf("-%s-func:%s----dequeue-----line:%d-%d-putDataFunc\n",__FILE__,__func__,__LINE__,len);
       // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
        unsigned char *tmpbuf = table1->dataQueue.front();
        if(NULL == tmpbuf){
            printf("--func:%s------erro----------line:%d--\n",__func__,__LINE__);
        }
       // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
        memcpy(&len,tmpbuf,sizeof(int));
        memcpy(&lenTmp,tmpbuf+sizeof(int),sizeof(int));
        if(len != lenTmp){
            printf("--func:%s------erro----len != lenTmp------line:%d--\n",__func__,__LINE__);
        }
        //printf("-%s-func:%s---------line:%d-%d-\n",__FILE__,__func__,__LINE__,len);
        printf("--func:%s----------------line:%d--vector size=%d len=%d\n",__func__,__LINE__,table1->dataQueue.size(),len);
        if((len<ERRO_BUF_LEN)&&(len>0)){
            // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
            memcpy(buf,tmpbuf+2*sizeof(int),len);
        }else{
            len = 0;
            printf("--func:%s------erro data len----------line:%d--\n",__func__,__LINE__);
        }
      //  printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
        table1->dataQueue.erase(table1->dataQueue.begin());
        if(table1->dataQueue.size()>20){
           table1->dataQueue.clear();
        }
       // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
        if((len<ERRO_BUF_LEN)&&(len>0)){
            // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
            if(NULL != tmpbuf){
               // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
                delete[] tmpbuf;
            }else{
                printf("--func:%s-----------------no no-------------------------line:%d--\n",__func__,__LINE__);
            }
            tmpbuf = NULL;
        }

       // printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
        // vector<unsigned char*>(table1->dataQueue).swap(table1->dataQueue);
        //printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
    }

    pthread_mutex_unlock(&table1->mutex);
    
    return ;
}

H265LiveVideoServerMediaSubssion::H265LiveVideoServerMediaSubssion(UsageEnvironment& env, Boolean reuseFirstSource, table *table1)
    : H265VideoFileServerMediaSubsession(env, fFileName, reuseFirstSource)//H264VideoFileServerMediaSubsession不是我们需要修改的文件，
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


H265LiveVideoServerMediaSubssion::~H265LiveVideoServerMediaSubssion()
{
    delete m_fb;
    m_fb = NULL;
}

FramedSource* H265LiveVideoServerMediaSubssion::createNewStreamSource(unsigned clientSessionId, unsigned& estBitrate)
{
    
    estBitrate = 1000; // kbps, estimate
    //printf("--func:%s----------------line:%d--\n",__func__,__LINE__);
    m_fb->pRegister(m_table);
    //printf("--func:%s----------------line:%d--\n",__func__,__LINE__);

    //创建视频源
    H264FramedLiveSource* liveSource = H264FramedLiveSource::createNew(envir(), m_table,m_fb);
    //    ByteStreamMemoryBufferSource* liveSource = ByteStreamMemoryBufferSource::createNew(envir(), (u_int8_t*)Server_databuf, *Server_datasize,False);
    if (liveSource == NULL)
    {

        return NULL;
    }

    FramedSource* videoES = liveSource;
    //return H264VideoStreamFramer::createNew(envir(), videoES,False);
    return H265VideoStreamFramer::createNew(envir(), videoES);
    
    // Create a framer for the Video Elementary Stream:
    //return H264VideoStreamFramer::createNew(envir(), liveSource);
}

