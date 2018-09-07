#include "HWCameraCmd.h"
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned char *fb_ptr = 0;
unsigned char *screenbuf = 0;
struct fb_var_screeninfo vinfo;
struct fb_fix_screeninfo fbfix;

int m_fd,m_fd_fb;    
struct v4l2_format m_format;
VideoBuffer *m_videoBuffer = 0;
unsigned char *m_buffer = 0;
unsigned char *camerabuf = 0;
unsigned int *m_rgb16torgb32table = 0;
int screensize = 0;

int FbdevInit()
{
    m_fd_fb = open("/dev/fb0", O_RDWR);
    if(m_fd_fb<0)
    {
        printf("%s --- %d\n",__func__,__LINE__);
        close(m_fd_fb);
        exit(1);
    }
    if(ioctl(m_fd_fb, FBIOGET_VSCREENINFO, &vinfo)<0)
    {
        printf("%s --- %d\n",__func__,__LINE__);
        close(m_fd_fb);
        exit(1);
    }

    if(ioctl(m_fd_fb, FBIOGET_FSCREENINFO, &fbfix)<0)
    {
        printf("%s --- %d\n",__func__,__LINE__);
        close(m_fd_fb);
        exit(1);
    }

    int screensize = vinfo.xres * vinfo.yres * vinfo.bits_per_pixel / 8;

    camerabuf = (unsigned char*)malloc(320*240*4);
    if(camerabuf==0)
        exit(1);

    screenbuf = camerabuf;
    
    /*mmap LCD framebuffer to user space*/
    fb_ptr = (unsigned char *)mmap(NULL, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, m_fd_fb, 0);
    if(fb_ptr==0)
    {
        printf("%s --- %d\n",__func__,__LINE__);
        exit(1);//return false;
    }
    
    return 0;
}

int FbdevFree()
{
    if(m_fd_fb>0)
        close(m_fd_fb);
    
    if(fb_ptr>0)
        munmap(fb_ptr,screensize);

    printf("%s --- %d\n",__func__,__LINE__);        
    return 0;
}

#define ROTATE90 1
#define SCREEN_HEIGHT 320
#define SCREEN_WIDTH  240
void FbdevUpdate()
{
#ifdef ROTATE90
//    _DEBUG
    if(screenbuf==fb_ptr)
    {
        ioctl(m_fd_fb,0x468f);
        return;
    }
    int i = 0,j = 0;
    for(i = 0; i<320;i++)
    {
        unsigned int *dstptr = (unsigned int *)fb_ptr+ (SCREEN_HEIGHT* (SCREEN_WIDTH-1)+i);
        unsigned int *srcptr = (unsigned int *)screenbuf+i*SCREEN_WIDTH;
        //if(m_status == STOPPED_STATUS)
        //    return;
        for(j = 0; j< 240; j=j+4)
        {
            dstptr[-j*SCREEN_HEIGHT] = srcptr[j];
            dstptr[-(j+1)*SCREEN_HEIGHT] = srcptr[j+1];
            dstptr[-(j+2)*SCREEN_HEIGHT] = srcptr[j+2];
            dstptr[-(j+3)*SCREEN_HEIGHT] = srcptr[j+3];
        }
    }
    ioctl(m_fd_fb,0x468f);
    return;
#else
    if(screenbuf!=fb_ptr)
        memcpy(fb_ptr,screenbuf,320*240*4);
    ioctl(m_fd_fb,0x468f);
    return;
#endif
}

#define _R_16(x) (((x&0x00F8)<<16))
#define _B_16(x) (((x&0x1F00)>>5))
#define _G_16(x) ((((x&0x0007)<<13)|((x&0xE000)>>3)))
#define _RGB565to888(x) ( (_R_16(x))|(_G_16(x))|(_B_16(x)) )
int InitRGB24Table()
{
    int i;
    for(i = 0 ; i<65536;i++)
        m_rgb16torgb32table[i] = _RGB565to888(i);

    return 0;
}

int HWCameraRGBInit()
{
    int i;
    m_fd = open ("/dev/video21",O_RDONLY);
    if(m_fd<=0)
    {
        usleep(200*1000);
        goto APP_EXIT;
    }
    memset (&m_format,0,sizeof(m_format));
    usleep(30000);
    m_format.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(m_fd,VIDIOC_G_FMT,&m_format)<0)
    {
        usleep(200*1000);
        goto APP_EXIT;
    }
    m_videoBuffer = (VideoBuffer *)malloc (MAX_RGB_BUF_NUM*sizeof(VideoBuffer));
    if (!m_videoBuffer)
    {
        usleep(200*1000);
        goto APP_EXIT;
    }
    for (i=0;i<MAX_RGB_BUF_NUM;++i) // for (nbuffer=0;nbuffer<req.count;++nbuffer)
    {
        int length = 752*480;
        m_videoBuffer[i].length = length;
        m_videoBuffer[i].start = mmap (NULL, length,PROT_READ, MAP_SHARED, m_fd,0);
        usleep(30000);
        if (MAP_FAILED == m_videoBuffer[i].start)
        {
	    printf("mmap fail \n");
            usleep(200*1000);
            goto APP_EXIT;
        }
    }
    if(m_rgb16torgb32table==NULL)
        m_rgb16torgb32table = (unsigned int*)malloc(65536*4);
    if(m_rgb16torgb32table==NULL)
    {
        usleep(200*1000);
        goto APP_EXIT;
    }
    m_buffer = (unsigned char*)malloc(320*240*sizeof(char));
    if(m_buffer==NULL)
    {
        sleep(1);
        goto APP_EXIT;
    }
    memset(m_buffer,0x00,320*240*sizeof(char));
    InitRGB24Table();
    return 0;
