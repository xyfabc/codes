/*
 * Linux/sound/oss/jzoss/jz4770/dlv.c
 *
 * DLV CODEC driver for Ingenic Jz4770 MIPS processor
 *
 * 2011-11-xx	liulu <lliu@ingenic.cn>
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/semaphore.h>
#include <linux/vmalloc.h>
#include <asm/hardirq.h>
#include <asm/jzsoc.h>
#include <linux/fs.h>

#include "../../jz_snd.h"
#include "dlv.h"

#define TURN_ON_SB_HP_WHEN_INIT  1
#define __gpio_as_lineout()  __gpio_as_output1(32 * 5 + 8)

static struct semaphore *g_dlv_sem = 0;
static struct work_struct dlv_irq_work;
static spinlock_t dlv_irq_lock;
static int g_replay_volume = 1;
static int g_is_replaying = 0;
static int g_is_recording = 0;

//**************************************************************************
// debug
//**************************************************************************

static int dlv_dump_regs(const char *str);

static int jz4770_dlv_debug = 0;
module_param(jz4770_dlv_debug, int, 0644);
#define JZ4770_DLV_DEBUG_MSG(msg...)			\
	do {					\
		if (jz4770_dlv_debug)		\
		printk("jz4770_dlv: " msg);	\
	} while(0)

//**************************************************************************
// CODEC registers access routines 
//**************************************************************************

/**
 * CODEC read register
 *
 * addr:	address of register
 * return:	value of register
 */
static inline int codec_read_reg(int addr)
{
	volatile int reg;

	while (__icdc_rgwr_ready()) {
		;//nothing...
	}
	__icdc_set_addr(addr);
	reg = __icdc_get_value();
	reg = __icdc_get_value();
	reg = __icdc_get_value();
	reg = __icdc_get_value();
	reg = __icdc_get_value();
	return __icdc_get_value();
}

/**
 * CODEC write register
 *
 * addr:	address of register
 * val:		value to set
 */
void codec_write_reg(int addr, int val)
{
	volatile int reg;

	while (__icdc_rgwr_ready()) {
		printk("1. __icdc_rgwr_ready()=%d\n" ,__icdc_rgwr_ready());
		;//nothing...
	}
	REG_ICDC_RGADW = ICDC_RGADW_RGWR | ((addr << ICDC_RGADW_RGADDR_LSB) | val);
	//__icdc_set_rgwr();
	reg = __icdc_rgwr_ready();
	reg = __icdc_rgwr_ready();
	reg = __icdc_rgwr_ready();
	reg = __icdc_rgwr_ready();
	reg = __icdc_rgwr_ready();
	reg = __icdc_rgwr_ready();
	while (__icdc_rgwr_ready()) {
		printk("2. __icdc_rgwr_ready()=%d\n" ,__icdc_rgwr_ready());
		;//nothing...
	}
}

/**
 * CODEC write a bit of a register
 *
 * addr:	address of register
 * bitval:	bit value to modifiy
 * mask_bit:	indicate which bit will be modifiy (1 << nbit)
 */
static int codec_write_reg_bit(int addr, int bitval, int bit_mask)
{
	int val = codec_read_reg(addr);

	if (bitval)
		val |= bit_mask;
	else
		val &= ~bit_mask;
	codec_write_reg(addr, val);

	return 1;
}

/**
 * CODEC update some bits on register
 *
 * addr:	address of register
 * val:		these bits value to modifiy (0x3 << nbit)
 * mask:	indicate which bits will be modifiy (0xf << nbit)
 */
static int codec_update_reg(int addr,  int val, int mask) {

	int old_val = codec_read_reg(addr);
	old_val &= ~mask;
	old_val |= (val & mask);
	codec_write_reg(addr, old_val);

	return 1;
}


/**
 * CODEC  write mixer function register
 *
 * addr:	address of register
 * val:		value to set
 */
static void codec_write_mixer_reg(u8 addr, u8 val) {
	__dlv_clear_mixer_load();
	// first wirte data
	__dlv_set_mixer_data(val);
	//second set addr 
	__dlv_set_mixer_addr(addr);
	//th3, set load bit 
	__dlv_set_mixer_load();
	mdelay(1);
	__dlv_clear_mixer_load();
}
/**
 * CODEC read mixer function register
 *
 * addr:	address of register
 */
static int codec_read_mixer_reg(u8 addr)
{
	u8 regval;
	__dlv_set_mixer_addr(addr);
	__dlv_clear_mixer_load();
	mdelay(1);

	regval = codec_read_reg(DLC_DR_MIX_DATA);
	return regval;
}
/*
 * CODEC write  mixer function reigster bit
 * 
 * addr:	addr of mixer register
 * val:		value of mixer reigsiter
 * bit_mask:	indicate which bit will be modifiy (1 << nbit)
 */
static void codec_write_mixer_reg_bit(int addr, int bitval, int bit_mask)
{
	int val = codec_read_mixer_reg(addr);

	if (bitval)
		val |= bit_mask;
	else
		val &= ~bit_mask;
	codec_write_mixer_reg(addr, val);
}

/**
 * CODEC write agc  function register
 *
 * addr:	address of register
 * val:		value to set
 */
static void codec_write_agc_reg(int addr, int val)
{
	__dlv_clear_agc_load();
	//set data
	__dlv_set_agc_data(val);
	//set addr
	__dlv_set_agc_addr(addr);
	//set load
	__dlv_set_agc_load();
	mdelay(1);
	//clear load
	__dlv_clear_agc_load();

}
/**
 * CODEC read agc  function register
 *
 * addr:	address of register
 */
static int  codec_read_agc_reg(int addr)
{
	u8 regval;

	__dlv_set_agc_addr(addr);
	__dlv_clear_agc_load();
	mdelay(1);
	regval = codec_read_reg(DLC_DR_ADC_AGC);

	return regval;
}
/*
 * CODEC write  agc function reigster bit
 * 
 * addr:	addr of mixer register
 * val:		value of mixer reigsiter
 * bit_mask:	indicate which bit will be modifiy (1 << nbit)
 */
