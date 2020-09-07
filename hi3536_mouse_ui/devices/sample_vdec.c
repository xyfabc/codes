/******************************************************************************

  Copyright (C), 2011-2021, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : sample_vdec.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2013/7/1
  Description   :
  History       :
  1.Date        : 2013/7/1
    Author      :
    Modification: Created file

******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>

#include "sample_comm.h"
#include "hisi_vdec.h"
#include "hi_comm_ive.h"
#include "hi_ive.h"
#include "mpi_ive.h"
   #include <unistd.h>

VdecThreadParam stVdecSend[VDEC_MAX_CHN_NUM];
VIDEO_FRAME_INFO_S stUsrPicInfo;


typedef struct hiVDEC_USERPIC_S
{    
    HI_U32   u32PicWidth;
    HI_U32   u32PicHeigth;
    VB_POOL  u32PoolId;
    VB_BLK   u32BlkHandle;
    HI_U32   u32PhyAddr;
    HI_VOID  *pVirAddr;
}VDEC_USERPIC_S;

HI_VOID SAMPLE_VDEC_HandleSig(HI_S32 signo)
{
    if (SIGINT == signo || SIGTSTP == signo)
    {
        SAMPLE_COMM_SYS_Exit();
        printf("\033[0;31mprogram exit abnormally!\033[0;39m\n");
    }

    exit(0);
}

HI_VOID SAMPLE_VDEC_Usage(HI_VOID)
{
    printf("\n\n/************************************/\n");
    printf("please choose the case which you want to run:\n");
    printf("\t0:  VDH H264\n");
    printf("\t1:  VDH H265\n");
    printf("\t2:  VDH MPEG4\n");
    printf("\t3:  JPEG decoding\n");
    printf("\tq:  quit the whole sample\n");
    printf("sample command:");
}


static HI_VOID VDEC_PREPARE_USERPIC(VIDEO_FRAME_INFO_S *pstUsrPicInfo,  char *pFileName, VDEC_USERPIC_S* pstUserPic)
{
    HI_U32 u32PicSize;
    HI_S32 i, j;
    HI_U32 u32YStride;
    HI_U8  *p, *puv, *pu, *pv;
    FILE* fpYUV;
    HI_U32 s32Ret = HI_SUCCESS;
    
    u32PicSize = ALIGN_UP(pstUserPic->u32PicWidth, 16)* ALIGN_UP(pstUserPic->u32PicHeigth, 16)* 3/2;

    pstUserPic->u32BlkHandle = HI_MPI_VB_GetBlock(0, u32PicSize, "anonymous");
    if (VB_INVALID_HANDLE == pstUserPic->u32BlkHandle || HI_ERR_VB_ILLEGAL_PARAM == pstUserPic->u32BlkHandle)
    {
        SAMPLE_PRT("getBlock fail!\n");
        return;
    }
    pstUserPic->u32PhyAddr = HI_MPI_VB_Handle2PhysAddr(pstUserPic->u32BlkHandle);
    pstUserPic->u32PoolId = HI_MPI_VB_Handle2PoolId(pstUserPic->u32BlkHandle);
    HI_MPI_VB_MmapPool(pstUserPic->u32PoolId);
    s32Ret = HI_MPI_VB_GetBlkVirAddr(pstUserPic->u32PoolId, pstUserPic->u32PhyAddr, &pstUserPic->pVirAddr);
    if(s32Ret != HI_SUCCESS)
    {
        HI_MPI_VB_ReleaseBlock(pstUserPic->u32BlkHandle);
        HI_MPI_VB_MunmapPool(pstUserPic->u32PoolId);
        SAMPLE_PRT("HI_MPI_VB_GetBlkVirAddr fail for %#x!\n", s32Ret);
        return;
    }

    puv = malloc(u32PicSize);
    HI_ASSERT(puv != HI_NULL);

    p = (HI_U8 *)pstUserPic->pVirAddr;
    u32YStride = (pstUserPic->u32PicWidth + 15) & ( ~(15) );
    fpYUV = fopen(pFileName,"rb");
    if (fpYUV == NULL)
    {
        printf("can't open file %s in VDEC_PREPARE_USERPIC.\n", pFileName);
    }
    
    /* read the data of Y component*/
    fread(p, 1, pstUserPic->u32PicHeigth * u32YStride, fpYUV);

    /* read the data of UV component*/
    fread(puv, 1, (pstUserPic->u32PicHeigth * u32YStride) >> 1, fpYUV);
    pu = puv;
    pv = puv + ((pstUserPic->u32PicHeigth * u32YStride) >> 2);
    p  = pstUserPic->pVirAddr + (pstUserPic->u32PicHeigth * u32YStride);

    for (i = 0; i < (pstUserPic->u32PicHeigth >> 1); i++)
    {
        for (j=0; j<(pstUserPic->u32PicWidth >> 1); j++)
        {
            p[j*2+1] = pu[j];
            p[j*2+0] = pv[j];
        }

        pu += u32YStride >> 1;
        pv += u32YStride >> 1;
        p  += u32YStride;
    }

    free(puv);
    puv = HI_NULL;
    fclose(fpYUV);

    pstUsrPicInfo->u32PoolId = pstUserPic->u32PoolId;
    pstUsrPicInfo->stVFrame.enCompressMode = COMPRESS_MODE_NONE;
    pstUsrPicInfo->stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
    pstUsrPicInfo->stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
    pstUsrPicInfo->stVFrame.u32Width = pstUserPic->u32PicWidth;
    pstUsrPicInfo->stVFrame.u32Height = pstUserPic->u32PicHeigth;
    pstUsrPicInfo->stVFrame.u32Field = VIDEO_FIELD_FRAME;
    pstUsrPicInfo->stVFrame.u32PhyAddr[0] = pstUserPic->u32PhyAddr;
    pstUsrPicInfo->stVFrame.u32PhyAddr[1] = pstUserPic->u32PhyAddr + (u32YStride * pstUserPic->u32PicHeigth);
    pstUsrPicInfo->stVFrame.u32Stride[0] = u32YStride;
    pstUsrPicInfo->stVFrame.u32Stride[1] = u32YStride;
    pstUsrPicInfo->stVFrame.u64pts = 0;
}


