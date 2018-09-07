/*
 * linux/drivers/video/jz4780_lcd.c -- Ingenic Jz4780 LCD frame buffer device
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

/*
 * --------------------------------
 * NOTE:
 * This LCD driver support TFT16 TFT32 LCD, not support STN and Special TFT LCD
 * now.
 * 	It seems not necessory to support STN and Special TFT.
 * 	If it's necessary, update this driver in the future.
 * 	<Wolfgang Wang, Jun 10 2008>
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
#include <linux/act8600_power.h>
#include <linux/timer.h>

#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/processor.h>
#include <asm/jzsoc.h>

#include "console/fbcon.h"

#include "jz4780_lcd.h"
#include "jz4760_tve.h"

#ifdef CONFIG_JZ4780_SLCD_KGM701A3_TFT_SPFD5420A
#include "jz_kgm_spfd5420a.h"
#endif

#if defined(CONFIG_JZ_AOSDC)
#include "jz_aosd.h"
#endif

MODULE_DESCRIPTION("Jz4780 LCD Controller driver");
MODULE_AUTHOR("lltang, <lltang@ingenic.cn>");
MODULE_LICENSE("GPL");

static void display_h_color_bar(int w, int h, int bpp, int *ptr); 

#define D(fmt, args...) \
	printk(KERN_ERR "%s(): "fmt"\n", __func__, ##args)

#define E(fmt, args...) \
	printk(KERN_ERR "%s(): "fmt"\n", __func__, ##args)

#define JZ_FB_DEBUG 1
static int lcd_backlight_level = 102;
struct jz4780lcd_info jz4780_lcd_panel = {
#if defined(CONFIG_JZ4780_LCD_TOPPOLY_TD043MGEB1)
	.panel = {
		.cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */
		LCD_CFG_NEWDES | /* 8words descriptor */
		LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
		LCD_CFG_MODE_TFT_24BIT | 	/* output 18bpp */
		LCD_CFG_HSP | 	/* Hsync polarity: active low */
		LCD_CFG_VSP,	/* Vsync polarity: leading edge is falling edge */
		.slcd_cfg = 0,
		.ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_64,	/* 16words burst, enable out FIFO underrun irq */
		800, 480, 60, 1, 1, 40, 215, 10, 34,
	},
	.osd = {
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
		 (1 << 16) |
	         LCD_OSDC_PREMULT1 | 		
	         LCD_OSDC_PREMULT0 | 		
		 LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0,  /* disable colorkey */
		 .colorkey1 = 0,  /* disable colorkey */
		 .alpha0 = 0xA0,	/* alpha value */
		 .alpha1 = 0xA0,	/* alpha value */
		 .fg0 = {16, 0, 0, 800, 480}, /* bpp, x, y, w, h */
		 .fg1 = {24, 0, 0, 800, 480}, /* bpp, x, y, w, h */
	 },
#elif defined(CONFIG_JZ4780_LCD_AUO_A043FL01V2)
	.panel = {
		.cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */
		LCD_CFG_NEWDES | /* 8words descriptor */
		LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
		LCD_CFG_MODE_TFT_24BIT | 	/* output 18bpp */
		LCD_CFG_HSP | 	/* Hsync polarity: active low */
		LCD_CFG_VSP,	/* Vsync polarity: leading edge is falling edge */
		.slcd_cfg = 0,
		.ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_64,	/* 16words burst, enable out FIFO underrun irq */
		480, 272, 60, 41, 10, 8, 4, 4, 2,
	},
	.osd = {
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
		 (1 << 16) |
	         LCD_OSDC_PREMULT1 | 		
	         LCD_OSDC_PREMULT0 | 		
		 //LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0,  /* disable colorkey */
		 .colorkey1 = 0,  /* disable colorkey */
		 .alpha0 = 0xA0,	/* alpha value */
		 .alpha1 = 0xA0,	/* alpha value */
		 .ipu_restart = 0x80001000, /* ipu restart */
		 .fg0 = {24, 0, 0, 100, 100}, /* bpp, x, y, w, h */
		 .fg1 = {30, 0, 0, 480, 272}, /* bpp, x, y, w, h */
	 },
#elif defined(CONFIG_JZ4780_LCD_HYNIX_HT12X14)
	.panel = {
//		.cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */ 
		.cfg = LCD_CFG_LCDPIN_LCD | /* Underrun recover */ 
		LCD_CFG_NEWDES | /* 8words descriptor */
		LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
		LCD_CFG_MODE_TFT_18BIT|(0<<9)|	/* output 16bpp */
		LCD_CFG_PCP |	/* PCLK polarity: the falling edge acts as data strobe*/
		LCD_CFG_HSP |	/* Hsync polarity: active low */
		LCD_CFG_VSP,	/* Vsync polarity: leading edge is falling edge */
		.slcd_cfg = 0,
		.ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_64,	/* 16words burst, enable out FIFO underrun irq */
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		1024, 768, 60, 1, 1, 75, 0, 3, 0,
	},
	.osd = {
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
		 (1 << 16) |
	         LCD_OSDC_PREMULT1 | 		
	         LCD_OSDC_PREMULT0 | 		
		 //LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0xff0000, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0,  /* disable colorkey */
		 .colorkey1 = 0,  /* disable colorkey */
		 .alpha0 = 0xff,	/* alpha value */
		 .alpha1 = 0xff,	/* alpha value */
		 .fg0 = {24, 0, 0, 1024, 768}, /* bpp, x, y, w, h */
		 .fg1 = {24, 0, 0, 320, 240}, /* bpp, x, y, w, h */
		},
#elif defined(CONFIG_JZ4780_LCD_KD101N2)
	.panel = {
		.cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */ 
		LCD_CFG_NEWDES | /* 8words descriptor */
		LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
		LCD_CFG_MODE_TFT_24BIT|(0<<9)|	/* output 16bpp */
		LCD_CFG_PCP |	/* PCLK polarity: the falling edge acts as data strobe*/
		LCD_CFG_HSP |	/* Hsync polarity: active low */
		LCD_CFG_VSP,	/* Vsync polarity: leading edge is falling edge */
		.slcd_cfg = 0,
		.ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_64,	/* 16words burst, enable out FIFO underrun irq */
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		1024, 600, 75, 1, 1, 320, 0, 35, 0,
	},
	.osd = {
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
		 (1 << 16) |
	         LCD_OSDC_PREMULT1 | 		
	         LCD_OSDC_PREMULT0 | 		
		 //LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0,  /* disable colorkey */
		 .colorkey1 = 0,  /* disable colorkey */
		 .alpha0 = 0xff,	/* alpha value */
		 .alpha1 = 0xff,	/* alpha value */
		 .fg0 = {24, 0, 0, 1024, 600}, /* bpp, x, y, w, h */
		 .fg1 = {24, 0, 0, 320, 240}, /* bpp, x, y, w, h */
		},

#elif defined(CONFIG_JZ4780_LCD_APPROVAL)
	.panel = {
		.cfg = LCD_CFG_LCDPIN_LCD | LCD_CFG_RECOVER | /* Underrun recover */ 
		LCD_CFG_NEWDES | /* 8words descriptor */
		LCD_CFG_MODE_GENERIC_TFT | /* General TFT panel */
		LCD_CFG_MODE_TFT_24BIT|(0<<9)|	/* output 16bpp */
		LCD_CFG_PCP |	/* PCLK polarity: the falling edge acts as data strobe*/
		LCD_CFG_HSP |	/* Hsync polarity: active low */
		LCD_CFG_VSP,	/* Vsync polarity: leading edge is falling edge */
		.slcd_cfg = 0,
		.ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_64,	/* 16words burst, enable out FIFO underrun irq */
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		1366, 768, 60, 1, 1, 193, 0, 37, 0,
	},
	.osd = {
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
		 (1 << 16) |
	         LCD_OSDC_PREMULT1 | 		
	         LCD_OSDC_PREMULT0 | 		
		 //LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0,  /* disable colorkey */
		 .colorkey1 = 0,  /* disable colorkey */
		 .alpha0 = 0xff,	/* alpha value */
		 .alpha1 = 0xff,	/* alpha value */
		 .fg0 = {24, 0, 0, 1366, 300}, /* bpp, x, y, w, h */
		 .fg1 = {24, 0, 0, 320, 240}, /* bpp, x, y, w, h */
		},

#elif defined(CONFIG_JZ4780_SLCD_KGM701A3_TFT_SPFD5420A)
	.panel = {
//		 .cfg = LCD_CFG_LCDPIN_SLCD | LCD_CFG_RECOVER | /* Underrun recover*/
		 .cfg = LCD_CFG_LCDPIN_SLCD | /* Underrun recover*/
		 LCD_CFG_NEWDES | /* 8words descriptor */
		 LCD_CFG_MODE_SLCD, /* TFT Smart LCD panel */
		 .slcd_cfg = SLCD_CFG_DWIDTH_18BIT | SLCD_CFG_CWIDTH_18BIT | SLCD_CFG_CS_ACTIVE_LOW | SLCD_CFG_RS_CMD_LOW | SLCD_CFG_CLK_ACTIVE_FALLING | SLCD_CFG_TYPE_PARALLEL,
		 .ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,	/* 16words burst, enable out FIFO underrun irq */
		 400, 240, 60, 0, 0, 0, 0, 0, 0,
	 },
	.osd = {
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
		 (1 << 16) |
	         LCD_OSDC_PREMULT0 | 		
		 //LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0, /* disable colorkey */
		 .colorkey1 = 0, /* disable colorkey */
		 .alpha0 = 0xA0,	/* alpha value */
		 .alpha1 = 0xA0,	/* alpha value */
		 .ipu_restart = 0x80001000, /* ipu restart */
		 .fg0 = {24, 0, 0, 400, 240}, /* bpp, x, y, w, h */
		 .fg1 = {30, 0, 0, 400, 240}, /* bpp, x, y, w, h */
	 },
#else
#error "Select LCD panel first!!!"
#endif
};

struct jz4780lcd_info jz4780_info_hdmi_VGA = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
		LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,           
		//LCD_CFG_PCP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_32,                                  
	       //width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		   640,   480,  60,   96,    2, 16, 48, 10, 33,       //VGA      
	},                                      
        .osd = {                                
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
	         LCD_OSDC_PREMULT0 | 		
		 //LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0, /* disable colorkey */
		 .colorkey1 = 0, /* disable colorkey */
		 .alpha0 = 0xA0,	/* alpha value */
		 .alpha1 = 0xA0,	/* alpha value */
		 .ipu_restart = 0x80001000, /* ipu restart */
		 .fg0 = {16, 0, 0, 640, 480},   // bpp, x, y, w, h
                 .fg1 = {16, 0, 0, 320, 240},       // bpp, x, y, w, h
        },
};

struct jz4780lcd_info jz4780_info_hdmi_720x480p = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
		LCD_CFG_PCP ,//| LCD_CFG_HSP | LCD_CFG_VSP,           
		//LCD_CFG_PCP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_64,                                  
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		//480, 272, 60, 41, 10, 8, 4, 4, 2,
		720,480, 60, 62, 6, 16, 60,  9,30,       //HDMI-480P       
	},                                      
        .osd = {                                
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
	         LCD_OSDC_PREMULT0 | 		
		 LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0, /* disable colorkey */
		 .colorkey1 = 0, /* disable colorkey */
		 .alpha0 = 0xA0,	/* alpha value */
		 .alpha1 = 0xA0,	/* alpha value */
		 .ipu_restart = 0x80001000, /* ipu restart */
		 .fg0 = {24, 0, 0, 720, 480},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 720, 480},       // bpp, x, y, w, h
        },
};


//dtd.c differ with cea-861
struct jz4780lcd_info jz4780_info_hdmi_1440x480i = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_INTER_CCIR656 |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
		LCD_CFG_PCP ,//| LCD_CFG_HSP | LCD_CFG_VSP,           
		//LCD_CFG_PCP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_32,                                  
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		1440,240, 60, 124, 3, 38, 114,  4,15,       //HDMI-480P       
	},                                      
        .osd = {                                
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 (1 << LCD_OSDC_COEF_SLE0_BIT) | /*src over mode*/           
		 (3 << LCD_OSDC_COEF_SLE1_BIT) |
	         LCD_OSDC_PREMULT0 | 		
		 //LCD_OSDC_F1EN | /* enable Foreground1 */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = 0,
		 .bgcolor0 = 0x0, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0, /* disable colorkey */
		 .colorkey1 = 0, /* disable colorkey */
		 .alpha0 = 0xA0,	/* alpha value */
		 .alpha1 = 0xA0,	/* alpha value */
		 .ipu_restart = 0x80001000, /* ipu restart */
		 .fg0 = {24, 0, 0, 720, 480},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 640, 480},       // bpp, x, y, w, h
        },
};

static void set_i2s_external_codec(void)
{
#if	defined(CONFIG_JZ4760_CYGNUS) || defined(CONFIG_JZ4760_CYGNUS)
	/* gpio defined based on CYGNUS board */
        __gpio_as_func1(3*32 + 12); //blck
        __gpio_as_func0(3*32 + 13); //sync
        __gpio_as_func0(4*32 + 7);  //sd0
        __gpio_as_func0(4*32 + 11); //sd1
        __gpio_as_func0(4*32 + 12); //sd2
        __gpio_as_func0(4*32 + 13); //sd3
#endif

        __i2s_external_codec();

        __aic_select_i2s();
        __i2s_select_i2s();
        __i2s_as_master();

        REG_AIC_I2SCR |= AIC_I2SCR_ESCLK;

        __i2s_disable_record();
        __i2s_disable_replay();
        __i2s_disable_loopback();

        REG_AIC_FR &= ~AIC_FR_TFTH_MASK;
        REG_AIC_FR |= ((8) << AIC_FR_TFTH_LSB);
        REG_AIC_FR &= ~AIC_FR_RFTH_MASK;
        REG_AIC_FR |= ((8) << AIC_FR_RFTH_LSB);

	__i2s_enable();

}

#if defined(CONFIG_JZ4780_HDMI_DISPLAY)  
#define   AIC_FR_TFTH_BIT         16
#define   AIC_FR_RFTH_BIT         24

#define PANEL_MODE_HDMI_480P   	 3
#define PANEL_MODE_HDMI_576P   	 4
#define PANEL_MODE_HDMI_720P50 	 5
#define PANEL_MODE_HDMI_720P60 	 6
#define PANEL_MODE_HDMI_1080P30  7
#define PANEL_MODE_HDMI_1080P50  8
#define PANEL_MODE_HDMI_1080P60  9

struct jz4780lcd_info jz4780_info_hdmi_480p = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
                       LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_32,                                  
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		640,480, 60, 96, 2,48,16,  33,10,       //HDMI-480P       
		//800,600,58,128,4,88,40,23,1,i
		//1024,768,60,136,6,160,24,29,3,
	},                                      
        .osd = {                                
                .osd_cfg =  LCD_OSDC_OSDEN      |               // Use OSD mode
                 LCD_OSDC_ALPHAEN                       |               // enable alpha
                 LCD_OSDC_F0EN                          ,               // enable Foreground0    
                // LCD_OSDC_F1EN,                                         // enable Foreground1    
		
                 .osd_ctrl = 0,                                         // disable ipu,          
                 .rgb_ctrl = 0,                                                                  
                 .bgcolor = 0x000000,                           // set background color Black    
                 .colorkey0 = 0,                                        // disable colorkey
                 .colorkey1 = 0,                                        // disable colorkey
                 .alpha = 0xa0,                                         // alpha value
                 .ipu_restart = 0x8000085d,                     // ipu restart
                 .fg_change = FG_CHANGE_ALL,            // change all initially
		 .fg0 = {32, 0, 0, 640, 480},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 640, 480},       // bpp, x, y, w, h
        },
};
struct jz4780lcd_info jz4780_info_hdmi_576p = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
                       LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_32,                                  
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		720,576,50,64,5,68,12,40,4,                               
		//800,600,58,128,4,88,40,23,1,i
		//1024,768,60,136,6,160,24,29,3,
	},                                      
        .osd = {                                
                .osd_cfg =  LCD_OSDC_OSDEN      |               // Use OSD mode
                 LCD_OSDC_ALPHAEN                       |               // enable alpha
                 LCD_OSDC_F0EN                          ,               // enable Foreground0    
                // LCD_OSDC_F1EN,                                         // enable Foreground1    
		
                 .osd_ctrl = 0,                                         // disable ipu,          
                 .rgb_ctrl = 0,                                                                  
                 .bgcolor = 0x000000,                           // set background color Black    
                 .colorkey0 = 0,                                        // disable colorkey
                 .colorkey1 = 0,                                        // disable colorkey
                 .alpha = 0xa0,                                         // alpha value
                 .ipu_restart = 0x8000085d,                     // ipu restart
                 .fg_change = FG_CHANGE_ALL,            // change all initially
		 .fg0 = {32, 0, 0, 720, 576},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 720, 576},       // bpp, x, y, w, h
        },
};
struct jz4780lcd_info jz4780_info_hdmi_720p50 = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
                       LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_32,                                  
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
                1280,720,50,40,5,440,220,20,5,
	//	1280, 720, 50, 152, 15, 22, 200, 14, 1
	//	1280,720,46,1,1,121,259,6,19,                       
		//800,600,58,128,4,88,40,23,1,i
		//1024,768,60,136,6,160,24,29,3,
	},                                      
        .osd = {                                
                .osd_cfg =  LCD_OSDC_OSDEN      |               // Use OSD mode
                 LCD_OSDC_ALPHAEN                       |               // enable alpha
                 LCD_OSDC_F0EN                          ,               // enable Foreground0    
                // LCD_OSDC_F1EN,                                         // enable Foreground1    
		
                 .osd_ctrl = 0,                                         // disable ipu,          
                 .rgb_ctrl = 0,                                                                  
                 .bgcolor = 0x000000,                           // set background color Black    
                 .colorkey0 = 0,                                        // disable colorkey
                 .colorkey1 = 0,                                        // disable colorkey
		.alpha = 0xa0,                                         // alpha value
		.ipu_restart = 0x8000085d,                     // ipu restart
		.fg_change = FG_CHANGE_ALL,            // change all initially
		.fg0 = {32, 0, 0, 1280, 720},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 1280, 720},       // bpp, x, y, w, h
        },
};
struct jz4780lcd_info jz4780_info_hdmi_720p60 = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
                       LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_32,                                  
		//  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		1280,720,60,40,5,110,220,20,5,//74250000                  
		//800,600,58,128,4,88,40,23,1,i
		//1024,768,60,136,6,160,24,29,3,
	},                                      
        .osd = {                                
                .osd_cfg =  LCD_OSDC_OSDEN      |               // Use OSD mode
                 LCD_OSDC_ALPHAEN                       |               // enable alpha
                 LCD_OSDC_F0EN                          ,               // enable Foreground0    
                // LCD_OSDC_F1EN,                                         // enable Foreground1    
		
                 .osd_ctrl = 0,                                         // disable ipu,          
                 .rgb_ctrl = 0,                                                                  
                 .bgcolor = 0x000000,                           // set background color Black    
                 .colorkey0 = 0,                                        // disable colorkey
                 .colorkey1 = 0,                                        // disable colorkey
                 .alpha = 0xa0,                                         // alpha value
                 .ipu_restart = 0x8000085d,                     // ipu restart
                 .fg_change = FG_CHANGE_ALL,            // change all initially
		 .fg0 = {32, 0, 0, 1280, 720},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 1280, 720},       // bpp, x, y, w, h
        },
};
struct jz4780lcd_info jz4780_info_hdmi_1080p30 = {
        .panel = {
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |
                       LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_BST_32,
                //  width,height,freq,hsync,vsync,elw,blw,efw,bfw
                1920,1080,30,44,5,148,88,36,4
                //800,600,58,128,4,88,40,23,1,i
                //1024,768,60,136,6,160,24,29,3,
        },
        .osd = {
                .osd_cfg =  LCD_OSDC_OSDEN      |               // Use OSD mode
                 LCD_OSDC_ALPHAEN                       |               // enable alpha
                 LCD_OSDC_F0EN                          ,               // enable Foreground0
                // LCD_OSDC_F1EN,                                         // enable Foreground1

                 .osd_ctrl = 0,                                         // disable ipu,
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000,                           // set background color Black
                 .colorkey0 = 0,                                        // disable colorkey
                 .colorkey1 = 0,                                        // disable colorkey
                 .alpha = 0xa0,                                         // alpha value
                 .ipu_restart = 0x8000085d,                     // ipu restart
                 .fg_change = FG_CHANGE_ALL,            // change all initially
                 .fg0 = {32, 0, 0, 1920, 1080},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 1920, 1080},       // bpp, x, y, w, h
        },
};




