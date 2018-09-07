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
/*= 本库的接口类型，及返回的错误码定义，>=0时为成功，可表示特定意义。     =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* 定义制作方式有：UNIX/LINUX下的.o、Windows下的DLL，Windows下的LIB */
#if defined(__GNUC__) && /* GCC至少是4版本 */ \
        ((__GNUC__ >= 4) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3))
#define SPF_API(__type) __attribute__ ((visibility("default"))) __type
#else
#define SPF_API(__type) __type __stdcall
#endif

/*===============================================================*/

/* 针对需要关切数据类型大小的场合定义使用 */
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

/* Unix或Linux下时，需要的变量类型，及部分宏定义 */
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

#else /* 在Windows环境下编译 */

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

#define LR_BY(w)		HI_BY(w)					/* Host低字节 */
#define HR_BY(w)		LO_BY(w)					/* Host高字节 */
#define MR_WD(h, l)		MK_WD(l, h)					/* Host字 */

#define SwrP(w)		((((w) << 8) & 0xFF00) | (((w) >> 8) & 0xFF))
#define SwaP(w)		(WORD)SwrP(w)
#define SdwP(dw)	((SwrP(dw) << 16) | SwrP((dw) >> 16))

#define MeSwaP(w)		w = SwaP(w)					/* 翻倒WORD字 */
#define MeSdwP(dw)		dw = SdwP(dw)				/* 翻倒DW双字 */

#define min(a, b)		(((a) < (b)) ? (a) : (b))	/* 求最小值 */
#define max(a, b)		(((a) > (b)) ? (a) : (b))	/* 求最大值 */

#define lmtMin(v, th)	if((v) < (th)) (v) = (th)	/* 限定最小值 */
#define lmtMax(v, th)	if((v) > (th)) (v) = (th)	/* 限定最大值 */
#define lmtLmt(v, n, x)	lmtMin(v, n); lmtMax(v, x)	/* 限定区间值 */

#define setBit(v, b)	(v) |= (1 << (b))			/* 置1指定bit */
#define clrBit(v, b)	(v) &= (~(1 << (b)))		/* 清0指定bit */
#define getBit(v, b)	(((v) & (1 << (b))) != 0)	/* 取?指定bit */

/*===============================================================*/

#define SureValAscnd(va, vb, tv) /* 按从小->大的排序 */ \
{ \
	if((va) > (vb)) \
	{ \
		tv tmp = (va); (va) = (vb); (vb) = tmp; \
	} \
}

#define SureValDscnd(va, vb, tv) /* 按从大->小的排序 */ \
{ \
	if((va) < (vb)) \
	{ \
		tv tmp = (va); (va) = (vb); (vb) = tmp; \
	} \
}

/*===============================================================*/

U16  PtrGet16(U8 **q);								/* 小端读02字节 */
U32  PtrGet32(U8 **q);								/* 小端读04字节 */
void PtrSet16(U8 **q, U16 u);						/* 小端写02字节 */
void PtrSet32(U8 **q, U32 u);						/* 小端写04字节 */

/*===============================================================*/

#ifndef RGB					/* 定义RGB的色彩值 */
typedef UINT				COLORREF;
typedef UINT				*LPCOLORREF;
#define RGB(r,g,b)			/* 生成0-B-G-R颜色 */ \
((COLORREF)(((BYTE)(r)|((WORD)((BYTE)(g))<<8))|(((UINT)(BYTE)(b))<<16)))
#endif /* #ifndef RGB */

/*===============================================================*/

#define WSQ_COF				10000					/* WSQ_倍乘系数 */
#define WSQ_TXT				"TechShino"				/* WSQ_厂商字串 */

enum {PIC_NON, PIC_BMP, PIC_JPG, PIC_TIF, PIC_PNG,	/* 支持图像格式 */
		PIC_WSQ, PIC_JP2, PIC_JPC, PIC_J2K, PIC_J2C, PIC_TFF};
#define PICS_MAXI			10						/* 最多支持数量 */

static LPCTSTR PICS_FILTER = "All Files(*.BmP,*.JpG,*.TiF,*.PnG,"
	"*.WsQ,*.Jp2,*.JpC,*.J2K,*.J2C,*.TiFF)|*.BmP;*.JpG;*.TiF;*.PnG;"
	"*.WsQ;*.Jp2;*.JpC;*.J2K;*.J2C;*.TiFF||";		/* 对话框过滤器 */
