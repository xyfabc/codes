/*
 * Linux/sound/oss/jzoss/codecs/jz4780/dlv.h
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */

#ifndef __DLV_H__
#define __DLV_H__

/* Power setting */
#define POWER_ON		0
#define POWER_OFF		1
/* JACK status */
#define JACKED			1
#define NOJACK			0

/* File ops mode W & R */
#define REPLAY			1
#define RECORD			2


#define CODEC_MODE_WC_LINE_IN1    (0x1U << 1)
#define CODEC_MODE_WC_LINE_IN2    (0x1U << 2)
#define CODEC_MODE_WC_HP         (0x1U << 3)
#define CODEC_MODE_WC_LINE_OUT   (0x1U << 4)

#define CODEC_MODE_WC_ADC_V        (0x1U << 16)
#define CODEC_MODE_WC_DAC_V        (0x1U << 17)
#define CODEC_MODE_WC_MIC1_V       (0x1U << 18)
#define CODEC_MODE_WC_MIC2_V       (0x1U << 19)
#define CODEC_MODE_WC_HP_V         (0x1U << 20)
#define CODEC_MODE_WC_LINE_OUT_V   (0x1U << 21)

struct dlv_codec_mode{
	int	id;
	int	widget_ctrl; 
};


#define DLV_DEBUG_SEM(x,y...) //printk(x,##y);

#define DLV_LOCK()                                           \
	do{                                                  \
		if(g_dlv_sem)                                \
		if(down_interruptible(g_dlv_sem))   	     \
		return -ERESTARTSYS;            	     \
		DLV_DEBUG_SEM("dlvsemlock lock\n");          \
	}while(0)

#define DLV_UNLOCK()                                         \
	do{                                                  \
		if(g_dlv_sem)                                \
		up(g_dlv_sem);                               \
		DLV_DEBUG_SEM("dlvsemlock unlock\n");        \
	}while(0)

#define DLV_LOCKINIT()                                      \
	do{                                                 \
		if(g_dlv_sem == NULL)                       \
		g_dlv_sem = (struct semaphore *)vmalloc(sizeof(struct semaphore)); \
		if(g_dlv_sem)                               \
		init_MUTEX_LOCKED(g_dlv_sem);               \
		DLV_DEBUG_SEM("dlvsemlock init\n");         \
	}while(0)

#define DLV_LOCKDEINIT()                                     \
	do{                                                  \
		if(g_dlv_sem)                                \
		vfree(g_dlv_sem);                            \
		g_dlv_sem = NULL;                            \
		DLV_DEBUG_SEM("dlvsemlock deinit\n");        \
	}while(0)

//SR0
#define DLC_SR0_STATUS		0x0
#define DLC_SR0_PON_ACK	(1 << 7)
#define DLC_SR0_IRQ_ACK	(1 << 6)
#define DLC_SR0_JACK_PRESENT	(1 << 5)
#define DLC_SR0_DAC_LOCKED	(1 << 4)

#define __dlv_sample_rate_is_locked()	((codec_read_reg(DLC_SR0_STATUS) & DLC_SR0_DAC_LOCKED) ? 1 : 0)
/*0-no jack; 1 jacked */
#define __dlv_hp_jack_status()		 ((codec_read_reg(DLC_SR0_STATUS) & DLC_SR0_JACK_PRESENT) ? JACKED : NOJACK)
#define __dlv_codec_req_irq()		 ((codec_read_reg(DLC_SR0_STATUS) & DLC_SR0_IRQ_ACK))
#define __dlv_codec_is_ready()		 (codec_read_reg(DLC_SR0_STATUS) & DLC_SR0_PON_ACK)

//SR1
#define DLC_SR1_STATUS		0x1
#define DLC_DAC_UNKOWN_FS	(1 << 4)

#define __dlv_fs_is_support()		(codec_read_reg(DLC_SR1_STATUS) & DLC_DAC_UNKOWN_FS ? 1 : 0)

//MR
#define DLC_MR_STATUS		0x7
#define DLC_ADC_MODE_UPD		(1 << 5)
#define DLC_ADC_MUTE_STATE_MASK		(3 << 3)
#define DLC_DAC_MODE_UPD		(1 << 2)
#define DLC_DAC_MUTE_STATE_MASK		(3 << 0)

#define __get_adc_update()		((codec_read_reg(DLC_MR_STATUS) & DLC_ADC_MODE_UPD) ? 1 : 0)
#define __get_adc_mute_state()		((codec_read_reg(DLC_MR_STATUS) & DLC_ADC_MUTE_STATE_MASK) >> 3)

#define __get_dac_update()		((codec_read_reg(DLC_MR_STATUS) & DLC_DAC_MODE_UPD) ? 1 : 0)
#define __get_dac_mute_state()		((codec_read_reg(DLC_MR_STATUS) & DLC_DAC_MUTE_STATE_MASK) >> 0)

//AICR_DAC register
#define DLC_AICR_DAC		0x8

#define DLC_DAC_ADWL_LSB	6
#define DLC_DAC_ADWL_MASK		(0x3 << 6)
#define DLC_DAC_ADWL_16BIT		(0x0 << 6)
#define DLC_DAC_ADWL_18BIT		(0x1 << 6)
#define DLC_DAC_ADWL_20BIT		(0x2 << 6)
#define DLC_DAC_ADWL_24BIT		(0x3 << 6)
#define DLC_DAC_MASTER_MODE 		(1 << 5)
#define DLC_SB_AICR_DAC			(1 << 4)

#define DLC_DAC_AUDIOIF_MASK		(3 << 0)
#define DLC_DAC_AUDIOIF_PARA		(0x0 << 0)
#define DLC_DAC_AUDIOIF_LEFT		(0x1 << 0)
#define DLC_DAC_AUDIOIF_DSP		(0x2 << 0)
#define DLC_DAC_AUDIOIF_I2S		(0x3 << 0)

#define __dlv_dac_16bit_sample()	  codec_update_reg(DLC_AICR_DAC, DLC_DAC_ADWL_16BIT, DLC_DAC_ADWL_MASK)
#define __dlv_dac_18bit_sample()   codec_update_reg(DLC_AICR_DAC, DLC_DAC_ADWL_18BIT, DLC_DAC_ADWL_MASK)
#define __dlv_dac_20bit_sample()   codec_update_reg(DLC_AICR_DAC, DLC_DAC_ADWL_20BIT, DLC_DAC_ADWL_MASK)
#define __dlv_dac_24bit_sample()   codec_update_reg(DLC_AICR_DAC, DLC_DAC_ADWL_24BIT, DLC_DAC_ADWL_MASK)

