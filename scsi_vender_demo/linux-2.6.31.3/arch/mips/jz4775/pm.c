/*
 * linux/arch/mips/jz4775/pm.c
 *
 * JZ4775 Power Management Routines
 *
 * Copyright (C) 2006 - 2010 Ingenic Semiconductor Inc.
 *
 *  This program is free software; you can distribute it and/or modify it
 *  under the terms of the GNU General Public License (Version 2) as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope it will be useful, but WITHOUT
 *  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 *  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 *  for more details.
 *
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  59 Temple Place - Suite 330, Boston MA 02111-1307, USA.
 *
 */

#include <linux/init.h>
#include <linux/pm.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/suspend.h>
#include <linux/proc_fs.h>
#include <linux/sysctl.h>

#include <asm/cacheops.h>
#include <asm/jzsoc.h>

#define CONFIG_PM_POWERDOWN_P0 y
#define JZ_PM_SIMULATE_BATTERY y
//#define CONFIG_RTC_DRV_JZ4775 y
#undef CONFIG_RTC_DRV_JZ4775



#ifdef JZ_PM_SIMULATE_BATTERY
#define CONFIG_BATTERY_JZ
#define JZ_PM_BATTERY_SIMED
#endif


#if defined(CONFIG_RTC_DRV_JZ4775) && defined(CONFIG_BATTERY_JZ)
//extern unsigned long jz_read_bat(void);
//extern int g_jz_battery_min_voltage;
static unsigned int usr_alarm_data = 0;
static int alarm_state = 0;
#endif

#undef DEBUG
//#define DEBUG
#ifdef DEBUG
#define dprintk(x...)	printk(x)
#else
#define dprintk(x...)
#endif

extern void jz_board_do_sleep(unsigned long *ptr);
extern void jz_board_do_resume(unsigned long *ptr);
#if defined(CONFIG_PM_POWERDOWN_P0)
extern void jz_cpu_sleep(void);
extern void jz_cpu_resume(void);
#endif
#if defined(CONFIG_INPUT_WM831X_ON)
extern void wm8310_power_off(void);
#endif
int jz_pm_do_hibernate(void)
{
	printk("Put CPU into hibernate mode.\n");
#if defined(CONFIG_INPUT_WM831X_ON)
	printk("The power will be off.\n");
	wm8310_power_off();
	while(1);
#else

	/* Mask all interrupts */
	OUTREG32(INTC_ICMSR(0), 0xffffffff);
	OUTREG32(INTC_ICMSR(1), 0x7ff);

	/*
	 * RTC Wakeup or 1Hz interrupt can be enabled or disabled
	 * through  RTC driver's ioctl (linux/driver/char/rtc_jz.c).
	 */

	/* Set minimum wakeup_n pin low-level assertion time for wakeup: 100ms */
	rtc_write_reg(RTC_HWFCR, HWFCR_WAIT_TIME(100));

	/* Set reset pin low-level assertion time after wakeup: must  > 60ms */
	rtc_write_reg(RTC_HRCR, HRCR_WAIT_TIME(60));

	/* Scratch pad register to be reserved */
	rtc_write_reg(RTC_HSPR, HSPR_RTCV);

	/* clear wakeup status register */
	rtc_write_reg(RTC_HWRSR, 0x0);

	/* set wake up valid level as low  and disable rtc alarm wake up.*/
        rtc_write_reg(RTC_HWCR,0x8);

	/* Put CPU to hibernate mode */
	rtc_write_reg(RTC_HCR, RTC_HCR_PD);

	while (1) {
		printk("We should NOT come here, please check the jz4775rtc.h!!!\n");
	};
#endif

	/* We can't get here */
	return 0;
}

#if defined(CONFIG_RTC_DRV_JZ4775) && defined(CONFIG_BATTERY_JZ)
static int alarm_remain = 0;
//#define ALARM_TIME (1 * 60)
#define ALARM_TIME (1)
static inline void jz_save_alarm(void) {
	uint32_t rtc_rtcsr = 0,rtc_rtcsar = 0;

	rtc_rtcsar = rtc_read_reg(RTC_RTCSAR); /* second alarm register */
	rtc_rtcsr = rtc_read_reg(RTC_RTCSR); /* second register */

	alarm_remain = rtc_rtcsar - rtc_rtcsr;
}

static inline void jz_restore_alarm(void) {
	if (alarm_remain > 0) {
		rtc_write_reg(RTC_RTCSAR, rtc_read_reg(RTC_RTCSR) + alarm_remain);
		rtc_set_reg(RTC_RTCCR,0x3<<2); /* alarm enable, alarm interrupt enable */
	}
}
unsigned int * resume_times = NULL;
static void jz_set_nc1hz(void)
{
	*resume_times += 1;
	printk("test resume times is %d\n", *resume_times);
	rtc_write_reg(RTC_RTCGR, 9829);
	rtc_set_reg(RTC_RTCCR, 1 << 5);
}