APP_EXIT:
    if(m_fd>0)
        close(m_fd);
    if(m_videoBuffer)
    {
        for(i = 0 ; i<MAX_RGB_BUF_NUM;i++)
        {
            if(m_videoBuffer[i].start && m_videoBuffer[i].length)
                munmap(m_videoBuffer[i].start,m_videoBuffer[i].length);
        }
        free(m_videoBuffer);
    }
    if(m_rgb16torgb32table)
        free(m_rgb16torgb32table);
    if(m_buffer)
        free(m_buffer);
    return -1;
}


HWCameraRGBFree(void)
{
    int i;
    if(m_fd>0)
        close(m_fd);
    if(m_videoBuffer)
    {
        for(i = 0 ; i<MAX_RGB_BUF_NUM;i++)
        {
            if(m_videoBuffer[i].start && m_videoBuffer[i].length)
                munmap(m_videoBuffer[i].start,m_videoBuffer[i].length);
        }
        free(m_videoBuffer);
    }
    if(m_rgb16torgb32table)
        free(m_rgb16torgb32table);
    if(m_buffer)
        free(m_buffer);
}


#define PACKRGB32(r,g,b) ((r<<16)|(g<<8)|b)
static void ConvertRaw2RGB32(unsigned char* raw, int w, int h, unsigned int* rgb32)
{    
    int i,j;
    unsigned char r,g,b;
    return ;
    for(i = 2 ; i < h; i+=2)
    {
        for(j = 2; j< w; j+=2)
        {
            r = (raw[(i-1)*w+j]+raw[(i+1)*w+j])>>1;
            g = raw[i*w+j];
            b = (raw[i*w+j-1] + raw[i*w+j+1])>>1;
            rgb32[(i>>1)+(j>>1)*h] = PACKRGB32(r,g,b);
        }
    }
}
static inline void ConvertYUV2RGB32(unsigned char y, unsigned char uu, unsigned char vv,unsigned int * rgb32)
{
    int r,g,b;
    int u1,v1;
    int rdif,invgdif,bdif;
    
/*    if(y>235)
        y = 235;
    else if(y<5)
        y = 5;

    if(uu>200)
        uu = 200;
    else if(uu<20)
        uu = 20;

    if(vv>200)
        vv = 200;
    else if(vv<20)
        vv = 20;*/

    
    u1 = uu-128;
    v1 = vv-128;
    rdif = (359*v1)>>8;
    invgdif = (179*v1+86*u1)>>8;
    bdif = (454*u1)>>8;
    r = y+rdif;//+20;
    g = y-invgdif;
    b = y+bdif;

    //rdif = v1+((v1*104)>>8);
    //invgdif = ((u1*88)>>8)+((v1*183)>>8);
    //bdif = u1 + ((u1*199)>>8);
    //r = y+rdif;
    //g = y-invgdif;
    //b = y+bdif;
    if(r>255)
        r = 255;
    else if(r<0)
        r = 0;

    if(g>255)
        g = 255;
    else if(g<0)
        g = 0;

    if(b>255)
        b = 255;
    else if(b<0)
        b = 0;
    *rgb32 = PACKRGB32(r,g,b);
}

