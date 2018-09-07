/*=///////////////////////////////////////////////////////////////////////=*/
/*= TesoPics.h : Defines the function header for the LIB application.     =*/
/*= Made By ShanChengKun.PentiumWorkingRoom   Based Made in 2011.03.30    =*/
/*= CopyRight(C) 1996-2018 . Email: sck007@163.com . Update on 2013.12.30 =*/
/*=///////////////////////////////////////////////////////////////////////=*/

#if !defined(_SCK007_TESOPICSLIB_INCLUDED_PENTIUMWORKINGROOM_ZNN008_)
#define _SCK007_TESOPICSLIB_INCLUDED_PENTIUMWORKINGROOM_ZNN008_

#ifdef __cplusplus
extern "C" {
#endif

/*=///////////////////////////////////////////////////////////////////////=*/
/*= ����Ľӿ����ͣ������صĴ����붨�壬>=0ʱΪ�ɹ����ɱ�ʾ�ض����塣     =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* ����������ʽ�У�UNIX/LINUX�µ�.o��Windows�µ�DLL��Windows�µ�LIB */
#if defined(__GNUC__) && /* GCC������4�汾 */ \
        ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#define SPF_API(__type) __attribute__ ((visibility("default"))) __type
#else
#define SPF_API(__type) __type __stdcall
#endif

/*===============================================================*/

/* �����Ҫ�����������ʹ�С�ĳ��϶���ʹ�� */
#ifndef TESO_ZNN_TYPES
#define TESO_ZNN_TYPES

typedef signed char			S8;
typedef unsigned char		U8;

typedef signed short		S16;
typedef unsigned short		U16;

typedef signed int			S32;
typedef unsigned int		U32;

#define UMAX				0xFFFFFFFF

#endif /* #ifndef TESO_ZNN_TYPES */

/* Unix��Linux��ʱ����Ҫ�ı������ͣ������ֺ궨�� */
#ifndef WIN32
#ifndef TESO_SPF_TYPES
#define TESO_SPF_TYPES

typedef unsigned char		BYTE;
typedef signed char			SBYTE;
typedef signed char			CHAR;

typedef unsigned short		USHORT;
typedef unsigned short		WORD;
typedef signed short		SHORT;

typedef unsigned int		UINT;
typedef signed int			INT;

typedef unsigned int		DWORD;
typedef unsigned int		ULONG;
typedef signed int			LONG;

typedef int					BOOL;
typedef float				FLOAT;
typedef double				DOUBLE;

/* void parameters */
#ifndef VOID
	#define VOID			void
#endif

/* Pointer stuff */
#ifndef NULL
	#define NULL			0
#endif

/* constant values */
#ifndef CONST
	#define CONST			const
#endif

/* BOOL stuff - false */
#ifndef FALSE
	#define FALSE			0
#endif

/* BOOL stuff - true */
#ifndef TRUE
	#define TRUE			1
#endif

typedef const char *		LPCTSTR;

/* register storage */
#ifndef REGISTER
	#define REGISTER 
#endif

/* functions inline */
#ifndef INLINE
	#define INLINE
#endif

#endif /* #ifndef TESO_SPF_TYPES */

#else /* ��Windows�����±��� */

#ifndef REGISTER
	#define REGISTER		register
#endif

#ifndef INLINE
	#define INLINE			inline
#endif

#endif /* #ifndef WIN32 */

/*===============================================================*/

#define LO_BY(w)		((BYTE)((w) & 0xFF))
#define HI_BY(w)		((BYTE)(((WORD)(w) >> 8) & 0xFF))
#define MK_WD(h, l)		(((WORD)(h) << 8) | ((WORD)(l) & 0xFF))

#define LR_BY(w)		HI_BY(w)					/* Host���ֽ� */
#define HR_BY(w)		LO_BY(w)					/* Host���ֽ� */
#define MR_WD(h, l)		MK_WD(l, h)					/* Host�� */

#define SwrP(w)		((((w) << 8) & 0xFF00) | (((w) >> 8) & 0xFF))
#define SwaP(w)		(WORD)SwrP(w)
#define SdwP(dw)	((SwrP(dw) << 16) | SwrP((dw) >> 16))

#define MeSwaP(w)		w = SwaP(w)					/* ����WORD�� */
#define MeSdwP(dw)		dw = SdwP(dw)				/* ����DW˫�� */

#define min(a, b)		(((a) < (b)) ? (a) : (b))	/* ����Сֵ */
#define max(a, b)		(((a) > (b)) ? (a) : (b))	/* �����ֵ */

#define lmtMin(v, th)	if((v) < (th)) (v) = (th)	/* �޶���Сֵ */
#define lmtMax(v, th)	if((v) > (th)) (v) = (th)	/* �޶����ֵ */
#define lmtLmt(v, n, x)	lmtMin(v, n); lmtMax(v, x)	/* �޶�����ֵ */

#define setBit(v, b)	(v) |= (1 << (b))			/* ��1ָ��bit */
#define clrBit(v, b)	(v) &= (~(1 << (b)))		/* ��0ָ��bit */
#define getBit(v, b)	(((v) & (1 << (b))) != 0)	/* ȡ?ָ��bit */

/*===============================================================*/

#define SureValAscnd(va, vb, tv) /* ����С->������� */ \
{ \
	if((va) > (vb)) \
	{ \
		tv tmp = (va); (va) = (vb); (vb) = tmp; \
	} \
}

#define SureValDscnd(va, vb, tv) /* ���Ӵ�->С������ */ \
{ \
	if((va) < (vb)) \
	{ \
		tv tmp = (va); (va) = (vb); (vb) = tmp; \
	} \
}

/*===============================================================*/

U16  PtrGet16(U8 **q);								/* С�˶�02�ֽ� */
U32  PtrGet32(U8 **q);								/* С�˶�04�ֽ� */
void PtrSet16(U8 **q, U16 u);						/* С��д02�ֽ� */
void PtrSet32(U8 **q, U32 u);						/* С��д04�ֽ� */

/*===============================================================*/

#ifndef RGB					/* ����RGB��ɫ��ֵ */
typedef UINT				COLORREF;
typedef UINT				*LPCOLORREF;
#define RGB(r,g,b)			/* ����0-B-G-R��ɫ */ \
((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((UINT)(BYTE)(b))<<16)))
#endif /* #ifndef RGB */