static void jz_set_alarm(void)
{
	uint32_t rtc_rtcsr = 0,rtc_rtcsar = 0;

	rtc_rtcsar = rtc_read_reg(RTC_RTCSAR); /* second alarm register */
	rtc_rtcsr = rtc_read_reg(RTC_RTCSR); /* second register */
#if 0
	if(rtc_rtcsar <= rtc_rtcsr) {
#endif
		printk("1\n");
		rtc_write_reg(RTC_RTCSAR,rtc_rtcsr + ALARM_TIME);
		rtc_set_reg(RTC_RTCCR,0x3<<2); /* alarm enable, alarm interrupt enable */
		alarm_state = 1;	       /* alarm on */

	rtc_rtcsar = rtc_read_reg(RTC_RTCSAR); /* second alarm register */
	rtc_rtcsr = rtc_read_reg(RTC_RTCSR); /* second register */

	printk("%s: rtc_rtcsar = %u rtc_rtcsr = %u alarm_state = %d\n", __func__, rtc_rtcsar, rtc_rtcsr, alarm_state);
}
#undef ALARM_TIME
#endif

#define DIV(a,b,c,d,e)					\
	({							\
	 unsigned int retval;					\
	 retval = ((a - 1) << CPCCR_CDIV_LSB)   |		\
	 ((b - 1) << CPCCR_L2DIV_LSB)  		|		\
	 ((c - 1) << CPCCR_H0DIV_LSB)  		|		\
	 ((d - 1) << CPCCR_H2DIV_LSB)  		|		\
	 ((e - 1) << CPCCR_PDIV_LSB);				\
	 retval;						\
	 })

#define cache_prefetch(label)						\
do{									\
	unsigned long addr,size,end;					\
	/* Prefetch codes from label */					\
	addr = (unsigned long)(&&label) & ~(32 - 1);			\
	size = 32 * 256; /* load 128 cachelines */			\
	end = addr + size;						\
	for (; addr < end; addr += 32) {				\
		__asm__ volatile (					\
				".set mips32\n\t"			\
				" cache %0, 0(%1)\n\t"			\
				".set mips32\n\t"			\
				:					\
				: "I" (Index_Prefetch_I), "r"(addr));	\
	}								\
}									\
while(0)

unsigned int jz_set_div(unsigned int div)
{
	unsigned int cpccr;
	unsigned int retval;

	cpccr = REG_CPM_CPCCR;
	retval = cpccr;
	cpccr = (cpccr & BITS_H2L(31, CPCCR_SEL_H2PLL_LSB)) | CPCCR_CE | div;
	cache_prefetch(jz_set_div_L1);
jz_set_div_L1:
	//REG_CPM_CPPSR = 0x1;
	REG_CPM_CPCCR = cpccr;
	while(REG_CPM_CPCSR & CPCSR_DIV_BUSY) ;

	return retval;
}

int jz_pm_do_sleep(void)
{
	unsigned int div;
	unsigned long delta;
	unsigned long nfcsr = REG_NEMC_NFCSR;
	unsigned long opcr = INREG32(CPM_OPCR);
	unsigned long icmr0 = INREG32(INTC_ICMR(0));
	unsigned long icmr1 = INREG32(INTC_ICMR(1));
	unsigned long sadc = REG_SADC_ENA;
	unsigned long sleep_gpio_save[6 * GPIO_PORT_NUM];
	unsigned long cpuflags;
	unsigned long addr, size, end, cnt;
	unsigned int lpm;
	void (*resume_addr)(void);
#if defined(CONFIG_RTC_DRV_JZ4775)
	void * tmp = (void *)(0xb3422008);
	resume_times = (unsigned int *)tmp + 2050;
	*resume_times = 0;
#endif
#if defined(CONFIG_RTC_DRV_JZ4775) && defined(CONFIG_BATTERY_JZ)
 __jz_pm_do_sleep_start:
#endif
	printk("Put CPU into sleep mode.\n");
	//*(volatile unsigned *)0xb30100bc = 0;

 	/* set SLEEP mode */
	CMSREG32(CPM_LCR, LCR_LPM_SLEEP, LCR_LPM_MASK);

	/* Preserve current time */
	delta = xtime.tv_sec - REG_RTC_RTCSR;

	/* Save CPU irqs */
	local_irq_save(cpuflags);

        /* Disable nand flash */
	REG_NEMC_NFCSR = ~0xff;
	//printk("Disable nand flash OK!!! \n");
#if 1		
        /* stop sadc */
	REG_SADC_ENA |= SADC_ENA_POWER;
	while ((REG_SADC_ENA & SADC_ENA_POWER) != SADC_ENA_POWER) {
		dprintk("REG_SADC_ENA = 0x%x\n", REG_SADC_ENA);
		udelay(100);
	}
#endif

	/* Stop ohci power */
	REG32(0xb0000048) &= ~(1 << 17);

	REG_CPM_OPCR &= ~(3 << 6);
	REG32(0xb000003c) |= (1 << 25);  //common
	printk("REG_CPM_LCR:%x\n",REG_CPM_LCR);

	/* Mask all interrupts */
	OUTREG32(INTC_ICMSR(0), 0xffffffff);
	OUTREG32(INTC_ICMSR(1), 0xffffffff & ~(1 << 18));
	//OUTREG32(INTC_ICMSR(1), 0xbfff);

#if defined(CONFIG_RTC_DRV_JZ4775)
	OUTREG32(INTC_ICMCR(1), 0x1);
//	jz_set_alarm();
	jz_set_nc1hz();
	__intc_ack_irq(IRQ_RTC);
	__intc_unmask_irq(IRQ_RTC); //unmask rtc irq
	//rtc_clr_reg(RTC_RTCCR,RTC_RTCCR_AF);
	rtc_clr_reg(RTC_RTCCR,RTC_RTCCR_1HZ);
#else
	/* mask rtc interrupts */
	OUTREG32(INTC_ICMSR(1), 0x1);
#endif

	/* Sleep on-board modules */
	jz_board_do_sleep(sleep_gpio_save);

	dprintk("control = 0x%08x icmr0 = 0x%08x icmr1 = 0x%08x\n",
	       INREG32(RTC_RTCCR), INREG32(INTC_ICMR(0)), INREG32(INTC_ICMR(1)));

	/* disable externel clock Oscillator in sleep mode */
	CLRREG32(CPM_OPCR, OPCR_O1SE);

	/* select 32K crystal as RTC clock in sleep mode */
	SETREG32(CPM_OPCR, OPCR_ERCS);

	/* close CPU, VPU, GPU */
	REG_CPM_LCR |= (0xf << 28);
	while(!(REG_CPM_LCR & (0x1 << 24)));
	while(!(REG_CPM_LCR & (0x1 << 25)));
	while(!(REG_CPM_LCR & (0x1 << 26)));
	while(!(REG_CPM_LCR & (0x1 << 27)));

	REG_CPM_OPCR |= (1 << 2);  //select rtc clk
	REG_CPM_OPCR &= ~(1 << 4);  //extclk disable
	REG_CPM_OPCR |= (0xff << 8);  //p0 not power done
	REG_CPM_OPCR |=  (1 << 25);

	REG_CPM_LCR &= ~(3);   //sleep mode
	REG_CPM_LCR |= 1;

	/* Clear previous reset status */
	REG_CPM_RSR = 0;

	/* like jz4775, for cpu 533M 1:2:4 */
	OUTREG32(CPM_PSWC0ST, 0);
	OUTREG32(CPM_PSWC1ST, 8);
	OUTREG32(CPM_PSWC2ST, 11);
	OUTREG32(CPM_PSWC3ST, 0);

#if defined(CONFIG_PM_POWERDOWN_P0)
	printk("Shutdown P0\n");

	/* power down the p0 */
	SETREG32(CPM_OPCR, OPCR_PD);

      	/* Clear previous reset status */
	CLRREG32(CPM_RSR, RSR_PR | RSR_WR | RSR_P0R);

	resume_addr = (void (*)(void))(0xb3422008);
	OUTREG32(CPM_SLBC, 0x1);
	OUTREG32(CPM_SLPC, 0xb3422008);

	memcpy((unsigned int *)resume_addr, (unsigned int *)jz_cpu_resume, 1024*4);

	printk("resume addr = 0x%08x\n", resume_addr);

	rtc_clr_reg(RTC_RTCCR,RTC_RTCCR_AF);

#ifdef CONFIG_JZ_SYSTEM_AT_CARD
	lpm = REG_MSC_LPM(0);
#endif
	/* *** go zzz *** */
//	printk("Run jz_cpu_sleep core....\n");
	div = DIV(2, 4, 4, 4, 8);
	div = jz_set_div(div);

	jz_cpu_sleep();

	div = DIV(1, 2, 4, 4, 8);
	jz_set_div(div);
	printk("wake up\n");

#ifdef CONFIG_JZ_SYSTEM_AT_CARD
	REG_MSC_LPM(0) = lpm;
#endif

#else

	__asm__(".set\tmips32\n\t"
			"sync\n\t"
			".set\tmips32");

	/* Prefetch codes from L1 */
	addr = (unsigned long)(&&L1) & ~(32 - 1);
	size = 32 * 128; /* load 128 cachelines */
	end = addr + size;

	for (; addr < end; addr += 32) {
		__asm__ volatile (
				".set mips32\n\t"
				" cache %0, 0(%1)\n\t"
				".set mips32\n\t"
				:
				: "I" (Index_Prefetch_I), "r"(addr));
	}

	/* wait for EMC stable */
	cnt = 0x3ffff;
	while(cnt--);


	/* Start of the prefetching codes */
L1:
	*((volatile unsigned int *)0xb3020050) = 0xff00ff00;


	__asm__ volatile (".set\tmips32\n\t"
			"wait\n\t"
			"nop\n\t"
			"nop\n\t"
			"nop\n\t"
			"nop\n\t"
			".set\tmips32");

	*((volatile unsigned int *)0xb3020050) = 0x0000ff00;
L2:

	/* End of the prefetching codes */
#endif

	/*if power down p0 ,return from sleep.S*/

	/* Restore to IDLE mode */
	CMSREG32(CPM_LCR, LCR_LPM_IDLE, LCR_LPM_MASK);

	/* Restore nand flash control register, it must be restored,
	   because it will be clear to 0 in bootrom. */
	REG_NEMC_NFCSR = nfcsr;


	/* Restore interrupts FIXME:*/
	OUTREG32(INTC_ICMR(0), icmr0);
	OUTREG32(INTC_ICMR(1), icmr1);

	/* Restore sadc */
	//REG_SADC_ENA = sadc;	

	/* Resume on-board modules */
	jz_board_do_resume(sleep_gpio_save);

	/* Restore Oscillator and Power Control Register */
	OUTREG32(CPM_OPCR, opcr);

	/* Restore CPU interrupt flags */
	local_irq_restore(cpuflags);

	/* Restore current time */
	xtime.tv_sec = REG_RTC_RTCSR + delta;

	printk("Resume CPU from sleep mode.\n");

	CLRREG32(CPM_RSR, RSR_PR | RSR_WR | RSR_P0R);

//	printk("===>Leave CPU Sleep\n");
#if  defined(CONFIG_RTC_DRV_JZ4775) && defined(CONFIG_BATTERY_JZ)
	if((INREG32(RTC_RTCCR) & RTC_RTCCR_AF)) {
		rtc_clr_reg(RTC_RTCCR, RTC_RTCCR_AF | RTC_RTCCR_AE | RTC_RTCCR_AIE);
	}
#endif
#if defined(CONFIG_RTC_DRV_JZ4775) && defined(CONFIG_BATTERY_JZ)
//	jz_restore_alarm();
#endif
#if defined(CONFIG_RTC_DRV_JZ4775)
	goto __jz_pm_do_sleep_start;
#endif

	return 0;
}

#define K0BASE  KSEG0
void jz_flush_cache_all(void)
{
	unsigned long addr;

	/* Clear CP0 TagLo */
	asm volatile ("mtc0 $0, $28\n\t"::);

	for (addr = K0BASE; addr < (K0BASE + 0x4000); addr += 32) {
		asm volatile (
				".set mips3\n\t"
				" cache %0, 0(%1)\n\t"
				".set mips2\n\t"
				:
				: "I" (Index_Writeback_Inv_D), "r"(addr));

		asm volatile (
				".set mips3\n\t"
				" cache %0, 0(%1)\n\t"
				".set mips2\n\t"
				:
				: "I" (Index_Store_Tag_I), "r"(addr));
	}

	asm volatile ("sync\n\t"::);

	/* invalidate BTB */
	asm volatile (
			".set mips32\n\t"
			" mfc0 %0, $16, 7\n\t"
			" nop\n\t"
			" ori $0, 2\n\t"
			" mtc0 %0, $16, 7\n\t"
			" nop\n\t"
			".set mips2\n\t"
			:
			: "r"(addr));
}

void jz_pm_hibernate(void)
{
	jz_pm_do_hibernate();
}

static int jz4775_pm_valid(suspend_state_t state)
{
	return state == PM_SUSPEND_MEM;
}

/*
 * Jz CPU enter save power mode
 */
static int jz4775_pm_enter(suspend_state_t state)
{
	jz_pm_do_sleep();
	return 0;
}

static struct platform_suspend_ops jz4775_pm_ops = {
	.valid		= jz4775_pm_valid,
	.enter		= jz4775_pm_enter,
};

/*
 * Initialize power interface
 */
int __init jz_pm_init(void)
{
	printk("Power Management for JZ\n");

	suspend_set_ops(&jz4775_pm_ops);
	return 0;
}

#ifdef JZ_PM_BATTERY_SIMED
#undef CONFIG_BATTERY_JZ
#endif

