/*
 * linux/drivers/video/jz_aosd.c -- Ingenic Jz LCD frame buffer device
 *
 * Copyright (C) 2005-2008, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 *  You should have received a copy of the  GNU General Public License along
 *  with this program; if not, write  to the Free Software Foundation, Inc.,
 *  675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/string.h>
#include <linux/mm.h>
#include <linux/tty.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/fb.h>
#include <linux/init.h>
#include <linux/dma-mapping.h>
#include <linux/platform_device.h>
#include <linux/pm.h>

#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/processor.h>
#include <asm/jzsoc.h>

#include "jz_aosd.h"

MODULE_DESCRIPTION("Jz ALPHA OSD driver");
MODULE_AUTHOR("lltang, <lltang@ingenic.cn>");
MODULE_LICENSE("GPL");

#define D(fmt, args...) \
//	printk(KERN_ERR "%s(): "fmt"\n", __func__, ##args)

#define E(fmt, args...) \
	printk(KERN_ERR "%s(): "fmt"\n", __func__, ##args)

extern struct jz_aosd_info *aosd_info;
extern wait_queue_head_t compress_wq;

int irq_flag = 0;

/* The following routine is only for test */

void print_aosd_registers(void)	/* debug */
{
	/* Compress Resgisters */
	printk("REG_COMP_CTRL:\t0x%08x\n", REG_COMP_CTRL);
	printk("REG_COMP_CFG:\t0x%08x\n", REG_COMP_CFG);
	printk("REG_COMP_STATE:\t0x%08x\n", REG_COMP_STATE);
	printk("REG_COMP_FWDCNT:\t0x%08x\n", REG_COMP_FWDCNT);
	printk("REG_COMP_WOFFS:\t0x%08x\n", REG_COMP_WOFFS);
	printk("REG_COMP_WCMD:\t0x%08x\n", REG_COMP_WCMD);
	printk("REG_COMP_WADDR:\t0x%08x\n", REG_COMP_WADDR);
	printk("REG_COMP_ROFFS:\t0x%08x\n", REG_COMP_ROFFS);
	printk("REG_COMP_RSIZE:\t0x%08x\n", REG_COMP_RSIZE);
	printk("REG_COMP_RADDR:\t0x%08x\n", REG_COMP_RADDR);

	printk("REG_COMP_WADDR:\t0x%08x\n", REG_COMP_WADDR);
	printk("==================================\n");

}

void jz_start_compress(void)
{
	aosd_info->compress_done = 0;
	compress_start();
}

void jz_compress_set_mode(struct jz_aosd_info *info)
{
	unsigned int width, height;

//	printk("<< aosd: addr0:%x, waddr:%x, width:%d, length:%d, with_alpha:%d >>\n", info->addr0, info->waddr, info->width, info->height, info->with_alpha);
	width = info->width - 1;
	height= info->height - 1;

        /*SET SCR AND DES ADDR*/
	compress_set_raddr(info->raddr);
	compress_set_waddr(info->waddr);

	/*SET DST OFFSIZE */
	compress_set_doffs(info->dst_stride);
	/*SET SCR OFFSET*/
	compress_set_roffs(info->src_stride);

	/*SET SIZE*/
	// frame's actual size, no need to align,pixel number
	compress_set_size(width, height);

	if(!aosd_info->is64unit)
		compress_set_128unit();
	else
		compress_set_64unit();

	if(aosd_info->bpp == 24){
		compress_set_bpp24();

		if(aosd_info->is24c)
			compress_to_24c();
		else
			compress_to_24();
	}else{
		compress_set_bpp16();
		compress_to_16();
	}

	print_aosd_registers();
}


static irqreturn_t jz_interrupt_handler(int irq, void *dev_id)
{
	if (compress_get_ad()){
		printk("compress end\n");
		aosd_info->compress_done = 1;
		compress_set_raddr(aosd_info->raddr); 
		printk("REG_COMP_WADDR:%x\n",REG_COMP_WADDR);
		printk("REG_COMP_RADDR:%x\n",REG_COMP_RADDR);
		compress_clear_ad();
		wake_up(&compress_wq);
	}else{
		printk("REG_COMP_STATE:%x,compress_get_ad():%d\n",REG_COMP_STATE,compress_get_ad());
	}


	return IRQ_HANDLED;
}

int jz_aosd_init(void)
{
	int ret,irq;

	init_waitqueue_head(&compress_wq);

	/* aosd and compress use the same IRQ number */
	if (request_irq(IRQ_AOSD, jz_interrupt_handler, IRQF_DISABLED,"oasd_compress", 0)) {
		printk("Faield to request ALPHA OSD IRQ.\n");
		ret = -EBUSY;
		goto failed;
	}

	compress_mask_ad(); /* enable compress and enable finished int */
	return 0;

failed:
	free_irq(IRQ_AOSD,0);
	return ret;
}