struct jz4780lcd_info jz4780_info_hdmi_1080p50 = {
        .panel = {
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |
                       LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,
                .slcd_cfg = 0,
                .ctrl = LCD_CTRL_BST_32|LCD_CTRL_OFUM,
                //  width,height,freq,hsync,vsync,elw,blw,efw,bfw
                1920,1080,50,44,5,148,528,36,4
                //800,600,58,128,4,88,40,23,1,i
                //1024,768,60,136,6,160,24,29,3,
        },
        .osd = {
                .osd_cfg =  LCD_OSDC_OSDEN      |               // Use OSD mode
                 LCD_OSDC_ALPHAEN                       |               // enable alpha
                 LCD_OSDC_F0EN                          ,               // enable Foreground0
                // LCD_OSDC_F1EN,                                         // enable Foreground1

                 .osd_ctrl = 0,                                         // disable ipu,
                 .rgb_ctrl = 0,
                 .bgcolor = 0x000000,                           // set background color Black
                 .colorkey0 = 0,                                        // disable colorkey
                 .colorkey1 = 0,                                        // disable colorkey
                 .alpha = 0xa0,                                         // alpha value
                 .ipu_restart = 0x8000085d,                     // ipu restart
                 .fg_change = FG_CHANGE_ALL,            // change all initially
                 .fg0 = {32, 0, 0, 1920, 1080},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 1920, 1080},       // bpp, x, y, w, h
        },
};



struct jz4780lcd_info jz4780_info_hdmi_1080p60 = {                            
        .panel = {                                                     
                .cfg = LCD_CFG_MODE_GENERIC_TFT | LCD_CFG_MODE_TFT_24BIT |
                       LCD_CFG_NEWDES | LCD_CFG_RECOVER |                 
                       LCD_CFG_PCP | LCD_CFG_HSP | LCD_CFG_VSP,           
                .slcd_cfg = 0,                                            
		.ctrl = LCD_CTRL_BST_64|LCD_CTRL_OFUM,                                  //  width,height,freq,hsync,vsync,elw,blw,efw,bfw         
		1920,1080,60,44,5,148,88,37,4	                 
		//800,600,58,128,4,88,40,23,1,i
		//1024,768,60,136,6,160,24,29,3,
	},            
        .osd = {                                
                .osd_cfg =  LCD_OSDC_OSDEN      |               // Use OSD mode
                 LCD_OSDC_ALPHAEN                       |               // enable alpha
                 LCD_OSDC_F0EN                          ,               // enable Foreground0    
                // LCD_OSDC_F1EN,                                         // enable Foreground1    
		
                 .osd_ctrl = 0,                                         // disable ipu,          
                 .rgb_ctrl = 0,                                                                  
                 .bgcolor = 0x000000,                           // set background color Black    
                 .colorkey0 = 0,                                        // disable colorkey
                 .colorkey1 = 0,                                        // disable colorkey
                 .alpha = 0xa0,                                         // alpha value
                 .ipu_restart = 0x8000085d,                     // ipu restart
                 .fg_change = FG_CHANGE_ALL,            // change all initially
		 .fg0 = {32, 0, 0, 1920, 1080},   // bpp, x, y, w, h
                 .fg1 = {32, 0, 0, 1920, 1080},       // bpp, x, y, w, h
        },
};
#endif


struct jz4780lcd_info jz4780_info_tve = {
	.panel = {
		.cfg = LCD_CFG_TVEN | /* output to tve */
		LCD_CFG_NEWDES | /* 8words descriptor */
		LCD_CFG_RECOVER | /* underrun protect */
		LCD_CFG_MODE_INTER_CCIR656, /* Interlace CCIR656 mode */
		.ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,	/* 16words burst */
		TVE_WIDTH_PAL, TVE_HEIGHT_PAL, TVE_FREQ_PAL, 0, 0, 0, 0, 0, 0,
	},
	.osd = {
		 .osd_cfg = LCD_OSDC_OSDEN | /* Use OSD mode */
//		 LCD_OSDC_ALPHAEN | /* enable alpha */
		 LCD_OSDC_F0EN,	/* enable Foreground0 */
		 .osd_ctrl = 0,		/* disable ipu,  */
		 .rgb_ctrl = LCD_RGBC_YCC, /* enable RGB => YUV */
		 .bgcolor0 = 0x00000000, /* set background color Black */
		 .bgcolor1 = 0x0, /* set background color Black */
		 .colorkey0 = 0, /* disable colorkey */
		 .colorkey1 = 0, /* disable colorkey */
		 .alpha0 = 0xA0,	/* alpha value */
		 .alpha1 = 0xA0,	/* alpha value */
		 .ipu_restart = 0x80000100, /* ipu restart */
		 .fg0 = {32,},	/*  */
		 .fg0 = {32,},
	},
};

struct timer_list timeout_timer; 
int timeout_flag = 0;

#define DMA_DESC_NUM 		11
#define FB_ALIGN   (16*4)
#define AOSD_ALIGN (4)		/* AOSD words aligned at least */

struct lcd_cfb_info {
	struct fb_info		fb;
	struct {
		u16 red, green, blue;
	} palette[NR_PALETTE];

	int b_lcd_display;
	int b_lcd_pwm;
	int backlight_level;
	spinlock_t	update_lock;
	unsigned 	frame_requested;
	unsigned 	frame_done;
	wait_queue_head_t frame_wq;
	
	int id;
	struct jz4780_lcd_dma_desc *dma_desc_base;
	struct jz4780_lcd_dma_desc *dma0_desc_palette, *dma0_desc0, *dma0_desc1, *dma1_desc0, *dma1_desc1, *dma0_tv_desc0, *dma0_tv_desc1, *dma1_tv_desc0, *dma1_tv_desc1;

	unsigned char *lcd_palette;
	unsigned char *lcd_frame0;
	unsigned char *lcd_frame1;

	struct jz4780_lcd_dma_desc *dma0_desc_cmd0, *dma0_desc_cmd;
	unsigned char *lcd_cmdbuf;
	unsigned char *buf_comp00, *buf_comp01; //buffer for compress output
	unsigned char *buf_comp10, *buf_comp11; //buffer for compress output
	

#if defined(CONFIG_JZ_AOSDC)
	struct jz_aosd_info *aosd_info;
	wait_queue_head_t compress_wq;
	u16 aosd_page_shift;
#endif

	int is_fg1;

	struct jz4780lcd_info *jz4780_lcd_info;
	struct jz4780lcd_enh_info *enh_info;

	int fg0_use_compress_decompress_mode;
	int fg1_use_compress_decompress_mode;

	int fg0_use_x2d_block_mode;
	int fg1_use_x2d_block_mode;

};

struct lcd_cfb_info *jz4780fb0_info;
struct lcd_cfb_info *jz4780fb1_info;
struct jz_aosd_info *aosd_info;
wait_queue_head_t compress_wq;

static void jz4780fb_deep_set_mode(struct lcd_cfb_info *cfb);
void jz4780fb_reset_descriptor(struct lcd_cfb_info *cfb);
static int jz4780fb_set_backlight_level(int n);

static int screen_on(int id);
static int screen_off(int id);
static void calc_line_frm_size(int bpp, struct jz4780lcd_fg_t *fg, unsigned int *line_size, unsigned int *frm_size);
static int bpp_to_data_bpp(int tbpp);

#if JZ_FB_DEBUG
static void print_lcdc_registers(int n)	/* debug */
{
	/* LCD Controller Resgisters */
	printk("REG_LCD_CFG:\t0x%08x\n", REG_LCD_CFG(n));
	printk("REG_LCD_CTRL:\t0x%08x\n", REG_LCD_CTRL(n));
	printk("REG_LCD_STATE:\t0x%08x\n", REG_LCD_STATE(n));
	printk("REG_LCD_OSDC:\t0x%08x\n", REG_LCD_OSDC(n));
	printk("REG_LCD_OSDCTRL:\t0x%08x\n", REG_LCD_OSDCTRL(n));
	printk("REG_LCD_OSDS:\t0x%08x\n", REG_LCD_OSDS(n));
	printk("REG_LCD_BGC0:\t0x%08x\n", REG_LCD_BGC0(n));
	printk("REG_LCD_BGC1:\t0x%08x\n", REG_LCD_BGC1(n));
	printk("REG_LCD_KEK0:\t0x%08x\n", REG_LCD_KEY0(n));
	printk("REG_LCD_KEY1:\t0x%08x\n", REG_LCD_KEY1(n));
	printk("REG_LCD_ALPHA0:\t0x%08x\n", REG_LCD_ALPHA(n));
	printk("REG_LCD_IPUR:\t0x%08x\n", REG_LCD_IPUR(n));
	printk("REG_LCD_VAT:\t0x%08x\n", REG_LCD_VAT(n));
	printk("REG_LCD_DAH:\t0x%08x\n", REG_LCD_DAH(n));
	printk("REG_LCD_DAV:\t0x%08x\n", REG_LCD_DAV(n));
	printk("REG_LCD_XYP0:\t0x%08x\n", REG_LCD_XYP0(n));
	printk("REG_LCD_XYP1:\t0x%08x\n", REG_LCD_XYP1(n));
	printk("REG_LCD_SIZE0:\t0x%08x\n", REG_LCD_SIZE0(n));
	printk("REG_LCD_SIZE1:\t0x%08x\n", REG_LCD_SIZE1(n));
	printk("REG_LCD_RGBC\t0x%08x\n", REG_LCD_RGBC(n));
	printk("REG_LCD_VSYNC:\t0x%08x\n", REG_LCD_VSYNC(n));
	printk("REG_LCD_HSYNC:\t0x%08x\n", REG_LCD_HSYNC(n));
	printk("REG_LCD_PS:\t0x%08x\n", REG_LCD_PS(n));
	printk("REG_LCD_CLS:\t0x%08x\n", REG_LCD_CLS(n));
	printk("REG_LCD_SPL:\t0x%08x\n", REG_LCD_SPL(n));
	printk("REG_LCD_REV:\t0x%08x\n", REG_LCD_REV(n));
	printk("REG_LCD_IID:\t0x%08x\n", REG_LCD_IID(n));
	printk("REG_LCD_DA0:\t0x%08x\n", REG_LCD_DA0(n));
	printk("REG_LCD_SA0:\t0x%08x\n", REG_LCD_SA0(n));
	printk("REG_LCD_FID0:\t0x%08x\n", REG_LCD_FID0(n));
	printk("REG_LCD_CMD0:\t0x%08x\n", REG_LCD_CMD0(n));
	printk("REG_LCD_OFFS0:\t0x%08x\n", REG_LCD_OFFS0(n));
	printk("REG_LCD_PW0:\t0x%08x\n", REG_LCD_PW0(n));
	printk("REG_LCD_CNUM0:\t0x%08x\n", REG_LCD_CNUM0(n));
	printk("REG_LCD_DESSIZE0:\t0x%08x\n", REG_LCD_DESSIZE0(n));
	printk("REG_LCD_DA1:\t0x%08x\n", REG_LCD_DA1(n));
	printk("REG_LCD_SA1:\t0x%08x\n", REG_LCD_SA1(n));
	printk("REG_LCD_FID1:\t0x%08x\n", REG_LCD_FID1(n));
	printk("REG_LCD_CMD1:\t0x%08x\n", REG_LCD_CMD1(n));
	printk("REG_LCD_OFFS1:\t0x%08x\n", REG_LCD_OFFS1(n));
	printk("REG_LCD_PW1:\t0x%08x\n", REG_LCD_PW1(n));
	printk("REG_LCD_CNUM1:\t0x%08x\n", REG_LCD_CNUM1(n));
	printk("REG_LCD_DESSIZE1:\t0x%08x\n", REG_LCD_DESSIZE1(n));
	printk("==================================\n");
	printk("REG_LCD_VSYNC:\t%d:%d\n", REG_LCD_VSYNC(n)>>16, REG_LCD_VSYNC(n)&0xfff);
	printk("REG_LCD_HSYNC:\t%d:%d\n", REG_LCD_HSYNC(n)>>16, REG_LCD_HSYNC(n)&0xfff);
	printk("REG_LCD_VAT:\t%d:%d\n", REG_LCD_VAT(n)>>16, REG_LCD_VAT(n)&0xfff);
	printk("REG_LCD_DAH:\t%d:%d\n", REG_LCD_DAH(n)>>16, REG_LCD_DAH(n)&0xfff);
	printk("REG_LCD_DAV:\t%d:%d\n", REG_LCD_DAV(n)>>16, REG_LCD_DAV(n)&0xfff);
	printk("==================================\n");
	printk("REG_LCD_PCFG:\t0x%08x\n", REG_LCD_PCFG(n));
#if 1 
	/* Smart LCD Controller Resgisters */
	printk("REG_SLCD_CFG:\t0x%08x\n", REG_SLCD_CFG);
	printk("REG_SLCD_CTRL:\t0x%08x\n", REG_SLCD_CTRL);
	printk("REG_SLCD_STATE:\t0x%08x\n", REG_SLCD_STATE);
	printk("==================================\n");
#endif
}

/*
static void dump_dma_desc(void)
{
	unsigned int * pii = (unsigned int *)dma_desc_base;
	int i, j;
	for (j=0;j< DMA_DESC_NUM ; j++) {
		printk("dma_desc%d(0x%08x):\n", j, (unsigned int)pii);
		for (i =0; i<8; i++ ) {
			printk("\t\t0x%08x\n", *pii++);
		}
	}
}
*/
static void dump_dma_desc( struct jz4780_lcd_dma_desc *desc)
{
	printk("================== desc:%x=======================\n",(unsigned int)virt_to_phys((void *)desc));
	printk("next_desc: %x\n", desc->next_desc); 	/* LCDDAx */
	printk("databuf:%x\n", desc->databuf);   	/* LCDSAx */
	printk("frame_id:%x\n", desc->frame_id);  	/* LCDFIDx */
	printk("cmd:%x\n", desc->cmd); 		        /* LCDCMDx */
	printk("offsize:%x\n", desc->offsize);       	/* Stride Offsize(in word) */
	printk("page_width:%x\n", desc->page_width); 	/* Stride Pagewidth(in word) */
	printk("cmd_num:%x\n", desc->cmd_num); 		/* Command Number(for SLCD) and CPOS(except SLCD) */
	printk("desc_size:%x\n", desc->desc_size); 	/* Foreground Size and alpha value*/
	printk("\n\n");
}

static void dump_enh_info(struct jz4780lcd_enh_info *info)
{
	printk("enh_cfg:%x\n",info->enh_cfg);
	printk("ycc2rgb:%x\n",info->ycc2rgb);
	printk("rgb2ycc:%x\n",info->rgb2ycc);
	printk("brightness:%x\n",info->brightness);
	printk("contrast:%x\n",info->contrast);
	printk("hue:%x\n",info->hue);
	printk("saturation:%x\n",info->saturation);
	printk("dither:%x\n",info->dither);
}

#else
#define print_lcdc_registers(n)
#define dump_dma_desc(a)
#endif


static void ctrl_enable(struct lcd_cfb_info *cfb)
{
	if ( cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD ){ /* enable Smart LCD DMA */
		REG_SLCD_CTRL |= ((1 << 0) | (1 << 2));	
		REG_SLCD_CTRL &= ~(1 << 3);
	}
	
	__lcd_state_clr_all(cfb->id);
	__lcd_osds_clr_all(cfb->id);
	__lcd_clr_dis(cfb->id);
	__lcd_set_ena(cfb->id); /* enable lcdc */
	if ( cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD ){ /* enable Smart LCD DMA */
		__lcd_slcd_special_on();
	}

	return;
}

static void ctrl_disable(struct lcd_cfb_info *cfb)
{
	if ( cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD ||
			cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_TVEN ) /*  */

		__lcd_clr_ena(cfb->id); /* Smart lcd and TVE mode only support quick disable */
	else {
		if(!__is_lcd_ena(cfb->id))
			return;

		__lcd_set_dis(cfb->id); /* regular disable */

		while(!__lcd_disable_done(cfb->id));

		__lcd_clr_ldd(cfb->id);
	}

	return;
}

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
        chan &= 0xffff;
        chan >>= 16 - bf->length;
        return chan << bf->offset;
}

static int jz4780fb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			  u_int transp, struct fb_info *info)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	unsigned short *ptr, ctmp;