#define __dlv_dac_paralle_mode()  codec_update_reg(DLC_AICR_DAC, DLC_DAC_AUDIOIF_PARA, DLC_DAC_AUDIOIF_MASK) 
#define __dlv_dac_left_mode() 	  codec_update_reg(DLC_AICR_DAC, DLC_DAC_AUDIOIF_LEFT, DLC_DAC_AUDIOIF_MASK) 
#define __dlv_dac_dsp_mode() 	  codec_update_reg(DLC_AICR_DAC, DLC_DAC_AUDIOIF_DSP, DLC_DAC_AUDIOIF_MASK) 
#define __dlv_dac_i2s_mode() 	  codec_update_reg(DLC_AICR_DAC, DLC_DAC_AUDIOIF_I2S, DLC_DAC_AUDIOIF_MASK) 

#define __dlv_set_dac_active()	  codec_write_reg_bit(DLC_AICR_DAC, 0, DLC_SB_AICR_DAC) 
#define __dlv_set_dac_master()    codec_write_reg_bit(DLC_AICR_DAC, 0, DLC_DAC_MASTER_MODE ) 
#define __dlv_set_dac_slave()  	  codec_write_reg_bit(DLC_AICR_DAC, 1, DLC_DAC_MASTER_MODE ) 

//AICR_ADC register
#define DLC_AICR_ADC		0x9
#define DLC_ADC_ADWL_LSB	6
#define DLC_ADC_ADWL_MASK	(0x3 << 6)
#define DLC_ADC_ADWL_16BIT	(0x0 << 6)
#define DLC_ADC_ADWL_18BIT	(0x1 << 6)
#define DLC_ADC_ADWL_20BIT	(0x2 << 6)
#define DLC_ADC_ADWL_24BIT	(0x3 << 6)

#define DLC_SB_AICR_ADC                 (1 << 4)
#define DLC_ADC_AUDIOIF_MASK            (3 << 0)
#define DLC_ADC_AUDIOIF_PARA            (0x0 << 0)
#define DLC_ADC_AUDIOIF_LEFT            (0x1 << 0)
#define DLC_ADC_AUDIOIF_DSP             (0x2 << 0)
#define DLC_ADC_AUDIOIF_I2S             (0x3 << 0)

#define __dlv_adc_16bit_sample()  codec_update_reg(DLC_AICR_ADC, DLC_ADC_ADWL_16BIT, DLC_ADC_ADWL_MASK)
#define __dlv_adc_18bit_sample()  codec_update_reg(DLC_AICR_ADC, DLC_ADC_ADWL_18BIT, DLC_ADC_ADWL_MASK)
#define __dlv_adc_20bit_sample()  codec_update_reg(DLC_AICR_ADC, DLC_ADC_ADWL_20BIT, DLC_ADC_ADWL_MASK)
#define __dlv_adc_24bit_sample()  codec_update_reg(DLC_AICR_ADC, DLC_ADC_ADWL_24BIT, DLC_ADC_ADWL_MASK)

#define __dlv_adc_paralle_mode()  codec_update_reg(DLC_AICR_ADC, DLC_ADC_AUDIOIF_PARA, DLC_ADC_AUDIOIF_MASK) 
#define __dlv_adc_letf_mode()     codec_update_reg(DLC_AICR_ADC, DLC_ADC_AUDIOIF_LEFT, DLC_ADC_AUDIOIF_MASK) 
#define __dlv_adc_dsp_mode()      codec_update_reg(DLC_AICR_ADC, DLC_ADC_AUDIOIF_DSP, DLC_ADC_AUDIOIF_MASK) 
#define __dlv_adc_i2s_mode()      codec_update_reg(DLC_AICR_ADC, DLC_ADC_AUDIOIF_I2S, DLC_ADC_AUDIOIF_MASK) 

#define __dlv_set_adc_active()    codec_write_reg_bit(DLC_AICR_ADC, 0, DLC_SB_AICR_ADC) 

//CR_LO register
#define DLC_CR_LO		0xB

#define DLC_LO_MUTE		(1 << 7)
#define DLC_SB_LO		(1 << 4)
#define DLC_LO_SEL_LSB		0

#define DLC_LO_SEL_MASK		(0x7 << 0)
#define DLC_LO_SEL_DACL		(0x0 << 0)
#define DLC_LO_SEL_DACR		(0x1 << 0)
#define DLC_LO_SEL_DACL_AND_DACR (0x2 << 0)
#define DLC_LO_SEL_AIL		(0x4 << 0)
#define DLC_LO_SEL_AIR		(0x5 << 0)
#define DLC_LO_SEL_AIL_AND_AIR	(0x6 << 0)

#define __dlv_get_sb_line_out()		((codec_read_reg(DLC_CR_LO) & DLC_SB_LO) ? POWER_OFF : POWER_ON)

#define __dlv_switch_sb_line_out(pwrstat)				\
	do {								\
		if (__dlv_get_sb_line_out() != pwrstat) {		\
			codec_write_reg_bit(DLC_CR_LO, pwrstat, DLC_SB_LO); \
		}							\
	} while (0)

#define __dlv_enable_lineout_mute()					\
	do {								\
		codec_write_reg_bit(DLC_CR_LO, 1, DLC_LO_MUTE);		\
	} while (0)

#define __dlv_disable_lineout_mute()					\
	do {								\
		codec_write_reg_bit(DLC_CR_LO, 0, DLC_LO_MUTE);		\
	} while (0)

#define __dlv_lineout_from_dacl()	codec_update_reg(DLC_CR_LO, DLC_LO_SEL_DACL, DLC_LO_SEL_MASK)
#define __dlv_lineout_from_dacr()	codec_update_reg(DLC_CR_LO, DLC_LO_SEL_DACR, DLC_LO_SEL_MASK)
#define __dlv_lineout_from_dacl_and_dacr() codec_update_reg(DLC_CR_LO, DLC_LO_SEL_DACL_AND_DACR, DLC_LO_SEL_MASK)
#define __dlv_lineout_from_ail()	codec_update_reg(DLC_CR_LO, DLC_LO_SEL_AIL, DLC_LO_SEL_MASK)
#define __dlv_lineout_from_air()	codec_update_reg(DLC_CR_LO, DLC_LO_SEL_AIR, DLC_LO_SEL_MASK)
#define __dlv_lineout_from_ail_and_air()codec_update_reg(DLC_CR_LO, DLC_LO_SEL_AIL_AND_AIR, DLC_LO_SEL_MASK)

//CR_Hp
#define DLC_CR_HP		0xD

#define DLC_HP_MUTE		(1 << 7)
#define DLC_SB_HP		(1 << 4)

#define DLC_HP_SEL_MASK		(7 << 0)
#define DLC_HP_DACL_AND_DACR	(0x0 << 0)
#define DLC_HP_DACL		(0x1 << 0)
#define DLC_HP_AIL_AND_AIR	(0x2 << 0)
#define DLC_HP_AIL		(0x3 << 0)
#define DLC_HP_AIR		(0x4 << 0)

