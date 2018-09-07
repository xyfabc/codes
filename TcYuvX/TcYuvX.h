/*=///////////////////////////////////////////////////////////////////////=*/
/*= TcYuvX.h : Defines the YUV��� header for the LIB application.      =*/
/*= Made By ShanChengKun.PentiumWorkingRoom   Based Made in 2014.12.08    =*/
/*= CopyRight(C) 1996-2018 . Email: sck007@163.com . Update on 2015.09.19 =*/
/*=///////////////////////////////////////////////////////////////////////=*/

#if !defined(_SCK007_TCYUVLIB_INCLUDED_PENTIUMWORKINGROOM_ZNN008_)
#define _SCK007_TCYUVLIB_INCLUDED_PENTIUMWORKINGROOM_ZNN008_

/*=///////////////////////////////////////////////////////////////////////=*/

typedef unsigned char		BYTE;
typedef unsigned short		WORD;
typedef unsigned int		UINT;

/*=///////////////////////////////////////////////////////////////////////=*/

/* DShow��4:2:2����ѡ��ʽ(YUYV)�����-YUY2����С(nWd * nHi * 2) */
void RgbFromPackYUY2(BYTE *pRgb, const BYTE *pYuv, int nWd, int nHi);

/* Yuv420SP(Yƽ��UV���)��0=DShow��NV12��1=��׿NV21����С(nWd * nHi * 3 / 2) */
void RgbFromYuv420SP(BYTE *pRgb, const BYTE *pYuv, int nWd, int nHi, int nFor);

/* Yuv420P(YUV��Ϊƽ��)��0=��׼��I420��1=H264��YV12����С(nWd * nHi * 3 / 2) */
void RgbFromYuv420P(BYTE *pRgb, const BYTE *pYuv, int nWd, int nHi, int nFor);

/* ��RAW��ʽʹ�ò�ֵ�ķ�������RGB���ݣ�(0=BG,1=RG,2=GB,3=GR) */
void RgbFromRaw(BYTE *pRgb, const BYTE *pRaw, int nWd, int nHi, int nWho);

/*=///////////////////////////////////////////////////////////////////////=*/

#endif /* _SCK007_TCYUVLIB_INCLUDED_PENTIUMWORKINGROOM_ZNN008_ */

/*=///////////////////////////////////////////////////////////////////////=*/
/*= The end of this file. =*/