static void codec_write_agc_reg_bit(int addr, int bitval, int bit_mask)
{
	int val = codec_read_agc_reg(addr);

	if (bitval)
		val |= bit_mask;
	else
		val &= ~bit_mask;
	codec_write_agc_reg(addr, val);
}

//wait bit set to 1
static inline void codec_sleep_wait_bitset(int reg, unsigned bit_mask, int stime, int line)
{
	while(!(codec_read_reg(reg) & bit_mask)) {
		msleep(stime);
	}
}
//wait bit set to 0
static inline void codec_sleep_wait_bitclear(int reg, unsigned bit_mask, int stime)
{
	while((codec_read_reg(reg) & bit_mask))
		msleep(stime);
}

//**************************************************************************
//   DLV CODEC operations routines
//**************************************************************************
static inline void turn_on_dac(int timeout)
{
	if(__dlv_get_dac_mute()){

		//clear DAC MUTE Flag
		__dlv_set_irq_flag(DLC_IFR_DAC_MUTE);
		udelay(10);
		//disable DAC mute
		__dlv_disable_dac_mute();
		//wait DAC_MUTE_EVENT set to 1
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MUTE, timeout,__LINE__);
		//clear DAC mute flag
		__dlv_set_irq_flag(DLC_IFR_DAC_MUTE);
		udelay(10);

		//wait DAC_MUTE_EVENT set to 1
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MUTE, timeout,__LINE__);
		//clear DAC mute flag
		__dlv_set_irq_flag(DLC_IFR_DAC_MUTE);


	}
}

static inline void turn_off_dac(int timeout)
{
	if (!(__dlv_get_dac_mute())){

		//clear DAC MUTE Flag
		__dlv_set_irq_flag(DLC_IFR_DAC_MUTE);
		udelay(10);
		//enable dac mute
		__dlv_enable_dac_mute();
		//wait DAC_MUTE_EVENT set to 1
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MUTE, timeout,__LINE__);
		//clear DAC MUTE Flag
		__dlv_set_irq_flag(DLC_IFR_DAC_MUTE);
		udelay(10);

		//wait DAC_MUTE_EVENT set to 1
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MUTE, timeout,__LINE__);
		//clear DAC MUTE Flag
		__dlv_set_irq_flag(DLC_IFR_DAC_MUTE);
	}
}
static inline void turn_on_adc(int timeout)
{

	if(__dlv_get_adc_mute()){

		//clear ADC mute flag
		__dlv_set_irq_flag(DLC_IFR_ADC_MUTE);
		udelay(10);
		//disable adc mute
		__dlv_disable_adc_mute();
		//wait adc mute flag 
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_ADC_MUTE, timeout, __LINE__);
		//clear ADC mute flag
		__dlv_set_irq_flag(DLC_IFR_ADC_MUTE);
		udelay(10);

		//wait adc mute flag 
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_ADC_MUTE, timeout, __LINE__);
		//clear ADC mute flag
		__dlv_set_irq_flag(DLC_IFR_ADC_MUTE);
	}
}
static inline void turn_off_adc(int timeout)
{

	if(!__dlv_get_adc_mute()){

		//clear ADC mute flag
		__dlv_set_irq_flag(DLC_IFR_ADC_MUTE);
		udelay(10);
		//enable adc mute
		__dlv_enable_adc_mute();
		//wait adc mute flag 
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_ADC_MUTE, timeout, __LINE__);
		//clear ADC mute flag
		__dlv_set_irq_flag(DLC_IFR_ADC_MUTE);
		udelay(10);

		//wait adc mute flag 
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_ADC_MUTE, timeout, __LINE__);
		//clear ADC mute flag
		__dlv_set_irq_flag(DLC_IFR_ADC_MUTE);
	}
}

static inline void turn_on_sb_hp(void)
{
	if (__dlv_get_sb_hp()){

		//clear dac mode flags
		__dlv_set_irq_flag(DLC_IFR_DAC_MODE);
		udelay(10);
		//SB HP
		__dlv_switch_sb_hp(POWER_ON); //SB_HP->0 POP
		//wait dac mode flags
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MODE, 100, __LINE__);
		//clear dac mode flags
		__dlv_set_irq_flag(DLC_IFR_DAC_MODE);
		udelay(10);

		//wait dac mode flags
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MODE, 100, __LINE__);
		//clear dac mode flags
		__dlv_set_irq_flag(DLC_IFR_DAC_MODE);
	}
}

static inline void turn_off_sb_hp(void)
{
	if (!__dlv_get_sb_hp()){

		//clear dac mode flags
		__dlv_set_irq_flag(DLC_IFR_DAC_MODE);
		udelay(100);
		//SB HP
		__dlv_switch_sb_hp(POWER_OFF); //SB_HP->0 POP
		//wait dac mode flags
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MODE, 100, __LINE__);
		//clear dac mode flags
		__dlv_set_irq_flag(DLC_IFR_DAC_MODE);
		udelay(10);

		//wait dac mode flags
		codec_sleep_wait_bitset(DLC_IFR, DLC_IFR_DAC_MODE, 100, __LINE__);
		//clear dac mode flags
		__dlv_set_irq_flag(DLC_IFR_DAC_MODE);
	}
}

//************************************************************************
//  dlv control
//************************************************************************

static void dlv_shutdown(void) {

	turn_off_dac(5);
	__aic_write_tfifo(0x0);
	__aic_write_tfifo(0x0);
	__i2s_enable_replay();
	msleep(1);
	//	__i2s_disable_replay();

	__dlv_set_hp_volume(0x80);// set Hp gain to +6dB
	turn_off_sb_hp();
	mdelay(1);

	__dlv_enable_hp_mute();
	mdelay(1);

	__dlv_switch_sb_dac(POWER_OFF);
	mdelay(1);

	__dlv_switch_sb_sleep(POWER_OFF);
	mdelay(1);

	__dlv_switch_sb(POWER_OFF);
	mdelay(1);
}

