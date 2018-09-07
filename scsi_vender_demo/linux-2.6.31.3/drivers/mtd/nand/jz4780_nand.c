/*
 * linux/drivers/mtd/nand/jz4780_nand.c
 *
 * JZ4780 NAND driver
 *
 * Copyright (c) 2005 - 2007 Ingenic Semiconductor Inc.
 * Author: <lhhuang@ingenic.cn>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * NOTE:
 *	The OOB size is align with 4 bytes now. 
 * 	If your nand's OOB size not align with 4,and all of the data in OOB area is valid,
 * 	please FIXUP the value of desc->dcnt when init the write/read dma descs, in jz4770_nand_dma_init func.
 * 	<hpyang@ingenic.cn> 2011/04/02
 */

#include <linux/slab.h>
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/init.h>

#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/partitions.h>

#include <asm/io.h>
#include <asm/jzsoc.h>

//#define DEBUG_TEST_NAND 	1 
//#define DEBUG_PN 	1

#define USE_DIRECT	1
#define USE_BCH		1
#define	USE_COUNTER	0
#define	COUNT_0		0	/* 1:count the number of 1; 0:count the number of 0 */
#define PN_ENABLE		(1 | PN_RESET)
#define PN_DISABLE		0
#define PN_RESET		(1 << 1)
#define COUNTER_ENABLE  	((1 << 3) | COUNTER_RESET)
#define COUNTER_DISABLE 	(0 << 3)
#define COUNTER_RESET   	(1 << 5)
#define COUNT_FOR_1		(0 << 4)
#define COUNT_FOR_0		(1 << 4)

#define MISC_DEBUG	0
#define BCH_DEBUG	0
#define DMA_DEBUG	0

#if MISC_DEBUG
#define	dprintk(n,x...) printk(n,##x)
#else
#define	dprintk(n,x...)
#endif

#if BCH_DEBUG
#define	bch_printk(n,x...) printk(n,##x)
#else
#define	bch_printk(n,x...)
#endif

#define BCH_DR_BYTE	1
//#define BCH_DR_HWORD	1
//#define BCH_DR_WORD	1

#if defined(BCH_DR_BYTE) || defined(BCH_DR_HWORD) || defined(BCH_DR_WORD)
#else
#define BCH_DR_BYTE 	1
#endif

#define BCH_ECC_BIT	CONFIG_MTD_HW_BCH_BIT
#define PAR_SIZE	(14 * BCH_ECC_BIT / 8)

#if 0
#define NAND_TYPE_COMMON	0
#define NAND_TYPE_TOGGLE	1
#endif

#define NAND_DATA_PORT1		0xBB000000	/* read-write area in static bank 1 */
#define NAND_DATA_PORT2		0xBA000000	/* read-write area in static bank 2 */
#define NAND_DATA_PORT3		0xB9000000	/* read-write area in static bank 3 */
#define NAND_DATA_PORT4		0xB8000000	/* read-write area in static bank 4 */
#define NAND_DATA_PORT5		0xB7000000
#define NAND_DATA_PORT6		0xB6000000

#define NAND_ADDR_OFFSET	0x00800000      /* address port offset for unshare mode */
#define NAND_CMD_OFFSET		0x00400000      /* command port offset for unshare mode */

struct buf_be_corrected {
	u8 *data;
	u8 *oob;
	u8 eccsize; 
};

static u32 addr_offset;
static u32 cmd_offset;

#if 0
static u32 nand_type;
#endif

extern int global_page; /* for two-plane operations */
extern int global_mafid; /* ID of manufacture */

/*
 * MTD structure for JzSOC board
 */
static struct mtd_info *jz_mtd = NULL;
extern struct mtd_info *jz_mtd1;
extern char all_use_planes;

/*
 * Define partitions for flash devices
 */
#if defined(CONFIG_JZ4770_F4770) || defined(CONFIG_JZ4770_PISCES) || defined(CONFIG_JZ4780_F4780)
static struct mtd_partition partition_info[] = {
	{name:"NAND BOOT partition",
rl_offset:0 * 0x100000LL,
	  rl_size:4 * 0x100000LL,
	  use_planes: 0},
	{name:"NAND KERNEL partition",
rl_offset:4 * 0x100000LL,
	  rl_size:4 * 0x100000LL,
	  use_planes: 0},
	{name:"NAND ROOTFS partition",
rl_offset:8 * 0x100000LL,
	  rl_size:504 * 0x100000LL,
	  use_planes: 0},
	{name:"NAND DATA partition",
rl_offset:512 * 0x100000LL,
	  rl_size:512 * 0x100000LL,
	  use_planes: 1},
	{name:"NAND VFAT partition",
rl_offset:1024 * 0x100000LL,
	  rl_size:1024 * 0x100000LL,
	  use_planes: 1},
};

/* Define max reserved bad blocks for each partition.
 * This is used by the mtdblock-jz.c NAND FTL driver only.
 *
 * The NAND FTL driver reserves some good blocks which can't be
 * seen by the upper layer. When the bad block number of a partition
 * exceeds the max reserved blocks, then there is no more reserved
 * good blocks to be used by the NAND FTL driver when another bad
 * block generated.
 */
static int partition_reserved_badblocks[] = {
	2,			/* reserved blocks of mtd0 */
	2,			/* reserved blocks of mtd1 */
	10,			/* reserved blocks of mtd2 */
	10,			/* reserved blocks of mtd3 */
	20			/* reserved blocks of mtd4 */
};				/* reserved blocks of mtd5 */
#endif				/* CONFIG_JZ4760_CYGNUS || CONFIG_JZ4760_LEPUS */