static HI_VOID VDEC_RELEASE_USERPIC(VDEC_USERPIC_S* pstUserPic)
{
    HI_S32 s32Ret = HI_SUCCESS;

    s32Ret = HI_MPI_VB_ReleaseBlock(pstUserPic->u32BlkHandle);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VB_ReleaseBlock fail for %#x!\n", s32Ret);
    }

    s32Ret = HI_MPI_VB_MunmapPool(pstUserPic->u32PoolId);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("HI_MPI_VB_MunmapPool fail for %#x!\n", s32Ret);
    }
}


HI_S32 SAMPLE_VDEC_VdhH264(HI_VOID)
{

}

int enableUserPic(int chn)
{
    HI_S32 s32Ret;

    SAMPLE_PRT("****enableUserPic******\n");
    HI_MPI_VDEC_DisableUserPic(chn);
    s32Ret = HI_MPI_VDEC_SetUserPic(chn, &stUsrPicInfo);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("****enableUserPic****chn%d*******HI_MPI_VDEC_SetUserPic fail for %#x!\n", chn,s32Ret);
    }else{

        SAMPLE_PRT("*****enableUserPic***chn%d*********HI_MPI_VDEC_SetUserPic ok for %#x!\n",chn, s32Ret);
    }

    HI_MPI_VDEC_StopRecvStream(chn);
    s32Ret = HI_MPI_VDEC_EnableUserPic(chn, HI_TRUE);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("HI_MPI_VDEC_EnableUserPic fail for %#x!\n", s32Ret);
        // goto END2;
    }
    printf("---enableUserPic  %d\n",chn);
}
int chnStartRecvStream(int chn)
{
    VDEC_CHN_STAT_S stStat;
    HI_MPI_VDEC_Query(chn, &stStat);
    if(stStat.bStartRecvStream == HI_FALSE){
        // chnStartRecvStream(cameraId);
        HI_MPI_VDEC_DisableUserPic(chn);
        HI_S32 s32Ret = HI_MPI_VDEC_StartRecvStream(chn);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
            // goto END2;
        }
        printf("---yyyyyyyyyyyyyyyyy\n");
        printf("--------chnStartRecvStream---------%d--------\n",chn);
    }else{

    }

}


int yuv420sp2bgaSoft(VIDEO_FRAME_INFO_S  *pFrameInfo,unsigned char* pBuf,int *flag)
{

    HI_U32  uvHeight;
    HI_U32  u32PhyAddr;
    HI_U64  u64Size;
    HI_U8*  pu8VirYAddr;
    HI_U8*  pu8VirUVAddr;
    HI_U8*  pMemContent;
    int h = 0;
    unsigned char *yuv_buf=(unsigned char*)malloc(1920*1080*4);
    unsigned char *pYUV = yuv_buf;
    u64Size = (pFrameInfo->stVFrame.u32Stride[0]) * ((HI_U64)pFrameInfo->stVFrame.u32Height) *3 / 2;
    // u64PhyAddr = pFrameInfo->stVFrame.u64PhyAddr[0];
    u32PhyAddr = pFrameInfo->stVFrame.u32PhyAddr[0];
    printf("%s,%s,%d--------%d-----\n",__FILE__,__FUNCTION__,__LINE__,pFrameInfo->stVFrame.u32Stride[0] * ((HI_U64)pFrameInfo->stVFrame.u32Height));

    pu8VirYAddr = (HI_U8*) HI_MPI_SYS_Mmap(pFrameInfo->stVFrame.u32PhyAddr[0], pFrameInfo->stVFrame.u32Stride[0] * ((HI_U64)pFrameInfo->stVFrame.u32Height));
    if (NULL == pu8VirYAddr)
    {
        printf("HI_MPI_SYS_Mmap fail !\n");

        return 0;
    }
//    pu8VirUVAddr = (HI_U8*) HI_MPI_SYS_Mmap(pFrameInfo->stVFrame.u32PhyAddr[1], pFrameInfo->stVFrame.u32Stride[1] * ((HI_U64)pFrameInfo->stVFrame.u32Height/2));
//    if (NULL == pu8VirUVAddr)
//    {
//        printf("HI_MPI_SYS_Mmap fail !\n");
//        HI_MPI_SYS_Munmap(pu8VirYAddr, pFrameInfo->stVFrame.u32Stride[0]) * ((HI_U64)pFrameInfo->stVFrame.u32Height);
//        return 0;
//    }
//    for (h = 0; h < pFrameInfo->stVFrame.u32Height; h++)
//    {
//        pMemContent = pu8VirYAddr + h * pFrameInfo->stVFrame.u32Stride[0];
//        memcpy(pYUV, pMemContent, pFrameInfo->stVFrame.u32Width);
//        pYUV += pFrameInfo->stVFrame.u32Width;
//    }

//    uvHeight = pFrameInfo->stVFrame.u32Height /2 ;
//    for (h = 0; h < uvHeight; h++)
//    {
//        pMemContent = pu8VirUVAddr + h * pFrameInfo->stVFrame.u32Stride[1];
//        memcpy(pYUV, pMemContent, pFrameInfo->stVFrame.u32Width);
//        pYUV += pFrameInfo->stVFrame.u32Width;
//    }

   // RgbFromYuv420SP(pBuf, yuv_buf, 1920, 1080, 0);
    printf("%s,%s,%d,%d,\n",__FILE__,__FUNCTION__,__LINE__,pFrameInfo->stVFrame.u32Stride[0]);
    printf("%s,%s,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,pFrameInfo->stVFrame.u32Stride[1]);
    HI_MPI_SYS_Munmap(pu8VirYAddr, pFrameInfo->stVFrame.u32Stride[0]) * ((HI_U64)pFrameInfo->stVFrame.u32Height);
   // HI_MPI_SYS_Munmap(pu8VirUVAddr, pFrameInfo->stVFrame.u32Stride[1]) * ((HI_U64)pFrameInfo->stVFrame.u32Height/2);
    printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,pFrameInfo->stVFrame.u32Width,pFrameInfo->stVFrame.u32Height);

}
int mutex = 0;

