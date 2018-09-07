/*
 * linux/drivers/video/jz_lcd.c -- Ingenic Jz LCD frame buffer device
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
#ifdef CONFIG_PMU_ACT8600_SUPPORT
#include <linux/act8600_power.h>
#endif
#include <linux/timer.h>

#include <asm/irq.h>
#include <asm/pgtable.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <asm/processor.h>
#include <asm/jzsoc.h>

#include "console/fbcon.h"
#include "jz4760_tve.h"
#include "jz4775_lcd.h"

/*add by qinbh*/
#include "move/logo.newC330.h"
#include "move/logo.h"
#include "move/move.h"

#if defined(CONFIG_JZ_LCD_BL35026V0)
#include "move/a.h"
#endif

MODULE_DESCRIPTION("Jz LCD Controller driver");
MODULE_AUTHOR("lltang, <lltang@ingenic.cn>");
MODULE_LICENSE("GPL");


//#define JZ_FB_DEBUG 1

#ifdef JZ_FB_DEBUG
	#define D(fmt, args...) \
		printk(KERN_ERR "%s(): "fmt"\n", __func__, ##args)
#else
	#define	D(fmt, args...)
#endif 
	
#if defined(CONFIG_JZ_LCD_TOPPOLY_TD043MGEB1)
	#include "panel/lcd_toppoly_td043mgeb1.h"	
#elif defined(CONFIG_JZ_LCD_AUO_A043FL01V2)
	#include "panel/lcd_auo_a043fl01v2.h"
#elif defined(CONFIG_JZ_SLCD_KGM701A3_TFT_SPFD5420A)
	#include "panel/jz_kgm_spfd5420a.h"
#elif defined(CONFIG_JZ4770_SLCD_FORISE_BL28021)
	#include "panel/jz_slcd_bl28021.h"
#elif defined(CONFIG_JZ_LCD_BL35026V0)
	#include "panel/jz_lcd_bl35026v0.h"
#else
	#error "Select LCD panel first!!!"
#endif


#define DMA_DESC_NUM 		11
#define FB_ALIGN   		(16 * 4)

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
	struct jz_lcd_dma_desc *dma_desc_base;
	struct jz_lcd_dma_desc *dma0_desc_palette, *dma0_desc0, *dma0_desc1, *dma1_desc0, *dma1_desc1, *dma0_tv_desc0, *dma0_tv_desc1, *dma1_tv_desc0, *dma1_tv_desc1;

	unsigned char *lcd_palette;
	unsigned char *lcd_frame0;
	unsigned char *lcd_frame1;

	struct jz_lcd_dma_desc *dma0_desc_cmd0, *dma0_desc_cmd;
	unsigned char *lcd_cmdbuf;

	int is_fg1;
	struct jzlcd_info *jz_lcd_info;

	int fg0_use_x2d_block_mode;
	int fg1_use_x2d_block_mode;

};

struct lcd_cfb_info *jzfb_info;


static void jzfb_deep_set_mode(struct lcd_cfb_info *cfb);
void jzfb_reset_descriptor(struct lcd_cfb_info *cfb);
static int jzfb_set_backlight_level(struct lcd_cfb_info *cfb_info, int n);

static int screen_on(void *ptr);
static int screen_off(void *ptr);

static void display_h_color_bar(int w, int h, int bpp, int *ptr); 
static void display_v_color_bar(int w, int h, int bpp, int *ptr);

#ifdef JZ_FB_DEBUG
static void print_lcdc_registers(struct lcd_cfb_info *cfb_info)	/* debug */
{
	int n = cfb_info->id;
	
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

	/* Smart LCD Controller Resgisters */
	printk("REG_SLCD_CFG:\t0x%08x\n", REG_SLCD_CFG);
	printk("REG_SLCD_CTRL:\t0x%08x\n", REG_SLCD_CTRL);
	printk("REG_SLCD_STATE:\t0x%08x\n", REG_SLCD_STATE);
	printk("==================================\n");

	printk("==================================\n");
        printk("REG_CPM_CPCCR:\t0x%08x\n", REG_CPM_CPCCR);
        printk("REG_CPM_LPCDR0:\t0x%08x\n", REG_CPM_LPCDR0);

}

static void dump_dma_desc( struct jz_lcd_dma_desc *desc)
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

#else
#define print_lcdc_registers(n)
#define dump_dma_desc(a)
#endif

/* Display Logo, add by qinbh */
static struct timer_list logo_timer;
static struct timer_list resetadd_timer;

static int DisplayLogo(int sx, int sy, int ex, int ey, unsigned char *data)
{
	unsigned char *ptr;
	int y,x,i,j;
	int tmp1,tmp2;
	int tmp3,tmp4;
	int pic_high = ey - sy;	
	int pic_width = ex - sx;

	ptr = (unsigned char *)jzfb_info->lcd_frame0;
	
	for(x = sy,i = 0; x < ey; x ++,i++) {
		tmp1 = x * pic_width * 4 ;
		tmp3 = i * 4;
		for(y = sx,j = 0; y < ex; y ++,j ++) {
			tmp2 = y * 4;
			tmp4 = pic_high * j * 4;
		  	
			ptr[tmp1 + tmp2 + 0]=data[tmp3 + tmp4 + 0];
			ptr[tmp1 + tmp2 + 1]=data[tmp3 + tmp4 + 1];
			ptr[tmp1 + tmp2 + 2]=data[tmp3 + tmp4 + 2];
		}
	}	

	dma_cache_wback((unsigned long)(jzfb_info->lcd_frame0), (320)*240*4);

	return 0;
}

static void logo_proc(void)
{
	static int move_point = 0;
#if defined(CONFIG_JZ_LCD_BL35026V0)

#else
	DisplayLogo(65 ,132, 116 + 65,37 + 132, (unsigned char *)mylogo01[move_point]);
	move_point = (move_point+1)%14;
#endif
}
extern int LED_OPENED;
static void myfunc(unsigned long data)
{
	logo_proc();
	if(LED_OPENED)
	{
		del_timer(&logo_timer);
		// printk("logo timer deleted\n");		// add, yjt, 20130513
	}
	else
	{
		mod_timer(&logo_timer, jiffies+msecs_to_jiffies(250));
		// printk("logo timer modify\n");		// add, yjt, 20130513
	}
}
static void MoveLogo_timer_init(void)
{
	setup_timer(&logo_timer, myfunc, 0);
	logo_timer.expires = jiffies + msecs_to_jiffies(100);
	add_timer(&logo_timer);

	return;
}

