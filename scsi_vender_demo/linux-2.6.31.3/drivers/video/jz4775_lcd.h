/*
 * linux/drivers/video/jz_lcd.h -- Ingenic Jz On-Chip LCD frame buffer device
 *
 * Copyright (C) 2005-2008, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __JZ_LCD_H__
#define __JZ_LCD_H__

//#include <asm/io.h>


#define NR_PALETTE		(256)
#define PALETTE_SIZE	(NR_PALETTE * 2)

enum DISP_PANEL_TYPE
{
	DISP_PANEL_TYPE_LCD,
	DISP_PANEL_TYPE_SLCD,
	DISP_PANEL_TYPE_TVE
};

/* use new descriptor(8 words) */
struct jz_lcd_dma_desc {
	unsigned int next_desc; 	/* LCDDAx */
	unsigned int databuf;   	/* LCDSAx */
	unsigned int frame_id;  	/* LCDFIDx */
	unsigned int cmd; 		/* LCDCMDx */
	unsigned int offsize;       	/* Stride Offsize(in word) */
	unsigned int page_width; 	/* Stride Pagewidth(in word) */
	unsigned int cmd_num; 		/* Command Number(for SLCD) and CPOS(except SLCD) */
	unsigned int desc_size; 	/* Foreground Size and alpha value*/
};

struct jzlcd_panel_t {
	unsigned int cfg;	/* panel mode and pin usage etc. */
	unsigned int slcd_cfg;	/* Smart lcd configurations */
	unsigned int ctrl;	/* lcd controll register */
	unsigned int w;		/* Panel Width(in pixel) */
	unsigned int h;		/* Panel Height(in line) */
	unsigned int fclk;	/* frame clk */
	unsigned int hsw;	/* hsync width, in pclk */
	unsigned int vsw;	/* vsync width, in line count */
	unsigned int elw;	/* end of line, in pclk */
	unsigned int blw;	/* begin of line, in pclk */
	unsigned int efw;	/* end of frame, in line count */
	unsigned int bfw;	/* begin of frame, in line count */
	
	int type;			/* panel type: LCD, SLCD, TVE ... */
};


struct jzlcd_fg_t {
	int bpp;	/* foreground bpp */
	int x;		/* foreground start position x */
	int y;		/* foreground start position y */
	int w;		/* foreground width */
	int h;		/* foreground height */
};

struct jzlcd_osd_t {
	unsigned int osd_cfg;	/* OSDEN, ALHPAEN, F0EN, F1EN, etc */
	unsigned int osd_ctrl;	/* IPUEN, OSDBPP, etc */
	unsigned int rgb_ctrl;	/* RGB Dummy, RGB sequence, RGB to YUV */
	unsigned int bgcolor0;	/* background color(RGB888) */
	unsigned int bgcolor1;	/* background color(RGB888) */
	unsigned int colorkey0;	/* foreground0's Colorkey enable, Colorkey value */
	unsigned int colorkey1; /* foreground1's Colorkey enable, Colorkey value */
	unsigned int alpha0;	/* ALPHAEN0, alpha value */
	unsigned int alpha1;	/* ALPHAEN1, alpha value */
	unsigned int ipu_restart; /* IPU Restart enable, ipu restart interval time */

	struct jzlcd_fg_t fg0;	/* foreground 0 */
	struct jzlcd_fg_t fg1;	/* foreground 1 */
};

struct jzlcd_info {
	struct jzlcd_panel_t panel;
	struct jzlcd_osd_t osd;
};


/* Jz LCDFB supported I/O controls. */
#define FBIOSETBACKLIGHT	0x4688 /* set back light level */
#define FBIODISPON		0x4689 /* display on */
#define FBIODISPOFF		0x468a /* display off */
#define FBIORESET		0x468b /* lcd reset */
#define FBIOPRINT_REG		0x468c /* print lcd registers(debug) */
#define FBIOROTATE		0x46a0 /* rotated fb */
#define FBIOGETBUFADDRS		0x46a1 /* get buffers addresses */
#define FBIO_GET_MODE		0x46a2 /* get lcd info */
#define FBIO_SET_MODE		0x46a3 /* set osd mode */
#define FBIO_RESET_DESC 	0x46a4 /* set panel and osd mode */
#define FBIO_MODE_SWITCH	0x46a5 /* switch mode between LCD and TVE */
#define FBIO_GET_TVE_MODE	0x46a6 /* get tve info */
#define FBIO_SET_TVE_MODE	0x46a7 /* set tve mode */
#define FBIO_SET_ENH            0x46a8 /* set enh enable or disable*/
#define FBIO_GET_ENH            0x46a9 /* get enh enable*/
#define FBIO_SET_ENH_VALUE      0x46aa /* set enh value*/
#define FBIO_FG1_COMPRESS_EN    0x46ab /* set fg1 compress function enable or disable */
#define FBIO_FG0_COMPRESS_EN    0x46ac /* set fg0 compress function enable or disable */
#define FBIO_FG0_16x16_BLOCK_EN  0x46ad
#define FBIO_FG1_16x16_BLOCK_EN  0x46ae
#define FBIO_SET_FG              0x46af
#define FBIO_GET_COMP_MODE       0x46b0
#define FBIO_SET_COMP_MODE       0x46b1
#define FBIO_DO_REFRESH   	0x468f

/*
 * LCD backlight definition
 */

#define LCD_BACKLIGHT_TYPE_LEVEL	(0)
#define	LCD_BACKLIGHT_TYPE_PWM		(1)
#define	LCD_BACKLIGHT_TYPE_DEFAULT	LCD_BACKLIGHT_TYPE_PWM

void display_panel_init(void *ptr);

#endif /* __JZ_LCD_H__ */