static LPCTSTR PICS_EXNM[PICS_MAXI] = {".BmP",		/* 扩展名字符串 */
	".JpG",".TiF",".PnG",".WsQ",".Jp2",".JpC",".J2K",".J2C",".TiFF"};
static LPCTSTR SUPT_PICS = "BmP;JpG;TiF;PnG;WsQ;Jp2;JpC;J2K;J2C;TiFF";

/*=///////////////////////////////////////////////////////////////////////=*/
/*= CRC32校验码，Base64编解码，1D和2D内存的分配和释放等功能               =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* 给定一个数据区及其长度，求出它的CRC32检验码 */
SPF_API(UINT) GeneraCrc32
					(
					UINT dwCrc,						/* 初始的校验值 */
					const void *hBuf,				/* 待校验数据区 */
					UINT dwLen						/* 数据区的长度 */
					);

/* B64编码：作Bin=>B64转换，编码后返回不含'\0'的长度 */
SPF_API(int) EnCodeB64
					(
					void *pVdst,					/* 输出缓冲内存 */
					const void *pVsrc,				/* 输入0=求长度 */
					int nSlen/* = 0*/				/* 负返回无'\0' */
					);

/* B64解码：作Bin<=B64转换，解码后返回有效的目标长度 */
SPF_API(int) DeCodeB64
					(
					void *pVdst,					/* 输出0=求长度 */
					const void *pVsrc,				/* 输入数据缓冲 */
					int nSlen/* = 0*/				/* 输入区的长度 */
					);

/* 解压=0须自知输出长，压缩(1最快,6中档,9最好)，返回0失败 */
SPF_API(UINT) ZipBinInMem
					(
					int nMod,						/* 0=解压!0压缩 */
					void *hDst,						/* 输出缓冲内存 */
					UINT uDlen,						/* 输出缓冲大小 */
					const void *hSrc,				/* 输入数据缓冲 */
					UINT uSlen						/* 输入区的长度 */
					);

/*===============================================================*/

/* 申请二维数组空间，给定列行数，单元字节大小 */
SPF_API(void **) TesoNewMo2D
					(
					int iCol, int iRow,				/* 列数和行数 */
					int iItm						/* 步进字节量 */
					);

/* 释放二维数组空间，并置此指针原址为NULL无效 */
SPF_API(void) TesoDelMo2D
					(
					void ***pPtr					/* 二维指针组 */
					);

/*===============================================================*/

/* 确认内存为给定的大小，若原有且不等则重新分配 */
SPF_API(BOOL) TesoNewMemo
					(
					BYTE **hMem,					/* 内存指针地址 */
					int *poSz,						/* 原来内存大小 */
					int nwSz						/* 需新内存大小 */
					);

/* 释放给定的指针指向的堆上内存，并置此指针为NULL空无效 */
SPF_API(void) TesoDelMemo
					(
					BYTE **hMem						/* 内存指针地址 */
					);

/* 确认内存为给定的大小，若原有不足够大则重新分配 */
SPF_API(BOOL) TesoMemSure
					(
					BYTE **hMem,					/* 内存指针地址 */
					int *poSz,						/* 原来内存大小 */
					int nwSz						/* 需新内存大小 */
					);

/*===============================================================*/

/* 确认内存为新的宽高色彩所需要大小，0=RGB24，1=灰度8 */
SPF_API(BOOL) SureImgMemo
					(
					BYTE **hBuf,					/* 图像指针地址 */
					int *pOwd, int *pOhi,			/* 原图的宽和高 */
					int *pOmd,						/* 原图0=彩1=灰 */
					int nNwd, int nNhi,				/* 需要的宽和高 */
					int nNmd						/* 需要0=彩1=灰 */
					);

/* 释放给定指向的图像堆上内存，并置此指针为空NULL无效 */
SPF_API(void) FreeImgMemo
					(
					BYTE **hBuf						/* 图像指针地址 */
					);

/*=///////////////////////////////////////////////////////////////////////=*/
/*= 图像BMP和JPG的加载和保存(含之间转换)，图像数据的剪裁、翻转、变换等    =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* 拷贝指定的图像到等宽高的目标区 */
SPF_API(BYTE *) CopyAnImage
					(
					BYTE *hDst,						/* NULL则返新区 */
					const BYTE *pImg,				/* 输入源图像区 */
					int nWd, int nHi,				/* 像素的宽和高 */
					int nMo							/* 格式0=彩1=灰 */
					);

