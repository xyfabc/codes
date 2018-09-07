/*
 * linux/drivers/mtd/nand/jz4775_nand_ubi.c
 *
 * JZ4775 NAND driver
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
#define USE_PN		0

#define NAND_DEBUG	0
#define BCH_DEBUG	0
#define DMA_DEBUG	0
#define DEBUG_LEVEL	0

#if NAND_DEBUG
#define	dprintk(n,x...) printk(n,##x)
#else
#define	dprintk(n,x...)
#endif
#if DEBUG_LEVEL
#define DBG1(n,x...)	printk(n,##x)
#else
#define DBG1(n,x...)
#endif

#if BCH_DEBUG
#define	bch_printk(n,x...) printk(n,##x)
#else
#define	bch_printk(n,x...)
#endif

#if DMA_DEBUG
#define	dma_printk(n,x...) printk(n,##x)
#else
#define	dma_printk(n,x...)
#endif

#define BCH_DR_BYTE	1
//#define BCH_DR_HWORD	1
//#define BCH_DR_WORD	1

#if defined(BCH_DR_BYTE) || defined(BCH_DR_HWORD) || defined(BCH_DR_WORD)
#else
#define BCH_DR_BYTE 	1
#endif

#define BCH_BIT		CONFIG_MTD_HW_BCH_BIT
#define PAR_SIZE	(14 * BCH_BIT / 8)

#define NAND_DATA_PORT1		0xBB000000	/* read-write area in static bank 1 */
#define NAND_DATA_PORT2		0xBA000000	/* read-write area in static bank 2 */
#define NAND_DATA_PORT3		0xB9000000	/* read-write area in static bank 3 */

#define NAND_ADDR_OFFSET	0x00800000      /* address port offset for unshare mode */
#define NAND_CMD_OFFSET		0x00400000      /* command port offset for unshare mode */

#define PDMA_FW_TCSM            0xB3422000
#define PDMA_MSG_CHANNEL        3

/* For DMA debug */
#define PDMA_BANK0              0xB3422000
#define PDMA_BANK1              0xB3423000
#define PDMA_BANK2              0xB3424000
#define PDMA_BANK3              0xB3425000
#define PDMA_BANK4              0xB3426000
#define PDMA_BANK5              0xB3427000
#define PDMA_MSG_TCSMVA         PDMA_BANK5
#define PDMA_MSG_TCSMPA         PDMA_MSG_TCSMVA & 0x1FFFFFFF

struct buf_be_corrected {
	u8 *data;
	u8 *oob;
	u8 eccsize; 
};

#if defined(CONFIG_MTD_NAND_DMA)
DECLARE_WAIT_QUEUE_HEAD(nand_dma_wait_queue);

static volatile int dma_mailbox_ack = 0;
extern unsigned int pdma_fw[7 * 1024 / 4];
static void request_pdma_nand(struct pdma_msg *msg, unsigned int tcsm_pa);
#endif

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
static struct mtd_partition partition_info[] = {
	{name:"NAND BOOT partition",
	  rl_offset:0 * 0x100000LL,
	  rl_size:4 * 0x100000LL,
	  use_planes: 0},
	{name:"NAND KERNEL partition",
	  rl_offset:4 * 0x100000LL,
	  rl_size:4 * 0x100000LL,
	  use_planes: 0},
	{name:"NAND UBIFS partition",
	  rl_offset:8 * 0x100000LL,
	  rl_size:504 * 0x100000LL,
	  use_planes: 1},
	{name:"NAND VFAT1 partition",
	  rl_offset:512 * 0x100000LL,
	  rl_size:512 * 0x100000LL,
	  use_planes: 1},
	{name:"NAND VFAT2 partition",
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
	20,			/* reserved blocks of mtd2 */
	20,			/* reserved blocks of mtd3 */
	20,			/* reserved blocks of mtd4 */
};				/* reserved blocks of mtd5 */


/*-------------------------------------------------------------------------
 * Following three functions are exported and used by the mtdblock-jz.c
 * NAND FTL driver only.
 */

extern void buffer_dump(uint8_t *buffer, int length, const char *comment, char *file, char *function, int line);

void mem_dump(uint8_t *buf, int len)
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
		printk("%08x: ", i << 4);

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
EXPORT_SYMBOL(mem_dump);

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

#ifndef CONFIG_MTD_NAND_TOGGLE
/* HWcontrol common nandflash */
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
			default:
				printk("NAND: nand_nce OUT of BANK! => 0x%x\n", nand_nce);
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
		DBG1("write cmd:0x%x to 0x%x\n",dat,(u32)this->IO_ADDR_W);
	}
}

#else