static void dlv_startup(void){
	__dlv_switch_sb(POWER_ON);
	mdelay(300);

	__dlv_switch_sb_sleep(POWER_ON);
	mdelay(400);

#if TURN_ON_SB_HP_WHEN_INIT == 1
	__dlv_switch_sb_dac(POWER_ON);
	mdelay(1);
	__dlv_disable_hp_mute();
	mdelay(1);
	turn_on_sb_hp();
	__dlv_set_hp_volume(0x89);
	__dlv_set_dac_gain(0x80);
#endif
}

//**************************************************************************
// CODEC mode ctrl
//**************************************************************************

static unsigned int g_current_mode;
static unsigned int g_replay_wc;
static unsigned int g_record_wc;
static unsigned int g_bypass_wc;
static unsigned int g_jack_on;

static struct dlv_codec_mode g_codec_modes[] = {
	//adc
	{CODEC_MODE_AIPN1_2_ADC, 	 CODEC_MODE_WC_MIC1_V | CODEC_MODE_WC_ADC_V},
	{CODEC_MODE_AIP2_2_ADC,  	 CODEC_MODE_WC_MIC1_V | CODEC_MODE_WC_ADC_V},
	{CODEC_MODE_AIP3_2_ADC,  	 CODEC_MODE_WC_MIC2_V | CODEC_MODE_WC_ADC_V},
	{CODEC_MODE_AIP2_AIP3_2_ADC, CODEC_MODE_WC_MIC1_V | CODEC_MODE_WC_MIC2_V | CODEC_MODE_WC_ADC_V},
	//hp
	{CODEC_MODE_DACL_DACR_2_HP,  CODEC_MODE_WC_DAC_V | CODEC_MODE_WC_HP_V},
	{CODEC_MODE_DACL_2_HP, 	 CODEC_MODE_WC_DAC_V | CODEC_MODE_WC_HP_V}, 
	//bypass to hp
	{CODEC_MODE_AIPN1_2_HP,      CODEC_MODE_WC_LINE_IN1  | CODEC_MODE_WC_HP},
	{CODEC_MODE_AIP2_2_HP,  	 CODEC_MODE_WC_LINE_IN1  | CODEC_MODE_WC_HP},
	{CODEC_MODE_AIP3_2_HP,  	 CODEC_MODE_WC_LINE_IN2  | CODEC_MODE_WC_HP},
	{CODEC_MODE_AIP2_AIP3_2_HP,  CODEC_MODE_WC_LINE_IN1  | CODEC_MODE_WC_LINE_IN2 | CODEC_MODE_WC_HP},

	//line out
	{CODEC_MODE_DACL_2_LO, 	 CODEC_MODE_WC_DAC_V | CODEC_MODE_WC_LINE_OUT_V},
	{CODEC_MODE_DACR_2_LO,  	 CODEC_MODE_WC_DAC_V | CODEC_MODE_WC_LINE_OUT_V},	
	{CODEC_MODE_DACL_DACR_2_LO,	 CODEC_MODE_WC_DAC_V | CODEC_MODE_WC_LINE_OUT_V},	
	//bypass to line out 
	{CODEC_MODE_AIPN1_2_LO,	 CODEC_MODE_WC_LINE_IN1 | CODEC_MODE_WC_LINE_OUT},
	{CODEC_MODE_AIP2_2_LO, 	 CODEC_MODE_WC_LINE_IN1 | CODEC_MODE_WC_LINE_OUT},
	{CODEC_MODE_AIP3_2_LO, 	 CODEC_MODE_WC_LINE_IN2 | CODEC_MODE_WC_LINE_OUT},
	{CODEC_MODE_AIP2_AIP3_2_LO,  CODEC_MODE_WC_LINE_IN2 | CODEC_MODE_WC_LINE_IN1 | CODEC_MODE_WC_LINE_OUT},

};