/*-------------------------------------------------------------------------
 * Following three functions are exported and used by the mtdblock-jz.c
 * NAND FTL driver only.
 */

extern void buffer_dump(uint8_t *buffer, int length, const char *comment, char *file, char *function, int line);

static void mem_dump(uint8_t *buf, int len)
{
	int line = len / 16;
	int i, j;

	if (len & 0xF)
		line++;

	printk("\n<0x%08x> Length: %dBytes \n", (unsigned int)buf, len);

	printk("--------| ");
	for (i = 0; i < 16; i++)
		printk("%02d ", i);

	printk("\n");

	for (i = 0; i < line; i++) {
		printk("%08x: ", i);

		if (i < (len / 16)) {
			for (j = 0; j < 16; j++)
				printk("%02x ", buf[(i << 4) + j]);
		} else {
			for (j = 0; j < (len & 0xF); j++)
				printk("%02x ", buf[(i << 4) + j]);
		}

		printk("\n");
	}

	printk("\n");
}

static void calc_partition_size(struct mtd_info *mtd)
{
	int total_partitions,count;
	struct nand_chip *this = mtd->priv;
	total_partitions = sizeof(partition_info) / sizeof(struct mtd_partition);
	for(count = 0; count < total_partitions; count++){ //For the partition which accessed by driver must use -o mode
		partition_info[count].size = partition_info[count].rl_size 
			- (partition_info[count].rl_size >> this->page_shift) * mtd->freesize;
		partition_info[count].offset = partition_info[count].rl_offset
			- (partition_info[count].rl_offset >> this->page_shift) * mtd->freesize;
		if(mtd_mod_by_eb(partition_info[count].size, mtd)){
			partition_info[count].size -= mtd_mod_by_eb(partition_info[count].size, mtd); 
		}
	}
}

unsigned short get_mtdblock_write_verify_enable(void)
{
#ifdef CONFIG_MTD_MTDBLOCK_WRITE_VERIFY_ENABLE
	return 1;
#endif
	return 0;
}

EXPORT_SYMBOL(get_mtdblock_write_verify_enable);

unsigned short get_mtdblock_oob_copies(void)
{
	return CONFIG_MTD_OOB_COPIES;
}

EXPORT_SYMBOL(get_mtdblock_oob_copies);

int *get_jz_badblock_table(void)
{
	return partition_reserved_badblocks;
}

EXPORT_SYMBOL(get_jz_badblock_table);

/*------------------------hwcontrol common nandflash-------------------------------------------------*/

static void jz_hwcontrol_common(struct mtd_info *mtd, int dat, u32 ctrl)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	u32 nandaddr = (u32)this->IO_ADDR_W;
	extern u8 nand_nce;  /* defined in nand_base.c, indicates which chip select is used for current nand chip */

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_NCE) {
			switch (nand_nce) {
				case NAND_NCE1:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT1;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE1 | NEMC_NFCSR_NFE1;
					break;
				case NAND_NCE2:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT2;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE2 | NEMC_NFCSR_NFE2;
					break;
				case NAND_NCE3:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT3;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE3 | NEMC_NFCSR_NFE3;
					break;
				case NAND_NCE4:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT4;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE4 | NEMC_NFCSR_NFE4;
					break;
				case NAND_NCE5:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT5;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE5 | NEMC_NFCSR_NFE5;
					break;
				case NAND_NCE6:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT6;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE6 | NEMC_NFCSR_NFE6;
					break;
				default:
					printk("error: no nand_nce 0x%x\n",nand_nce);
					break;
			}
		} else {

			REG_NEMC_NFCSR = 0;
		}

		if (ctrl & NAND_ALE)
			nandaddr = (u32)((u32)(this->IO_ADDR_W) | addr_offset);
		else
			nandaddr = (u32)((u32)(this->IO_ADDR_W) & ~addr_offset);
		if (ctrl & NAND_CLE)
			nandaddr = (u32)(nandaddr | cmd_offset);
		else
			nandaddr = (u32)(nandaddr & ~cmd_offset);
	}

	this->IO_ADDR_W = (void __iomem *)nandaddr;
	if (dat != NAND_CMD_NONE) {
		writeb(dat, this->IO_ADDR_W);
		/* printk("write cmd:0x%x to 0x%x\n",dat,(u32)this->IO_ADDR_W); */
	}
}