//	D("regno:%d,RGBt:(%d,%d,%d,%d)\t", regno, red, green, blue, transp);

	if (regno >= NR_PALETTE)
		return 1;

	cfb->palette[regno].red		= red ;
	cfb->palette[regno].green	= green;
	cfb->palette[regno].blue	= blue;
	if (cfb->fb.var.bits_per_pixel <= 16) {
/*
		red	>>= 8;
		green	>>= 8;
		blue	>>= 8;

		red	&= 0xff;
		green	&= 0xff;
		blue	&= 0xff;
*/
	}
	switch (cfb->fb.var.bits_per_pixel) {
	case 1:
	case 2:
	case 4:
	case 8:
		printk("set palette ----< \n");
		if (((cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) == LCD_CFG_MODE_SINGLE_MSTN ) ||
		    ((cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) == LCD_CFG_MODE_DUAL_MSTN )) {
			ctmp = (77L * red + 150L * green + 29L * blue) >> 8;
			ctmp = ((ctmp >> 3) << 11) | ((ctmp >> 2) << 5) |
				(ctmp >> 3);
		} else {
			/* RGB 565 */
/*
			if (((red >> 3) == 0) && ((red >> 2) != 0))
				red = 1 << 3;
			if (((blue >> 3) == 0) && ((blue >> 2) != 0))
				blue = 1 << 3;
			ctmp = ((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
*/
			ctmp = ((red) << 11) | ((green) << 5) | (blue );

		}

		ptr = (unsigned short *)cfb->lcd_palette;
		ptr = (unsigned short *)(((u32)ptr)|0xa0000000);
		ptr[regno] = ctmp;

		break;

	case 15:
		if (regno < 16)
			((u32 *)cfb->fb.pseudo_palette)[regno] =
				((red >> 3) << 10) |
				((green >> 3) << 5) |
				(blue >> 3);
		break;
	case 16:
		if (regno < 16) {
			((u32 *)cfb->fb.pseudo_palette)[regno] =
				((red >> 3) << 11) |
				((green >> 2) << 5) |
				(blue >> 3);
		}
		break;
	case 17 ... 32:
		if (regno < 16)
			((u32 *)cfb->fb.pseudo_palette)[regno] =
				(red << 16) |
				(green << 8) |
				(blue << 0);

/*		if (regno < 16) {
			unsigned val;
                        val  = chan_to_field(red, &cfb->fb.var.red);
                        val |= chan_to_field(green, &cfb->fb.var.green);
                        val |= chan_to_field(blue, &cfb->fb.var.blue);
			((u32 *)cfb->fb.pseudo_palette)[regno] = val;
		}
*/

		break;
	}
	return 0;
}

/*
 * switch to tve mode from lcd mode
 * mode:
 * 	PANEL_MODE_TVE_PAL: switch to TVE_PAL mode
 * 	PANEL_MODE_TVE_NTSC: switch to TVE_NTSC mode
 */
static void jz4780lcd_info_switch_to_TVE(struct lcd_cfb_info *cfb, int mode)
{
	struct jz4780lcd_info *info;
	struct jz4780lcd_osd_t *osd_lcd;
	int x, y, w, h;

	info = cfb->jz4780_lcd_info = &jz4780_info_tve;
	osd_lcd = &jz4780_lcd_panel.osd;

	switch ( mode ) {
	case PANEL_MODE_TVE_PAL:
		info->panel.cfg |= LCD_CFG_TVEPEH; /* TVE PAL enable extra halfline signal */
		info->panel.w = TVE_WIDTH_PAL;
		info->panel.h = TVE_HEIGHT_PAL;
		info->panel.fclk = TVE_FREQ_PAL;

		w = TVE_WIDTH_PAL16;
		h = TVE_HEIGHT_PAL16;
		x = (TVE_WIDTH_PAL - w ) / 2;
		y = (TVE_HEIGHT_PAL - h) / 2;
		
		info->osd.fg0.bpp = osd_lcd->fg0.bpp;
		info->osd.fg0.x = x;
		info->osd.fg0.y = y;
		info->osd.fg0.w = w;
		info->osd.fg0.h = h;


		info->osd.fg1.bpp = 32;	/* use RGB888 in TVE mode*/
		info->osd.fg1.x = x;
		info->osd.fg1.y = y;
		info->osd.fg1.w = w;
		info->osd.fg1.h = h;
		break;
	case PANEL_MODE_TVE_NTSC:
		info->panel.cfg &= ~LCD_CFG_TVEPEH; /* TVE NTSC disable extra halfline signal */
		info->panel.w = TVE_WIDTH_NTSC;
		info->panel.h = TVE_HEIGHT_NTSC;
		info->panel.fclk = TVE_FREQ_NTSC;

		w = TVE_WIDTH_NTSC16;
		h = TVE_HEIGHT_NTSC16;
		x = (TVE_WIDTH_NTSC - TVE_WIDTH_NTSC16) / 2;
		y = (TVE_HEIGHT_NTSC - TVE_HEIGHT_NTSC16) / 2;

		info->osd.fg0.bpp = osd_lcd->fg0.bpp;
		info->osd.fg0.x = x;
		info->osd.fg0.y = y;
		info->osd.fg0.w = w;
		info->osd.fg0.h = h;

		info->osd.fg1.bpp = 32;	/* use RGB888 int TVE mode */
		info->osd.fg1.x = x;
		info->osd.fg1.y = y;
		info->osd.fg1.w = w;
		info->osd.fg1.h = h;
		break;
	default:
		printk("%s, %s: Unknown tve mode\n", __FILE__, __FUNCTION__);
	}
}

