#ifndef __LCD_PANEL_H__
#define __LCD_PANEL_H__

#ifdef CONFIG_FB_JZ_TVE

struct jzlcd_info jz_lcd_panel = {
	.panel = {
		.cfg = LCD_CFG_TVEN | /* output to tve */
		LCD_CFG_NEWDES | /* 8words descriptor */
		LCD_CFG_RECOVER | /* underrun protect */
		LCD_CFG_MODE_INTER_CCIR656, /* Interlace CCIR656 mode */
		.ctrl = LCD_CTRL_OFUM | LCD_CTRL_BST_16,	/* 16words burst */
		TVE_WIDTH_PAL, TVE_HEIGHT_PAL, TVE_FREQ_PAL, 0, 0, 0, 0, 0, 0,
		.type = DISP_PANEL_TYPE_TVE,
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
	}
};

void display_panel_init(void *ptr)
{

}

#endif

#endif