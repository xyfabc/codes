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

#define MISC_DEBUG	1
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

#define BCH_ECC_BIT	CONFIG_MTD_HW_BCH_BIT
#define PAR_SIZE	(14 * BCH_ECC_BIT / 8)

#define NAND_TYPE_COMMON	0
#define NAND_TYPE_TOGGLE	1

#define NAND_DATA_PORT1		0xBB000000	/* read-write area in static bank 1 */
#define NAND_DATA_PORT2		0xBA000000	/* read-write area in static bank 2 */
#define NAND_DATA_PORT3		0xB9000000	/* read-write area in static bank 3 */
#define NAND_DATA_PORT4		0xB8000000	/* read-write area in static bank 4 */
#define NAND_DATA_PORT5		0xB7000000
#define NAND_DATA_PORT6		0xB6000000

#define NAND_ADDR_OFFSET	0x00800000      /* address port offset for unshare mode */
#define NAND_CMD_OFFSET		0x00400000      /* command port offset for unshare mode */

#define PDMA_FW_TCSM		0xB3422000
#define PDMA_MSG_CHANNEL	3
#define PDMA_MSG_TCSMPA		0x13425800
#define PDMA_BANK3 		0xB3423800
#define PDMA_BANK4 		0xB3424000
#define PDMA_BANK5 		0xB3424800
#define PDMA_BANK6 		0xB3425000
#define PDMA_MSG_TCSMVA		0xB3425800

DECLARE_WAIT_QUEUE_HEAD(nand_dma_wait_queue);

static volatile int dma_ack = 0;
static struct pdma_msg msg;
extern unsigned char pdma_fw[7 * 1024];
extern int chipnr;

static u32 addr_offset;
static u32 cmd_offset;

extern int global_page; /* for two-plane operations */
extern int global_mafid; /* ID of manufacture */
extern int global_wrs;

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

/*-------------------------------------------------------------------------*/