#if 0
static void jz4780fb_set_enhancment(struct jz4780lcd_enh_info *info){
	unsigned int new_cfg = info->enh_cfg;
	unsigned int last_cfg = enh_info->enh_cfg;
	unsigned int differ = new_cfg ^ last_cfg;
	int i;

	if(differ & LCD_ENHCFG_ENH_EN){
		if(new_cfg & LCD_ENHCFG_ENH_EN)
			__enh_enable();
		else
			__enh_disable();
	}

	if(!(__is_enh_enabled()))
		return;


        /* dither enable or disable */
	if(differ & LCD_ENHCFG_DITHER_EN){
		if(new_cfg & LCD_ENHCFG_DITHER_EN){
			__dither_enable();
			__set_dither_value(info->dither);
		}
		else{
			__dither_disable();
			add_timer(&timeout_timer);
			while(!(__dither_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
		}
	}

        /* ycc2rgb enable or disable */
	if(differ & LCD_ENHCFG_YCC2RGB_EN){
		if(new_cfg & LCD_ENHCFG_YCC2RGB_EN)
		{
			__ycc2rgb_enable();
			__set_ycc2rgb_value(info->ycc2rgb);
		}
		else{
			__ycc2rgb_disable();
			add_timer(&timeout_timer);
			while(!(__ycc2rgb_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
		}
	}

        /*saturation enable or disable */	
	if(differ & LCD_ENHCFG_SATURATION_EN){
		if(new_cfg & LCD_ENHCFG_SATURATION_EN){
			__saturation_enable();
			__set_saturatin_value(info->saturation);
		}
		else{
			__saturation_disable();
			add_timer(&timeout_timer);
			while(!(__saturation_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
		}
	}


        /*vee enable or disable */	
	if(differ & LCD_ENHCFG_VEE_EN){
		if(new_cfg & LCD_ENHCFG_VEE_EN){
			unsigned int data0, data1;
			
			__vee_enable();
			for(i=0 ;i<LCD_VEECFG_LEN/4; i++){
				data0 = (info->vee[i]&LCD_VEECFG_DATA0_MASK);
				data1 = (info->vee[i] & LCD_VEECFG_DATA1_MASK)>>LCD_VEECFG_DATA1_BIT;
				__set_vee_value(data1, data0, i);
			}
		}
		else{
			__vee_disable();
			add_timer(&timeout_timer);
			while(!(__vee_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
			
		}
	}

        /*hue enable or disable */	
	if(differ & LCD_ENHCFG_HUE_EN){
		if(new_cfg & LCD_ENHCFG_HUE_EN){
			__hue_enable();
			__set_hue_value(info->hue>>LCD_CHROCFG0_HUE_SIN_BIT, info->hue&LCD_CHROCFG0_HUE_COS_MASK);
		}
		else{
			__hue_disable();
			add_timer(&timeout_timer);
			while(!(__hue_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
			
		}
	}

        /*brightness enable or disable */	
	if(differ & LCD_ENHCFG_BRIGHTNESS_EN){
		if(new_cfg & LCD_ENHCFG_BRIGHTNESS_EN){
			__brightness_enable();
			__set_brightness_value(info->brightness);
		}
		else{
			__brightness_disable();
			add_timer(&timeout_timer);
			while(!(__brightness_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
		}
	}

        /*contrast enable or disable */	
	if(differ & LCD_ENHCFG_CONTRAST_EN){
		if(new_cfg & LCD_ENHCFG_CONTRAST_EN){
			__contrast_enable();
			__set_contrast_value(info->contrast);
		}
		else{
			__contrast_disable();
			add_timer(&timeout_timer);
			while(!(__contrast_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
		}
	}

        /*gamma enable or disable */	
	if(differ & LCD_ENHCFG_GAMMA_EN){
		if(new_cfg & LCD_ENHCFG_GAMMA_EN){
			unsigned int data0, data1;
			
			__gamma_enable();
			for(i=0 ;i<LCD_GAMMACFG_LEN/4; i++){
				data0 = (info->gamma[i]&LCD_GAMMACFG_DATA0_MASK);
				data1 = (info->gamma[i] & LCD_GAMMACFG_DATA1_MASK)>>LCD_GAMMACFG_DATA1_BIT;
				__set_gamma_value(data1, data0, i);
			}
		}
		else{
			__gamma_disable();
			add_timer(&timeout_timer);
			while(!(__gamma_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
		}
	}

        /* rgb2ycc enable or disable */
	if(differ & LCD_ENHCFG_YCC2RGB_EN){
		if(new_cfg & LCD_ENHCFG_YCC2RGB_EN)
		{
			__rgb2ycc_enable();
			__set_rgb2ycc_value(info->rgb2ycc);
		}
		else{
			__rgb2ycc_disable();
			add_timer(&timeout_timer);
			while(!(__rgb2ycc_disable_done() || timeout_flag));
			del_timer(&timeout_timer);
		}
	}
	
	memcpy(enh_info, info, sizeof(struct jz4780lcd_enh_info));
}
#else
static void jz4780fb_set_enhancment(struct jz4780lcd_enh_info *info, struct lcd_cfb_info *cfb){
	unsigned int new_cfg = info->enh_cfg;
	unsigned int last_cfg = cfb->enh_info->enh_cfg;
	unsigned int differ = new_cfg ^ last_cfg;
	int n = cfb->id;
	int i;

	printk("---> new_cfg:%x\n",new_cfg);
	printk("---> last_cfg:%x\n",last_cfg);
	printk("---> differ:%x\n",differ);

	for(i=0; i<=9; i++){
		if(differ & (1<<i)){                                     //enable or disable changed
			if(new_cfg & (1 << i)){                             //enable
				REG_LCD_ENHCFG(n) |= (1 << i);
				printk("set enh, bit:%d\n",i);
			}else{
				REG_LCD_ENHCFG(n) &= ~(1 << i);               //disable
				while(!(REG_LCD_ENHSTATUS(n) | (1 << i)));    //wait disable done
			}
		}
	}

	cfb->enh_info->enh_cfg = info->enh_cfg;
}

static void jz4780fb_set_enhancment_value(struct jz4780lcd_enh_info *info, struct lcd_cfb_info *cfb){
	unsigned int data0, data1;
	int i;
	int n = cfb->id;

	if(is_dither_enabled(n)){
		printk("set dither value:%x\n",info->dither);
		__set_dither_value(n, info->dither);
	}
	if(is_ycc2rgb_enabled(n)){
		printk("set ycc2rgb :%x\n",info->ycc2rgb);
		__set_ycc2rgb_value(n, info->ycc2rgb);
	}
	if(is_saturation_enabled(n)){
		printk("set saturatin:%x\n",info->saturation);
		__set_saturatin_value(n, info->saturation);
	}
/*
	if(is_vee_enabled(n))
		for(i=0 ;i<LCD_VEECFG_LEN/4; i++){
			data0 = (info->vee[i]&LCD_VEECFG_DATA0_MASK);
			data1 = (info->vee[i] & LCD_VEECFG_DATA1_MASK)>>LCD_VEECFG_DATA1_BIT;
			__set_vee_value(data1, data0, i);
		}
*/
	if(is_hue_enabled(n)){
		printk("set hue:%x\n",info->hue);
		__set_hue_value(n, info->hue>>LCD_CHROCFG0_HUE_SIN_BIT, info->hue&LCD_CHROCFG0_HUE_COS_MASK);
	}
	if(is_brightness_enabled(n)){
		printk("set brightness:%x\n",info->brightness);
		__set_brightness_value(n, info->brightness);
	}
	if(is_contrast_enabled(n)){
		printk("set contrast:%x\n",info->contrast);
		__set_contrast_value(n, info->contrast);
	}
/*
	if(is_gamma_enabled())
		for(i=0 ;i<LCD_GAMMACFG_LEN/4; i++){
			data0 = (info->gamma[i]&LCD_GAMMACFG_DATA0_MASK);
			data1 = (info->gamma[i] & LCD_GAMMACFG_DATA1_MASK)>>LCD_GAMMACFG_DATA1_BIT;
			__set_gamma_value(data1, data0, i);
		}
*/	
	if(is_rgb2ycc_enabled(n)){
		printk("set rgb2ycc:%x\n",info->rgb2ycc);
		__set_rgb2ycc_value(n, info->rgb2ycc);
	}
	memcpy(cfb->enh_info, info, sizeof(struct jz4780lcd_enh_info));
}
#endif

int fg0_should_use_compress_decompress_mode(struct lcd_cfb_info *cfb)
{
#ifdef CONFIG_JZ_AOSDC
	return cfb->fg0_use_compress_decompress_mode;
#else
	return 0;
#endif
}

int fg1_should_use_compress_decompress_mode(struct lcd_cfb_info *cfb)
{
#ifdef CONFIG_JZ_AOSDC
	return cfb->fg1_use_compress_decompress_mode;
#else
	return 0;
#endif
}

int fg0_should_use_x2d_block_mode(struct lcd_cfb_info *cfb)
{
	return cfb->fg0_use_x2d_block_mode;
}

int fg1_should_use_x2d_block_mode(struct lcd_cfb_info *cfb)
{
	return cfb->fg1_use_x2d_block_mode;
}

static int jz4780fb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
        void __user *argp = (void __user *)arg;
	struct jz4780lcd_info usrinfo;
	struct jz4780lcd_enh_info uenhinfo;
	int comp_en, x2d_en;
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;

	switch (cmd) {
	case FBIOSETBACKLIGHT:
		jz4780fb_set_backlight_level(arg);

		break;

	case FBIODISPON:
		D("FBIO DIS ON\n");
		ctrl_enable(cfb);
		screen_on(cfb->id);

		break;

	case FBIODISPOFF:
		D("FBIO DIS OFF\n");
		screen_off(cfb->id);
		ctrl_disable(cfb);

		break;
	case FBIOPRINT_REG:
		print_lcdc_registers(cfb->id);

		break;

	case FBIO_GET_MODE:
		D("fbio get mode\n");

		if (copy_to_user(argp, cfb->jz4780_lcd_info, sizeof(struct jz4780lcd_info)))
			return -EFAULT;

		break;

#ifdef CONFIG_JZ_AOSDC
	case FBIO_GET_COMP_MODE:
		D("fbio get comp  mode\n");

		if (copy_to_user(argp, cfb->aosd_info, sizeof(struct jz_aosd_info)))
			return -EFAULT;

		break;

	case FBIO_SET_COMP_MODE:
		D("fbio set comp mode\n");

		if (copy_from_user( &(cfb->aosd_info->is24c),argp, sizeof(int))){
			printk("error la\n");
			return -EFAULT;
		}

		printk("aosd_info->is24c:%d\n",cfb->aosd_info->is24c);
		break;

#endif

	case FBIO_RESET_DESC:
		D("reset descriptor\n");
		if (copy_from_user(cfb->jz4780_lcd_info,argp, sizeof(struct jz4780lcd_info)))
			return -EFAULT;

		jz4780fb_reset_descriptor(cfb);

		break;

	case FBIO_SET_FG:
		D("set fg\n");
		if (copy_from_user(&cfb->is_fg1, argp, sizeof(int)))
			return -EFAULT;

		break;

	case FBIO_SET_ENH:
		D("set enhancment \n");
		if (copy_from_user(&uenhinfo, argp, sizeof(struct jz4780lcd_enh_info)))
			return -EFAULT;

	//	dump_enh_info(&uenhinfo);
		jz4780fb_set_enhancment(&uenhinfo, cfb);
		break;

	case FBIO_GET_ENH:
		D("get enhancment \n");
		if (copy_to_user(argp, &cfb->enh_info, sizeof(struct jz4780lcd_enh_info)))
			return -EFAULT;

	//	dump_enh_info(&enh_info);
		break;

	case FBIO_SET_ENH_VALUE:
		D("set enhancment \n");
		if (copy_from_user(&uenhinfo, argp, sizeof(struct jz4780lcd_enh_info)))
			return -EFAULT;

		jz4780fb_set_enhancment_value(&uenhinfo, cfb);
		break;

#ifdef CONFIG_JZ_AOSDC
	case FBIO_FG0_COMPRESS_EN:
		if (copy_from_user(&comp_en, argp, sizeof(int)))
			return -EFAULT;

		aosd_info = cfb->aosd_info;
		compress_wq = cfb->compress_wq;

		if(comp_en){
			D("set fg0 compress enable \n");
			cfb->fg0_use_compress_decompress_mode = 1;
		}else{
			D("set fg0 compress disable \n");
			cfb->fg0_use_compress_decompress_mode = 0;
		}

		break;
	case FBIO_FG1_COMPRESS_EN:
		if (copy_from_user(&comp_en, argp, sizeof(int)))
			return -EFAULT;

		aosd_info = cfb->aosd_info;
		compress_wq = cfb->compress_wq;

		if(comp_en){
			D("set fg1 compress enable \n");
			cfb->fg1_use_compress_decompress_mode = 1;
		}else{
			D("set fg1 compress disable \n");
			cfb->fg1_use_compress_decompress_mode = 0;
		}
		break;
#endif
	case FBIO_FG0_16x16_BLOCK_EN:
		if (copy_from_user(&x2d_en, argp, sizeof(int)))
			return -EFAULT;
		if(x2d_en){
			D("set fg0 x2d block enable \n");
			cfb->fg0_use_x2d_block_mode = 1;
		}else{
			D("set fg0 x2d disable \n");
			cfb->fg0_use_x2d_block_mode = 0;
		}
		break;

	case FBIO_FG1_16x16_BLOCK_EN:
		if (copy_from_user(&x2d_en, argp, sizeof(int)))
			return -EFAULT;
		if(x2d_en){
			D("set fg1 x2d block enable \n");
			cfb->fg1_use_x2d_block_mode = 1;
		}else{
			D("set fg1 x2d block disable \n");
			cfb->fg1_use_x2d_block_mode = 0;
		}
		break;

	case FBIO_MODE_SWITCH:
		D("FBIO_MODE_SWITCH");
		switch (arg) {
#ifdef CONFIG_FB_JZ4780_TVE
			case PANEL_MODE_TVE_PAL: 	/* switch to TVE_PAL mode */
			case PANEL_MODE_TVE_NTSC: 	/* switch to TVE_NTSC mode */
				jz4780lcd_info_switch_to_TVE(arg);
				jz4780tve_init(arg); /* tve controller init */
				udelay(100);
				cpm_start_clock(CGM_TVE);
				jz4780tve_enable_tve();
				REG_TVE_CTRL |= (1 << 21);
				/* turn off lcd backlight */
				screen_off(cfb->id);
				break;
#endif
#if defined(CONFIG_JZ4780_HDMI_DISPLAY)  
 			case PANEL_MODE_HDMI_480P:
				set_i2s_external_codec();
				jz4780_lcd_info =&jz4780_info_hdmi_480p ;
				screen_off(cfb->id);
				break;

			case PANEL_MODE_HDMI_576P:
				set_i2s_external_codec();
				jz4780_lcd_info =&jz4780_info_hdmi_576p ;
				screen_off(cfb->id);
				break;

			case PANEL_MODE_HDMI_720P50:
				set_i2s_external_codec();
				jz4780_lcd_info =&jz4780_info_hdmi_720p50 ;
				screen_off(cfb->id);
				break;

			case PANEL_MODE_HDMI_720P60:
   				set_i2s_external_codec();
				jz4780_lcd_info =&jz4780_info_hdmi_720p60 ;
				screen_off(cfb->id);
				break;

			case PANEL_MODE_HDMI_1080P30:
                                set_i2s_external_codec();
                                jz4780_lcd_info =&jz4780_info_hdmi_1080p30;
                                screen_off(cfb->id);
                                break;

                        case PANEL_MODE_HDMI_1080P50:
                                set_i2s_external_codec();
                                jz4780_lcd_info =&jz4780_info_hdmi_1080p50;
                                screen_off(cfb->id);
                                break;

			case PANEL_MODE_HDMI_1080P60:
   				set_i2s_external_codec();
				jz4780_lcd_info =&jz4780_info_hdmi_1080p60;
				screen_off(cfb->id);
				break;
#endif	//CONFIG_JZ4780_HDMI_DISPLAY
			case PANEL_MODE_LCD_PANEL: 	/* switch to LCD mode */
			default :
				/* turn off TVE, turn off DACn... */
#ifdef CONFIG_FB_JZ4780_TVE
				jz4780tve_disable_tve();
				cpm_stop_clock(CGM_TVE);
#endif
				cfb->jz4780_lcd_info = &jz4780_lcd_panel;
				/* turn on lcd backlight */
				screen_on(cfb->id);
				break;
		}
		jz4780fb_deep_set_mode(cfb);
#if JZ_FB_DEBUG
		display_h_color_bar(cfb->jz4780_lcd_info->osd.fg0.w, cfb->jz4780_lcd_info->osd.fg0.h, cfb->jz4780_lcd_info->osd.fg0.bpp, (int *)cfb->lcd_frame0);
#endif
		break;

#ifdef CONFIG_FB_JZ4780_TVE
	case FBIO_GET_TVE_MODE:
		D("fbio get TVE mode\n");
		if (copy_to_user(argp, jz4780_tve_info, sizeof(struct jz4780tve_info)))
			return -EFAULT;
		break;
	case FBIO_SET_TVE_MODE:
		D("fbio set TVE mode\n");
		if (copy_from_user(jz4780_tve_info, argp, sizeof(struct jz4780tve_info)))
			return -EFAULT;
		/* set tve mode */
		jz4780tve_set_tve_mode(jz4780_tve_info);
		break;
#endif
	default:
		printk("%s, unknown command(0x%x)", __FILE__, cmd);
		break;
	}

	return ret;
}

/* Use mmap /dev/fb can only get a non-cacheable Virtual Address. */
static int jz4780fb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	unsigned long start;
	unsigned long off;
	u32 len;
	D("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
	off = vma->vm_pgoff << PAGE_SHIFT;
	//fb->fb_get_fix(&fix, PROC_CONSOLE(info), info);

	/* frame buffer memory */
	start = cfb->fb.fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + cfb->fb.fix.smem_len);
	start &= PAGE_MASK;
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;

	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);	/* Uncacheable */

#if 1
 	pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
 	pgprot_val(vma->vm_page_prot) |= _CACHE_UNCACHED;		/* Uncacheable */
//	pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NONCOHERENT;	/* Write-Back */
#endif

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

/* checks var and eventually tweaks it to something supported,
 * DO NOT MODIFY PAR */
static int jz4780fb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	printk("jz4780fb_check_var, not implement\n");
	return 0;
}


/*
 * set the video mode according to info->var
 */
static int jz4780fb_set_par(struct fb_info *info)
{
	printk("jz4780fb_set_par, not implemented\n");
	return 0;
}


/*
 * (Un)Blank the display.
 * Fix me: should we use VESA value?
 */
static int jz4780fb_blank(int blank_mode, struct fb_info *info)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;

	D("jz4780 fb_blank %d %p", blank_mode, info);
	switch (blank_mode) {
	case FB_BLANK_UNBLANK:
		//case FB_BLANK_NORMAL:
			/* Turn on panel */
		__lcd_set_ena(cfb->id);
		screen_on(cfb->id);
		printk("FB_BLANK_UNBLANK\n");
		break;

	case FB_BLANK_NORMAL:
		printk("FB_BLANK_NORMAL\n");
	case FB_BLANK_VSYNC_SUSPEND:
		printk("FB_BLANK_VSYNC_SUSPEND\n");
	case FB_BLANK_HSYNC_SUSPEND:
		printk("FB_BLANK_HSYNC_SUSPEND\n");
	case FB_BLANK_POWERDOWN:
		printk("FB_BLANK_POWERDOWN\n");		
#if 0
		/* Turn off panel */
		__lcd_display_off();
		__lcd_set_dis();
#endif
		break;
	default:
		break;

	}
	return 0;
}

/*
 * pan display
 */
static int jz4780fb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	int dy;
	struct fb_info *fb = info;

	printk("%s, %d\n",__func__,__LINE__);
	if (!var || !cfb) {
		return -EINVAL;
	}

	if (var->xoffset - cfb->fb.var.xoffset) {
		/* No support for X panning for now! */
		return -EINVAL;
	}

	dy = var->yoffset;// - fb->var.yoffset;

	if (dy) {
		cfb->dma0_desc0->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame0 + (fb->fix.line_length * dy));
	}
	else {
		cfb->dma0_desc0->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame0);
	}
	return 0;
}


/* use default function cfb_fillrect, cfb_copyarea, cfb_imageblit */
static struct fb_ops jz4780fb_ops = {
	.owner			= THIS_MODULE,
	.fb_setcolreg		= jz4780fb_setcolreg,
	.fb_check_var 		= jz4780fb_check_var,
	.fb_set_par 		= jz4780fb_set_par,
	.fb_blank		= jz4780fb_blank,
	.fb_pan_display		= jz4780fb_pan_display,
	.fb_fillrect		= cfb_fillrect,
	.fb_copyarea		= cfb_copyarea,
	.fb_imageblit		= cfb_imageblit,
	.fb_mmap		= jz4780fb_mmap,
	.fb_ioctl		= jz4780fb_ioctl,
};

static int jz4780fb_set_var(struct fb_var_screeninfo *var, int con,
			struct fb_info *info)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	struct jz4780lcd_info *lcd_info = cfb->jz4780_lcd_info;
	int chgvar = 0;

	var->height	            = lcd_info->osd.fg0.h;	/* tve mode */
	var->width	            = lcd_info->osd.fg0.w;
	var->bits_per_pixel	    = lcd_info->osd.fg0.bpp;

	var->vmode                  = FB_VMODE_NONINTERLACED;
	var->activate               = cfb->fb.var.activate;
	var->xres                   = var->width;
	var->yres                   = var->height;
	var->xres_virtual           = var->width;
	var->yres_virtual           = var->height;
	var->xoffset                = 0;
	var->yoffset                = 0;
	var->pixclock               = 0;
	var->left_margin            = 0;
	var->right_margin           = 0;
	var->upper_margin           = 0;
	var->lower_margin           = 0;
	var->hsync_len              = 0;
	var->vsync_len              = 0;
	var->sync                   = 0;
	var->activate              &= ~FB_ACTIVATE_TEST;

	/*
	 * CONUPDATE and SMOOTH_XPAN are equal.  However,
	 * SMOOTH_XPAN is only used internally by fbcon.
	 */
	if (var->vmode & FB_VMODE_CONUPDATE) {
		var->vmode |= FB_VMODE_YWRAP;
		var->xoffset = cfb->fb.var.xoffset;
		var->yoffset = cfb->fb.var.yoffset;
	}

	if (var->activate & FB_ACTIVATE_TEST)
		return 0;

	if ((var->activate & FB_ACTIVATE_MASK) != FB_ACTIVATE_NOW)
		return -EINVAL;

	if (cfb->fb.var.xres != var->xres)
		chgvar = 1;
	if (cfb->fb.var.yres != var->yres)
		chgvar = 1;
	if (cfb->fb.var.xres_virtual != var->xres_virtual)
		chgvar = 1;
	if (cfb->fb.var.yres_virtual != var->yres_virtual)
		chgvar = 1;
	if (cfb->fb.var.bits_per_pixel != var->bits_per_pixel)
		chgvar = 1;

	//display = fb_display + con;

	var->red.msb_right	= 0;
	var->green.msb_right	= 0;
	var->blue.msb_right	= 0;

	switch(var->bits_per_pixel){
	case 1:	/* Mono */
		cfb->fb.fix.visual	= FB_VISUAL_MONO01;
		cfb->fb.fix.line_length	= (var->xres * var->bits_per_pixel) / 8;
		break;
	case 2:	/* Mono */
		var->red.offset		= 0;
		var->red.length		= 2;
		var->green.offset	= 0;
		var->green.length	= 2;
		var->blue.offset	= 0;
		var->blue.length	= 2;

		cfb->fb.fix.visual	= FB_VISUAL_PSEUDOCOLOR;
		cfb->fb.fix.line_length	= (var->xres * var->bits_per_pixel) / 8;
		break;
	case 4:	/* PSEUDOCOLOUR*/
		var->red.offset		= 0;
		var->red.length		= 4;
		var->green.offset	= 0;
		var->green.length	= 4;
		var->blue.offset	= 0;
		var->blue.length	= 4;

		cfb->fb.fix.visual	= FB_VISUAL_PSEUDOCOLOR;
		cfb->fb.fix.line_length	= var->xres / 2;
		break;
	case 8:	/* PSEUDOCOLOUR, 256 */
		var->red.offset		= 0;
		var->red.length		= 8;
		var->green.offset	= 0;
		var->green.length	= 8;
		var->blue.offset	= 0;
		var->blue.length	= 8;

		cfb->fb.fix.visual	= FB_VISUAL_PSEUDOCOLOR;
		cfb->fb.fix.line_length	= var->xres ;
		break;
	case 15: /* DIRECTCOLOUR, 32k */
		var->bits_per_pixel	= 15;
		var->red.offset		= 10;
		var->red.length		= 5;
		var->green.offset	= 5;
		var->green.length	= 5;
		var->blue.offset	= 0;
		var->blue.length	= 5;

		cfb->fb.fix.visual	= FB_VISUAL_DIRECTCOLOR;
		cfb->fb.fix.line_length	= var->xres_virtual * 2;
		break;
	case 16: /* DIRECTCOLOUR, 64k */
		var->bits_per_pixel	= 16;
		var->red.offset		= 11;
		var->red.length		= 5;
		var->green.offset	= 5;
		var->green.length	= 6;
		var->blue.offset	= 0;
		var->blue.length	= 5;

		cfb->fb.fix.visual	= FB_VISUAL_TRUECOLOR;
		cfb->fb.fix.line_length	= var->xres_virtual * 2;
		break;
	case 17 ... 32:
		/* DIRECTCOLOUR, 256 */
		var->bits_per_pixel	= 32;

		var->red.offset		= 16;
		var->red.length		= 8;
		var->green.offset	= 8;
		var->green.length	= 8;
		var->blue.offset	= 0;
		var->blue.length	= 8;
		var->transp.offset  	= 24;
		var->transp.length 	= 8;

		cfb->fb.fix.visual	= FB_VISUAL_TRUECOLOR;
		cfb->fb.fix.line_length	= var->xres_virtual * 4;
		break;

	default: /* in theory this should never happen */
		printk(KERN_WARNING "%s: don't support for %dbpp\n",
		       cfb->fb.fix.id, var->bits_per_pixel);
		break;
	}

	cfb->fb.var = *var;
	cfb->fb.var.activate &= ~FB_ACTIVATE_ALL;

	/*
	 * Update the old var.  The fbcon drivers still use this.
	 * Once they are using cfb->fb.var, this can be dropped.
	 *					--rmk
	 */
	//display->var = cfb->fb.var;
	/*
	 * If we are setting all the virtual consoles, also set the
	 * defaults used to create new consoles.
	 */
//	fb_set_cmap(&cfb->fb.cmap, &cfb->fb);

	return 0;
}

static struct lcd_cfb_info * jz4780fb_set_fb_info(struct lcd_cfb_info *cfb)
{


	cfb->backlight_level		= LCD_DEFAULT_BACKLIGHT;

	strcpy(cfb->fb.fix.id, "jz-lcd");
	cfb->fb.fix.type	= FB_TYPE_PACKED_PIXELS;
	cfb->fb.fix.type_aux	= 0;
	cfb->fb.fix.xpanstep	= 1;
	cfb->fb.fix.ypanstep	= 1;
	cfb->fb.fix.ywrapstep	= 0;
	cfb->fb.fix.accel	= FB_ACCEL_NONE;

	cfb->fb.var.nonstd	= 0;
	cfb->fb.var.activate	= FB_ACTIVATE_NOW;
	cfb->fb.var.height	= -1;
	cfb->fb.var.width	= -1;
	cfb->fb.var.accel_flags	= FB_ACCELF_TEXT;

	cfb->fb.fbops		= &jz4780fb_ops;
	cfb->fb.flags		= FBINFO_FLAG_DEFAULT;

	cfb->fb.pseudo_palette	= (void *)(cfb + 1);

	switch (cfb->jz4780_lcd_info->osd.fg0.bpp) {
	case 1:
		fb_alloc_cmap(&cfb->fb.cmap, 4, 0);
		break;
	case 2:
		fb_alloc_cmap(&cfb->fb.cmap, 8, 0);
		break;
	case 4:
		fb_alloc_cmap(&cfb->fb.cmap, 32, 0);
		break;
	case 8:

	default:
		fb_alloc_cmap(&cfb->fb.cmap, 256, 0);
		break;
	}
	D("fb_alloc_cmap,fb.cmap.len:%d....\n", cfb->fb.cmap.len);

	return cfb;
}

static int bpp_to_data_bpp(int tbpp)
{
	int bpp = tbpp;
	switch (bpp) {
		case 1:
		case 2:
		case 4:
		case 8:
		case 16:
		case 32:
			break;

		case 15:
			bpp = 16;
			break;
		case 30:
		case 24:
			bpp = 32;
			break;
		case 36:
			bpp = 24;
			break;
		default:
			bpp = -EINVAL;
	}

	return bpp;
}

/*
 * Map screen memory
 */
static int jz4780fb_map_smem(struct lcd_cfb_info *cfb)
{
	unsigned long page;
	int line_length;
	unsigned int page_shift, needroom, needroom1, needroom2, bpp, w, h;

	bpp = 32;
	D("FG0 BPP: %d, Data BPP: %d.", cfb->jz4780_lcd_info->osd.fg0.bpp, bpp);

	w = cfb->jz4780_lcd_info->panel.w;
	h = cfb->jz4780_lcd_info->panel.h;

	/* FrameBuffer Size */
	line_length = (w * bpp) >> 3;
	printk("w:%d, bpp:%d\t, line_length:%d\n",w, bpp,line_length);
	line_length = (line_length + (FB_ALIGN-1)) & (~(FB_ALIGN-1));
	needroom1 = needroom = line_length * h;
	
#ifdef CONFIG_JZ_AOSDC
	/* AOSD buffer Size */
	line_length = ((w * bpp) >> 3) >> 2;
	line_length = (line_length + ((line_length + 63)/64));
	needroom2 = (line_length << 2) * h;
#endif

#if defined(CONFIG_FB_JZ4780_LCD_USE_2LAYER_FRAMEBUFFER)
	needroom *= 2;
#endif // two layer

	printk("line_length :%x\t, needroom: %x\n",line_length, needroom);
	page_shift = get_order(needroom);
	printk("page_shift:%d\n",page_shift);

	cfb->lcd_palette = (unsigned char *)__get_free_pages(GFP_KERNEL, 0);
	cfb->lcd_frame0 = (unsigned char *)__get_free_pages(GFP_KERNEL, page_shift);

	if ((!cfb->lcd_palette) || (!cfb->lcd_frame0)){
		printk("Not enough memory for lcd,CONFIG_FORCE_MAX_ZONEORDER:%d \n",CONFIG_FORCE_MAX_ZONEORDER);
		return -ENOMEM;
	}
	memset((void *)cfb->lcd_palette, 0, PAGE_SIZE);
	memset((void *)cfb->lcd_frame0, 0, PAGE_SIZE << page_shift);

	cfb->dma_desc_base = (struct jz4780_lcd_dma_desc *)((void*)cfb->lcd_palette + ((PALETTE_SIZE+3)/4)*4);

#if defined(CONFIG_FB_JZ4780_SLCD)
	cfb->lcd_cmdbuf = (unsigned char *)__get_free_pages(GFP_KERNEL, 0);
	memset((void *)cfb->lcd_cmdbuf, 0, PAGE_SIZE);

	{	int data, i, *ptr;
		ptr = (unsigned int *)cfb->lcd_cmdbuf;
		data = WR_GRAM_CMD;
		data = ((data & 0xff) << 1) | ((data & 0xff00) << 2);
		for(i = 0; i < 3; i++){
			ptr[i] = data;
		}
	}
#endif

#if defined(CONFIG_FB_JZ4780_LCD_USE_2LAYER_FRAMEBUFFER)
	cfb->lcd_frame1 = cfb->lcd_frame0 + needroom1;
	printk("lcd_frame0:%p, lcd_frame1:%p\n",cfb->lcd_frame0,cfb->lcd_frame1);
#endif

	/*
	 * Set page reserved so that mmap will work. This is necessary
	 * since we'll be remapping normal memory.
	 */
	page = (unsigned long)cfb->lcd_palette;
	SetPageReserved(virt_to_page((void*)page));

	for (page = (unsigned long)cfb->lcd_frame0;
	     page < PAGE_ALIGN((unsigned long)cfb->lcd_frame0 + (PAGE_SIZE<<page_shift));
	     page += PAGE_SIZE) {
		SetPageReserved(virt_to_page((void*)page));
	}

	cfb->fb.fix.smem_start = virt_to_phys((void *)cfb->lcd_frame0);
	cfb->fb.fix.smem_len = (PAGE_SIZE << page_shift); /* page_shift/2 ??? */
	cfb->fb.screen_base =
		(unsigned char *)(((unsigned int)cfb->lcd_frame0&0x1fffffff) | 0xa0000000);

	if (!cfb->fb.screen_base) {
		printk("jz4780fb, %s: unable to map screen memory\n", cfb->fb.fix.id);
		return -ENOMEM;
	}

#ifdef CONFIG_JZ_AOSDC
	/* Alloc memory for compress function*/
	page_shift = get_order(needroom2);

	cfb->buf_comp00 = (unsigned char *)__get_free_pages(GFP_KERNEL, page_shift);
	cfb->buf_comp01 = (unsigned char *)__get_free_pages(GFP_KERNEL, page_shift);
	cfb->buf_comp10 = (unsigned char *)__get_free_pages(GFP_KERNEL, page_shift);
	cfb->buf_comp11 = (unsigned char *)__get_free_pages(GFP_KERNEL, page_shift);
	if (!cfb->buf_comp00 || !cfb->buf_comp01 || !cfb->buf_comp10 || !cfb->buf_comp11) {
		printk("<< no memory for buf_comp >>\n");
		return -ENOMEM;
	}

	printk("buf_comp00:%p\t, buf_comp01:%p\t, buf_comp10:%p\t, buf_comp11:%p\n",cfb->buf_comp00,cfb->buf_comp01, cfb->buf_comp10, cfb->buf_comp11);

	memset((void *)cfb->buf_comp00, 0, PAGE_SIZE << page_shift);
	memset((void *)cfb->buf_comp01, 0, PAGE_SIZE << page_shift);
	memset((void *)cfb->buf_comp10, 0, PAGE_SIZE << page_shift);
	memset((void *)cfb->buf_comp11, 0, PAGE_SIZE << page_shift);

	for (page = (unsigned long)cfb->buf_comp00;
	     page < PAGE_ALIGN((unsigned long)cfb->buf_comp00 + (PAGE_SIZE<<page_shift));
	     page += PAGE_SIZE) {
		SetPageReserved(virt_to_page((void*)page));
	}

	for (page = (unsigned long)cfb->buf_comp01;
	     page < PAGE_ALIGN((unsigned long)cfb->buf_comp01 + (PAGE_SIZE<<page_shift));
	     page += PAGE_SIZE) {
		SetPageReserved(virt_to_page((void*)page));
	}

	for (page = (unsigned long)cfb->buf_comp10;
	     page < PAGE_ALIGN((unsigned long)cfb->buf_comp10 + (PAGE_SIZE<<page_shift));
	     page += PAGE_SIZE) {
		SetPageReserved(virt_to_page((void*)page));
	}

	for (page = (unsigned long)cfb->buf_comp11;
	     page < PAGE_ALIGN((unsigned long)cfb->buf_comp11 + (PAGE_SIZE<<page_shift));
	     page += PAGE_SIZE) {
		SetPageReserved(virt_to_page((void*)page));
	}

	cfb->aosd_page_shift = page_shift;
#endif

	return 0;
}

static void jz4780fb_free_fb_info(struct lcd_cfb_info *cfb)
{
	if (cfb) {
		fb_alloc_cmap(&cfb->fb.cmap, 0, 0);
		kfree(cfb);
	}
}

static void jz4780fb_unmap_smem(struct lcd_cfb_info *cfb)
{
	struct page * map = NULL;
	unsigned char *tmp;
	unsigned int page_shift, needroom, bpp, w, h;

	bpp = cfb->jz4780_lcd_info->osd.fg0.bpp;
	if ( bpp == 18 || bpp == 24)
		bpp = 32;
	if ( bpp == 15 )
		bpp = 16;
	w = cfb->jz4780_lcd_info->osd.fg0.w;
	h = cfb->jz4780_lcd_info->osd.fg0.h;
	needroom = ((w * bpp + 7) >> 3) * h;
#if defined(CONFIG_FB_JZ4780_LCD_USE_2LAYER_FRAMEBUFFER)
	bpp = cfb->jz4780_lcd_info->osd.fg1.bpp;
	if ( bpp == 18 || bpp == 24)
		bpp = 32;
	if ( bpp == 15 )
		bpp = 16;
	w = cfb->jz4780_lcd_info->osd.fg1.w;
	h = cfb->jz4780_lcd_info->osd.fg1.h;
	needroom += ((w * bpp + 7) >> 3) * h;
#endif

	for (page_shift = 0; page_shift < 12; page_shift++)
		if ((PAGE_SIZE << page_shift) >= needroom)
			break;

	if (cfb && cfb->fb.screen_base) {
		iounmap(cfb->fb.screen_base);
		cfb->fb.screen_base = NULL;
		release_mem_region(cfb->fb.fix.smem_start,
				   cfb->fb.fix.smem_len);
	}

	if (cfb->lcd_palette) {
		map = virt_to_page(cfb->lcd_palette);
		clear_bit(PG_reserved, &map->flags);
		free_pages((int)cfb->lcd_palette, 0);
	}

	if (cfb->lcd_frame0) {
		for (tmp=(unsigned char *)cfb->lcd_frame0;
		     tmp < cfb->lcd_frame0 + (PAGE_SIZE << page_shift);
		     tmp += PAGE_SIZE) {
			map = virt_to_page(tmp);
			clear_bit(PG_reserved, &map->flags);
		}
		free_pages((int)cfb->lcd_frame0, page_shift);
	}
}

/* initial dma descriptors */
static void jz4780fb_descriptor_init(struct lcd_cfb_info* cfb)
{
	struct jz4780lcd_info * lcd_info = cfb->jz4780_lcd_info;
	unsigned int pal_size;
	int n = cfb->id;

	switch ( lcd_info->osd.fg0.bpp ) {
	case 1:
		pal_size = 4;
		break;
	case 2:
		pal_size = 8;
		break;
	case 4:
		pal_size = 32;
		break;
	case 8:
	default:
		pal_size = 512;
	}

	pal_size /= 4;

	cfb->dma0_desc_palette 	= cfb->dma_desc_base + 0;
	cfb->dma0_desc0 		= cfb->dma_desc_base + 1;
	cfb->dma0_desc1 		= cfb->dma_desc_base + 2;
	cfb->dma0_desc_cmd0 		= cfb->dma_desc_base + 3; /* use only once */
	cfb->dma0_desc_cmd 		= cfb->dma_desc_base + 4;
	cfb->dma1_desc0 		= cfb->dma_desc_base + 5;
	cfb->dma1_desc1 		= cfb->dma_desc_base + 6;
	cfb->dma0_tv_desc0           = cfb->dma_desc_base + 7;
	cfb->dma0_tv_desc1           = cfb->dma_desc_base + 8;
	cfb->dma1_tv_desc0           = cfb->dma_desc_base + 9;
	cfb->dma1_tv_desc1           = cfb->dma_desc_base + 10;

	/*
	 * Normal TFT panel's DMA Chan0:
	 *	TO LCD Panel:
	 * 		no palette:	dma0_desc0 <<==>> dma0_desc0
	 * 		palette :	dma0_desc_palette <<==>> dma0_desc0
	 *	TO TV Encoder:
	 * 		no palette:	dma0_desc0 <<==>> dma0_desc1
	 * 		palette:	dma0_desc_palette --> dma0_desc0
	 * 				--> dma0_desc1 --> dma0_desc_palette --> ...
	 *
	 * SMART LCD TFT panel(dma0_desc_cmd)'s DMA Chan0:
	 *	TO LCD Panel:
	 * 		no palette:	dma0_desc_cmd <<==>> dma0_desc0
	 * 		palette :	dma0_desc_palette --> dma0_desc_cmd
	 * 				--> dma0_desc0 --> dma0_desc_palette --> ...
	 *	TO TV Encoder:
	 * 		no palette:	dma0_desc_cmd --> dma0_desc0
	 * 				--> dma0_desc1 --> dma0_desc_cmd --> ...
	 * 		palette:	dma0_desc_palette --> dma0_desc_cmd
	 * 				--> dma0_desc0 --> dma0_desc1
	 * 				--> dma0_desc_palette --> ...
	 * DMA Chan1:
	 *	TO LCD Panel:
	 * 		dma1_desc0 <<==>> dma1_desc0
	 *	TO TV Encoder:
	 * 		dma1_desc0 <<==>> dma1_desc1
	 */

#if defined(CONFIG_FB_JZ4780_SLCD)
	/* First CMD descriptors, use only once, cmd_num isn't 0 */
	cfb->dma0_desc_cmd0->next_desc 	= (unsigned int)virt_to_phys(cfb->dma0_desc0);
	cfb->dma0_desc_cmd0->databuf 	= (unsigned int)virt_to_phys((void *)cfb->lcd_cmdbuf);
	cfb->dma0_desc_cmd0->frame_id 	= (unsigned int)0x0da0cad0; /* dma0's cmd0 */
	cfb->dma0_desc_cmd0->cmd 		= LCD_CMD_CMD | 2 | LCD_CMD_FRM_EN; /* command */
	cfb->dma0_desc_cmd0->offsize 	= 0;
	cfb->dma0_desc_cmd0->page_width 	= 0;
	cfb->dma0_desc_cmd0->cmd_num 	= 2;  //3;

	/* Dummy Command Descriptor, cmd_num is 0 */
	cfb->dma0_desc_cmd->next_desc 	= (unsigned int)virt_to_phys(cfb->dma0_desc0);
	cfb->dma0_desc_cmd->databuf 		= 0;
	cfb->dma0_desc_cmd->frame_id 	= (unsigned int)0x0da000cd; /* dma0's cmd0 */
	cfb->dma0_desc_cmd->cmd 		= LCD_CMD_CMD | 0 | LCD_CMD_FRM_EN; /* dummy command */
	cfb->dma0_desc_cmd->cmd_num 		= 0;
	cfb->dma0_desc_cmd->offsize 		= 0;
	cfb->dma0_desc_cmd->page_width 	= 0;

	/* Palette Descriptor */
	cfb->dma0_desc_palette->next_desc 	= (unsigned int)virt_to_phys(cfb->dma0_desc_cmd0);
#else
	/* Palette Descriptor */
	cfb->dma0_desc_palette->next_desc 	= (unsigned int)virt_to_phys(cfb->dma0_desc0);
#endif
	cfb->dma0_desc_palette->databuf 	= (unsigned int)virt_to_phys((void *)cfb->lcd_palette);
	cfb->dma0_desc_palette->frame_id 	= (unsigned int)0xaaaaaaaa;
	cfb->dma0_desc_palette->cmd 		= LCD_CMD_PAL | pal_size; /* Palette Descriptor */

	/* DMA0 Descriptor0 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) /* TVE mode */
		cfb->dma0_desc0->next_desc 	= (unsigned int)virt_to_phys(cfb->dma0_desc1);
	else{			/* Normal TFT LCD */
#if defined(CONFIG_FB_JZ4780_SLCD)
			cfb->dma0_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc_cmd);
#else
			cfb->dma0_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc0);
#endif
	}

	cfb->dma0_desc0->databuf = virt_to_phys((void *)cfb->lcd_frame0);
	cfb->dma0_desc0->frame_id = (unsigned int)0x0000da00; /* DMA0'0 */
	cfb->dma0_desc0->cmd_num = 0;
	cfb->dma0_desc0->cmd = 0;
	cfb->dma0_desc0->page_width = 0;
	cfb->dma0_desc0->offsize = 0;

	/* DMA0 Descriptor1 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		cfb->dma0_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc0);
	}else{
#if defined(CONFIG_FB_JZ4780_SLCD)  /* for smatlcd */
		cfb->dma0_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc_cmd);
#else
		cfb->dma0_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc1);
#endif
	}
	cfb->dma0_desc1->frame_id = (unsigned int)0x0000da01; /* DMA0'1 */
	cfb->dma0_desc1->cmd_num = 0;
	cfb->dma0_desc1->page_width = 0;
	cfb->dma0_desc1->offsize = 0;

	if (lcd_info->osd.fg0.bpp <= 8) /* load palette only once at setup */
		REG_LCD_DA0(n) = virt_to_phys(cfb->dma0_desc_palette);
	else {
#if defined(CONFIG_FB_JZ4780_SLCD)  /* for smartlcd */
		REG_LCD_DA0(n) = virt_to_phys(cfb->dma0_desc_cmd0); //smart lcd
#else
		REG_LCD_DA0(n) = virt_to_phys(cfb->dma0_desc0); //tft
#endif
	}

	/* DMA1 Descriptor0 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) /* TVE mode */
		cfb->dma1_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma1_desc1);
	else			/* Normal TFT LCD */
		cfb->dma1_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma1_desc0);

	cfb->dma1_desc0->databuf = virt_to_phys((void *)cfb->lcd_frame1);
	cfb->dma1_desc0->frame_id = (unsigned int)0x0000da10; /* DMA1'0 */
	cfb->dma1_desc0->cmd_num = 0;
	cfb->dma1_desc0->page_width = 0;
	cfb->dma1_desc0->offsize = 0;


	/* DMA1 Descriptor1 */
	if ( lcd_info->panel.cfg & LCD_CFG_TVEN ) { /* TVE mode */
		cfb->dma1_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma1_desc0);
	}else {
		cfb->dma1_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma1_desc1);
	}
	cfb->dma1_desc1->frame_id = (unsigned int)0x0000da11; /* DMA1'1 */
	cfb->dma1_desc1->cmd_num = 0;
	cfb->dma1_desc1->page_width = 0;
	cfb->dma1_desc1->offsize = 0;
	
	REG_LCD_DA1(n) = virt_to_phys(cfb->dma1_desc0);	/* set Dma-chan1's Descripter Addrress */

	/*tmp tv Descripter*/
	cfb->dma0_tv_desc0->frame_id = (unsigned int)0xab00da00; /* DMA0'0 */
	cfb->dma0_tv_desc1->frame_id = (unsigned int)0xab00da01; /* DMA0'0 */
	cfb->dma1_tv_desc0->frame_id = (unsigned int)0xab00da10; /* DMA0'0 */
	cfb->dma1_tv_desc1->frame_id = (unsigned int)0xab00da11; /* DMA0'0 */

	dma_cache_wback_inv((unsigned int)(cfb->dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz4780_lcd_dma_desc));
}