/* HWcontrol toggle nandflash */
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
				break;
			case NAND_NCE2:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT2;
				break;
			case NAND_NCE3:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT3;
				break;
			default:
				printk("NAND: nand_nce OUT of BANK! => 0x%x\n", nand_nce);
			}
		} else {
			REG_NEMC_NFCSR = 0;
		}

		if (ctrl & TNAND_NCE_SET) {
			switch (nand_nce) {
			case NAND_NCE1:
			case NAND_NCE2:
			case NAND_NCE3:
				REG_NEMC_NFCSR = NEMC_NFCSR_TNFE(nand_nce) | NEMC_NFCSR_NFE(nand_nce);
				break;
			default:
				printk("NAND: nand_nce OUT of BANK! => 0x%x\n", nand_nce);
			}
		}

		if (ctrl & NAND_FCE_SET) {
			switch (nand_nce) {
			case NAND_NCE1:
			case NAND_NCE2:
			case NAND_NCE3:
				__tnand_dphtd_sync(nand_nce);
				REG_NEMC_NFCSR |= NEMC_NFCSR_DAEC | NEMC_NFCSR_NFCE(nand_nce);
//				__tnand_dae_clr();
				break;
			default:
				printk("NAND: nand_nce OUT of BANK! => 0x%x\n", nand_nce);
			}
		}

		if (ctrl & NAND_FCE_CLEAN) {
			switch (nand_nce) {
			case NAND_NCE1:
			case NAND_NCE2:
			case NAND_NCE3:
				REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE(nand_nce);
				__tnand_dphtd_sync(nand_nce);
				break;
			default:
				printk("NAND: nand_nce OUT of BANK! => 0x%x\n", nand_nce);
			}
		}

		/* toggle nand read preparation */
		if (ctrl & TNAND_READ_PERFORM){
			REG_NEMC_TGWE |= NEMC_TGWE_DAE;
			__tnand_dae_sync();
		}

		/* toggle nand write preparation */
		if (ctrl & TNAND_WRITE_PERFORM){
			switch (nand_nce) {
			case NAND_NCE1:	
			case NAND_NCE2:
			case NAND_NCE3:
				REG_NEMC_TGWE |= NEMC_TGWE_DAE | NEMC_TGWE_SDE(nand_nce);
				__tnand_dae_sync();
				__tnand_wcd_sync();
				break;
			default:
				printk("NAND: nand_nce OUT of BANK! => 0x%x\n", nand_nce);
			}
		}

		if (ctrl & TNAND_DAE_CLEAN) {
			switch (nand_nce) {
			case NAND_NCE1:	
			case NAND_NCE2:
			case NAND_NCE3:
				REG_NEMC_NFCSR |= NEMC_NFCSR_DAEC;
				__tnand_dae_clr();
				break;
			default:
				printk("NAND: nand_nce OUT of BANK! => 0x%x\n", nand_nce);
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
	}

	this->IO_ADDR_W = (void __iomem *)nandaddr;
	if (dat != NAND_CMD_NONE) {
		writeb(dat, this->IO_ADDR_W);
		DBG1("write cmd:0x%x to 0x%x\n",dat,(u32)this->IO_ADDR_W); 
	}
}
#endif /* CONFIG_MTD_NAND_TOGGLE */

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
#define GPIO_CS2_N (32*0+22)
#define GPIO_CS3_N (32*0+23)

#if 1
	/* disable nor flash cs2 rd we */
	REG_GPIO_PXINTC(0) = 0x00430000;
	REG_GPIO_PXMASKC(0) = 0x00430000;
	REG_GPIO_PXPAT1C(0) = 0x00430000;
	REG_GPIO_PXPAT0C(0) = 0x00430000;
#endif

#if 0
	__gpio_as_output1(182);
#endif

#ifdef CONFIG_MTD_NAND_BUS_WIDTH_16
//#define SMCR_VAL   0x0d444440
#define SMCR_VAL   0x0fff7740  //slowest
	__gpio_as_nand_16bit(1);
#else
//#define SMCR_VAL   0x0d444400
#define SMCR_VAL   0x0fff7700  //slowest
	__gpio_as_nand_8bit(1);
#endif

#ifdef CONFIG_MTD_NAND_TOGGLE
	__gpio_as_nand_toggle();
	REG_NEMC_TGDR = 0x1D;
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
}

#ifdef CONFIG_MTD_HW_BCH_ECC
static void jzsoc_nand_enable_bch_hwecc(struct mtd_info *mtd, int mode)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int ecc_size = this->ecc.size;

	switch (mode) {
	case NAND_READ_OOB :
		ecc_size = this->eccpos;
	case NAND_ECC_READ :
		__bch_cnt_set(ecc_size, PAR_SIZE);
		__bch_decoding(BCH_BIT);
		break;

	case NAND_WRITE_OOB :
		ecc_size = this->eccpos;
	case NAND_ECC_WRITE :
		__bch_cnt_set(ecc_size, PAR_SIZE);
		__bch_encoding(BCH_BIT);
		break;
	}
}

/**
 * bch_correct
 * @mtd:
 * @dat:        data to be corrected
 * @err_hword:	BCH Error Half-Word in ecc_size indicate
 */
