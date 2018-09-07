#ifndef __IC_ID_CARD_H__
#define __IC_ID_CARD_H__

#include <linux/ioctl.h>

#define IC_ID_CARD_IO_MAGIC		0xde

#define IC_ID_CARD_REQUEST_CARD_START	_IO(IC_ID_CARD_IO_MAGIC, 0)
#define IC_ID_CARD_REQUEST_CARD_STOP	_IO(IC_ID_CARD_IO_MAGIC, 1)
#define IC_ID_CARD_GET_CARD_ID			_IOR(IC_ID_CARD_IO_MAGIC, 2, unsigned int)
#define IC_ID_CARD_READ_CARD				_IO(IC_ID_CARD_IO_MAGIC, 3)

#endif /* __IC_ID_CARD_H__ */