/*-------------------------------hwcontrol toggle nandflash--------------------------------------*/
static void jz_hwcontrol_toggle(struct mtd_info *mtd, int dat, u32 ctrl)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	u32 nandaddr = (u32)this->IO_ADDR_W;
	extern u8 nand_nce;  /* defined in nand_base.c, indicates which chip select is used for current nand chip */

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_NCE) {
			switch (nand_nce) {
				case NAND_NCE1:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT1;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE1 | NEMC_NFCSR_NFE1 | NEMC_NFCSR_TNFE1;
					break;
				case NAND_NCE2:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT2;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE2 | NEMC_NFCSR_NFE2 | NEMC_NFCSR_TNFE2;
					break;
				case NAND_NCE3:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT3;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE3 | NEMC_NFCSR_NFE3 | NEMC_NFCSR_TNFE3;
					break;
				case NAND_NCE4:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT4;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE4 | NEMC_NFCSR_NFE4 | NEMC_NFCSR_TNFE4;
					break;
				case NAND_NCE5:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT5;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE5 | NEMC_NFCSR_NFE5 | NEMC_NFCSR_TNFE5;
					break;
				case NAND_NCE6:
					this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT6;
					REG_NEMC_NFCSR = NEMC_NFCSR_NFCE6 | NEMC_NFCSR_NFE6 | NEMC_NFCSR_TNFE6;
					break;
				default:
					printk("error: no nand_nce 0x%x\n", nand_nce);
					break;
			}
		}

		if (ctrl & NAND_NCE_CLR) {
			REG_NEMC_NFCSR = 0x80000000;
		}

		if (ctrl & NAND_FCE_SET) {
			switch (nand_nce) {
				case NAND_NCE1:
					__tnand_dphtd_sync(nand_nce);
					REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE1;
					break;
				case NAND_NCE2:
					__tnand_dphtd_sync(nand_nce);
					REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE2;
					break;
				case NAND_NCE3:
					__tnand_dphtd_sync(nand_nce);
					REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE3;
					break;
				case NAND_NCE4:
					__tnand_dphtd_sync(nand_nce);
					REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE4;
					break;
				case NAND_NCE5:
					__tnand_dphtd_sync(nand_nce);
					REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE5;
					break;
				case NAND_NCE6:
					__tnand_dphtd_sync(nand_nce);
					REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE6;
					break;
				default:
					printk("error: no nand_nce 0x%x\n", nand_nce);
			}
		}

		if (ctrl & NAND_FCE_CLEAN) {
			switch (nand_nce) {
				case NAND_NCE1:
					REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE1;
					__tnand_dphtd_sync(nand_nce);
					break;
				case NAND_NCE2:
					REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE2;
					__tnand_dphtd_sync(nand_nce);
					break;
				case NAND_NCE3:
					REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE3;
					__tnand_dphtd_sync(nand_nce);
					break;
				case NAND_NCE4:
					REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE4;
					__tnand_dphtd_sync(nand_nce);
					break;
				case NAND_NCE5:
					REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE5;
					__tnand_dphtd_sync(nand_nce);
					break;
				case NAND_NCE6:
					REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE5;
					__tnand_dphtd_sync(nand_nce);
					break;
				default:
					printk("error: no nand_nce 0x%x\n", nand_nce);
			}
		}

		if (ctrl & NAND_ALE)
			nandaddr = (u32)((u32)(this->IO_ADDR_W) | addr_offset);
		else
			nandaddr = (u32)((u32)(this->IO_ADDR_W) & ~addr_offset);

		if (ctrl & NAND_CLE)
			nandaddr = (u32)(nandaddr | cmd_offset);
		else
			nandaddr = (u32)(nandaddr & ~cmd_offset);

		/* PN enable */
		if (ctrl & NAND_CTL_SETPN)
			REG_NEMC_PNCR = NEMC_PNCR_PNEN | NEMC_PNCR_PNRST;

		/* PN disable */
		if (ctrl & NAND_CTL_CLRPN)
			REG_NEMC_PNCR = 0;

		/* toggle nand read preparation */
		if (ctrl & TNAND_READ_PREPARE){
			REG_NEMC_TGWE |= NEMC_TGWE_DAE;
			__tnand_dae_sync();
		}

		/* toggle nand write preparation */
		if (ctrl & TNAND_WRITE_PREPARE){
			switch (nand_nce) {
				case NAND_NCE1:	
				case NAND_NCE2:
				case NAND_NCE3:
				case NAND_NCE4:
				case NAND_NCE5:
				case NAND_NCE6:
					REG_NEMC_TGWE |= NEMC_TGWE_DAE | NEMC_TGWE_SDE(nand_nce);
					__tnand_dae_sync();
					__tnand_wcd_sync();
					break;
				default:
					printk("error: no nand_nce 0x%x\n", nand_nce);
			}
		}

		if (ctrl & TNAND_DAE_CLEAN) {
			switch (nand_nce) {
				case NAND_NCE1:	
				case NAND_NCE2:
				case NAND_NCE3:
				case NAND_NCE4:
				case NAND_NCE5:
				case NAND_NCE6:
					REG_NEMC_TGWE &= ~NEMC_TGWE_DAE;
					__tnand_dae_clr();
					break;
				default:
					printk("error: no nand_nce 0x%x\n", nand_nce);
			}
		}

	}

	this->IO_ADDR_W = (void __iomem *)nandaddr;
	if (dat != NAND_CMD_NONE) {
		writeb(dat, this->IO_ADDR_W);
		/* printk("write cmd:0x%x to 0x%x\n",dat,(u32)this->IO_ADDR_W); */
//		 printk("write cmd:0x%x to 0x%x\n",dat,(u32)this->IO_ADDR_W); 
	}
}

static int jz_device_ready(struct mtd_info *mtd)
{
	int ready;
	volatile int wait = 20;
	while (wait--);
	ready = ((REG_GPIO_PXPIN(0) & 0x00100000) ? 1 : 0);
	return ready;
}

/*
 * NEMC setup
 */