static void bch_error_correct(struct mtd_info *mtd, u16 *databuf, int err_bit)
{
	u32 err_mask, idx; /* indicates an error half_word */

	idx = (REG_BCH_ERR(err_bit) & BCH_ERR_INDEX_MASK) >> BCH_ERR_INDEX_BIT;
	err_mask = (REG_BCH_ERR(err_bit) & BCH_ERR_MASK_MASK) >> BCH_ERR_MASK_BIT;

	dprintk("NAND Error:idx=0x%04x, err_mask=0x%04x\n", idx, err_mask);
	databuf[idx] ^= (u16)err_mask;
}

/**
 * jzsoc_nand_bch_correct_data
 * @mtd:	mtd info structure
 * @dat:        data to be corrected
 * @read_ecc:   pointer to ecc buffer calculated when nand writing
 * @calc_ecc:   no used
 */
static int jzsoc_nand_bch_correct_data(struct mtd_info *mtd, u_char *buf, u_char *read_ecc, u_char *calc_ecc)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int eccsize = ((struct buf_be_corrected *)buf)->eccsize;
	int eccbytes = this->ecc.bytes;
	int ecc_pos = this->eccpos;
	u32 errcnt, errtotal, state;
	int i, k;
	int ret_val = 0;

	if (eccsize != ecc_pos)
		eccsize = this->ecc.size;

#if defined(BCH_DR_BYTE)
	/* Write data to REG_BCH_DR8 */
	for (k = 0; k < eccsize; k++) {
		REG_BCH_DR8 = ((struct buf_be_corrected *)buf)->data[k];
	}
#elif defined(BCH_DR_HWORD)
	/* Write data to REG_BCH_DR16 */
	for (k = 0; k < (eccsize >> 1); k++) {
		REG_BCH_DR16 = ((u16 *)((struct buf_be_corrected *)buf)->data)[k];
	}
#elif defined(BCH_DR_WORD)
	/* Write data to REG_BCH_DR32 */
	for (k = 0; k < (eccsize >> 2); k++) {
		REG_BCH_DR32 = ((u32 *)((struct buf_be_corrected *)buf)->data)[k];
	}
#endif

	/* Write parities to REG_BCH_DR8 */
	for (k = 0; k < eccbytes; k++) {
		REG_BCH_DR = read_ecc[k];
	}

#if BCH_DEBUG
	buffer_dump(((struct buf_be_corrected *)buf)->data, eccsize, "data",
			__FILE__, __func__, __LINE__);
	buffer_dump(read_ecc, eccbytes, "par", __FILE__, __func__, __LINE__);
#endif

	/* Wait for completion */
	__bch_decode_sync();

	/* Check decoding */
	state = REG_BCH_INTS;

	if (state & BCH_INTS_UNCOR) {
		/* Error occurred */
		printk("NAND: Uncorrectable ECC error--\n");
		ret_val = -1;
	} else if (state & BCH_INTS_ERR) {
		errcnt = (state & BCH_INTS_ERRC_MASK) >> BCH_INTS_ERRC_BIT;
		errtotal = (state & BCH_INTS_TERRC_MASK) >> BCH_INTS_TERRC_BIT;

		dprintk("NAND detected errors,INTS is 0x%08x,errcnt = %d\n", REG_BCH_INTS, errcnt);
		for (i = 0; i < errcnt; i++)
			bch_error_correct(mtd, (u16 *)((struct buf_be_corrected *)buf)->data, i);
	}

	__bch_decints_clear();
	__bch_disable();

	return ret_val < 0 ? -1 : 0;
}

static int jzsoc_nand_calculate_bch_ecc(struct mtd_info *mtd, const u_char * buf, u_char * ecc_code)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	int eccsize = ((struct buf_be_corrected *)buf)->eccsize;
	int eccbytes = this->ecc.bytes;
	int ecc_pos = this->eccpos;
	volatile u8 *paraddr = (volatile u8 *)BCH_PAR0;
	short i;

	if (eccsize != ecc_pos)
		eccsize = this->ecc.size;

#if defined(BCH_DR_BYTE)
	/* Write data to REG_BCH_DR8 */
	for (i = 0; i < eccsize; i++) {
		REG_BCH_DR8 = ((struct buf_be_corrected *)buf)->data[i];
	}
#elif defined(BCH_DR_HWORD)
	/* Write data to REG_BCH_DR16 */
	for (i = 0; i < (eccsize >> 1); i++) {
		REG_BCH_DR16 = ((unsigned short *)((struct buf_be_corrected *)buf)->data)[i];
	}