/*===============================================================*/

#define WSQ_COF				10000					/* WSQ_����ϵ�� */
#define WSQ_TXT				"TechShino"				/* WSQ_�����ִ� */

enum {PIC_NON, PIC_BMP, PIC_JPG, PIC_TIF, PIC_PNG,	/* ֧��ͼ���ʽ */
		PIC_WSQ, PIC_JP2, PIC_JPC, PIC_J2K, PIC_J2C, PIC_TFF};
#define PICS_MAXI			10						/* ���֧������ */

static LPCTSTR PICS_FILTER = "All Files(*.BmP,*.JpG,*.TiF,*.PnG,"
	"*.WsQ,*.Jp2,*.JpC,*.J2K,*.J2C,*.TiFF)|*.BmP;*.JpG;*.TiF;*.PnG;"
	"*.WsQ;*.Jp2;*.JpC;*.J2K;*.J2C;*.TiFF||";		/* �Ի�������� */
static LPCTSTR PICS_EXNM[PICS_MAXI] = {".BmP",		/* ��չ���ַ��� */
	".JpG",".TiF",".PnG",".WsQ",".Jp2",".JpC",".J2K",".J2C",".TiFF"};
static LPCTSTR SUPT_PICS = "BmP;JpG;TiF;PnG;WsQ;Jp2;JpC;J2K;J2C;TiFF";

/*=///////////////////////////////////////////////////////////////////////=*/
/*= CRC32У���룬Base64����룬1D��2D�ڴ�ķ�����ͷŵȹ���               =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* ����һ�����������䳤�ȣ��������CRC32������ */
SPF_API(UINT) GeneraCrc32
					(
					UINT dwCrc,						/* ��ʼ��У��ֵ */
					const void *hBuf,				/* ��У�������� */
					UINT dwLen						/* �������ĳ��� */
					);

/* B64���룺��Bin=>B64ת��������󷵻ز���'\0'�ĳ��� */
SPF_API(int) EnCodeB64
					(
					void *pVdst,					/* ��������ڴ� */
					const void *pVsrc,				/* ����0=�󳤶� */
					int nSlen/* = 0*/				/* ��������'\0' */
					);

