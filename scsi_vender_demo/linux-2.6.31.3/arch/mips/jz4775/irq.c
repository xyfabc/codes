/*
 * linux/arch/mips/jz4775/irq.c
 *
 * JZ4775 interrupt routines.
 *
 * Copyright (c) 2006-2007  Ingenic Semiconductor Inc.
 * Author: <lhhuang@ingenic.cn>
 *
 *  This program is free software; you can redistribute	 it and/or modify it
 *  under  the terms of	 the GNU General  Public License as published by the
 *  Free Software Foundation;  either version 2 of the	License, or (at your
 *  option) any later version.
 */
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/irq.h>
#include <linux/kernel_stat.h>
#include <linux/module.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/interrupt.h>
#include <linux/ioport.h>
#include <linux/timex.h>
#include <linux/slab.h>
#include <linux/random.h>
#include <linux/delay.h>
#include <linux/bitops.h>

#include <asm/bootinfo.h>
#include <asm/io.h>
#include <asm/mipsregs.h>
#include <asm/system.h>
#include <asm/jzsoc.h>

/*
 * INTC irq type
 */

static void enable_intc_irq(unsigned int irq)
{
	__intc_unmask_irq(irq);
}

static void disable_intc_irq(unsigned int irq)
{
	__intc_mask_irq(irq);
}

static void mask_and_ack_intc_irq(unsigned int irq)
{
	__intc_mask_irq(irq);
	__intc_ack_irq(irq);
}

static void end_intc_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS))) {
		enable_intc_irq(irq);
	}
}

static unsigned int startup_intc_irq(unsigned int irq)
{
	enable_intc_irq(irq);
	return 0;
}

static void shutdown_intc_irq(unsigned int irq)
{
	disable_intc_irq(irq);
}

static struct irq_chip intc_irq_type = {
	.typename = "INTC",
	.startup = startup_intc_irq,
	.shutdown = shutdown_intc_irq,
	.unmask = enable_intc_irq,
	.mask = disable_intc_irq,
	.ack = mask_and_ack_intc_irq,
	.end = end_intc_irq,
};

/*
 * GPIO irq type
 */

static void enable_gpio_irq(unsigned int irq)
{
	unsigned int intc_irq;

	if (irq < (IRQ_GPIO_0 + 32)) {
		intc_irq = IRQ_GPIO0;
	}
	else if (irq < (IRQ_GPIO_0 + 64)) {
		intc_irq = IRQ_GPIO1;
	}
	else if (irq < (IRQ_GPIO_0 + 96)) {
		intc_irq = IRQ_GPIO2;
	}
	else if (irq < (IRQ_GPIO_0 + 128)) {
		intc_irq = IRQ_GPIO3;
	}
	else if (irq < (IRQ_GPIO_0 + 160)) {
		intc_irq = IRQ_GPIO4;
	}
	else {
		intc_irq = IRQ_GPIO5;
	}

	enable_intc_irq(intc_irq);
	__gpio_unmask_irq(irq - IRQ_GPIO_0);
}

static void disable_gpio_irq(unsigned int irq)
{
	__gpio_mask_irq(irq - IRQ_GPIO_0);
}

static void mask_and_ack_gpio_irq(unsigned int irq)
{
	__gpio_mask_irq(irq - IRQ_GPIO_0);
	__gpio_ack_irq(irq - IRQ_GPIO_0);
}

static void end_gpio_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS))) {
		enable_gpio_irq(irq);
	}
}

static unsigned int startup_gpio_irq(unsigned int irq)
{
	enable_gpio_irq(irq);
	return 0;
}

static void shutdown_gpio_irq(unsigned int irq)
{
	disable_gpio_irq(irq);
}

static struct irq_chip gpio_irq_type = {
	.typename = "GPIO",
	.startup = startup_gpio_irq,
	.shutdown = shutdown_gpio_irq,
	.unmask = enable_gpio_irq,
	.mask = disable_gpio_irq,
	.ack = mask_and_ack_gpio_irq,
	.end = end_gpio_irq,
};

/*
 * DMA irq type
 */
static void enable_dma_irq(unsigned int irq)
{
	unsigned int intc_irq;

	intc_irq = IRQ_PDMA;
	__intc_unmask_irq(intc_irq);
	__dmac_channel_enable_irq(irq - IRQ_DMA_0);
}

static void disable_dma_irq(unsigned int irq)
{
	int chan = irq - IRQ_DMA_0;
	__dmac_disable_channel(chan);
	__dmac_channel_disable_irq(chan);
}

