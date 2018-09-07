/*
 *  linux/include/asm-mips/mach-jz4775/jz4775.h
 *
 *  JZ4775 common definition.
 *
 *  Copyright (C) 2008 Ingenic Semiconductor Inc.
 *
 *  Author: <cwjia@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifndef __ASM_JZ4775_H__
#define __ASM_JZ4775_H__

/*------------------------------------------------------------------
 * Platform definitions
 */

#define JZ_SOC_NAME "JZ4775"

#ifdef CONFIG_JZ4775_F4775
#include <asm/mach-jz4775/board-f4775.h>
#endif

#ifdef CONFIG_JZ4775_ORION
#include <asm/mach-jz4775/board-orion.h>
#endif

#ifdef CONFIG_JZ4775_MENSA
#include <asm/mach-jz4775/board-mensa.h>
#endif

/* Add other platform definition here ... */

/*------------------------------------------------------------------
 *  Register definitions
 */
#include <asm/mach-jz4775/regs.h>

#include <asm/mach-jz4775/jz4775misc.h>
#include <asm/mach-jz4775/jz4775gpio.h>
#include <asm/mach-jz4775/jz4775intc.h>
#include <asm/mach-jz4775/jz4775aic.h>
#include <asm/mach-jz4775/jz4775bch.h>
#include <asm/mach-jz4775/jz4775cim.h>
#include <asm/mach-jz4775/jz4775cpm.h>
#include <asm/mach-jz4775/jz4775ddrc.h>
#include <asm/mach-jz4775/jz4775emc.h>
#include <asm/mach-jz4775/jz4775i2c.h>
#include <asm/mach-jz4775/jz4775ipu.h>
#include <asm/mach-jz4775/jz4775lcdc.h>
#include <asm/mach-jz4775/jz4775mc.h>
#include <asm/mach-jz4775/jz4775me.h>
#include <asm/mach-jz4775/jz4775msc.h>
#include <asm/mach-jz4775/jz4775nemc.h>
#include <asm/mach-jz4775/jz4775otg.h>
#include <asm/mach-jz4775/jz4775otp.h>
#include <asm/mach-jz4775/jz4775owi.h>
#include <asm/mach-jz4775/jz4775ost.h>
#include <asm/mach-jz4775/jz4775sysost.h>
#include <asm/mach-jz4775/jz4775pcm.h>
#include <asm/mach-jz4775/jz4775pdma.h>
#include <asm/mach-jz4775/jz4775rtc.h>
#include <asm/mach-jz4775/jz4775sadc.h>
#include <asm/mach-jz4775/jz4775scc.h>
#include <asm/mach-jz4775/jz4775ssi.h>
#include <asm/mach-jz4775/jz4775tcu.h>
#include <asm/mach-jz4775/jz4775tssi.h>
#include <asm/mach-jz4775/jz4775efuse.h>
#include <asm/mach-jz4775/jz4775tve.h>
#include <asm/mach-jz4775/jz4775uart.h>
#include <asm/mach-jz4775/jz4775wdt.h>
#include <asm/mach-jz4775/jz4775aosd.h>
#include <asm/mach-jz4775/jz4775lvds.h>
//#include <asm/mach-jz4775/jz4775mcu.h>

#include <asm/mach-jz4775/dma.h>
#include <asm/mach-jz4775/message.h>
#include <asm/mach-jz4775/misc.h>
#include <asm/mach-jz4775/platform.h>

/*------------------------------------------------------------------
 * Follows are related to platform definitions
 */

#include <asm/mach-jz4775/serial.h>

#endif /* __ASM_JZ4775_H__ */
