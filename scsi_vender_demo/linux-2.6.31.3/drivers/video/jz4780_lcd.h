/*
 * linux/drivers/video/jz4780_lcd.h -- Ingenic Jz4780 On-Chip LCD frame buffer device
 *
 * Copyright (C) 2005-2008, Ingenic Semiconductor Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#ifndef __JZ4780_LCD_H__
#define __JZ4780_LCD_H__

//#include <asm/io.h>


#define NR_PALETTE	256
#define PALETTE_SIZE	(NR_PALETTE*2)

/* use new descriptor(8 words) */
struct jz4780_lcd_dma_desc {
	unsigned int next_desc; 	/* LCDDAx */
	unsigned int databuf;   	/* LCDSAx */
	unsigned int frame_id;  	/* LCDFIDx */
	unsigned int cmd; 		/* LCDCMDx */
	unsigned int offsize;       	/* Stride Offsize(in word) */
	unsigned int page_width; 	/* Stride Pagewidth(in word) */
	unsigned int cmd_num; 		/* Command Number(for SLCD) and CPOS(except SLCD) */
	unsigned int desc_size; 	/* Foreground Size and alpha value*/
};

struct jz4780lcd_panel_t {
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
};


struct jz4780lcd_fg_t {
	int bpp;	/* foreground bpp */
	int x;		/* foreground start position x */
	int y;		/* foreground start position y */
	int w;		/* foreground width */
	int h;		/* foreground height */
};

struct jz4780lcd_osd_t {
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

	struct jz4780lcd_fg_t fg0;	/* foreground 0 */
	struct jz4780lcd_fg_t fg1;	/* foreground 1 */
};

struct jz4780lcd_info {
	struct jz4780lcd_panel_t panel;
	struct jz4780lcd_osd_t osd;
};

struct jz4780lcd_enh_info{
	unsigned int enh_cfg;
	unsigned int ycc2rgb;
	unsigned int rgb2ycc;
	unsigned int brightness;
	unsigned int contrast;
	unsigned int hue;
	unsigned int saturation;
	unsigned int dither;
	unsigned int gamma[512];
	unsigned int vee[512];
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

/*
 * LCD panel specific definition
 */
/* AUO */
#if defined(CONFIG_JZ4780_LCD_AUO_A043FL01V2)
#if defined(CONFIG_JZ4780_F4780) || defined(CONFIG_JZ4775_F4775) 
	#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/
#else
#error "driver/video/Jzlcd.h, please define SPI pins on your board."
#endif

#define __lcd_special_pin_init()		\
	do {						\
		udelay(50);				\
		__gpio_as_output0(LCD_RET);		\
		udelay(100);				\
		__gpio_as_output1(LCD_RET);		\
	} while (0)
#define __lcd_special_on()			     \
	do {					     \
		udelay(50);			     \
		__gpio_as_output0(LCD_RET);	     \
		udelay(100);			     \
		__gpio_as_output1(LCD_RET);	     \
} while (0)

	#define __lcd_special_off() \
	do { \
		__gpio_as_output0(LCD_RET);		\
	} while (0)

#endif	/* CONFIG_JZLCD_AUO_A030FL01_V1 */

/* TRULY_TFTG320240DTSW */
#if defined(CONFIG_JZ4780_LCD_TRULY_TFTG320240DTSW_16BIT) || defined(CONFIG_JZ4780_LCD_TRULY_TFTG320240DTSW_18BIT)

#if defined(CONFIG_JZ4770_F4770) 
#define LCD_RESET_PIN	(32*5+10)// LCD_REV, GPF10
#else
#error "Define LCD_RESET_PIN on your board"
#endif

#define __lcd_special_on() \
do { \
	__gpio_as_output1(LCD_REAS_OUTPUT1); \
	udelay(100); \
	__gpio_as_output0(LCD_REAS_OUTPUT1); \
	udelay(100); \
	__gpio_as_output1(LCD_REAS_OUTPUT1); \
} while (0)

#endif /* CONFIG_JZ4780_LCD_TRULY_TFTG320240DTSW */

// Wolfgang 2008.02.23
#if defined(CONFIG_JZ4780_LCD_TOPPOLY_TD025THEA7_RGB_DELTA) || defined(CONFIG_JZ4780_LCD_TOPPOLY_TD025THEA7_RGB_DUMMY)

#if defined(CONFIG_JZ4780_LCD_TOPPOLY_TD025THEA7_RGB_DELTA)
#define PANEL_MODE 0x02		/* RGB Delta */
#elif defined(CONFIG_JZ4780_LCD_TOPPOLY_TD025THEA7_RGB_DUMMY)
#define PANEL_MODE 0x00		/* RGB Dummy */
#endif

#if defined(CONFIG_JZ4770_F4770)
	#define SPEN	(32*2+29)       //GPB29
	#define SPCK	(32*2+28)       //GPB28
	#define SPDA	(32*2+21)       //GPB21
	#define LCD_RET (32*5+6)        // GPF6  //use for lcd reset
#else
#error "please define SPI pins on your board."
#endif