#if defined(CONFIG_JZ4770_SLCD_FORISE_BL28021)		// if ... endif, yjt, 20130821
#define RESET_CYCLE (5000)
#define RESET_INIT_CYCLE (5000 * 3)
static void Reset_slcd_address_func(void)
{
	//printk("Now is:%s\n",__func__);
	Mcupanel_ResetStartAddr();
	mod_timer(&resetadd_timer,jiffies + msecs_to_jiffies(RESET_CYCLE));
}
static void ResetSlcdStartAddr_timer_init(void)
{
	printk("Reset SLCD start-point timer init!\n");
	setup_timer(&resetadd_timer,Reset_slcd_address_func,0);
	resetadd_timer.expires = jiffies + msecs_to_jiffies(RESET_INIT_CYCLE);
	add_timer(&resetadd_timer);
}
#endif
/* End of Display logo */

//backlight
static void lcd_open_backlight(void *ptr)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)ptr;
	int n = cfb->backlight_level;

	if (cfb->b_lcd_pwm == LCD_BACKLIGHT_TYPE_PWM) {
		__gpio_as_pwm(1);                             
		__tcu_disable_pwm_output(LCD_PWM_CHN);	
		__tcu_stop_counter(LCD_PWM_CHN);		
		__tcu_init_pwm_output_high(LCD_PWM_CHN);
		__tcu_set_pwm_output_shutdown_abrupt(LCD_PWM_CHN);
		__tcu_select_clk_div1(LCD_PWM_CHN);		
		__tcu_mask_full_match_irq(LCD_PWM_CHN);		
		__tcu_mask_half_match_irq(LCD_PWM_CHN);		
		__tcu_clear_counter_to_zero(LCD_PWM_CHN);	
		__tcu_set_full_data(LCD_PWM_CHN, JZ_EXTAL / 30000);	
		__tcu_set_half_data(LCD_PWM_CHN, JZ_EXTAL / 30000 * n / LCD_PWM_FULL); 
		__tcu_enable_pwm_output(LCD_PWM_CHN);			
		__tcu_select_extalclk(LCD_PWM_CHN);		
		__tcu_start_counter(LCD_PWM_CHN);	
	} else if (cfb->b_lcd_pwm == LCD_BACKLIGHT_TYPE_LEVEL) {
		__gpio_as_output1(GPIO_LCD_BACKLIGHT);
	}
}

static void lcd_close_backlight(void *ptr)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)ptr;
	
	if (cfb->b_lcd_pwm == LCD_BACKLIGHT_TYPE_PWM) {
		__tcu_disable_pwm_output(LCD_PWM_CHN);	
		__tcu_stop_counter(LCD_PWM_CHN);
	} else if (cfb->b_lcd_pwm == LCD_BACKLIGHT_TYPE_LEVEL) {
		__gpio_as_output0(GPIO_LCD_BACKLIGHT);
	}	
}

static void lcd_set_backlight_level(void *ptr)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)ptr;
	int n = cfb->backlight_level;

	if (cfb->b_lcd_pwm == LCD_BACKLIGHT_TYPE_PWM) {
		__tcu_set_full_data(LCD_PWM_CHN, JZ_EXTAL / 30000);	
		__tcu_set_half_data(LCD_PWM_CHN, JZ_EXTAL / 30000 * n / LCD_PWM_FULL); 
	}
	else if (cfb->b_lcd_pwm == LCD_BACKLIGHT_TYPE_LEVEL) {
		__gpio_as_output1(GPIO_LCD_BACKLIGHT);
	}
}

//power
static void display_on(void *ptr)
{
	__gpio_as_output1(GPIO_LCD_POWER);
	
#if (defined(CONFIG_JZ4775_MENSA) && defined(CONFIG_PMU_ACT8600_SUPPORT))
	act8600_output_enable(ACT8600_OUT7, ACT8600_OUT_ON);
#endif
}

static void display_off(void *ptr)
{
	__gpio_as_output0(GPIO_LCD_POWER);
#if (defined(CONFIG_JZ4775_MENSA) && defined(CONFIG_PMU_ACT8600_SUPPORT))
	act8600_output_enable(ACT8600_OUT7, ACT8600_OUT_OFF);
#endif
}

static void ctrl_enable(struct lcd_cfb_info *cfb)
{
	__lcd_state_clr_all(cfb->id);
	__lcd_osds_clr_all(cfb->id);
	__lcd_clr_dis(cfb->id);

	if ( cfb->jz_lcd_info->panel.type  == DISP_PANEL_TYPE_SLCD) { /* enable Smart LCD DMA */
		REG_SLCD_CTRL |= ((1 << 0) | (1 << 2));	
		REG_SLCD_CTRL &= ~(1 << 3);
		
		//first enable lcd controller, then write slcd init 
		__lcd_set_ena(cfb->id); 	/* enable lcdc */
		display_panel_init(cfb);

		REG_SLCD_CTRL &= ~((1 << 2));
	} else {
		display_panel_init(cfb);
		__lcd_set_ena(cfb->id); 	/* enable lcdc */
	}
}

static void ctrl_disable(struct lcd_cfb_info *cfb)
{
	if ( (cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_SLCD) ||
		(cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_TVE) ) /*  */
		__lcd_clr_ena(cfb->id); /* Smart lcd and TVE mode only support quick disable */
	else {
		__lcd_set_dis(cfb->id); /* regular disable */
		while(!__lcd_disable_done(cfb->id));
		__lcd_clr_ldd(cfb->id);
	}
}

static inline u_int chan_to_field(u_int chan, struct fb_bitfield *bf)
{
        chan &= 0xffff;
        chan >>= 16 - bf->length;
        return chan << bf->offset;
}

