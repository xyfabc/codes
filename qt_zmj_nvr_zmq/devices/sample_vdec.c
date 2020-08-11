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
    VB_CONF_S stVbConf, stModVbConf;
    HI_S32 i, s32Ret = HI_SUCCESS;
    VDEC_CHN_ATTR_S stVdecChnAttr[VDEC_MAX_CHN_NUM];
    VdecThreadParam stVdecSend[VDEC_MAX_CHN_NUM];
    VPSS_GRP_ATTR_S stVpssGrpAttr[VDEC_MAX_CHN_NUM];
    SIZE_S stSize, stRotateSize;
    VO_DEV VoDev;
    VO_LAYER VoLayer;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;
    ROTATE_E enRotate;
    HI_U32 u32VdCnt = 4, u32GrpCnt = 4;
    pthread_t   VdecThread[2*VDEC_MAX_CHN_NUM];

    stSize.u32Width = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;
	
    /************************************************
    step1:  init SYS and common VB 
    *************************************************/
    SAMPLE_COMM_VDEC_Sysconf(&stVbConf, &stSize);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step2:  init mod common VB
    *************************************************/
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_H264, &stSize, u32VdCnt);	
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);
    if(s32Ret != HI_SUCCESS)
    {	    	
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step3:  start VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ChnAttr(u32VdCnt, &stVdecChnAttr[0], PT_H264, &stSize);
    s32Ret = SAMPLE_COMM_VDEC_Start(u32VdCnt, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END2;
    }

    /************************************************
    step4:  start VPSS
    *************************************************/
    stRotateSize.u32Width = stRotateSize.u32Height = MAX2(stSize.u32Width, stSize.u32Height);
    SAMPLE_COMM_VDEC_VpssGrpAttr(u32GrpCnt, &stVpssGrpAttr[0], &stRotateSize);
    s32Ret = SAMPLE_COMM_VPSS_Start(u32GrpCnt, &stRotateSize, 1, &stVpssGrpAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	    
        SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
        goto END3;
    }

    /************************************************
    step5:  start VO
    *************************************************/	
    VoDev = SAMPLE_VO_DEV_DHD0;
    VoLayer = SAMPLE_VO_LAYER_VHD0;
    
#ifdef HI_FPGA
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;
    stVoPubAttr.enIntfType = VO_INTF_VGA;
#else
    stVoPubAttr.enIntfSync = VO_OUTPUT_3840x2160_30;
    stVoPubAttr.enIntfType = VO_INTF_HDMI;
#endif
    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_1;
    }

#ifndef HI_FPGA
    if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
        goto END4_2;
    }
#endif

    stVoLayerAttr.bClusterMode = HI_FALSE;
    stVoLayerAttr.bDoubleFrame = HI_FALSE;
    stVoLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;    

    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync, \
        &stVoLayerAttr.stDispRect.u32Width, &stVoLayerAttr.stDispRect.u32Height, &stVoLayerAttr.u32DispFrmRt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto  END4_2;
    }
    stVoLayerAttr.stImageSize.u32Width = stVoLayerAttr.stDispRect.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVoLayerAttr.stDispRect.u32Height;
    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stVoLayerAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("SAMPLE_COMM_VO_StartLayer fail for %#x!\n", s32Ret);
        goto END4_3;
    }	

    s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, VO_MODE_16MUX);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_4;
    }

    /************************************************
    step6:  VDEC bind VPSS
    *************************************************/	
    for(i=0; i<u32GrpCnt; i++)
    {
    	s32Ret = SAMPLE_COMM_VDEC_BindVpss(i, i);
    	if(s32Ret != HI_SUCCESS)
    	{	    
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            goto END5;
    	}	
    }
    	
    /************************************************
    step7:  VPSS bind VO
    *************************************************/
    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {	    
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
            goto END6;
        }	
    }	

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
	SAMPLE_COMM_VDEC_ThreadParam(u32VdCnt, &stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_H264_PATH);
	
    SAMPLE_COMM_VDEC_StartSendStream(u32VdCnt, &stVdecSend[0], &VdecThread[0]);

    /***  get the stat info of luma pix  ***/
    SAMPLE_COMM_VDEC_StartGetLuma(u32VdCnt, &stVdecSend[0], &VdecThread[0]);

    /***  set the rotational angle of decode pic  ***/
    printf("SAMPLE_TEST: set set the rotational angle of decode pic now.");
    sleep(5);
    for(i=0; i<u32VdCnt; i++)
    {   
        enRotate = ROTATE_90;
        HI_MPI_VDEC_SetRotate(i, enRotate);
    }
    sleep(3);
    for(i=0; i<u32VdCnt; i++)		
    {   
        enRotate = ROTATE_180;
        HI_MPI_VDEC_SetRotate(i, enRotate);
    }
    sleep(3);
    for(i=0; i<u32VdCnt; i++)		
    {   
        enRotate = ROTATE_270;
        HI_MPI_VDEC_SetRotate(i, enRotate);
    }
    sleep(3);	
    for(i=0; i<u32VdCnt; i++)		
    {   
        enRotate = ROTATE_NONE;
        HI_MPI_VDEC_SetRotate(i, enRotate);
    }

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
        //goto END2;
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
    for(i = 0; i < VO_MAX_CHN_NUM; i ++)
        HI_MPI_VO_DisableChn(nVoDev, i);
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
    stVoPubAttr.enIntfType = VO_INTF_HDMI;

    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        //goto END4_1;
    }