#define __dlv_get_sb_hp()		((codec_read_reg(DLC_CR_HP) & DLC_SB_HP) ? POWER_OFF : POWER_ON)

#define __dlv_switch_sb_hp(pwrstat)					\
	do {								\
		if (__dlv_get_sb_hp() != pwrstat) {			\
			codec_write_reg_bit(DLC_CR_HP, pwrstat, DLC_SB_HP); \
		}							\
		\
	} while (0)

#define __dlv_enable_hp_mute()					\
	do {							\
		codec_write_reg_bit(DLC_CR_HP, 1, DLC_HP_MUTE);	\
		\
	} while (0)

#define __dlv_disable_hp_mute()					\
	do {							\
		codec_write_reg_bit(DLC_CR_HP, 0, DLC_HP_MUTE);	\
		\
	} while (0)

#define __dlv_hp_from_dacl_and_dacr() codec_update_reg(DLC_CR_HP, DLC_HP_DACL_AND_DACR, DLC_HP_SEL_MASK)
#define __dlv_hp_from_dacl()	      codec_update_reg(DLC_CR_HP, DLC_HP_DACL, DLC_HP_SEL_MASK)
#define __dlv_hp_from_ail_and_air()   codec_update_reg(DLC_CR_HP, DLC_HP_AIL_AND_AIR, DLC_HP_SEL_MASK)
#define __dlv_hp_from_ail()	      codec_update_reg(DLC_CR_HP, DLC_HP_AIL, DLC_HP_SEL_MASK)
#define __dlv_hp_from_air()	      codec_update_reg(DLC_CR_HP, DLC_HP_AIR, DLC_HP_SEL_MASK)

//CR DMIC
#define DLC_CR_DMIC		0x10
#define DLC_CR_DMIC_CLKON	(1 << 7)

#define __dlv_enable_dmic_clock()     codec_write_reg_bit(DLC_CR_DMIC, 1, DLC_CR_DMIC_CLKON) 
#define __dlv_disable_dmic_clock()    codec_write_reg_bit(DLC_CR_DMIC, 0, DLC_CR_DMIC_CLKON) 


//CR MIC 1
#define DLC_CR_MIC1              0x11

#define DLC_CR_MIC_STEREO       (1 << 7)
#define DLC_CR_MIC_1_DIFF  	(1 << 6)
#define DLC_CR_SB_MICBIAS1      (1 << 5)
#define DLC_CR_SB_MIC1 	 	(1 << 4)
#define DLC_CR_MIC_1_SEL       (1 << 0)

#define __dlv_mic_stereo_input()       codec_write_reg_bit(DLC_CR_MIC1, 1, DLC_CR_MIC_STEREO)
#define __dlv_mic_mono_input()         codec_write_reg_bit(DLC_CR_MIC1, 0, DLC_CR_MIC_STEREO)

#define __dlv_mic_1_diff_input()      codec_write_reg_bit(DLC_CR_MIC1, 1, DLC_CR_MIC_1_DIFF)
#define __dlv_mic_1_signal_input()     codec_write_reg_bit(DLC_CR_MIC1, 0, DLC_CR_MIC_1_DIFF)

#define __dlv_get_sb_micbias1()       ((codec_read_reg(DLC_CR_MIC1) & DLC_CR_SB_MICBIAS1) ? POWER_OFF : POWER_ON)
#define __dlv_switch_sb_micbias1(pwrstat)                                \
	do {                                                            \
		if (__dlv_get_sb_micbias1() != pwrstat) {                \
			codec_write_reg_bit(DLC_CR_MIC1, pwrstat, DLC_CR_SB_MICBIAS1); \
		}                                                       \
	} while (0)


#define __dlv_enable_mic1()  codec_write_reg_bit(DLC_CR_MIC1, 0, DLC_CR_SB_MIC1)
#define __dlv_disable_mic1() codec_write_reg_bit(DLC_CR_MIC1, 1, DLC_CR_SB_MIC1)

#define __dlv_set_micsel_aipn1()	codec_write_reg_bit(DLC_CR_MIC1, 0, DLC_CR_MIC_1_SEL)
#define __dlv_set_micsel_aip2()		codec_write_reg_bit(DLC_CR_MIC1, 1, DLC_CR_MIC_1_SEL)

//CR MIC2
#define DLC_CR_MIC2              0x12

#define DLC_CR_MIC_2_DIFF       (1 << 6)
#define DLC_CR_SB_MICBIAS2      (1 << 5)
#define DLC_CR_SB_MIC2          (1 << 4)
#define DLC_CR_MIC_2_SEL        (1 << 0)

#define __dlv_mic_2_singal_input()  codec_write_reg_bit(DLC_CR_MIC2, 0, DLC_CR_MIC_2_DIFF)

#define __dlv_get_sb_micbias2()       ((codec_read_reg(DLC_CR_MIC2) & DLC_CR_SB_MICBIAS2) ? POWER_OFF : POWER_ON)
#define __dlv_switch_sb_micbias2(pwrstat)                                \
	do {                                                            \
		if (__dlv_get_sb_micbias2() != pwrstat) {                \
			codec_write_reg_bit(DLC_CR_MIC2, pwrstat, DLC_CR_SB_MICBIAS2); \
		}                                                       \
	} while (0)


#define __dlv_enable_mic2()  codec_write_reg_bit(DLC_CR_MIC2, 0, DLC_CR_SB_MIC2)
#define __dlv_disable_mic2() codec_write_reg_bit(DLC_CR_MIC2, 1, DLC_CR_SB_MIC2)

#define __dlv_set_micsel_aip3()    codec_write_reg_bit(DLC_CR_MIC2, 0, DLC_CR_MIC_2_SEL)

//CR LI1
#define DLC_CR_LI_1             0x13

#define DLC_CR_LI_1_DIFF 		      (1 << 6)
#define DLC_CR_LI_1_SB_LIBY     	      (1 << 4)
#define DLC_CR_LI_1_SEL			      (1 << 0)

#define __dlv_li_1_diff_input()		codec_write_reg_bit(DLC_CR_LI_1, 1, DLC_CR_LI_1_DIFF)
#define __dlv_li_1_singal_input()	codec_write_reg_bit(DLC_CR_LI_1, 0, DLC_CR_LI_1_DIFF)

#define __dlv_enable_li_1_for_bypass()  codec_write_reg_bit(DLC_CR_LI_1, 0, DLC_CR_LI_1_SB_LIBY)
#define __dlv_disable_li_1_for_bypass() codec_write_reg_bit(DLC_CR_LI_1, 1, DLC_CR_LI_1_SB_LIBY)