#elif defined(BCH_DR_WORD)
	/* Write data to REG_BCH_DR32 */
	for (i = 0; i < (eccsize >> 2); i++) {
		REG_BCH_DR32 = ((unsigned int *)((struct buf_be_corrected *)buf)->data)[i];
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

#if defined(CONFIG_MTD_NAND_DMA)
static void dump_jz_dma_special(void)
{
	printk("\nDMA NEMC channel\n");
	printk(" DSA:  0x%08x\n", REG_DMAC_DSAR(1));
	printk(" DTA:  0x%08x\n", REG_DMAC_DTAR(1));
	printk(" DRT:  0x%08x\n", REG_DMAC_DRSR(1));
	printk(" DCT:  0x%08x\n", REG_DMAC_DTCR(1));
	printk(" DCCS: 0x%08x\n", REG_DMAC_DCCSR(1));
	printk(" DCMD: 0x%08x\n", REG_DMAC_DCMD(1));

	printk("\nDMA BCH channel\n");
	printk(" DSA:  0x%08x\n", REG_DMAC_DSAR(0));
	printk(" DTA:  0x%08x\n", REG_DMAC_DTAR(0));
	printk(" DRT:  0x%08x\n", REG_DMAC_DRSR(0));
	printk(" DCT:  0x%08x\n", REG_DMAC_DTCR(0));
	printk(" DCCS: 0x%08x\n", REG_DMAC_DCCSR(0));
	printk(" DCMD: 0x%08x\n", REG_DMAC_DCMD(0));

	printk("\n");
}

#if DMA_DEBUG
static void dump_jz_pdma_detail(void)
{
	printk("\nPDMA Status\n");
	printk(" DMAC: 0x%08x\n", REG_DMAC_DMACR);
	printk(" DCIRQP: 0x%08x\n", REG_DMAC_DCIRQP);
	printk(" MCS: 0x%08x\n", REG_DMAC_DMCSR);
	printk(" MNMB: 0x%08x\n", REG_DMAC_DMNMB);
	printk(" MINT: 0x%08x\n", REG_DMAC_DMINT);

	printk("\nNAND Status\n");
	printk(" BCHCR: 0x%08x\n", REG_BCH_CR);
	printk(" BCHCNT: 0x%08x\n", REG_BCH_CNT);
	printk(" BCHINTS: 0x%08x\n", REG_BCH_INTS);
	printk(" NEMCNFCSR: 0x%08x\n", REG_NEMC_NFCSR);
	printk("\n");
}
#endif
#endif

/**
 * nand_write_page_hwecc_bch - [REPLACABLE] hardware ecc based page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * Not for syndrome calculating ecc controllers which need a special oob layout
 */
#if defined(CONFIG_MTD_NAND_DMA)
static void nand_write_page_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf)
{
	struct pdma_msg	msg;
	struct pdma_msg *msg_write = &msg;
	int pagesize = mtd->writesize / chip->planenum;
	int page = global_page;
	unsigned int mb_write;

	dprintk("\nNAND DMA write send ops msg. page: %d\n", page);
	dma_printk("\nNEMC_NFCSR: %08x\n", REG_NEMC_NFCSR);

	/* Fill message structure */
	msg_write->cmd = MSG_NAND_WRITE;
	msg_write->info[MSG_NAND_BANK] = chip->chip_num;
	msg_write->info[MSG_DDR_ADDR] = (u32)buf;
	msg_write->info[MSG_PAGEOFF] = page;

	dma_cache_wback_inv((u32)buf, pagesize);
	request_pdma_nand(msg_write, PDMA_MSG_TCSMPA);

	__dmac_mnmb_get(mb_write);

	dma_printk("NAND write - PDMA_MNMB: %d\n", mb_write);
#if DMA_DEBUG
	dump_jz_dma_special();
	dump_jz_pdma_detail();
#endif
#if 0
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[0]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[1]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[2]);
//	printk("\nmcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[3]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[4]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[5]);


//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[0]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[1]);
//	mem_dump((unsigned char *)PDMA_BANK4, 512);
#endif
}

#else	/* NAND write in CPU mode */

#if defined(USE_BCH)
static void nand_write_page_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps / chip->planenum;
	int oobsize = mtd->oobsize / chip->planenum;
	int freesize = mtd->freesize / chip->planenum;
	int ecc_pos = chip->eccpos;
	uint8_t *p = (uint8_t *)buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	static struct buf_be_corrected buf_calc0;
	struct buf_be_corrected *buf_calc = &buf_calc0;
	
	dprintk("\nchip->planenum:%d eccsteps:%d eccsize:%d eccbytes:%d ecc_pos:%d ",
				chip->planenum,eccsteps,eccsize,eccbytes,ecc_pos);
	dprintk("freesize:%d oobsize:%d\n",
				freesize,oobsize);
#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | TNAND_WRITE_PERFORM | NAND_CTRL_CHANGE);
#endif

#if USE_PN
	__pn_enable();
#endif
	memset(ecc_calc, 0xFF, oobsize + freesize);
	for (i = 0; i < eccsteps; i++, p += eccsize) {
		buf_calc->data = (u8 *)buf + eccsize * i;
		buf_calc->eccsize = eccsize;

		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->ecc.calculate(mtd, (u8 *)buf_calc, &ecc_calc[eccbytes * i]);

		chip->write_buf(mtd, p, eccsize);
	}

	if (freesize)
		chip->write_buf(mtd, &ecc_calc[oobsize - ecc_pos], freesize);

	for (i = 0; i < oobsize - ecc_pos; i++)
		chip->oob_poi[ecc_pos + i] = ecc_calc[i];

#if USE_PN
	__pn_enable();