#ifndef HI_FPGA
     if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStop())
     {
        SAMPLE_PRT("Stop SAMPLE_COMM_VO_HdmiStart failed!\n");
     }
    if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
        //goto END4_2;
    }
#endif

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
    for(i=0; i<u32VdCnt; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
          //  goto END5;
        }	
    }
		
    /************************************************
    step7:  VPSS bind VO
    *************************************************/
    for(i=0; i<u32GrpCnt; i++)
    {
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
           // goto END6;
        }	
    }	

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    
    SAMPLE_COMM_VDEC_ThreadParam(u32VdCnt, &stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_H265_PATH);

	SAMPLE_PRT("sample init ok ---------------------------------------\n");

    // HI_MPI_VDEC_StopRecvStream(3);
    // HI_MPI_VDEC_EnableUserPic(3, HI_TRUE);

  //  SAMPLE_COMM_VDEC_StartSendStream(u32VdCnt, &stVdecSend[0], &VdecThread[0]);
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
    VB_CONF_S stVbConf, stModVbConf;
    HI_S32 i, s32ChnNum, s32Ret = HI_SUCCESS;
    VDEC_CHN_ATTR_S stVdecChnAttr[VDEC_MAX_CHN_NUM];
    VdecThreadParam stVdecSend[VDEC_MAX_CHN_NUM];
    VPSS_GRP_ATTR_S stVpssGrpAttr[VDEC_MAX_CHN_NUM];
    SIZE_S stSize;
    VO_DEV VoDev;
    VO_LAYER VoLayer;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;
    pthread_t	VdecThread[2*VDEC_MAX_CHN_NUM];
    VDEC_USERPIC_S stUserPic;
    VIDEO_FRAME_INFO_S stUsrPicInfo;

    s32ChnNum = 1;

    stSize.u32Width = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;
	
    /************************************************
    step1:  init SYS and common VB 
    *************************************************/
    SAMPLE_COMM_VDEC_Sysconf(&stVbConf, &stSize);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }
	
    /************************************************
    step2:  init mod common VB
    *************************************************/
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_MP4VIDEO, &stSize, s32ChnNum);	
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);
    if(s32Ret != HI_SUCCESS)
    {			
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step3:  start VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ChnAttr(s32ChnNum, &stVdecChnAttr[0], PT_MP4VIDEO, &stSize);
    s32Ret = SAMPLE_COMM_VDEC_Start(s32ChnNum, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END2;
    }

    /*** set the attr of user pic ***/
    stUserPic.u32PicWidth = 960;
    stUserPic.u32PicHeigth = 576;
    VDEC_PREPARE_USERPIC(&stUsrPicInfo,  "UserPic_960x576.yuv", &stUserPic);
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = HI_MPI_VDEC_SetUserPic(i, &stUsrPicInfo);
        if(s32Ret != HI_SUCCESS)
        {	
            SAMPLE_PRT("HI_MPI_VDEC_SetUserPic fail for %#x!\n", s32Ret);
            goto END2;
        }	
    }

    /************************************************
    step4:  start VPSS
    *************************************************/
    SAMPLE_COMM_VDEC_VpssGrpAttr(s32ChnNum, &stVpssGrpAttr[0], &stSize);
    s32Ret = SAMPLE_COMM_VPSS_Start(s32ChnNum, &stSize, 1, &stVpssGrpAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
        goto END3;
    }

    /************************************************
    step5:  start VO
    *************************************************/
    VoDev = SAMPLE_VO_DEV_DHD0;
    VoLayer = SAMPLE_VO_LAYER_VHD0;
    //SAMPLE_COMM_VDEC_VoAttr(s32ChnNum, VoDev ,&stVoPubAttr, &stVoLayerAttr);
