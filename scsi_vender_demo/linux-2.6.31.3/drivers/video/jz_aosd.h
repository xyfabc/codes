/*
 * linux/drivers/video/jz_aosd.h -- Ingenic  On-Chip AOSD device
 *
 * Copyright (C) 2005-2008, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __JZ_AOSD_H__
#define __JZ_AOSD_H__

struct jz_aosd_info {
	unsigned long raddr;	/* 64Words aligned at least */
	unsigned long waddr;	/* 64Words aligned at least */

	__u32         addr_len;
	__u32         compress_done;
	__u32         bpp;
	__u32         is24c;
	__u32         is64unit; 	/* meanless */
	__u32         height;		/* in lines */
	__u32         width;		/* in pixels */
	__u32         src_stride;	/* in bytes, 16Words aligned at least */
	__u32         dst_stride;	/* in bytes, 16Words aligned at least  */
	__u32         buf;
}; 

void jz_compress_set_mode(struct jz_aosd_info*);
void jz_start_compress(void);
void print_aosd_registers(void);
int jz_aosd_init(void);
#endif /* __JZ_AOSD_H__ */