static void jz_device_setup(void)
{
	// PORT 0:
	// PORT 1:
	// PORT 2:
	// PIN/BIT N		FUNC0		FUNC1
	//	21		CS1#		-
	//	22		CS2#		-
	//	23		CS3#		-
	//	24		CS4#		-
	//	25		CS5#		-
	//	26		CS6#		-
#define GPIO_CS2_N (32*0+22)
#define GPIO_CS3_N (32*0+23)
#define GPIO_CS4_N (32*0+24)
#define GPIO_CS5_N (32*0+25)
#define GPIO_CS6_N (32*0+26)


	#define SMCR_VAL   0x11444400
 //	#define SMCR_VAL   0x0fff7700  //slowest

	__gpio_as_nand_8bit(1);

#ifdef CFG_NAND_TOGGLE
	__gpio_as_nand_toggle();
#endif
	/* Read/Write timings */
	REG_NEMC_SMCR1 = SMCR_VAL;

#if defined(CONFIG_MTD_NAND_CS2)
	__gpio_as_func0(GPIO_CS2_N);

	/* Read/Write timings */
	REG_NEMC_SMCR2 = SMCR_VAL;
#endif

#if defined(CONFIG_MTD_NAND_CS3)
	__gpio_as_func0(GPIO_CS3_N);

	/* Read/Write timings */
	REG_NEMC_SMCR3 = SMCR_VAL;
#endif

#if defined(CONFIG_MTD_NAND_CS4)
	__gpio_as_func0(GPIO_CS4_N);

	/* Read/Write timings */
	REG_NEMC_SMCR4 = SMCR_VAL;
#endif

#if defined(CONFIG_MTD_NAND_CS5)
	__gpio_as_func0(GPIO_CS5_N);

	/* Read/Write timings */
	REG_NEMC_SMCR5 = SMCR_VAL;
#endif

#if defined(CONFIG_MTD_NAND_CS6)
	__gpio_as_func0(GPIO_CS6_N);

	/* Read/Write timings */
	REG_NEMC_SMCR6 = SMCR_VAL;
#endif

}

#ifdef CONFIG_MTD_HW_BCH_ECC

static void jzsoc_nand_enable_bch_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int ecc_pos = this->eccpos;
	int ecc_size = this->ecc.size;

	REG_BCH_INTS = 0xffffffff;
	if (mode == NAND_ECC_READ) {
		__bch_cnt_set(ecc_size, PAR_SIZE);
		__bch_decoding(BCH_ECC_BIT);
#if defined(CONFIG_MTD_NAND_DMA)
		//		__ecc_dma_enable();
#endif
	}

	if (mode == NAND_ECC_WRITE) {
		__bch_cnt_set(ecc_size, PAR_SIZE);
		__bch_encoding(BCH_ECC_BIT);
#if defined(CONFIG_MTD_NAND_DMA)
		//		__ecc_dma_enable();
#endif
	}

	if (mode == NAND_READ_OOB) {
		__bch_cnt_set(ecc_pos, PAR_SIZE);
		__bch_decoding(BCH_ECC_BIT);
#if defined(CONFIG_MTD_NAND_DMA)
		//		REG_BCH_INTES = BCH_INTES_DECFES;
		//		REG_BCH_INTEC = ~BCH_INTEC_DECFEC;
#endif
	}

	if (mode == NAND_WRITE_OOB) {
		__bch_cnt_set(ecc_pos, PAR_SIZE);
		__bch_encoding(BCH_ECC_BIT);
#if defined(CONFIG_MTD_NAND_DMA)
		//		REG_BCH_INTES = BCH_INTES_ENCFES;
		//		REG_BCH_INTEC = ~BCH_INTEC_ENCFEC;
#endif
	}
}

/**
 * bch_correct
 * @mtd:
 * @dat:        data to be corrected
 * @err_hword:	BCH Error Half-Word in ecc_size indicate
 */
static void bch_error_correct(struct mtd_info *mtd, u8 *dat, int err_bit)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int eccsize = ((struct buf_be_corrected *)dat)->eccsize;
	int ecc_pos = this->eccpos;
	//	int eccsize = this->ecc.size;
	unsigned short err_mask;
	unsigned short *data0;
	int i, idx; /* indicates an error half_word */

	if (eccsize != ecc_pos)
		eccsize = this->ecc.size;

	idx = REG_BCH_ERR(err_bit) & BCH_ERR_INDEX_MASK;
	err_mask = (REG_BCH_ERR(err_bit) & BCH_ERR_MASK_MASK) >> BCH_ERR_MASK_BIT;

	data0 = (unsigned short *)((struct buf_be_corrected *)dat)->data; // note data0 value
	//data0 = (unsigned short *)dat; // note data0 value

	i = idx * 2;
	printk("error:idx=0x%04x, err_mask=0x%04x\n", idx, err_mask);
	if (i < eccsize) {
		data0[idx] ^= err_mask;
	}
}

/**
 * jzsoc_nand_bch_correct_data
 * @mtd:	mtd info structure
 * @dat:        data to be corrected
 * @read_ecc:   pointer to ecc buffer calculated when nand writing
 * @calc_ecc:   no used
 */
static int jzsoc_nand_bch_correct_data(struct mtd_info *mtd, u_char *dat, u_char *read_ecc, u_char *calc_ecc)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int eccsize = ((struct buf_be_corrected *)dat)->eccsize;
	int eccbytes = this->ecc.bytes;
	int ecc_pos = this->eccpos;
	short k;
	u32 errcnt, errtotal, stat;
	int i;

	if (eccsize != ecc_pos)
		eccsize = this->ecc.size;

#if defined(BCH_DR_BYTE)
	/* Write data to REG_BCH_DR8 */
	for (k = 0; k < eccsize; k++) {
		REG_BCH_DR8 = ((struct buf_be_corrected *)dat)->data[k];
	}
#elif defined(BCH_DR_HWORD)
	/* Write data to REG_BCH_DR16 */
	for (k = 0; k < (eccsize >> 1); k++) {
		REG_BCH_DR16 = ((unsigned short *)((struct buf_be_corrected *)dat)->data)[k];
	}
#elif defined(BCH_DR_WORD)
	/* Write data to REG_BCH_DR32 */
	for (k = 0; k < (eccsize >> 2); k++) {
		REG_BCH_DR32 = ((unsigned int *)((struct buf_be_corrected *)dat)->data)[k];
	}