#define __dlv_set_lisel_aipn1()		codec_write_reg_bit(DLC_CR_LI_1, 0, DLC_CR_LI_1_SEL)
#define __dlv_set_lisel_aip2() 		codec_write_reg_bit(DLC_CR_LI_1, 1, DLC_CR_LI_1_SEL)

//CR LI2
#define DLC_CR_LI_2             0x14

#define DLC_CR_LI_2_DIFF                      (1 << 6)
#define DLC_CR_LI_2_SB_LIBY                   (1 << 4)
#define DLC_CR_LI_2_SEL                       (1 << 0)

#define __dlv_li_2_singal_input()       codec_write_reg_bit(DLC_CR_LI_2, 0, DLC_CR_LI_2_DIFF)

#define __dlv_enable_li_2_for_bypass()  codec_write_reg_bit(DLC_CR_LI_2, 0, DLC_CR_LI_2_SB_LIBY)
#define __dlv_disable_li_2_for_bypass() codec_write_reg_bit(DLC_CR_LI_2, 1, DLC_CR_LI_2_SB_LIBY)

#define __dlv_set_lisel_aip3()          codec_write_reg_bit(DLC_CR_LI_2, 0, DLC_CR_LI_2_SEL)

//CR_DAC
#define DLC_CR_DAC		0x17

#define DLC_CR_DAC_MUTE		(1 << 7)
#define DLC_CR_DAC_LEFT_ONLY	(1 << 5)
#define DLC_CR_SB_DAC		(1 << 4)

#define __dlv_get_dac_mute()        ((codec_read_reg(DLC_CR_DAC) & DLC_CR_DAC_MUTE) ? 1 : 0)
#define __dlv_enable_dac_mute()						\
	do {								\
		codec_write_reg_bit(DLC_CR_DAC, 1, DLC_CR_DAC_MUTE);	\
	} while (0)

#define __dlv_disable_dac_mute()					\
	do {								\
		codec_write_reg_bit(DLC_CR_DAC, 0, DLC_CR_DAC_MUTE);	\
		\
	} while (0)


#define __dlv_get_sb_dac()		((codec_read_reg(DLC_CR_DAC) & DLC_CR_SB_DAC) ? POWER_OFF : POWER_ON)
#define __dlv_switch_sb_dac(pwrstat)					\
	do {								\
		if (__dlv_get_sb_dac() != pwrstat) {			\
			codec_write_reg_bit(DLC_CR_DAC, pwrstat, DLC_CR_SB_DAC); \
		}							\
		\
	} while (0)

#define __dlv_enable_dac_left_only()  codec_write_reg_bit(DLC_CR_DAC, 1, DLC_CR_DAC_LEFT_ONLY)
#define __dlv_disable_dac_left_only() codec_write_reg_bit(DLC_CR_DAC, 0, DLC_CR_DAC_LEFT_ONLY)

//CR_ADC
#define DLC_CR_ADC		0x18

#define DLC_CR_ADC_MUTE		(1 << 7)
#define DLC_CR_ADC_DMIC_SEL	(1 << 6)
#define DLC_CR_ADC_LEFT_ONLY	(1 << 5)
#define DLC_CR_SB_ADC		(1 << 4)

#define __dlv_get_adc_mute()        ((codec_read_reg(DLC_CR_ADC) & DLC_CR_ADC_MUTE) ? 1 : 0)
#define __dlv_enable_adc_mute()                                         \
	do {                                                            \
		codec_write_reg_bit(DLC_CR_ADC, 1, DLC_CR_ADC_MUTE);      \
		\
	} while (0)

#define __dlv_disable_adc_mute()                                        \
	do {                                                            \
		codec_write_reg_bit(DLC_CR_ADC, 0, DLC_CR_ADC_MUTE);      \
		\
	} while (0)

#define __dlv_adc_to_filter()	codec_write_reg_bit(DLC_CR_ADC, 0, DLC_CR_ADC_DMIC_SEL)
#define __dlv_dmic_to_filter()	codec_write_reg_bit(DLC_CR_ADC, 1, DLC_CR_ADC_DMIC_SEL)

#define __dlv_enable_adc_left_only()	codec_write_reg_bit(DLC_CR_ADC, 1, DLC_CR_ADC_LEFT_ONLY)
#define __dlv_disable_adc_left_only()	codec_write_reg_bit(DLC_CR_ADC, 0, DLC_CR_ADC_LEFT_ONLY)

#define __dlv_get_sb_adc()          ((codec_read_reg(DLC_CR_ADC) & DLC_CR_SB_ADC) ? POWER_OFF : POWER_ON)
#define __dlv_switch_sb_adc(pwrstat)                                    \
	do {                                                            \
		if (__dlv_get_sb_adc() != pwrstat) {                    \
			codec_write_reg_bit(DLC_CR_ADC, pwrstat, DLC_CR_SB_ADC); \
		}                                                       \
		\
	} while (0)


#define DLC_CR_IN_SEL_LSB	0
#define DLC_CR_IN_SEL_MASK	0x3
#define DLC_CR_INPUT1_TO_LR	0x0
#define DLC_CR_INPUT2_TO_LR	0x1
/* if MIC_STEREO */
#define DLC_CR_INPUT1L_INPUT2R	0x0
#define DLC_CR_INPUT1R_INPUT2L	0x1

#define __dlv_adc_input1_to_lr()	codec_update_reg(DLC_CR_ADC, DLC_CR_INPUT1_TO_LR, DLC_CR_IN_SEL_MASK)
#define __dlv_adc_input2_to_lr()	codec_update_reg(DLC_CR_ADC, DLC_CR_INPUT2_TO_LR, DLC_CR_IN_SEL_MASK)

#define __dlv_adc_input1L_input2R()	codec_update_reg(DLC_CR_ADC, DLC_CR_INPUT1L_INPUT2R, DLC_CR_IN_SEL_MASK)
#define __dlv_adc_input1R_input2L()	codec_update_reg(DLC_CR_ADC, DLC_CR_INPUT1R_INPUT2L, DLC_CR_IN_SEL_MASK)

//CR_MIXER 
#define DLC_CR_MIX		0x19
//DR_MIXER
#define DLC_DR_MIX_DATA		0x1A

#define DLC_CR_MIX_ENA		(1 << 7)
#define DLC_CR_MIX_LOAD		(1 << 6)

#define DLC_CR_MIX_ADDR_LSB	0
#define DLC_CR_MIX_ADDR_MASK	(0x3F << 0)		

#define DLC_CR_MIX_0		(0x00 << 0)
#define DLC_CR_MIX_1		(0x01 << 0)
#define DLC_CR_MIX_2		(0x02 << 0)
#define DLC_CR_MIX_3		(0x03 << 0)