static void mask_and_ack_dma_irq(unsigned int irq)
{
	unsigned int intc_irq;

	disable_dma_irq(irq);
	intc_irq = IRQ_PDMA;
	__intc_ack_irq(intc_irq);

	//__dmac_channel_ack_irq(irq-IRQ_DMA_0); /* needed?? add 20080506, Wolfgang */
	//__dmac_channel_disable_irq(irq - IRQ_DMA_0);
}

static void end_dma_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS))) {
		enable_dma_irq(irq);
	}
}

static unsigned int startup_dma_irq(unsigned int irq)
{
	enable_dma_irq(irq);
	return 0;
}

static void shutdown_dma_irq(unsigned int irq)
{
	disable_dma_irq(irq);
}

static struct irq_chip dma_irq_type = {
	.typename = "DMA",
	.startup = startup_dma_irq,
	.shutdown = shutdown_dma_irq,
	.unmask = enable_dma_irq,
	.mask = disable_dma_irq,
	.ack = mask_and_ack_dma_irq,
	.end = end_dma_irq,
};

/*
 * MCU(inside PDMA) irq type
 */
static void enable_mcu_irq(unsigned int irq)
{
	unsigned int intc_irq;

	intc_irq = IRQ_PDMAM;
	__intc_unmask_irq(intc_irq);
	__dmac_mmb_unmask_irq(irq - IRQ_MCU_0);
}

static void disable_mcu_irq(unsigned int irq)
{
	__dmac_mmb_mask_irq(irq - IRQ_MCU_0);
}

static void mask_and_ack_mcu_irq(unsigned int irq)
{
	__dmac_mmb_mask_irq(irq - IRQ_MCU_0);
	__dmac_mmb_ack_irq(irq - IRQ_MCU_0);
}

static void end_mcu_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED|IRQ_INPROGRESS))) {
		enable_mcu_irq(irq);
	}
}

static unsigned int startup_mcu_irq(unsigned int irq)
{
	enable_mcu_irq(irq);
	return 0;
}

static void shutdown_mcu_irq(unsigned int irq)
{
	disable_mcu_irq(irq);
}

static struct irq_chip mcu_irq_type = {
	.typename = "MCU",
	.startup = startup_mcu_irq,
	.shutdown = shutdown_mcu_irq,
	.unmask = enable_mcu_irq,
	.mask = disable_mcu_irq,
	.ack = mask_and_ack_mcu_irq,
	.end = end_mcu_irq,
};

//-------------------------------------------------------------------
/*
 * SYSOST irq type
 */
static void enable_sysost_irq(unsigned int irq)
{
	__sysost_unmask_irq(irq - IRQ_SYSOST_0);
}

static void disable_sysost_irq(unsigned int irq)
{
	__sysost_mask_irq(irq - IRQ_SYSOST_0);
}

static void mask_and_ack_sysost_irq(unsigned int irq)
{
	__sysost_mask_irq(irq - IRQ_SYSOST_0);
	__sysost_ack_irq(irq - IRQ_SYSOST_0);
}

static void end_sysost_irq(unsigned int irq)
{
	if (!(irq_desc[irq].status & (IRQ_DISABLED | IRQ_INPROGRESS))) {
		enable_sysost_irq(irq);
	}
}

static unsigned int startup_sysost_irq(unsigned int irq)
{
	enable_sysost_irq(irq);
	return 0;
}

static void shutdown_sysost_irq(unsigned int irq)
{
	disable_sysost_irq(irq);
}

static struct irq_chip sysost_irq_type = {
	.typename = "SYSOST",
	.startup = startup_sysost_irq,
	.shutdown = shutdown_sysost_irq,
	.unmask = enable_sysost_irq,
	.mask = disable_sysost_irq,
	.ack = mask_and_ack_sysost_irq,
	.end = end_sysost_irq,
};

//----------------------------------------------------------------------