#endif

	/* Write parities to REG_BCH_DR8 */
	for (k = 0; k < eccbytes; k++) {
		REG_BCH_DR8 = read_ecc[k];
	}

#if MISC_DEBUG
	//		buffer_dump(((struct buf_be_corrected *)dat)->data, eccsize, "data",
	//				 __FILE__, __func__, __LINE__);
	//		buffer_dump(read_ecc, eccbytes, "par", __FILE__, __func__, __LINE__);
#endif

	/* Wait for completion */
	__bch_decode_sync();

	/* Check decoding */
	stat = REG_BCH_INTS;

	if (stat & BCH_INTS_ERR) {
		/* Error occurred */
		if (stat & BCH_INTS_UNCOR) {
			printk("NAND: Uncorrectable ECC error--\n");
			return -1;	
		}else{
			errcnt = (stat & BCH_INTS_ERRC_MASK) >> BCH_INTS_ERRC_BIT;
			errtotal = (stat & BCH_INTS_TERRC_MASK) >> BCH_INTS_TERRC_BIT;

			printk(">>>have detected errors,INTS is 0x%08x,errcnt = %d\n", REG_BCH_INTS, errcnt);
			for (i = 0; i < errcnt; i++)
				bch_error_correct(mtd, dat, i);
		}
	}	
	__bch_decints_clear();
	__bch_disable();

	return 0;
}

static int jzsoc_nand_calculate_bch_ecc(struct mtd_info *mtd, const u_char * dat, u_char * ecc_code)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int eccsize = this->ecc.size;
	int eccbytes = this->ecc.bytes;
	volatile u8 *paraddr = (volatile u8 *)BCH_PAR0;
	short i;

#if defined(BCH_DR_BYTE)
	/* Write data to REG_BCH_DR8 */
	for (i = 0; i < eccsize; i++) {
		REG_BCH_DR8 = ((struct buf_be_corrected *)dat)->data[i];
	}
#elif defined(BCH_DR_HWORD)
	/* Write data to REG_BCH_DR16 */
	for (i = 0; i < (eccsize >> 1); i++) {
		REG_BCH_DR16 = ((unsigned short *)((struct buf_be_corrected *)dat)->data)[i];
	}
#elif defined(BCH_DR_WORD)
	/* Write data to REG_BCH_DR32 */
	for (i = 0; i < (eccsize >> 2); i++) {
		REG_BCH_DR32 = ((unsigned int *)((struct buf_be_corrected *)dat)->data)[i];
	}
#endif
	__bch_encode_sync();

	for (i = 0; i < eccbytes; i++) {
		ecc_code[i] = *paraddr++;
	}

	__bch_encints_clear();
	__bch_disable();
	
	return 0;
}

static int nand_oob_hwecc_bchenc(struct mtd_info *mtd, struct nand_chip *chip)
{
	int ecc_pos = chip->eccpos;
	u8 *oobbuf = chip->oob_poi;
	u8 *parbuf = oobbuf + ecc_pos;
	volatile u8 *paraddr = (volatile u8 *)BCH_PAR0;
	int eccbytes = chip->ecc.bytes;
	int count;

	chip->ecc.hwctl(mtd, NAND_WRITE_OOB);

	for(count = 0; count < ecc_pos; count++)
		REG_BCH_DR8 = *oobbuf++;

	__bch_encode_sync();
	__bch_disable();

	for (count = 0; count < eccbytes; count++)
		*parbuf++ = *paraddr++;

	return 0;
}

#if defined(CONFIG_MTD_NAND_DMA)



#else	/* nand write in cpu mode */


#if defined(USE_BCH)
static void nand_write_page_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps / chip->planenum;
	int ecc_pos = chip->eccpos;
	int oob_per_eccsize = ecc_pos / eccsteps;
	int oobsize = mtd->oobsize / chip->planenum;
	int ecctotal = chip->ecc.total / chip->planenum;
	uint8_t *p = (uint8_t *)buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	static struct buf_be_corrected buf_calc0;
	struct buf_be_corrected *buf_calc = &buf_calc0;

	for (i = 0; i < eccsteps; i++, p += eccsize) {
		buf_calc->data = (u8 *)buf + eccsize * i;
		buf_calc->oob = chip->oob_poi + oob_per_eccsize * i;
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->ecc.calculate(mtd, (u8 *)buf_calc, &ecc_calc[eccbytes*i]);
		chip->write_buf(mtd, p, eccsize);
	}

	for (i = 0; i < ecctotal; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	chip->write_buf(mtd, chip->oob_poi, oobsize);
}
#else
static void nand_write_page_nobch(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf)
{
	int pagesize = mtd->rl_writesize;

	chip->write_buf(mtd, buf, rl_writesize)
}
#endif

#if 0
/* nand write using two-plane mode */
static void nand_write_page_hwecc_bch_planes(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf)
{
	int pagesize = mtd->rl_writesize >> 1;
	int ppb = mtd->rl_erasesize / mtd->rl_writesize;
	int page;

	page = (global_page / ppb) * ppb + global_page; /* = global_page%ppb + (global_page/ppb)*ppb*2 */

	/* send cmd 0x80, the MSB should be valid if realplane is 4 */
	if (chip->realplanenum == 2)
	{
		if(global_mafid == 0x2c)
			chip->cmdfunc(mtd, 0x80, 0x00, page);
		else
			chip->cmdfunc(mtd, 0x80, 0x00, 0x00);
	}
	else
		chip->cmdfunc(mtd, 0x80, 0x00, page & (1 << (chip->chip_shift - chip->page_shift)));

	nand_write_page_hwecc_bch(mtd, chip, buf);

	chip->cmdfunc(mtd, 0x11, -1, -1); /* send cmd 0x11 */
	ndelay(100);
	while(!chip->dev_ready(mtd));

	chip->cmdfunc(mtd, 0x81, 0x00, page + ppb); /* send cmd 0x81 */
	nand_write_page_hwecc_bch(mtd, chip, buf + pagesize);
}
#endif
#endif /* CONFIG_MTD_NAND_DMA */

