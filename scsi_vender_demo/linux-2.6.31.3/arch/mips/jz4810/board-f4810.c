/*
 * linux/arch/mips/jz4760/board-f4760.c
 *
 * JZ4760 F4760 board setup routines.
 *
 * Copyright (c) 2006-2008  Ingenic Semiconductor Inc.
 * Author: <jlwei@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <linux/init.h>
#include <linux/sched.h>
#include <linux/ioport.h>
#include <linux/mm.h>
#include <linux/console.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include <asm/cpu.h>
#include <asm/bootinfo.h>
#include <asm/mipsregs.h>
#include <asm/reboot.h>

#include <linux/mmc/host.h>
#include <asm/jzsoc.h>
extern int __init jz_add_msc_devices(unsigned int controller, struct jz_mmc_platform_data *plat);

#define WM831X_LDO_MAX_NAME 6

extern void (*jz_timer_callback)(void);

static void dancing(void)
{
	static unsigned char slash[] = "\\|/-";
//	static volatile unsigned char *p = (unsigned char *)0xb6000058;
	static volatile unsigned char *p = (unsigned char *)0xb6000016;
	static unsigned int count = 0;
	*p = slash[count++];
	count &= 3;
}

static void f4810_timer_callback(void)
{
	static unsigned long count = 0;

	if ((++count) % 50 == 0) {
		dancing();
		count = 0;
	}
}

static void __init board_cpm_setup(void)
{
	/* Stop unused module clocks here.
	 * We have started all module clocks at arch/mips/jz4760/setup.c.
	 */
}

static void __init board_gpio_setup(void)
{
	/*
	 * Initialize SDRAM pins
	 */
}

static struct i2c_board_info falcon_i2c0_devs[] __initdata = {
        {
                I2C_BOARD_INFO("cm3511", 0x30),
        },
        {
                I2C_BOARD_INFO("ov3640", 0x3c),
        },
        {
                I2C_BOARD_INFO("ov7690", 0x21),
        },
        {
        },
};

void __init board_i2c_init(void) {
        i2c_register_board_info(0, falcon_i2c0_devs, ARRAY_SIZE(falcon_i2c0_devs));
}

static void f4810_sd_gpio_init(struct device *dev)
{
	printk("sd_gpio_init\n");
	//__gpio_as_msc0_8bit();
	{
        REG_GPIO_PXINTC(0) = 0x00ec0000;        
        REG_GPIO_PXMASKC(0)  = 0x00ec0000;      
        REG_GPIO_PXPAT1C(0) = 0x00ec0000;       
        REG_GPIO_PXPAT0S(0) = 0x00ec0000;       
        REG_GPIO_PXINTC(0) = 0x00100000;        
        REG_GPIO_PXMASKC(0)  = 0x00100000;      
        REG_GPIO_PXPAT1C(0) = 0x00100000;       
        REG_GPIO_PXPAT0C(0) = 0x00100000;      
	}

	__gpio_as_output1(GPIO_SD0_VCC_EN_N);
	__gpio_as_input(GPIO_SD0_CD_N);
}

static void f4810_sd_power_on(struct device *dev)
{
	printk("sd_power_on");
	__msc0_enable_power();
}

static void f4810_sd_power_off(struct device *dev)
{
	printk("sd_power_off");
	__msc0_disable_power();
}

static void f4810_sd_cpm_start(struct device *dev)
{
//	cpm_start_clock(CGM_MSC0);
}

static unsigned int f4810_sd_status(struct device *dev)
{
	unsigned int status;

	return 1;
	status = (unsigned int) __gpio_get_pin(GPIO_SD0_CD_N);
#if ACTIVE_LOW_MSC0_CD
	return !status;
#else
	return status;
#endif
}

static void f4810_sd_plug_change(int state)
{
#if 0
	if(state == CARD_INSERTED) /* wait for remove */
#if ACTIVE_LOW_MSC0_CD
		__gpio_as_irq_high_level(MSC0_HOTPLUG_PIN);
#else
		__gpio_as_irq_low_level(MSC0_HOTPLUG_PIN);
#endif
	else		      /* wait for insert */
#if ACTIVE_LOW_MSC0_CD
		__gpio_as_irq_low_level(MSC0_HOTPLUG_PIN);
#else
		__gpio_as_irq_high_level(MSC0_HOTPLUG_PIN);
#endif
#endif
}

static unsigned int f4810_sd_get_wp(struct device *dev)
{
	unsigned int status;

	status = (unsigned int) __gpio_get_pin(MSC0_WP_PIN);
	return (status);
}

struct jz_mmc_platform_data f4810_sd_data = {
#ifndef CONFIG_JZ_MSC0_SDIO_SUPPORT
	.support_sdio   = 0,
#else
	.support_sdio   = 1,
#endif
	.ocr_mask	= MMC_VDD_32_33 | MMC_VDD_33_34,
	.status_irq	= 0,//MSC0_HOTPLUG_IRQ,
	.detect_pin     = 0,//GPIO_SD0_CD_N,
	.init           = f4810_sd_gpio_init,
	.power_on       = f4810_sd_power_on,
	.power_off      = f4810_sd_power_off,
	.cpm_start	= f4810_sd_cpm_start,
	.status		= f4810_sd_status,
	.plug_change	= f4810_sd_plug_change,
	.write_protect  = f4810_sd_get_wp,
	.max_bus_width  = MMC_CAP_MMC_HIGHSPEED | MMC_CAP_SD_HIGHSPEED ,//| MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_JZ_MSC0_BUS_1
	.bus_width      = 1,
#elif defined  CONFIG_JZ_MSC0_BUS_4
	.bus_width      = 4,
#else
	.bus_width      = 8,
#endif
};

