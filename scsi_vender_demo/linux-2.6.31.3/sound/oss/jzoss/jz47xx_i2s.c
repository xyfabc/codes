/*
 * Linux/sound/oss/jz47xx_i2s.c
 *
 * I2S controller for Ingenic Jz47xx MIPS processor
 *
 * 2011-11-xx	liulu <lliu@ingenic.cn>
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */
#include <linux/module.h>
#include <linux/soundcard.h>
#include <asm/jzsoc.h>
#include <linux/fs.h>
#include "jz_snd.h"

#define I2S_RFIFO_DEPTH 32
#define I2S_TFIFO_DEPTH 64

#ifdef CONFIG_I2S_DLV_4770
#define  CGM_AIC0	CGM_AIC
#else
#define  CGM_AIC0	CGM_AIC0
#endif
//FIXME: use external codec????
static int is_recording = 0;
static int is_playing = 0;
/* static int g_record_data_width = 16; */
/* static int g_record_channels = 2; */

static int jz47xx_i2s_debug = 0;
module_param(jz47xx_i2s_debug, int, 0644);
#define JZ47XX_I2S_DEBUG_MSG(msg...)			\
	do {					\
		if (jz47xx_i2s_debug)		\
		printk("I2S: " msg);	\
	} while(0)


void jz47xx_i2s_dump_regs(const char *str)
{
	char *regname[] = {"aicfr","aiccr","aiccr1","aiccr2","i2scr","aicsr","acsr","i2ssr",
		"accar", "accdr", "acsar", "acsdr", "i2sdiv", "aicdr"};
	int i;
	unsigned int addr;

	printk("AIC regs dump, %s\n", str);
	for (i = 0; i < 13*4; i += 4) {
		addr = 0xb0020000 + i;
		printk("%s\t0x%08x -> 0x%08x\n", regname[i/4], addr, *(unsigned int *)addr);
	}
}

void jz47xx_i2s_tx_ctrl(int on)
{
	JZ47XX_I2S_DEBUG_MSG("enter %s, on = %d\n", __func__, on);
	if (on) {
		is_playing = 1;
		/* enable replay */
		__i2s_enable_transmit_dma();
		__i2s_enable_replay();
		__i2s_enable();
	} else if (is_playing) {
		is_playing = 0;
		/* disable replay & capture */
		while(!__aic_transmit_underrun());
		__i2s_disable_replay();
		__i2s_disable_transmit_dma();

		if (!is_recording)
			__i2s_disable();
	}
}

void jz47xx_i2s_rx_ctrl(int on)
{
	JZ47XX_I2S_DEBUG_MSG("enter %s, on = %d\n", __func__, on);
	if (on) {
		is_recording = 1;
		/* enable capture */
		__i2s_enable_receive_dma();
		__i2s_enable_record();
		__i2s_enable();
	} else {
		is_recording = 0;
		/* disable replay & capture */
		__i2s_disable_record();
		__i2s_disable_receive_dma();

		if (!is_playing)
			__i2s_disable();
	}
}

int jz47xx_i2s_set_channels(int mode, int channels)
{
	JZ47XX_I2S_DEBUG_MSG("enter %s, mode = %d, channels = %d\n", __func__, mode, channels);
	if (mode & MODE_REPLAY) {
		if (channels == 1) {
			__aic_enable_mono2stereo();
			__aic_out_channel_select(0);
		} else {
			__aic_disable_mono2stereo();
			__aic_out_channel_select(1);
		}
	}
	if (mode & MODE_RECORD) {
		/* g_record_channels = channels; */
		/* g_record_channels = 2; */
		/* __i2s_set_receive_trigger((g_record_data_width / 8 * g_record_channels) * 2 / 2 - 1); */
	}

	return 0;
}

