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
#define ERR_FAIL_WIGGINS_INIT				-14 	// Τ���źų�ʼ��ʧ��
#define ERR_FAIL_KEY_INIT                          -15 	// ������ʼ��ʧ��
#define ERR_FAIL_AUDIO_INIT						-16 	// ��Ƶ��ʼ��ʧ��
#define ERR_FAIL_BATTERY_INIT					-17 	// ��ع����ʼ��ʧ��
#define ERR_FAIL_COM0_INIT						-18 	// ��ӡ���ڳ�ʼ��ʧ��
#define ERR_FAIL_COM1_INIT						-19 	// ��������ڳ�ʼ��ʧ��
#define ERR_FAIL_COM2_INIT						-20 	// ID�����ڳ�ʼ��ʧ��
#define ERR_FAIL_CAMERA_INIT					-21 	// camera init err
#define ERR_FAIL_COM_INIT						-22 	// ���ڳ�ʼ��ʧ��
#define ERR_FAIL_GPIO_INIT						-23 	// GPIO ��ʼ��ʧ��

#endif
