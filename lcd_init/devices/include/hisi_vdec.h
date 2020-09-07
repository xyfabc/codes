#ifndef __HISI_VDEC_H__
#define __HISI_VDEC_H__


#include "hi_type.h"
#include "sample_comm.h"






#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */

extern int img_flag;
int vdec_init(HI_U32 chnCnt, SAMPLE_VO_MODE_E enMode);
int HiSiFrameDataCallBack(void *frameBuf, long frameLength,unsigned long long u64pts,u_int8_t cameraId, void *args);
int enableUserPic(int chn);
int chnStartRecvStream(int chn);
int getChnStatus(int chn);
void clearChnStatus(int chn);
int GetDataForAlg(unsigned char* pBuf, int chn, int *flag);
int GetDataForView(unsigned char* pBuf,int chn);
int GetGBADataForView(unsigned char* pBuf, int chn, int *flag);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

#endif /*__HISI_VDEC_H__ */




