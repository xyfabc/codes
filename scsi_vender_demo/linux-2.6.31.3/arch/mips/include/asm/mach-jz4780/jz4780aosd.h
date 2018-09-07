/*
 * linux/include/asm-mips/mach-jz4780/jz4780aosd.h
 *
 * JZ4780 COMPRESS register definition.
 *
 * Copyright (C) 2010 Ingenic Semiconductor Co., Ltd.
 */

#ifndef __JZ4780AOSD_H__
#define __JZ4780AOSD_H__

#define AOSD_BASE        0xB3070000

/*************************************************************************
 * OSD (On Screen Display)
 *************************************************************************/
/*************************************************************************
 * COMPRESS
 *************************************************************************/

#define COMP_CTRL              (AOSD_BASE + 0x000)
#define COMP_CFG               (AOSD_BASE + 0x004)
#define COMP_STATE             (AOSD_BASE + 0x008)
#define COMP_FWDCNT            (AOSD_BASE + 0x00c)
#define COMP_WADDR             (AOSD_BASE + 0x104)
#define COMP_WCMD              (AOSD_BASE + 0x108)
#define COMP_WOFFS             (AOSD_BASE + 0x10c)
#define COMP_RADDR             (AOSD_BASE + 0x114)
#define COMP_RSIZE             (AOSD_BASE + 0x118)
#define COMP_ROFFS             (AOSD_BASE + 0x11c)

#define REG_COMP_CTRL          REG32(COMP_CTRL)
#define REG_COMP_CFG           REG32(COMP_CFG)
#define REG_COMP_STATE         REG32(COMP_STATE)
#define REG_COMP_FWDCNT        REG32(COMP_FWDCNT)
#define REG_COMP_WADDR         REG32(COMP_WADDR)
#define REG_COMP_WCMD          REG32(COMP_WCMD)
#define REG_COMP_WOFFS         REG32(COMP_WOFFS)
#define REG_COMP_RADDR         REG32(COMP_RADDR)
#define REG_COMP_RSIZE         REG32(COMP_RSIZE)
#define REG_COMP_ROFFS         REG32(COMP_ROFFS)

/* Compress Ctrl register */
#define COMP_CTRL_ENA          BIT0
#define COMP_CTRL_QD           BIT2

/* Compress Configure register */
#define COMP_CFG_ADM           BIT0
#define COMP_CFG_QDM           BIT2
#define COMP_CFG_TMODE         BIT4

/* Compress State register */
#define COMP_AD_FLAG           BIT0
#define COMP_QD_FLAG           BIT2

/* Compress Command register */
#define COMP_WCMD_BPP_MSK      (3 << 27)
#define COMP_WCMD_BPP_16       (1 << 27)
#define COMP_WCMD_BPP_24       (2 << 27)
#define COMP_WCMD_COMPMD_MSK   (3 << 29)
#define COMP_WCMD_COMPMD_16    (2 << 29)
#define COMP_WCMD_COMPMD_24C   (1 << 29)
#define COMP_WCMD_COMPMD_24    (0 << 29)

/* Compress Dst Offset */
#define COMP_DOFFS_MSK        (0xffffff)

/* Compress Src Size */
#define COMP_RWIDTH_BIT        0
#define COMP_RHEIGHT_BIT       12

/* Compress Src Offset */
#define COMP_ROFFS_MSK         (0xffffff)

#define compress_start()        (REG_COMP_CTRL |= COMP_CTRL_ENA)      
#define compress_to_24c()       (REG_COMP_WCMD = (REG_COMP_WCMD & ~COMP_WCMD_COMPMD_MSK) | COMP_WCMD_COMPMD_24C)      
#define compress_to_24()        (REG_COMP_WCMD = (REG_COMP_WCMD & ~COMP_WCMD_COMPMD_MSK) | COMP_WCMD_COMPMD_24)
#define compress_to_16()        (REG_COMP_WCMD = (REG_COMP_WCMD & ~COMP_WCMD_COMPMD_MSK) | COMP_WCMD_COMPMD_16)
#define compress_set_bpp16()    (REG_COMP_WCMD = (REG_COMP_WCMD & ~COMP_WCMD_BPP_MSK) | COMP_WCMD_BPP_16)
#define compress_set_bpp24()    (REG_COMP_WCMD = (REG_COMP_WCMD & ~COMP_WCMD_BPP_MSK) | COMP_WCMD_BPP_24)
#define compress_set_128unit() (REG_COMP_CFG |= COMP_CFG_TMODE) 
#define compress_set_64unit() (REG_COMP_CFG &= ~COMP_CFG_TMODE) 
#define compress_set_doffs(n)   (REG_COMP_WOFFS = (REG_COMP_WOFFS & ~COMP_DOFFS_MSK) | (n & COMP_DOFFS_MSK))
#define compress_set_roffs(n)   (REG_COMP_ROFFS = (REG_COMP_ROFFS & ~COMP_ROFFS_MSK) | (n & COMP_ROFFS_MSK))
#define compress_set_waddr(n)   (REG_COMP_WADDR = n)
#define compress_set_raddr(n)   (REG_COMP_RADDR = n)
#define compress_mask_ad()      (REG_COMP_CFG |= COMP_CFG_ADM)
#define compress_mask_qd()      (REG_COMP_CFG |= COMP_CFG_QDM)
#define compress_get_ad()       (REG_COMP_STATE & COMP_AD_FLAG)
#define compress_get_qd()       (REG_COMP_STATE & COMP_QD_FLAG)
#define compress_clear_ad()       (REG_COMP_STATE |= COMP_AD_FLAG)
#define compress_clear_qd()       (REG_COMP_STATE |= COMP_QD_FLAG)
#define compress_set_size(w, h)   (REG_COMP_RSIZE = (w << COMP_RWIDTH_BIT) | (h << COMP_RHEIGHT_BIT))
#define compress_get_fwdcnt()     (REG_COMP_FWDCNT)
#endif /* __JZ4780AOSD_H__ */