static void jz_hwcontrol_common(struct mtd_info *mtd, int dat, u32 ctrl)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	u32 nandaddr = (u32)this->IO_ADDR_W;
	extern u8 nand_nce;  /* defined in nand_base.c, indicates which chip select is used for current nand chip */

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_NCE) {
			switch (nand_nce) {
			case 0:
				REG_NEMC_NFCSR = 0;
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

static void jz_hwcontrol_toggle(struct mtd_info *mtd, int dat, u32 ctrl)
{
	struct nand_chip *this = (struct nand_chip *)(mtd->priv);
	u32 nandaddr = (u32)this->IO_ADDR_W;
	extern u8 nand_nce;  /* defined in nand_base.c, indicates which chip select is used for current nand chip */

	if (ctrl & NAND_CTRL_CHANGE) {
		if (ctrl & NAND_NCE) {
			switch (nand_nce) {
			case 0:
				REG_NEMC_NFCSR = 0;
			case NAND_NCE1:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT1;
				REG_NEMC_NFCSR = NEMC_NFCSR_NFE1 | NEMC_NFCSR_TNFE1;
				break;
			case NAND_NCE2:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT2;
				REG_NEMC_NFCSR = NEMC_NFCSR_NFE2 | NEMC_NFCSR_TNFE2;
				break;
			case NAND_NCE3:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT3;
				REG_NEMC_NFCSR = NEMC_NFCSR_NFE3 | NEMC_NFCSR_TNFE3;
				break;
			case NAND_NCE4:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT4;
				REG_NEMC_NFCSR = NEMC_NFCSR_NFE4 | NEMC_NFCSR_TNFE4;
				break;
			case NAND_NCE5:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT5;
				REG_NEMC_NFCSR = NEMC_NFCSR_NFE5 | NEMC_NFCSR_TNFE5;
				break;
			case NAND_NCE6:
				this->IO_ADDR_W = this->IO_ADDR_R = (void __iomem *)NAND_DATA_PORT6;
				REG_NEMC_NFCSR = NEMC_NFCSR_NFE6 | NEMC_NFCSR_TNFE6;
				break;
			default:
				printk("error: no nand_nce 0x%x\n", nand_nce);
				break;
			}
		}

		if (ctrl & NAND_FCE_SET) {
			switch (nand_nce) {
			case NAND_NCE1:
				__tnand_dphtd_sync(nand_nce);
				REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE1 | NEMC_NFCSR_DAEC;
				break;
			case NAND_NCE2:
				__tnand_dphtd_sync(nand_nce);
				REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE2 | NEMC_NFCSR_DAEC;
				break;
			case NAND_NCE3:
				__tnand_dphtd_sync(nand_nce);
				REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE3 | NEMC_NFCSR_DAEC;
				break;
			case NAND_NCE4:
				__tnand_dphtd_sync(nand_nce);
				REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE4 | NEMC_NFCSR_DAEC;
				break;
			case NAND_NCE5:
				__tnand_dphtd_sync(nand_nce);
				REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE5 | NEMC_NFCSR_DAEC;
				break;
			case NAND_NCE6:
				__tnand_dphtd_sync(nand_nce);
				REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE6 | NEMC_NFCSR_DAEC;
				break;
			default:
				printk("error: no nand_nce 0x%x\n", nand_nce);
			}
			__tnand_dae_clr();
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
				REG_NEMC_NFCSR |= NEMC_NFCSR_DAEC;
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
	volatile int wait = 10;
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

#if 1
	//nor flash cs2 rd we => function 0
	REG_GPIO_PXINTC(0) = 0x00430000;
	REG_GPIO_PXMASKC(0) = 0x00430000;
	REG_GPIO_PXPAT1C(0) = 0x00430000;
	REG_GPIO_PXPAT0C(0) = 0x00430000;
#endif

#define SMCR_VAL   0x11444400
//#define SMCR_VAL   0x0fff7700  //slowest
	__gpio_as_nand_8bit(1);

#ifdef CONFIG_MTD_NAND_TOGGLE
	__gpio_as_nand_toggle();
	REG_NEMC_TGDR = 0x1D;
//	REG_NEMC_TGCR1 = 0x0000220A;

#if 0
	printk("\ntoggle NAND dpsdelay probe..");
	__tnand_dqsdelay_probe();
	printk("done --NEMC_TGDR: %08x\n", REG_NEMC_TGDR);
#endif
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

static void request_pdma_nand(struct pdma_msg *msg, unsigned int tcsm_pa)
{
	int trans_count = sizeof(struct pdma_msg);
	int err;

//	printk("---DMAC: %08x\n", REG_PDMAC_DMAC);
	if (!(REG_PDMAC_DMAC & PDMAC_DMAC_DMAE))
		__pdmac_enable();

	REG_PDMAC_DTC(PDMA_MSG_CHANNEL) = trans_count;
	REG_PDMAC_DRT(PDMA_MSG_CHANNEL) = PDMAC_DRT_RT_AUTO;

	REG_PDMAC_DSA(PDMA_MSG_CHANNEL) = CPHYSADDR((u32)msg);
	REG_PDMAC_DTA(PDMA_MSG_CHANNEL) = (u32)tcsm_pa;

	//----------------------------
#if 0
	printk("\n----\n");
	printk("PDMA_DRT: %08x\n", REG_PDMAC_DRT(PDMA_MSG_CHANNEL));
	printk("PDMA_DCT: %08x\n", REG_PDMAC_DTC(PDMA_MSG_CHANNEL));
	printk("PDMA_DSA: %08x\n", REG_PDMAC_DSA(PDMA_MSG_CHANNEL));
	printk("PDMA_DTA: %08x\n", REG_PDMAC_DTA(PDMA_MSG_CHANNEL));
	printk("PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(PDMA_MSG_CHANNEL));
	printk("PDMA_DCMD: %08x\n", REG_PDMAC_DCMD(PDMA_MSG_CHANNEL));
	printk("\n");
	printk("PDMA_DMAC: %08x\n", REG_PDMAC_DMAC);
	printk("----\n");
#endif

	dma_cache_wback_inv((u32)msg, trans_count);
	__pdmac_channel_launch(PDMA_MSG_CHANNEL);

	dma_ack = 0;

	do {
		printk("msg wait.\n");
#if 1
		err = wait_event_interruptible_timeout(nand_dma_wait_queue, dma_ack, 10 * HZ);
#else
		err = wait_event_interruptible(nand_dma_wait_queue, dma_ack);
#endif
//		dprintk("exit.\n");
	} while (err == -ERESTARTSYS);

	if (!err)
		printk("----IRQ err: %d\n", err);
	printk("---MSG PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(PDMA_MSG_CHANNEL));
}

static int nand_read_page_dma(struct mtd_info *mtd, struct nand_chip *chip, uint8_t *buf)
{
	struct pdma_msg *msg_read = &msg;
	unsigned int mb_read;
	int i;

	printk("\n----PDMA read start send msg\n");
	printk("NEMC_NFCSR: %08x\n", REG_NEMC_NFCSR);
	
	memset((unsigned char *)PDMA_BANK4, 0, 1024);

	msg_read->cmd = MSG_NAND_READ;
	msg_read->info[MSG_NAND_BANK] = chip->chip_num;
	msg_read->info[MSG_DDR_ADDR] = (u32)buf;
	msg_read->info[MSG_PAGEOFF] = global_page;

	dma_cache_wback_inv((u32)buf, mtd->rl_writesize);
	request_pdma_nand(msg_read, PDMA_MSG_TCSMPA);

	__mcu_mnmb_get(mb_read);
	printk("---read result: %08x\n", mb_read);
	printk("NEMC_NFCSR: %08x\n", REG_NEMC_NFCSR);
#if 1
	for (i = 0; i < 11; i++)
		dprintk("mcu nand info %08x\n", ((unsigned int *)PDMA_MSG_TCSMVA)[i]);

#endif
	printk("\n---->\n");
	printk("NEMC_channel\n");
	printk("PDMA_DRT: %08x\n", REG_PDMAC_DRT(1));
	printk("PDMA_DCT: %08x\n", REG_PDMAC_DTC(1));
	printk("PDMA_DSA: %08x\n", REG_PDMAC_DSA(1));
	printk("PDMA_DTA: %08x\n", REG_PDMAC_DTA(1));
	printk("PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(1));
	printk("PDMA_DCMD: %08x\n", REG_PDMAC_DCMD(1));
	printk("\n");
	printk("---BCH level: %d\n", BCH_ECC_BIT);
	printk("BCH_channel\n");
	printk("PDMA_DRT: %08x\n", REG_PDMAC_DRT(0));
	printk("PDMA_DCT: %08x\n", REG_PDMAC_DTC(0));
	printk("PDMA_DSA: %08x\n", REG_PDMAC_DSA(0));
	printk("PDMA_DTA: %08x\n", REG_PDMAC_DTA(0));
	printk("PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(0));
	printk("PDMA_DCMD: %08x\n", REG_PDMAC_DCMD(0));

	printk("BCH_CR: %08x\n", REG_BCH_CR);
	printk("BCH_CNT: %08x\n", REG_BCH_CNT);
	printk("BCH INTS: %08x\n", REG_BCH_INTS);
	printk("NEMC_NFCSR: %08x\n", REG_NEMC_NFCSR);
	printk("\n");

	
#if 0
	printk("PDMA_DCIRQP: %08x\n", REG_PDMAC_DCIRQP);
	printk("PDMA_DMAC: %08x\n", REG_PDMAC_DMAC);
#endif
	printk("PDMA_MCS: %08x\n", REG_PDMAC_MCS);
	printk("PDMA_MNMB: %08x\n", REG_PDMAC_MNMB);
	printk("PDMA_MINT: %08x\n", REG_PDMAC_MINT);
	printk("----\n");

	dprintk("---PDMA_MNMB read : %08x\n", mb_read);

#if 0
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[0]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[1]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[2]);
	printk("\nmcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[3]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[4]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[5]);
#endif
#if 0
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[3]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[4]);
	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[5]);

	printk("--- tscm dump\n");
//	mem_dump((unsigned char *)PDMA_BANK4, 512);
	mem_dump((unsigned char *)PDMA_BANK5, 512);
	mem_dump((unsigned char *)(PDMA_BANK5 + 1024), 128);
	mem_dump((unsigned char *)PDMA_BANK6, 512);
	mem_dump((unsigned char *)(PDMA_BANK6 + 1024), 128);
#endif
#if 1
	if (mb_read == MB_NAND_UNCOR_ECC) {
		printk("DDR_channel\n");
		printk("PDMA_DRT: %08x\n", REG_PDMAC_DRT(2));
		printk("PDMA_DCT: %08x\n", REG_PDMAC_DTC(2));
		printk("PDMA_DSA: %08x\n", REG_PDMAC_DSA(2));
		printk("PDMA_DTA: %08x\n", REG_PDMAC_DTA(2));
		printk("PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(2));
		printk("PDMA_DCMD: %08x\n", REG_PDMAC_DCMD(2));

		printk("--- tscm dump\n");
		printk("--PDMA_BANK5\n\n");
		mem_dump((unsigned char *)PDMA_BANK5, 1024);
		mem_dump((unsigned char *)(PDMA_BANK5 + 1024), 128);
		printk("--PDMA_BANK6\n");
		mem_dump((unsigned char *)PDMA_BANK6, 1024);
		mem_dump((unsigned char *)(PDMA_BANK6 + 1024), 128);
		printk("bchints %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[5]);
		printk("pipecnt %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[6]);
		printk("irqp %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[7]);
	}
#endif

	if (mb_read == MB_NAND_UNCOR_ECC)
		return -1;
	return 0;
}

static int nand_read_oob_hwecc_bch(struct mtd_info *mtd, struct nand_chip *chip, int page, int sndcmd)
{
	int oobsize = mtd->oobsize / chip->planenum;

	memset(chip->oob_poi,0x00,mtd->oobsize);

	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		sndcmd = 0;
	}

#ifdef CONFIG_MTD_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
#endif

	chip->read_buf(mtd, chip->oob_poi, oobsize);

	/* BCH decoding OOB */
//	bch_decode_oob(mtd, chip);

	return sndcmd;
}


static void nand_write_page_dma(struct mtd_info *mtd, struct nand_chip *chip, const uint8_t *buf)
{
	struct pdma_msg *msg_write = &msg;
	unsigned int mb_write;
	int i;

	printk("\n----PDMA write start send msg\n");
	printk("\nNEMC_NFCSR: %08x\n", REG_NEMC_NFCSR);
	msg_write->cmd = MSG_NAND_WRITE;
	msg_write->info[MSG_NAND_BANK] = chip->chip_num;
	msg_write->info[MSG_DDR_ADDR] = (u32)buf;
	msg_write->info[MSG_PAGEOFF] = global_page;

	dma_cache_wback_inv((u32)buf, mtd->rl_writesize);
	request_pdma_nand(msg_write, PDMA_MSG_TCSMPA);

	__mcu_mnmb_get(mb_write);

#if 0
	printk("---write result\n");
	for (i = 0; i < 11; i++)
		dprintk("mcu nand info %08x\n", ((unsigned int *)PDMA_MSG_TCSMVA)[i]);
#endif

	printk("\n---->\n");
	printk("NEMC_channel\n");
	printk("PDMA_DRT: %08x\n", REG_PDMAC_DRT(1));
	printk("PDMA_DCT: %08x\n", REG_PDMAC_DTC(1));
	printk("PDMA_DSA: %08x\n", REG_PDMAC_DSA(1));
	printk("PDMA_DTA: %08x\n", REG_PDMAC_DTA(1));
	printk("PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(1));
	printk("PDMA_DCMD: %08x\n", REG_PDMAC_DCMD(1));
	printk("\n");
	printk("BCH_channel\n");
	printk("PDMA_DRT: %08x\n", REG_PDMAC_DRT(0));
	printk("PDMA_DCT: %08x\n", REG_PDMAC_DTC(0));
	printk("PDMA_DSA: %08x\n", REG_PDMAC_DSA(0));
	printk("PDMA_DTA: %08x\n", REG_PDMAC_DTA(0));
	printk("PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(0));
	printk("PDMA_DCMD: %08x\n", REG_PDMAC_DCMD(0));
	printk("BCH CR:	%08x\n", REG_BCH_CR);
	printk("BCH INTS: %08x\n", REG_BCH_INTS);
	printk("NEMC_NFCSR: %08x\n", REG_NEMC_NFCSR);
	printk("\n");

	
	printk("PDMA_DCIRQP: %08x\n", REG_PDMAC_DCIRQP);
	printk("PDMA_DMAC: %08x\n", REG_PDMAC_DMAC);
	printk("PDMA_MCS: %08x\n", REG_PDMAC_MCS);
	printk("PDMA_MNMB: %08x\n", REG_PDMAC_MNMB);
	printk("PDMA_MINT: %08x\n", REG_PDMAC_MINT);
	printk("----\n");

	dprintk("---write - PDMA_MNMB : %08x\n", mb_write);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[0]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[1]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[2]);
//	printk("\nmcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[3]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[4]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[5]);


//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[0]);
//	printk("mcu info %08x\n", ((unsigned int *)(PDMA_BANK3+ 1024))[1]);
//	mem_dump((unsigned char *)PDMA_BANK4, 512);
	if (mb_write == MB_NAND_WRITE_FAIL)
		global_wrs = -1;
	if (mb_write == MB_NAND_WRITE_DONE)
		global_wrs = 0;
}

static void nand_msg_dma_init(void)
{
	__pdmac_channel_programmable_clear();
	__pdmac_channel_programmable_set(PDMA_MSG_CHANNEL);

	REG_PDMAC_DCCS(PDMA_MSG_CHANNEL) = PDMAC_DCCS_NDES;

	REG_PDMAC_DCMD(PDMA_MSG_CHANNEL) =  PDMAC_DCMD_SAI | PDMAC_DCMD_DAI |
					PDMAC_DCMD_SWDH_32 | PDMAC_DCMD_DWDH_32 |
					PDMAC_DCMD_TSZ_AUTO | PDMAC_DCMD_TIE;
}

static irqreturn_t nand_mb_irq(int irq, void *dev_id)
{
	int mb_num;
	volatile int wakeup = 0;

	mb_num = irq - IRQ_MCU_0;

	printk("\nMaibox IRQ...\n");
	printk("---PDMA_DMINT: %08x irq\n", REG_MCU_DMINT);

	if (mb_num == 0) {
		/* clear mb int */
		REG_MCU_DMINT &= ~MCU_DMINT_NIP;
		wakeup = 1;
	} else if (mb_num == 1) {
		REG_MCU_DMINT &= ~MCU_DMINT_SIP;
		wakeup = 1;
	}

	printk("---PDMA_DMINT clear: %08x irq\n", REG_MCU_DMINT);
	if (wakeup) {
		if (mb_num == 0) {
			dprintk("pdma MailBox irq, wake up----\n");
			dma_ack = 1;
			wake_up_interruptible(&nand_dma_wait_queue);
		}

		wakeup = 0;
	}

	return IRQ_HANDLED;
}

static int nand_mailbox_irq_init(void)
{
	int err;
	
	printk("----request msg irq\n");
	printk("---PDMA_DMINT: %08x irq\n", REG_MCU_DMINT);
	printk("---PDMA_MNMB: %08x\n", REG_MCU_DMNMB);
	err = request_irq(IRQ_MCU_0, nand_mb_irq, IRQF_DISABLED, "nand_msg", NULL);
	if (err) {
		printk("can't reqeust PDMA MB channel.\n");
		return -1;
	}
	return 0;
}

static void nand_pdma_firmware_load(void)
{
	int load_size = sizeof(pdma_fw);
	unsigned char *dest = (unsigned char *)PDMA_FW_TCSM;
	int i;

	printk("Load fw to PDMA...\n");
	printk("---load fw size 0x%08x\n", load_size);

	/* load fw to pdma tcsm */
	__mcu_reset_set();
	for (i = 0; i < load_size; i++)
		dest[i] = pdma_fw[i];
	__mcu_reset_clear();

	printk("finish load fw\n");
}

extern int nemc_bch_test(struct mtd_info *mtd);

/*
 * Main initialization routine
 */
int __init jznand_init(void)
{
	struct nand_chip *this;
	struct pdma_msg *msg_init = &msg;
	unsigned int mb_info;
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
	nand_pdma_firmware_load();

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
#ifdef CONFIG_MTD_NAND_TOGGLE
	printk("Toggle NAND.\n");
	this->cmd_ctrl = jz_hwcontrol_toggle;
#else
	this->cmd_ctrl = jz_hwcontrol_common;
#endif
	this->dev_ready = jz_device_ready;

#ifdef CONFIG_MTD_HW_BCH_ECC
	this->ecc.size = 1024;
	this->ecc.bytes = PAR_SIZE;
	this->ecc.read_page = nand_read_page_dma;
	this->ecc.write_page = nand_write_page_dma;
	this->ecc.mode = NAND_ECC_HW;

	this->ecc.read_oob = nand_read_oob_hwecc_bch;
//	this->ecc.write_oob = nand_write_oob_hwecc_bch;
#endif

#ifdef  CONFIG_MTD_SW_HM_ECC
	this->ecc.mode = NAND_ECC_SOFT;
#endif
	/* 20 us command delay time */
	this->chip_delay = 20;
	/* Scan to find existance of the device */
	ret = nand_scan_ident(jz_mtd, NAND_MAX_CHIPS);

	if (jz_mtd->rl_writesize <= 512)
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

	nand_msg_dma_init();
	nand_mailbox_irq_init();

	printk("---pdma NAND Init\n");
	/* PDMA Init NAND Info */
	msg_init->cmd = MSG_NAND_INIT;
	msg_init->info[MSG_NANDTYPE] = NAND_TYPE_COMMON;
	msg_init->info[MSG_PAGESIZE] = jz_mtd->rl_writesize;
	msg_init->info[MSG_OOBSIZE] = jz_mtd->oobsize;
	msg_init->info[MSG_ROWCYCLE] = this->rowcycle;

	msg_init->info[MSG_ECCLEVEL] = BCH_ECC_BIT;
	msg_init->info[MSG_ECCSIZE] = this->ecc.size;
	msg_init->info[MSG_ECCBYTES] = PAR_SIZE;
	msg_init->info[MSG_ECCSTEPS] = this->ecc.steps;
//	msg_init->info[MSG_ECCSTEPS] = 1;
	msg_init->info[MSG_ECCTOTAL] = this->ecc.total;
	msg_init->info[MSG_ECCPOS] = this->eccpos;
	printk("---pdma info init finish\n");

	request_pdma_nand(msg_init, PDMA_MSG_TCSMPA);
	
	printk("PDMA MNMB: 0x%08x\n", REG_PDMAC_MNMB);
	__mcu_mnmb_get(mb_info);

	printk("\n---->\n");
	printk("PDMA_DRT: %08x\n", REG_PDMAC_DRT(PDMA_MSG_CHANNEL));
	printk("PDMA_DCT: %08x\n", REG_PDMAC_DTC(PDMA_MSG_CHANNEL));
	printk("PDMA_DSA: %08x\n", REG_PDMAC_DSA(PDMA_MSG_CHANNEL));
	printk("PDMA_DTA: %08x\n", REG_PDMAC_DTA(PDMA_MSG_CHANNEL));
	printk("PDMA_DCCS: %08x\n", REG_PDMAC_DCCS(PDMA_MSG_CHANNEL));
	printk("PDMA_DCMD: %08x\n", REG_PDMAC_DCMD(PDMA_MSG_CHANNEL));
	printk("\n");
	
	printk("PDMA_DCIRQP: %08x\n", REG_PDMAC_DCIRQP);
	printk("PDMA_DMAC: %08x\n", REG_PDMAC_DMAC);
	printk("PDMA_MCS: %08x\n", REG_PDMAC_MCS);
	printk("PDMA_MNMB: %08x\n", REG_PDMAC_MNMB);
	printk("PDMA_MINT: %08x\n", REG_PDMAC_MINT);
	printk("----\n");

	for (i = 0; i < 11; i++)
		dprintk("mcu nand info %08x\n", ((unsigned int *)PDMA_MSG_TCSMVA)[i]);

	if (mb_info != MB_NAND_INIT_DONE) {
		printk(" NAND DMA : MCU Init Failed!\n");
		kfree (jz_mtd);
		return -ENXIO;
	}
	printk("NAND DMA : MCU Init Success\n");

#if 0
	((struct nand_chip *) (&jz_mtd1[1]))->ecc.read_page = nand_read_page_hwecc_bch;
	((struct nand_chip *) (&jz_mtd1[1]))->ecc.write_page = nand_write_page_hwecc_bch;
#endif

	/* Register the partitions */
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

	/* TEST NAND */
	nemc_bch_test(jz_mtd);

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