#ifdef HI_FPGA
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;
    stVoPubAttr.enIntfType = VO_INTF_VGA;
#else
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;
    stVoPubAttr.enIntfType = VO_INTF_HDMI | VO_INTF_VGA;
#endif
    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_1;
    }
#ifndef HI_FPGA
    if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
        goto END4_2;
    }
#endif
    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync, \
        &stVoLayerAttr.stDispRect.u32Width, &stVoLayerAttr.stDispRect.u32Height, &stVoLayerAttr.u32DispFrmRt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto  END4_2;
    }
    stVoLayerAttr.stImageSize.u32Width = stVoLayerAttr.stDispRect.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVoLayerAttr.stDispRect.u32Height;
    stVoLayerAttr.bClusterMode = HI_FALSE;
    stVoLayerAttr.bDoubleFrame = HI_FALSE;
    stVoLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;    
    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stVoLayerAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_2;
    }	

    s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, VO_MODE_1MUX);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_3;
    }

    /************************************************
    step6:  VDEC bind VPSS
    *************************************************/
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            goto END5;
        }	
    }
		
    /************************************************
    step7:  VPSS bind VO
    *************************************************/
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
            goto END6;
        }	
    }	


    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ThreadParam(s32ChnNum, &stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_MPEG4_PATH);	
    SAMPLE_COMM_VDEC_StartSendStream(s32ChnNum, &stVdecSend[0], &VdecThread[0]);

    /***  get the stat info of luma pix  ***/
    SAMPLE_COMM_VDEC_StartGetLuma(s32ChnNum, &stVdecSend[0], &VdecThread[0]);
  
    /***  insert user pic  ***/
    sleep(5);	
    printf("SAMPLE_TEST: stop to receive stream and  instant to enable user pic now!\n");
    for(i=0; i<s32ChnNum; i++)
    {
        HI_MPI_VDEC_StopRecvStream(i);		
        HI_MPI_VDEC_EnableUserPic(i, HI_TRUE); 	
    }
	
    sleep(2);
    printf("SAMPLE_TEST: Disable user pic and  start to receive stream now!\n");
    for(i=0; i<s32ChnNum; i++)
    {
        HI_MPI_VDEC_DisableUserPic(i);		
        HI_MPI_VDEC_StartRecvStream(i);
    }

    sleep(5);
    printf("SAMPLE_TEST: stop to receive stream and  delay to enable user pic now!\n");	
    for(i=0; i<s32ChnNum; i++)
    {
        HI_MPI_VDEC_StopRecvStream(i);
        HI_MPI_VDEC_EnableUserPic(i, HI_FALSE); 			
    }
	
    sleep(2);
    printf("SAMPLE_TEST: Disable user pic and  start to receive stream now!\n");
    for(i=0; i<s32ChnNum; i++)
    {
        HI_MPI_VDEC_DisableUserPic(i);
        HI_MPI_VDEC_StartRecvStream(i);		
    }

    /***  control the send stream thread and get luma info thread  ***/
    SAMPLE_COMM_VDEC_CmdCtrl(s32ChnNum, &stVdecSend[0]);

    SAMPLE_COMM_VDEC_StopSendStream(s32ChnNum, &stVdecSend[0], &VdecThread[0]);

    SAMPLE_COMM_VDEC_StopGetLuma(s32ChnNum, &stVdecSend[0], &VdecThread[0]);

    /***release the vb of user pic ***/
    VDEC_RELEASE_USERPIC(&stUserPic);
		
END6:	
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VO_UnBindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vpss unbind vo fail for %#x!\n", s32Ret);
        }	
    }		

END5:
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_UnBindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vdec unbind vpss fail for %#x!\n", s32Ret);
        }	
    }

END4_3:
    SAMPLE_COMM_VO_StopChn(VoLayer, VO_MODE_1MUX);	
END4_2: 
    SAMPLE_COMM_VO_StopLayer(VoLayer);
END4_1: 
    SAMPLE_COMM_VO_StopDev(VoDev);
#ifndef HI_FPGA
    SAMPLE_COMM_VO_HdmiStop();
#endif

END3:
    SAMPLE_COMM_VPSS_Stop(s32ChnNum, VPSS_CHN0);

END2:
    SAMPLE_COMM_VDEC_Stop(s32ChnNum);		
	
END1:
    SAMPLE_COMM_SYS_Exit(); 

    return	s32Ret;
}