static void f4810_tf_gpio_init(struct device *dev)
{
	__gpio_as_msc1_8bit();
//	__gpio_as_output1(GPIO_SD1_VCC_EN_N);
//	__gpio_as_input(GPIO_SD1_CD_N);
}

static void f4810_tf_power_on(struct device *dev)
{
//	__msc1_enable_power();
}

static void f4810_tf_power_off(struct device *dev)
{
//	__msc1_disable_power();
}

static void f4810_tf_cpm_start(struct device *dev)
{
//	cpm_start_clock(CGM_MSC1);
}

static unsigned int f4810_tf_status(struct device *dev)
{
	return 1;
#if 0
	unsigned int status = 0;
	status = (unsigned int) __gpio_get_pin(GPIO_SD1_CD_N);
#if ACTIVE_LOW_MSC1_CD
	return !status;
#else
	return status;
#endif
#endif
}

static void f4810_tf_plug_change(int state)
{
#if 0
	if(state == CARD_INSERTED) /* wait for remove */
#if ACTIVE_LOW_MSC1_CD
		__gpio_as_irq_high_level(MSC1_HOTPLUG_PIN);
#else
		__gpio_as_irq_low_level(MSC1_HOTPLUG_PIN);
#endif
	else		      /* wait for insert */
#if ACTIVE_LOW_MSC1_CD
		__gpio_as_irq_low_level(MSC1_HOTPLUG_PIN);
#else
		__gpio_as_irq_high_level(MSC1_HOTPLUG_PIN);
#endif
#endif
}

struct jz_mmc_platform_data f4810_tf_data = {
#ifndef CONFIG_JZ_MSC1_SDIO_SUPPORT
	.support_sdio   = 0,
#else
	.support_sdio   = 1,
#endif
	.ocr_mask	= MMC_VDD_32_33 | MMC_VDD_33_34,
	.status_irq	= 0,//MSC1_HOTPLUG_IRQ,
	.detect_pin     = 0,//GPIO_SD1_CD_N,
	.init           = f4810_tf_gpio_init,
	.power_on       = f4810_tf_power_on,
	.power_off      = f4810_tf_power_off,
	.cpm_start	= f4810_tf_cpm_start,
	.status		= f4810_tf_status,
	.plug_change	= f4810_tf_plug_change,
	.max_bus_width  = MMC_CAP_SD_HIGHSPEED | MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_JZ_MSC1_BUS_1
	.bus_width      = 1,
#else
	.bus_width      = 4,
#endif
};

static void f4810_msc2_gpio_init(struct device *dev)
{
	//__gpio_as_msc2_8bit();
	{
        REG_GPIO_PXINTC(1) = 0xf0300000;        
        REG_GPIO_PXMASKC(1) = 0xf0300000;       
        REG_GPIO_PXPAT0C(1) = 0xf0300000;       
        REG_GPIO_PXPAT1C(1)  = 0xf0300000;      
	}
	return;
}

static void f4810_msc2_power_on(struct device *dev)
{
	return;
}

static void f4810_msc2_power_off(struct device *dev)
{
	return;
}

static void f4810_msc2_cpm_start(struct device *dev)
{
//	cpm_start_clock(CGM_MSC2);
}

static unsigned int f4810_msc2_status(struct device *dev)
{
	return 1;	      /* default: no card */
}

static void f4810_msc2_plug_change(int state)
{
	return;
}

struct jz_mmc_platform_data f4810_msc2_data = {
#ifndef CONFIG_JZ_MSC2_SDIO_SUPPORT
	.support_sdio   = 0,
#else
	.support_sdio   = 1,
#endif
	.ocr_mask	= MMC_VDD_32_33 | MMC_VDD_33_34,
	.status_irq	= 0, //MSC1_HOTPLUG_IRQ,
	.detect_pin     = 0, //GPIO_SD1_CD_N,
	.init           = f4810_msc2_gpio_init,
	.power_on       = f4810_msc2_power_on,
	.power_off      = f4810_msc2_power_off,
	.cpm_start	= f4810_msc2_cpm_start,
	.status		= f4810_msc2_status,
	.plug_change	= f4810_msc2_plug_change,
	.max_bus_width  = MMC_CAP_SD_HIGHSPEED | MMC_CAP_4_BIT_DATA,
#ifdef CONFIG_JZ_MSC2_BUS_1
	.bus_width      = 1,
#else
	.bus_width      = 4,
#endif
};

void __init board_msc_init(void)
{
	jz_add_msc_devices(0, &f4810_sd_data);
	jz_add_msc_devices(1, &f4810_tf_data);
	jz_add_msc_devices(2, &f4810_msc2_data);
	printk("add msc2\n");
}

void __init jz_board_setup(void)
{

	printk("JZ4810 F4810 board setup\n");
//	jz_restart(NULL);
	board_cpm_setup();
	board_gpio_setup();

	jz_timer_callback = f4810_timer_callback;
}

/**
 * Called by arch/mips/kernel/proc.c when 'cat /proc/cpuinfo'.
 * Android requires the 'Hardware:' field in cpuinfo to setup the init.%hardware%.rc.
 */
const char *get_board_type(void)
{
	return "f4810";
}

/*****
 * Wm831x init
 *****/