static int jzfb_setcolreg(u_int regno, u_int red, u_int green, u_int blue,
			  u_int transp, struct fb_info *info)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	unsigned short *ptr, ctmp;


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
			if (((cfb->jz_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) == LCD_CFG_MODE_SINGLE_MSTN ) ||
			    ((cfb->jz_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) == LCD_CFG_MODE_DUAL_MSTN )) {
				ctmp = (77L * red + 150L * green + 29L * blue) >> 8;
				ctmp = ((ctmp >> 3) << 11) | ((ctmp >> 2) << 5) | (ctmp >> 3);
			} else {
				ctmp = ((red) << 11) | ((green) << 5) | (blue );
			}
	
			ptr = (unsigned short *)cfb->lcd_palette;
			ptr = (unsigned short *)(((u32)ptr)|0xa0000000);
			ptr[regno] = ctmp;
	
			break;
	
		case 15:
			if (regno < 16)
				((u32 *)cfb->fb.pseudo_palette)[regno] =
					((red >> 3) << 10) | ((green >> 3) << 5) | (blue >> 3);
			break;
		case 16:
			if (regno < 16) {
				((u32 *)cfb->fb.pseudo_palette)[regno] = 
					((red >> 3) << 11) | ((green >> 2) << 5) | (blue >> 3);
			}
			break;
		case 17 ... 32:
			if (regno < 16)
				((u32 *)cfb->fb.pseudo_palette)[regno] = 
					(red << 16) | (green << 8) | (blue << 0);
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
static void jzlcd_info_switch_to_TVE(struct lcd_cfb_info *cfb, int mode)
{
	struct jzlcd_info *info;
	struct jzlcd_osd_t *osd_lcd;
	int x, y, w, h;

	info = cfb->jz_lcd_info = &jz_lcd_panel;
	osd_lcd = &jz_lcd_panel.osd;

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

int fg0_should_use_x2d_block_mode(struct lcd_cfb_info *cfb)
{
	return cfb->fg0_use_x2d_block_mode;
}

int fg1_should_use_x2d_block_mode(struct lcd_cfb_info *cfb)
{
	return cfb->fg1_use_x2d_block_mode;
}

static int jzfb_ioctl(struct fb_info *info, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	void __user *argp = (void __user *)arg;
	struct jzlcd_info usrinfo;

	int x2d_en;
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;

	switch (cmd) {
	case FBIO_DO_REFRESH:
                //Mcupanel_ResetStartAddr();
                break;
	case FBIOSETBACKLIGHT:
		D("FBIO SET BLACKLIGHT\n");
		jzfb_set_backlight_level(cfb, arg);
		break;

	case FBIODISPON:
		D("FBIO DIS ON\n");
		ctrl_enable(cfb);
		screen_on(cfb);

		break;

	case FBIODISPOFF:
		D("FBIO DIS OFF\n");
		screen_off(cfb);
		ctrl_disable(cfb);

		break;
	case FBIOPRINT_REG:
		D("FBIO debug regs\n");
		print_lcdc_registers(cfb);
		break;

	case FBIO_GET_MODE:
		D("fbio get mode\n");
		if (copy_to_user(argp, cfb->jz_lcd_info, sizeof(struct jzlcd_info)))
			return -EFAULT;
			
		break;

	case FBIO_RESET_DESC:
		D("reset descriptor\n");
		if (copy_from_user(cfb->jz_lcd_info,argp, sizeof(struct jzlcd_info)))
			return -EFAULT;

		jzfb_reset_descriptor(cfb);

		break;

	case FBIO_SET_FG:
		D("set fg\n");
		if (copy_from_user(&cfb->is_fg1, argp, sizeof(int)))
			return -EFAULT;

		break;

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
#ifdef CONFIG_FB_JZ_TVE
			case PANEL_MODE_TVE_PAL: 	/* switch to TVE_PAL mode */
			case PANEL_MODE_TVE_NTSC: 	/* switch to TVE_NTSC mode */
				jzlcd_info_switch_to_TVE(arg);
				jztve_init(arg); /* tve controller init */
				udelay(100);
				cpm_start_clock(CGM_TVE);
				jztve_enable_tve();
				REG_TVE_CTRL |= (1 << 21);
				/* turn off lcd backlight */
				screen_off(cfb->id);
				break;
#endif
			case PANEL_MODE_LCD_PANEL: 	/* switch to LCD mode */
			default :
				/* turn off TVE, turn off DACn... */
#ifdef CONFIG_FB_JZ_TVE
				jztve_disable_tve();
				cpm_stop_clock(CGM_TVE);
#endif
				cfb->jz_lcd_info = &jz_lcd_panel;
				/* turn on lcd backlight */
				screen_on(cfb);
				break;
		}
		jzfb_deep_set_mode(cfb);
#ifdef JZ_FB_DEBUG
		display_h_color_bar(cfb->jz_lcd_info->osd.fg0.w, cfb->jz_lcd_info->osd.fg0.h, cfb->jz_lcd_info->osd.fg0.bpp, (int *)cfb->lcd_frame0);
#endif
		break;

#ifdef CONFIG_FB_JZ_TVE
	case FBIO_GET_TVE_MODE:
		D("fbio get TVE mode\n");
		if (copy_to_user(argp, jz_tve_info, sizeof(struct jztve_info)))
			return -EFAULT;
		break;
	case FBIO_SET_TVE_MODE:
		D("fbio set TVE mode\n");
		if (copy_from_user(jz_tve_info, argp, sizeof(struct jztve_info)))
			return -EFAULT;
		/* set tve mode */
		jztve_set_tve_mode(jz_tve_info);
		break;
#endif
	default:
		printk("%s, unknown command(0x%x)", __FILE__, cmd);
		break;
	}

	return ret;
}