HI_S32 SAMPLE_VDEC_JpegDecoding(HI_VOID)
{	
    VB_CONF_S stVbConf, stModVbConf;
    HI_S32 i, s32ChnNum, s32Ret = HI_SUCCESS;
    VDEC_CHN_ATTR_S stVdecChnAttr[VDEC_MAX_CHN_NUM];
    VdecThreadParam stVdecSend[VDEC_MAX_CHN_NUM];
    VPSS_GRP_ATTR_S stVpssGrpAttr[VDEC_MAX_CHN_NUM];
    SIZE_S stSize;
    VO_DEV VoDev;
    VO_LAYER VoLayer;
    VO_PUB_ATTR_S stVoPubAttr;
    VO_VIDEO_LAYER_ATTR_S stVoLayerAttr;
    pthread_t	VdecThread[2*VDEC_MAX_CHN_NUM];

    s32ChnNum = 1;

    stSize.u32Width = HD_WIDTH;
    stSize.u32Height = HD_HEIGHT;

    /************************************************
    step1:  init SYS and common VB 
    *************************************************/
    SAMPLE_COMM_VDEC_Sysconf(&stVbConf, &stSize);
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if(s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("init sys fail for %#x!\n", s32Ret);
        goto END1;
    }
	
    /************************************************
    step2:  init mod common VB
    *************************************************/
    SAMPLE_COMM_VDEC_ModCommPoolConf(&stModVbConf, PT_JPEG, &stSize, s32ChnNum);	
    s32Ret = SAMPLE_COMM_VDEC_InitModCommVb(&stModVbConf);
    if(s32Ret != HI_SUCCESS)
    {			
        SAMPLE_PRT("init mod common vb fail for %#x!\n", s32Ret);
        goto END1;
    }

    /************************************************
    step3:  start VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ChnAttr(s32ChnNum, &stVdecChnAttr[0], PT_JPEG, &stSize);
    s32Ret = SAMPLE_COMM_VDEC_Start(s32ChnNum, &stVdecChnAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {	
        SAMPLE_PRT("start VDEC fail for %#x!\n", s32Ret);
        goto END2;
    }

    /************************************************
    step4:  start VPSS
    *************************************************/
    SAMPLE_COMM_VDEC_VpssGrpAttr(s32ChnNum, &stVpssGrpAttr[0], &stSize);
    s32Ret = SAMPLE_COMM_VPSS_Start(s32ChnNum, &stSize, 1, &stVpssGrpAttr[0]);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("start VPSS fail for %#x!\n", s32Ret);
        goto END3;
    }

    /************************************************
    step5:  start VO
    *************************************************/
    VoDev = SAMPLE_VO_DEV_DHD0;
    VoLayer = SAMPLE_VO_LAYER_VHD0;
    //SAMPLE_COMM_VDEC_VoAttr(s32ChnNum, VoDev ,&stVoPubAttr, &stVoLayerAttr);
#ifdef HI_FPGA
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;
    stVoPubAttr.enIntfType = VO_INTF_VGA;
#else
    stVoPubAttr.enIntfSync = VO_OUTPUT_1080P30;
    stVoPubAttr.enIntfType = VO_INTF_HDMI | VO_INTF_VGA;
#endif
    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stVoPubAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_1;
    }

#ifndef HI_FPGA
    if (HI_SUCCESS != SAMPLE_COMM_VO_HdmiStart(stVoPubAttr.enIntfSync))
    {
        SAMPLE_PRT("Start SAMPLE_COMM_VO_HdmiStart failed!\n");
        goto END4_2;
    }
