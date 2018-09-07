/*
 *	drivers/char/jzchar/Wieg.c
 *
 *	Wiegand driver for Hanwang E356A, only support 26bit,34bit mode.
 *
 * AUTHOR: GOLF,  A.D. 2011-08-17
 *
 *	Copyright (C) 2006 ggoollff, WAHA!HA! (^_^)
 *
 *	This program is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU General Public License
 *	as published by the Free Software Foundation; either version
 *	2 of the License, or (at your option) any later version.
 *
 */

#include <linux/utsname.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>

#include <linux/device.h>

#include "relay.h"

#define DEBUG_MESG_GOLF
#undef	DEBUG_MESG_GOLF



static int Relay_Open(struct inode *inode, struct file *file)
{
	int retval1;

#ifdef DEBUG_MESG_GOLF	
	printk("Relay_Open \n");
#endif
	//PC29,PC31
	__gpio_as_output(GPIO_PINX_RELAY0);
	__gpio_as_output(GPIO_PINX_RELAY1);

#if 0
	for(;;)
	{
		__gpio_set_pin(GPIO_PINX_RELAY0);
		//__gpio_set_pin(GPIO_PINX_RELAY1);
		mdelay(3000);
		__gpio_clear_pin(GPIO_PINX_RELAY0);
		//__gpio_clear_pin(GPIO_PINX_RELAY1);
		mdelay(3000);		
	}
#endif
	return 0;
}

static unsigned int Relay_Write(struct file *fp, unsigned int *buf, size_t count)
{
	// kbd_flag = 0;	

	return count;
}

static int Relay_release(struct inode *inode, struct file *file)
{
	return 0;
}

static int Relay_Ioctl(struct inode *inode, struct file *file, unsigned int cmd, unsigned long arg)
{
	unsigned int relayX = arg; 
	
#if 0
	printk("relayX: %d\n", relayX);
#endif

#if 0
// yjt, add, 20130516
#if 1
	relayX = GPIO_PINX_RELAY1;		
#endif
// 历史原因，将开锁PIN号放入了应用层，导致硬件有改动时，需要修改应用层来匹配
// 因为A11系列机器只有一个开门继电器，ALARM继电器已经取消
// 所以在这里强制将relayX 赋值为 GPIO_PINX_RELAY1，用于控制门锁
#else
	if(relayX == 131)		// 历史原因, PIN 131是开门PIN, yjt, 20130821
		relayX = GPIO_PINX_RELAY1;
	else
		relayX = GPIO_PINX_RELAY0;
#endif

	if((GPIO_PINX_RELAY0 != relayX) && (GPIO_PINX_RELAY1 != relayX)){
		printk("BAD RELAY NUMBER:%d\n",relayX);
		return 0;
	}

	switch(cmd)
	{
		case IOCTL_RELAY_SET0:
		{	
			__gpio_clear_pin(relayX);

			break;
		}
		case IOCTL_RELAY_SET1:
		{
			__gpio_set_pin(relayX);
			break;
		}
		default:
			printk("Error cmd. \n\n");
	}
	
    return 0;
}
 
struct file_operations Relay_fops = 
{
	.open 		= Relay_Open, 
	.ioctl 		= Relay_Ioctl, 
	.write		= Relay_Write,
	.release 	= Relay_release,
};

struct class *rly_class;

static int Relay_create_device(int major_num, char *dev_name, char *class_name)
{
	rly_class = class_create(THIS_MODULE, class_name);
	device_create(rly_class, NULL, MKDEV(major_num, 0), dev_name, "relay", 0);

	return 0;
}

int __init HW_Relay_Init(void)
{
	int result;
	
	result = register_chrdev(REALY_MAJOR, "relay", &Relay_fops);
	if(result<0)
	{
		printk("\nFALLED: Cannot register relay_driver! \n");
		
		return result;
	}
	else
	{
		printk("Register relay_driver OK!\n");
	}
	
	Relay_create_device(REALY_MAJOR, "relay", "class_rly");

	return 0;
}

void __exit HW_Relay_Exit(void)
{
	unregister_chrdev(REALY_MAJOR, "relay");
	device_destroy(rly_class, MKDEV(REALY_MAJOR, 0));
	class_destroy(rly_class);
}

module_init(HW_Relay_Init);
module_exit(HW_Relay_Exit);

MODULE_AUTHOR("golf here <golf@163.com>");
MODULE_DESCRIPTION("wiegand Driver");
MODULE_LICENSE("GPL");