/* Use mmap /dev/fb can only get a non-cacheable Virtual Address. */
static int jzfb_mmap(struct fb_info *info, struct vm_area_struct *vma)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	unsigned long start;
	unsigned long off;
	u32 len;
	
	D("%s, %s, %d\n", __FILE__, __FUNCTION__, __LINE__);
	
	//fb->fb_get_fix(&fix, PROC_CONSOLE(info), info);

	/* frame buffer memory */
	start = cfb->fb.fix.smem_start;
	len = PAGE_ALIGN((start & ~PAGE_MASK) + cfb->fb.fix.smem_len);
	start &= PAGE_MASK;
	off = vma->vm_pgoff << PAGE_SHIFT;
	
	if ((vma->vm_end - vma->vm_start + off) > len)
		return -EINVAL;
	off += start;

	vma->vm_pgoff = off >> PAGE_SHIFT;
	vma->vm_flags |= VM_IO;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);	/* Uncacheable */

 	pgprot_val(vma->vm_page_prot) &= ~_CACHE_MASK;
 	pgprot_val(vma->vm_page_prot) |= _CACHE_UNCACHED;		/* Uncacheable */
//	pgprot_val(vma->vm_page_prot) |= _CACHE_CACHABLE_NONCOHERENT;	/* Write-Back */

	if (io_remap_pfn_range(vma, vma->vm_start, off >> PAGE_SHIFT,
			       vma->vm_end - vma->vm_start,
			       vma->vm_page_prot)) {
		return -EAGAIN;
	}
	return 0;
}

/* checks var and eventually tweaks it to something supported,
 * DO NOT MODIFY PAR */
static int jzfb_check_var(struct fb_var_screeninfo *var, struct fb_info *info)
{
	printk("jzfb_check_var, not implement\n");
	return 0;
}


/*
 * set the video mode according to info->var
 */
static int jzfb_set_par(struct fb_info *info)
{
	printk("jzfb_set_par, not implemented\n");
	return 0;
}


/*
 * (Un)Blank the display.
 * Fix me: should we use VESA value?
 */