static void dlv_set_codec_mode(int mode_bit)
{
	if(mode_bit == CODEC_MODE_AIPN1_2_ADC){
		//set MIC1 Path
		__dlv_mic_mono_input();
		__dlv_mic_1_diff_input();
		__dlv_switch_sb_micbias1(POWER_ON);
		__dlv_set_micsel_aipn1();
		__dlv_enable_mic1();
		//set ADC Path
		__dlv_adc_input1_to_lr();
		//set mixer function  l from l r from l
		__dlv_set_aiadc_mixer(AI_ADCL_NORMAL_INPUT | AI_ADCR_CROSS_INPUT);
		__dlv_aiadc_mixer_only_input();//only input 
		__dlv_mixer_enable();

	}else if(mode_bit == CODEC_MODE_AIP2_2_ADC){
		//set MIC1 Path
		__dlv_mic_mono_input();
		__dlv_mic_1_signal_input();
		__dlv_switch_sb_micbias1(POWER_ON);
		__dlv_set_micsel_aip2();
		__dlv_enable_mic1();
		//set ADC Path
		__dlv_adc_input1_to_lr();
		//set mixer function l from l r from l
		__dlv_set_aiadc_mixer(AI_ADCL_NORMAL_INPUT | AI_ADCR_CROSS_INPUT);
		__dlv_aiadc_mixer_only_input();//only input 
		__dlv_mixer_enable();

	}else if(mode_bit == CODEC_MODE_AIP3_2_ADC){
		//set MIC2 Path
		__dlv_mic_mono_input();
		__dlv_mic_2_singal_input();
		__dlv_switch_sb_micbias2(POWER_ON);
		__dlv_set_micsel_aip3();
		__dlv_enable_mic2();
		//set ADC Path
		__dlv_adc_input2_to_lr();
		//set mixer function l from r r form r
		__dlv_set_aiadc_mixer(AI_ADCL_CROSS_INPUT | AI_ADCR_NORMAL_INPUT);
		__dlv_aiadc_mixer_only_input();//only input 
		__dlv_mixer_enable();

	}else if(mode_bit == CODEC_MODE_AIP2_AIP3_2_ADC){
		//set MIC1 PATH
		__dlv_mic_stereo_input();
		__dlv_mic_1_signal_input();
		__dlv_switch_sb_micbias1(POWER_ON);
		__dlv_set_micsel_aip2();
		__dlv_enable_mic1();
		//set MIC2 Path
		__dlv_mic_2_singal_input();
		__dlv_switch_sb_micbias2(POWER_ON);
		__dlv_set_micsel_aip3();
		__dlv_enable_mic2();
		//set ADC Path
		__dlv_adc_input1L_input2R();
		//disable adc left only  
		__dlv_disable_adc_left_only();

	}else if(mode_bit == CODEC_MODE_DACL_DACR_2_HP){
		//set hp path
		__dlv_hp_from_dacl_and_dacr();
		__dlv_disable_dac_left_only();

	}else if(mode_bit == CODEC_MODE_DACL_2_HP){
		__dlv_hp_from_dacl();
		/*ensure DAC is mute before change left_only  */
		if (!(__dlv_get_dac_mute())){
			turn_off_dac(5);
			__dlv_enable_dac_left_only();
			turn_on_dac(10);
		}else
			__dlv_enable_dac_left_only();

	}else if(mode_bit == CODEC_MODE_AIPN1_2_HP){
		/* set LI_1 path */
		__dlv_set_lisel_aipn1();
		__dlv_li_1_diff_input();
		__dlv_enable_li_1_for_bypass();
		__dlv_disable_li_2_for_bypass();
		/* set hp path */
		__dlv_hp_from_ail();

	}else if(mode_bit == CODEC_MODE_AIP2_2_HP){
		/* set LI_1 Path */
		__dlv_set_lisel_aip2();
		__dlv_li_1_singal_input();
		__dlv_enable_li_1_for_bypass();
		__dlv_disable_li_2_for_bypass();
		/* set Hp path */
		__dlv_hp_from_ail();

	}else if(mode_bit == CODEC_MODE_AIP3_2_HP){
		/*set LI_2 Path */
		__dlv_li_2_singal_input();
		__dlv_set_lisel_aip3();
		__dlv_enable_li_2_for_bypass();
		__dlv_disable_li_1_for_bypass();
		/* set Hp path */
		__dlv_hp_from_air();

	}else if(mode_bit == CODEC_MODE_AIP2_AIP3_2_HP){
		__dlv_li_1_singal_input();
		__dlv_set_lisel_aip2();
		__dlv_enable_li_1_for_bypass();

		__dlv_li_2_singal_input();
		__dlv_set_lisel_aip3();
		__dlv_enable_li_2_for_bypass();
		__dlv_hp_from_ail_and_air();

	}else if(mode_bit == CODEC_MODE_DACL_2_LO){
		//set LO path
		__dlv_lineout_from_dacl();
		__dlv_switch_sb_line_out(POWER_ON);
		__dlv_disable_lineout_mute();

	}else if(mode_bit == CODEC_MODE_DACR_2_LO){

		;//nothing

	}else if(mode_bit == CODEC_MODE_DACL_DACR_2_LO){
		__gpio_as_lineout();
		__dlv_lineout_from_dacl_and_dacr();
		__dlv_switch_sb_line_out(POWER_ON);
		__dlv_disable_lineout_mute();

	}else if(mode_bit == CODEC_MODE_AIPN1_2_LO){
		/* set LI_1 path */
		__dlv_set_lisel_aipn1();
		__dlv_li_1_diff_input();
		__dlv_enable_li_1_for_bypass();
		__dlv_disable_li_2_for_bypass();
		/* set lineout path */
		__dlv_lineout_from_ail();
		__dlv_switch_sb_line_out(POWER_ON);
		__dlv_disable_lineout_mute();

	}else if(mode_bit == CODEC_MODE_AIP2_2_LO){
		/* set LI_1 path */
		__dlv_set_lisel_aip2();
		__dlv_li_1_singal_input();
		__dlv_enable_li_1_for_bypass();
		__dlv_disable_li_2_for_bypass();
		/* set lineout path */
		__dlv_lineout_from_ail();
		__dlv_switch_sb_line_out(POWER_ON);
		__dlv_disable_lineout_mute();

	}else if(mode_bit == CODEC_MODE_AIP3_2_LO){

		__dlv_set_lisel_aip3();
		__dlv_li_2_singal_input();
		__dlv_enable_li_2_for_bypass();
		__dlv_disable_li_1_for_bypass();

		__dlv_lineout_from_air();
		__dlv_switch_sb_line_out(POWER_ON);
		__dlv_disable_lineout_mute();

	}else if(mode_bit == CODEC_MODE_AIP2_AIP3_2_LO){
		__dlv_set_lisel_aip2();
		__dlv_li_1_singal_input();
		__dlv_enable_li_1_for_bypass();

		__dlv_set_lisel_aip3();
		__dlv_li_2_singal_input();
		__dlv_enable_li_2_for_bypass();
		__dlv_lineout_from_ail_and_air();
		__dlv_switch_sb_line_out(POWER_ON);
		__dlv_disable_lineout_mute();
	}else {
		printk("error mode id ...\n") ;
	}

}