int yuv420sp2bga(VIDEO_FRAME_INFO_S  *pFrameInfo,unsigned char* pBuf,int *flag)
{
    if(mutex){
        return 0;
    }
    mutex = 1;
   if((pFrameInfo==NULL)&& (pBuf==NULL)){
      return -1;
   }
     HI_S32 s32Ret;
   HI_U32 u32Size = 0;
   IVE_HANDLE pIveHandle;
   IVE_SRC_IMAGE_S pstSrc;
   IVE_DST_IMAGE_S pstDst;
   IVE_CSC_CTRL_S pstCscCtrl;

   HI_BOOL bFinish = HI_FALSE;
   HI_BOOL bBlock = HI_TRUE;

   pstCscCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;


   // src buff
   pstSrc.enType = IVE_IMAGE_TYPE_YUV420SP;
   pstSrc.u32PhyAddr[0] = pFrameInfo->stVFrame.u32PhyAddr[0];
   pstSrc.u32PhyAddr[1] = pFrameInfo->stVFrame.u32PhyAddr[1];
   pstSrc.u32PhyAddr[2] = pFrameInfo->stVFrame.u32PhyAddr[2];

   pstSrc.pu8VirAddr[0] = (HI_U8  *)pFrameInfo->stVFrame.pVirAddr[0];
   pstSrc.pu8VirAddr[1] = (HI_U8  *)pFrameInfo->stVFrame.pVirAddr[1];
   pstSrc.pu8VirAddr[2] = (HI_U8  *)pFrameInfo->stVFrame.pVirAddr[2];

   pstSrc.u16Width = pFrameInfo->stVFrame.u32Width;
   pstSrc.u16Height = pFrameInfo->stVFrame.u32Height;
   pstSrc.u16Stride[0] = pFrameInfo->stVFrame.u32Stride[0];
   pstSrc.u16Stride[1] = pFrameInfo->stVFrame.u32Stride[1];
   pstSrc.u16Stride[2] = pFrameInfo->stVFrame.u32Stride[2];


   //des buf
   pstDst.enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
   pstDst.u16Width   = pstSrc.u16Width ; //高宽同stSrcData
   pstDst.u16Height  = pstSrc.u16Height ;
   pstDst.u16Stride[0]  = pFrameInfo->stVFrame.u32Stride[0]*3;
  // pstDst.u16Stride[1]  = pFrameInfo->stVFrame.u32Stride[1]*3;
   //pstDst.u16Stride[2]  = pFrameInfo->stVFrame.u32Stride[2]*3;

   //printf("%s %s  %d---pFrameInfo->stVFrame.u32Stride[0] = %d-%d\n", __FILE__ ,__func__, __LINE__,pFrameInfo->stVFrame.u32Stride[0],pstSrc.u16Height);

   u32Size = pstDst.u16Stride[0] *  pstDst.u16Height;
   s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&pstDst.u32PhyAddr[0], (HI_VOID**)&pstDst.pu8VirAddr[0], "User", HI_NULL, u32Size);  //1080×1920×3

   if(HI_SUCCESS != s32Ret)
   {
       // HI_MPI_SYS_MmzFree(stSrcData.au64PhyAddr[0], (HI_VOID**)stSrcData.au64VirAddr[0]);
       printf("HI_MPI_SYS_MmzAlloc_Cached Failed!");
       return NULL;
   }

   memset((HI_VOID**)pstDst.pu8VirAddr[0], 0, u32Size);
  // printf("%s %s  %d-u32Size-%d--\n", __FILE__ ,__func__, __LINE__,u32Size);
   s32Ret = HI_MPI_SYS_MmzFlushCache(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0], u32Size);
   if(HI_SUCCESS != s32Ret)
   {
       printf("%s %s  %d-erro-%x--\n", __FILE__ ,__func__, __LINE__,s32Ret);
       //HI_MPI_SYS_MmzFree(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0]);
       return NULL;
   }

   s32Ret =  HI_MPI_IVE_CSC(&pIveHandle, &pstSrc,&pstDst, &pstCscCtrl, HI_TRUE);
   if(HI_SUCCESS != s32Ret)
   {
       printf("%s %s  %d-erro-%x--\n", __FILE__ ,__func__, __LINE__,s32Ret);
       HI_MPI_SYS_MmzFree(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0]);
       return -1;
   }
  // printf("%s %s  %d----\n", __FILE__ ,__func__, __LINE__);
   s32Ret = HI_MPI_IVE_Query(pIveHandle, &bFinish, bBlock);
   while(HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
   {
       usleep(100);
       s32Ret = HI_MPI_IVE_Query(pIveHandle,&bFinish,bBlock);
   }
   if(s32Ret != HI_SUCCESS)
   {
       SAMPLE_PRT("HI_MPI_IVE_Query fail,Error(%#x)\n",s32Ret);
       return -1;
   }
   while (*flag != 1)
   {
      usleep(100);
   }
   if (*flag = 1){
       memcpy(pBuf, pstDst.pu8VirAddr[0], u32Size);
   }else{
       printf("%s %s  %d--busy--\n", __FILE__ ,__func__, __LINE__);
   }

   HI_MPI_SYS_MmzFree(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0]);

   mutex = 0;
}

int GetGBADataForView(unsigned char* pBuf,int chn,int *flag)
{
    HI_S32 s32Ret;
    VIDEO_FRAME_INFO_S  frameInfo;
    s32Ret = HI_MPI_VPSS_GetChnFrame(chn, 0, &frameInfo, -1);
    if(s32Ret == HI_SUCCESS)
    {

        yuv420sp2bga(&frameInfo,pBuf,flag);
        //yuv420sp2bgaSoft(&frameInfo,pBuf,flag);
        HI_MPI_VPSS_ReleaseChnFrame(chn, 0, &frameInfo);
        return 1;
    }
    else
    {
        if(0xA007800E== s32Ret){
            printf("HI_MPI_VPSS_GetChnFrame HI_ERR_VPSS_BUF_EMPTY failed =0x%x\n",s32Ret);
        }else{
            printf("HI_MPI_VPSS_GetChnFrame  failed = 0x%x\n",s32Ret);
        }
        return 0;
    }
}

int img_flag = 1;