#define RGB_CAMERA_MODE_YUV
unsigned int GetDataForDisplay(unsigned char *framebuffer)
{
    unsigned int ret = 1;

    unsigned char frameIndex= 0 ;
    unsigned short *src = NULL;
    unsigned int *dst = NULL;
    unsigned short tmp1,tmp2;
    unsigned int tmpvalue1;
    unsigned char r,g,b;
    unsigned char *cdst,*csrc;
    int i = 0,j = 0;

    if(framebuffer == NULL)
    {
        return 0;
    }

    if(ioctl(m_fd, VIDIOC_DQBUF, &frameIndex) < 0 || frameIndex >= MAX_RGB_BUF_NUM)
    {
        usleep(200*1000);
        return 0;
    }
    src = (unsigned short*)m_videoBuffer[frameIndex].start;
    csrc = (unsigned char *)m_videoBuffer[frameIndex].start;
    dst = (unsigned int *)framebuffer;
    cdst = (unsigned char *)framebuffer;
    if(src == NULL || src == MAP_FAILED)
    {
        usleep(200*1000);
        return 0;
    }
    //JYZ_PrintLog("x:%d,y:%d,h:%d,w:%d",rect.x(),rect.y(),rect.height(),rect.width());
#ifdef PLATFORM_JZ4740
    for(i = rect.y(); i< rect.y()+rect.height();i=i+2)
    {
        for(j=rect.x(); j< rect.x() + rect.width(); j++)
        {
            tmp1 = src[(j*320+i)];
            tmp2 = src[(j*320+i+1)];
            dst[i*240+j] = m_rgb16torgb32table[tmp1];
            dst[(i+1)*240+j] = m_rgb16torgb32table[tmp2];
        }
    }
#else
    for(i = 0; i< 320;i=i+2)
//    for(i = 0; i< 240;i=i+2)
    {
        for(j=0; j<240; j++)
//        for(j=0; j< 320; j++)
        {

#ifdef RGB_CAMERA_MIRROR_H
            unsigned int *dst1=&dst[i*240+239-j];
            unsigned int *dst2=&dst[(i+1)*240+239-j];
#else
            unsigned int *dst1=&dst[i*240+j];
//            unsigned int *dst1=&dst[i*320+j];
            unsigned int *dst2=&dst[(i+1)*240+j];
//            unsigned int *dst2=&dst[(i+1)*320+j];
#endif

#if 0
#ifdef RGB_CAMERA_MODE_YUV
            unsigned int tmp1,tmp2,tmp3,tmp4;
#ifdef ROTATE180
            tmp2 = src[(479-2*j)*640+(639-i*2)];
            tmp1 = src[(479-2*j)*640+(639-(i*2+1))];
            tmp4 = src[(479-2*j)*640+(639-(i+1)*2)];
            tmp3 = src[(479-2*j)*640+(639-((i+1)*2+1))];
            ConvertYUV2RGB32(((tmp2&0xFF)+(tmp1&0xFF))>>1,tmp1>>8,tmp2>>8,dst1);
            ConvertYUV2RGB32(((tmp3&0xFF)+(tmp4&0xFF))>>1,tmp3>>8,tmp4>>8,dst2);
#else
            tmp1 = src[(j*640+i)*2];
            tmp2 = src[(j*640+i)*2+1];
            tmp3 = src[(j*640+i+1)*2];
            tmp4 = src[(j*640+i+1)*2+1];
            ConvertYUV2RGB32(((tmp2&0xFF)+(tmp1&0xFF))>>1,tmp1>>8,tmp2>>8,dst1);
            ConvertYUV2RGB32(((tmp3&0xFF)+(tmp4&0xFF))>>1,tmp3>>8,tmp4>>8,dst2);
#endif
#elif defined(RGB_CAMERA_MODE_RAW)
            unsigned char r1,g1,b1,r2,g2,b2;

            r1 = src[2*j*640+2*i+1];
            r2 = src[2*j*640+2*i-1];
            g1 = src[2*j*640+2*i];
            b1 = src[(2*j+1)*640+2*i];
            b2 = src[(2*j-1)*640+2*i];
            *dst1 = PACKRGB32((r1+r2)>>1,g1,(b1+b2)>>1);

            r1 = src[2*j*640+2*i+3];
            r2 = src[2*j*640+2*i+1];
            g1 = src[2*j*640+2*i+2];
            b1 = src[(2*j+1)*640+2*i+2];
            b2 = src[(2*j-1)*640+2*i+2];
            *dst2 = PACKRGB32((r1+r2)>>1,g1,(b1+b2)>>1);
#else
            tmp1 = src[(j*640+i)*2];
            tmp2 = src[(j*640+i+1)*2];
            *dst1 = m_rgb16torgb32table[tmp1];
            *dst2 = m_rgb16torgb32table[tmp2];
#endif
#endif
#define MTH 752
#define MTV 480
	    //*dst1 = (csrc[i*752+j])<<24+(csrc[i*752+j])<<16+(csrc[i*752+j])<<8;
	    //*dst2 = (csrc[(i+1)*752+j])<<24+(csrc[(i+1)*752+j])<<16+(csrc[(i+1)*752+j])<<8;
#if 1
	    r=csrc[j*MTH*2+2*i];
	    g=csrc[(j)*MTH*2+2*(i+1)];
	    *dst1 = PACKRGB32(r,r,r);
	    *dst2 = PACKRGB32(g,g,g);
#endif	    


        }
    }
#endif
    return 0;
}


int main()
{
    FbdevInit();
    HWCameraRGBInit();
    while(1)
    {
        GetDataForDisplay(screenbuf);
        usleep(1000);
        FbdevUpdate();
    }
    HWCameraRGBFree();
    FbdevFree();
    return 0;
}