static void jz4780fb_set_panel_mode(struct lcd_cfb_info *cfb)
{
	struct jz4780lcd_info * lcd_info  = cfb->jz4780_lcd_info;
	struct jz4780lcd_panel_t *panel = &lcd_info->panel;
	int n = cfb->id;

	lcd_info->panel.cfg |= LCD_CFG_NEWDES; /* use 8words descriptor always */

	printk("lcd:%d, Line:%d, func:%s\n",n,__LINE__,__func__);
	printk("LCD_CTRL(%d):%x",n,LCD_CTRL(n));
	REG_LCD_CTRL(n) = lcd_info->panel.ctrl; /* LCDC Controll Register */
	printk("lcd:%d, Line:%d, func:%s\n",n,__LINE__,__func__);
	REG_LCD_CFG(n) = lcd_info->panel.cfg; /* LCDC Configure Register */

	if ( lcd_info->panel.cfg & LCD_CFG_LCDPIN_SLCD ) /* enable Smart LCD DMA */
		REG_SLCD_CFG = lcd_info->panel.slcd_cfg; /* Smart LCD Configure Register */
	//	REG_SLCD_CTRL = SLCD_CTRL_DMA_EN;

	switch ( lcd_info->panel.cfg & LCD_CFG_MODE_MASK ) {
	case LCD_CFG_MODE_GENERIC_TFT:
	case LCD_CFG_MODE_INTER_CCIR656:
	case LCD_CFG_MODE_NONINTER_CCIR656:
	case LCD_CFG_MODE_SLCD:
	default:		/* only support TFT16 TFT32, not support STN and Special TFT by now(10-06-2008)*/
		REG_LCD_VAT(n) = (((panel->blw + panel->w + panel->elw + panel->hsw)) << 16) | (panel->vsw + panel->bfw + panel->h + panel->efw);
		REG_LCD_DAH(n) = ((panel->hsw + panel->blw) << 16) | (panel->hsw + panel->blw + panel->w);
		REG_LCD_DAV(n) = ((panel->vsw + panel->bfw) << 16) | (panel->vsw + panel->bfw + panel->h);
		REG_LCD_HSYNC(n) = (0 << 16) | panel->hsw;
		REG_LCD_VSYNC(n) = (0 << 16) | panel->vsw;
		break;
	}
}


static void jz4780fb_set_osd_mode(struct lcd_cfb_info *cfb)
{
	struct jz4780lcd_info * lcd_info = cfb->jz4780_lcd_info;
	int n = cfb->id;

	REG_LCD_OSDC(n) 	= lcd_info->osd.osd_cfg | (1 << 16); 
	REG_LCD_RGBC(n)  	= lcd_info->osd.rgb_ctrl;
	REG_LCD_BGC0(n)  	= lcd_info->osd.bgcolor0;
	REG_LCD_BGC1(n)  	= lcd_info->osd.bgcolor1;
	REG_LCD_KEY0(n) 	= lcd_info->osd.colorkey0;
	REG_LCD_KEY1(n) 	= lcd_info->osd.colorkey1;
	REG_LCD_IPUR(n) 	= lcd_info->osd.ipu_restart;
}