#endif
	/* Write OOB area */
	chip->write_buf(mtd, chip->oob_poi, oobsize);
#if USE_PN
	__pn_disable();
#endif

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_FCE_CLEAN | NAND_CTRL_CHANGE);	
#endif
}

#else

static void nand_write_page_nobch(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf)
{
	int pagesize = mtd->writesize;

	chip->write_buf(mtd, buf, writesize)
}
#endif /* USE_BCH */
#endif /* CONFIG_MTD_NAND_DMA */

static int nand_write_oob_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip, int page)
{
	int oobsize = mtd->oobsize / chip->planenum;
	int status;

	memset(chip->oob_poi, 0x00, mtd->oobsize);

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif
	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->writesize, page);

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_WRITE_PERFORM | NAND_CTRL_CHANGE);
#endif
	chip->write_buf(mtd, chip->oob_poi, oobsize);
#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#endif
	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

#if 0
/* nand write using two-plane mode */
static void nand_write_page_hwecc_bch_planes(struct mtd_info *mtd, struct nand_chip *chip,
		const uint8_t *buf)
{
	int pagesize = mtd->writesize >> 1;
	int ppb = mtd->erasesize / mtd->writesize;
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

/**
 * nand_read_page_hwecc_bch - [REPLACABLE] hardware ecc based page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * Not for syndrome calculating ecc controllers which need a special oob layout
 */
#if defined(CONFIG_MTD_NAND_DMA)
static int nand_read_page_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf)
{
	struct pdma_msg	msg;
	struct pdma_msg *msg_read = &msg;
	int pagesize = mtd->writesize / chip->planenum;
	int page = global_page;
	unsigned int mb_read;

	dprintk("\nNAND DMA read send ops msg. page: %d\n", page);
	dma_printk("NEMC_NFCSR: %08x\n", REG_NEMC_NFCSR);

	/* Fill message structure */
	msg_read->cmd = MSG_NAND_READ;
	msg_read->info[MSG_NAND_BANK] = chip->chip_num;
	msg_read->info[MSG_DDR_ADDR] = (u32)buf;
	msg_read->info[MSG_PAGEOFF] = page;

	dma_cache_wback_inv((u32)buf, pagesize);
	request_pdma_nand(msg_read, PDMA_MSG_TCSMPA);

	__dmac_mnmb_get(mb_read);
	dma_printk("NAND read - PDMA-MNMB: %d\n", mb_read);
#if DMA_DEBUG
	dump_jz_dma_special();
	dump_jz_pdma_detail();
#endif
#if 0
#if 1
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[0]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[1]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[2]);
//	printk("\nmcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[3]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[4]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[5]);
#endif
#if 0
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[3]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[4]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[5]);

	printk("--- tscm dump\n");
	mem_dump((unsigned char *)PDMA_BANK4, 512);
	mem_dump((unsigned char *)PDMA_BANK5, 512);
	mem_dump((unsigned char *)(PDMA_BANK5 + 1024), 128);
	mem_dump((unsigned char *)PDMA_BANK6, 512);
	mem_dump((unsigned char *)(PDMA_BANK6 + 1024), 128);
#endif
#endif

	if (mb_read == MB_NAND_UNCOR_ECC) {
		printk("NAND: ecc Uncorrectable:global_page = %d,chip->planenum = %d\n",global_page,chip->planenum);
//		mem_dump(chip->oob_poi, 218);
		mtd->ecc_stats.failed++;

		return -1;
	}

	return 0;
}

#else	/* NAND read in CPU mode */

#if defined(USE_BCH)
static int nand_read_page_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf)
{
	int eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps / chip->planenum;
	int ecc_pos = chip->eccpos;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint8_t *ecc_poi = chip->oob_poi + chip->eccpos;
	int pagesize = mtd->writesize / chip->planenum;
	int oobsize = mtd->oobsize / chip->planenum;
	int freesize = mtd->freesize / chip->planenum;
	int ecctotal = chip->ecc.total / chip->planenum;
	static struct buf_be_corrected buf_correct0;
	int data_per_page = mtd->writesize;	
	int i, j;

	dprintk("\nchip->planenum:%d eccsteps:%d eccsize:%d eccbytes:%d ecc_pos:%d\n",
				chip->planenum,eccsteps,eccsize,eccbytes,ecc_pos);
	dprintk("pagesize:%d oobsize:%d ecctotal:%d data_per_page:%d\n",
				pagesize,oobsize,ecctotal,data_per_page);

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | TNAND_READ_PERFORM | NAND_CTRL_CHANGE);
#endif

#if USE_PN
	__pn_enable();
#endif
	/* Read Data area */
	chip->read_buf(mtd, buf, pagesize);
#if USE_PN
	__pn_enable();
#endif
	/* Read OOB area */
	chip->read_buf(mtd, chip->oob_poi, oobsize);
#if USE_PN
	__pn_disable();