#define __dlv_mixer_enable()	codec_write_reg_bit(DLC_CR_MIX, 1, DLC_CR_MIX_ENA)
#define __dlv_mixer_disable()	codec_write_reg_bit(DLC_CR_MIX, 0, DLC_CR_MIX_ENA)

#define __dlv_set_mixer_load()	codec_write_reg_bit(DLC_CR_MIX, 1, DLC_CR_MIX_LOAD)
#define __dlv_clear_mixer_load()	codec_write_reg_bit(DLC_CR_MIX, 0, DLC_CR_MIX_LOAD)

#define __dlv_set_mixer_addr(addr)	codec_update_reg(DLC_CR_MIX, (addr << DLC_CR_MIX_ADDR_LSB), DLC_CR_MIX_ADDR_MASK)

#define __dlv_set_mixer_data(data)	codec_write_reg(DLC_DR_MIX_DATA, data)



#define CR_MIX_0	0x0

#define AI_DACL_SEL_MASK	(3 << 6)
#define AI_DACL_NORMAL_INPUT	(0x0 << 6)
#define AI_DACL_CROSS_INPUT	(0x1 << 6)
#define AI_DACL_MIXED_INPUT	(0x2 << 6)
#define AI_DACL_0_INPUT		(0x3 << 6)

#define AI_DACR_SEL_MASK	(3 << 4)
#define AI_DACR_NORMAL_INPUT	(0x0 << 4)
#define AI_DACR_CROSS_INPUT	(0x1 << 4)
#define AI_DACR_MIXED_INPUT	(0x2 << 4)
#define AI_DACR_0_INPUT		(0x3 << 4)

#define AI_DAC_MIXER		(1 << 0)

#define __dlv_set_aidac_mixer(mode)                                    \
	do {                                                            \
		int old_mode = codec_read_mixer_reg(CR_MIX_0);		\
		old_mode &= ~((AI_DACL_SEL_MASK) | (AI_DACR_SEL_MASK));	\
		old_mode |= mode;					\
		codec_write_mixer_reg(CR_MIX_0, old_mode);			\
	} while (0)

#define __dlv_aidac_mixer_only_dac()		codec_write_mixer_reg_bit(CR_MIX_0, 0, AI_DAC_MIXER)
#define __dlv_aidac_mixer_dac_and_adc()	codec_write_mixer_reg_bit(CR_MIX_0, 1, AI_DAC_MIXER)


#define CR_MIX_1	0x1

#define MIX_DACL_SEL_MASK        (3 << 6)
#define MIX_DACL_NORMAL_INPUT    (0x0 << 6)
#define MIX_DACL_CROSS_INPUT     (0x1 << 6)
#define MIX_DACL_MIXED_INPUT     (0x2 << 6)
#define MIX_DACL_0_INPUT         (0x3 << 6)

#define MIX_DACR_SEL_MASK        (3 << 4)
#define MIX_DACR_NORMAL_INPUT    (0x0 << 4)
#define MIX_DACR_CROSS_INPUT     (0x1 << 4)
#define MIX_DACR_MIXED_INPUT     (0x2 << 4)
#define MIX_DACR_0_INPUT         (0x3 << 4)

#define __dlv_set_mixdac_mixer(mode)                                    \
	do {                                                            \
		int old_mode = codec_read_mixer_reg(CR_MIX_1);		\
		old_mode &= ~((MIX_DACL_SEL_MASK) | (MIX_DACR_SEL_MASK));	\
		old_mode |= mode;					\
		codec_write_mixer_reg(CR_MIX_1, old_mode);			\
	} while (0)


#define CR_MIX_2	0x2

#define AI_ADCL_SEL_MASK        (3 << 6)
#define AI_ADCL_NORMAL_INPUT    (0x0 << 6)
#define AI_ADCL_CROSS_INPUT     (0x1 << 6)
#define AI_ADCL_MIXED_INPUT     (0x2 << 6)
#define AI_ADCL_0_INPUT         (0x3 << 6)

#define AI_ADCR_SEL_MASK        (3 << 4)
#define AI_ADCR_NORMAL_INPUT    (0x0 << 4)
#define AI_ADCR_CROSS_INPUT     (0x1 << 4)
#define AI_ADCR_MIXED_INPUT     (0x2 << 4)
#define AI_ADCR_0_INPUT         (0x3 << 4)

#define AI_ADC_MIXER		(1 << 0)

#define __dlv_set_aiadc_mixer(mode)                                    \
	do {                                                            \
		int old_mode = codec_read_mixer_reg(CR_MIX_2);		\
		old_mode &= ~((AI_ADCL_SEL_MASK) | (AI_ADCR_SEL_MASK));	\
		old_mode |= mode;					\
		codec_write_mixer_reg(CR_MIX_2, old_mode);			\
	} while (0)

#define __dlv_aiadc_mixer_only_input()		codec_write_mixer_reg_bit(CR_MIX_2, 0, AI_ADC_MIXER)
#define __dlv_aiadc_mixer_input_and_dac()	codec_write_mixer_reg_bit(CR_MIX_2, 1, AI_DAC_MIXER)


#define CR_MIX_3	0x3

#define MIX_ADCL_SEL_MASK        (3 << 6)
#define MIX_ADCL_NORMAL_INPUT    (0x0 << 6)
#define MIX_ADCL_CROSS_INPUT     (0x1 << 6)
#define MIX_ADCL_MIXED_INPUT     (0x2 << 6)
#define MIX_ADCL_0_INPUT         (0x3 << 6)

#define MIX_ADCR_SEL_MASK        (3 << 4)
#define MIX_ADCR_NORMAL_INPUT    (0x0 << 4)
#define MIX_ADCR_CROSS_INPUT     (0x1 << 4)
#define MIX_ADCR_MIXED_INPUT     (0x2 << 4)
#define MIX_ADCR_0_INPUT         (0x3 << 4)

#define __dlv_set_mixadc_mixer(mode)                                    \
	do {                                                            \
		int old_mode = codec_read_mixer_reg(CR_MIX_3);		\
		old_mode &= ~((AI_ADCL_SEL_MASK) | (AI_ADCR_SEL_MASK));	\
		old_mode |= mode;					\
		codec_write_mixer_reg(CR_MIX_2, old_mode);			\
	} while (0)


//CR_VIC
#define DLC_CR_VIC		0x1B

#define DLC_VIC_SB_SLEEP	(1 << 1)
#define DLC_VIC_SB		(1 << 0)

#define __dlv_get_sb()			((codec_read_reg(DLC_CR_VIC) & DLC_VIC_SB) ? POWER_OFF : POWER_ON)
#define __dlv_switch_sb(pwrstat)					\
	do {								\
		if (__dlv_get_sb() != pwrstat) {			\
			codec_write_reg_bit(DLC_CR_VIC, pwrstat, DLC_VIC_SB); \
		}							\
	} while (0)