/* 分别提取图像RGBYUV=[0,5]通道分量图 */
SPF_API(BYTE *) ExtrChannel
					(
					BYTE *hDst,						/* NULL则返新区 */
					const BYTE *pImg,				/* 输入源图像区 */
					int nWd, int nHi,				/* 像素的宽和高 */
					int nMo,						/* 格式0=彩1=灰 */
					int nChn						/* RGBYUV=[0,5] */
					);

/*===============================================================*/

/* 针对RGB24的彩色图像内存，交换R和B两个色值顺序 */
SPF_API(void) SwapDataR_B
					(
					BYTE *pRgb,						/* 彩色24图像区 */
					int nWd, int nHi,				/* 像素的宽和高 */
					int nMo							/* 格式0=彩1=灰 */
					);

/* 从大图剪裁出小图(出图格式：灰度＝8Bit，彩＝RGB24，拓印) */
SPF_API(BYTE *) CropRectImg
					(
					BYTE *pCrp,						/* 另存剪裁图像 */
					int nSlf, int nStp,				/* 剪裁区左上角 */
					int nDwd, int nDhi,				/* 剪裁出图宽高 */
					const BYTE *pImg,				/* 源入整幅大图 */
					int nSwd, int nShi,				/* 大图宽高可负 */
					int nTyp,						/* 0G,1RGB,2BGR */
					int nFill/* = -1*/				/* 正填色负内图 */
					);

/* 将源图像变换到不等宽高的新图像，常用于DPI变换 */
SPF_API(void *) ScaleGryRgb
					(
					const void *hSimg,				/* 源始图像指针 */
					int nSwd, int nShi,				/* 源始图的宽高 */
					void *hDimg,					/* 目标图像指针 */
					int nDwd, int nDhi,				/* 目标图的宽高 */
					int nCmd						/* 格式0=彩1=灰 */
					);

/* 将给定的彩色24位图转成灰度图(当!pGray且!nTyp时，若!bNew返回pImg) */
SPF_API(BYTE *) RgbCnvtGray
					(
					BYTE *pGray,					/* 灰度图像指针 */
					const BYTE *pImg,				/* 彩色图区指针 */
					int nWd, int nHi,				/* 像素的宽和高 */
					int nTyp,						/* 0G,1RGB,2BGR */
					int bNewGry/* = 0*/				/* !pGray时分配 */
					);

/* 给定顺时针[0, 360)角和中心，旋转存于等大目标区，填充白背景 */
SPF_API(BYTE *) RotateImage
					(
					BYTE *pRot,						/* 另存旋转图像 */
					const BYTE *pImg,				/* 源图的数据区 */
					int nWd, int nHi,				/* 源图的宽和高 */
					int nMod,						/* 0=彩色1=灰度 */
					int nAgl,						/* 顺时旋转角度 */
					int nCx, int nCy				/* 旋转中心位置 */
					);

/*===============================================================*/

/* 将图像反色，处理每个颜色通道，负片效果，返是否已修改 */
SPF_API(BOOL) ImageDoRvrs
					(
					BYTE *pImg,						/* 待修改的图像 */
					int iWd, int iHi,				/* 图像的宽和高 */
					int iMd							/* 格式0=彩1=灰 */
					);

/* 将图像上下翻转，在拓印和非拓印间转换，返是否已修改 */
SPF_API(BOOL) ImageDoUpdw
					(
					BYTE *pImg,						/* 待修改的图像 */
					int iWd, int iHi,				/* 图像的宽和高 */
					int iMd							/* 格式0=彩1=灰 */
					);

/* 将图像左右翻转，在镜像和非镜像间转换，返是否已修改 */
SPF_API(BOOL) ImageDoLfrt
					(
					BYTE *pImg,						/* 待修改的图像 */
					int iWd, int iHi,				/* 图像的宽和高 */
					int iMd							/* 格式0=彩1=灰 */
					);