#endif

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#endif

	if (!freesize) {
		for (i = 0; i < ecctotal; i++)
			ecc_code[i] = ecc_poi[i];
	} else {
		for (i = 0; i < oobsize - ecc_pos; i++)
			ecc_code[i] = ecc_poi[i];
		for (j = 0; j < ecctotal - oobsize + ecc_pos; j++)
			ecc_code[i + j] = buf[data_per_page + j];
	}

	for (i = 0; i < eccsteps; i++) {
		int state;
		struct buf_be_corrected *buf_correct = &buf_correct0;

		buf_correct->data = buf + eccsize * i;
		buf_correct->eccsize = eccsize;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
#if 0
		state = chip->ecc.correct(mtd, (u8 *)buf_correct, &ecc_code[eccbytes * i + 1],
				&ecc_calc[eccbytes * i + 1]);
#endif
		state = chip->ecc.correct(mtd, (u8 *)buf_correct, &ecc_code[eccbytes * i],
				&ecc_calc[eccbytes * i]);
		if (state < 0) {
			printk("NAND: ecc Uncorrectable:global_page = %d,chip->planenum = %d\n",global_page,chip->planenum);
			mem_dump(buf, 256);
			mem_dump(chip->oob_poi, 218);
			mtd->ecc_stats.failed++;

			return -1;
		} else
			mtd->ecc_stats.corrected += state;
	}

	return 0;
}

#else

static int nand_read_page_no_bch(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf)
{
	int pagesize = mtd->writesize;

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | TNAND_READ_PERFORM | NAND_CTRL_CHANGE);
#endif

	chip->read_buf(mtd, buf, pagesize);
#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#endif

	return 0;
}
#endif /* USE_BCH */
#endif /* CONFIG_MTD_NAND_DMA */

static int nand_read_oob_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
	int oobsize = mtd->oobsize / chip->planenum;

	memset(chip->oob_poi,0x00,mtd->oobsize);

	if (sndcmd) {
#ifdef CONFIG_MTD_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		sndcmd = 0;
	}

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | TNAND_READ_PERFORM | NAND_CTRL_CHANGE);
#endif

	chip->read_buf(mtd, chip->oob_poi, oobsize);

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#endif

	return sndcmd;
}

