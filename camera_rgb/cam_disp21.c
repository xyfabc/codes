#include "HWCameraCmd.h"
#include <sys/ioctl.h>
#include <linux/videodev.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "TcYuvX.h"
#include "write_bmp_func.h"
#include <time.h>
#include <unistd.h>





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
#if 0
#define IMG_W 1280

#define IMG_H 720
#else

#define IMG_W 640
#define IMG_H 480
#endif


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
    m_fd = open ("/dev/video22",O_RDONLY);
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
        int length = IMG_W*IMG_H*2;
        m_videoBuffer[i].length = length;
        m_videoBuffer[i].start = mmap (NULL, length,PROT_READ, MAP_SHARED, m_fd,0);
        usleep(30000);
        if (MAP_FAILED == m_videoBuffer[i].start)
        {
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


void HWCameraRGBFree(void)
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


#if 0

typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;
/*ERROR*/
#define BMPERR_INTERNAL_ERROR 1 /*include malloc failure, open file failure*/
#define BMPERR_WIDTH_NOT_FOUR_TIMES 2
#define BMPERR_INVALID_INPUT 3 

int save_grey_bmp(uint8_t *pSrc, int32_t width, int32_t height, uint8_t **pDst, char *file_name)
{
	if( (width == 0) || (height==0) ) return (BMPERR_INVALID_INPUT);
	if(file_name == NULL) return (BMPERR_INVALID_INPUT);

	int i = 0;

	int dataSizePerLine = (width + 3)/4*4;//for future use
	int nsizeImage = dataSizePerLine * height;
	int nsrcSize = nsizeImage; //width * height;
	
	uint8_t *p = (uint8_t *)malloc(sizeof(uint8_t)*(14 + 40 + 4*256 + nsrcSize));
	if(p==NULL) return (BMPERR_INTERNAL_ERROR);

	int nfilesize = 14 + 40 + 4*256 + nsizeImage;
	int noffbits = 14 + 40 + 4*256;

	/*file header, 14 bytes */
	p[0] = 'B'; p[1] = 'M';//bfType
	p[2] = (nfilesize)&0xff; p[3] = (nfilesize>>8)&0xff; p[4] = (nfilesize>>16)&0xff; p[5] = (nfilesize>>24)&0xff;//bdSize
	p[6] = 0x00; p[7] = 0x00;//bfReserved1
	p[8] = 0x00; p[9] = 0x00;//bfReserved2
	p[10] = (noffbits)&0xff; p[11] = (noffbits>>8)&0xff; p[12] = (noffbits>>16)&0xff; p[13] = (noffbits>>24)&0xff;//bfOffBits

	/*bitmap information header, 40 bytes*/
	p[14] = 0x28; p[15] = 0x00; p[16] = 0x00; p[17] = 0x00;//biSize, fixed 40
	p[18] = (width)&0xff; p[19] = (width>>8)&0xff; p[20] = (width>>16)&0xff; p[21] = (width>>24)&0xff;//Width
	p[22] = (height)&0xff; p[23] = (height>>8)&0xff; p[24] = (height>>16)&0xff; p[25] = (height>>24)&0xff;//Height
	p[26] = 0x01; p[27] = 0x00;//biPlanes, 1
	p[28] = 0x08; p[29] = 0x00;//biBitCount,here grey 8 bits
	p[30] = 0x00; p[31] = 0x00; p[32] = 0x00; p[33] = 0x00;//biCompression, 0 no compression
	p[34] = (nsizeImage)&0xff; p[35] = (nsizeImage>>8)&0xff; p[36] = (nsizeImage>>16)&0xff; p[37] = (nsizeImage>>24)&0xff;//biSizeImage
	p[38] = 0x00; p[39] = 0x00; p[40] = 0x00; p[41] = 0x00;//biXPelsPerMeter
	p[42] = 0x00; p[43] = 0x00; p[44] = 0x00; p[45] = 0x00;//biYPelsPerMeter
	p[46] = 0x00; p[47] = 0x00; p[48] = 0x00; p[49] = 0x00;//biClrUsed, 0
	p[50] = 0x00; p[51] = 0x00; p[52] = 0x00; p[53] = 0x00;//biClrImportant, 0

	/*color table*/
	for(i=0; i<256; i++){
		p[54+4*i+0] = i;
		p[54+4*i+1] = i;
		p[54+4*i+2] = i;
		p[54+4*i+3] = 0;
	}
	uint8_t *q = p+14+40+4*256;
	
	unsigned char zeropad[] = "\0\0\0\0";

        pSrc += width * (height - 1);
        for( i = 0; i< height ; i++ ){
                memcpy(q, pSrc, width);
                if(dataSizePerLine > width){
                        memcpy(q+width, zeropad, dataSizePerLine - width);
                }
                q += dataSizePerLine;
                pSrc -= width;//here here, note bmp save order, bottom->up, left->right
        }



/*	for(i=0;i<height;i++){
		memcpy(q, pSrc, width);
		if(dataSizePerLine > width){
			memcpy(q+width, zeropad, dataSizePerLine -  width);
		}
		q += dataSizePerLine;
		pSrc += width;
	}		
*/
	FILE *fp = fopen(file_name, "wb");
	if(fp==NULL) return (BMPERR_INTERNAL_ERROR);
	int j = 0;
	for(j=0;j<nfilesize;j++){
		fwrite(p+j, 1, 1, fp);
	//	printf("j: %d, p[%d]: %d\n", j, j, p[j]);
	}
	fclose(fp);

	if(pDst == NULL){
		if(p!=NULL){ free(p); p = NULL;}
	}else{
		*pDst = p;
	}

	return 0;
}


//rgb -> bgr, up & left -> bottom & left
int save_rgb_bmp(uint8_t *pSrc, int32_t width, int32_t height, uint8_t **pDst, char *file_name)
{
	if( (width == 0) || (height == 0) ) return (BMPERR_INVALID_INPUT);
	if(file_name==NULL) return (BMPERR_INVALID_INPUT);

	int i = 0, k = 0;
	int dataSizePerLine = (width*3+3)/4*4;//for future use
	int nsizeImage = dataSizePerLine * height;
	int nsrcSize = nsizeImage;

	//printf("dataSizePerLine: %d, nsizeImage: %d, nsrcSize: %d\n", dataSizePerLine, nsizeImage, nsrcSize);

	uint8_t *p = (uint8_t *)malloc(sizeof(uint8_t)*(14 + 40 + nsrcSize));
	if(p==NULL) return (BMPERR_INTERNAL_ERROR);

	int nfilesize = 14 + 40 + nsizeImage;
	
	/*file header, 14 bytes */
	p[0] = 'B'; p[1] = 'M';//bfType
	p[2] = (nfilesize)&0xff; p[3] = (nfilesize>>8)&0xff; p[4] = (nfilesize>>16)&0xff; p[5] = (nfilesize>>24)&0xff;//bdSize
	p[6] = 0x00; p[7] = 0x00;//bfReserved1
	p[8] = 0x00; p[9] = 0x00;//bfReserved2
	p[10] = 0x36; p[11] = 0x00; p[12] = 0x00; p[13] = 0x00;//bfOffBits, 54 = 0x36

	/*bitmap information header, 40 bytes*/
	p[14] = 0x28; p[15] = 0x00; p[16] = 0x00; p[17] = 0x00;//biSize, fixed 40
	p[18] = (width)&0xff; p[19] = (width>>8)&0xff; p[20] = (width>>16)&0xff; p[21] = (width>>24)&0xff;//Width
	p[22] = (height)&0xff; p[23] = (height>>8)&0xff; p[24] = (height>>16)&0xff; p[25] = (height>>24)&0xff;//Height
	p[26] = 0x01; p[27] = 0x00;//biPlanes, 1
	p[28] = 0x18; p[29] = 0x00;//biBitCount,here rgb 24 bits
	p[30] = 0x00; p[31] = 0x00; p[32] = 0x00; p[33] = 0x00;//biCompression, 0 no compression
	p[34] = (nsizeImage)&0xff; p[35] = (nsizeImage>>8)&0xff; p[36] = (nsizeImage>>16)&0xff; p[37] = (nsizeImage>>24)&0xff;//biSizeImage
	p[38] = 0x00; p[39] = 0x00; p[40] = 0x00; p[41] = 0x00;//biXPelsPerMeter
	p[42] = 0x00; p[43] = 0x00; p[44] = 0x00; p[45] = 0x00;//biYPelsPerMeter
	p[46] = 0x00; p[47] = 0x00; p[48] = 0x00; p[49] = 0x00;//biClrUsed, 0
	p[50] = 0x00; p[51] = 0x00; p[52] = 0x00; p[53] = 0x00;//biClrImportant, 0

	unsigned char  *q = p + 54;
        unsigned char zeropad[] = "\0\0\0\0";
	int step = width * 4;
	pSrc += step * (height - 1);

//rgb -> bgr, up & left -> bottom & left
    	for( k = 0; k < height; k++)
    	{
        	for( i = 0; i < width; i++, q += 4, pSrc += 4 )
        	{
            		unsigned char t0 = pSrc[0], t1 = pSrc[1], t2 = pSrc[2];
            		q[2] = t0; q[1] = t1; q[0] = t2;
        	}
                if(dataSizePerLine > step){
                        memcpy(q, zeropad, dataSizePerLine - step);
                }

        	q += dataSizePerLine - step;
		pSrc -= step; pSrc -= step;
    	}

//bgr -> bgr, but order inverse.
/*        for(i=0;i<height;i++){
                memcpy(q, pSrc, width * 3);
                if(dataSizePerLine > width* 3){
                        memcpy(q+width*3, zeropad, dataSizePerLine -  width * 3);
                }
                q += dataSizePerLine;
                pSrc += width * 3;
        }
*/
//	memcpy(p+54, pSrc, nsrcSize);
	
	FILE *fp = fopen(file_name, "wb");
	if(fp==NULL) return (BMPERR_INTERNAL_ERROR);
	int j = 0;
	for(j=0;j<nfilesize;j++){
		fwrite(p+j, 1, 1, fp);
	//	printf("j: %d, p[%d]: %d\n", j, j, p[j]);
	}
	fclose(fp);

	if(pDst == NULL){
		if(p!=NULL){ free(p); p = NULL;}
	}else{
		*pDst = p;
	}

	return 0;
}

#endif

#define RGB_CAMERA_MODE_YUV
#define ROTATE180

unsigned int GetDataForDisplay(unsigned char *framebuffer)
{
    unsigned int ret = 1;

    unsigned char frameIndex= 0 ;
    unsigned short *src = NULL;
    unsigned int *dst = NULL;
    unsigned short tmp1,tmp2;
    unsigned int tmpvalue1;
    unsigned char r,g,b;
	unsigned char *cdst = NULL;
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
	cdst = (unsigned char*)m_videoBuffer[frameIndex].start;
    dst = (unsigned int *)framebuffer;
	uint8_t * m_buffer = (uint8_t*)malloc(3*IMG_H*IMG_W*sizeof(uint8_t));
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
    {
        for(j=0; j< 240; j++)
        {
#ifdef RGB_CAMERA_MIRROR_H
            unsigned int *dst1=&dst[i*240+239-j];
            unsigned int *dst2=&dst[(i+1)*240+239-j];
#else
            unsigned int *dst1=&dst[i*240+j];
            unsigned int *dst2=&dst[(i+1)*240+j];
#endif
#ifdef RGB_CAMERA_MODE_YUV
            unsigned int tmp1,tmp2,tmp3,tmp4;
#ifdef ROTATE180
            tmp2 = src[(IMG_H-1-2*j)*IMG_W+(IMG_W-1-i*2)];
            tmp1 = src[(IMG_H-1-2*j)*IMG_W+(IMG_W-1-(i*2+1))];
            tmp4 = src[(IMG_H-1-2*j)*IMG_W+(IMG_W-1-(i+1)*2)];
            tmp3 = src[(IMG_H-1-2*j)*IMG_W+(IMG_W-1-((i+1)*2+1))];
            ConvertYUV2RGB32(((tmp2&0xFF)+(tmp1&0xFF))>>1,tmp1>>8,tmp2>>8,dst1);
            ConvertYUV2RGB32(((tmp3&0xFF)+(tmp4&0xFF))>>1,tmp3>>8,tmp4>>8,dst2);
			//m_buffer[i*240+j] = *dst1;
			//m_buffer[(i+1)*240+j] = *dst2;
			//*dst1 = PACKRGB32(((tmp2&0xFF)+(tmp1&0xFF))>>1,((tmp2&0xFF)+(tmp1&0xFF))>>1,((tmp2&0xFF)+(tmp1&0xFF))>>1);
			//*dst2 = PACKRGB32(((tmp3&0xFF)+(tmp4&0xFF))>>1,((tmp3&0xFF)+(tmp4&0xFF))>>1,((tmp3&0xFF)+(tmp4&0xFF))>>1);

			//*dst1 = PACKRGB32(tmp2>>8,tmp2>>8,tmp2>>8);
			//*dst2 = PACKRGB32(tmp4>>8,tmp4>>8,tmp4>>8);

		 	//ConvertYUV2RGB32(((tmp2&0xFF)+(tmp1&0xFF))>>1,0,0,dst1);
           // ConvertYUV2RGB32(((tmp3&0xFF)+(tmp4&0xFF))>>1,0,0,dst2);
#else
            tmp1 = src[(j*IMG_W+i)*2];
            tmp2 = src[(j*IMG_W+i)*2+1];
            tmp3 = src[(j*IMG_W+i+1)*2];
            tmp4 = src[(j*IMG_W+i+1)*2+1];
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
            tmp1 = src[(j*IMG_W+i)*2];
            tmp2 = src[(j*IMG_W+i+1)*2];
            *dst1 = m_rgb16torgb32table[tmp1];
            *dst2 = m_rgb16torgb32table[tmp2];
#endif
        }
    }
#endif

	
#if 1
/*
	printf("\n---------\n");

	for(i=0;i<IMG_H*IMG_W;i = i+2){
			//m_buffer[i] = (src[i]&0xFF);
			ConvertYUV2RGB32(src[i]&0xFF,src[i]>>8,src[i+1]>>8,&m_buffer[i]);
			ConvertYUV2RGB32(src[i+1]&0xFF,src[i]>>8,src[i+1]>>8,&m_buffer[i+1]);
	}*/
	//printf("\n----1-----\n");
#if 1
 		if (access("/dev/sda1",R_OK) == 0){
			RgbFromPackYUY2((uint8_t*)m_buffer, cdst,IMG_W, IMG_H);
			char file_name[64] ={0};
			static int count = 0;
			
			struct tm *t;
		   	 time_t tt;
			time(&tt);
			t = localtime(&tt);
 
			sprintf(file_name,"/mnt/usb/test--%02d_%02d_%02d_%02d.bmp",t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
			printf("\n----------------file_name= %s\n",file_name);
						
			save_rgb_bmp((uint8_t*)m_buffer,  IMG_W,IMG_H, NULL, file_name);
			sync();
			count++;
		}
#endif

#endif
	free(m_buffer);


    return 0;
}


int main()
{
    FbdevInit();
    HWCameraRGBInit();
    while(1)
    {
        GetDataForDisplay(screenbuf);
        usleep(4000);
        FbdevUpdate();
    }
    HWCameraRGBFree();
    FbdevFree();
    return 0;
}