int GetDataForView(unsigned char* pBuf,int chn)
{
    HI_S32 s32Ret;
    VIDEO_FRAME_INFO_S  frameInfo;
    if(img_flag!= 1){
      return 0;
    }
    s32Ret = HI_MPI_VPSS_GetChnFrame(chn, 0, &frameInfo, -1);
    unsigned char* pRGB = pBuf;
    // s32Ret = HI_MPI_VDEC_GetImage(chn, &frameInfo, -1);
    if(s32Ret == HI_SUCCESS)
    {
        HI_U32 u32Size = 0;
        IVE_HANDLE pIveHandle;
        IVE_SRC_IMAGE_S pstSrc;
        IVE_DST_IMAGE_S pstDst;
        IVE_CSC_CTRL_S pstCscCtrl;

        HI_BOOL bFinish = HI_FALSE;
        HI_BOOL bBlock = HI_TRUE;

        pstCscCtrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;


        // src buff
        pstSrc.enType = IVE_IMAGE_TYPE_YUV420SP;
        pstSrc.u32PhyAddr[0] = frameInfo.stVFrame.u32PhyAddr[0];
        pstSrc.u32PhyAddr[1] = frameInfo.stVFrame.u32PhyAddr[1];
        pstSrc.u32PhyAddr[2] = frameInfo.stVFrame.u32PhyAddr[2];

        pstSrc.pu8VirAddr[0] = (HI_U8  *)frameInfo.stVFrame.pVirAddr[0];
        pstSrc.pu8VirAddr[1] = (HI_U8  *)frameInfo.stVFrame.pVirAddr[1];
        pstSrc.pu8VirAddr[2] = (HI_U8  *)frameInfo.stVFrame.pVirAddr[2];

        pstSrc.u16Width = frameInfo.stVFrame.u32Width;
        pstSrc.u16Height = frameInfo.stVFrame.u32Height;
        pstSrc.u16Stride[0] = frameInfo.stVFrame.u32Stride[0];
        pstSrc.u16Stride[1] = frameInfo.stVFrame.u32Stride[1];
        pstSrc.u16Stride[2] = frameInfo.stVFrame.u32Stride[2];


        //des buf
        pstDst.enType = IVE_IMAGE_TYPE_U8C3_PACKAGE;
        pstDst.u16Width   = pstSrc.u16Width ; //高宽同stSrcData
        pstDst.u16Height  = pstSrc.u16Height ;
        pstDst.u16Stride[0]  = frameInfo.stVFrame.u32Stride[0]*3;
       // pstDst.u16Stride[1]  = frameInfo.stVFrame.u32Stride[1]*3;
        //pstDst.u16Stride[2]  = frameInfo.stVFrame.u32Stride[2]*3;

        //printf("%s %s  %d---frameInfo.stVFrame.u32Stride[0] = %d-%d\n", __FILE__ ,__func__, __LINE__,frameInfo.stVFrame.u32Stride[0],pstSrc.u16Height);

        u32Size = pstDst.u16Stride[0] *  pstDst.u16Height;
        s32Ret = HI_MPI_SYS_MmzAlloc_Cached(&pstDst.u32PhyAddr[0], (HI_VOID**)&pstDst.pu8VirAddr[0], "User", HI_NULL, u32Size);  //1080×1920×3

        if(HI_SUCCESS != s32Ret)
        {
            // HI_MPI_SYS_MmzFree(stSrcData.au64PhyAddr[0], (HI_VOID**)stSrcData.au64VirAddr[0]);
            printf("HI_MPI_SYS_MmzAlloc_Cached Failed!");
            return NULL;
        }

        memset((HI_VOID**)pstDst.pu8VirAddr[0], 0, u32Size);
       // printf("%s %s  %d-u32Size-%d--\n", __FILE__ ,__func__, __LINE__,u32Size);
        s32Ret = HI_MPI_SYS_MmzFlushCache(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0], u32Size);
        if(HI_SUCCESS != s32Ret)
        {
            printf("%s %s  %d-erro-%x--\n", __FILE__ ,__func__, __LINE__,s32Ret);
            //HI_MPI_SYS_MmzFree(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0]);
            return NULL;
        }

       // IVE_FILTER_AND_CSC_CTRL_S  ctrl;
      //  ctrl.enMode = IVE_CSC_MODE_VIDEO_BT601_YUV2RGB;
        s32Ret =  HI_MPI_IVE_CSC(&pIveHandle, &pstSrc,&pstDst, &pstCscCtrl, HI_TRUE);
        // s32Ret =  HI_MPI_IVE_FilterAndCSC(&pIveHandle, &pstSrc,&pstDst, &ctrl, HI_TRUE);
        if(HI_SUCCESS != s32Ret)
        {
            printf("%s %s  %d-erro-%x--\n", __FILE__ ,__func__, __LINE__,s32Ret);
            HI_MPI_SYS_MmzFree(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0]);
            return -1;
        }
       // printf("%s %s  %d----\n", __FILE__ ,__func__, __LINE__);
        s32Ret = HI_MPI_IVE_Query(pIveHandle, &bFinish, bBlock);
        while(HI_ERR_IVE_QUERY_TIMEOUT == s32Ret)
        {
            usleep(100);
            s32Ret = HI_MPI_IVE_Query(pIveHandle,&bFinish,bBlock);
        }
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("HI_MPI_IVE_Query fail,Error(%#x)\n",s32Ret);
            return -1;
        }
      //  printf("%s %s  %d----\n", __FILE__ ,__func__, __LINE__);

        // memcpy(pBuf, pstDst.pu8VirAddr[0], pstDst.u16Width*pstDst.u16Height);
        if(img_flag){
         memcpy(pRGB, pstDst.pu8VirAddr[0], u32Size);
        }

//        for (h = 0; h < pstDst.u16Height; h++)
//        {
//            pMemContent = pstDst.pu8VirAddr[0] + h * pstDst.u16Stride[0];
//          //  printf("%s %s  %d--%d-%d-\n", __FILE__ ,__func__, __LINE__,h,h * pstDst.u16Stride[0]);