static void bch_decode_oob(struct mtd_info *mtd, struct nand_chip *chip)
{
	int ecc_pos = chip->eccpos;
	static struct buf_be_corrected buf_correct0;
	struct buf_be_corrected *buf_correct = &buf_correct0;
	int stat;
	u8 *oobbuf;

	oobbuf = chip->oob_poi;
	buf_correct->data = oobbuf;
	buf_correct->eccsize = ecc_pos;
	chip->ecc.hwctl(mtd, NAND_READ_OOB);
	stat = chip->ecc.correct(mtd, (u8 *)buf_correct, oobbuf + ecc_pos, NULL);
	if (stat < 0) {
		printk("OOB:ecc Uncorrectable:global_page = %d,chip->planenum = %d\n",global_page,chip->planenum);
		mtd->ecc_stats.failed++;
	} else {
		mtd->ecc_stats.corrected += stat;
	}
}

/**
 * nand_read_page_hwecc_bch - [REPLACABLE] hardware ecc based page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * Not for syndrome calculating ecc controllers which need a special oob layout
 */
#if defined(CONFIG_MTD_NAND_DMA)
#else	/* nand read in cpu mode */

#if defined(USE_BCH)
static int nand_read_page_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf)
{
	int eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps / chip->planenum;
	int ecc_pos = chip->eccpos;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	int pagesize = mtd->rl_writesize / chip->planenum;
	int oobsize = mtd->oobsize / chip->planenum;
	int freesize = mtd->freesize / chip->planenum;
	int ecctotal = chip->ecc.total / chip->planenum;
	static struct buf_be_corrected buf_correct0;
	int data_per_page = mtd->writesize;	
	int i, j;

	dprintk("\nchip->planenum:%d eccsteps:%d eccsize:%d eccbytes:%d ecc_pos:%d pagesize:%d oobsize:%d \
			ecctotal:%d data_per_page:%d\n",chip->planenum,eccsteps,eccsize,
			eccbytes,ecc_pos,pagesize,oobsize,ecctotal,data_per_page);

#ifndef DEBUG_PN

#ifdef CFG_NAND_PN
	REG32(0xb3410100) = 0x03;
#endif
	chip->read_buf(mtd, buf, pagesize);
	chip->read_buf(mtd, chip->oob_poi, oobsize);

#else
	printk(">>>no pn read page:\n");
	chip->read_buf(mtd, buf, pagesize);
	chip->read_buf(mtd, chip->oob_poi, oobsize);
	mem_dump(buf, 1024);
#endif


#ifdef CFG_NAND_PN
	REG32(0xb3410100) = 0;
#endif

#if 0
	printk(">>>i have make 2 faults in ecc.read()\n");
	buf[0] ^= 0x01;	
	buf[4] ^= 0x01;
#endif

	if (!freesize) {
		for (i = 0; i < ecctotal; i++) {
			ecc_code[i] = chip->oob_poi[eccpos[i]];
		}
	} else {
		for (i = 0; i < oobsize - ecc_pos; i++) {
			ecc_code[i] = chip->oob_poi[ecc_pos + i];
		}
		for (j = 0; j < ecctotal - oobsize + ecc_pos; j++) {
			ecc_code[i + j] = buf[data_per_page + j];
		}
	}

	for (i = 0; i < eccsteps; i++) {
		int stat;
		struct buf_be_corrected *buf_correct = &buf_correct0;

		buf_correct->data = buf + eccsize * i;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
//		stat = chip->ecc.correct(mtd, (u8 *)buf_correct, &ecc_code[eccbytes * i + 1],
//				&ecc_calc[eccbytes * i + 1]);
		stat = chip->ecc.correct(mtd, (u8 *)buf_correct, &ecc_code[eccbytes * i],
				&ecc_calc[eccbytes * i]);
		if (stat < 0)
		{
			printk("ecc Uncorrectable:global_page = %d,chip->planenum = %d\n",global_page,chip->planenum);
			mtd->ecc_stats.failed++;
		}
		else
			mtd->ecc_stats.corrected += stat;
	}

	//	bch_decode_oob(mtd, chip);

	return 0;
}
#else
static int nand_read_page_no_bch(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf)
{
	int pagesize = mtd->writesize;

	chip->read_buf(mtd, buf, pagesize);
}
#endif

static int nand_read_oob_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
	int oobsize = mtd->oobsize / chip->planenum;

	memset(chip->oob_poi,0x00,mtd->oobsize);

	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		sndcmd = 0;
	}

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
#endif

	chip->read_buf(mtd, chip->oob_poi, oobsize);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN |  NAND_CTRL_CHANGE);
#endif

	/* BCH decoding OOB */
//	bch_decode_oob(mtd, chip);

	return sndcmd;
}