static int jzfb_blank(int blank_mode, struct fb_info *info)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;

	D("jz fb_blank %d %p", blank_mode, info);
	
	switch (blank_mode) {
		case FB_BLANK_UNBLANK:
			__lcd_set_ena(cfb->id);
			screen_on(cfb);
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
			screen_off(cfb);
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
static int jzfb_pan_display(struct fb_var_screeninfo *var, struct fb_info *info)
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
static struct fb_ops jzfb_ops = {
	.owner			= THIS_MODULE,
	.fb_setcolreg		= jzfb_setcolreg,
	.fb_check_var 		= jzfb_check_var,
	.fb_set_par 		= jzfb_set_par,
	.fb_blank		= jzfb_blank,
	.fb_pan_display		= jzfb_pan_display,
	.fb_fillrect		= cfb_fillrect,
	.fb_copyarea		= cfb_copyarea,
	.fb_imageblit		= cfb_imageblit,
	.fb_mmap		= jzfb_mmap,
	.fb_ioctl		= jzfb_ioctl,
};

static int jzfb_set_var(struct fb_var_screeninfo *var, int con,
			struct fb_info *info)
{
	struct lcd_cfb_info *cfb = (struct lcd_cfb_info *)info;
	struct jzlcd_info *lcd_info = cfb->jz_lcd_info;
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

	return 0;
}

static struct lcd_cfb_info * jzfb_set_fb_info(struct lcd_cfb_info *cfb)
{
	cfb->backlight_level	= LCD_DEFAULT_BACKLIGHT;
	cfb->b_lcd_display		= 0;
//	cfb->b_lcd_pwm			= LCD_BACKLIGHT_TYPE_DEFAULT;
	cfb->b_lcd_pwm			= LCD_BACKLIGHT_TYPE_LEVEL;
	
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

	cfb->fb.fbops		= &jzfb_ops;
	cfb->fb.flags		= FBINFO_FLAG_DEFAULT;

	cfb->fb.pseudo_palette	= (void *)(cfb + 1);

	switch (cfb->jz_lcd_info->osd.fg0.bpp) {
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
static int jzfb_map_smem(struct lcd_cfb_info *cfb)
{
	unsigned long page;
	int line_length;
	unsigned int page_shift, needroom, needroom1, bpp, w, h;

	bpp = 32;
	w = cfb->jz_lcd_info->panel.w;
	h = cfb->jz_lcd_info->panel.h;
	/* FrameBuffer Size */
	line_length = (w * bpp) >> 3;
	line_length = (line_length + (FB_ALIGN-1)) & (~(FB_ALIGN-1));
	needroom1 = needroom = line_length * h;
	
	printk("w:%d, bpp:%d\t, line_length:%d\n",w, bpp,line_length);

#if defined(CONFIG_FB_JZ_LCD_USE_2LAYER_FRAMEBUFFER)
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

	cfb->dma_desc_base = (struct jz_lcd_dma_desc *)((void*)cfb->lcd_palette + ((PALETTE_SIZE+3)/4)*4);

	//slcd
	if (cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_SLCD) {
		cfb->lcd_cmdbuf = (unsigned char *)__get_free_pages(GFP_KERNEL, 0);
		memset((void *)cfb->lcd_cmdbuf, 0, PAGE_SIZE);
	
		{	int data, i, *ptr;
			ptr = (unsigned int *)cfb->lcd_cmdbuf;
			data = 0x0022;	//WR_GRAM_CMD;
			data = ((data & 0xff) << 1) | ((data & 0xff00) << 2);
			for(i = 0; i < 3; i++) {
				ptr[i] = data;
			}
		}
	}

#if defined(CONFIG_FB_JZ_LCD_USE_2LAYER_FRAMEBUFFER)
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
		printk("jzfb, %s: unable to map screen memory\n", cfb->fb.fix.id);
		return -ENOMEM;
	}

	return 0;
}

static void jzfb_free_fb_info(struct lcd_cfb_info *cfb)
{
	if (cfb) {
		fb_alloc_cmap(&cfb->fb.cmap, 0, 0);
		kfree(cfb);
	}
}

static void jzfb_unmap_smem(struct lcd_cfb_info *cfb)
{
	struct page * map = NULL;
	unsigned char *tmp;
	unsigned int page_shift, needroom;
	unsigned int bpp, w, h, line_length;
	
	bpp = 32;
	w = cfb->jz_lcd_info->panel.w;
	h = cfb->jz_lcd_info->panel.h;
	
	/* FrameBuffer Size */
	line_length = (w * bpp) >> 3;
	line_length = (line_length + (FB_ALIGN-1)) & (~(FB_ALIGN-1));
	needroom = line_length * h;

#if defined(CONFIG_FB_JZ_LCD_USE_2LAYER_FRAMEBUFFER)
	needroom *= 2;
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
static void jzfb_descriptor_init(struct lcd_cfb_info* cfb)
{
	struct jzlcd_info * lcd_info = cfb->jz_lcd_info;
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
	cfb->dma0_desc_cmd0 	= cfb->dma_desc_base + 3; /* use only once */
	cfb->dma0_desc_cmd 		= cfb->dma_desc_base + 4;
	cfb->dma1_desc0 		= cfb->dma_desc_base + 5;
	cfb->dma1_desc1 		= cfb->dma_desc_base + 6;
	cfb->dma0_tv_desc0      = cfb->dma_desc_base + 7;
	cfb->dma0_tv_desc1      = cfb->dma_desc_base + 8;
	cfb->dma1_tv_desc0      = cfb->dma_desc_base + 9;
	cfb->dma1_tv_desc1      = cfb->dma_desc_base + 10;

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

	if (cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_SLCD) {
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
	} else {
		/* Palette Descriptor */
		cfb->dma0_desc_palette->next_desc 	= (unsigned int)virt_to_phys(cfb->dma0_desc0);	
	}
	
	cfb->dma0_desc_palette->databuf 	= (unsigned int)virt_to_phys((void *)cfb->lcd_palette);
	cfb->dma0_desc_palette->frame_id 	= (unsigned int)0xaaaaaaaa;
	cfb->dma0_desc_palette->cmd 		= LCD_CMD_PAL | pal_size; /* Palette Descriptor */

	/* DMA0 Descriptor0 */
	if (lcd_info->panel.type == DISP_PANEL_TYPE_TVE) {
		/* TVE mode */
		cfb->dma0_desc0->next_desc 	= (unsigned int)virt_to_phys(cfb->dma0_desc1);
	} else 	if (cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_SLCD) {	
		/* smart LCD */
		cfb->dma0_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc_cmd);
	} else {	/* Normal TFT LCD */
		cfb->dma0_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc0); 			
	}

	cfb->dma0_desc0->databuf = virt_to_phys((void *)cfb->lcd_frame0);
	cfb->dma0_desc0->frame_id = (unsigned int)0x0000da00; /* DMA0'0 */
	cfb->dma0_desc0->cmd_num = 0;
	cfb->dma0_desc0->cmd = 0;
	cfb->dma0_desc0->page_width = 0;
	cfb->dma0_desc0->offsize = 0;

	/* DMA0 Descriptor1 */
	if (lcd_info->panel.type == DISP_PANEL_TYPE_TVE ) { /* TVE mode */
		cfb->dma0_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc0);
	} else if (cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_SLCD) {  /* for smatlcd */
		cfb->dma0_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc_cmd);
	} else {
		cfb->dma0_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc1);
	}
	
	cfb->dma0_desc1->frame_id = (unsigned int)0x0000da01; /* DMA0'1 */
	cfb->dma0_desc1->cmd_num = 0;
	cfb->dma0_desc1->page_width = 0;
	cfb->dma0_desc1->offsize = 0;

	if (lcd_info->osd.fg0.bpp <= 8) { /* load palette only once at setup */
		REG_LCD_DA0(n) = virt_to_phys(cfb->dma0_desc_palette);
	} else { 
		if (cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_SLCD) {  /* for smartlcd */
			REG_LCD_DA0(n) = virt_to_phys(cfb->dma0_desc_cmd0); //smart lcd
		} else {
#if defined(CONFIG_JZ_LCD_BL35026V0)
			REG_LCD_DA0(n) = virt_to_phys(cfb->dma0_desc0); //tft
#endif
		}
	}
	
	/* DMA1 Descriptor0 */
	if (lcd_info->panel.type == DISP_PANEL_TYPE_TVE ) { /* TVE mode */
		cfb->dma1_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma1_desc1);
	} else {			/* Normal TFT LCD */
		cfb->dma1_desc0->next_desc = (unsigned int)virt_to_phys(cfb->dma1_desc0);	
	}
	cfb->dma1_desc0->databuf = virt_to_phys((void *)cfb->lcd_frame1);
	cfb->dma1_desc0->frame_id = (unsigned int)0x0000da10; /* DMA1'0 */
	cfb->dma1_desc0->cmd_num = 0;
	cfb->dma1_desc0->page_width = 0;
	cfb->dma1_desc0->offsize = 0;

	/* DMA1 Descriptor1 */
	if ( lcd_info->panel.type == DISP_PANEL_TYPE_TVE) { /* TVE mode */
		cfb->dma1_desc1->next_desc = (unsigned int)virt_to_phys(cfb->dma1_desc0);
	} else {
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

	dma_cache_wback_inv((unsigned int)(cfb->dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz_lcd_dma_desc));
}