/* 图像旋转90度(顺时针1，逆时针-1)，更新数据，返是否已修改 */
SPF_API(BOOL) ImageDoRota
					(
					BYTE **hImg,					/* 待修改的图像 */
					int *pWd, int *pHi,				/* 图像的宽和高 */
					int iMd,						/* 格式0=彩1=灰 */
					int iDir						/* 顺+逆-单位90 */
					);

/* 将图像按期望的宽度等比例放缩，更新数据，返是否已修改 */
SPF_API(BOOL) ImageDoZoom
					(
					BYTE **hImg,					/* 待修改的图像 */
					int *pWd, int *pHi,				/* 图像的宽和高 */
					int iMd,						/* 格式0=彩1=灰 */
					int iDw							/* 期望的目标宽 */
					);

/* 灰度化彩图，目标区(0＝用绿G生成，1＝源RGB格式，2＝源BGR格式) */
SPF_API(BOOL) ImageDoGray
					(
					BYTE **hImg,					/* 待修改的图像 */
					int nWd, int nHi,				/* 像素的宽和高 */
					int *pMd,						/* 格式0=彩1=灰 */
					int nTyp						/* 0G,1RGB,2BGR */
					);

/*=///////////////////////////////////////////////////////////////////////=*/
/*= 支持多图像格式文件的读取加载，保存生成，以及操作内存中的图像          =*/
/*=///////////////////////////////////////////////////////////////////////=*/

/* 图像格式转换，色彩(0=RGB24，1=绿灰8，2=亮度8)，JPG[1, 100] */
SPF_API(BOOL) FmtImgSavAs
					(
					LPCTSTR chSrc,					/* 源始图像文件 */
					LPCTSTR chDst,					/* 目标图像文件 */
					int nGmd,						/* 分辨率新模式 */
					int nQty/* = 0*/				/* JPEG另存质量 */
					);

/* 加载图像数据，色彩(-1=Origin, 0=RGB24，1=绿灰8，2=亮度8) */
SPF_API(BOOL) LoadImgFile
					(
					LPCTSTR chImg,					/* 源始图像文件 */
					BYTE **hBuf,					/* 图指针的地址 */
					int *pWide, int *pHigh,			/* 原图的宽和高 */
					int *pCmo,						/* 格式0=彩1=灰 */
					int *pDpi, int nGmd				/* 分辨率新模式 */
					);

/* 保存图像数据，色彩(0=RGB24，1=绿灰8，2=亮度8)，JPG[1, 100] */
SPF_API(BOOL) SaveImgFile
					(
					LPCTSTR chImg,					/* 目标图像文件 */
					const BYTE *hBuf,				/* 图像内存数据 */
					int nWide, int nHigh,			/* 原图的宽和高 */
					int nCmo,						/* 格式0=彩1=灰 */
					int nDpi, int nGmd,				/* 分辨率新模式 */
					int nQty/* = 0*/				/* JPEG另存质量 */
					);

/*===============================================================*/

/* 加载图像数据，色彩(-1=Origin, 0=RGB24，1=绿灰8，2=亮度8) */
SPF_API(BOOL) LoadImgBuff
					(
					BYTE **hImg,					/* 图指针的地址 */
					int *pWide, int *pHigh,			/* 原图的宽和高 */
					int *pCmo,						/* 格式0=彩1=灰 */
					int *pDpi, int nGmd,			/* 分辨率新模式 */
					const BYTE *hBuf,				/* 图像内存数据 */
					int nBsz						/* 此内存的大小 */
					);

/* 保存图像数据，色彩(0=RGB24，1=绿灰8，2=亮度8)，JPG[1, 100] */
SPF_API(int) SaveImgBuff
					(
					const BYTE *hImg,				/* 图像内存数据 */
					int nWide, int nHigh,			/* 原图的宽和高 */
					int nCmo,						/* 格式0=彩1=灰 */
					int nDpi, int nGmd,				/* 分辨率新模式 */
					int nQty,						/* JPEG另存质量 */
					BYTE **hBuf,					/* 生成目标图区 */
					int *pBsz,						/* 此图区的大小 */
					int nFmt						/* 2-JPG, 4-PNG */
					);

/*=///////////////////////////////////////////////////////////////////////=*/

#ifdef __cplusplus
}
#endif

#endif /* _SCK007_TESOPICSLIB_INCLUDED_PENTIUMWORKINGROOM_ZNN008_ */

/*=///////////////////////////////////////////////////////////////////////=*/
/*= The end of this file. =*/