#endif
    s32Ret = SAMPLE_COMM_VO_GetWH(stVoPubAttr.enIntfSync, \
        &stVoLayerAttr.stDispRect.u32Width, &stVoLayerAttr.stDispRect.u32Height, &stVoLayerAttr.u32DispFrmRt);
    if (s32Ret != HI_SUCCESS)
    {
        SAMPLE_PRT("failed with %#x!\n", s32Ret);
        goto  END4_2;
    }
    stVoLayerAttr.stImageSize.u32Width = stVoLayerAttr.stDispRect.u32Width;
    stVoLayerAttr.stImageSize.u32Height = stVoLayerAttr.stDispRect.u32Height;
    stVoLayerAttr.bClusterMode = HI_FALSE;
    stVoLayerAttr.bDoubleFrame = HI_FALSE;
    stVoLayerAttr.enPixFormat = SAMPLE_PIXEL_FORMAT;    
    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stVoLayerAttr);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_2;
    }	

    s32Ret = SAMPLE_COMM_VO_StartChn(VoLayer, VO_MODE_1MUX);
    if(s32Ret != HI_SUCCESS)
    {		
        SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
        goto END4_3;
    }


    /************************************************
    step6:  VDEC bind VPSS
    *************************************************/
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_BindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vdec bind vpss fail for %#x!\n", s32Ret);
            goto END5;
        }	
    }
	
	
    /************************************************
    step7:  VPSS bind VO
    *************************************************/
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VO_BindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vpss bind vo fail for %#x!\n", s32Ret);
            goto END6;
        }	
    }	

    /************************************************
    step8:  send stream to VDEC
    *************************************************/
    SAMPLE_COMM_VDEC_ThreadParam(s32ChnNum, &stVdecSend[0], &stVdecChnAttr[0], SAMPLE_1080P_JPEG_PATH);	
    SAMPLE_COMM_VDEC_StartSendStream(s32ChnNum, &stVdecSend[0], &VdecThread[0]);

    /***  get the stat info of luma pix  ***/
    SAMPLE_COMM_VDEC_StartGetLuma(s32ChnNum, &stVdecSend[0], &VdecThread[0]);

    /***  control the send stream thread and get luma info thread  ***/
    SAMPLE_COMM_VDEC_CmdCtrl(s32ChnNum, &stVdecSend[0]);

    SAMPLE_COMM_VDEC_StopSendStream(s32ChnNum, &stVdecSend[0], &VdecThread[0]);

    SAMPLE_COMM_VDEC_StopGetLuma(s32ChnNum, &stVdecSend[0], &VdecThread[0]);

		
END6:
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VO_UnBindVpss(VoLayer, i, i, VPSS_CHN0);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vpss unbind vo fail for %#x!\n", s32Ret);
        }	
    }		

END5:
    for(i=0; i<s32ChnNum; i++)
    {
        s32Ret = SAMPLE_COMM_VDEC_UnBindVpss(i, i);
        if(s32Ret != HI_SUCCESS)
        {		
            SAMPLE_PRT("vdec unbind vpss fail for %#x!\n", s32Ret);
        }	
    }

END4_3:
    SAMPLE_COMM_VO_StopChn(VoLayer, VO_MODE_1MUX);	
END4_2: 
    SAMPLE_COMM_VO_StopLayer(VoLayer);
END4_1: 
    SAMPLE_COMM_VO_StopDev(VoDev);
#ifndef HI_FPGA
    SAMPLE_COMM_VO_HdmiStop();
#endif

END3:
    SAMPLE_COMM_VPSS_Stop(s32ChnNum, VPSS_CHN0);

END2:
    SAMPLE_COMM_VDEC_Stop(s32ChnNum);		
	
END1:
    SAMPLE_COMM_SYS_Exit(); 

    return	s32Ret;
}

int vdec_init(HI_U32 chnCnt, SAMPLE_VO_MODE_E enMode)
{
    HI_S32 s32Ret = HI_SUCCESS;
    
    signal(SIGINT, SAMPLE_VDEC_HandleSig);
    signal(SIGTERM, SAMPLE_VDEC_HandleSig);
     
    SAMPLE_VDEC_VdhH265(chnCnt,enMode);
    	
 
    return s32Ret;
}

static int vdec_main(int argc, char *argv[])
{
    HI_S32 s32Ret = HI_SUCCESS;
    HI_CHAR ch;
    HI_BOOL bExit = HI_FALSE;
    
    signal(SIGINT, SAMPLE_VDEC_HandleSig);
    signal(SIGTERM, SAMPLE_VDEC_HandleSig);

    /******************************************
    1 choose the case
    ******************************************/
    while (1)
    {
        SAMPLE_VDEC_Usage();
        ch = getchar();
        if (10 == ch)
        {
            continue;
        }
        getchar();
        switch (ch)
        {
            case '0':
            {
    	        SAMPLE_VDEC_VdhH264();
    	        break;
            }
            case '1':
            {
    	      //  SAMPLE_VDEC_VdhH265(4);
    	        break;
            }    
            case '2':
            {
    	        SAMPLE_VDEC_VdhMpeg4();
    	        break;
            }
            case '3':
            {
    	        SAMPLE_VDEC_JpegDecoding();
    	        break;
    	    }
            case 'q':
            case 'Q':
            {
    	        bExit = HI_TRUE;
    	        break;
    	    }

    	    default :
    	    {
    	        printf("input invaild! please try again.\n");
    	        break;
    	    }
        }
        
        if (bExit)
        {
            break;
        }
    }

    return s32Ret;
}