/* B64���룺��Bin<=B64ת��������󷵻���Ч��Ŀ�곤�� */
SPF_API(int) DeCodeB64
					(
					void *pVdst,					/* ���0=�󳤶� */
					const void *pVsrc,				/* �������ݻ��� */
					int nSlen/* = 0*/				/* �������ĳ��� */
					);

/* ��ѹ=0����֪�������ѹ��(1���,6�е�,9���)������0ʧ�� */
SPF_API(UINT) ZipBinInMem
					(
					int nMod,						/* 0=��ѹ!0ѹ�� */
					void *hDst,						/* ��������ڴ� */
					UINT uDlen,						/* ��������С */
					const void *hSrc,				/* �������ݻ��� */
					UINT uSlen						/* �������ĳ��� */
					);

/*===============================================================*/

/* �����ά����ռ䣬��������������Ԫ�ֽڴ�С */
SPF_API(void **) TesoNewMo2D
					(
					int iCol, int iRow,				/* ���������� */
					int iItm						/* �����ֽ��� */
					);

/* �ͷŶ�ά����ռ䣬���ô�ָ��ԭַΪNULL��Ч */
SPF_API(void) TesoDelMo2D
					(
					void ***pPtr					/* ��άָ���� */
					);

/*===============================================================*/

/* ȷ���ڴ�Ϊ�����Ĵ�С����ԭ���Ҳ��������·��� */
SPF_API(BOOL) TesoNewMemo
					(
					BYTE **hMem,					/* �ڴ�ָ���ַ */
					int *poSz,						/* ԭ���ڴ��С */
					int nwSz						/* �����ڴ��С */
					);

/* �ͷŸ�����ָ��ָ��Ķ����ڴ棬���ô�ָ��ΪNULL����Ч */
SPF_API(void) TesoDelMemo
					(
					BYTE **hMem						/* �ڴ�ָ���ַ */
					);

/* ȷ���ڴ�Ϊ�����Ĵ�С����ԭ�в��㹻�������·��� */
SPF_API(BOOL) TesoMemSure
					(
					BYTE **hMem,					/* �ڴ�ָ���ַ */
					int *poSz,						/* ԭ���ڴ��С */
					int nwSz						/* �����ڴ��С */
					);

/*===============================================================*/

/* ȷ���ڴ�Ϊ�µĿ��ɫ������Ҫ��С��0=RGB24��1=�Ҷ�8 */
SPF_API(BOOL) SureImgMemo
					(
					BYTE **hBuf,					/* ͼ��ָ���ַ */
					int *pOwd, int *pOhi,			/* ԭͼ�Ŀ�͸� */
					int *pOmd,						/* ԭͼ0=��1=�� */
					int nNwd, int nNhi,				/* ��Ҫ�Ŀ�͸� */
					int nNmd						/* ��Ҫ0=��1=�� */
					);

/* �ͷŸ���ָ���ͼ������ڴ棬���ô�ָ��Ϊ��NULL��Ч */
SPF_API(void) FreeImgMemo
					(
					BYTE **hBuf						/* ͼ��ָ���ַ */
					);

/*=///////////////////////////////////////////////////////////////////////=*/
/*= ͼ��BMP��JPG�ļ��غͱ���(��֮��ת��)��ͼ�����ݵļ��á���ת���任��    =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* ����ָ����ͼ�񵽵ȿ�ߵ�Ŀ���� */
SPF_API(BYTE *) CopyAnImage
					(
					BYTE *hDst,						/* NULL������ */
					const BYTE *pImg,				/* ����Դͼ���� */
					int nWd, int nHi,				/* ���صĿ�͸� */
					int nMo							/* ��ʽ0=��1=�� */
					);

/* �ֱ���ȡͼ��RGBYUV=[0,5]ͨ������ͼ */
SPF_API(BYTE *) ExtrChannel
					(
					BYTE *hDst,						/* NULL������ */
					const BYTE *pImg,				/* ����Դͼ���� */
					int nWd, int nHi,				/* ���صĿ�͸� */
					int nMo,						/* ��ʽ0=��1=�� */
					int nChn						/* RGBYUV=[0,5] */
					);

/*===============================================================*/

/* ���RGB24�Ĳ�ɫͼ���ڴ棬����R��B����ɫֵ˳�� */
SPF_API(void) SwapDataR_B
					(
					BYTE *pRgb,						/* ��ɫ24ͼ���� */
					int nWd, int nHi,				/* ���صĿ�͸� */
					int nMo							/* ��ʽ0=��1=�� */
					);