static void set_bpp(int *bpp, struct jz4780_lcd_dma_desc *dma_desc){

	dma_desc->cmd_num = (dma_desc->cmd_num & ~LCD_CPOS_BPP_MASK); 
	
	if (*bpp == 1 )
		dma_desc->cmd_num |= LCD_CPOS_BPP_1;
	else if (*bpp == 2 )
		dma_desc->cmd_num |= LCD_CPOS_BPP_2;
	else if (*bpp == 4 )
		dma_desc->cmd_num |= LCD_CPOS_BPP_4;
	else if (*bpp == 8 )
		dma_desc->cmd_num |= LCD_CPOS_BPP_8;
	else if (*bpp == 15 )
		dma_desc->cmd_num |= (LCD_CPOS_BPP_15_16 | LCD_CPOS_RGB555);
	else if (*bpp == 16 ){
		dma_desc->cmd_num |= (LCD_CPOS_BPP_15_16);
		dma_desc->cmd_num &= ~(LCD_CPOS_RGB555);
	}else if (*bpp == 18 || *bpp == 24 ) {
		dma_desc->cmd_num |= LCD_CPOS_BPP_18_24;
	}else if(*bpp == 36){
		dma_desc->cmd_num |= LCD_CPOS_BPP_CMPS_24;
	}else if(*bpp == 30){
		dma_desc->cmd_num |= LCD_CPOS_BPP_30;
	}else {
		printk("The BPP %d is not supported %d\n", *bpp,__LINE__);
		dma_desc->cmd_num |= LCD_CPOS_BPP_18_24;
	}

	printk("cmd_num:%x\n",dma_desc->cmd_num);
}

static void set_alpha_things( struct jz4780lcd_info * info, struct jz4780_lcd_dma_desc *new_desc, int is_fg1)
{
	if(is_fg1){
		/* change alpha value*/
		//if( info->osd.alpha1 != jz4780_lcd_info->osd.alpha1)
		new_desc->desc_size &= ~(LCD_DESSIZE_ALPHA_MASK);
		new_desc->desc_size |= (info->osd.alpha1 << LCD_DESSIZE_ALPHA_BIT) ;

		/* change alpha mode */
		if(info->osd.osd_cfg & LCD_OSDC_ALPHAMD1)
			new_desc->cmd_num |= LCD_CPOS_ALPHAMD;
		else
			new_desc->cmd_num &= ~LCD_CPOS_ALPHAMD;	
		
		/* change premult*/
		if(info->osd.osd_cfg & LCD_OSDC_PREMULT1)
			new_desc->cmd_num |= LCD_CPOS_PREMULT;
		else
			new_desc->cmd_num &= ~LCD_CPOS_PREMULT;	

		/* change coefficient*/
		new_desc->cmd_num &= ~(LCD_OSDC_COEF_SLE1_MASK);
		new_desc->cmd_num |= ((info->osd.osd_cfg & LCD_OSDC_COEF_SLE1_MASK) >> LCD_OSDC_COEF_SLE1_BIT) << LCD_CPOS_COEF_SLE_BIT;
		
	}else{
		/* change alpha value*/
		new_desc->desc_size &= ~(LCD_DESSIZE_ALPHA_MASK);
		new_desc->desc_size |= (info->osd.alpha0 << LCD_DESSIZE_ALPHA_BIT) ;

		/* change alpha mode */
		if(info->osd.osd_cfg & LCD_OSDC_ALPHAMD0)
			new_desc->cmd_num |= LCD_CPOS_ALPHAMD;
		else
			new_desc->cmd_num &= ~LCD_CPOS_ALPHAMD;	
		
		/* change premult*/
		if(info->osd.osd_cfg & LCD_OSDC_PREMULT0)
			new_desc->cmd_num |= LCD_CPOS_PREMULT;
		else
			new_desc->cmd_num &= ~LCD_CPOS_PREMULT;	
		/* change coefficient*/
		//if((info->osd.osd_cfg & LCD_OSDC_COEF_SLE0_MASK) != (jz4780_lcd_info->osd.osd_cfg & LCD_OSDC_COEF_SLE0_MASK))
		new_desc->cmd_num &= ~(LCD_OSDC_COEF_SLE0_MASK);
		new_desc->cmd_num |= ((info->osd.osd_cfg & LCD_OSDC_COEF_SLE0_MASK) >> LCD_OSDC_COEF_SLE0_BIT) << LCD_CPOS_COEF_SLE_BIT;
	}
}

static void adj_xy_wh(struct jz4780lcd_fg_t *fg, struct lcd_cfb_info *cfb)
{

	if ( fg->x >= cfb->jz4780_lcd_info->panel.w )
		fg->x = cfb->jz4780_lcd_info->panel.w;
	if ( fg->y >= cfb->jz4780_lcd_info->panel.h )
		fg->y = cfb->jz4780_lcd_info->panel.h;
	if ( (fg->x + fg->w) > cfb->jz4780_lcd_info->panel.w )
		fg->w = cfb->jz4780_lcd_info->panel.w - fg->x;
	if ( (fg->y + fg->h) > cfb->jz4780_lcd_info->panel.h )
		fg->h = cfb->jz4780_lcd_info->panel.h - fg->y;
}

static void calc_line_frm_size(int bpp, struct jz4780lcd_fg_t *fg, unsigned int *line_size, unsigned int *frm_size)
{
	*line_size = (fg->w*bpp+7)/8;
	*line_size = ((*line_size+3)>>2)<<2; /* word aligned */
	*frm_size = *line_size * fg->h; /* bytes */
	
}

void jz4780fb_reset_descriptor( struct lcd_cfb_info *cfb)
{
	int fg_line_size, fg_frm_size;
	int bpp;
	unsigned long irq_flags;
	struct jz4780lcd_info * info = cfb->jz4780_lcd_info;
	struct jz4780_lcd_dma_desc *new_desc;
	struct jz4780_lcd_dma_desc *cur_desc;
	struct jz4780lcd_fg_t *fg_info;
	unsigned char *buf_comp0; //buffer for compress output
	unsigned char *buf_comp1;

	if(!cfb->is_fg1){
		fg_info = &info->osd.fg0;

		if(REG_LCD_FID0(cfb->id) == cfb->dma0_desc0->frame_id){
			cur_desc = cfb->dma0_desc0;
			new_desc = cfb->dma0_desc1;
			buf_comp0 = cfb->buf_comp01;
		}else{
			cur_desc = cfb->dma0_desc1;
			new_desc = cfb->dma0_desc0;
			buf_comp0 = cfb->buf_comp00;
		}
	}else{
		fg_info = &info->osd.fg1;
		
		if(REG_LCD_FID1(cfb->id) == cfb->dma1_desc0->frame_id){
			cur_desc = cfb->dma1_desc0;
			new_desc = cfb->dma1_desc1;
			buf_comp1 = cfb->buf_comp11;
		}else{
			cur_desc = cfb->dma1_desc1;
			new_desc = cfb->dma1_desc0;
			buf_comp1 = cfb->buf_comp10;
		}
	}
	
	new_desc->next_desc = (unsigned int)virt_to_phys(new_desc);
	
	adj_xy_wh(fg_info, cfb);

	/* change xy position */
	new_desc->cmd_num = (fg_info->x << LCD_CPOS_XPOS_BIT | fg_info->y << LCD_CPOS_YPOS_BIT) ;

	/* change bpp */
	set_bpp(&fg_info->bpp, new_desc);

	bpp = bpp_to_data_bpp(fg_info->bpp);

#ifdef CONFIG_JZ_AOSDC
	if ((!cfb->is_fg1 && fg0_should_use_compress_decompress_mode(cfb))
		 || (cfb->is_fg1 && fg1_should_use_compress_decompress_mode(cfb))){

		unsigned int wpl;
		printk("use compress la\n");

		cfb->aosd_info->height = fg_info->h;	 
		cfb->aosd_info->is64unit = 0; 

		wpl = (fg_info->w * bpp) >> 5;

		if(fg_info->bpp == 24){
			if(cfb->aosd_info->is24c){
				cfb->aosd_info->bpp = fg_info->bpp;
				cfb->aosd_info->width = fg_info->w;
				cfb->aosd_info->src_stride = (cfb->aosd_info->width * bpp) >> 5;

			}else{
				cfb->aosd_info->bpp = 16;
				cfb->aosd_info->width = fg_info->w * 2;
				cfb->aosd_info->src_stride = (cfb->aosd_info->width * bpp) >> 5;
			}	

		}else{
			cfb->aosd_info->bpp = fg_info->bpp;
			//24 bpp use word, if width=320(pixel),stride=320.
			cfb->aosd_info->width = fg_info->w;
			cfb->aosd_info->src_stride = ((cfb->aosd_info->width * 32) >> 5);
		}
		// in words.if out Data is not a sequential access, wpl=(panel.w*bpp)>>5
		cfb->aosd_info->dst_stride = (wpl + (wpl + 63)/64);


		if (cfb->is_fg1) {		
			cfb->aosd_info->raddr = (unsigned int)virt_to_phys((void *)cfb->lcd_frame1);
			cfb->aosd_info->waddr = (unsigned int)virt_to_phys((void *)buf_comp1);
		}
		else {
			cfb->aosd_info->raddr = (unsigned int)virt_to_phys((void *)cfb->lcd_frame0);
			cfb->aosd_info->waddr = (unsigned int)virt_to_phys((void *)buf_comp0);
		}

		jz_compress_set_mode(cfb->aosd_info);
		jz_start_compress();
		wait_event_interruptible(
			cfb->compress_wq, cfb->aosd_info->compress_done == 1);

		if(!cfb->aosd_info->is24c)	
		{
                        //the last line of dst buffer
			unsigned int *dst = (unsigned int *)((unsigned int *)phys_to_virt((unsigned long)cfb->aosd_info->waddr) + (cfb->aosd_info->dst_stride * (cfb->aosd_info->height - 1))); 
			unsigned int *src = (unsigned int *)((unsigned int *)phys_to_virt((unsigned long)cfb->aosd_info->raddr) + (wpl * (cfb->aosd_info->height - 1)));
			
			unsigned int cmd_num = wpl / 64; 
			unsigned int mod64 = wpl % 64;
			unsigned int cmd;
			
			volatile int i=0, j=0;
			int cpsize=64*4;
			
			if(wpl < 64){
				cmd = ((wpl + 1) << 16 | mod64) | (1 << 29);
				cpsize = wpl *4 ;
				
				*(dst+i++) = cmd;
				memcpy((dst + i), src, cpsize);
				
			}else if((cmd_num == 1) && (mod64 != 0)){
				cmd = (wpl + cmd_num) << 16 | (0x40 + mod64) | (1 << 29);
				cpsize = wpl *4 ;
				
				*(dst+i++) = cmd;
				memcpy((dst + i), src, cpsize);
				
			}else{
				/*the first cmd of the line*/
				cmd = (wpl + cmd_num) << 16 | 0x40 | (1 << 29);
				cpsize = 0x40 * 4;
				*(dst + i++) = cmd;
				memcpy((dst + i), src+(j * 64), cpsize);
				i += (cpsize >> 2);
				j++;

				cmd = 0x20000040;
				while(j < (cmd_num - 1))
				{
					*(dst+i++) = cmd;
					memcpy((dst + i), src+(j * 64), cpsize);
					i += (cpsize >> 2);
					j++;
				}
				
				cmd = (cmd & ~(0xff)) | (0x40 + mod64);
				cpsize += (mod64 * 4);
				*(dst+i++) = cmd;
				memcpy((dst + i), src+(j * 64), cpsize);
			}
			dma_cache_wback((unsigned int)(dst),(wpl+cmd_num)<<2);
		}
	}
#endif
	
	/* change  display size */
	calc_line_frm_size(bpp, fg_info, &fg_line_size, &fg_frm_size);
	
	new_desc->desc_size = ((fg_info->w-1) << LCD_DESSIZE_WIDTH_BIT | (fg_info->h-1) << LCD_DESSIZE_HEIGHT_BIT);

	if(!cfb->is_fg1){
		if(fg0_should_use_x2d_block_mode(cfb)){
			printk("%s,use x2d\n",__func__);
			/////////////////////////
			new_desc->cmd = LCD_CMD_16X16_BLOCK | fg_info->h;
			new_desc->cmd |= LCD_CMD_SOFINT | LCD_CMD_EOFINT;
			new_desc->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame0);
			new_desc->offsize = (fg_info->w/16)*(((16*bpp)/32)*16); /* in words */
			new_desc->page_width = fg_line_size>>2; /* in words */
		}
		else if(fg0_should_use_compress_decompress_mode(cfb)){
#ifdef CONFIG_JZ_AOSDC
			printk("%s,use compress\n",__func__);
			if(!cfb->aosd_info->is24c){
				printk("!24c\n");
				new_desc->cmd = LCD_CMD_COMPRE_EN | fg_info->h;
				new_desc->offsize = (cfb->aosd_info->dst_stride); /* in words */
			}else{
				printk("24c\n");
				new_desc->cmd =  compress_get_fwdcnt();
				new_desc->offsize = 0; /* in words */
				new_desc->cmd_num = (new_desc->cmd_num & ~LCD_CPOS_BPP_MASK); 
				new_desc->cmd_num |= LCD_CPOS_BPP_CMPS_24;
			}
			new_desc->cmd |= LCD_CMD_SOFINT | LCD_CMD_EOFINT;

			new_desc->databuf = (unsigned int)virt_to_phys((void *)buf_comp0);
#endif
		}
		else{
			printk("%s , %d,use normal lcd\n",__func__,__LINE__);
			new_desc->cmd = fg_frm_size>>2 | LCD_CMD_SOFINT | LCD_CMD_EOFINT;
			new_desc->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame0);
			new_desc->offsize = 0;
#ifdef CONFIG_FB_JZ4780_SLCD
			printk("use slcd\n");
			new_desc->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc_cmd);
			cfb->dma0_desc_cmd->next_desc = (unsigned int)virt_to_phys(new_desc);
			cfb->dma0_desc_cmd0->next_desc = (unsigned int)virt_to_phys(new_desc);
#endif
		}
	}else{
		if(fg1_should_use_x2d_block_mode(cfb)){
			printk("%s,use x2d\n",__func__);
			/////////////////////////
			new_desc->cmd = LCD_CMD_16X16_BLOCK | fg_info->h;
			new_desc->cmd |= LCD_CMD_SOFINT | LCD_CMD_EOFINT;
			new_desc->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame1);
			new_desc->offsize = (fg_info->w/16)*(((16*bpp)/32)*16); /* in words */
			new_desc->page_width = fg_line_size>>2; /* in words */
		}
		else if(fg1_should_use_compress_decompress_mode(cfb)){
#ifdef CONFIG_JZ_AOSDC
			printk("%s,use compress\n",__func__);
			if(!cfb->aosd_info->is24c){
				printk("!24c\n");
				new_desc->cmd = LCD_CMD_COMPRE_EN | fg_info->h;
				new_desc->offsize = (cfb->aosd_info->dst_stride); /* in words */
			}else{
				printk("24c\n");
				new_desc->cmd = compress_get_fwdcnt();
				new_desc->offsize = 0; /* in words */
				new_desc->cmd_num = (new_desc->cmd_num & ~LCD_CPOS_BPP_MASK); 
				new_desc->cmd_num |= LCD_CPOS_BPP_CMPS_24;
			}

			new_desc->cmd |= LCD_CMD_SOFINT | LCD_CMD_EOFINT;

			new_desc->databuf = (unsigned int)virt_to_phys((void *)buf_comp1);
#endif
		}
		else{
			printk("%s , %d,use normal lcd\n",__func__,__LINE__);
			new_desc->cmd = fg_frm_size>>2 | LCD_CMD_SOFINT | LCD_CMD_EOFINT;
			new_desc->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame1);
			new_desc->offsize = 0;
		}
		
	}
	/* change enable */		
	if(!cfb->is_fg1){
		if( info->osd.osd_cfg & LCD_OSDC_F0EN )
			new_desc->cmd |= LCD_CMD_FRM_EN;
		else
			new_desc->cmd &= ~LCD_CMD_FRM_EN;
	}else{
		if( info->osd.osd_cfg & LCD_OSDC_F1EN )
			new_desc->cmd |= LCD_CMD_FRM_EN;
		else
			new_desc->cmd &= ~LCD_CMD_FRM_EN;
	}	

        /* change alpha things*/
	if(info->osd.osd_cfg & LCD_OSDC_ALPHAEN){
		set_alpha_things(info, new_desc, cfb->is_fg1);
	}

	dma_cache_wback((unsigned int)(new_desc), sizeof(struct jz4780_lcd_dma_desc));

	if(!cfb->is_fg1){
		if (fg_info->bpp <= 8){ 
			printk("JZ4780 do not support palette!!!!!\n");
/*
			cur_desc->next_desc = (unsigned int)virt_to_phys(dma0_desc_palette);
			//REG_LCD_DA0 = (unsigned int)virt_to_phys(dma0_desc_palette);
			dma0_desc_palette->next_desc = (unsigned int)virt_to_phys(new_desc);
*/
		}
	}
	cur_desc->next_desc = (unsigned int)virt_to_phys(new_desc);
	if(!cfb->is_fg1 && (REG_LCD_FID0(cfb->id) == 0)){
#if defined(CONFIG_FB_JZ4780_SLCD)  /* for smartlcd */
		REG_LCD_DA0(cfb->id) = virt_to_phys(cfb->dma0_desc_cmd0); //smart lcd
		printk("slcd, da0:%x\n",REG_LCD_DA0(cfb->id));
		printk("slcd , dma0_desc_cmd0->next_desc:%x\n",cfb->dma0_desc_cmd0->next_desc);
		printk("slcd , dma0_desc_cmd->next_desc:%x\n",cfb->dma0_desc_cmd->next_desc);
		printk("slcd , new_desc->next_desc:%x\n",new_desc->next_desc);
#else
		REG_LCD_DA0(cfb->id) = (unsigned int)virt_to_phys(new_desc);
#endif
	}else if(cfb->is_fg1 && (REG_LCD_FID1(cfb->id) == 0))
		REG_LCD_DA1(cfb->id) = (unsigned int)virt_to_phys(new_desc);

	dma_cache_wback((unsigned int)(cfb->dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz4780_lcd_dma_desc));
	dump_dma_desc(new_desc);

	spin_lock_irqsave(&cfb->update_lock, irq_flags);
	REG_LCD_STATE(cfb->id) &= ~LCD_STATE_EOF;
	__lcd_enable_eof_intr(cfb->id);

	cfb->frame_requested++;
	spin_unlock_irqrestore(&cfb->update_lock, irq_flags);

	printk("lcd%d ctrl:%x\n",cfb->id, REG_LCD_CTRL(cfb->id));
	if(cfb->frame_requested != cfb->frame_done)
		wait_event_interruptible_timeout(cfb->frame_wq, cfb->frame_done == cfb->frame_requested, HZ);
	
	if(cfb->frame_requested != cfb->frame_done)
		printk("wait eof timeout\n");

	__lcd_disable_eof_intr(cfb->id);

	return 0;
}