static void jzfb_set_panel_mode(struct lcd_cfb_info *cfb)
{
	struct jzlcd_info * lcd_info  = cfb->jz_lcd_info;
	struct jzlcd_panel_t *panel = &lcd_info->panel;
	int n = cfb->id;

	lcd_info->panel.cfg |= LCD_CFG_NEWDES; /* use 8words descriptor always */

	printk("lcd:%d, Line:%d, func:%s\n",n,__LINE__,__func__);
	REG_LCD_CTRL(n) = lcd_info->panel.ctrl; /* LCDC Controll Register */
	REG_LCD_CFG(n) = lcd_info->panel.cfg; /* LCDC Configure Register */

	if ( lcd_info->panel.type == DISP_PANEL_TYPE_SLCD ) /* enable Smart LCD DMA */
		REG_SLCD_CFG = lcd_info->panel.slcd_cfg; 	/* Smart LCD Configure Register */

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

static void jzfb_set_osd_mode(struct lcd_cfb_info *cfb)
{
	struct jzlcd_info * lcd_info = cfb->jz_lcd_info;
	int n = cfb->id;

	REG_LCD_OSDC(n) 	= lcd_info->osd.osd_cfg | (1 << 16); 
	REG_LCD_RGBC(n)  	= lcd_info->osd.rgb_ctrl;
	REG_LCD_BGC0(n)  	= lcd_info->osd.bgcolor0;
	REG_LCD_BGC1(n)  	= lcd_info->osd.bgcolor1;
	REG_LCD_KEY0(n) 	= lcd_info->osd.colorkey0;
	REG_LCD_KEY1(n) 	= lcd_info->osd.colorkey1;
	REG_LCD_IPUR(n) 	= lcd_info->osd.ipu_restart;
}

static void set_bpp(int *bpp, struct jz_lcd_dma_desc *dma_desc){

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

static void set_alpha_things( struct jzlcd_info * info, struct jz_lcd_dma_desc *new_desc, int is_fg1)
{
	if(is_fg1){
		/* change alpha value*/
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
		//if((info->osd.osd_cfg & LCD_OSDC_COEF_SLE0_MASK) != (jz_lcd_info->osd.osd_cfg & LCD_OSDC_COEF_SLE0_MASK))
		new_desc->cmd_num &= ~(LCD_OSDC_COEF_SLE0_MASK);
		new_desc->cmd_num |= ((info->osd.osd_cfg & LCD_OSDC_COEF_SLE0_MASK) >> LCD_OSDC_COEF_SLE0_BIT) << LCD_CPOS_COEF_SLE_BIT;
	}
}

static void adj_xy_wh(struct jzlcd_fg_t *fg, struct lcd_cfb_info *cfb)
{

	if ( fg->x >= cfb->jz_lcd_info->panel.w )
		fg->x = cfb->jz_lcd_info->panel.w;
	if ( fg->y >= cfb->jz_lcd_info->panel.h )
		fg->y = cfb->jz_lcd_info->panel.h;
	if ( (fg->x + fg->w) > cfb->jz_lcd_info->panel.w )
		fg->w = cfb->jz_lcd_info->panel.w - fg->x;
	if ( (fg->y + fg->h) > cfb->jz_lcd_info->panel.h )
		fg->h = cfb->jz_lcd_info->panel.h - fg->y;
}

static void calc_line_frm_size(int bpp, struct jzlcd_fg_t *fg, unsigned int *line_size, unsigned int *frm_size)
{
	*line_size = (fg->w * bpp + 7) / 8;
	*line_size = ((*line_size + 3) >> 2) << 2; /* word aligned */
	*frm_size = *line_size * fg->h; /* bytes */
}


void jzfb_reset_descriptor( struct lcd_cfb_info *cfb)
{
	int fg_line_size, fg_frm_size, bpp;
	unsigned long irq_flags;
	struct jz_lcd_dma_desc *new_desc, *cur_desc;
	struct jzlcd_fg_t *fg_info;
	struct jzlcd_info *info = cfb->jz_lcd_info;


	if(!cfb->is_fg1) {
		fg_info = &info->osd.fg0;

		if(REG_LCD_FID0(cfb->id) == cfb->dma0_desc0->frame_id) {
			cur_desc = cfb->dma0_desc0;
			new_desc = cfb->dma0_desc1;
		}else {
			cur_desc = cfb->dma0_desc1;
			new_desc = cfb->dma0_desc0;
		}
	}else {
		fg_info = &info->osd.fg1;	
		if(REG_LCD_FID1(cfb->id) == cfb->dma1_desc0->frame_id){
			cur_desc = cfb->dma1_desc0;
			new_desc = cfb->dma1_desc1;
		}else {
			cur_desc = cfb->dma1_desc1;
			new_desc = cfb->dma1_desc0;
		}
	}
	
	adj_xy_wh(fg_info, cfb);
	
	new_desc->next_desc = (unsigned int)virt_to_phys(new_desc);
	/* change xy position */
	new_desc->cmd_num = (fg_info->x << LCD_CPOS_XPOS_BIT | fg_info->y << LCD_CPOS_YPOS_BIT) ;

	/* change bpp */
	set_bpp(&fg_info->bpp, new_desc);
	
	/* change  display size */
	bpp = bpp_to_data_bpp(fg_info->bpp);
	calc_line_frm_size(bpp, fg_info, &fg_line_size, &fg_frm_size);
	new_desc->desc_size = ((fg_info->w - 1) << LCD_DESSIZE_WIDTH_BIT | (fg_info->h - 1) << LCD_DESSIZE_HEIGHT_BIT);

	if(!cfb->is_fg1){
		if(fg0_should_use_x2d_block_mode(cfb)){
			printk("%s,use x2d\n",__func__);

			new_desc->cmd = LCD_CMD_16X16_BLOCK | fg_info->h | LCD_CMD_SOFINT | LCD_CMD_EOFINT;
			new_desc->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame0);
			new_desc->offsize = (fg_info->w / 16) * (((16 * bpp) / 32) * 16); /* in words */
			new_desc->page_width = fg_line_size >> 2; /* in words */
		}
		else{
			printk("%s , %d,use lcd\n",__func__,__LINE__);
			new_desc->cmd = fg_frm_size >> 2 | LCD_CMD_SOFINT | LCD_CMD_EOFINT;
			new_desc->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame0);
			new_desc->offsize = 0;
			
			if (cfb->jz_lcd_info->panel.type  == DISP_PANEL_TYPE_SLCD) {
				printk("use slcd\n");
				new_desc->next_desc = (unsigned int)virt_to_phys(cfb->dma0_desc_cmd);
				cfb->dma0_desc_cmd->next_desc = (unsigned int)virt_to_phys(new_desc);
				cfb->dma0_desc_cmd0->next_desc = (unsigned int)virt_to_phys(new_desc);
			}
		}
	}else{
		if(fg1_should_use_x2d_block_mode(cfb)){
			printk("%s,use x2d\n",__func__);
	
			new_desc->cmd = LCD_CMD_16X16_BLOCK | fg_info->h;
			new_desc->cmd |= LCD_CMD_SOFINT | LCD_CMD_EOFINT;
			new_desc->databuf = (unsigned int)virt_to_phys((void *)cfb->lcd_frame1);
			new_desc->offsize = (fg_info->w / 16) * (((16 * bpp) / 32) * 16); /* in words */
			new_desc->page_width = fg_line_size >> 2; /* in words */
		}
		else{
			printk("%s , %d,use normal lcd\n",__func__,__LINE__);
			new_desc->cmd = fg_frm_size >> 2 | LCD_CMD_SOFINT | LCD_CMD_EOFINT;
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

	dma_cache_wback((unsigned int)(new_desc), sizeof(struct jz_lcd_dma_desc));

	if(!cfb->is_fg1){
		if (fg_info->bpp <= 8){ 
			printk("JZ do not support palette!!!!!\n");
		}
	}
	
	dump_dma_desc(new_desc);
	
	cur_desc->next_desc = (unsigned int)virt_to_phys(new_desc);
	dma_cache_wback((unsigned int)(cfb->dma_desc_base), (DMA_DESC_NUM)*sizeof(struct jz_lcd_dma_desc));

	//if it is set first
	if(!cfb->is_fg1 && (REG_LCD_FID0(cfb->id) == 0)){
		if (cfb->jz_lcd_info->panel.type == DISP_PANEL_TYPE_SLCD) {/* for smartlcd */		
			REG_LCD_DA0(cfb->id) = virt_to_phys(cfb->dma0_desc_cmd0);
		}
		else {
			REG_LCD_DA0(cfb->id) = (unsigned int)virt_to_phys(new_desc);
		}
	}else if(cfb->is_fg1 && (REG_LCD_FID1(cfb->id) == 0))
		REG_LCD_DA1(cfb->id) = (unsigned int)virt_to_phys(new_desc);

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

}

static void jzfb_change_clock(struct lcd_cfb_info *cfb)
{
	struct jzlcd_info * lcd_info = cfb->jz_lcd_info;

#if defined(CONFIG_FPGA)
	REG_LCD_REV(cfb->id) = 0x1;
	printk("FPGA test, pixclk %d\n", JZ_EXTAL / (((REG_LCD_REV(cfb->id) & 0xFF) + 1) * 2));
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
		pclk = val * (lcd_info->panel.w * 3 + lcd_info->panel.hsw + lcd_info->panel.elw + lcd_info->panel.blw) * 
			     (lcd_info->panel.h + lcd_info->panel.vsw + lcd_info->panel.efw + lcd_info->panel.bfw); 
	}

	cpm_set_clock(CGU_LPCLK0, pclk);	
	__cpm_start_lcd0();
		
	jz_clocks.pixclk0 = cpm_get_clock(CGU_LPCLK0);
	printk("LCDC0: PixClock:%d\n", jz_clocks.pixclk0);
	
#endif
}

/*
 * jzfb_deep_set_mode,
 *
 */
static void jzfb_deep_set_mode(struct lcd_cfb_info *cfb)
{
	jzfb_descriptor_init(cfb);
	cfb->is_fg1 = 0;
	jzfb_reset_descriptor(cfb);
	cfb->is_fg1 = 1;
	jzfb_reset_descriptor(cfb);
#if !defined(CONFIG_JZ_LCD_BL35026V0)
	jzfb_set_panel_mode(cfb);
	jzfb_set_osd_mode(cfb);
#endif
	jzfb_set_var(&cfb->fb.var, -1, &cfb->fb);
}


static irqreturn_t jzfb_interrupt_handler(int irq, void *dev_id)
{
	unsigned int state;
	static int irqcnt = 0;
	int id = 0;
	struct lcd_cfb_info *cfb;
	struct platform_device *dev = dev_id;
	cfb = dev->dev.driver_data;
	
	id = cfb->id;
	printk("id:%d\n",id);

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
static int jzfb_suspend(struct platform_device *pdev, pm_message_t state)
{
	struct lcd_cfb_info *cfb = pdev->dev.driver_data;

	printk("%s(): called.\n", __func__);

	screen_off(cfb);
	__lcd_clr_ena(cfb->id);
	display_off(cfb);

	return 0;
}

/*
 * Resume the LCDC.
 */
static int jzfb_resume(struct platform_device *pdev)
{
	struct lcd_cfb_info *cfb = pdev->dev.driver_data;

	printk("%s(): called.\n", __func__);
	display_on(cfb);	
	screen_on(cfb);
	__lcd_set_ena(cfb->id);
	
	return 0;
}

#else
#define jzfb_suspend      NULL
#define jzfb_resume       NULL
#endif /* CONFIG_PM */

/* The following routine is only for test */
#ifdef JZ_FB_DEBUG

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

			for (i = wpl*4;i < wpl*40;i++) {
				*ptr++ = 0xff0000ff;
				*ptr++ = 0x00ff0000;
				*ptr++ = 0x0000ff00;
			}
			break;

		}
	}
}
#endif