#define __dlv_get_sb_sleep()		((codec_read_reg(DLC_CR_VIC) & DLC_VIC_SB_SLEEP) ? POWER_OFF : POWER_ON)
#define __dlv_switch_sb_sleep(pwrstat)					\
	do {								\
		if (__dlv_get_sb_sleep() != pwrstat) {			\
			codec_write_reg_bit(DLC_CR_VIC, pwrstat, DLC_VIC_SB_SLEEP); \
		}							\
		\
	} while (0)

//CR_CK
#define DLC_CCR		0x1C

#define DLC_CCR_MCLK_ENA	(1 << 4)

#define DLC_CCR_CRYSTAL_LSB	0
#define DLC_CCR_CRYSTAL_MASK	0xf

/* default to 12MHZ(Note: must be 12MHZ) */
#define DLC_CCR_CRYSTAL_12M	0x0
#define DLC_CCR_CRYSTAL_13M	0x2

#define __dlv_enable_mclk()	codec_write_reg_bit(DLC_CCR, 0, DLC_CCR_MCLK_ENA);
#define __dlv_disable_mclk()	codec_write_reg_bit(DLC_CCR, 1, DLC_CCR_MCLK_ENA)

#define __dlv_set_12m_crystal()						\
	do {								\
		codec_update_reg(DLC_CCR, DLC_CCR_CRYSTAL_12M, DLC_CCR_CRYSTAL_MASK); \
	} while (0)


//FRC_DAC
#define DLC_FCR_DAC		0x1D

#define DLC_DAC_FREQ_LSB	0
#define DLC_DAC_FREQ_MASK	0xf

/*
 * rate = b'0000 - b'1010 (8K ~ 96k)
 */
#define __dlv_set_dac_sample_rate(rate)		\
	codec_update_reg(DLC_FCR_DAC, (rate) << DLC_DAC_FREQ_LSB, DLC_DAC_FREQ_MASK)

//FRC_ADC
#define DLC_FCR_ADC		0x20

#define DLC_ADC_HPF		(1 << 6)

#define DLC_ADC_WNF_LSB		4
#define DLC_ADC_WNF_MASK	(3 << 4)

#define DLC_ADC_FREQ_LSB	0
#define DLC_ADC_FREQ_MASK	0xF

#define __dlv_enable_high_pass()	codec_write_reg_bit(DLC_FCR_ADC, 1, DLC_ADC_HPF)
#define __dlv_disable_high_pass()	codec_write_reg_bit(DLC_FCR_ADC, 0, DLC_ADC_HPF)

/* n == 0 ,the function disable */
#define __dlv_set_adc_wind_noise_filter(n)	\
	codec_update_reg(DLC_FCR_ADC, (n) << DLC_ADC_WNF_LSB, DLC_ADC_WNF_MASK)

/*
 * rate = b'0000 - b'1010 (8K ~ 96k)
 */
#define __dlv_set_adc_sample_rate(rate)		\
	codec_update_reg(DLC_FCR_ADC, (rate) << DLC_ADC_FREQ_LSB, DLC_ADC_FREQ_MASK)


//CR_TIMER_MSB and CR_TIMER_LSB	 
#define DLC_CR_TIMER_MSB	0x21
#define DLC_CR_TIMER_LSB	0x22

//ICR
#define DLC_ICR			0x23

#define DLC_ICR_INT_FORM_LSB	6
#define DLC_ICR_INT_FORM_MASK	(0x3 << 6)

#define DLC_ICR_INT_HIGH	(0x0 << 6)
#define DLC_ICR_INT_LOW		(0x1 << 6)
#define DLC_ICR_INT_HPULSE_8	(0x2 << 6)
#define DLC_ICR_INT_LPULSE_8	(0x3 << 6)

#define __dlv_set_int_form(v)						\
	do {								\
		codec_update_reg(DLC_ICR, (v), DLC_ICR_INT_FORM_MASK);	\
	} while (0)

//IMR
#define DLC_IMR		0x24

#define DLC_IMR_LOCK		(1 << 7)
#define DLC_IMR_SCLR		(1 << 6)
#define DLC_IMR_JACK		(1 << 5)
#define DLC_IMR_ADC_MUTE	(1 << 2)
#define DLC_IMR_DAC_MODE	(1 << 1)
#define DLC_IMR_DAC_MUTE	(1 << 0)

#define ICR_ALL_MASK							\
	(DLC_IMR_LOCK | DLC_IMR_SCLR | DLC_IMR_JACK | DLC_IMR_ADC_MUTE | DLC_IMR_DAC_MODE | DLC_IMR_DAC_MUTE)

#define ICR_COMMON_MASK	(ICR_ALL_MASK & (~(DLC_IMR_SCLR | DLC_IMR_JACK)))

#define __dlv_set_irq_mask(mask)		\
	do {					\
		codec_write_reg(DLC_IMR, mask);	\
		\
	} while (0)

//IFR
#define DLC_IFR		0x25

#define DLC_IFR_LOCK		(1 << 7)
#define DLC_IFR_SCLR		(1 << 6)
#define DLC_IFR_JACK		(1 << 5)
#define DLC_IFR_ADC_MUTE        (1 << 2)
#define DLC_IFR_DAC_MODE        (1 << 1)
#define DLC_IFR_DAC_MUTE        (1 << 0)

#define IFR_ALL_FLAGS	(DLC_IFR_SCLR | DLC_IFR_JACK | DLC_IFR_LOCK | DLC_IFR_ADC_MUTE | DLC_IFR_DAC_MUTE |  DLC_IFR_DAC_MODE)

#define __dlv_set_irq_flag(flag)			\
	do {						\
		codec_write_reg(DLC_IFR, flag);	\
		\
	} while (0)

#define __dlv_get_irq_flag()		(codec_read_reg(DLC_IFR))


//IMR2
#define DLC_IMR_2	0x26
#define DLC_IMR2_TIMER_END	(1 << 4)

#define __dlv_enable_timer_end_inter()	codec_write_reg_bit(DLC_IMR_2, 1, DLC_IMR2_TIMER_END)

//IFR2
#define DLC_IFR_2	0x27
#define DLC_IFR2_TIMER_END	(1 << 4)

#define __dlv_get_timer_flag()		codec_read_reg(DLC_IMR_2)


//GCR_HP	HPL
#define DLC_GCR_HPL	0x28
#define DLC_GCR_HPL_LRGO	(1 << 7)

#define DLC_GOL_LSB	0
#define DLC_GOL_MASK	0x1f

//GCR_HP	HPR
#define DLC_GCR_HPR	0x29

#define DLC_GOR_LSB	0
#define DLC_GOR_MASK	0x1f