	#define __spi_write_reg1(reg, val) \
	do { \
		unsigned char no;\
		unsigned short value;\
		unsigned char a=0;\
		unsigned char b=0;\
		a=reg;\
		b=val;\
		__gpio_as_output1(SPEN);\
		udelay(100);\
		__gpio_as_output0(SPCK);\
		__gpio_as_output0(SPDA);\
		__gpio_as_output0(SPEN);\
		udelay(25);\
		value=((a<<8)|(b&0xFF));\
		for(no=0;no<16;no++)\
		{\
			__gpio_as_output0(SPCK);\
			if((value&0x8000)==0x8000)\
				__gpio_as_output1(SPDA);\
			else\
				__gpio_as_output0(SPDA);\
			udelay(25);\
			__gpio_as_output1(SPCK);\
			value=(value<<1); \
			udelay(25);\
		 }\
		__gpio_as_output0(SPCK);\
		__gpio_as_output1(SPEN);\
		udelay(100);\
	} while (0)

	#define __spi_write_reg(reg, val) \
	do {\
		__spi_write_reg1((reg<<2), val); \
		udelay(100); \
	}while(0)

	#define __lcd_special_pin_init() \
	do { \
		__gpio_as_output1(LCD_RET);\
		mdelay(15);\
		__gpio_as_output0(LCD_RET);\
		mdelay(15);\
		__gpio_as_output1(LCD_RET);\
	} while (0)

	#define __lcd_special_on() \
	do { \
	mdelay(10); \
	__spi_write_reg(0x00, 0x10); \
	__spi_write_reg(0x01, 0xB1); \
	__spi_write_reg(0x00, 0x10); \
	__spi_write_reg(0x01, 0xB1); \
	__spi_write_reg(0x02, PANEL_MODE); /* RGBD MODE */ \
	__spi_write_reg(0x03, 0x01); /* Noninterlace*/	\
	mdelay(10); \
	} while (0)

	#define __lcd_special_off() \
	do { \
	} while (0)

#endif	/* CONFIG_JZ4780_LCD_TOPPOLY_TD025THEA7_RGB_DELTA */


#if defined(CONFIG_JZ4780_LCD_FOXCONN_PT035TN01) || defined(CONFIG_JZ4780_LCD_INNOLUX_PT035TN01_SERIAL)

#if defined(CONFIG_JZ4780_LCD_FOXCONN_PT035TN01) /* board FUWA */
#define MODE 0xcd 		/* 24bit parellel RGB */
#endif
#if defined(CONFIG_JZ4780_LCD_INNOLUX_PT035TN01_SERIAL)
#define MODE 0xc9		/* 8bit serial RGB */
#endif

#if defined(CONFIG_JZ4770_F4770)
        #define SPEN            (32*3+13)       /*LCD_CS GPD13*/
        #define SPCK            (32*4+13)       /*LCD_SCL GPE13*/
        #define SPDA            (32*1+29)       /*LCD_SDA GPB29*/
        #define LCD_RET         (32*4+12)       /*LCD_RESET use for lcd reset*/
#else
#error "driver/video/Jzlcd.h, please define SPI pins on your board."
#endif

	#define __spi_write_reg1(reg, val) \
	do { \
		unsigned char no;\
		unsigned short value;\
		unsigned char a=0;\
		unsigned char b=0;\
		a=reg;\
		b=val;\
		__gpio_as_output1(SPEN);\
		__gpio_as_output1(SPCK);\
		__gpio_as_output0(SPDA);\
		__gpio_as_output0(SPEN);\
		udelay(25);\
		value=((a<<8)|(b&0xFF));\
		for(no=0;no<16;no++)\
		{\
			__gpio_as_output0(SPCK);\
			if((value&0x8000)==0x8000)\
			__gpio_as_output1(SPDA);\
			else\
			__gpio_as_output0(SPDA);\
			udelay(25);\
			__gpio_as_output1(SPCK);\
			value=(value<<1); \
			udelay(25);\
		 }\
		__gpio_as_output1(SPEN);\
		udelay(100);\
	} while (0)