static void dlv_mode_set_widget(unsigned int wc){
	g_bypass_wc = 0;
	g_replay_wc = 0;
	g_record_wc = 0;
	if(wc & CODEC_MODE_WC_LINE_IN1){
		__dlv_switch_sb_micbias1(POWER_ON);//AIP/N1 is MIC 
		__dlv_enable_li_1_for_bypass();
		g_bypass_wc |= CODEC_MODE_WC_LINE_IN1;
	}else{
		__dlv_disable_li_1_for_bypass();
	}

	if(wc & CODEC_MODE_WC_LINE_IN2){
		__dlv_enable_li_2_for_bypass();
		g_bypass_wc |= CODEC_MODE_WC_LINE_IN2;
	}else{
		__dlv_disable_li_2_for_bypass();
	}

	if(wc & CODEC_MODE_WC_HP){
#if TURN_ON_SB_HP_WHEN_INIT == 0
		turn_on_sb_hp();
#endif
		__dlv_disable_hp_mute();
		g_bypass_wc |= CODEC_MODE_WC_HP;
	}else{
		__dlv_enable_hp_mute();
#if TURN_ON_SB_HP_WHEN_INIT == 0
		turn_off_sb_hp();
#endif
	}    

	if(wc & CODEC_MODE_WC_LINE_OUT){
		__dlv_switch_sb_line_out(POWER_ON);
		__dlv_disable_lineout_mute();
		g_bypass_wc |= CODEC_MODE_WC_LINE_OUT;
	}else{
		__dlv_switch_sb_line_out(POWER_OFF);
		__dlv_enable_lineout_mute();
	}    

	if(wc & CODEC_MODE_WC_ADC_V){
		g_record_wc |= CODEC_MODE_WC_ADC_V;
	}

	if(wc & CODEC_MODE_WC_DAC_V){
		g_replay_wc |= CODEC_MODE_WC_DAC_V;
	}

	if(wc & CODEC_MODE_WC_MIC1_V){
		g_record_wc |= CODEC_MODE_WC_MIC1_V;
	}    

	if(wc & CODEC_MODE_WC_MIC2_V){
		g_record_wc |= CODEC_MODE_WC_MIC2_V;
	}    

	if(wc & CODEC_MODE_WC_HP_V){
		g_replay_wc |= CODEC_MODE_WC_HP_V;
	}    

	if(wc & CODEC_MODE_WC_LINE_OUT_V){
		g_replay_wc |= CODEC_MODE_WC_LINE_OUT_V;
	}    
}

static int dlv_mode_ctrl(unsigned int mode)
{
	int i;
	unsigned int  wc, mode_l, mode_bit;
	JZ4770_DLV_DEBUG_MSG("enter %s, mode = 0x%x\n", __func__, mode);

	if(g_is_replaying || g_is_recording){
		return -1;
	}
	wc = 0;
	mode_l = mode;
	while(mode_l != 0){ // detecte every mode 
		mode_bit = mode_l & (mode_l - 1);
		mode_bit ^= mode_l;
		mode_l = mode_l & (mode_l - 1);

		for(i = 0; i < sizeof(g_codec_modes)/sizeof(struct dlv_codec_mode); ++i){
			if(mode_bit == g_codec_modes[i].id){ 
				dlv_set_codec_mode(g_codec_modes[i].id);
				wc |= g_codec_modes[i].widget_ctrl;
				break;
			}
		}
	}
	JZ4770_DLV_DEBUG_MSG("enter %s, wc = 0x%x\n", __func__, wc);

	dlv_mode_set_widget(wc);

	g_current_mode = mode;
	return 0;
}

//**************************************************************************
// jack detect handle
//**************************************************************************

static void dlv_jack_ctrl(int on)
{
	JZ4770_DLV_DEBUG_MSG("enter %s, on = %d\n", __func__, on);
	if(g_is_replaying){//replay
		if(on){
			JZ4770_DLV_DEBUG_MSG("enter %s, jack on\n", __func__);
			printk("head phone is jacked...\n");
			dlv_mode_ctrl(CODEC_MODE_DACL_DACR_2_HP);
			//open hp
#if TURN_ON_SB_HP_WHEN_INIT == 0
			turn_on_sb_hp();
#endif
			__dlv_disable_hp_mute();
			//close line out
			__dlv_switch_sb_line_out(POWER_OFF);
			__dlv_enable_lineout_mute();
		}else{
			printk("headphne is no jacked\n");
			JZ4770_DLV_DEBUG_MSG("enter %s, jack off\n", __func__);
			dlv_mode_ctrl(CODEC_MODE_DACL_DACR_2_LO);
			//open line out
			__dlv_switch_sb_line_out(POWER_ON);
			__dlv_disable_lineout_mute();
			//close hp
			__dlv_enable_hp_mute();
#if TURN_ON_SB_HP_WHEN_INIT == 0
			turn_off_sb_hp();
#endif
		}
	}
	g_jack_on = on;
}

//**************************************************************************
//  irq handle
//**************************************************************************

/**
 * CODEC short circut handler
 *
 * To protect CODEC, CODEC will be shutdown when short circut occured.
 * Then we have to restart it.
 */
#define VOL_DELAY_BASE 22               //per VOL delay time in ms
static inline void dlv_short_circut_handler(void)
{
	unsigned short curr_vol;
	unsigned int	dlv_ifr, delay;
	int i;
	printk("JZ DLV: Short circut detected! restart CODEC hp out finish.\n");
	curr_vol = codec_read_reg(DLC_GCR_HPL);
	delay = VOL_DELAY_BASE * (0x20 - (curr_vol & 0x1f));
	/* min volume */
	__dlv_set_hp_volume(0xbf);
	printk("Short circut volume delay %d ms curr_vol=%x \n", delay,curr_vol);
	msleep(500);
	turn_off_dac(5);
	__dlv_set_hp_volume(0x80);// set Hp gain to +6dB
	turn_off_sb_hp();
	mdelay(1);
	__dlv_enable_hp_mute();
	mdelay(1);
	__dlv_switch_sb_dac(POWER_OFF);
	mdelay(1);
	__dlv_switch_sb_sleep(POWER_OFF);
	mdelay(1);
	__dlv_switch_sb(POWER_OFF);
	mdelay(1);

	while (1) {
		dlv_ifr = __dlv_get_irq_flag();
		printk("waiting for short circuit recover finish ----- dlv_ifr = 0x%02x\n", dlv_ifr);
		if ((dlv_ifr & DLC_IFR_SCLR ) == 0)
			break;
		__dlv_set_irq_flag((DLC_IFR_SCLR));
		msleep(10);
	}

	__dlv_switch_sb(POWER_ON);
	mdelay(300);

	__dlv_switch_sb_sleep(POWER_ON);
	mdelay(400);

	__dlv_switch_sb_dac(POWER_ON);
	mdelay(1);
	__dlv_disable_hp_mute();
	mdelay(1);
	turn_on_sb_hp();
	__dlv_set_hp_volume(0x89);
	__dlv_set_dac_gain(0x80);


	if(g_is_replaying){
#if TURN_ON_SB_HP_WHEN_INIT == 0
		__dlv_switch_sb_dac(POWER_ON);
		mdelay(1);
		__dlv_disable_hp_mute();
		if(((g_replay_wc & CODEC_MODE_WC_HP_V) && !(g_bypass_wc & CODEC_MODE_WC_HP)) ||  g_jack_on){
			turn_on_sb_hp();
		}
		__dlv_set_hp_volume(0x89);
		__dlv_set_dac_gain(0x80);
#endif
		if(g_replay_volume > 0){
			turn_on_dac(5);
		}
	}
	__dlv_set_hp_volume(curr_vol);
	msleep(delay);

}

