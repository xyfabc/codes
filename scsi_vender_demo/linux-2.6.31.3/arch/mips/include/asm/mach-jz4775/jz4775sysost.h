/*
 * jz4775ost.h
 * JZ4775 OST register definition
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 *
 * Author: whxu@ingenic.cn
 */

#ifndef __JZ4775SYSOST_H__
#define __JZ4775SYSOST_H__

/*
 * Operating system timer module(OST) address definition
 */

/*
 * OST registers offset address definition
 */
#define SYSOST_BASE				(0xB2000000)
#define SYSOST_PHY_BASE				(0x12000000)

#define MAX_SYSOST_NUM				32	//sysost irq num, really used 3.

#define IRQ_SYSOST_FULLIRQ			(IRQ_SYSOST_0 + 7)
#define IRQ_SYSOST_HALFIRQ			(IRQ_SYSOST_0 + 23)
#define IRQ_SYSOST_OSTIRQ			(IRQ_SYSOST_0 + 15)

//channel 1
#define SYSOST_TSR_OFFSET		(0x1c)
#define SYSOST_TSSR_OFFSET		(0x2c)
#define SYSOST_TSCR_OFFSET		(0x3c)
#define SYSOST_TER_OFFSET		(0x10)
#define SYSOST_TESR_OFFSET		(0x14)
#define SYSOST_TECR_OFFSET		(0x18)
#define SYSOST_TFR_OFFSET		(0x20)
#define SYSOST_TFSR_OFFSET		(0x24)
#define SYSOST_TFCR_OFFSET		(0x28)
#define SYSOST_TMR_OFFSET		(0x30)
#define SYSOST_TMSR_OFFSET		(0x34)
#define SYSOST_TMCR_OFFSET		(0x38)
#define SYSOST_TDFR_OFFSET		(0xb0)
#define SYSOST_TDHR_OFFSET		(0xb4)
#define SYSOST_TCNT_OFFSET		(0xb8)
#define SYSOST_TCSR_OFFSET		(0xbc)
#define SYSOST_TDFSR_OFFSET		(0xc0)
#define SYSOST_TDHSR_OFFSET		(0xc4)
#define SYSOST_SSR_OFFSET		(0xc8)

//channel 2
#define SYSOST_OSTDR_OFFSET		(0xe0)  /* rw, 32, 0x???????? */
#define SYSOST_OSTCNTL_OFFSET		(0xe4)
#define SYSOST_OSTCNTH_OFFSET		(0xe8)
#define SYSOST_OSTCSR_OFFSET		(0xec)  /* rw, 16, 0x0000 */

#define SYSOST_OSTCNTH_BUF_OFFSET	(0xfc)

/*
 * OST registers address definition
 */
#define SYSOST_TSR			(SYSOST_BASE + SYSOST_TSR_OFFSET)
#define SYSOST_TSSR			(SYSOST_BASE + SYSOST_TSSR_OFFSET)
#define SYSOST_TSCR			(SYSOST_BASE + SYSOST_TSCR_OFFSET)
#define SYSOST_TER			(SYSOST_BASE + SYSOST_TER_OFFSET)
#define SYSOST_TESR			(SYSOST_BASE + SYSOST_TESR_OFFSET)
#define SYSOST_TECR			(SYSOST_BASE + SYSOST_TECR_OFFSET)
#define SYSOST_TFR			(SYSOST_BASE + SYSOST_TFR_OFFSET)
#define SYSOST_TFSR			(SYSOST_BASE + SYSOST_TFSR_OFFSET)
#define SYSOST_TFCR			(SYSOST_BASE + SYSOST_TFCR_OFFSET)
#define SYSOST_TMR			(SYSOST_BASE + SYSOST_TMR_OFFSET)
#define SYSOST_TMSR			(SYSOST_BASE + SYSOST_TMSR_OFFSET)
#define SYSOST_TMCR			(SYSOST_BASE + SYSOST_TMCR_OFFSET)
#define SYSOST_TDFR			(SYSOST_BASE + SYSOST_TDFR_OFFSET)
#define SYSOST_TDHR			(SYSOST_BASE + SYSOST_TDHR_OFFSET)
#define SYSOST_TCNT			(SYSOST_BASE + SYSOST_TCNT_OFFSET)
#define SYSOST_TCSR			(SYSOST_BASE + SYSOST_TCSR_OFFSET)
#define SYSOST_TDFSR			(SYSOST_BASE + SYSOST_TDFSR_OFFSET)
#define SYSOST_TDHSR			(SYSOST_BASE + SYSOST_TDHSR_OFFSET)
#define SYSOST_SSR			(SYSOST_BASE + SYSOST_SSR_OFFSET)