static int screen_off(void *ptr)
{
	struct lcd_cfb_info *cfb = ptr;
	
	if (cfb->b_lcd_display) {
		lcd_close_backlight(cfb);
		cfb->b_lcd_display = 0;
	}
	
	return 0;
}

static int screen_on(void *ptr)
{
	struct lcd_cfb_info *cfb = ptr;
	
	if (!cfb->b_lcd_display) {
		lcd_open_backlight(cfb);
		cfb->b_lcd_display = 1;
	}
	
	return 0;
}

static int jzfb_set_backlight_level(struct lcd_cfb_info *cfb_info, int n)
{
	struct lcd_cfb_info *cfb = cfb_info; 

	if (n) {
		if (n > LCD_MAX_BACKLIGHT)
			n = LCD_MAX_BACKLIGHT;

		if (n < LCD_MIN_BACKLIGHT)
			n = LCD_MIN_BACKLIGHT;
			
		if (!cfb->b_lcd_display) {
			lcd_open_backlight(cfb);
			cfb->b_lcd_display = 1;
		}
		
		cfb->backlight_level = n;
		lcd_set_backlight_level(cfb);
	} else{
		/* Turn off LCD backlight. */
		cfb->backlight_level = 0;
		lcd_close_backlight(cfb);
		cfb->b_lcd_display = 0;
	}
	
	return 0;
}