/* �Ӵ�ͼ���ó�Сͼ(��ͼ��ʽ���Ҷȣ�8Bit���ʣ�RGB24����ӡ) */
SPF_API(BYTE *) CropRectImg
					(
					BYTE *pCrp,						/* ������ͼ�� */
					int nSlf, int nStp,				/* ���������Ͻ� */
					int nDwd, int nDhi,				/* ���ó�ͼ��� */
					const BYTE *pImg,				/* Դ��������ͼ */
					int nSwd, int nShi,				/* ��ͼ��߿ɸ� */
					int nTyp,						/* 0G,1RGB,2BGR */
					int nFill/* = -1*/				/* ����ɫ����ͼ */
					);

/* ��Դͼ��任�����ȿ�ߵ���ͼ�񣬳�����DPI�任 */
SPF_API(void *) ScaleGryRgb
					(
					const void *hSimg,				/* Դʼͼ��ָ�� */
					int nSwd, int nShi,				/* Դʼͼ�Ŀ�� */
					void *hDimg,					/* Ŀ��ͼ��ָ�� */
					int nDwd, int nDhi,				/* Ŀ��ͼ�Ŀ�� */
					int nCmd						/* ��ʽ0=��1=�� */
					);

/* �������Ĳ�ɫ24λͼת�ɻҶ�ͼ(��!pGray��!nTypʱ����!bNew����pImg) */
SPF_API(BYTE *) RgbCnvtGray
					(
					BYTE *pGray,					/* �Ҷ�ͼ��ָ�� */
					const BYTE *pImg,				/* ��ɫͼ��ָ�� */
					int nWd, int nHi,				/* ���صĿ�͸� */
					int nTyp,						/* 0G,1RGB,2BGR */
					int bNewGry/* = 0*/				/* !pGrayʱ���� */
					);

/* ����˳ʱ��[0, 360)�Ǻ����ģ���ת���ڵȴ�Ŀ���������ױ��� */
SPF_API(BYTE *) RotateImage
					(
					BYTE *pRot,						/* �����תͼ�� */
					const BYTE *pImg,				/* Դͼ�������� */
					int nWd, int nHi,				/* Դͼ�Ŀ�͸� */
					int nMod,						/* 0=��ɫ1=�Ҷ� */
					int nAgl,						/* ˳ʱ��ת�Ƕ� */
					int nCx, int nCy				/* ��ת����λ�� */
					);

/*===============================================================*/

/* ��ͼ��ɫ������ÿ����ɫͨ������ƬЧ�������Ƿ����޸� */
SPF_API(BOOL) ImageDoRvrs
					(
					BYTE *pImg,						/* ���޸ĵ�ͼ�� */
					int iWd, int iHi,				/* ͼ��Ŀ�͸� */
					int iMd							/* ��ʽ0=��1=�� */
					);

/* ��ͼ�����·�ת������ӡ�ͷ���ӡ��ת�������Ƿ����޸� */
SPF_API(BOOL) ImageDoUpdw
					(
					BYTE *pImg,						/* ���޸ĵ�ͼ�� */
					int iWd, int iHi,				/* ͼ��Ŀ�͸� */
					int iMd							/* ��ʽ0=��1=�� */
					);

/* ��ͼ�����ҷ�ת���ھ���ͷǾ����ת�������Ƿ����޸� */
SPF_API(BOOL) ImageDoLfrt
					(
					BYTE *pImg,						/* ���޸ĵ�ͼ�� */
					int iWd, int iHi,				/* ͼ��Ŀ�͸� */
					int iMd							/* ��ʽ0=��1=�� */
					);

/* ͼ����ת90��(˳ʱ��1����ʱ��-1)���������ݣ����Ƿ����޸� */
SPF_API(BOOL) ImageDoRota
					(
					BYTE **hImg,					/* ���޸ĵ�ͼ�� */
					int *pWd, int *pHi,				/* ͼ��Ŀ�͸� */
					int iMd,						/* ��ʽ0=��1=�� */
					int iDir						/* ˳+��-��λ90 */
					);

