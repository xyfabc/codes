/*
@ name: laserx.h
@ func: about laserx protocal process lib api.

create at 2014-06-05 17:15, by Techshino S.Y.C

*/


#ifndef __LASERX_H__
#define __LASERX_H__

#include "zmq.h"

#ifdef __cplusplus
extern "C" {
#endif



#ifdef __GUNC__
/* linux */
#define EXPORT

#elif defined(WIN32)
/* windows */
#ifdef _USRDLL
/* bulid as dll */
#define EXPORT						_declspec(dllexport)
#else
#define EXPORT						
#endif

#else
#define EXPORT
#endif


/* frame type Binary */ 
#define LX_FORMAT_BIN				'B'

/* frame type Split */  
#define LX_FORMAT_SPT				'S'

/* frame type detect, a particular useage */
#define LX_FORMAT_DET				'?'	 


typedef unsigned int				u32;
typedef unsigned char				u8;

#ifndef __cplusplus
typedef int							bool;
#endif


/* boolean value */
#ifndef TRUE
#define TRUE						1
#endif

#ifndef FALSE
#define FALSE						0
#endif



/* project error number */
#define ERR_OK 						0 	// 成功
#define ERR_FAIL 					-1 	// 失败
#define ERR_TIMEOUT 				-2 	// 超时
#define ERR_PARA 					-3 	// 参数错
#define ERR_FILE					-4	// 文件操作失败
#define ERR_MEM						-5	// 内存分配失败
#define ERR_NOT_MATCH				-6	// 特征比对不匹配
#define ERR_CRC						-7  //校验错误
#define ERR_CMD						-8  //不支持命令
#define ERR_DISCONNECT     			-9  //连接断开
#define ERR_NO_USER     			-10  //未注册用户
#define ERR_NO_FEATURE     			-11  //获取特征失败，无法获取特征
#define ERR_PARSE					-12  //协议解析失败，底层收发没问题
#define ERR_CMD_UNM					-13  //send cmd != rec cmd, unmatch

/* io callback function */
typedef struct {
    int (*send)(int dev, void *data, int len);
    int (*recv)(int dev, void *data, int len);
    void (*clear)(int dev);
} cmd_callback_t;

/* some runtime para */
typedef struct
{
	u32 pos;      	/* where is parsing right now */
	u32 frame_len;	/* the len domain in frame */
	u32 total_len;	/* frame totle length include (HDR FMT CMD LEN DATA CHK) */
	int nibble;
	u8 cmd;
	u8 half_byte;
	u8 sum;
	u8 format;
} parse_para_t;


/* laserx frame struct */
typedef struct
{
	u8 cmd;			
	u8 format;		
	u8 ret;			
	u8 *data;		
	u32 len;		
} laserx_frame_t;



typedef struct
{
	int dev;		/* device descriptor or handle */
    void *s;
    int zmq_flags;
	/* send callback function, return < 0 if some error occur */
	int (*send)(int dev, void *data, int len);
	/* receive callback function, return < 0 if some error occur */
	int (*recv)(int dev, void *data, int len);


    int (*zmq_send) (void *s, const void *buf, size_t len, int flags);
    int (*zmq_recv) (void *s, void *buf, size_t len, int flags);
} laserx_callback_t;




/* laserx_deframe, check all domain out. if the frame is split format,
trans it to bin format first, then check it out;
@data: point to the frame
@len: the frame totle length
@ack: TRUE make a ack frame, FALSE make a cmd frame
@frame: return the frame domain info */
EXPORT void laserx_deframe(u8 *data, u32 len, bool ack, laserx_frame_t *frame);


/* laserx_parse, parse the recieved data. when return <0, para must be clear.
 * @data: one byte data
 * @para: point to the parse para, it is needed in the parse process
 * @return: 0 not end, 1 parse ok, <0 fail */
EXPORT int laserx_parse(u8 data, parse_para_t *para, bool ack);


/* laserx_enframe, build a laserx frame.
@buf: store the frame builded
@size: the size of the frame
@ack: TRUE make a ack frame, FALSE make a cmd frame
@return: 0 success, <0 fail */
EXPORT int laserx_enframe(laserx_frame_t *frame, u8 *buf, u32 *size, bool ack);


/* laserx_send, send the frame according to the laserx frame arch
@frame: the frame info
@ack: TRUE as a ack frame, FALSE as a cmd frame
@cb: the callback function, actual do the send */
EXPORT int laserx_send(laserx_frame_t *frame, bool ack, laserx_callback_t *cb);

EXPORT int laserx_zmq_send(laserx_frame_t *frame, bool ack, laserx_callback_t *cb);


/* laserx_recv, receive data and parse it. if have received a good frame, return
the frame architecture. if the frame is split format, it will be transed to bin format.
@buf: the buffer to receive data
@size: the buffer max size
@ack: TRUE as a ack frame, FALSE as a cmd frame
@cb: the callback function, actual do the receive
@frame: return the frame arch in the buffer */
EXPORT int laserx_recv(u8 *buf, u32 size, bool ack, 
					  laserx_callback_t *cb, laserx_frame_t *frame);


/* the lib build info */
EXPORT void laserx_info(char buf[64]);


EXPORT int cmd_ack(int fd, int cmd, int err, void *data, int len);

#ifdef __cplusplus
}
#endif

#endif


//-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-* FILE  END *-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*-*//
