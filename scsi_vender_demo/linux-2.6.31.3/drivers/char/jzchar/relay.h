#ifndef __WIEGAND_H__
#define __WIEGAND_H__


#include <asm/jzsoc.h>

#define REALY_MAJOR     53

#define IOCTL_RELAY_SET0		0
#define IOCTL_RELAY_SET1		1

#define GPIO_PINX_RELAY0		( 32 * 1 + 20 ) // modify, yjt, 20130821, PB20, IO_ALARM
#define GPIO_PINX_RELAY1		( 32 * 1 + 31 ) // modify, yjt, 20130821, PA0 -> PE2 -> PB31, IO_LOCKER


#endif  // __WIEGAND_H__