/* ��ͼ�������Ŀ�ȵȱ����������������ݣ����Ƿ����޸� */
SPF_API(BOOL) ImageDoZoom
					(
					BYTE **hImg,					/* ���޸ĵ�ͼ�� */
					int *pWd, int *pHi,				/* ͼ��Ŀ�͸� */
					int iMd,						/* ��ʽ0=��1=�� */
					int iDw							/* ������Ŀ��� */
					);

/* �ҶȻ���ͼ��Ŀ����(0������G���ɣ�1��ԴRGB��ʽ��2��ԴBGR��ʽ) */
SPF_API(BOOL) ImageDoGray
					(
					BYTE **hImg,					/* ���޸ĵ�ͼ�� */
					int nWd, int nHi,				/* ���صĿ�͸� */
					int *pMd,						/* ��ʽ0=��1=�� */
					int nTyp						/* 0G,1RGB,2BGR */
					);

/*=///////////////////////////////////////////////////////////////////////=*/
/*= ֧�ֶ�ͼ���ʽ�ļ��Ķ�ȡ���أ��������ɣ��Լ������ڴ��е�ͼ��          =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* ͼ���ʽת����ɫ��(0=RGB24��1=�̻�8��2=����8)��JPG[1, 100] */
SPF_API(BOOL) FmtImgSavAs
					(
					LPCTSTR chSrc,					/* Դʼͼ���ļ� */
					LPCTSTR chDst,					/* Ŀ��ͼ���ļ� */
					int nGmd,						/* �ֱ�����ģʽ */
					int nQty/* = 0*/				/* JPEG������� */
					);

/* ����ͼ�����ݣ�ɫ��(-1=Origin, 0=RGB24��1=�̻�8��2=����8) */
SPF_API(BOOL) LoadImgFile
					(
					LPCTSTR chImg,					/* Դʼͼ���ļ� */
					BYTE **hBuf,					/* ͼָ��ĵ�ַ */
					int *pWide, int *pHigh,			/* ԭͼ�Ŀ�͸� */
					int *pCmo,						/* ��ʽ0=��1=�� */
					int *pDpi, int nGmd				/* �ֱ�����ģʽ */
					);

/* ����ͼ�����ݣ�ɫ��(0=RGB24��1=�̻�8��2=����8)��JPG[1, 100] */
SPF_API(BOOL) SaveImgFile
					(
					LPCTSTR chImg,					/* Ŀ��ͼ���ļ� */
					const BYTE *hBuf,				/* ͼ���ڴ����� */
					int nWide, int nHigh,			/* ԭͼ�Ŀ�͸� */
					int nCmo,						/* ��ʽ0=��1=�� */
					int nDpi, int nGmd,				/* �ֱ�����ģʽ */
					int nQty/* = 0*/				/* JPEG������� */
					);

/*===============================================================*/

/* ����ͼ�����ݣ�ɫ��(-1=Origin, 0=RGB24��1=�̻�8��2=����8) */
SPF_API(BOOL) LoadImgBuff
					(
					BYTE **hImg,					/* ͼָ��ĵ�ַ */
					int *pWide, int *pHigh,			/* ԭͼ�Ŀ�͸� */
					int *pCmo,						/* ��ʽ0=��1=�� */
					int *pDpi, int nGmd,			/* �ֱ�����ģʽ */
					const BYTE *hBuf,				/* ͼ���ڴ����� */
					int nBsz						/* ���ڴ�Ĵ�С */
					);

/* ����ͼ�����ݣ�ɫ��(0=RGB24��1=�̻�8��2=����8)��JPG[1, 100] */
SPF_API(int) SaveImgBuff
					(
					const BYTE *hImg,				/* ͼ���ڴ����� */
					int nWide, int nHigh,			/* ԭͼ�Ŀ�͸� */
					int nCmo,						/* ��ʽ0=��1=�� */
					int nDpi, int nGmd,				/* �ֱ�����ģʽ */
					int nQty,						/* JPEG������� */
					BYTE **hBuf,					/* ����Ŀ��ͼ�� */
					int *pBsz,						/* ��ͼ���Ĵ�С */
					int nFmt						/* 2-JPG, 4-PNG */
					);

/*=///////////////////////////////////////////////////////////////////////=*/

#ifdef __cplusplus
}
#endif

#endif /* _SCK007_TESOPICSLIB_INCLUDED_PENTIUMWORKINGROOM_ZNN008_ */

/*=///////////////////////////////////////////////////////////////////////=*/
/*= The end of this file. =*/