#define SYSOST_OSTDR			(SYSOST_BASE + SYSOST_OSTDR_OFFSET)
#define SYSOST_OSTCNTL			(SYSOST_BASE + SYSOST_OSTCNTL_OFFSET)
#define SYSOST_OSTCNTH			(SYSOST_BASE + SYSOST_OSTCNTH_OFFSET)
#define SYSOST_OSTCSR			(SYSOST_BASE + SYSOST_OSTCSR_OFFSET)
#define SYSOST_OSTCNTH_BUF		(SYSOST_BASE + SYSOST_OSTCNTH_BUF_OFFSET)

/*
 * OST registers common define
 */

//TSR
#define SYSOST_TSR_OSTS			BIT15
#define SYSOST_TSR_STOP			BIT7

//TER
#define SYSOST_TER_OSTST		BIT15
#define SYSOST_TER_TCST			BIT7

//TFR
#define SYSOST_TFR_HFLAG		BIT23
#define SYSOST_TFR_OSTFLAG		BIT15
#define SYSOST_TER_FFLAG		BIT7

//TMR
#define SYSOST_TMR_HMASK		BIT23
#define SYSOST_TMR_OSTMASK		BIT15
#define SYSOST_TMR_FMASK		BIT7

//TDFR
#define SYSOST_TDFR_MASK		(0x0FFFFFFF)

//TDHR
#define SYSOST_TDHR_MASK		(0x0FFFFFFF)

//TDFSR
#define SYSOST_TDFSR_MASK		(0x0FFFFFFF)

//TDHSR
#define SYSOST_TDHSR_MASK		(0x0FFFFFFF)

//TCSR
#define SYSOST_TCSR_PRESCALE_LSB	3
#define SYSOST_TCSR_PRESCALE_MASK	BITS_H2L(5, SYSOST_TCSR_PRESCALE_LSB)
#define SYSOST_TCSR_PRESCALE1		(0x0 << SYSOST_TCSR_PRESCALE_LSB)
#define SYSOST_TCSR_PRESCALE4		(0x1 << SYSOST_TCSR_PRESCALE_LSB)
#define SYSOST_TCSR_PRESCALE16		(0x2 << SYSOST_TCSR_PRESCALE_LSB)
#define SYSOST_TCSR_PRESCALE64		(0x3 << SYSOST_TCSR_PRESCALE_LSB)
#define SYSOST_TCSR_PRESCALE256		(0x4 << SYSOST_TCSR_PRESCALE_LSB)
#define SYSOST_TCSR_PRESCALE1024	(0x5 << SYSOST_TCSR_PRESCALE_LSB)


//TCNT
#define SYSOST_TCNT_MASK		(0x0FFFFFFF)

//SSR
#define SYSOST_SSR_SSR			BIT0

/* Operating system control register(OSTCSR) */
#define SYSOST_OSTCSR_CNT_MD		BIT15


#ifndef __MIPS_ASSEMBLER

#define REG_SYSOST_TSR			REG32(SYSOST_TSR)
#define REG_SYSOST_TSSR			REG16(SYSOST_TSSR)
#define REG_SYSOST_TSCR			REG16(SYSOST_TSCR)
#define REG_SYSOST_TER			REG16(SYSOST_TER)
#define REG_SYSOST_TESR			REG16(SYSOST_TESR)
#define REG_SYSOST_TECR			REG16(SYSOST_TECR)
#define REG_SYSOST_TFR			REG32(SYSOST_TFR)
#define REG_SYSOST_TFSR			REG32(SYSOST_TFSR)
#define REG_SYSOST_TFCR			REG32(SYSOST_TFCR)
#define REG_SYSOST_TMR			REG32(SYSOST_TMR)
#define REG_SYSOST_TMSR			REG32(SYSOST_TMSR)
#define REG_SYSOST_TMCR			REG32(SYSOST_TMCR)
#define REG_SYSOST_TDFR			REG32(SYSOST_TDFR)
#define REG_SYSOST_TDHR			REG32(SYSOST_TDHR)
#define REG_SYSOST_TCNT			REG32(SYSOST_TCNT)
#define REG_SYSOST_TCSR			REG16(SYSOST_TCSR)
#define REG_SYSOST_TDFSR		REG32(SYSOST_TDFSR)
#define REG_SYSOST_TDHSR		REG32(SYSOST_TDHSR)
#define REG_SYSOST_SSR			REG16(SYSOST_SSR)