void __init arch_init_irq(void)
{
	int i;

	clear_c0_status(0xff04); /* clear ERL */
	set_c0_status(0x1400);   /* set IP2 IP4 */	

	/* Set up INTC irq
	 */
//	printk("%s, %d\n",__func__,__LINE__);
	for (i = 0; i < NUM_INTC; i++) {
		disable_intc_irq(i);
		set_irq_chip_and_handler(i, &intc_irq_type, handle_level_irq);
	}


	/* Set up DMAC irq
	 */
//	printk("%s, %d\n",__func__,__LINE__);
	for (i = 0; i < NUM_DMA; i++) {
		disable_dma_irq(IRQ_DMA_0 + i);
		set_irq_chip_and_handler(IRQ_DMA_0 + i, &dma_irq_type, handle_level_irq);
	}

	/* Set up GPIO irq
	 */
//	printk("%s, %d\n",__func__,__LINE__);
#ifndef JZ_BOOTUP_UART_TXD
#error "JZ_BOOTUP_UART_TXD is not set, please define it int your board header file!"
#endif
#ifndef JZ_BOOTUP_UART_RXD
#error "JZ_BOOTUP_UART_RXD is not set, please define it int your board header file!"
#endif
	for (i = 0; i < NUM_GPIO; i++) {
		if (unlikely(i == 31))
			continue;
		if (unlikely(i == 30))
			continue;
#if 0
		if (unlikely((i > 64) && (i < 96)))			// add, yjt, 20130823. 
			continue;
#endif
		if (unlikely(i == JZ_BOOTUP_UART_TXD))
			continue;
		if (unlikely(i == JZ_BOOTUP_UART_RXD))
			continue;
		disable_gpio_irq(IRQ_GPIO_0 + i);
		set_irq_chip_and_handler(IRQ_GPIO_0 + i, &gpio_irq_type, handle_level_irq);
	}
	

	/* 
	 * Set up MCU irq
	 */
//	printk("%s, %d\n",__func__,__LINE__);
	for (i = 0; i < NUM_MCU; i++) {
		disable_mcu_irq(IRQ_MCU_0 + i);
		set_irq_chip_and_handler(IRQ_MCU_0 + i, &mcu_irq_type, handle_level_irq);
	}
	
	

	/* 
	 * Set up OST irq
	 */
//	printk("%s, %d\n",__func__,__LINE__);
	for (i = 0; i < NUM_SYSOST; i++) {
		disable_sysost_irq(IRQ_SYSOST_0 + i);
		set_irq_chip_and_handler(IRQ_SYSOST_0 + i, &sysost_irq_type, handle_level_irq);
	}
	
//	printk("%s, %d\n",__func__,__LINE__);
}

static int plat_real_irq(int irq)
{
	int group = 0;

	if ((irq >= IRQ_GPIO5) && (irq <= IRQ_GPIO0)) {
		group = IRQ_GPIO0 - irq;
		irq = __gpio_group_irq(group);
		if (irq >= 0)
			irq += IRQ_GPIO_0 + 32 * group;
	} else {
		switch (irq) {
		case IRQ_PDMA:
			irq = __dmac_get_irq();
			if (irq < 0) {
				printk("REG_DMAC_DMAIPR = 0x%08x\n", REG_DMAC_DMAIPR);
				return irq;
			}
			irq += IRQ_DMA_0;
			break;
		case IRQ_PDMAM:
			irq = __mcu_get_irq();
			if (irq < 0) {
				printk("REG_DMAC_DMINT = 0x%08x\n", REG_DMAC_DMINT);
				return irq;
			}
			irq += IRQ_MCU_0;
			break;
		}
	}

	return irq;
}

asmlinkage void plat_irq_dispatch(void)
{
	int irq = 0;
	u32 cause = 0;
	unsigned long intc_ipr0 = 0, intc_ipr1 = 0;
	unsigned long sysost_ipr = 0;

	cause = read_c0_cause();

/* here, we must handle IP4 priority, because ost interrupt result IP2 peding too.  
	
	intc_irq ---------------------------|
					    |
	ost_irq -----------|-------------|  |
			   |	    	 |  |
			   |	 	 |  |
		--------------------------------------
			| 12   |  11   |  10   |
		--------------------------------------		
			 IP4	 IP3	 IP2

			cp0_cause
*/

	if ((cause >> 12) & 0x1){		
		sysost_ipr = __sysost_get_irq() & (~__sysost_get_mask());
		irq = ffs(sysost_ipr) - 1;
		irq += IRQ_SYSOST_0;
		__asm__ __volatile__("nop;nop;");

	} else if ((cause >> 10) & 0x1) {		//hard int 0
		intc_ipr0 = REG_INTC_IPR(0);
		intc_ipr1 = REG_INTC_IPR(1);

		if (!(intc_ipr0 || intc_ipr1))	
			return;

		if (intc_ipr0) {
			irq = ffs(intc_ipr0) - 1;
			intc_ipr0 &= ~(1<<irq);
		} else {
			irq = ffs(intc_ipr1) - 1;
			intc_ipr1 &= ~(1<<irq);
			irq += 32;
		}

		irq = plat_real_irq(irq);
		WARN((irq < 0), "irq raised, but no irq pending!\n");
		if (irq < 0)
			return;
	}
	else
		return;

	do_IRQ(irq);
}