#define __dlv_set_hp_volume(vol)					\
	do {								\
		int ___vol = ((vol) & DLC_GOL_MASK) | DLC_GCR_HPL_LRGO; \
		codec_write_reg(DLC_GCR_HPL, ___vol);			\
	} while (0)

#define __dlv_set_hp_volume_lr(lvol, rvol)		\
	do {						\
		int ___lvol = (lvol) & DLC_GOL_MASK;	\
		int ___rvol = (rvol) & DLC_GOR_MASK;	\
		codec_write_reg(DLC_GCR_HPL, ___lvol);	\
		codec_write_reg(DLC_GCR_HPR, ___rvol);	\
	} while(0)

//GCR_LIBY	L
#define DLC_GCR_LIBYL	0x2A
#define DLC_GCR_LIBYL_LRGI	(1 << 7)

#define DLC_GIL_LSB	0
#define DLC_GIL_MASK	0x1f

//GCR_LIBY	R
#define DLC_GCR_LIBYR	0x2B
#define DLC_GIR_LSB	0
#define DLC_GIR_MASK	0x1f

#define __dlv_set_line_in_bypass_volume(vol)				\
	do {								\
		int ___vol = (vol & DLC_GIL_MASK) | DLC_GCR_LIBYL_LRGI;	\
		codec_write_reg(DLC_GCR_LIBYL, ___vol);			\
	} while (0)

#define __dlv_set_line_in_bypass_volume_rl(lvol, rvol)	\
	do {						\
		int ___lvol = (lvol) & DLC_GIL_MASK;	\
		int ___rvol = (rvol) & DLC_GIR_MASK;	\
		codec_write_reg(DLC_GCR_LIBYL, ___lvol);	\
		codec_write_reg(DLC_GCR_LIBYR, ___rvol);	\
	} while (0)

//GCR_DAC	L
#define DLC_GCR_DACL	0x2C

#define DLC_GCR_RLGOD	(1 << 7)
#define DLC_GODL_LSB	0
#define DLC_GODL_MASK	0x1f

//GCR_DAC	R
#define DLC_GCR_DACR	0x2D

#define DLC_GODR_LSB	0
#define DLC_GODR_MASK	0x1f

#define __dlv_set_dac_gain(g)						\
	do {								\
		int ___g = ((g) & DLC_GODL_MASK) | DLC_GCR_RLGOD;	\
		codec_write_reg(DLC_GCR_DACL, ___g);			\
	} while(0)

#define __dlv_set_dac_gain_lr(lg, rg)			\
	do {						\
		int ___lg = (lg) & DLC_GODL_MASK;	\
		int ___rg = (rg) & DLC_GODR_MASK;	\
		codec_write_reg(DLC_GCR_DACL, ___lg);	\
		codec_write_reg(DLC_GCR_DACR, ___rg);	\
	} while(0)

//GCR_MIC1	
#define DLC_GCR_MIC1	0x2E

#define DLC_GIM1_LSB	0
#define DLC_GIM1_MASK	0x7

#define __dlv_set_mic1_boost(n) codec_update_reg(DLC_GCR_MIC1, (n), DLC_GIM1_MASK)

//GCR_MIC2
#define DLC_GCR_MIC2	0x2F
#define DLC_GIM2_LSB	0
#define DLC_GIM2_MASK	0x7

#define __dlv_set_mic2_boost(n) codec_update_reg(DLC_GCR_MIC2, (n), DLC_GIM2_MASK)

//GCR_ADC	L
#define DLC_GCR_ADCL	0x30

#define DLC_GCR_LRGID	(1 << 7)
#define DLC_GIDL_LSB	0
#define DLC_GIDL_MASK	0x3f


#define DLC_GCR_ADCR	0x31
#define DLC_GIDR_LSB	0
#define DLC_GIDR_MASK	0x3f

#define __dlv_set_adc_gain(g)						\
	do {								\
		int ___g = ((g) & DLC_GODL_MASK) | DLC_GCR_RLGOD;	\
		codec_write_reg(DLC_GCR_ADCL, ___g);			\
	} while(0)

#define __dlv_set_adc_gain_lr(lg, rg)			\
	do {						\
		int ___lg = (lg) & DLC_GODL_MASK;	\
		int ___rg = (rg) & DLC_GODR_MASK;	\
		codec_write_reg(DLC_GCR_ADCL, ___lg);	\
		codec_write_reg(DLC_GCR_ADCR, ___rg);	\
	} while(0)

//GCR_MIXDAC	L
#define DLC_GCR_MIX_DACL	0x34
#define DLC_GCR_GOMIXLR		(1 << 7)
#define DLC_GOMIXL_LSB		0
#define DLC_GOMIXL_MASK		0x1f

//GCR_MIXDAC	R
#define DLC_GCR_MIX_DACR	0x35
#define DLC_GOMIXR_LSB		0
#define DLC_GOMIXR_MASK		0x1f

#define __dlv_set_mix_dac_gain(g)					\
	do {                                                            \
		int ___g = ((g) & DLC_GOMIXL_MASK) | DLC_GCR_GOMIXLR;       \
		codec_write_reg(DLC_GCR_MIX_DACL, ___g);                      \
	} while(0)

#define __dlv_set_mix_adc_gain_lr(lg, rg)                   \
	do {                                            \
		int ___lg = (lg) & DLC_GOMIXL_MASK;       \
		int ___rg = (rg) & DLC_GOMIXR_MASK;       \
		codec_write_reg(DLC_GCR_MIX_DACL, ___lg);     \
		codec_write_reg(DLC_GCR_MIX_DACR, ___rg);     \
	} while(0)

//GCR_MIXADC	L
#define DLC_GCR_MIX_ADCL        0x36
#define DLC_GCR_GIMIXLR         (1 << 7)
#define DLC_GIMIXL_LSB          0
#define DLC_GIMIXL_MASK         0x1f

//GCR_MIXADC	R
#define DLC_GCR_MIX_ADCR        0x37
#define DLC_GIMIXR_LSB          0
#define DLC_GIMIXR_MASK         0x1f

#define __dlv_set_mixer_adc_gain(g)                                     \
	do {                                                            \
		int ___g = ((g) & DLC_GIMIXL_MASK) | DLC_GCR_GIMIXLR;       \
		codec_write_reg(DLC_GCR_MIX_ADCL, ___g);                      \
	} while(0)

#define __dlv_set_mixer_adc_gain_lr(lg, rg)                   \
	do {                                            \
		int ___lg = (lg) & DLC_GIMIXL_MASK;       \
		int ___rg = (rg) & DLC_GIMIXR_MASK;       \
		codec_write_reg(DLC_GCR_MIX_ADCL, ___lg);     \
		codec_write_reg(DLC_GCR_MIX_ADCR, ___rg);     \
	} while(0)