#if 0
static int nand_read_page_hwecc_bch_planes(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf)
{
	int pagesize = mtd->rl_writesize >> 1;
	int ppb = mtd->rl_erasesize / mtd->rl_writesize;
	uint32_t page;

	page = (global_page / ppb) * ppb + global_page; /* = global_page%ppb + (global_page/ppb)*ppb*2 */

	/* Read first page */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);
	nand_read_page_hwecc_bch(mtd, chip, buf);

	/* Read 2nd page */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page + ppb);
	nand_read_page_hwecc_bch(mtd, chip, buf+pagesize);
	return 0;
}
#endif
#endif /* CONFIG_MTD_NAND_DMA */

#endif /* CONFIG_MTD_HW_BCH_ECC */

#if 0
/* read oob using two-plane mode */
static int nand_read_oob_std_planes(struct mtd_info *mtd, struct nand_chip *chip,
		int global_page, int sndcmd)
{
	int page;
	int oobsize = mtd->oobsize >> 1;
	int ppb = mtd->rl_erasesize / mtd->rl_writesize;

	page = (global_page / ppb) * ppb + global_page; /* = global_page%ppb + (global_page/ppb)*ppb*2 */

	/* Read first page OOB */
	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	}
	chip->read_buf(mtd, chip->oob_poi, oobsize);

	/* Read second page OOB */
	page += ppb;
	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		sndcmd = 0;
	}
	chip->read_buf(mtd, chip->oob_poi+oobsize, oobsize);

	return 0;
}

/* write oob using two-plane mode */
static int nand_write_oob_std_planes(struct mtd_info *mtd, struct nand_chip *chip,
		int global_page)
{
	int status = 0, page;
	const uint8_t *buf = chip->oob_poi;
	int pagesize = mtd->rl_writesize >> 1;
	int oobsize = mtd->oobsize >> 1;
	int ppb = mtd->rl_erasesize / mtd->rl_writesize;

	page = (global_page / ppb) * ppb + global_page; /* = global_page%ppb + (global_page/ppb)*ppb*2 */

	/* send cmd 0x80, the MSB should be valid if realplane is 4 */
	if (chip->realplanenum == 2)
	{
		if(global_mafid == 0x2c)
			chip->cmdfunc(mtd, 0x80, pagesize, page);
		else
			chip->cmdfunc(mtd, 0x80, pagesize, 0x00);
	}
	else
		chip->cmdfunc(mtd, 0x80, pagesize, page & (1 << (chip->chip_shift - chip->page_shift)));

	chip->write_buf(mtd, buf, oobsize);
	/* Send first command to program the OOB data */
	chip->cmdfunc(mtd, 0x11, -1, -1);
	ndelay(100);
	status = chip->waitfunc(mtd, chip);

	page += ppb;
	buf += oobsize;
	chip->cmdfunc(mtd, 0x81, pagesize, page);
	chip->write_buf(mtd, buf, oobsize);
	/* Send command to program the OOB data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	/* Wait long R/B */
	ndelay(100);
	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/* nand erase using two-plane mode */
static void single_erase_cmd_planes(struct mtd_info *mtd, int global_page)
{
	struct nand_chip *chip = mtd->priv;
	int page, ppb = mtd->rl_erasesize / mtd->rl_writesize;

	page = (global_page / ppb) * ppb + global_page; /* = global_page%ppb + (global_page/ppb)*ppb*2 */

	/* send cmd 0x60, the MSB should be valid if realplane is 4 */
	if (chip->realplanenum == 2)
	{
		if(global_mafid == 0x2c)
			chip->cmdfunc(mtd, 0x60, -1, page);
		else
			chip->cmdfunc(mtd, 0x60, -1, 0x00);
	}
	else
		chip->cmdfunc(mtd, 0x60, -1, page & (1 << (chip->chip_shift - chip->page_shift)));

	page += ppb;
	chip->cmdfunc(mtd, 0x60, -1, page & (~(ppb-1))); /* send cmd 0x60 */

	chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1); /* send cmd 0xd0 */
	/* Do not need wait R/B or check status */
}
#endif

extern int nemc_bch_test(struct mtd_info *mtd);

/*
 * Main initialization routine
 */