#define REG_SYSOST_OSTDR		REG32(SYSOST_OSTDR)
#define REG_SYSOST_OSTCNTL		REG32(SYSOST_OSTCNTL)
#define REG_SYSOST_OSTCNTH		REG32(SYSOST_OSTCNTH)
#define REG_SYSOST_OSTCSR		REG16(SYSOST_OSTCSR)
#define REG_SYSOST_OSTCNTH_BUF		REG32(SYSOST_OSTCNTH_BUF)

//ops
#define __sysost_start_ostclk()		(REG_SYSOST_TSCR = SYSOST_TSR_OSTS)
#define __sysost_stop_ostclk() 		(REG_SYSOST_TSSR = SYSOST_TSR_OSTS)
#define __sysost_start_tcclk()		(REG_SYSOST_TSCR = SYSOST_TSR_STOP)
#define __sysost_stop_tcclk()		(REG_SYSOST_TSSR = SYSOST_TSR_STOP)

#define __sysost_enable_osten()		(REG_SYSOST_TESR = SYSOST_TER_OSTST)
#define __sysost_disable_osten()	(REG_SYSOST_TECR = SYSOST_TER_OSTST)
#define __sysost_enable_tcen()		(REG_SYSOST_TESR = SYSOST_TER_TCST)
#define __sysost_disable_tcen()		(REG_SYSOST_TECR = SYSOST_TER_TCST)

#define __sysost_set_irq(n)		(REG_SYSOST_TFSR = 1 << (n))
#define __sysost_ack_irq(n)		(REG_SYSOST_TFCR = 1 << (n))
#define __sysost_get_irq()		(REG_SYSOST_TFR)

#define __sysost_mask_irq(n)		(REG_SYSOST_TMSR = 1 << (n))
#define __sysost_unmask_irq(n)		(REG_SYSOST_TMCR = 1 << (n))
#define __sysost_get_mask()		(REG_SYSOST_TMR)

#define __sysost_set_full_data(val)	(REG_SYSOST_TDFR = (val) & SYSOST_TDFR_MASK)
#define __sysost_set_half_data(val)	(REG_SYSOST_TDHR = (val) & SYSOST_TDHR_MASK)
#define __sysost_set_shadow_full(val)	(REG_SYSOST_TDFSR = (val) & SYSOST_TDFSR_MASK)
#define __sysost_set_shadow_half(val)	(REG_SYSOST_TDHSR = (val) & SYSOST_TDHSR_MASK)

#define __sysost_set_cnt(val)		(REG_SYSOST_TCNT = (val) & SYSOST_TCNT_MASK)
#define __sysost_get_cnt()		(REG_SYSOST_TCNT & SYSOST_TCNT_MASK)

#define __sysost_set_prescale(n)	(REG_SYSOST_TCSR = (REG_SYSOST_TCSR & 0xffC7) | (n))

#define __sysost_enable_shadow()	(REG_SYSOST_SSR = REG_SYSOST_SSR | SYSOST_SSR_SSR)
#define __sysost_disable_shadow()	(REG_SYSOST_SSR = REG_SYSOST_SSR & ~(SYSOST_SSR_SSR))

//channel2
#define __sysost_enable_ost_md()	(REG_SYSOST_OSTCSR |= SYSOST_OSTCSR_CNT_MD)
#define __sysost_disable_ost_md()	(REG_SYSOST_OSTCSR &= ~(SYSOST_OSTCSR_CNT_MD))
#define __sysost_set_ost_data(val)	(REG_SYSOST_OSTDR = (val))
#define __sysost_get_ost_data()		(REG_SYSOST_OSTDR)
#define __sysost_set_ost_cnth(val)	(REG_SYSOST_OSTCNTH = (val))
#define __sysost_get_ost_cnth()		(REG_SYSOST_OSTCNTH)
#define __sysost_set_ost_cntl(val)	(REG_SYSOST_OSTCNTL = (val))
#define __sysost_get_ost_cntl()		(REG_SYSOST_OSTCNTL)
#define __sysost_get_ost_cnth_buf()	(REG_SYSOST_OSTCNTH_BUF)


#endif /* __MIPS_ASSEMBLER */

#endif /* __JZ4775SYSOST_H__ */