static ssize_t show_bl_level(struct device *device,
			     struct device_attribute *attr, char *buf)
{
	struct lcd_cfb_info *cfb = device->driver_data;
	
	return snprintf(buf, PAGE_SIZE, "%d\n", cfb->backlight_level);
}

static ssize_t store_bl_level(struct device *device,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int n;
	char *ep;
	struct lcd_cfb_info *cfb = device->driver_data;
	
	n = simple_strtoul(buf, &ep, 0);
	if (*ep && *ep != '\n')
		return -EINVAL;
	printk("level: %d\n", n);
	jzfb_set_backlight_level(cfb, n);

	return count;
}

static struct device_attribute device_attrs[] = {
	__ATTR(backlight_level, S_IRUGO | S_IWUSR, show_bl_level, store_bl_level),
};

static int jzfb_device_attr_register(struct fb_info *fb_info)
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

static int jzfb_device_attr_unregister(struct fb_info *fb_info)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(device_attrs); i++)
		device_remove_file(fb_info->dev, &device_attrs[i]);

	return 0;
}
/* End */

static void gpio_init(struct lcd_cfb_info *cfb)
{
	/* gpio init __gpio_as_lcd */
	if (cfb->jz_lcd_info->panel.cfg & LCD_CFG_MODE_TFT_16BIT)
		__gpio_as_lcd_16bit();
	else if (cfb->jz_lcd_info->panel.cfg & LCD_CFG_MODE_TFT_24BIT)
		__gpio_as_lcd_24bit();
	else
 		__gpio_as_lcd_18bit();

	/* In special mode, we only need init special pin,
	 * as general lcd pin has init in uboot */
	switch (cfb->jz_lcd_info->panel.cfg & LCD_CFG_MODE_MASK) {
		case LCD_CFG_MODE_SPECIAL_TFT_1:
		case LCD_CFG_MODE_SPECIAL_TFT_2:
		case LCD_CFG_MODE_SPECIAL_TFT_3:
			__gpio_as_lcd_special();
			break;
		default:
			break;
	}
}

static int __devinit jzfb_probe(struct platform_device *dev)
{
	struct lcd_cfb_info *cfb = NULL;
	int irq;
	int rv = 0;
	
	cfb = kmalloc(sizeof(struct lcd_cfb_info) + sizeof(u32) * 16, GFP_KERNEL);
	if (!cfb)
		return -1;

	memset(cfb, 0, sizeof(struct lcd_cfb_info) );

	cfb->id = dev->id;
	
	cfb->jz_lcd_info = &jz_lcd_panel; 
	jzfb_info = cfb;
	irq = IRQ_LCD0;

	jzfb_set_fb_info(cfb);

//	screen_off(cfb->id);
//	ctrl_disable(cfb);
	
	/* init clk */
	jzfb_change_clock(cfb);

	rv = jzfb_map_smem(cfb);
	if (rv)
		goto failed;

	spin_lock_init(&cfb->update_lock);
	init_waitqueue_head(&cfb->frame_wq);
	cfb->frame_requested = cfb->frame_done = 0;

	jzfb_deep_set_mode(cfb);

	rv = register_framebuffer(&cfb->fb);
	if (rv < 0) {
		printk("Failed to register framebuffer device.");
		goto failed;
	}

	//we use it in the future
	dev->dev.driver_data = cfb;
	cfb->fb.dev->driver_data = cfb;

	printk("fb%d: %s frame buffer device, using %dK of video memory\n",
	       cfb->fb.node, cfb->fb.fix.id, cfb->fb.fix.smem_len >> 10);

	jzfb_device_attr_register(&cfb->fb);

	if (request_irq(irq, jzfb_interrupt_handler, IRQF_DISABLED, "lcd", dev)) {
		printk("Faield to request LCD IRQ.\n");
		rv = -EBUSY;
		goto failed;
	}
	
	display_on(cfb);
	gpio_init(cfb);

#if defined(CONFIG_JZ_LCD_BL35026V0)
	DisplayLogo(0, 0, 320, 240, yjt);
#else
	DisplayLogo(0, 0, 240, 320, mylogobg);
#endif

	ctrl_enable(cfb);
	screen_on(cfb);

	MoveLogo_timer_init();
#if defined(CONFIG_JZ4770_SLCD_FORISE_BL28021)		// yjt, 20130821
	ResetSlcdStartAddr_timer_init();
#endif
	
#ifdef JZ_FB_DEBUG
	display_h_color_bar(cfb->jz_lcd_info->osd.fg0.w, cfb->jz_lcd_info->osd.fg0.h, cfb->jz_lcd_info->osd.fg0.bpp, (int *)cfb->lcd_frame0);
	print_lcdc_registers(cfb);
#endif

	return 0;
failed:
	jzfb_unmap_smem(cfb);
	jzfb_free_fb_info(cfb);

	return rv;
}

static int __devexit jzfb_remove(struct platform_device *pdev)
{
	struct lcd_cfb_info *cfb;

	cfb = pdev->dev.driver_data;
		
	jzfb_device_attr_unregister(&cfb->fb);
	
	display_off(cfb);
	
	jzfb_unmap_smem(cfb);
	jzfb_free_fb_info(cfb);
	jzfb_info = NULL;
	
	return 0;
}

static struct platform_driver jz_fb_driver = {
	.probe	= jzfb_probe,
	.remove = jzfb_remove,
	.suspend = jzfb_suspend,
	.resume = jzfb_resume,
	.driver = {
		.name = "jz-lcd",
		.owner = THIS_MODULE,
	},
};

static int __init jzfb_init(void)
{
	return platform_driver_register(&jz_fb_driver);
}

static void __exit jzfb_cleanup(void)
{
	platform_driver_unregister(&jz_fb_driver);
}

module_init(jzfb_init);
module_exit(jzfb_cleanup);
