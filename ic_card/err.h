/*
@name: err.h
@func: error code definition

create at 2017.05.12 by Eric.Xi


*/


#ifndef __ERR_H__
#define __ERR_H__



#define HW_DEBUG
#ifdef HW_DEBUG
#define DBG_PRINTF(s,...)			printf(s, ##__VA_ARGS__)
#else
#define DBG_PRINTF(s,...) 
#endif



#define Uint32	unsigned int
#define Uint8	unsigned char


#define		CMD_SET_ANNTENA			0x010C
#define		CMD_REQUEST_CARD			0x0201
#define		CMD_ANTI_COLLISION			0x0202
#define		CMD_SELECT					0x0203




/* project error number */
#define ERR_OK 						0 	// æˆåŠŸ
#define ERR_FAIL 					-1 	// å¤±è´¥
#define ERR_TIMEOUT 				-2 	// è¶…æ—¶
#define ERR_PARA 					-3 	// å‚æ•°é”™
#define ERR_FILE					-4	// æ–‡ä»¶æ“ä½œå¤±è´¥
#define ERR_MEM						-5	// å†…å­˜åˆ†é…å¤±è´¥
#define ERR_NOT_MATCH				-6	// ç‰¹å¾æ¯”å¯¹ä¸åŒ¹é…
#define ERR_CRC						-7  //æ ¡éªŒé”™è¯¯
#define ERR_CMD						-8  //ä¸æ”¯æŒå‘½ä»¤
#define ERR_DISCONNECT     			-9  //è¿æ¥æ–­å¼€
#define ERR_NO_USER     			-10  //æœªæ³¨å†Œç”¨æˆ·
#define ERR_FAIL_WIGGINS_INIT				-14 	// Î¤¸ùĞÅºÅ³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_KEY_INIT                          -15 	// °´¼ü³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_AUDIO_INIT						-16 	// ÒôÆµ³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_BATTERY_INIT					-17 	// µç³Ø¹ÜÀí³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_COM0_INIT						-18 	// ´òÓ¡´®¿Ú³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_COM1_INIT						-19 	// ²â¾àµç»ú´®¿Ú³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_COM2_INIT						-20 	// ID¿¨´®¿Ú³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_CAMERA_INIT					-21 	// camera init err
#define ERR_FAIL_COM_INIT						-22 	// ´®¿Ú³õÊ¼»¯Ê§°Ü
#define ERR_FAIL_GPIO_INIT						-23 	// GPIO ³õÊ¼»¯Ê§°Ü

#endif