#define I2S_FIFO_DEPTH 32
int jz47xx_i2s_set_width(int mode, int width){
	int onetrans_bit = 16*8;

	JZ47XX_I2S_DEBUG_MSG("enter %s, mode = %d, width = %d\n", __func__, mode, width);
	switch(width){
		case 8:
			onetrans_bit = 16 * 8;
			break;
		case 16:
			onetrans_bit = 16 * 8;
			break;
		case 17 ... 32:
			onetrans_bit = 32 * 8;
			break;
		default:
			printk("%s: Unkown mode(sound data width) %d\n", __FUNCTION__, width);
			break;
	}
	if (mode & MODE_REPLAY) {
		__i2s_set_oss_sample_size(width);
		if ((I2S_FIFO_DEPTH - onetrans_bit / width) >= 30) {
			__i2s_set_transmit_trigger(14);
		} else {
			__i2s_set_transmit_trigger((I2S_FIFO_DEPTH - onetrans_bit / width) / 2);
		}
	}
	if (mode & MODE_RECORD){
		__i2s_set_iss_sample_size(width);
		/* g_record_data_width = width; */
		/* __i2s_set_receive_trigger((g_record_data_width / 8 * g_record_channels) * 2 / 2 - 1); */
		__i2s_set_receive_trigger((onetrans_bit / width) / 2);
	}

	return 0;
}
static void __set_aic_mode(void)
{
	__gpio_as_func2(32 * 4 + 5);//SYS_CLK
	__gpio_as_func0(32 * 3 + 13);//LR (SYNC)
	__gpio_as_func1(32 * 3 + 12); //BITCLK 
	__gpio_as_func1(32 * 4 + 9); //LR (ISYNC)
	__gpio_as_func1(32 * 4 + 8); //IBITCLK
	__gpio_as_func0(32 * 4 + 6); //SDIN
	__gpio_as_func0(32 * 4 + 7); //SDOUT

}
void set_extern_mode(void)
{
	__aic_reset();
	mdelay(10);
	__set_aic_mode();
	__aic_external_codec();  
	__aic_select_i2s();     
	/* I2S mode */
	__i2s_select_i2s();
	REG_AIC_I2SDIV |= 0x303;

	/* set AIC master mode */
	__i2s_stop_bitclk();
	__i2s_stop_ibitclk();
	__i2s_as_master();           

	REG_AIC_FR |= (1 << 8) | (1 << 9) | (1 << 10);
	__i2s_start_bitclk();
	__i2s_start_ibitclk();
}
//#ifdef CONFIG_I2S_DLV_4780
#if defined (CONFIG_I2S_DLV_4780) || defined (CONFIG_I2S_DLV_4775)
/*
 * set I2S clock
 * for clk_sour  1: Apll, 0 : Epll
 */
static void set_i2s_clock(int clk_sour, unsigned int clk_hz)
{
	unsigned int extclk = __cpm_get_extalclk();
	unsigned int pllclk;
	unsigned int div;

	JZ47XX_I2S_DEBUG_MSG("enter %s\n", __func__);

	if(clk_hz == extclk){
		__cpm_select_i2sclk_exclk();
		REG_CPM_I2SCDR |= I2SCDR_CE_I2S;
		while(REG_CPM_I2SCDR & (I2SCDR_I2S_BUSY));
	}else
	{

		if(clk_sour){
			pllclk = cpm_get_xpllout(SCLK_APLL);
			REG_CPM_I2SCDR &= ~(I2SCDR_I2PCS);
			printk("i2S from APLL and clock is %d\n", pllclk);
		}else{
			pllclk = cpm_get_xpllout(SCLK_MPLL);
			REG_CPM_I2SCDR |= I2SCDR_I2PCS;
			printk("i2S from EPLL and clock is %d\n", pllclk);
		}
		if((pllclk % clk_hz) * 2 >= clk_hz)
			div = (pllclk / clk_hz) + 1;
		else
			div = pllclk / clk_hz;
		OUTREG32(CPM_I2SCDR, I2SCDR_I2CS | I2SCDR_CE_I2S | (div - 1));
		while (INREG32(CPM_I2SCDR) & I2SCDR_I2S_BUSY){
			printk("Wait : I2SCDR stable\n");
			udelay(100);
		}
	}
}
#endif


int __init jz47xx_i2s_init(void)
{
	JZ47XX_I2S_DEBUG_MSG("enter %s\n", __func__);

	cpm_start_clock(CGM_AIC0);
	/* Select exclk as i2s clock */
	REG_AIC_I2SCR |= AIC_I2SCR_ESCLK;
#ifdef CONFIG_I2S_DLV_4770
	cpm_set_clock(CGU_I2SCLK, JZ_EXTAL);
#else
	set_i2s_clock(1, 12000000);
#endif

	printk("start set AIC register....\n");

	__i2s_disable();
	__aic_disable_transmit_dma();
	__aic_disable_receive_dma();
	__i2s_disable_record();
	__i2s_disable_replay();
	__i2s_disable_loopback();

	__i2s_internal_codec();
	__i2s_as_slave();
	__i2s_select_i2s();
	__aic_select_i2s();
	__aic_play_lastsample();
	__i2s_start_bitclk();
	__i2s_set_transmit_trigger(I2S_TFIFO_DEPTH / 4);
	__i2s_set_receive_trigger(I2S_RFIFO_DEPTH / 4);
	//__i2s_send_rfirst();

	__aic_write_tfifo(0x0);
	__aic_write_tfifo(0x0);
	__i2s_enable_replay();
	__i2s_enable();
	mdelay(1);

	__i2s_disable_replay();
	__i2s_disable();

	jz47xx_i2s_tx_ctrl(0);
	jz47xx_i2s_rx_ctrl(0);

	return 0;
}

void __exit jz47xx_i2s_deinit(void)
{
	JZ47XX_I2S_DEBUG_MSG("enter %s\n", __func__);
	//do nothing
	return;
}