//            memcpy(pRGB, pMemContent, pstDst.u16Stride[0]);
//            pRGB += pstDst.u16Stride[0];
//        }

 //printf("%s %s  %d----\n", __FILE__ ,__func__, __LINE__);

        HI_MPI_SYS_MmzFree(pstDst.u32PhyAddr[0], (HI_VOID**)pstDst.pu8VirAddr[0]);

// printf("%s %s  %d----\n", __FILE__ ,__func__, __LINE__);
        HI_MPI_VPSS_ReleaseChnFrame(chn, 0, &frameInfo);

        // printf("%s %s  %d----\n", __FILE__ ,__func__, __LINE__);
        //HI_MPI_VDEC_ReleaseImage(chn,&frameInfo);
        return 1;
    }
    else
    {
        if(0xA007800E== s32Ret){
            printf("HI_MPI_VPSS_GetChnFrame HI_ERR_VPSS_BUF_EMPTY failed =0x%x\n",s32Ret);
        }else{
            printf("HI_MPI_VPSS_GetChnFrame  failed = 0x%x\n",s32Ret);
        }
        return 0;
    }
}


#if 1
int GetDataForAlg(unsigned char* pBuf,int chn,int *flag)
{
    // printf("%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__);
    HI_S32 s32Ret;
    VIDEO_FRAME_INFO_S  frameInfo;

    s32Ret = HI_MPI_VPSS_GetChnFrame(chn, 0, &frameInfo, -1);
    //s32Ret = HI_MPI_VPSS_GetGrpFrame(chn, &frameInfo, -1);
    // s32Ret = HI_MPI_VDEC_GetImage(chn, &frameInfo, -1);
    if(s32Ret == HI_SUCCESS)
    {

        HI_U32  uvHeight;
        HI_U32  u32PhyAddr;
        HI_U64  u64Size;
        HI_U8*  pu8VirYAddr;
        HI_U8*  pu8VirUVAddr;
        HI_U8*  pMemContent;
        HI_U32  y_size = frameInfo.stVFrame.u32Stride[0] * ((HI_U64)frameInfo.stVFrame.u32Height);
        HI_U32  uv_size =  frameInfo.stVFrame.u32Stride[1] * ((HI_U64)frameInfo.stVFrame.u32Height/2);
        int h = 0;
#if 1
        u64Size = (frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height) *3 / 2;
        // u64PhyAddr = frameInfo.stVFrame.u64PhyAddr[0];
        u32PhyAddr = frameInfo.stVFrame.u32PhyAddr[0];
        // printf("%s,%s,%d------%d--%d-----\n",__FILE__,__FUNCTION__,__LINE__,((HI_U64)frameInfo.stVFrame.u32Width) , ((HI_U64)frameInfo.stVFrame.u32Height));

        //        pu8VirYAddr = frameInfo.stVFrame.pVirAddr[0];
        //        pu8VirUVAddr = frameInfo.stVFrame.pVirAddr[1];
        pu8VirYAddr = (HI_U8*) HI_MPI_SYS_Mmap(frameInfo.stVFrame.u32PhyAddr[0],y_size);
        if (NULL == pu8VirYAddr)
        {
            printf("HI_MPI_SYS_Mmap fail !\n");

            return 0;
        }
        pu8VirUVAddr = (HI_U8*) HI_MPI_SYS_Mmap(frameInfo.stVFrame.u32PhyAddr[1],uv_size);
        if (NULL == pu8VirUVAddr)
        {
            printf("HI_MPI_SYS_Mmap fail !\n");
            HI_MPI_SYS_Munmap(pu8VirYAddr, frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height);
            return 0;
        }
//        while (*flag != 1)
//        {
//           usleep(100);
//        }
        unsigned char* pRGB = pBuf;
        for (h = 0; h < frameInfo.stVFrame.u32Height; h++)
        {
            pMemContent = pu8VirYAddr + h * frameInfo.stVFrame.u32Stride[0];
            memcpy(pRGB, pMemContent, frameInfo.stVFrame.u32Width);
            pRGB += frameInfo.stVFrame.u32Width;
        }

        uvHeight = frameInfo.stVFrame.u32Height /2 ;
        for (h = 0; h < uvHeight; h++)
        {
            pMemContent = pu8VirUVAddr + h * frameInfo.stVFrame.u32Stride[1];
            memcpy(pRGB, pMemContent, frameInfo.stVFrame.u32Width);
            pRGB += frameInfo.stVFrame.u32Width;
        }
      //  RgbFromYuv420SP(pBuf, pRGB, 1920, 1080, 0);


        HI_MPI_SYS_Munmap(pu8VirYAddr, y_size);
        HI_MPI_SYS_Munmap(pu8VirUVAddr, uv_size);
        //printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,frameInfo.stVFrame.u32Width,frameInfo.stVFrame.u32Height);
        //        memcpy(pBuf, (unsigned char*)frameInfo.stVFrame.u64VirAddr[0], FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT);
        //        memcpy(pBuf+FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT, (unsigned char*)frameInfo.stVFrame.u64VirAddr[1], FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT/2);
#endif
        HI_MPI_VPSS_ReleaseChnFrame(chn, 0, &frameInfo);
        //HI_MPI_VPSS_ReleaseGrpFrame(chn, &frameInfo);
        //HI_MPI_VDEC_ReleaseImage(chn,&frameInfo);
        return 1;
    }
    else
    {
        if(0xA007800E== s32Ret){
            printf("HI_MPI_VPSS_GetChnFrame HI_ERR_VPSS_BUF_EMPTY failed =0x%x\n",s32Ret);
        }else{
            printf("HI_MPI_VPSS_GetChnFrame  failed = 0x%x\n",s32Ret);
        }
        return 0;
    }
}