	#define __spi_write_reg(reg, val) \
	do {\
		__spi_write_reg1((reg<<2|2), val); \
		udelay(100); \
	}while(0)

	#define __lcd_special_pin_init() \
	do { \
		__gpio_as_output0(LCD_RET);\
		mdelay(150);\
		__gpio_as_output1(LCD_RET);\
	} while (0)

	#define __lcd_special_on() \
	do { \
		udelay(50);\
		__gpio_as_output0(LCD_RET);\
		mdelay(150);\
		__gpio_as_output1(LCD_RET);\
		mdelay(10);\
		__spi_write_reg(0x00, 0x03); \
		__spi_write_reg(0x01, 0x40); \
		__spi_write_reg(0x02, 0x11); \
		__spi_write_reg(0x03, MODE); /* mode */ \
		__spi_write_reg(0x04, 0x32); \
		__spi_write_reg(0x05, 0x0e); \
		__spi_write_reg(0x07, 0x03); \
		__spi_write_reg(0x08, 0x08); \
		__spi_write_reg(0x09, 0x32); \
		__spi_write_reg(0x0A, 0x88); \
		__spi_write_reg(0x0B, 0xc6); \
		__spi_write_reg(0x0C, 0x20); \
		__spi_write_reg(0x0D, 0x20); \
	} while (0)	//reg 0x0a is control the display direction:DB0->horizontal level DB1->vertical level

/*		__spi_write_reg(0x02, 0x03); \
		__spi_write_reg(0x06, 0x40); \
		__spi_write_reg(0x0a, 0x11); \
		__spi_write_reg(0x0e, 0xcd); \
		__spi_write_reg(0x12, 0x32); \
		__spi_write_reg(0x16, 0x0e); \
		__spi_write_reg(0x1e, 0x03); \
		__spi_write_reg(0x22, 0x08); \
		__spi_write_reg(0x26, 0x40); \
		__spi_write_reg(0x2a, 0x88); \
		__spi_write_reg(0x2e, 0x88); \
		__spi_write_reg(0x32, 0x20); \
		__spi_write_reg(0x36, 0x20); \
*/
//	} while (0)	//reg 0x0a is control the display direction:DB0->horizontal level DB1->vertical level

