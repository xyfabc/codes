//
// Copyright (c) Ingenic Semiconductor Co., Ltd. 2008.
//

#ifndef __OV9650SET_H__
#define __OV9650SET_H__

#include <linux/i2c.h>

void init_set(struct i2c_client *client);
void preview_set(struct i2c_client *client);
void capture_set(struct i2c_client *client);


#endif //__OV9650SET_H__