int GetRGBDataForAlgy(unsigned char* pBuf,int chn)
{
    printf("%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__);
    HI_S32 s32Ret;
    VIDEO_FRAME_INFO_S  frameInfo;
     unsigned char* pRGB = pBuf;
    s32Ret = HI_MPI_VPSS_GetChnFrame(chn, 0, &frameInfo, -1);
    // s32Ret = HI_MPI_VDEC_GetImage(chn, &frameInfo, -1);
    if(s32Ret == HI_SUCCESS)
    {

        HI_U32  uvHeight;
        HI_U64  u64PhyAddr;
        HI_U32  u32PhyAddr;
        HI_U64  u64Size;
        HI_U32  u32DataSize;
        HI_U8*  pu8VirYAddr;
        HI_U8*  pu8VirUVAddr;
        HI_U8*  pMemContent;
        int h = 0;
#if 1
        u64Size = (frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height) *3 / 2;
        // u64PhyAddr = frameInfo.stVFrame.u64PhyAddr[0];
        u32PhyAddr = frameInfo.stVFrame.u32PhyAddr[0];
        printf("%s,%s,%d--------%d-----\n",__FILE__,__FUNCTION__,__LINE__,frameInfo.stVFrame.u32Stride[0] * ((HI_U64)frameInfo.stVFrame.u32Height));

        pu8VirYAddr = (HI_U8*) HI_MPI_SYS_Mmap(frameInfo.stVFrame.u32PhyAddr[0], frameInfo.stVFrame.u32Stride[0] * ((HI_U64)frameInfo.stVFrame.u32Height));
        if (NULL == pu8VirYAddr)
        {
            printf("HI_MPI_SYS_Mmap fail !\n");

            return 0;
        }
        pu8VirUVAddr = (HI_U8*) HI_MPI_SYS_Mmap(frameInfo.stVFrame.u32PhyAddr[1], frameInfo.stVFrame.u32Stride[1] * ((HI_U64)frameInfo.stVFrame.u32Height/2));
        if (NULL == pu8VirUVAddr)
        {
            printf("HI_MPI_SYS_Mmap fail !\n");
            HI_MPI_SYS_Munmap(pu8VirYAddr, frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height);
            return 0;
        }
        for (h = 0; h < frameInfo.stVFrame.u32Height; h++)
        {
            pMemContent = pu8VirYAddr + h * frameInfo.stVFrame.u32Stride[0];
            memcpy(pRGB, pMemContent, frameInfo.stVFrame.u32Width);
            pRGB += frameInfo.stVFrame.u32Width;
        }

        uvHeight = frameInfo.stVFrame.u32Height /2 ;
        for (h = 0; h < uvHeight; h++)
        {
            pMemContent = pu8VirUVAddr + h * frameInfo.stVFrame.u32Stride[1];
            memcpy(pRGB, pMemContent, frameInfo.stVFrame.u32Width);
            pRGB += frameInfo.stVFrame.u32Width;
        }

        printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,u32DataSize,frameInfo.stVFrame.u32Stride[0]);
        printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,u32DataSize,frameInfo.stVFrame.u32Stride[1]);
        HI_MPI_SYS_Munmap(pu8VirYAddr, frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height);
        HI_MPI_SYS_Munmap(pu8VirUVAddr, frameInfo.stVFrame.u32Stride[1]) * ((HI_U64)frameInfo.stVFrame.u32Height/2);
        printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,frameInfo.stVFrame.u32Width,frameInfo.stVFrame.u32Height);
        //        memcpy(pBuf, (unsigned char*)frameInfo.stVFrame.u64VirAddr[0], FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT);
        //        memcpy(pBuf+FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT, (unsigned char*)frameInfo.stVFrame.u64VirAddr[1], FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT/2);
#endif
        HI_MPI_VPSS_ReleaseChnFrame(chn, 0, &frameInfo);
        //HI_MPI_VDEC_ReleaseImage(chn,&frameInfo);
        return 1;
    }
    else
    {
        if(0xA007800E== s32Ret){
            printf("HI_MPI_VPSS_GetChnFrame HI_ERR_VPSS_BUF_EMPTY failed =0x%x\n",s32Ret);
        }else{
            printf("HI_MPI_VPSS_GetChnFrame  failed = 0x%x\n",s32Ret);
        }
        return 0;
    }
}


#else
int GetDataForAlg(unsigned char* pBuf,int chn)
{
    printf("%s,%s,%d\n",__FILE__,__FUNCTION__,__LINE__);
    HI_S32 s32Ret;
    VIDEO_FRAME_INFO_S  frameInfo;
    s32Ret = HI_MPI_VPSS_GetChnFrame(chn, 0, &frameInfo, -1);
    // s32Ret = HI_MPI_VDEC_GetImage(chn, &frameInfo, -1);
    if(s32Ret == HI_SUCCESS)
    {

        HI_U32  uvHeight;
        HI_U64  u64PhyAddr;
        HI_U32  u32PhyAddr;
        HI_U64  u64Size;
        HI_U32  u32DataSize;
        HI_U8*  pu8VirYAddr;
        HI_U8*  pu8VirUVAddr;
        HI_U8*  pMemContent;
        int h = 0;
#if 1
        u64Size = (frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height) *3 / 2;
        // u64PhyAddr = frameInfo.stVFrame.u64PhyAddr[0];
        u32PhyAddr = frameInfo.stVFrame.u32PhyAddr[0];
        printf("%s,%s,%d--------%d-----\n",__FILE__,__FUNCTION__,__LINE__,frameInfo.stVFrame.u32Stride[0] * ((HI_U64)frameInfo.stVFrame.u32Height));

        pu8VirYAddr = (HI_U8*) HI_MPI_SYS_Mmap(frameInfo.stVFrame.u32PhyAddr[0], frameInfo.stVFrame.u32Stride[0] * ((HI_U64)frameInfo.stVFrame.u32Height));
        if (NULL == pu8VirYAddr)
        {
            printf("HI_MPI_SYS_Mmap fail !\n");

            return 0;
        }
        pu8VirUVAddr = (HI_U8*) HI_MPI_SYS_Mmap(frameInfo.stVFrame.u32PhyAddr[1], frameInfo.stVFrame.u32Stride[1] * ((HI_U64)frameInfo.stVFrame.u32Height/2));
        if (NULL == pu8VirUVAddr)
        {
            printf("HI_MPI_SYS_Mmap fail !\n");
            HI_MPI_SYS_Munmap(pu8VirYAddr, frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height);
            return 0;
        }
        for (h = 0; h < frameInfo.stVFrame.u32Height; h++)
        {
            pMemContent = pu8VirYAddr + h * frameInfo.stVFrame.u32Stride[0];
            memcpy(pBuf, pMemContent, frameInfo.stVFrame.u32Width);
            pBuf += frameInfo.stVFrame.u32Width;
        }

        uvHeight = frameInfo.stVFrame.u32Height /2 ;
        for (h = 0; h < uvHeight; h++)
        {
            pMemContent = pu8VirUVAddr + h * frameInfo.stVFrame.u32Stride[1];
            memcpy(pBuf, pMemContent, frameInfo.stVFrame.u32Width);
            pBuf += frameInfo.stVFrame.u32Width;
        }

        printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,u32DataSize,frameInfo.stVFrame.u32Stride[0]);
        printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,u32DataSize,frameInfo.stVFrame.u32Stride[1]);
        HI_MPI_SYS_Munmap(pu8VirYAddr, frameInfo.stVFrame.u32Stride[0]) * ((HI_U64)frameInfo.stVFrame.u32Height);
        HI_MPI_SYS_Munmap(pu8VirUVAddr, frameInfo.stVFrame.u32Stride[1]) * ((HI_U64)frameInfo.stVFrame.u32Height/2);
        printf("%s,%s,%d,%d,%d\n",__FILE__,__FUNCTION__,__LINE__,frameInfo.stVFrame.u32Width,frameInfo.stVFrame.u32Height);
        //        memcpy(pBuf, (unsigned char*)frameInfo.stVFrame.u64VirAddr[0], FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT);
        //        memcpy(pBuf+FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT, (unsigned char*)frameInfo.stVFrame.u64VirAddr[1], FACE_IMAGE_WIDTH * FACE_IMAGE_HEIGHT/2);