#if 0
static int nand_read_page_hwecc_bch_planes(struct mtd_info *mtd, struct nand_chip *chip, uint8_t * buf)
{
	int pagesize = mtd->writesize >> 1;
	int ppb = mtd->erasesize / mtd->writesize;
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
#endif /* CONFIG_MTD_HW_BCH_ECC */

#if 0
/* read oob using two-plane mode */
static int nand_read_oob_std_planes(struct mtd_info *mtd, struct nand_chip *chip,
		int global_page, int sndcmd)
{
	int page;
	int oobsize = mtd->oobsize >> 1;
	int ppb = mtd->erasesize / mtd->writesize;

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
	int pagesize = mtd->writesize >> 1;
	int oobsize = mtd->oobsize >> 1;
	int ppb = mtd->erasesize / mtd->writesize;

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
	int page, ppb = mtd->erasesize / mtd->writesize;

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

#if defined(CONFIG_MTD_NAND_DMA)
/*
 * request_pdma_nand -
 */
static void request_pdma_nand(struct pdma_msg *msg, unsigned int tcsm_pa)
{
	int trans_count = sizeof(struct pdma_msg);
	int err; 

	if (!(REG_DMAC_DMACR & DMAC_DMACR_DMAE))
		__dmac_enable();

	REG_DMAC_DTCR(PDMA_MSG_CHANNEL) = trans_count;
	REG_DMAC_DRSR(PDMA_MSG_CHANNEL) = DMAC_DRSR_RS_AUTO;

	REG_DMAC_DSAR(PDMA_MSG_CHANNEL) = CPHYSADDR((u32)msg);
	REG_DMAC_DTAR(PDMA_MSG_CHANNEL) = (u32)tcsm_pa;

	/* Fresh cache and clear flage */
	dma_cache_wback_inv((u32)msg, trans_count);
	dma_mailbox_ack = 0; 

	/* Launch DMA Channel */
	__dmac_channel_launch(PDMA_MSG_CHANNEL);

	do { 
#if 1
		err = wait_event_interruptible_timeout(nand_dma_wait_queue, dma_mailbox_ack, 3 * HZ); 
#else
		err = wait_event_interruptible(nand_dma_wait_queue, dma_mailbox_ack);
#endif
	} while (err == -ERESTARTSYS);

	if (!err) {
		printk("NAND DMA: wait event 3S timeout%d\n", err);
		dump_jz_dma_special();
	}
}

static irqreturn_t nand_mb_irq_handle(int irq, void *dev_id)
{
	volatile int wakeup = 0;

	dprintk("\nMaibox IRQ...\n");
	dprintk("PDMA_DMINT: %08x irq\n", REG_DMAC_DMINT);
	
	if (irq == IRQ_MCU_0) {
		dprintk("pdma MailBox irq, wake up----\n");
		dma_mailbox_ack = 1;
		wake_up_interruptible(&nand_dma_wait_queue);

		wakeup = 0;
	}

	return IRQ_HANDLED;
}

static int nand_msg_dma_init(void)
{
	__dmac_channel_programmable_default();
	__dmac_channel_programmable_set(PDMA_MSG_CHANNEL);

	REG_DMAC_DCCSR(PDMA_MSG_CHANNEL) = DMAC_DCCSR_NDES;

	REG_DMAC_DCMD(PDMA_MSG_CHANNEL) =  DMAC_DCMD_SAI | DMAC_DCMD_DAI |
					DMAC_DCMD_SWDH_32 | DMAC_DCMD_DWDH_32 |
					DMAC_DCMD_DS_AUTO | DMAC_DCMD_TIE;

	return request_irq(IRQ_MCU_0, nand_mb_irq_handle, IRQF_DISABLED, "nand_msg", NULL);
}

/*
 * pdma_mcu_reset_sync - Wait for MCU finish reset initialate
 *
 * NOTE: delay time: 10us * timeout
 */
static int pdma_mcu_reset_sync(void)
{
	volatile int timeout = 100;
	u32 sleep;

	sleep = __dmac_mcu_is_sleep();
	while (!sleep && timeout--) {
		udelay(10);
		sleep = __dmac_mcu_is_sleep();
	}

	return timeout <= 0 ? -1 : 0;
}

static int jz4775_nand_pdma_init(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	struct pdma_msg	msg;
	struct pdma_msg *msg_init = &msg;
        unsigned int mb_info;
	int ret;
#if DMA_DEBUG
	int i;
#endif

	/* Wait PDMA MCU Init finished */
	ret = pdma_mcu_reset_sync();
	if (ret) {
		printk(KERN_WARNING "NAND DMA: MCU Reset Failed!\n");
                return -ENXIO;
	}
	/* Init msg channel */
	ret = nand_msg_dma_init();
	if (ret) {
		printk(KERN_WARNING "NAND DMA: MCU Init irq Failed!\n");
                return -ENXIO;
	}

	/* PDMA Init NAND Info */
	msg_init->cmd = MSG_NAND_INIT;
	msg_init->info[MSG_NANDTYPE] = (chip->options&NAND_BUSWIDTH_16)?NAND_TYPE_16BIT:0;
	msg_init->info[MSG_PAGESIZE] = mtd->writesize / chip->planenum;
	msg_init->info[MSG_OOBSIZE] = mtd->oobsize / chip->planenum;
	msg_init->info[MSG_ROWCYCLE] = chip->rowcycle;

	msg_init->info[MSG_ECCLEVEL] = BCH_BIT;
	msg_init->info[MSG_ECCSIZE] = chip->ecc.size;
	msg_init->info[MSG_ECCBYTES] = PAR_SIZE;
	msg_init->info[MSG_ECCSTEPS] = chip->ecc.steps / chip->planenum;
	//msg_init->info[MSG_ECCSTEPS] = 1;
	msg_init->info[MSG_ECCTOTAL] = chip->ecc.total / chip->planenum;
	msg_init->info[MSG_ECCPOS] = chip->eccpos;

	request_pdma_nand(msg_init, PDMA_MSG_TCSMPA);

	__dmac_mnmb_get(mb_info);
#if DMA_DEBUG
	for (i = 0; i < 11; i++) {
		dma_printk("MCU NAND info %08x\n", ((unsigned int *)PDMA_MSG_TCSMVA)[i]);
	}
#endif

	if (mb_info != MB_NAND_INIT_DONE) {
		printk("NAND DMA: MCU Init Failed:%d\n", mb_info);
		/* Free MCU msg irq */
		free_irq(IRQ_MCU_0, NULL);
		return -ENXIO;
	}
	printk("NAND DMA: MCU Init Success\n");

	return 0;
}

static void nand_pdma_firmware_load(void)
{
	int load_size = ARRAY_SIZE(pdma_fw);
	u32 *dest = (u32 *)(PDMA_FW_TCSM);
	int i;

	/* load fw to pdma tcsm */
	__dmac_mcu_reset1();
	for (i = 0; i < load_size; i++)
		dest[i] = ((u32 *)pdma_fw)[i];

#if 0
	mem_dump((u8 *)pdma_fw, 256);
	printk("tscm dump\n");
	mem_dump((u8 *)dest, 256);
	memset(dest, 0x5a, 1024);
	mem_dump((u8 *)dest, 256);
	while (1);
#endif

#if 0
	for (i = 0; i < load_size; i++) {
		if (dest[i] != ((u32 *)pdma_fw)[i]) {
			printk("%s: read back check fail! i=%d\n", __func__, i);
			break;
		}
	}
#endif

	__dmac_mcu_reset0();
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

#ifdef CONFIG_MTD_NAND_TOGGLE
	printk(KERN_INFO " Toggle NAND.\n");
#endif

#if defined(CONFIG_MTD_NAND_DMA)
	/* Load firmware to MCU TCSM */
	nand_pdma_firmware_load();
#endif
	/* Allocate memory for MTD device structure and private data */
	jz_mtd = kmalloc(sizeof(struct mtd_info) + sizeof(struct nand_chip), GFP_KERNEL);
	if (!jz_mtd) {
		printk("Unable to allocate JzSOC NAND MTD device structure.\n");
		return -ENOMEM;
	}

	/* Allocate memory for NAND when using only one plane */
	jz_mtd1 = kmalloc(sizeof(struct mtd_info) + sizeof (struct nand_chip), GFP_KERNEL);
	if (!jz_mtd1) {
		printk ("Unable to allocate JzSOC NAND MTD device structure 1.\n");
		kfree(jz_mtd);
		return -ENOMEM;
	}

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

	/* enable toggle nand */
#ifdef CONFIG_MTD_NAND_TOGGLE
	this->cmd_ctrl = jz_hwcontrol_toggle;
#else
	this->cmd_ctrl = jz_hwcontrol_common;
#endif

	this->dev_ready = jz_device_ready;

#ifdef CONFIG_MTD_HW_BCH_ECC
	this->ecc.mode = NAND_ECC_HW;
	this->ecc.size = 1024;
	this->ecc.bytes = PAR_SIZE;
	this->ecc.hwctl = jzsoc_nand_enable_bch_hwecc;
	this->ecc.calculate = jzsoc_nand_calculate_bch_ecc;
	this->ecc.correct = jzsoc_nand_bch_correct_data;

	this->ecc.read_page = nand_read_page_hwecc_bch;
	this->ecc.write_page = nand_write_page_hwecc_bch;
	this->ecc.read_oob = nand_read_oob_hwecc_bch;
	this->ecc.write_oob = nand_write_oob_hwecc_bch;
#endif

#ifdef  CONFIG_MTD_SW_HM_ECC
	this->ecc.mode = NAND_ECC_SOFT;
#endif
	/* 20 us command delay time */
	this->chip_delay = 20;

	/* Scan to find existance of the device */
	ret = nand_scan_ident(jz_mtd, NAND_MAX_CHIPS);
	if (ret)
		goto invalid_nand;

	/* if real writesize is smaller */
	if(jz_mtd->writesize <= 512)
		this->ecc.size = 512;

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
				"and resized to writesize:%d oobsize:%d blocksize:0x%x \n",
				jz_mtd->writesize, jz_mtd->oobsize, jz_mtd->erasesize);
		}
	}