/**
 * CODEC work queue handler
 *
 * Handle bottom-half of SCLR & JACKE irq
 *
 */
static unsigned int s_dlv_sr_jack = 2;  //init it to a invalid value
static int dlv_irq_work_handler(struct work_struct *work)
{
	unsigned int	dlv_ifr, dlv_sr_jack;

	DLV_LOCK();

	do {
		dlv_ifr = __dlv_get_irq_flag();
		if (dlv_ifr & DLC_IFR_SCLR) {
			printk("JZ DLV: Short circut detected! dlv_ifr = 0x%02x\n",dlv_ifr);
			dlv_short_circut_handler();
		}
		dlv_ifr = __dlv_get_irq_flag();

	} while(dlv_ifr & (1 << 6));


	//detect jack
	if(dlv_ifr & DLC_IFR_JACK){
		msleep(200);
		dlv_sr_jack = __dlv_hp_jack_status();
		if(dlv_sr_jack != s_dlv_sr_jack){
			if(dlv_sr_jack){
				dlv_jack_ctrl(1);
			}else{
				dlv_jack_ctrl(0);
			}
		}
		s_dlv_sr_jack = __dlv_hp_jack_status();
		__dlv_set_irq_flag(DLC_IFR_JACK);
	}

	__dlv_set_irq_flag(dlv_ifr);
	/* Unmask*/
	__dlv_set_irq_mask(ICR_COMMON_MASK);
	DLV_UNLOCK();
	return 0;
}

/**
 * IRQ routine
 *
 * Now we are only interested in SCLR.
 */
static irqreturn_t dlv_codec_irq(int irq, void *dev_id)
{
	unsigned char dlv_ifr;
	unsigned long flags;

	spin_lock_irqsave(&dlv_irq_lock, flags);
	dlv_ifr = __dlv_get_irq_flag();
	__dlv_set_irq_mask(ICR_ALL_MASK);

	if (!(dlv_ifr & (DLC_IFR_SCLR | DLC_IFR_JACK))) {
		/* CODEC may generate irq with flag = 0xc0.
		 * We have to ingore it in this case as there is no mask for the reserve bit.
		 */
		printk("AIC interrupt, IFR = 0x%02x\n", dlv_ifr);
		__dlv_set_irq_mask(ICR_COMMON_MASK);
		spin_unlock_irqrestore(&dlv_irq_lock, flags);
		return IRQ_HANDLED;
	} else {
		spin_unlock_irqrestore(&dlv_irq_lock, flags);
		/* Handle SCLR and JACK in work queue. */
		schedule_work(&dlv_irq_work);
		return IRQ_HANDLED;
	}
}

//**************************************************************************
//  codec triggle handle
//**************************************************************************
static int dlv_set_replay_volume(int vol);
static int __init dlv_event_init(void)
{
	cpm_start_clock(CGM_AIC0);

	/* init codec params */
	/* ADC/DAC: active + i2s */
	__dlv_set_dac_active();
	__dlv_dac_i2s_mode();
	__dlv_set_adc_active();
	__dlv_adc_i2s_mode();
	/* The generated IRQ is a high level */
	__dlv_set_int_form(DLC_ICR_INT_HIGH);
	__dlv_set_irq_mask(ICR_COMMON_MASK);
	__dlv_set_irq_flag(IFR_ALL_FLAGS);

	/* 12M */
	__dlv_set_12m_crystal();

	/* disable AGC */
	__dlv_adc_agc_enable();

	/* mute lineout/HP */
	__dlv_enable_hp_mute();
	__dlv_enable_lineout_mute();

	//default enable adc left only
	__dlv_disable_adc_left_only();
	//default disable dac left only
	__dlv_disable_dac_left_only();

	//set default mode
	dlv_mode_ctrl(CODEC_MODE_AIPN1_2_ADC | CODEC_MODE_DACL_DACR_2_HP);
	dlv_startup();

	return 0;
}

static int __exit dlv_event_deinit(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);
	dlv_shutdown();
	return 0;
}

static int dlv_event_replay_start(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);
	//in order
	if(g_replay_wc & CODEC_MODE_WC_DAC_V){
#if TURN_ON_SB_HP_WHEN_INIT == 0
		__dlv_switch_sb_dac(POWER_ON);
		mdelay(1);
		if(((g_replay_wc & CODEC_MODE_WC_HP_V) && !(g_bypass_wc & CODEC_MODE_WC_HP)) ||  g_jack_on){
			__dlv_disable_hp_mute();
			turn_on_sb_hp();
		}
		__dlv_set_hp_volume(0x89);
		__dlv_set_dac_gain(0x80);
#endif
		if(((g_replay_wc & CODEC_MODE_WC_LINE_OUT_V) && !(g_bypass_wc & CODEC_MODE_WC_LINE_OUT)) || !(g_jack_on)){
			__gpio_as_lineout();
			__dlv_switch_sb_line_out(POWER_ON);
			__dlv_disable_lineout_mute();
		}
		if(g_replay_volume > 0){
			turn_on_dac(5);
		}

		if(((g_replay_wc & CODEC_MODE_WC_HP_V) && !(g_bypass_wc & CODEC_MODE_WC_HP)) || g_jack_on){
			__dlv_disable_hp_mute();
		}

	}
	g_is_replaying = 1;
	return 0;
}