//CR_ADC_AGC
#define DLC_CR_ADC_AGC              0x3A
//DR_MIXER
#define DLC_DR_ADC_AGC	            0x3B

#define DLC_CR_AGC_ENA          (1 << 7)
#define DLC_CR_AGC_LOAD         (1 << 6)

#define DLC_CR_AGC_ADDR_LSB     0
#define DLC_CR_AGC_ADDR_MASK    (0x3F << 0)             

#define DLC_CR_AGC_0            (0x00 << 0)
#define DLC_CR_AGC_1            (0x01 << 0)
#define DLC_CR_AGC_2            (0x02 << 0)
#define DLC_CR_AGC_3            (0x03 << 0)
#define DLC_CR_AGC_4            (0x04 << 0)

#define __dlv_adc_agc_enable()    codec_write_reg_bit(DLC_CR_ADC_AGC, 1, DLC_CR_AGC_ENA)
#define __dlv_adc_agc_disable()   codec_write_reg_bit(DLC_CR_ADC_AGC, 0, DLC_CR_AGC_ENA)

#define __dlv_set_agc_load()      codec_write_reg_bit(DLC_CR_ADC_AGC, 1, DLC_CR_AGC_LOAD)
#define __dlv_clear_agc_load()    codec_write_reg_bit(DLC_CR_ADC_AGC, 0, DLC_CR_AGC_LOAD)

#define __dlv_set_agc_addr(addr)      codec_update_reg(DLC_CR_ADC_AGC, (addr << DLC_CR_AGC_ADDR_LSB), DLC_CR_AGC_ADDR_MASK)
#define __dlv_set_agc_data(data)      codec_write_reg(DLC_DR_ADC_AGC, data)


#define ADC_AGC_0		0

#define DLC_CR_AGC_STEREO	(1 << 6)
#define DLC_CR_AGC_TARGET_LSB	2
#define DLC_CR_AGC_TARGET_MASK	(0xf << 2)

/*
 * target =  b'0000 ~ b'1111 (-6dB ~ -28.5dB)
 * by step of 1.5 dB
 */
#define __dlv_set_agc_target(target)					\
	do {                                                            \
		int val = codec_read_agc_reg(ADC_AGC_0);		\
		val &= ~(DLC_CR_AGC_TARGET_MASK);			\
		val |= (target << DLC_CR_AGC_TARGET_LSB);					\
		codec_write_agc_reg(ADC_AGC_0, val);			\
	} while (0)

//1: left and right is not equel, 0: left and right is equel
#define __dlv_enable_agc_stereo()	codec_write_agc_reg_bit(ADC_AGC_0, 1, DLC_CR_AGC_STEREO)
#define __dlv_disable_agc_stereo()	codec_write_agc_reg_bit(ADC_AGC_0, 0, DLC_CR_AGC_STEREO)


#define ADC_AGC_1			1

#define AGC_NOISE_GATE_ENA		(1 << 7)
#define AGC_NOISE_GATE_VAL_LSB		4
#define AGC_NOISE_GATE_VAL_MASK		(0x7 << 4)	
#define AGC_NOISE_GATE_HOLD_LSB		0
#define AGC_NOISE_GATE_HOLD_MASK	(0xf << 0)

#define __dlv_agc_enable_noise_gate()	codec_write_agc_reg_bit(ADC_AGC_1, 1, AGC_NOISE_GATE_ENA)
#define __dlv_agc_disable_noise_gate()	codec_write_agc_reg_bit(ADC_AGC_1, 0, AGC_NOISE_GATE_ENA)

/*
 * noise gate value = b'000 ~ b'111 (-72dB ~ -30dB)
 * by step of 6dB
 */
#define __dlv_set_noise_gate_value(val)					\
	do {                                                            \
		int v = codec_read_agc_reg(ADC_AGC_1);			\
		v &= ~(AGC_NOISE_GATE_VAL_MASK);			\
		v |= (val << AGC_NOISE_GATE_VAL_LSB);						\
		codec_write_agc_reg(ADC_AGC_1, v);			\
	} while (0)

/*
 * hold time = b'0000 ~ b'1111 (0ms ~ 32.768s
 * Time Step x2
 */
#define __dlv_set_noise_gate_hold(val)					\
	do {                                                            \
		int v = codec_read_agc_reg(ADC_AGC_1);		\
		v &= ~(AGC_NOISE_GATE_HOLD_MASK);			\
		v |= (val << AGC_NOISE_GATE_HOLD_MASK);					\
		codec_write_agc_reg(ADC_AGC_1, v);			\
	} while (0)


#define	ADC_AGC_2			2

#define ADC_AGC_ATK_LSB			4
#define ADC_AGC_ATK_MASK		(0xf << 4)
#define ADC_AGC_DCY_LSB			0
#define ADC_AGC_DCY_MASK		(0xf << 0)

#define __dlv_set_agc_rampdown_time(time)				\
	do {                                                            \
		int v = codec_read_agc_reg(ADC_AGC_2);			\
		v &= ~(ADC_AGC_ATK_MASK);				\
		v |= (time << ADC_AGC_ATK_MASK);						\
		codec_write_agc_reg(ADC_AGC_2, v);			\
	} while (0)


#define __dlv_set_agc_rampup_time(time)					\
	do {                                                            \
		int v = codec_read_agc_reg(ADC_AGC_2);			\
		v &= ~(ADC_AGC_DCY_MASK);				\
		v |= (time << ADC_AGC_DCY_MASK);						\
		codec_write_agc_reg(ADC_AGC_2, v);			\
	} while (0)

#define ADC_AGC_3			3

#define ADC_AGC_MAX_GAIN_LSB		0
#define ADC_AGC_MAX_GAIN_MASK		(0xf << 0)

#define __dlv_set_agc_max_gain(gain)					\
	do {                                                            \
		int v = codec_read_agc_reg(ADC_AGC_3);			\
		v &= ~(ADC_AGC_MAX_GAIN_MASK);				\
		v |= (gain << ADC_AGC_MAX_GAIN_MASK);						\
		codec_write_agc_reg(ADC_AGC_3, v);			\
	} while (0)

#define ADC_AGC_4			4

#define ADC_AGC_MIN_GAIN_LSB		0
#define ADC_AGC_MIN_GAIN_MASK		(0xf << 0)

#define __dlv_set_agc_min_gain(gain)					\
	do {                                                            \
		int v = codec_read_agc_reg(ADC_AGC_4);			\
		v &= ~(ADC_AGC_MIN_GAIN_MASK);				\
		v |= (gain << ADC_AGC_MIN_GAIN_MASK);						\
		codec_write_agc_reg(ADC_AGC_4, v);			\
	} while (0)


#endif // __JZ4770_DLV_H__