int __init jznand_init(void)
{
	struct nand_chip *this;
	int nr_partitions, ret, i;
	char *databuf;

	printk(KERN_INFO "JZ NAND init");
#if defined(CONFIG_MTD_NAND_DMA)
#if defined(CONFIG_MTD_NAND_DMABUF)
	printk(KERN_INFO " DMA mode, using DMA buffer in NAND driver.\n");
#else
	printk(KERN_INFO " DMA mode, using DMA buffer in upper layer.\n");
#endif
#else
	printk(KERN_INFO " CPU mode.\n");
#endif

	databuf = kmalloc(4096, GFP_KERNEL);
//	cpm_start_clock(32);
#if 0
	/* start bdma channel 0 & 1 */
	REG_BDMAC_DMACKES = 0x3;
#endif

#if 1
	//disable nor flash cs2 rd we
	REG_GPIO_PXINTC(0) = 0x00430000;
	REG_GPIO_PXMASKC(0) = 0x00430000;
	REG_GPIO_PXPAT1C(0) = 0x00430000;
	REG_GPIO_PXPAT0C(0) = 0x00430000;
#endif

	/* Allocate memory for MTD device structure and private data */
	jz_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	if (!jz_mtd) {
		printk("Unable to allocate JzSOC NAND MTD device structure.\n");
		return -ENOMEM;
	}

#if 0
	/* Allocate memory for NAND when using only one plane */
	jz_mtd1 = kmalloc(sizeof(struct mtd_info) + sizeof (struct nand_chip), GFP_KERNEL);
	if (!jz_mtd1) {
		printk ("Unable to allocate JzSOC NAND MTD device structure 1.\n");
		kfree(jz_mtd);
		return -ENOMEM;
	}
#endif
	jz_mtd1 = NULL;

	/* Get pointer to private data */
	this = (struct nand_chip *)(&jz_mtd[1]);

	/* Initialize structures */
	memset((char *)jz_mtd, 0, sizeof(struct mtd_info));
	memset((char *)this, 0, sizeof(struct nand_chip));

#ifdef CONFIG_MTD_NAND_BUS_WIDTH_16
	this->options |= NAND_BUSWIDTH_16;
#endif

	/* Link the private data with the MTD structure */
	jz_mtd->priv = this;


	addr_offset = NAND_ADDR_OFFSET;
	cmd_offset = NAND_CMD_OFFSET;

	/* Set & initialize NAND Flash controller */
	jz_device_setup();

	/* Set address of NAND IO lines to static bank1 by default */
	this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT1;
	this->IO_ADDR_W = (void __iomem *)NAND_DATA_PORT1;

#ifdef CFG_NAND_TOGGLE
	/* enable toggle nand */
	this->cmd_ctrl = jz_hwcontrol_toggle;
#else
	this->cmd_ctrl = jz_hwcontrol_common;
#endif

	this->dev_ready = jz_device_ready;

#ifdef CONFIG_MTD_HW_BCH_ECC
	this->ecc.calculate = jzsoc_nand_calculate_bch_ecc;
	this->ecc.correct = jzsoc_nand_bch_correct_data;
	this->ecc.hwctl = jzsoc_nand_enable_bch_hwecc;
	this->ecc.mode = NAND_ECC_HW;

#ifndef CFG_NAND_TOGGLE
	this->ecc.size = 512;
#else
	this->ecc.size = 1024;
#endif
	this->ecc.read_page = nand_read_page_hwecc_bch;
	this->ecc.write_page = nand_write_page_hwecc_bch;

	this->ecc.read_oob = nand_read_oob_hwecc_bch;
	//	this->ecc.write_oob = nand_write_oob_hwecc_bch;
	this->ecc.bytes = PAR_SIZE;
#endif

#ifdef  CONFIG_MTD_SW_HM_ECC
	this->ecc.mode = NAND_ECC_SOFT;
#endif
	/* 20 us command delay time */
	this->chip_delay = 20;

	/* Scan to find existance of the device */
	ret = nand_scan_ident(jz_mtd, NAND_MAX_CHIPS);

	/* if real writesize is smaller */
	if(jz_mtd->rl_writesize <= 512)
	{
		this->ecc.size = 512;
	}

#if 0
	if (!ret) {
		if (this->planenum == 2) {
			/* reset nand functions */
			this->erase_cmd = single_erase_cmd_planes;
			this->ecc.read_page = nand_read_page_hwecc_bch_planes;
			this->ecc.write_page = nand_write_page_hwecc_bch_planes;
			this->ecc.read_oob = nand_read_oob_std_planes;
			this->ecc.write_oob = nand_write_oob_std_planes;

			printk(KERN_INFO "Nand using two-plane mode, "
					"and resized to rl_writesize:%d oobsize:%d blocksize:0x%x \n",
					jz_mtd->rl_writesize, jz_mtd->oobsize, jz_mtd->rl_erasesize);
		}
	}
#endif

	/* Determine whether all the partitions will use multiple planes if supported */
	nr_partitions = sizeof(partition_info) / sizeof(struct mtd_partition);
	all_use_planes = 1;
	for (i = 0; i < nr_partitions; i++) {
		all_use_planes &= partition_info[i].use_planes;
	}

	if (!ret)
		ret = nand_scan_tail(jz_mtd);

	if (ret){
#if 0
		kfree (jz_mtd1);
#endif 
		kfree (jz_mtd);
		return -ENXIO;
	}

#if 0
	((struct nand_chip *) (&jz_mtd1[1]))->ecc.read_page = nand_read_page_hwecc_bch;
	((struct nand_chip *) (&jz_mtd1[1]))->ecc.write_page = nand_write_page_hwecc_bch;
#endif

	/* Register the partitions */
	printk ("Creating %d MTD partitions on \"%s\":\n", nr_partitions, jz_mtd->name);
	printk (KERN_NOTICE "Creating %d MTD partitions on \"%s\":\n", nr_partitions, jz_mtd->name);

	calc_partition_size(jz_mtd);

#if 0
	if ((this->planenum == 2) && !all_use_planes) {
		for (i = 0; i < nr_partitions; i++) {
			if (partition_info[i].use_planes)
				add_mtd_partitions(jz_mtd, &partition_info[i], 1);
			else
				add_mtd_partitions(jz_mtd1, &partition_info[i], 1);
		}
	} else {
		kfree(jz_mtd1);
		add_mtd_partitions(jz_mtd, partition_info, nr_partitions);
	}
#else
	add_mtd_partitions(jz_mtd, partition_info, nr_partitions);
#endif

#ifdef DEBUG_TEST_NAND
	printk("\n******************now go in nemc_bch_test()*******************\n");
	/* TEST NAND */
	nemc_bch_test(jz_mtd);
#endif
	return 0;
}

module_init(jznand_init);

/*
 * Clean up routine
 */
#ifdef MODULE


static void __exit jznand_cleanup(void)
{
#if defined(CONFIG_MTD_NAND_DMA)
#endif

	/* Unregister partitions */
	del_mtd_partitions(jz_mtd);

	/* Unregister the device */
	del_mtd_device(jz_mtd);

	/* Free the MTD device structure */
	if ((this->planenum == 2) && !all_use_planes)
		kfree (jz_mtd1);
	kfree(jz_mtd);
}

module_exit(jznand_cleanup);
#endif
