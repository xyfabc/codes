/*
 *  linux/include/asm-mips/mach-jz4780/jz4780.h
 *
 *  JZ4780 common definition.
 *
 *  Copyright (C) 2008 Ingenic Semiconductor Inc.
 *
 *  Author: <cwjia@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_JZ4780_H__
#define __ASM_JZ4780_H__

/*------------------------------------------------------------------
 * Platform definitions
 */

#define JZ_SOC_NAME "JZ4780"

#ifdef CONFIG_JZ4780_F4780
#include <asm/mach-jz4780/board-f4780.h>
#endif

#ifdef CONFIG_JZ4780_NORMA
#include <asm/mach-jz4780/board-norma.h>
#endif

#ifdef CONFIG_JZ4780_NP_T706
#include <asm/mach-jz4780/board-np-t706.h>
#endif

/* Add other platform definition here ... */

/*------------------------------------------------------------------
 *  Register definitions
 */
#include <asm/mach-jz4780/regs.h>

#include <asm/mach-jz4780/jz4780misc.h>
#include <asm/mach-jz4780/jz4780gpio.h>
#include <asm/mach-jz4780/jz4780intc.h>
#include <asm/mach-jz4780/jz4780aic.h>
#include <asm/mach-jz4780/jz4780bch.h>
#include <asm/mach-jz4780/jz4780cim.h>
#include <asm/mach-jz4780/jz4780cpm.h>
#include <asm/mach-jz4780/jz4780ddrc.h>
#include <asm/mach-jz4780/jz4780emc.h>
#include <asm/mach-jz4780/jz4780i2c.h>
#include <asm/mach-jz4780/jz4780ipu.h>
#include <asm/mach-jz4780/jz4780lcdc.h>
#include <asm/mach-jz4780/jz4780mc.h>
#include <asm/mach-jz4780/jz4780me.h>
#include <asm/mach-jz4780/jz4780msc.h>
#include <asm/mach-jz4780/jz4780nemc.h>
#include <asm/mach-jz4780/jz4780otg.h>
#include <asm/mach-jz4780/jz4780otp.h>
#include <asm/mach-jz4780/jz4780owi.h>
#include <asm/mach-jz4780/jz4780ost.h>
#include <asm/mach-jz4780/jz4780sysost.h>
#include <asm/mach-jz4780/jz4780pcm.h>
#include <asm/mach-jz4780/jz4780pdma.h>
#include <asm/mach-jz4780/jz4780rtc.h>
#include <asm/mach-jz4780/jz4780sadc.h>
#include <asm/mach-jz4780/jz4780scc.h>
#include <asm/mach-jz4780/jz4780ssi.h>
#include <asm/mach-jz4780/jz4780tcu.h>
#include <asm/mach-jz4780/jz4780tssi.h>
#include <asm/mach-jz4780/jz4780efuse.h>
#include <asm/mach-jz4780/jz4780tve.h>
#include <asm/mach-jz4780/jz4780uart.h>
#include <asm/mach-jz4780/jz4780wdt.h>
#include <asm/mach-jz4780/jz4780aosd.h>
#include <asm/mach-jz4780/jz4780lvds.h>
//#include <asm/mach-jz4780/jz4780mcu.h>

#include <asm/mach-jz4780/dma.h>
#include <asm/mach-jz4780/message.h>
#include <asm/mach-jz4780/misc.h>
#include <asm/mach-jz4780/platform.h>

/*------------------------------------------------------------------
 * Follows are related to platform definitions
 */

#include <asm/mach-jz4780/serial.h>

#endif /* __ASM_JZ4780_H__ */