#endif
        HI_MPI_VPSS_ReleaseChnFrame(chn, 0, &frameInfo);
        //HI_MPI_VDEC_ReleaseImage(chn,&frameInfo);
        return 1;
    }
    else
    {
        if(0xA007800E== s32Ret){
            printf("HI_MPI_VPSS_GetChnFrame HI_ERR_VPSS_BUF_EMPTY failed =0x%x\n",s32Ret);
        }else{
            printf("HI_MPI_VPSS_GetChnFrame  failed = 0x%x\n",s32Ret);
        }
        return 0;
    }
}
#endif
HI_S32 SAMPLE_VDEC_VdhH265(HI_U32 chnCnt, SAMPLE_VO_MODE_E enMode)
{
    VB_CONF_S stVbConf, stModVbConf;
    HI_S32 i, s32Ret = HI_SUCCESS;
    VDEC_CHN_ATTR_S stVdecChnAttr[VDEC_MAX_CHN_NUM];
    
    VPSS_GRP_ATTR_S stVpssGrpAttr[VDEC_MAX_CHN_NUM];
    SIZE_S stSize;
    VO_DEV VoDev = 0;
    VO_LAYER VoLayer = 0;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;
    HI_U32 u32VdCnt = chnCnt, u32GrpCnt = chnCnt;
    //pthread_t	VdecThread[2*VDEC_MAX_CHN_NUM];
    VDEC_PRTCL_PARAM_S stPrtclParam;
    VDEC_USERPIC_S stUserPic;
    //VIDEO_FRAME_INFO_S stUsrPicInfo;
    int nVoDev;

    stSize.u32Width = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;

    //sys exit add by eric.xi@zmj at 20191120
    //    RetValue = HI_MPI_SYS_Exit();
    //    if (RetValue != 0)
    //    {
    //        printf("[SDK] HI_MPI_SYS_Exit Fail = 0x%x\n", RetValue);
    //    }
    //    RetValue = HI_MPI_VB_Exit();
    //    if (RetValue != 0)
    //    {
    //        printf("[SDK] HI_MPI_VB_Exit Fail = 0x%x\n", RetValue);
    //    }
    nVoDev = 0;
    for(i = 0; i < VO_MAX_CHN_NUM; i ++){
        HI_MPI_VO_DisableChn(nVoDev, i);
    }
    //    HI_MPI_VO_DisableVideoLayer(nVoDev);
    //    HI_MPI_VO_Disable(nVoDev);
    SAMPLE_COMM_VO_StopLayer(VoLayer);
    SAMPLE_COMM_VO_StopDev(VoDev);
    SAMPLE_COMM_SYS_Exit();



    /************************************************
    step1:  init SYS and common VB
    *************************************************/
    SAMPLE_COMM_VDEC_Sysconf(&stVbConf, &stSize);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        //goto END1;
    }

    /************************************************
      step2:  init mod common VB
    *************************************************/
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_H265, &stSize, u32VdCnt);
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        //goto END1;
    }

    /************************************************
    step3:  start VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ChnAttr(u32VdCnt, &stVdecChnAttr[0], PT_H265, &stSize);

    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdCnt, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        //goto END2;
    }

    /***  set the protocol param (should stop to receive stream first) ***/
    for(i=0; i<u32VdCnt; i++)
    {
        s32Ret = HI_MPI_VDEC_StopRecvStream(i);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("stop VDEC fail for %#x!\n", s32Ret);
            // goto END2;
        }

        HI_MPI_VDEC_GetProtocolParam(i, &stPrtclParam);
        stPrtclParam.enType = PT_H265;
        stPrtclParam.stH265PrtclParam.s32MaxSpsNum   = 2;
        stPrtclParam.stH265PrtclParam.s32MaxPpsNum   = 55;
        stPrtclParam.stH265PrtclParam.s32MaxSliceSegmentNum = 100;
        stPrtclParam.stH265PrtclParam.s32MaxVpsNum = 10;
        s32Ret = HI_MPI_VDEC_SetProtocolParam(i, &stPrtclParam);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("stop VDEC fail for %#x!\n", s32Ret);
            //goto END2;
        }
        //eric.xi add
        s32Ret = HI_MPI_VDEC_SetDisplayMode(i, VIDEO_DISPLAY_MODE_PREVIEW);
        if(s32Ret != HI_SUCCESS){
            SAMPLE_PRT("VDEC SetDisplayMode fail for %#x!\n", s32Ret);
            //goto END2;
        }

        s32Ret = HI_MPI_VDEC_StartRecvStream(i);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
            // goto END2;
        }
    }

    /*** set the attr of user pic ***/
    stUserPic.u32PicWidth = 960;
    stUserPic.u32PicHeigth = 576;
    VDEC_PREPARE_USERPIC(&stUsrPicInfo,  "UserPic_960x576.yuv", &stUserPic);
    for(i=0; i<u32VdCnt; i++)
    {
        s32Ret = HI_MPI_VDEC_SetUserPic(i, &stUsrPicInfo);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("********chn%d*******HI_MPI_VDEC_SetUserPic fail for %#x!\n", i,s32Ret);
            //goto END2;
        }else{

            SAMPLE_PRT("********chn%d*********HI_MPI_VDEC_SetUserPic ok for %#x!\n",i, s32Ret);
        }
        HI_MPI_VDEC_EnableUserPic(i, HI_TRUE);
    }

    /************************************************
    step4:  start VPSS
    *************************************************/
    SAMPLE_COMM_VDEC_VpssGrpAttr(u32GrpCnt, &stVpssGrpAttr[0], &stSize);

    s32Ret = SAMPLE_COMM_VPSS_Start(u32GrpCnt, &stSize, 1, &stVpssGrpAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
        // goto END3;
    }
    /************************************************
    step5:  start VO
    *************************************************/
    VoDev = SAMPLE_VO_DEV_DHD0;
    VoLayer = SAMPLE_VO_LAYER_VHD0;

    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P60;
    stVoPubAttr.enIntfType = VO_INTF_HDMI|VO_INTF_VGA;

    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        //goto END4_1;
    }
    if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStop())
    {
        SAMPLE_PRT("Stop SAMPLE_COMM_VO_HdmiStart failed!\n");
    }
    if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
        //goto END4_2;
    }


    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync, \
                                  &stVoLayerAttr.stDispRect.u32Width, &stVoLayerAttr.stDispRect.u32Height, &stVoLayerAttr.u32DispFrmRt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        //goto  END4_2;
    }

    stVoLayerAttr.stImageSize.u32Width = stVoLayerAttr.stDispRect.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVoLayerAttr.stDispRect.u32Height;
    stVoLayerAttr.bClusterMode = HI_FALSE;
    stVoLayerAttr.bDoubleFrame = HI_FALSE;
    stVoLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;
    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stVoLayerAttr);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartLayer fail for %#x!\n", s32Ret);
        // goto END4_3;
    }


    //s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, VO_MODE_16MUX);
    s32Ret = ZMJ_COMM_VO_StartChn(VoLayer, enMode);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("SAMPLE_COMM_VO_StartChn fail for %#x!\n", s32Ret);
        // goto END4_4;
    }

    /************************************************
    step6:  VDEC bind VPSS
    *************************************************/