/* static int first_from_init_i2s = 1; */
static int dlv_event_replay_stop(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);

	if(g_replay_wc & CODEC_MODE_WC_DAC_V){
		turn_off_dac(5);
		if(((g_replay_wc & CODEC_MODE_WC_HP_V) && !(g_bypass_wc & CODEC_MODE_WC_HP)) || g_jack_on){
			__dlv_enable_hp_mute();
		}
		if(((g_replay_wc & CODEC_MODE_WC_LINE_OUT_V) && !(g_bypass_wc & CODEC_MODE_WC_LINE_OUT)) || g_jack_on){
			__dlv_enable_lineout_mute();
		}
		/* anti-pop workaround */
		__aic_write_tfifo(0x0);
		__aic_write_tfifo(0x0);
		__i2s_enable_replay();
		__i2s_enable();
		mdelay(1);
		__i2s_disable_replay();
		__i2s_disable();

#if TURN_ON_SB_HP_WHEN_INIT == 0
		if(((g_replay_wc & CODEC_MODE_WC_HP_V) && !(g_bypass_wc & CODEC_MODE_WC_HP)) ||
				(((g_replay_wc & CODEC_MODE_WC_JD_V) && !(g_bypass_wc & CODEC_MODE_WC_JD)) && g_jack_on)
		  ){
			turn_off_sb_hp();
		}
		__dlv_switch_sb_dac(POWER_OFF);
#endif
		if(((g_replay_wc & CODEC_MODE_WC_LINE_OUT_V) && !(g_bypass_wc & CODEC_MODE_WC_LINE_OUT)) || !g_jack_on){
			__dlv_switch_sb_line_out(POWER_OFF);
		}
	}
	g_is_replaying = 0;
	mdelay(10);
	return 0;
}

static int dlv_event_record_start(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);

	if(g_record_wc & CODEC_MODE_WC_ADC_V){
		__dlv_switch_sb_adc(POWER_ON);
		turn_on_adc(100);

	}
	g_is_recording = 1;
	return 0;
}

static int dlv_event_record_stop(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);

	if(g_record_wc & CODEC_MODE_WC_ADC_V){
		__dlv_switch_sb_adc(POWER_OFF);
		turn_off_adc(100);
	}
	g_is_recording = 0;
	return 0;
}

static int dlv_event_suspend(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);
	if(g_is_replaying){
		dlv_event_replay_stop();
	}
	if(g_is_recording){
		dlv_event_record_stop();
	}
	dlv_mode_ctrl(CODEC_MODE_OFF);
	dlv_shutdown();
	return 0;
}

static int dlv_event_resume(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);
	dlv_startup();
	dlv_mode_ctrl(g_current_mode);
	if(g_is_recording){
		dlv_event_record_start();
	}
	if(g_is_replaying){
		dlv_event_replay_start();
	}
	return 0;
}

//**************************************************************************
//  Provide interface to jz_snd.c
//**************************************************************************

static int dlv_dump_regs(const char *str)
{
	unsigned int i;
	unsigned char data;
	printk("codec register dump, %s:\n", str);
	for (i = 0; i < 60; i++) {
		data = codec_read_reg(i);
		printk("address = 0x%02x, data = 0x%02x\n", i, data);
	}

	return 0;
}

/**
 * Set record or replay data width
 *
 * mode:    record or replay
 * width:	data width to set
 * return:	data width after set
 *
 * Provide data width of codec control interface for jz_snd.c driver
 */
static int dlv_set_width(int mode, int width)
{
	int supported_width[] = {16, 18, 20, 24};
	int i, wd = -1;

	DLV_LOCK();
	JZ4770_DLV_DEBUG_MSG("enter %s, mode = %d, width = %d\n", __func__, mode, width);
	if (width < 16)
		wd = 16;
	else if (width > 24)
		wd = 24;
	else {
		for (i = 0; i < ARRAY_SIZE(supported_width); i++) {
			if (supported_width[i] <= width)
				wd = supported_width[i];
			else
				break;
		}
	}

	if (mode & MODE_REPLAY) {
		switch(wd) {
			case 16:
				__dlv_dac_16bit_sample();
				break;
			case 18:
				__dlv_dac_18bit_sample();
				break;
			case 20:
				__dlv_dac_20bit_sample();
				break;
			case 24:
				__dlv_dac_24bit_sample();
				break;
			default:
				;
		}
	} 
	if (mode & MODE_RECORD){
		switch(wd) {
			case 16:
				__dlv_adc_16bit_sample();
				break;
			case 18:
				__dlv_adc_18bit_sample();
				break;
			case 20:
				__dlv_adc_20bit_sample();
				break;
			case 24:
				__dlv_adc_24bit_sample();
				break;
			default:
				;
		}
	}

	DLV_UNLOCK();
	return wd;
}

/**
 * Set record or replay rate
 *
 * mode:    record or replay
 * rate:	rate to set
 * return:	rate after set
 *
 * Provide rate of codec control interface for jz_snd.c driver
 */
static int dlv_set_rate(int mode, int rate)
{
	int speed = 0, val;
	int mrate[] = {
		8000, 11025, 12000, 16000, 22050,
		24000, 32000, 44100, 48000,88200, 96000 
	};

	JZ4770_DLV_DEBUG_MSG("enter %s, mode = %d, rate = %d\n", __func__, mode, rate);

	for (val = 0; val < ARRAY_SIZE(mrate); val++) {
		if (rate <= mrate[val]) {
			speed = val;
			break;
		}
	}
	if (rate > mrate[ARRAY_SIZE(mrate) - 1]) {
		speed = ARRAY_SIZE(mrate) - 1;
	}

	DLV_LOCK();
	if(mode & MODE_REPLAY){
		__dlv_set_dac_sample_rate(speed);        
	}
	if(mode & MODE_RECORD){
		__dlv_set_adc_sample_rate(speed);
	}
	DLV_UNLOCK();
	return mrate[speed];
}