#if 0
static void jz4780fb_reset_tv_descriptor( struct jz4780lcd_info * info , int is_fg1)
{
	int fg_line_size, fg_frm_size;
	struct jz4780_lcd_dma_desc *new_desc0, *new_desc1;
	struct jz4780_lcd_dma_desc *cur_desc0, *cur_desc1;
	struct jz4780lcd_fg_t *fg_info;
	int bpp;

	if(!is_fg1){
		fg_info = &info->osd.fg0;
		
		if(REG_LCD_FID0 == dma0_desc0->frame_id){
			cur_desc0 = dma0_desc0;
			cur_desc1 = dma0_desc1;
			new_desc0 = dma0_tv_desc0;
			new_desc1 = dma0_tv_desc1;
		}else{
			cur_desc0 = dma0_tv_desc0;
			cur_desc1 = dma0_tv_desc1;
			new_desc0 = dma0_desc0;
			new_desc1 = dma0_desc1;
		}
	}else{
		fg_info = &info->osd.fg1;
		
		if(REG_LCD_FID1 == dma1_desc0->frame_id){
			cur_desc0 = dma1_desc0;
			cur_desc1 = dma1_desc1;
			new_desc0 = dma1_tv_desc0;
			new_desc1 = dma1_tv_desc1;
		}else{
			cur_desc0 = dma1_tv_desc0;
			cur_desc1 = dma1_tv_desc1;
			new_desc0 = dma1_desc0;
			new_desc1 = dma1_desc1;
		}
		
	}

	new_desc0->next_desc = (unsigned int)virt_to_phys(new_desc1);
	new_desc1->next_desc = (unsigned int)virt_to_phys(new_desc0);
	
	adj_xy_wh(fg_info, cfb);
	
	/* change xy position */
	new_desc0->cmd_num = new_desc1->cmd_num = (new_desc0->cmd_num & ~LCD_CPOS_POS_MASK) 
		| (fg_info->x << LCD_CPOS_XPOS_BIT | fg_info->y << LCD_CPOS_YPOS_BIT) ;
	
	/* change bpp */
	set_bpp(&fg_info->bpp, new_desc0);
	set_bpp(&fg_info->bpp, new_desc1);

	bpp = bpp_to_data_bpp(fg_info->bpp);
	/* change  display size */
	calc_line_frm_size(bpp, fg_info, &fg_line_size, &fg_frm_size);
	new_desc0->desc_size = new_desc1->desc_size &= ~(LCD_DESSIZE_SIZE_MASK);
	new_desc0->desc_size = new_desc1->desc_size |= (fg_info->w << LCD_DESSIZE_WIDTH_BIT | fg_info->h << LCD_DESSIZE_HEIGHT_BIT);

	if(!is_fg1){
		if(fg0_should_use_compress_decompress_mode()){
#ifdef CONFIG_JZ_AOSDC			
			new_desc0->cmd =  LCD_CMD_COMPRE_EN | (fg_info->h / 2 + fg_info->h % 2);
			new_desc1->cmd =  LCD_CMD_COMPRE_EN | (fg_info->h / 2);

			new_desc0->databuf = (unsigned int)virt_to_phys((void *)buf_comp1);
			new_desc1->databuf = (unsigned int)virt_to_phys((void *)buf_comp1 + aosd_info->dst_stride);

			new_desc0->offsize = new_desc1->offsize = (aosd_info->dst_stride)>>2; /* in words */
#endif
		}else{
			new_desc0->cmd = (fg_line_size * (fg_info->h / 2 + fg_info->h % 2))/4;
			new_desc1->cmd = (fg_line_size * (fg_info->h / 2))/4;

			new_desc0->databuf = virt_to_phys((void *)(lcd_frame0));
			new_desc1->databuf = virt_to_phys((void *)(lcd_frame0 + fg_line_size));

			new_desc0->offsize = new_desc1->offsize	= fg_line_size/4;
			new_desc0->page_width = new_desc1->page_width = fg_line_size/4;
		}
	}else{
		if(fg1_should_use_compress_decompress_mode()){
#ifdef CONFIG_JZ_AOSDC
			new_desc0->cmd =  LCD_CMD_COMPRE_EN | (fg_info->h / 2 + fg_info->h % 2);
			new_desc1->cmd =  LCD_CMD_COMPRE_EN | (fg_info->h / 2);

			new_desc0->databuf = (unsigned int)virt_to_phys((void *)buf_comp);
			new_desc1->databuf = (unsigned int)virt_to_phys((void *)buf_comp + aosd_info->dst_stride);

			new_desc0->offsize = new_desc1->offsize = (aosd_info->dst_stride)>>2; /* in words */
#endif
		}else{
			new_desc0->cmd = (fg_line_size * (fg_info->h / 2 + fg_info->h % 2))/4;
			new_desc1->cmd = (fg_line_size * (fg_info->h / 2))/4;

			new_desc0->databuf = virt_to_phys((void *)(lcd_frame1));
			new_desc1->databuf = virt_to_phys((void *)(lcd_frame1 + fg_line_size));
			
			new_desc0->offsize = new_desc1->offsize	= fg_line_size/4;
			new_desc0->page_width = new_desc1->page_width = fg_line_size/4;
			
		}
	}
		
	/* change enable */		
	if(!is_fg1){
		if( info->osd.osd_cfg | LCD_OSDC_F0EN )
			new_desc0->cmd = new_desc1->cmd |= LCD_CMD_FRM_EN;
		else
			new_desc0->cmd = new_desc1->cmd &= ~LCD_CMD_FRM_EN;
	}else{
		if( info->osd.osd_cfg | LCD_OSDC_F1EN )
			new_desc0->cmd = new_desc1->cmd |= LCD_CMD_FRM_EN;
		else
			new_desc0->cmd = new_desc1->cmd &= ~LCD_CMD_FRM_EN;
	}	
	
	dma_cache_wback((unsigned int)(new_desc0), sizeof(struct jz4780_lcd_dma_desc));
	dma_cache_wback((unsigned int)(new_desc1), sizeof(struct jz4780_lcd_dma_desc));

	if(REG_LCD_FID0 != 0 && REG_LCD_FID1 != 0){
		if (!is_fg1 && fg_info->bpp <= 8){ 
			REG_LCD_DA0 = ( unsigned int )virt_to_phys(dma0_desc_palette);
			dma0_desc_palette->next_desc = (unsigned int)virt_to_phys(new_desc0);
		}else
			cur_desc0->next_desc = (unsigned int)virt_to_phys(new_desc0);
	}else{
		if(!is_fg1)
			REG_LCD_DA0 = ( unsigned int )virt_to_phys(new_desc0);
		else
			REG_LCD_DA1 = ( unsigned int )virt_to_phys(new_desc0);
	}
	dma_cache_wback((unsigned int)(dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz4780_lcd_dma_desc));
}
#endif

static void jz4780fb_change_clock(struct lcd_cfb_info *cfb)
{
	struct jz4780lcd_info * lcd_info = cfb->jz4780_lcd_info;

	printk("%s, %d\n",__func__,__LINE__);
#if defined(CONFIG_FPGA)
	if(cfb->id == 0)  //hdmi
		REG_LCD_REV(cfb->id) = 0x1;
	else
		REG_LCD_REV(cfb->id) = 0x1;

	printk("FPGA test, pixclk divide REG_LCD_REV=0x%08x\n", REG_LCD_REV(cfb->id));
	printk("FPGA test, pixclk %d\n", JZ_EXTAL/(((REG_LCD_REV(cfb->id)&0xFF)+1)*2));
#else
	unsigned int val = 0;
	unsigned int pclk;

	/* Timing setting */
	val = lcd_info->panel.fclk; /* frame clk */

	if ( (lcd_info->panel.cfg & LCD_CFG_MODE_MASK) != LCD_CFG_MODE_SERIAL_TFT) {
		pclk = val * (lcd_info->panel.w + lcd_info->panel.hsw + lcd_info->panel.elw + lcd_info->panel.blw) *
			     (lcd_info->panel.h + lcd_info->panel.vsw + lcd_info->panel.efw + lcd_info->panel.bfw); 
	}
	else {
		/* serial mode: Hsync period = 3*Width_Pixel */
		pclk = val * (lcd_info->panel.w*3 + lcd_info->panel.hsw + lcd_info->panel.elw + lcd_info->panel.blw) * 
			     (lcd_info->panel.h + lcd_info->panel.vsw + lcd_info->panel.efw + lcd_info->panel.bfw); 
	}

	if(cfb->id == 1){
		cpm_set_clock(CGU_LPCLK1, pclk);
		__cpm_start_lcd1();

		jz_clocks.pixclk1 = cpm_get_clock(CGU_LPCLK1);
		printk("LCDC1: PixClock:%d\n", jz_clocks.pixclk1);
	}else{
		cpm_set_clock(CGU_HDMICLK, 27000000);
		cpm_set_clock(CGU_LPCLK0, pclk);
		
		__cpm_start_lcd0();
		__cpm_start_hdmi();
		
		jz_clocks.pixclk0 = cpm_get_clock(CGU_LPCLK0);
		printk("LCDC0: PixClock:%d\n", jz_clocks.pixclk0);
		printk("HDMI Clock:%d\n", cpm_get_clock(CGU_HDMICLK));
	}

#endif

}

/*
 * jz4780fb_deep_set_mode,
 *
 */
static void jz4780fb_deep_set_mode(struct lcd_cfb_info *cfb)
{
	struct jz4780lcd_info * lcd_info  = cfb->jz4780_lcd_info;

	jz4780fb_descriptor_init(cfb);
	cfb->is_fg1 = 0;
	jz4780fb_reset_descriptor(cfb);
	cfb->is_fg1 = 1;
	jz4780fb_reset_descriptor(cfb);
	jz4780fb_set_panel_mode(cfb);
	jz4780fb_set_osd_mode(cfb);
	jz4780fb_set_var(&cfb->fb.var, -1, &cfb->fb);
}


static irqreturn_t jz4780fb_interrupt_handler(int irq, void *dev_id)
{
	unsigned int state;
	static int irqcnt=0;
	int id;
	struct lcd_cfb_info *cfb;

	if ( irq == IRQ_LCD1){
		id = 1;
		cfb = jz4780fb1_info;
	}else{
		id = 0;
		cfb = jz4780fb0_info;
	}
	printk("id:%d\n",id);
//	cfb = container_of(id, struct lcd_cfb_info, id);

	state = __lcd_get_state(id);
	D("In the lcd interrupt handler, state=0x%x\n", state);

	if (state & LCD_STATE_IFU0) {
		printk("%s, InFiFo0 underrun\n", __FUNCTION__);
		__lcd_clr_infifo0underrun(id);
	}

	if (state & LCD_STATE_IFU1) {
		printk("%s, InFiFo1 underrun\n", __FUNCTION__);
		__lcd_clr_infifo1underrun(id);
	}

	if (state & LCD_STATE_OFU) { /* Out fifo underrun */
		__lcd_clr_outfifounderrun(id);

		if ( irqcnt++ > 5 ) {
			__lcd_disable_ofu_intr(id);
			printk("disable Out FiFo underrun irq.\n");
		}
		printk("%s, Out FiFo underrun.\n", __FUNCTION__);
	}

	if(state & LCD_STATE_EOF)
	{
		printk("get eof intr\n");
		__lcd_clr_eof(id);
	}
	
	cfb->frame_done = cfb->frame_requested;

	wake_up(&cfb->frame_wq);
	return IRQ_HANDLED;
}

#ifdef CONFIG_PM

/*
 * Suspend the LCDC.
 */
static int jz4780_fb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct lcd_cfb_info *cfb;


	printk("%s(): called.\n", __func__);

	screen_off(pdev->id);
	__lcd_clr_ena(pdev->id);

	if(pdev->id == 0)
		__cpm_stop_lcd0();
	else
		__cpm_stop_lcd1();

#if defined(CONFIG_JZ4770_PISCES)
	act8600_output_enable(ACT8600_OUT6,ACT8600_OUT_OFF);
#endif
	
	return 0;
}

/*
 * Resume the LCDC.
 */
static int jz4780_fb_resume(struct platform_device *pdev)
{
	struct lcd_cfb_info *cfb;

	printk("%s(): called.\n", __func__);
#if defined(CONFIG_JZ4770_PISCES)
	act8600_output_enable(ACT8600_OUT6,ACT8600_OUT_ON);
#endif

	if(pdev->id == 0)
		__cpm_start_lcd0();
	else
		__cpm_start_lcd1();

	screen_on(pdev->id);
	__lcd_set_ena(pdev->id);
	
	return 0;
}

#else
#define jzfb_suspend      NULL
#define jzfb_resume       NULL
#endif /* CONFIG_PM */

/* The following routine is only for test */

#if JZ_FB_DEBUG
static void test_gpio(int gpio_num, int delay)	{
//	__gpio_as_output(gpio_num);
	while(1) {
		__gpio_set_pin(gpio_num);
		udelay(delay);
		__gpio_clear_pin(gpio_num);
		udelay(delay);
	}
}

#if 0
static void display_v_color_bar(int w, int h, int tbpp, int *ptr) 
	
{
	int i,j;
	int bpp;
	unsigned short *p16;
	unsigned int *p32;
	
	p16 = (unsigned short *)ptr;
	p32 = (unsigned int *)ptr;
	bpp = bpp_to_data_bpp(tbpp);
	
	for (i = 0; i < h; i++) {
		for (j = 0; j < w; j++) {
			short c16;
			int c32;
			switch ((j / 10) % 4) {
			case 0:
				c16 = 0xF800;
				c32 = 0x00FF0000;
				break;
			case 1:
				c16 = 0x07C0;
				c32 = 0x0000FF00;
				break;
			case 2:
				c16 = 0x001F;
				c32 = 0x000000FF;
				break;
			default:
				c16 = 0xFFFF;
				c32 = 0xFFFFFFFF;
				break;
			}
			switch (bpp) {
			case 18:
			case 24:
			case 32:
				*p32++ = c32;
				break;
			default:
				*p16 ++ = c16;
			}
		}
/*
		if (w % PIXEL_ALIGN) {
			switch (bpp) {
			case 18:
			case 24:
			case 32:
				p32 += (ALIGN(mode->xres, PIXEL_ALIGN) - w);
				break;
			default:
				p16 += (ALIGN(mode->yres, PIXEL_ALIGN) - w);
				break;
			}
		}
*/
	}
}
#endif
#if 1
static void display_v_color_bar(int w, int h, int tbpp, int *ptr) {
	int i, j, bpp, wpl, data = 0;


	bpp = bpp_to_data_bpp(tbpp);
		
	wpl = w*bpp/32;

	
	printk("%s: w: %d, h: %d, bpp: %d, wpl: %d\n",__func__,  w, h, bpp, wpl);
	
	if (!(tbpp > 8))
		switch(tbpp){
		case 1:
			for (j = 0;j < h; j++)
				for (i = 0;i < wpl; i++) {
					*ptr++ = 0x00ff00ff;
				}
			break;
		case 2:
			for (j = 0;j < h; j++)
				for (i = 0;i < wpl; i++) {
					data = (i%4)*0x55555555;
					*ptr++ = data;
				}
			break;
		case 4:
			for (j = 0;j < h; j++)
				for (i = 0;i < wpl; i++) {
					data = (i%16)*0x11111111;
					*ptr++ = data;
				}
			break;
		case 8:
			for (j = 0;j < h; j++)
				for (i = 0;i < wpl; i+=2) {
					data = (i%(256))*0x01010101;
					*ptr++ = data;
					*ptr++ = data;
				}
			break;
		}
	else {
		switch(tbpp) {
		case 15:
		printk("%s, bpp 15\n",__func__);
			for (j = 0;j < h; j++)
				for (i = 0;i < wpl; i++) {
					if((i/4)%8==0)
						*ptr++ = 0xffffffff;
					else if ((i/4)%8==1)
						*ptr++ = 0x7c007c00;
					else if ((i/4)%8==2)
						*ptr++ = 0x7fe07fe0;
					else if ((i/4)%8==3)
						*ptr++ = 0x03e003e0;
					else if ((i/4)%8==4)
						*ptr++ = 0x03ff03ff;
					else if ((i/4)%8==5)
						*ptr++ = 0x001f001f;
					else if ((i/4)%8==6)
						*ptr++ = 0x7c1f7c1f;
					else if ((i/4)%8==7)
						*ptr++ = 0x00000000;
				}
			break;
		case 16:
			for (j = 0;j < h; j++)
				for (i = 0;i < wpl; i++) {
					if((i/4)%8==0)
						*ptr++ = 0xffffffff;
					else if ((i/4)%8==1)
						*ptr++ = 0xf800f800;
					else if ((i/4)%8==2)
						*ptr++ = 0xffe0ffe0;
					else if ((i/4)%8==3)
						*ptr++ = 0x07e007e0;
					else if ((i/4)%8==4)
						*ptr++ = 0x07ff07ff;
					else if ((i/4)%8==5)
						*ptr++ = 0x001f001f;
					else if ((i/4)%8==6)
						*ptr++ = 0xf81ff81f;
					else if ((i/4)%8==7)
						*ptr++ = 0x00000000;
				}
			break;
		case 24:
		case 32:
		printk("%s, bpp24\n",__func__);
			for (j = 0;j < h; j++)
				for (i = 0;i < wpl; i++) {
					if((i/8)%8==7)
						*ptr++ = 0xffffff;
					else if ((i/8)%8==1)
						*ptr++ = 0xff0000;
					else if ((i/8)%8==2)
						*ptr++ = 0xffff00;
					else if ((i/8)%8==3)
						*ptr++ = 0x00ff00;
					else if ((i/8)%8==4)
						*ptr++ = 0x00ffff;
					else if ((i/8)%8==5)
						*ptr++ = 0x0000ff;
					else if ((i/8)%8==6)
						*ptr++ = 0xff00ff;
					else if ((i/8)%8==0)
						*ptr++ = 0x000000;
				}
			break;
		case 30:
			printk("%s, bpp 30\n",__func__);
			for(j = 0; j < h; j++)
				for(i = 0; i < wpl; i++)
				{
					if((i/8)%8==7)
						*ptr++ = 0x3fffffff;
					else if ((i/8)%8==1)
						*ptr++ = 0x3ff00000;
					else if ((i/8)%8==2)
						*ptr++ = 0x3ffffc00;
					else if ((i/8)%8==3)
						*ptr++ = 0x000ffc00;
					else if ((i/8)%8==4)
						*ptr++ = 0x000fffff;
					else if ((i/8)%8==5)
						*ptr++ = 0x000003ff;
					else if ((i/8)%8==6)
						*ptr++ = 0x3ff003ff;
					else if ((i/8)%8==0)
						*ptr++ = 0x00000000;
				}
			break;
		}
	}

}
#endif