#if 0
    for(i=0; i<u32VdCnt; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            //  goto END5;
        }
    }
#else
    for(i=0; i<4; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            //  goto END5;
        }
    }
    for(i=0; i<4; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(i, i+4);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            //  goto END5;
        }
    }
#endif
    /************************************************
    step7:  VPSS bind VO
    *************************************************/

#if 0
    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
            // goto END6;
        }
    }

#else

    for(i=0; i<4; i++)
    {
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, i, i+4, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
            // goto END6;
        }
    }
#endif

    //    s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, 0, 0, VPSS_CHN0);
    //    if(s32Ret != HI_SUCCESS)
    //    {
    //        SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
    //        // goto END6;
    //    }

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    
    SAMPLE_COMM_VDEC_ThreadParam(u32VdCnt, &stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_H265_PATH);

    SAMPLE_PRT("sample init ok ---------------------------------------\n");

    // HI_MPI_VDEC_StopRecvStream(3);
    // HI_MPI_VDEC_EnableUserPic(3, HI_TRUE);

    //  SAMPLE_COMM_VDEC_StartSendStream(u32VdCnt, &stVdecSend[0], &VdecThread[0]);

    //    pthread_t		videotid[2];
    //    pthread_create(&videotid, NULL, videoget, NULL);
    //    pthread_create(&videotid, NULL, videocheck, NULL);
#if 0

    /***  get the stat info of luma pix  ***/
    SAMPLE_COMM_VDEC_StartGetLuma(u32VdCnt, &stVdecSend[0], &VdecThread[0]);

    /***  control the send stream thread and get luma info thread  ***/
    SAMPLE_COMM_VDEC_CmdCtrl(u32VdCnt, &stVdecSend[0]);

    SAMPLE_COMM_VDEC_StopSendStream(u32VdCnt, &stVdecSend[0], &VdecThread[0]);

    SAMPLE_COMM_VDEC_StopGetLuma(u32VdCnt, &stVdecSend[0], &VdecThread[0]);


END6:
    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = SAMPLE_COMM_VO_UnBindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("vpss unbind vo fail for %#x!\n", s32Ret);
        }
    }

END5:
    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_UnBindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {
            SAMPLE_PRT("vdec unbind vpss fail for %#x!\n", s32Ret);
        }
    }

END4_4:
    SAMPLE_COMM_VO_StopChn(VoLayer, VO_MODE_4MUX);
END4_3: 
    SAMPLE_COMM_VO_StopLayer(VoLayer);
END4_2: 
#ifndef HI_FPGA
    SAMPLE_COMM_VO_HdmiStop();
#endif
END4_1:
    SAMPLE_COMM_VO_StopDev(VoDev);

END3:
    SAMPLE_COMM_VPSS_Stop(u32GrpCnt, VPSS_CHN0);

END2:
    SAMPLE_COMM_VDEC_Stop(u32VdCnt);

END1:
    SAMPLE_COMM_SYS_Exit();

    return s32Ret;
#endif
    return HI_SUCCESS;

}




HI_S32 SAMPLE_VDEC_VdhMpeg4(HI_VOID)
{

}

HI_S32 SAMPLE_VDEC_JpegDecoding(HI_VOID)
{	

}

int vdec_init(HI_U32 chnCnt, SAMPLE_VO_MODE_E enMode)
{
    HI_S32 s32Ret = HI_SUCCESS;
    
    signal(SIGINT, SAMPLE_VDEC_HandleSig);
    signal(SIGTERM, SAMPLE_VDEC_HandleSig);

    SAMPLE_VDEC_VdhH265(chnCnt,enMode);


    return s32Ret;
}