/**
 * Set record or replay channels
 *
 * mode:    record or replay
 * channels:	channels to set
 * return:	channels after set
 *
 * Provide rate of codec control interface for jz_snd.c driver
 */
static int dlv_set_channels(int mode, int channels)
{
	JZ4770_DLV_DEBUG_MSG("enter %s, mode = %d, channels = %d\n", __func__, mode, channels);
	DLV_LOCK();
	if(mode & MODE_REPLAY){
		if(channels == 1){
			// left_only active 
			__dlv_enable_dac_left_only();
		}else{
			channels = 2;
			// left_only inactive 
			__dlv_disable_dac_left_only();
		}
	}
	DLV_UNLOCK();
	return channels;
}

//FIXME:set volume depends on mode (replay and record)

/**
 * Set replay volume
 *
 * vol:	vol to set
 * return:	vol after set
 *
 * Provide volume of codec control interface for jz_snd.c driver
 */
static int dlv_set_replay_volume(int vol)
{
	unsigned int fixed_vol;

	JZ4770_DLV_DEBUG_MSG("enter %s, vol = %d\n", __func__, vol);

	if(vol < 0){
		vol = 0;
	}
	if(vol > 100){
		vol = 100;
	}

	DLV_LOCK();
	fixed_vol = 31 * (100 - vol) / 100;
	__dlv_set_hp_volume(fixed_vol);

	g_replay_volume = vol;
	if (vol == 0) {
		__dlv_set_dac_gain(0x1f);
		if(g_is_replaying){
			turn_off_dac(10);
		}
	} else {
		__dlv_set_dac_gain(0x06);
		if(g_is_replaying){
			turn_on_dac(10);
		}
	}

	DLV_UNLOCK();
	return vol;
}

/**
 * Set record volume
 *
 * vol:	vol to set
 * return:	vol after set
 *
 * Provide volume of codec control interface for jz_snd.c driver
 */
static int dlv_set_record_volume(int vol)
{
	unsigned int fixed_vol;

	JZ4770_DLV_DEBUG_MSG("enter %s, vol = %d\n", __func__, vol);

	if(vol < 0){
		vol = 0;
	}
	if(vol > 100){
		vol = 100;
	}

	DLV_LOCK();
	fixed_vol = 31 * vol / 100;
	__dlv_set_adc_gain(fixed_vol);
	if (g_record_wc & CODEC_MODE_WC_MIC1_V)
		__dlv_set_mic1_boost(0);
	if(g_record_wc |= CODEC_MODE_WC_MIC2_V)
		__dlv_set_mic2_boost(0);

	DLV_UNLOCK();
	return 0;
}

static int dlv_set_mode(int mode)
{
	int ret = 0;
	JZ4770_DLV_DEBUG_MSG("enter %s, mode = 0x%x\n", __func__, mode);
	DLV_LOCK();
	ret = dlv_mode_ctrl(mode);
	DLV_UNLOCK();
	return ret;
}

static int dlv_set_gcr(struct codec_gcr_ctrl *ctrl)
{

	return 0;
}

/**
 * CODEC triggle  routine
 *
 * Provide event control interface for jz_snd.c driver
 */
static int dlv_triggle(int event)
{
	int ret;
	JZ4770_DLV_DEBUG_MSG("enter %s, event = %d\n", __func__, event);
	DLV_LOCK();

	ret = 0;
	switch(event){
		case CODEC_EVENT_INIT:
			ret = dlv_event_init();
			break;
		case CODEC_EVENT_DEINIT:
			ret = dlv_event_deinit();
			break;
		case CODEC_EVENT_SUSPEND:
			ret = dlv_event_suspend();
			break;
		case CODEC_EVENT_RESUME:
			ret = dlv_event_resume();
			break;
		case CODEC_EVENT_REPLAY_START:
			ret = dlv_event_replay_start();
			break;
		case CODEC_EVENT_REPLAY_STOP:
			ret = dlv_event_replay_stop();
			break;
		case CODEC_EVENT_RECORD_START:
			ret = dlv_event_record_start();
			break;
		case CODEC_EVENT_RECORD_STOP:
			ret = dlv_event_record_stop();
			break;
		default:
			break;
	}

	DLV_UNLOCK();
	return ret;
}

//**************************************************************************
//  Mudule
//**************************************************************************

static struct codec_ops codec_ops = {
	.dump_regs    = dlv_dump_regs,
	.set_width    = dlv_set_width,
	.set_rate     = dlv_set_rate,
	.set_channels = dlv_set_channels,
	.set_replay_volume  = dlv_set_replay_volume,
	.set_record_volume  = dlv_set_record_volume,
	.set_gcr = dlv_set_gcr,
	.triggle  = dlv_triggle,
	.set_mode = dlv_set_mode,
};

/**
 * Module init
 */
static int __init dlv_init(void)
{
	int retval;

	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);

	spin_lock_init(&dlv_irq_lock);
	//init and locked
	DLV_LOCKINIT();
	//unlock
	DLV_UNLOCK();

	register_codec(&codec_ops);

	//register irq
#if 1
	INIT_WORK(&dlv_irq_work, dlv_irq_work_handler);
	retval = request_irq(IRQ_AIC0, dlv_codec_irq, IRQF_DISABLED, "dlv_codec_irq", NULL);
	if (retval) {
		printk("JZ DLV: Could not get AIC CODEC irq %d\n", IRQ_AIC0);
		return retval;
	}
#endif

	return 0;
}

/**
 * Module exit
 */
static void __exit dlv_exit(void)
{
	JZ4770_DLV_DEBUG_MSG("enter %s\n", __func__);

	unregister_codec(&codec_ops);
	free_irq(IRQ_AIC0, NULL);

	DLV_LOCKDEINIT();
}

module_init(dlv_init);
module_exit(dlv_exit);

MODULE_AUTHOR("zli <lliu@ingenic.cn>");
MODULE_DESCRIPTION("zli internel codec driver");
MODULE_LICENSE("GPL");
