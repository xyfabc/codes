#include "framebuffer.h"
#include <fcntl.h>
#include "sample_comm.h"
#include "hisi_vdec.h"
#include <linux/fb.h>
#include "hifb.h"
#include "loadbmp.h"
#include "hi_tde_api.h"
#include "hi_tde_type.h"
#include "hi_tde_errcode.h"
#include "logo.h"


#define ZMJ_CURSOR_PATH		"./res/cursor.bmp"

//24 pix
static struct fb_bitfield s_a32 = {24, 8, 0};
static struct fb_bitfield s_r32 = {16, 8, 0};
static struct fb_bitfield s_g32 = {8,  8, 0};
static struct fb_bitfield s_b32 = {0,  8, 0};


//16 pix
static struct fb_bitfield s_r16 = {10, 5, 0};
static struct fb_bitfield s_g16 = {5, 5, 0};
static struct fb_bitfield s_b16 = {0, 5, 0};
static struct fb_bitfield s_a16 = {15, 1, 0};

unsigned char *fb_ptr;
int screensize = 0;
int m_fd;
struct fb_var_screeninfo vinfo;

framebuffer::framebuffer()
{

}

void drawGreen(HI_S32 m_fd,int screensize,HI_U8 *fb_ptr)
{
    int i = 0;
    int ret = -1;
    unsigned int * dst = (unsigned int*)fb_ptr;
    struct fb_var_screeninfo vinfo;
    int len = screensize/2;
    for(; i < len; i++)
    {
        *dst++ = 0xff00ff00;
    }
    if (ret = ioctl(m_fd, FBIOPAN_DISPLAY, &vinfo) < 0)
    {
        printf("FBIOPAN_DISPLAY failed! %d\n",ret);
        close(m_fd);
    }
    printf("drawGreen\n");
}
void drawRed(HI_S32 m_fd,int screensize,HI_U8 *fb_ptr)
{
    int i = 0;
    unsigned int * dst = (unsigned int*)fb_ptr;
    struct fb_var_screeninfo vinfo;
    int len = screensize/2;
    for(; i < len; i++)
    {
        *dst++ = 0xffFF0000;
    }
    if (ioctl(m_fd, FBIOPAN_DISPLAY, &vinfo) < 0)
    {
        printf("FBIOPAN_DISPLAY failed!\n");
        close(m_fd);
    }
    printf("drawRed\n");
}
void drawBlue(HI_S32 m_fd,int screensize,HI_U8 *fb_ptr)
{
    int i = 0;
    unsigned int * dst = (unsigned int*)fb_ptr;
    struct fb_var_screeninfo vinfo;
    int len = screensize/2;
    for(; i < len; i++)
    {
        *dst++ = 0xff0000ff;
    }
    if (ioctl(m_fd, FBIOPAN_DISPLAY, &vinfo) < 0)
    {
        printf("FBIOPAN_DISPLAY failed!\n");
        close(m_fd);
    }
    printf("drawBlue\n");
}



HI_S32 SAMPLE_HIFB_LoadBmp(const char *filename, HI_U8 *pAddr)
{
    OSD_SURFACE_S Surface;
    OSD_BITMAPFILEHEADER bmpFileHeader;
    OSD_BITMAPINFO bmpInfo;

    if(GetBmpInfo(filename,&bmpFileHeader,&bmpInfo) < 0)
    {
        SAMPLE_PRT("GetBmpInfo err!\n");
        return HI_FAILURE;
    }

    Surface.enColorFmt = OSD_COLOR_FMT_RGB1555;

    CreateSurfaceByBitMap(filename,&Surface,pAddr);

    return HI_SUCCESS;
}