#endif

	/* Determine whether all the partitions will use multiple planes if supported */
	nr_partitions = sizeof(partition_info) / sizeof(struct mtd_partition);
	all_use_planes = 1;
	for (i = 0; i < nr_partitions; i++) {
		all_use_planes &= partition_info[i].use_planes;
	}

	ret = nand_scan_tail(jz_mtd);
	if (ret)
		goto invalid_args;	

#if defined(CONFIG_MTD_NAND_DMA)
	printk ("Init DMA MCU:\n");
	ret = jz4775_nand_pdma_init(jz_mtd);
	if (ret)
		goto invalid_pdma;
#endif

//	((struct nand_chip *) (&jz_mtd1[1]))->ecc.read_page = nand_read_page_hwecc_bch;
//	((struct nand_chip *) (&jz_mtd1[1]))->ecc.write_page = nand_write_page_hwecc_bch;
	
	/* Register the partitions */
	printk ("Creating %d MTD partitions on \"%s\":\n", nr_partitions, jz_mtd->name);
	printk (KERN_NOTICE "Creating %d MTD partitions on \"%s\":\n", nr_partitions, jz_mtd->name);

	calc_partition_size(jz_mtd);

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

#ifdef DEBUG_TEST_NAND
	printk("\n******************now go in nemc_bch_test()*******************\n");
	/* TEST NAND */
	nemc_bch_test(jz_mtd);
	while(1);
#endif
	/* Normal return */
	return 0;

#if defined(CONFIG_MTD_NAND_DMA)
invalid_pdma:
	/* Free bbt */
	kfree(this->bbt);
#endif

invalid_args:
	/* Free the MTD buffers */
	if (!(this->options & NAND_OWN_BUFFERS) && !(this->buffers))
		kfree(this->buffers);

invalid_nand:
	/* Free the MTD device structure */
	kfree (jz_mtd1);
	kfree (jz_mtd);
	return -ENXIO;
}

module_init(jznand_init);

/*
 * Clean up routine
 */
#ifdef MODULE


static void __exit jznand_cleanup(void)
{
#if defined(CONFIG_MTD_NAND_DMA)
	/* Free MCU msg irq */
	free_irq(IRQ_MCU_0, NULL);
#endif

	/* Release NAND device */
	nand_release(jz_mtd);
#if 0
	/* Unregister partitions */
	del_mtd_partitions(jz_mtd);

	/* Unregister the device */
	del_mtd_device(jz_mtd);
#endif
	/* Free the MTD device structure */
	if ((this->planenum == 2) && !all_use_planes)
		kfree (jz_mtd1);
	kfree(jz_mtd);
}

module_exit(jznand_cleanup);
#endif
