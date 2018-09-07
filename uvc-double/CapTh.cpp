#include "CapTh.h"
#include <QDebug>

//#include "include/jpeglib.h"
#include <setjmp.h>

#include <iostream>
#include <string>
#include <stdlib.h>
#include "TcYuvX.h"






//struct my_error_mgr {
//  struct jpeg_error_mgr pub;	/* "public" fields */

//  jmp_buf setjmp_buffer;	/* for return to caller */
//};

//typedef struct my_error_mgr * my_error_ptr;

///*
// * Here's the routine that will replace the standard error_exit method:
// */

//METHODDEF(void)
//my_error_exit (j_common_ptr cinfo)
//{
//  /* cinfo->err really points to a my_error_mgr struct, so coerce pointer */
//  my_error_ptr myerr = (my_error_ptr) cinfo->err;

//  /* Always display the message. */
//  /* We could postpone this until after returning, if we chose. */
//  (*cinfo->err->output_message) (cinfo);

//  /* Return control to the setjmp point */
//  longjmp(myerr->setjmp_buffer, 1);
//}

//GLOBAL(int)
//convert(unsigned char *buf,int len,unsigned char **raw)
//{

//    unsigned char **tmp = raw;
//  struct jpeg_decompress_struct cinfo;

//  struct my_error_mgr jerr;
//  JSAMPARRAY buffer;		/* Output row buffer */
//  int row_stride;		/* physical row width in output buffer */

//  cinfo.err = jpeg_std_error(&jerr.pub);
//  jerr.pub.error_exit = my_error_exit;

//  if (setjmp(jerr.setjmp_buffer)) {

//    jpeg_destroy_decompress(&cinfo);
//   // fclose(infile);
//    return 0;
//  }

//  jpeg_create_decompress(&cinfo);
//  jpeg_mem_src(&cinfo, buf, len);
//  (void) jpeg_read_header(&cinfo, TRUE);
//  cinfo.out_color_space = JCS_GRAYSCALE;
//  (void) jpeg_start_decompress(&cinfo);

//  row_stride = cinfo.output_width * cinfo.output_components;
////  qDebug()<<cinfo.output_width<<cinfo.output_height<<row_stride<<cinfo.output_scanline<<cinfo.output_components;
//  buffer = (*cinfo.mem->alloc_sarray)
//          ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
// //qDebug()<<"output_scanline";
//  while (cinfo.output_scanline < cinfo.output_height) {

//    (void) jpeg_read_scanlines(&cinfo, buffer, 1);
//      // qDebug()<<"------------------------------";
////    /* Assume put_scanline_someplace wants a pointer and sample count. */
//      memcpy((*tmp+cinfo.output_scanline*row_stride),buffer[0],row_stride);
//      // qDebug()<<"------------------------------"<<cinfo.output_scanline*row_stride;
//     // *tmp = *tmp +row_stride;
//  }

//  (void) jpeg_finish_decompress(&cinfo);
//  jpeg_destroy_decompress(&cinfo);
// // fclose(infile);
//  return 1;
//}

//-------------------------------------------------------------
/**
\brief   水平镜像翻转
\param   pInputBuf            数据输入buffer指针
\param   pOutputBuf           数据输出buffer指针
\param   nWidth               图像的宽
\param   nHeight              图像的高

\return  图像镜像翻转状态
*/
//-------------------------------------------------------------
int  HorizontalMirror(unsigned char *pInputBuf, unsigned char *pOutputBuf, int nWidth, int nHeight)
{
    if ((pInputBuf == NULL) || (pOutputBuf == NULL))
    {
        return -1;
    }

    if ((nWidth <= 0) || (nHeight <= 0))
    {
        return -1;
    }

    int i = 0;
    int j = 0;

    int            nStep = 2 * nWidth;                  // 输出图像移动的步长
    unsigned char *pSrce = pInputBuf;                   // 指向输入图像的buffer
    unsigned char *pDest = pOutputBuf + (nWidth - 1);   // 指向输出图像的buffer

    // 水平镜像翻转
    for (i = 0; i < nHeight; i++)
    {
        for (j = 0; j < nWidth; j++)
        {
            *pDest = *pSrce;
            pDest--;
            pSrce++;
        }
        pDest += nStep;
    }
    return 0;
}


CapTh::CapTh()
{
    IsCapture = true;
    buf = new unsigned char[YUV_IMG_SIZE];
    rgb = new unsigned char[RGB_IMG_SIZE];
    gray = new unsigned char[RGB_IMG_SIZE];
    tmp = new unsigned char[YUV_IMG_SIZE];
      dev = string("/dev/media0");
}

void CapTh::run()
{
   // string dev = string("/dev/video1");
    //cap.setDevice(dev);
    V4l2 cap(dev);
    if(!cap.isValid()) {
        cerr << "Device: " << dev << " is not valid!!!" << endl;
        exit(1);
    }
     cap.setPixelFormat(WIDTH,HEIGHT,V4L2_PIX_FMT_YUYV,FPS,true);
//    if(0 == camera_id){
//        cap.setPixelFormat(WIDTH,HEIGHT,V4L2_PIX_FMT_YUYV, 15,true);
//    }else{
//        cap.setPixelFormat(WIDTH,HEIGHT,V4L2_PIX_FMT_YUYV, true);
//    }
    cap.startStreaming();
    while(1){
        if(IsCapture){

            int len = 0;
            cap.Grap(buf, &len,2000);
            qDebug()<<"UpdateRgbImgData"<<camera_id<<len;

            RgbFromPackYUY2(rgb, buf, WIDTH, HEIGHT);
            emit UpdateRgbImgData(rgb,camera_id);
            msleep(20);

        }else{
            //qDebug()<<"LOOP";
             msleep(1000);
        }
    }

}

void CapTh::StreamCtr(bool key)
{
//    if(key == IsCapture){
//        return;
//    }
//   if(key) {
//       cap.startStreaming();
//       IsCapture = true;
//   }else{
//       IsCapture = false;
//       cap.stopStreaming();
//   }
}