static int mounse_fd = 0;
int HIFB_G3_init()
{
    HI_S32 i,x,y,s32Ret;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    HI_U32 u32FixScreenStride = 0;
    HI_U8 *pShowScreen;
    HI_U8 *pHideScreen;
    HI_U32 u32HideScreenPhy = 0;
    HI_U16 *pShowLine;
    HI_U16 *ptemp = NULL;
    HIFB_ALPHA_S stAlpha;
    HIFB_POINT_S stPoint = {40, 112};
    HI_CHAR file[12] = "/dev/fb3";

    HI_CHAR image_name[128];
    HI_U8 *pDst = NULL;
    HI_BOOL bShow;
    HIFB_COLORKEY_S stColorKey;
    HI_U32 Phyaddr;
    HI_VOID *Viraddr;

    PTHREAD_HIFB_SAMPLE_INFO stInfo;
    stInfo.layer   =  3;
    stInfo.fd      = -1;
    stInfo.ctrlkey =  3;




    /* 1. open framebuffer device overlay 0 */
    stInfo.fd = open(file, O_RDWR, 0);
    if(stInfo.fd < 0)
    {
        SAMPLE_PRT("open %s failed!\n",file);
        return HI_NULL;
    }
    mounse_fd = stInfo.fd;
    SAMPLE_PRT("-----open %s ok------!\n",file);
    bShow = HI_FALSE;
    if (ioctl(stInfo.fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        SAMPLE_PRT("FBIOPUT_SHOW_HIFB failed!\n");
        return HI_NULL;
    }
    /* 2. set the screen original position */
    switch(stInfo.ctrlkey)
    {
    case GRAPHICS_LAYER_HC0:
    {
        stPoint.s32XPos = 150;
        stPoint.s32YPos = 150;
    }
        break;
    default:
    {
        stPoint.s32XPos = 0;
        stPoint.s32YPos = 0;
    }
    }

    if (ioctl(stInfo.fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        SAMPLE_PRT("set screen original show position failed!\n");
        close(stInfo.fd);
        return HI_NULL;
    }

    /* 3. get the variable screen info */
    if (ioctl(stInfo.fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
        SAMPLE_PRT("Get variable screen info failed!\n");
        close(stInfo.fd);
        return HI_NULL;
    }

    /* 4. modify the variable screen info
          the screen size: IMAGE_WIDTH*IMAGE_HEIGHT
          the virtual screen size: VIR_SCREEN_WIDTH*VIR_SCREEN_HEIGHT
          (which equals to VIR_SCREEN_WIDTH*(IMAGE_HEIGHT*2))
          the pixel format: ARGB1555
    */
    //usleep(4*1000*1000);

    var.xres_virtual = 32;
    var.yres_virtual = 32;
    var.xres = 32;
    var.yres = 32;

    var.transp= s_a16;
    var.red = s_r16;
    var.green = s_g16;
    var.blue = s_b16;
    var.bits_per_pixel = 16;
    var.activate = FB_ACTIVATE_NOW;

    /* 5. set the variable screeninfo */
    if (ioctl(stInfo.fd, FBIOPUT_VSCREENINFO, &var) < 0)
    {
        SAMPLE_PRT("Put variable screen info failed!\n");
        close(stInfo.fd);
        return HI_NULL;
    }

    /* 6. get the fix screen info */
    if (ioctl(stInfo.fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
        SAMPLE_PRT("Get fix screen info failed!\n");
        close(stInfo.fd);
        return HI_NULL;
    }
    u32FixScreenStride = fix.line_length;   /*fix screen stride*/

    /* 7. map the physical video memory for user use */
    pShowScreen = (HI_U8 *)mmap(HI_NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, stInfo.fd, 0);
    if(MAP_FAILED == pShowScreen)
    {
        SAMPLE_PRT("mmap framebuffer failed!\n");
        close(stInfo.fd);
        return HI_NULL;
    }

    memset(pShowScreen, 0x00, fix.smem_len);

    /* time to play*/
    bShow = HI_TRUE;
    if (ioctl(stInfo.fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        SAMPLE_PRT("FBIOPUT_SHOW_HIFB failed!\n");
        munmap(pShowScreen, fix.smem_len);
        close(stInfo.fd);
        return HI_NULL;
    }
    SAMPLE_PRT("expected: the red box will erased by colorkey!\n");
    stColorKey.bKeyEnable = HI_TRUE;
    stColorKey.u32Key = 0x0000;
    s32Ret = ioctl(stInfo.fd, FBIOPUT_COLORKEY_HIFB, &stColorKey);
    if (s32Ret < 0)
    {
        SAMPLE_PRT("FBIOPUT_COLORKEY_HIFB failed!\n");
        close(stInfo.fd);
        return HI_NULL;
    }

    /* move cursor */
    SAMPLE_HIFB_LoadBmp(ZMJ_CURSOR_PATH,pShowScreen);
    if (ioctl(stInfo.fd, FBIOPAN_DISPLAY, &var) < 0)
    {
        SAMPLE_PRT("FBIOPAN_DISPLAY failed!\n");
        munmap(pShowScreen, fix.smem_len);
        close(stInfo.fd);
        return HI_NULL;
    }


    /* unmap the physical memory */
    //    munmap(pShowScreen, fix.smem_len);
    //    bShow = HI_FALSE;
    //    if (ioctl(stInfo.fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    //    {
    //        SAMPLE_PRT("FBIOPUT_SHOW_HIFB failed!\n");
    //        close(stInfo.fd);
    //        return HI_NULL;
    //    }
    //    close(stInfo.fd);
    return HI_NULL;
}

#define WIDTH                  1920
#define HEIGHT                 1080
#define SAMPLE_IMAGE_WIDTH     1920
#define SAMPLE_IMAGE_HEIGHT    1080

#define SAMPLE_IMAGE_NUM       20
#define HIFB_RED_1555   0xFC00
#define HIFB_BLACK_1555   0x0000

#define GRAPHICS_LAYER_HC0 3
#define GRAPHICS_LAYER_G0  0
#define GRAPHICS_LAYER_G1  1


#define SAMPLE_IMAGE1_PATH		"./res/%d.bmp"
#define SAMPLE_IMAGE2_PATH		"./res/1280_720.bits"
#define SAMPLE_CURSOR_PATH		"./res/cursor.bmp"
#define ZMJ_LOGO_PATH           "./res/zmj_logo.bmp"

HI_VOID SAMPLE_HIFB_PANDISPLAY(void *pData)
{
    HI_S32 i,x,y,s32Ret;
    struct fb_fix_screeninfo fix;
    struct fb_var_screeninfo var;
    HI_U32 u32FixScreenStride = 0;
    HI_U8 *pShowScreen;
    HIFB_ALPHA_S stAlpha;
    HIFB_POINT_S stPoint = {40, 112};
    HI_CHAR file[12] = "/dev/fb0";
    HI_BOOL bShow;
    PTHREAD_HIFB_SAMPLE_INFO *pstInfo;


    if(HI_NULL == pData)
    {
        return ;
    }
    pstInfo = (PTHREAD_HIFB_SAMPLE_INFO *)pData;

    printf("--------stInfo.layer-------%d",pstInfo->layer);

    strcpy(file, "/dev/fb0");
    /* 1. open framebuffer device overlay 0 */
    pstInfo->fd = open(file, O_RDWR, 0);
    if(pstInfo->fd < 0)
    {
        SAMPLE_PRT("open %s failed!\n",file);
        return ;
    }

    bShow = HI_FALSE;
    if (ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &bShow) < 0)
    {
        SAMPLE_PRT("FBIOPUT_SHOW_HIFB failed!\n");
        return ;
    }
    /* 2. set the screen original position */
    switch(pstInfo->ctrlkey)
    {
    case GRAPHICS_LAYER_HC0:
    {
        stPoint.s32XPos = 150;
        stPoint.s32YPos = 150;
    }
        break;
    default:
    {
        stPoint.s32XPos = 0;
        stPoint.s32YPos = 0;
    }
    }

    if (ioctl(pstInfo->fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        SAMPLE_PRT("set screen original show position failed!\n");
        close(pstInfo->fd);
        return ;
    }

    /* 3. get the variable screen info */
    if (ioctl(pstInfo->fd, FBIOGET_VSCREENINFO, &var) < 0)
    {
        SAMPLE_PRT("Get variable screen info failed!\n");
        close(pstInfo->fd);
        return ;
    }

    /* 4. modify the variable screen info
          the screen size: IMAGE_WIDTH*IMAGE_HEIGHT
          the virtual screen size: VIR_SCREEN_WIDTH*VIR_SCREEN_HEIGHT
          (which equals to VIR_SCREEN_WIDTH*(IMAGE_HEIGHT*2))
          the pixel format: ARGB1555
    */
   // usleep(4*1000*1000);

    s32Ret = ioctl (pstInfo->fd, FBIOGET_VSCREENINFO, &var);
    if (s32Ret < 0)
    {
        SAMPLE_PRT("FBIOGET_VSCREENINFO failed!\n");
        close(pstInfo->fd);
        return ;
    }

#if 1
    var.red = s_r32;
    var.green = s_g32;
    var.blue = s_b32;
    var.transp = s_a32;
    var.bits_per_pixel = 32;
    var.activate=FB_ACTIVATE_NOW;
#else
    var.transp= s_a16;
    var.red = s_r16;
    var.green = s_g16;
    var.blue = s_b16;
    var.bits_per_pixel = 16;
    var.activate = FB_ACTIVATE_NOW;
#endif
    var.xres=WIDTH;
    var.yres=HEIGHT;
    var.xres_virtual=WIDTH;
    var.yres_virtual = HEIGHT*2;
    var.xoffset=0;
    var.yoffset=0;

    /* 5. set the variable screeninfo */
    if (ioctl(pstInfo->fd, FBIOPUT_VSCREENINFO, &var) < 0)
    {
        SAMPLE_PRT("Put variable screen info failed!\n");
        close(pstInfo->fd);
        return ;
    }

    /* 6. get the fix screen info */
    if (ioctl(pstInfo->fd, FBIOGET_FSCREENINFO, &fix) < 0)
    {
        SAMPLE_PRT("Get fix screen info failed!\n");
        close(pstInfo->fd);
        return ;
    }
    u32FixScreenStride = fix.line_length;   /*fix screen stride*/
    int screensize = var.xres * var.yres * var.bits_per_pixel / 8;
    printf("----screensize--%d---fix.smem_len-%d--var.xres%d var.yres%d-var.bits_per_pixel%d\n",screensize,fix.smem_len,var.xres,var.yres,var.bits_per_pixel);

    /* 7. map the physical video memory for user use */
    pShowScreen = (HI_U8 *)mmap(NULL, fix.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, pstInfo->fd, 0);
    //fb_ptr = (unsigned char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd, 0);
    if(MAP_FAILED == pShowScreen)
    {
        SAMPLE_PRT("mmap framebuffer failed!\n");
        close(pstInfo->fd);
        return ;
    }
    memset(pShowScreen, 0x00, fix.smem_len);

    /* time to play*/
    bShow = HI_FALSE;
    if(ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &bShow) < 0){
        SAMPLE_PRT("FBIOPUT_SHOW_HIFB failed!\n");
        munmap(pShowScreen, fix.smem_len);
        close(pstInfo->fd);
        return ;
    }

    if(ioctl(pstInfo->fd, FBIOGET_ALPHA_HIFB,	&stAlpha)){
        SAMPLE_PRT("Get alpha failed!\n");
        close(pstInfo->fd);
        return ;
    }

    stAlpha.bAlphaEnable = HI_TRUE;
    stAlpha.bAlphaChannel = HI_FALSE;
    stAlpha.u8Alpha0 = 0xff;
    stAlpha.u8Alpha1 = 0xff;
    stAlpha.u8GlobalAlpha = 0xff;
    if (ioctl(pstInfo->fd, FBIOPUT_ALPHA_HIFB,  &stAlpha) < 0){
        SAMPLE_PRT("Set alpha failed!\n");
        close(pstInfo->fd);
        return ;
    }

    bShow = HI_TRUE;
    if(ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &bShow) < 0){
        SAMPLE_PRT("FBIOPUT_SHOW_HIFB failed!\n");
        munmap(pShowScreen, fix.smem_len);
        close(pstInfo->fd);
        return ;
    }

//    while(1)
//    {
//        printf("show red green blue\n");
//        drawRed(pstInfo->fd,screensize,pShowScreen);
//        usleep(1*1000*1000);
//        drawGreen(pstInfo->fd,screensize,pShowScreen);
//        usleep(1*1000*1000);
//        drawBlue(pstInfo->fd,screensize,pShowScreen);
//        usleep(1*1000*1000);
//    }


    ShowBmp(ZMJ_LOGO_PATH,pstInfo->fd, &var,(char *)pShowScreen);

    bShow = HI_TRUE;
    if(ioctl(pstInfo->fd, FBIOPUT_SHOW_HIFB, &bShow) < 0){
        SAMPLE_PRT("FBIOPUT_SHOW_HIFB failed!\n");
        munmap(pShowScreen, fix.smem_len);
        close(pstInfo->fd);
        return ;
    }

    close(pstInfo->fd);
    return ;
}

HI_S32 SetMousePos(int x,int y)
{
    HIFB_POINT_S stPoint = {x, y};
    if(ioctl(mounse_fd, FBIOPUT_SCREEN_ORIGIN_HIFB, &stPoint) < 0)
    {
        SAMPLE_PRT("set screen original show position failed!\n");
    }
}


HI_S32 hifb_init()
{
    HI_S32 s32Ret = HI_SUCCESS;
    pthread_t phifb0 = -1;
    pthread_t phifb1 = -1;
    int i = 0;

    PTHREAD_HIFB_SAMPLE_INFO stInfo0;
    PTHREAD_HIFB_SAMPLE_INFO stInfo1;
    HI_U32 u32PicWidth         = WIDTH;
    HI_U32 u32PicHeight        = HEIGHT;
    SIZE_S  stSize;

    VO_LAYER VoLayer = 0;
    VO_DEV VoDev = SAMPLE_VO_DEV_DHD0;
    //VO_DEV VoDev = SAMPLE_VO_DEV_DHD1;
    VO_PUB_ATTR_S stPubAttr;
    VO_VIDEO_LAYER_ATTR_S stLayerAttr;
    HI_U32 u32VoFrmRate;

    VB_CONF_S       stVbConf;
    HI_U32 u32BlkSize;

    //sys exit
    s32Ret = HI_MPI_SYS_Exit();
    if (s32Ret != 0)
    {
        printf("[SDK] HI_MPI_SYS_Exit Fail = 0x%x\n", s32Ret);
    }
    s32Ret = HI_MPI_VB_Exit();
    if (s32Ret != 0)
    {
        printf("[SDK] HI_MPI_VB_Exit Fail = 0x%x\n", s32Ret);
    }
    for(i = 0; i < VO_MAX_CHN_NUM; i ++){
        HI_MPI_VO_DisableChn(VoLayer, i);
    }
    HI_MPI_VO_DisableVideoLayer(VoLayer);
    HI_MPI_VO_Disable(VoLayer);


    /******************************************
     step  1: init variable
    ******************************************/
    memset(&stVbConf,0,sizeof(VB_CONF_S));

    u32BlkSize = CEILING_2_POWER(u32PicWidth,SAMPLE_SYS_ALIGN_WIDTH)\
            * CEILING_2_POWER(u32PicHeight,SAMPLE_SYS_ALIGN_WIDTH) *2;

    stVbConf.u32MaxPoolCnt = 128;

    stVbConf.astCommPool[0].u32BlkSize = u32BlkSize;
    stVbConf.astCommPool[0].u32BlkCnt =  6;

    /******************************************
     step 2: mpp system init.
    ******************************************/
    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("system init failed with %d!\n", s32Ret);
        // goto SAMPLE_HIFB_NoneBufMode_0;
    }

    /******************************************
     step 3:  start vo hd0.
    *****************************************/

    s32Ret = HI_MPI_VO_UnBindGraphicLayer(GRAPHICS_LAYER_HC0, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("UnBindGraphicLayer failed with %d!\n", s32Ret);
        //goto SAMPLE_HIFB_NoneBufMode_0;
    }

    s32Ret = HI_MPI_VO_BindGraphicLayer(GRAPHICS_LAYER_HC0, VoDev);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("BindGraphicLayer failed with %d!\n", s32Ret);
        // goto SAMPLE_HIFB_NoneBufMode_0;
    }
    stPubAttr.enIntfSync = VO_OUTPUT_1080P60;
    stPubAttr.enIntfType = VO_INTF_HDMI;


    //stPubAttr.u32BgColor = 0x00FF00;  //green
    stPubAttr.u32BgColor = 0x0000FF;  //blue

    stLayerAttr.bClusterMode = HI_FALSE;
    stLayerAttr.bDoubleFrame = HI_FALSE;
    stLayerAttr.enPixFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;

    s32Ret = SAMPLE_COMM_VO_GetWH(stPubAttr.enIntfSync,&stSize.u32Width,\
                                  &stSize.u32Height,&u32VoFrmRate);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("get vo wh failed with %d!\n", s32Ret);
        //goto SAMPLE_HIFB_NoneBufMode_0;
    }
    memcpy(&stLayerAttr.stImageSize,&stSize,sizeof(stSize));

    stLayerAttr.u32DispFrmRt = 30 ;
    stLayerAttr.stDispRect.s32X = 0;
    stLayerAttr.stDispRect.s32Y = 0;
    stLayerAttr.stDispRect.u32Width = stSize.u32Width;
    stLayerAttr.stDispRect.u32Height = stSize.u32Height;

    s32Ret = SAMPLE_COMM_VO_StartDev(VoDev, &stPubAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo dev failed with %d!\n", s32Ret);
        // goto SAMPLE_HIFB_NoneBufMode_0;
    }

    s32Ret = SAMPLE_COMM_VO_StartLayer(VoLayer, &stLayerAttr);
    if (HI_SUCCESS != s32Ret)
    {
        SAMPLE_PRT("start vo layer failed with %d!\n", s32Ret);
        // goto SAMPLE_HIFB_NoneBufMode_1;
    }

    if (stPubAttr.enIntfType & VO_INTF_HDMI)
    {
        s32Ret = SAMPLE_COMM_VO_HdmiStart(stPubAttr.enIntfSync);
        if (HI_SUCCESS != s32Ret)
        {
            SAMPLE_PRT("start HDMI failed with %d!\n", s32Ret);
            // goto SAMPLE_HIFB_NoneBufMode_2;
        }
    }

    /******************************************
     step 4:  start hifb.
    *****************************************/
    stInfo0.layer   =  0;
    stInfo0.fd      = -1;
    stInfo0.ctrlkey =  2;
    SAMPLE_HIFB_PANDISPLAY((void *)(&stInfo0));
    HIFB_G3_init();

    return s32Ret;
}