static void display_h_color_bar(int w, int h, int tbpp, int *ptr) {
	int i, data = 0, bpp;
	int wpl; //word_per_line
	
	bpp = bpp_to_data_bpp(tbpp);

	wpl = w*bpp/32;
	if (!(tbpp > 8))
		for (i = 0;i < wpl*h;i++) {
			switch(tbpp){
			case 1:
				if(i%(wpl*8)==0)
					data = ((i/(wpl*8))%2)*0xffffffff;
					*ptr++ = data;
				break;
			case 2:
				if(i%(wpl*8)==0)
					data = ((i/(wpl*8))%4)*0x55555555;
					*ptr++ = data;
				break;
			case 4:
				if(i%(wpl*8)==0)
					data = ((i/(wpl*8))%16)*0x11111111;
				*ptr++ = data;
				break;
			case 8:
				if(i%(wpl*8)==0)
					data = ((i/(wpl*8))%256)*0x01010101;
				*ptr++ = data;
				break;
			}
		}
	else {

		switch(tbpp) {
		case 15:
			for (i = 0;i < wpl*h;i++) {
				if (((i/(wpl*8)) % 8) == 0)
					*ptr++ = 0xffffffff;
				else if (((i/(wpl*8)) % 8) == 1)
					*ptr++ = 0x7c007c00;
				else if (((i/(wpl*8)) % 8) == 2)
					*ptr++ = 0x7fe07fe0;
				else if (((i/(wpl*8)) % 8) == 3)
					*ptr++ = 0x03e003e0;
				else if (((i/(wpl*8)) % 8) == 4)
					*ptr++ = 0x03ff03ff;
				else if (((i/(wpl*8)) % 8) == 5)
					*ptr++ = 0x001f001f;
				else if (((i/(wpl*8)) % 8) == 6)
					*ptr++ = 0x7c1f7c1f;
				else if (((i/(wpl*8)) % 8) == 7)
					*ptr++ = 0x00000000;
			}
				break;

		case 16:
			for (i = 0;i < wpl*h;i++) {
				if (((i/(wpl*8)) % 8) == 0)
					*ptr++ = 0xffffffff;
				else if (((i/(wpl*8)) % 8) == 1)
					*ptr++ = 0xf800f800;
				else if (((i/(wpl*8)) % 8) == 2)
					*ptr++ = 0xffe0ffe0;
				else if (((i/(wpl*8)) % 8) == 3)
					*ptr++ = 0x07e007e0;
				else if (((i/(wpl*8)) % 8) == 4)
					*ptr++ = 0x07ff07ff;
				else if (((i/(wpl*8)) % 8) == 5)
					*ptr++ = 0x001f001f;
				else if (((i/(wpl*8)) % 8) == 6)
					*ptr++ = 0xf81ff81f;
				else if (((i/(wpl*8)) % 8) == 7)
					*ptr++ = 0x00000000;
			}
				break;
		case 24:
		case 32:


			for (i = 0;i < wpl*h;i++) {
//				*ptr++ = 0xff0000;
//				*ptr++ = (1 << 10);
#if 0
				if(i == 0)
					*ptr++ = 0x000000ff;
				else
					*ptr++ = 0x00000000;
#endif
			}

/*
			for (i = 0;i < wpl;i++) {
				*ptr++ = 0x00ff0000;
			}
			for (i = wpl;i < wpl*2;i++) {
				*ptr++ = 0x0000ff00;
			}

			for (i = wpl*2;i < wpl*3;i++) {
				*ptr++ = 0x000000ff;
			}

			for (i = wpl*3;i < wpl*h;i++) {
				*ptr++ = 0x00000000;
			}
*/
#if 1

			for (i = 0;i < wpl*h;i++) {
				if (((i/(wpl*8)) % 8) == 7)
					*ptr++ = 0xffffff;
				else if (((i/(wpl*8)) % 8) == 2)
					*ptr++ = 0xff0000;
				else if (((i/(wpl*8)) % 8) == 4)
					*ptr++ = 0xffff00;
				else if (((i/(wpl*8)) % 8) == 6)
					*ptr++ = 0x00ff00;
				else if (((i/(wpl*8)) % 8) == 1)
					*ptr++ = 0x00ffff;
				else if (((i/(wpl*8)) % 8) == 3)
					*ptr++ = 0x0000ff;
				else if (((i/(wpl*8)) % 8) == 5)
					*ptr++ = 0x000000;
				else if (((i/(wpl*8)) % 8) == 0)
					*ptr++ = 0xff00ff;
			}
#endif
			break;
		case 30:
			for (i = 0;i < wpl*h;i++) {
				if (((i/(wpl*8)) % 8) == 7)
					*ptr++ = 0x3fffffff;
				else if (((i/(wpl*8)) % 8) == 2)
					*ptr++ = 0x3ff00000;
				else if (((i/(wpl*8)) % 8) == 4)
					*ptr++ = 0x3ffffc00;
				else if (((i/(wpl*8)) % 8) == 6)
					*ptr++ = 0x000ffc00;
				else if (((i/(wpl*8)) % 8) == 1)
					*ptr++ = 0x000fffff;
				else if (((i/(wpl*8)) % 8) == 3)
					*ptr++ = 0x000003ff;
				else if (((i/(wpl*8)) % 8) == 5)
					*ptr++ = 0x00000000;
				else if (((i/(wpl*8)) % 8) == 0)
					*ptr++ = 0x3ff003ff;
			}
			break;
		case 36:
			for (i = 0;i < wpl*2;i++) {
				*ptr++ = 0x00ff0000;
				*ptr++ = 0x0000ff00;
				*ptr++ = 0xff0000ff;
			}

			for (i = wpl*2;i < wpl*4;i++) {
				*ptr++ = 0x0000ff00;
				*ptr++ = 0xff0000ff;
				*ptr++ = 0x00ff0000;
			}

			for (i = wpl*4;i < wpl*6;i++) {
				*ptr++ = 0xff0000ff;
				*ptr++ = 0x00ff0000;
				*ptr++ = 0x0000ff00;
			}
			break;

		}
	}
}
#endif

/* Backlight Control Interface via sysfs
 *
 * LCDC:
 * Enabling LCDC when LCD backlight is off will only affects cfb->display.
 *
 * Backlight:
 * Changing the value of LCD backlight when LCDC is off will only affect the cfb->backlight_level.
 *
 * - River.
 */
static int screen_off(int id)
{
	struct lcd_cfb_info *cfb;

	if(id == 0)
		cfb = jz4780fb0_info;
	else
		cfb = jz4780fb1_info;

	__lcd_close_backlight();
	__lcd_display_off();

#ifdef HAVE_LCD_PWM_CONTROL
	if (cfb->b_lcd_pwm) {
		__lcd_pwm_stop();
		cfb->b_lcd_pwm = 0;
	}
#endif

	cfb->b_lcd_display = 0;

	return 0;
}

static int screen_on(int id)
{
	struct lcd_cfb_info *cfb;

	if(id == 0)
		cfb = jz4780fb0_info;
	else
		cfb = jz4780fb1_info;

	__lcd_display_on();

	/* Really restore LCD backlight when LCD backlight is turned on. */
	if (cfb->backlight_level) {
#ifdef HAVE_LCD_PWM_CONTROL
		if (!cfb->b_lcd_pwm) {
			__lcd_pwm_start();
			cfb->b_lcd_pwm = 1;
		}
#endif
		__lcd_set_backlight_level(cfb->backlight_level);
	}

	cfb->b_lcd_display = 1;

	return 0;
}

static int jz4780fb_set_backlight_level(int n)
{
#if defined(CONFIG_SOC_JZ4780)
	struct lcd_cfb_info *cfb = jz4780fb1_info;
#else
	struct lcd_cfb_info *cfb = jz4780fb0_info; //only support lcd0 for 4775
#endif

	if (n) {
		if (n > LCD_MAX_BACKLIGHT)
			n = LCD_MAX_BACKLIGHT;

		if (n < LCD_MIN_BACKLIGHT)
			n = LCD_MIN_BACKLIGHT;

		/* Really change the value of backlight when LCDC is enabled. */
		if (cfb->b_lcd_display) {
#ifdef HAVE_LCD_PWM_CONTROL
			if (!cfb->b_lcd_pwm) {
				__lcd_pwm_start();
				cfb->b_lcd_pwm = 1;
			}
#endif
			__lcd_set_backlight_level(n);
		}
	}else{
		/* Turn off LCD backlight. */
		__lcd_close_backlight();

#ifdef HAVE_LCD_PWM_CONTROL
		if (cfb->b_lcd_pwm) {
			__lcd_pwm_stop();
			cfb->b_lcd_pwm = 0;
		}
#endif
	}

	cfb->backlight_level = n;

	return 0;
}

static ssize_t show_bl_level(struct device *device,
			     struct device_attribute *attr, char *buf)
{
#if defined(CONFIG_SOC_JZ4780)
	struct lcd_cfb_info *cfb = jz4780fb1_info;
#else
	struct lcd_cfb_info *cfb = jz4780fb0_info;	//for 4775
#endif
	return snprintf(buf, PAGE_SIZE, "%d\n", cfb->backlight_level);
}

static ssize_t store_bl_level(struct device *device,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int n;
	char *ep;

	n = simple_strtoul(buf, &ep, 0);
	if (*ep && *ep != '\n')
		return -EINVAL;

	jz4780fb_set_backlight_level(n);

	return count;
}

static struct device_attribute device_attrs[] = {
	__ATTR(backlight_level, S_IRUGO | S_IWUSR, show_bl_level, store_bl_level),
};

static int jz4780fb_device_attr_register(struct fb_info *fb_info)
{
	int error = 0;
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(device_attrs); i++) {
		error = device_create_file(fb_info->dev, &device_attrs[i]);

		if (error)
			break;
	}

	if (error) {
		while (--i >= 0)
			device_remove_file(fb_info->dev, &device_attrs[i]);
	}

	return 0;
}

static int jz4780fb_device_attr_unregister(struct fb_info *fb_info)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(device_attrs); i++)
		device_remove_file(fb_info->dev, &device_attrs[i]);

	return 0;
}
/* End */

static void gpio_init(struct lcd_cfb_info *cfb)
{
	__gpio_as_output1(32*1+23);
	__gpio_as_output1(32*4+0);

	__lcd_display_pin_init();

	/* gpio init __gpio_as_lcd */
	if (cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_MODE_TFT_16BIT)
		__gpio_as_lcd_16bit();
	else if (cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_MODE_TFT_24BIT)
		__gpio_as_lcd_24bit();
	else
 		__gpio_as_lcd_18bit();

	/* In special mode, we only need init special pin,
	 * as general lcd pin has init in uboot */
#if defined(CONFIG_SOC_JZ4780)
	switch (cfb->jz4780_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) {
	case LCD_CFG_MODE_SPECIAL_TFT_1:
	case LCD_CFG_MODE_SPECIAL_TFT_2:
	case LCD_CFG_MODE_SPECIAL_TFT_3:
		__gpio_as_lcd_special();
		break;
	default:
		;
	}
#endif

	return;
}

static void slcd_init(void)
{
	/* Configure SLCD module for setting smart lcd control registers */
#if defined(CONFIG_FB_JZ4780_SLCD)
	__lcd_as_smart_lcd(0);
	__init_slcd_bus();	/* Note: modify this depend on you lcd */
#endif
	return;
}

static int __devinit jz4780_fb_probe(struct platform_device *dev)
{
	struct lcd_cfb_info *cfb;
	int irq;
	int rv = 0;

	cfb = kmalloc(sizeof(struct lcd_cfb_info) + sizeof(u32) * 16, GFP_KERNEL);

	if (!cfb)
		return NULL;

	memset(cfb, 0, sizeof(struct lcd_cfb_info) );

	cfb->id = dev->id;

	if(cfb->id == 0){
#if defined(CONFIG_SOC_JZ4780)
		cfb->jz4780_lcd_info = &jz4780_info_hdmi_720x480p; /* default output to lcd panel */ 
		set_i2s_external_codec();
#else
		cfb->jz4780_lcd_info = &jz4780_lcd_panel; //  only lcd0
#endif
		jz4780fb0_info = cfb;
		irq = IRQ_LCD0;
	}
	else{
		cfb->jz4780_lcd_info = &jz4780_lcd_panel; /* default output to lcd panel */
		jz4780fb1_info = cfb;
		irq = IRQ_LCD1;
	}

	jz4780fb_set_fb_info(cfb);

//	screen_off(cfb->id);
//	ctrl_disable(cfb);

	gpio_init(cfb);
	slcd_init();
	
		/* init clk */
	jz4780fb_change_clock(cfb);

	rv = jz4780fb_map_smem(cfb);
	if (rv)
		goto failed;

	spin_lock_init(&cfb->update_lock);
	init_waitqueue_head(&cfb->frame_wq);
	cfb->frame_requested = cfb->frame_done;

	jz4780fb_deep_set_mode(cfb);
	
	
#ifdef CONFIG_JZ_AOSDC
	cfb->aosd_info = kmalloc(sizeof(struct jz_aosd_info) , GFP_KERNEL);
	if (!cfb->aosd_info) {
		printk("malloc aosd_info failed\n");
		return -ENOMEM;
	}
	memset(cfb->aosd_info, 0, sizeof(struct jz_aosd_info));

	jz_aosd_init();
#endif	
	cfb->enh_info = kmalloc(sizeof(struct jz4780lcd_enh_info), GFP_KERNEL);
	if (!cfb->enh_info) {
		printk("malloc enh_info failed\n");
		return -ENOMEM;
	}
	memset(cfb->enh_info, 0, sizeof(struct jz4780lcd_enh_info));

	printk("malloc enh info\n");

	rv = register_framebuffer(&cfb->fb);
	if (rv < 0) {
		D("Failed to register framebuffer device.");
		goto failed;
	}

	printk("fb%d: %s frame buffer device, using %dK of video memory\n",
	       cfb->fb.node, cfb->fb.fix.id, cfb->fb.fix.smem_len>>10);

	jz4780fb_device_attr_register(&cfb->fb);

	if (request_irq(irq, jz4780fb_interrupt_handler, IRQF_DISABLED, "lcd", 0)) {
		D("Faield to request LCD IRQ.\n");
		rv = -EBUSY;
		goto failed;
	}

	ctrl_enable(cfb);
	screen_on(cfb->id);

	if(cfb->id == 1){
#if 0
		REG_LVDS_CTRL &= ~(1 << 18);                /*reset*/
		REG_LVDS_PLL0 &= ~(1 << 29);                /*bg power on*/
		
		mdelay(5);
		
		REG_LVDS_PLL0 |= (1 << 30);                /*pll power on*/
		
		udelay(20);

		REG_LVDS_CTRL |= (1 << 18);                /*reset*/
/*
		while(!(REG_LVDS_PLL0 & (1 << 31))){
			printk("111 REG_LVDS_PLL0:%08x\n",REG_LVDS_PLL0);
			udelay(1000);
		}
*/		
//		REG_LVDS_CTRL = 0x600484a1;
//		REG_LVDS_CTRL = 0x600784a1;
		REG_LVDS_CTRL = 0x600580a1;
		REG_LVDS_PLL0 = 0x40002108 ;
		REG_LVDS_PLL1 = 0x8d000000;
//		REG_LVDS_PLL1 = 0xe9000000;

		while(!(REG_LVDS_PLL0 & (1 << 31))){
			printk("222REG_LVDS_PLL0:%08x\n",REG_LVDS_PLL0);
			udelay(1000);
		}
//		REG_LVDS_CTRL |= ((1 << 1) | (1 << 11));
#endif		
	}

#if JZ_FB_DEBUG
	display_v_color_bar(cfb->jz4780_lcd_info->osd.fg0.w, cfb->jz4780_lcd_info->osd.fg0.h, cfb->jz4780_lcd_info->osd.fg0.bpp, (int *)cfb->lcd_frame0);
	print_lcdc_registers(cfb->id);
#endif
	
	
	return 0;
failed:
	jz4780fb_unmap_smem(cfb);
	jz4780fb_free_fb_info(cfb);

	return rv;
}

static int __devexit jz4780_fb_remove(struct platform_device *pdev)
{
	struct lcd_cfb_info *cfb;

	if(pdev->id == 0)
		cfb = (struct lcd_cfb_info *)jz4780fb1_info;
	else
		cfb = (struct lcd_cfb_info *)jz4780fb0_info;
		
	jz4780fb_device_attr_unregister(&cfb->fb);

#if defined(CONFIG_JZ4770_PISCES)
	act8600_output_enable(ACT8600_OUT6,ACT8600_OUT_OFF);
#endif

	return 0;
}

static struct platform_driver jz4780_fb_driver = {
	.probe	= jz4780_fb_probe,
	.remove = jz4780_fb_remove,
	.suspend = jz4780_fb_suspend,
	.resume = jz4780_fb_resume,
	.driver = {
		.name = "jz-lcd",
		.owner = THIS_MODULE,
	},
};

static int __init jz4780_fb_init(void)
{
	return platform_driver_register(&jz4780_fb_driver);
}

static void __exit jz4780_fb_cleanup(void)
{
	platform_driver_unregister(&jz4780_fb_driver);
}

module_init(jz4780_fb_init);
module_exit(jz4780_fb_cleanup);