	#define __lcd_special_off() \
	do { \
		__spi_write_reg(0x00, 0x03); \
	} while (0)

#endif	/* CONFIG_JZ4780_LCD_FOXCONN_PT035TN01 or CONFIG_JZ4780_LCD_INNOLUX_PT035TN01_SERIAL */

#if defined(CONFIG_JZ4780_LCD_TOPPOLY_TD043MGEB1)
#if defined(CONFIG_SOC_JZ4780) || defined(CONFIG_SOC_JZ4775)
#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/
#else
#error "driver/video/jz_toppoly_td043mgeb1.h, please define SPI pins on your board."
#endif

#define __lcd_special_pin_init()		\
	do {					\
		__gpio_as_output0(LCD_RET);	\
		udelay(1000);			\
		__gpio_as_output1(LCD_RET);	\
		udelay(1000);			\
	} while (0)


#define __lcd_special_off() \
	do {					\
		__gpio_as_output0(LCD_RET);	\
		__gpio_as_input(LCD_RET);	\
	} while (0)

#endif	/* LCD_TOPPOLY_TD043MGEB1 */

#if defined(CONFIG_JZ4780_LCD_HANNSTAR_HSD070IDW1)
#if defined(CONFIG_JZ4770_PISCES)
	#define LCD_RESET 	(32*4+5)       /*LCD_RESET use for lcd reset*/
	#define LCD_DISP_N 	(32*4+2)       /*LCD_DISP_N use for lcd display*/

#else
#error "driver/video/jz4770_lcd.h, please define reset pins on your board."
#endif

#define __lcd_special_pin_init()		\
	do {						\
		__gpio_as_output0(LCD_RESET);	\
		udelay(1000);			\
		__gpio_as_output1(LCD_RESET);	\
		udelay(1000);			\
	} while (0)

#define __lcd_special_off() \
	do {					\
		__gpio_as_output0(LCD_DISP_N);	\
	} while (0)

#define __lcd_special_on() \
	do {					\
		__gpio_as_output1(LCD_DISP_N);	\
	} while (0)

#endif //CONFIG_JZ4780_LCD_HANNSTAR_HSD070IDW1

#if defined(CONFIG_JZ4780_LCD_TRULY_TFT_GG1P0319LTSW_W)
static inline void CmdWrite(unsigned int cmd)
{
	while (REG_SLCD_STATE & SLCD_STATE_BUSY); /* wait slcd ready */
	udelay(30);
	REG_SLCD_DATA = SLCD_DATA_RS_COMMAND | cmd;
}

static inline void DataWrite(unsigned int data)
{
	while (REG_SLCD_STATE & SLCD_STATE_BUSY); /* wait slcd ready */
//	udelay(30);
	REG_SLCD_DATA = SLCD_DATA_RS_DATA | data;
}


static inline void delay(long delay_time)
{
	long cnt;

//  delay_time *= (384/8);
	delay_time *= (43/8);

	for (cnt=0;cnt<delay_time;cnt++)
	{
		asm("nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
		    "nop\n"
			);
	}
}


/*---- LCD Initial ----*/
static void SlcdInit(void)
{
  delay(10000);
  CmdWrite(0x0301); //reset
  delay(10000);
  CmdWrite(0x0101);
  CmdWrite(0x0301);
  CmdWrite(0x0008);
  CmdWrite(0x2201);		//reset
  CmdWrite(0x0000);
  CmdWrite(0x0080);   //0x0020
  delay(10000);

  CmdWrite(0x2809);
  CmdWrite(0x1900);
  CmdWrite(0x2110);
  CmdWrite(0x1805);
  CmdWrite(0x1E01);
  CmdWrite(0x1847);
  delay(1000);
  CmdWrite(0x1867);
  delay(10000);
  CmdWrite(0x18F7);
  delay(10000);
  CmdWrite(0x2100);
  CmdWrite(0x2809);
  CmdWrite(0x1a05);
  CmdWrite(0x1900);
  CmdWrite(0x1f64);
  CmdWrite(0x2070);
  CmdWrite(0x1e81);
  CmdWrite(0x1b01);

  CmdWrite(0x0200);
  CmdWrite(0x0504);   //y address increcement
  CmdWrite(0x0D00);		//*240
  CmdWrite(0x1D08);
  CmdWrite(0x2300);
  CmdWrite(0x2D01);
  CmdWrite(0x337F);
  CmdWrite(0x3400);
  CmdWrite(0x3501);
  CmdWrite(0x3700);
  CmdWrite(0x42ef);		//x start from 239
  CmdWrite(0x4300);
  CmdWrite(0x4400);		//y start from 0
  CmdWrite(0x4500);
  CmdWrite(0x46EF);
  CmdWrite(0x4700);
  CmdWrite(0x4800);
  CmdWrite(0x4901);
  CmdWrite(0x4A3F);
  CmdWrite(0x4B00);
  CmdWrite(0x4C00);
  CmdWrite(0x4D00);
  CmdWrite(0x4E00);
  CmdWrite(0x4F00);
  CmdWrite(0x5000);
  CmdWrite(0x7600);
  CmdWrite(0x8600);
  CmdWrite(0x8720);
  CmdWrite(0x8802);
  CmdWrite(0x8903);
  CmdWrite(0x8D40);
  CmdWrite(0x8F05);
  CmdWrite(0x9005);
  CmdWrite(0x9144);
  CmdWrite(0x9244);
  CmdWrite(0x9344);
  CmdWrite(0x9433);
  CmdWrite(0x9505);
  CmdWrite(0x9605);
  CmdWrite(0x9744);
  CmdWrite(0x9844);
  CmdWrite(0x9944);
  CmdWrite(0x9A33);
  CmdWrite(0x9B33);
  CmdWrite(0x9C33);
  //==> SETP 3
  CmdWrite(0x0000);
  CmdWrite(0x01A0);
  CmdWrite(0x3B01);

  CmdWrite(0x2809);
  delay(1000);
  CmdWrite(0x1900);
  delay(1000);
  CmdWrite(0x2110);
  delay(1000);
  CmdWrite(0x1805);
  delay(1000);
  CmdWrite(0x1E01);
  delay(1000);
  CmdWrite(0x1847);
  delay(1000);
  CmdWrite(0x1867);
  delay(1000);
  CmdWrite(0x18F7);
  delay(1000);
  CmdWrite(0x2100);
  delay(1000);
  CmdWrite(0x2809);
  delay(1000);
  CmdWrite(0x1A05);
  delay(1000);
  CmdWrite(0x19E8);
  delay(1000);
  CmdWrite(0x1F64);
  delay(1000);
  CmdWrite(0x2045);
  delay(1000);
  CmdWrite(0x1E81);
  delay(1000);
  CmdWrite(0x1B09);
  delay(1000);
  CmdWrite(0x0020);
  delay(1000);
  CmdWrite(0x0120);
  delay(1000);

  CmdWrite(0x3B01);
  delay(1000);

  /* Set Window(239,319), Set Cursor(239,319) */
  CmdWrite(0x0510);
  CmdWrite(0x01C0);
  CmdWrite(0x4500);
  CmdWrite(0x46EF);
  CmdWrite(0x4800);
  CmdWrite(0x4700);
  CmdWrite(0x4A3F);
  CmdWrite(0x4901);
  CmdWrite(0x42EF);
  CmdWrite(0x443F);
  CmdWrite(0x4301);

}

#if defined(CONFIG_JZ4770_F4770)
#define PIN_CS_N 	(32*1+29)	/* a low voltage */
#define PIN_RD_N 	(32*2+9)	/* LCD_DE: GP C9, a high voltage */
#define PIN_RESET_N 	(32*5+10)	/* LCD_REV GP F10 */
#else
#error "Define special lcd pins for your platform."
#endif

#define __lcd_slcd_pin_init()						\
	do {								\
		__gpio_as_output1(PIN_RD_N); /*set read signal high */	\
		__gpio_as_output1(PIN_RESET_N);				\
		mdelay(100);						\
		__gpio_as_output0(PIN_RESET_N);				\
		mdelay(100);						\
		__gpio_as_output1(PIN_RESET_N);				\
		/* Configure SLCD module */				\
		REG_LCD_CTRL &= ~(LCD_CTRL_ENA|LCD_CTRL_DIS); /* disable lcdc */ \
		REG_LCD_CFG = LCD_CFG_LCDPIN_SLCD | 0x0D; /* LCM */	\
		REG_SLCD_CTRL &= ~SLCD_CTRL_DMA_EN; /* disable slcd dma */ \
		REG_SLCD_CFG = SLCD_CFG_DWIDTH_16BIT | SLCD_CFG_CWIDTH_16BIT | SLCD_CFG_CS_ACTIVE_LOW | SLCD_CFG_RS_CMD_LOW | SLCD_CFG_CLK_ACTIVE_FALLING | SLCD_CFG_TYPE_PARALLEL; \
		REG_LCD_REV = 0x04;	/* lcd clock??? */			\
		printk("Fuwa test, pixclk divide REG_LCD_REV=0x%08x\n", REG_LCD_REV); \
}while (0)

#define __lcd_slcd_special_on()						\
	do {								\
		__lcd_slcd_pin_init();					\
		SlcdInit();						\
		REG_SLCD_CTRL |= SLCD_CTRL_DMA_EN; /* slcdc dma enable */ \
	} while (0)

#endif	/* #if CONFIG_JZ4780_LCD_TRULY_TFT_GG1P0319LTSW_W */

#if defined(CONFIG_JZ4780_LCD_HYNIX_HT12X14)

#if defined(CONFIG_JZ4780_F4780) || defined (CONFIG_JZ4775_F4775)
	#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/
#else
#error "driver/video/jz4770_lcd.h, please define I2C0 pins on your board."
#endif

#define __lcd_special_pin_init()		\
	do {						\
		__gpio_as_output0(LCD_RET);	\
		udelay(1000);			\
		__gpio_as_output1(LCD_RET);	\
		udelay(1000);			\
	} while (0)

#define __lcd_special_off() \
	do {					\
		__gpio_as_output0(LCD_RET);	\
		__gpio_as_input(LCD_RET);	\
	} while (0)
#endif //CONFIG_JZ4780_LCD_HYNIX_HT12X14

#if defined(CONFIG_JZ4780_LCD_KD101N2) || defined(CONFIG_JZ4780_LCD_APPROVAL)
#if defined(CONFIG_JZ4770_F4770)
	#define ISPCK		(32*3+31)       /*LCD_ISCL*/
	#define ISPDA		(32*3+30)       /*LCD_ISDA*/
	#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4770_PISCES)
	#define ISPCK		(32*3+31)       /*LCD_ISCL*/
	#define ISPDA		(32*3+30)       /*LCD_ISDA*/
	#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4780_F4780) || defined(CONFIG_JZ4775_F4775)
	#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/

#else
#error "driver/video/jz4770_lcd.h, please define I2C0 pins on your board."
#endif

#define __lcd_special_pin_init()		\
	do {						\
		__gpio_as_output0(LCD_RET);	\
		udelay(1000);			\
		__gpio_as_output1(LCD_RET);	\
		udelay(1000);			\
	} while (0)

#define __lcd_special_off() \
	do {					\
		__gpio_as_output0(LCD_RET);	\
		__gpio_as_input(LCD_RET);	\
	} while (0)
#endif //CONFIG_JZ4780_LCD_KD101N2

#if defined(CONFIG_JZ4780_LCD_KD101N4)
#if defined(CONFIG_JZ4770_F4770)
	#define ISPCK		(32*3+31)       /*LCD_ISCL*/
	#define ISPDA		(32*3+30)       /*LCD_ISDA*/
	#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/
#elif defined(CONFIG_JZ4770_PISCES)
	#define ISPCK		(32*3+31)       /*LCD_ISCL*/
	#define ISPDA		(32*3+30)       /*LCD_ISDA*/
	#define LCD_RET 	(32*4+2)       /*LCD_DISP_N use for lcd reset*/
#else
#error "driver/video/jz4770_lcd.h, please define I2C0 pins on your board."
#endif

#define __lcd_special_pin_init()		\
	do {						\
		__gpio_as_output1(ISPCK);			\
		udelay(500);			\
		__gpio_as_output1(ISPDA);			\
		udelay(500);			\
		__gpio_as_output0(LCD_RET);	\
		udelay(1000);			\
		__gpio_as_output1(LCD_RET);	\
		udelay(1000);			\
	} while (0)

#define __lcd_special_off() \
	do {					\
		__gpio_as_output0(LCD_RET);	\
		__gpio_as_input(LCD_RET);	\
	} while (0)
#endif //CONFIG_JZ4780_LCD_KD101N4

#ifndef __lcd_special_pin_init
#define __lcd_special_pin_init()
#endif
#ifndef __lcd_special_on
#define __lcd_special_on()
#endif
#ifndef __lcd_special_off
#define __lcd_special_off()
#endif


/*
 * Platform specific definition
 */
#if defined(CONFIG_JZ4780_VGA_DISPLAY)
#define __lcd_display_pin_init()
#define __lcd_display_on()
#define __lcd_display_off()
#elif defined(CONFIG_SOC_JZ4780)
#define __lcd_display_pin_init() \
do { \
	__lcd_special_pin_init();	   \
} while (0)

#define __lcd_display_on() \
do { \
	__lcd_special_on();			\
} while (0)

#define __lcd_display_off() \
do { \
	__lcd_special_off(); \
} while (0)

#else /* other boards */

#define __lcd_display_pin_init() \
do { \
	__lcd_special_pin_init();	   \
} while (0)
#define __lcd_display_on() \
do { \
	__lcd_special_on();			\
	__lcd_set_backlight_level(80);		\
} while (0)

#define __lcd_display_off() \
do { \
	__lcd_close_backlight();	   \
	__lcd_special_off();	 \
} while (0)
#endif /* LEPUS */


/*****************************************************************************
 * LCD display pin dummy macros
 *****************************************************************************/

#ifndef __lcd_display_pin_init
#define __lcd_display_pin_init()
#endif
#ifndef __lcd_slcd_special_on
#define __lcd_slcd_special_on()
#endif
#ifndef __lcd_display_on
#define __lcd_display_on()
#endif
#ifndef __lcd_display_off
#define __lcd_display_off()
#endif
#ifndef __lcd_set_backlight_level
#define __lcd_set_backlight_level(n)
#endif

#endif /* __JZ4780_LCD_H__ */
