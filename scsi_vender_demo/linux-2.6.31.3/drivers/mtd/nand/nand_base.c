/*
 *  drivers/mtd/nand.c
 *
 *  Overview:
 *   This is the generic MTD driver for NAND flash devices. It should be
 *   capable of working with almost all NAND chips currently available.
 *   Basic support for AG-AND chips is provided.
 *
 *	Additional technical information is available on
 *	http://www.linux-mtd.infradead.org/doc/nand.html
 *
 *  Copyright (C) 2000 Steven J. Hill (sjhill@realitydiluted.com)
 *		  2002-2006 Thomas Gleixner (tglx@linutronix.de)
 *
 *  Credits:
 *	David Woodhouse for adding multichip support

 *	Aleph One Ltd. and Toby Churchill Ltd. for supporting the
 *	rework for 2K page size chips
 *
 *  TODO:
 *	Enable cached programming for 2k page size chips
 *	Check, if mtd->ecctype should be set to MTD_ECC_HW
 *	if we have HW ecc support.
 *	The AG-AND chips have nice features for speed improvement,
 *	which are not supported yet. Read / program 4 pages in one go.
 *	BBT table is not serialized, has to be fixed
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/err.h>
#include <linux/sched.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/mtd/nand_ecc.h>
#include <linux/mtd/compatmac.h>
#include <linux/interrupt.h>
#include <linux/bitops.h>
#include <linux/leds.h>
#include <asm/io.h>

#ifdef CONFIG_MTD_PARTITIONS
#include <linux/mtd/partitions.h>
#endif

#include <asm/jzsoc.h>
#include "mtd/mtd_bch4bit_n8.h"

u8 nand_nce; /* indicates which chip select on JZSOC is used for current nand chip */
int global_page; /* page index of large page used for nand with multiple planes */
int global_mafid; /* ID of manufacture */
struct mtd_info *jz_mtd1 = NULL; /* for 1 plane operation */

u8 g_calc_lock = 0;/*lock the calc_to_phys function for it cann't do twice in one main fuction*/

#define LOCK_CALC 	g_calc_lock = 1
#define UNLOCK_CALC	g_calc_lock = 0

/* indicates whether multiple planes operation is used  by all partitions
   if multiple planes is supported by NAND */
char all_use_planes = 1;

/* The pointer to the address of block cache for partitions which work
   over mtdblock-jz */
extern struct mtd_partition partition_info[]; /* defined in jz47xx_nand.c */
unsigned char **jz_mtdblock_cache = NULL; /* used by mtdblock-jz.c */

/* Define default oob placement schemes for large and small page devices */
static struct nand_ecclayout nand_oob_8 = {
	.eccbytes = 3,
	.eccpos = {0, 1, 2},
	.oobfree = {
		{.offset = 3,
		 .length = 2},
		{.offset = 6,
		 .length = 2}}
};

static struct nand_ecclayout nand_oob_16 = {
        .eccbytes = 7,
        .eccpos = {9, 10, 11, 12, 13, 14, 15},
        .oobfree = {
                {.offset = 0,
                 .length = 4},
                {.offset = 6,
                 .length = 12}}
};

static struct nand_ecclayout nand_oob_64 = {
#if defined(CONFIG_MTD_HW_RS_ECC)
/* Reed-Solomon ECC */
	.eccbytes = 36,
	.eccpos = {
		28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51, 52, 53, 54, 55,
		56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 26}}
#elif defined(CONFIG_MTD_HW_BCH_ECC)
/* BCH ECC */
	.eccbytes = 28,
	.eccpos = {
		24, 25, 26, 27, 28, 29, 30, 31,
		32, 33, 34, 35, 36, 37, 38, 39,
		40, 41, 42, 43, 44, 45, 46, 47,
		48, 49, 50, 51},
	.oobfree = {
		{.offset = 2,
		 .length = 22}}
#else
/* HW&SW Hamming ECC */
	.eccbytes = 24,
	.eccpos = {
		   40, 41, 42, 43, 44, 45, 46, 47,
		   48, 49, 50, 51, 52, 53, 54, 55,
		   56, 57, 58, 59, 60, 61, 62, 63},
	.oobfree = {
		{.offset = 2,
		 .length = 38}}
#endif
};

static struct nand_ecclayout nand_oob_128 = {
#if defined(CONFIG_MTD_HW_RS_ECC)
/* Reed-Solomon ECC */
	.eccbytes = 72,
	.eccpos = {
		28, 29, 30, 31,	32, 33, 34, 35,
		36, 37, 38, 39, 40, 41, 42, 43,
		44, 45, 46, 47, 48, 49, 50, 51,
		52, 53, 54, 55, 56, 57, 58, 59,
		60, 61, 62, 63, 64, 65, 66, 67,
		68, 69, 70, 71, 72, 73, 74, 75,
		76, 77, 78, 79, 80, 81, 82, 83,
		84, 85, 86, 87, 88, 89, 90, 91,
		92, 93, 94, 95,	96, 97, 98, 99},
	.oobfree = {
		{.offset = 2,
		 .length = 26},
		{.offset = 100,
		 .length = 28}}
#elif defined(CONFIG_MTD_HW_BCH_ECC)
#if !defined(CONFIG_MTD_HW_BCH_8BIT)
/* 4-bit BCH ECC */
	.eccbytes = 56,
	.eccpos = {
		24, 25, 26, 27,	28, 29, 30, 31,
		32, 33, 34, 35,	36, 37, 38, 39,
		40, 41, 42, 43,	44, 45, 46, 47,
		48, 49, 50, 51,	52, 53, 54, 55,
		56, 57, 58, 59,	60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79},
	.oobfree = {
		{.offset = 2,
		 .length = 22},
		{.offset = 80,
		 .length = 48}}
#else
/* 8-bit BCH ECC */
	.eccbytes = 112,
	.eccpos = {
		 24, 25, 26, 27, 28, 29, 30, 31,
		 32, 33, 34, 35, 36, 37, 38, 39,
		 40, 41, 42, 43, 44, 45, 46, 47,
		 48, 49, 50, 51, 52, 53, 54, 55,
		 56, 57, 58, 59, 60, 61, 62, 63,
		 64, 65, 66, 67, 68, 69, 70, 71,
		 72, 73, 74, 75, 76, 77, 78, 79,
		 80, 81, 82, 83, 84, 85, 86, 87,
		 88, 89, 90, 91, 92, 93, 94, 95,
		 96, 97, 98, 99, 100, 101, 102, 103,
		 104, 105, 106, 107, 108, 109, 110, 111,
		 112, 113, 114, 115, 116, 117, 118, 119,
		 120, 121, 122, 123, 124, 125, 126, 127,
		 128, 129, 130, 131, 132, 133, 134, 135,},
	.oobfree = {
		 {.offset = 2,
		  .length = 22}}

#endif
#else
/* HW&SW Hamming ECC */
	.eccbytes = 48,
	.eccpos = {
		   80, 81, 82, 83, 84, 85, 86, 87,
		   88, 89, 90, 91, 92, 93, 94, 95,
		   96, 97, 98, 99, 100, 101, 102, 103,
		   104, 105, 106, 107, 108, 109, 110, 111,
		   112, 113, 114, 115, 116, 117, 118, 119,
		   120, 121, 122, 123, 124, 125, 126, 127},
	.oobfree = {
		{.offset = 2,
		 .length = 78}}
#endif
};

static struct nand_ecclayout nand_oob_218 = {
#if defined(CONFIG_MTD_HW_RS_ECC)
/* Reed-Solomon ECC */
	.eccbytes = 72,
	.eccpos = {
		28, 29, 30, 31,	32, 33, 34, 35,
		36, 37, 38, 39, 40, 41, 42, 43,
		44, 45, 46, 47, 48, 49, 50, 51,
		52, 53, 54, 55, 56, 57, 58, 59,
		60, 61, 62, 63, 64, 65, 66, 67,
		68, 69, 70, 71, 72, 73, 74, 75,
		76, 77, 78, 79, 80, 81, 82, 83,
		84, 85, 86, 87, 88, 89, 90, 91,
		92, 93, 94, 95,	96, 97, 98, 99},
	.oobfree = {
		{.offset = 2,
		 .length = 26},
		{.offset = 100,
		 .length = 28}}
#elif defined(CONFIG_MTD_HW_BCH_ECC)
#if !defined(CONFIG_MTD_HW_BCH_8BIT)
/* 4-bit BCH ECC */
	.eccbytes = 56,
	.eccpos = {
		24, 25, 26, 27,	28, 29, 30, 31,
		32, 33, 34, 35,	36, 37, 38, 39,
		40, 41, 42, 43,	44, 45, 46, 47,
		48, 49, 50, 51,	52, 53, 54, 55,
		56, 57, 58, 59,	60, 61, 62, 63,
		64, 65, 66, 67, 68, 69, 70, 71,
		72, 73, 74, 75, 76, 77, 78, 79},
	.oobfree = {
		{.offset = 2,
		 .length = 22},
		{.offset = 80,
		 .length = 48}}
#else
/* 8-bit BCH ECC */
	.eccbytes = 112,
	.eccpos = {
		 24, 25, 26, 27, 28, 29, 30, 31,
		 32, 33, 34, 35, 36, 37, 38, 39,
		 40, 41, 42, 43, 44, 45, 46, 47,
		 48, 49, 50, 51, 52, 53, 54, 55,
		 56, 57, 58, 59, 60, 61, 62, 63,
		 64, 65, 66, 67, 68, 69, 70, 71,
		 72, 73, 74, 75, 76, 77, 78, 79,
		 80, 81, 82, 83, 84, 85, 86, 87,
		 88, 89, 90, 91, 92, 93, 94, 95,
		 96, 97, 98, 99, 100, 101, 102, 103,
		 104, 105, 106, 107, 108, 109, 110, 111,
		 112, 113, 114, 115, 116, 117, 118, 119,
		 120, 121, 122, 123, 124, 125, 126, 127,
		 128, 129, 130, 131, 132, 133, 134, 135,},
	.oobfree = {
		 {.offset = 2,
		  .length = 22}}

#endif
#else
/* HW&SW Hamming ECC */
	.eccbytes = 48,
	.eccpos = {
		   80, 81, 82, 83, 84, 85, 86, 87,
		   88, 89, 90, 91, 92, 93, 94, 95,
		   96, 97, 98, 99, 100, 101, 102, 103,
		   104, 105, 106, 107, 108, 109, 110, 111,
		   112, 113, 114, 115, 116, 117, 118, 119,
		   120, 121, 122, 123, 124, 125, 126, 127},
	.oobfree = {
		{.offset = 2,
		 .length = 78}}
#endif
};

static struct nand_ecclayout nand_oob_436 = {
/* 24-bit BCH ECC */
	.eccbytes = 112,
	.eccpos = {
		 24, 25, 26, 27, 28, 29, 30, 31,
		 32, 33, 34, 35, 36, 37, 38, 39,
		 40, 41, 42, 43, 44, 45, 46, 47,
		 48, 49, 50, 51, 52, 53, 54, 55,
		 56, 57, 58, 59, 60, 61, 62, 63,
		 64, 65, 66, 67, 68, 69, 70, 71,
		 72, 73, 74, 75, 76, 77, 78, 79,
		 80, 81, 82, 83, 84, 85, 86, 87,
		 88, 89, 90, 91, 92, 93, 94, 95,
		 96, 97, 98, 99, 100, 101, 102, 103,
		 104, 105, 106, 107, 108, 109, 110, 111,
		 112, 113, 114, 115, 116, 117, 118, 119,
		 120, 121, 122, 123, 124, 125, 126, 127,},
	.oobfree = {
		 {.offset = 2,
		  .length = 22}}

};

static struct nand_ecclayout nand_oob_1024 = {
#if defined(CONFIG_MTD_HW_BCH_8BIT)
/* 8-bit BCH ECC */
	.eccbytes = 112,
	.eccpos = {
		 24, 25, 26, 27, 28, 29, 30, 31,
		 32, 33, 34, 35, 36, 37, 38, 39,
		 40, 41, 42, 43, 44, 45, 46, 47,
		 48, 49, 50, 51, 52, 53, 54, 55,
		 56, 57, 58, 59, 60, 61, 62, 63,
		 64, 65, 66, 67, 68, 69, 70, 71,
		 72, 73, 74, 75, 76, 77, 78, 79,
		 80, 81, 82, 83, 84, 85, 86, 87,
		 88, 89, 90, 91, 92, 93, 94, 95,
		 96, 97, 98, 99, 100, 101, 102, 103,
		 104, 105, 106, 107, 108, 109, 110, 111,
		 112, 113, 114, 115, 116, 117, 118, 119,
		 120, 121, 122, 123, 124, 125, 126, 127,
		 128, 129, 130, 131, 132, 133, 134, 135,},
	.oobfree = {
		 {.offset = 2,
		  .length = 22}}

#else
/* 40-bit BCH ECC */
	.eccbytes = 560,
	.eccpos = {
		 24, 25, 26, 27, 28, 29, 30, 31,
		 32, 33, 34, 35, 36, 37, 38, 39,
		 40, 41, 42, 43, 44, 45, 46, 47,
		 48, 49, 50, 51, 52, 53, 54, 55,
		 56, 57, 58, 59, 60, 61, 62, 63,
		 64, 65, 66, 67, 68, 69, 70, 71,
		 72, 73, 74, 75, 76, 77, 78, 79,
		 80, 81, 82, 83, 84, 85, 86, 87,
		 88, 89, 90, 91, 92, 93, 94, 95,
		 96, 97, 98, 99, 100, 101, 102, 103,
		 104, 105, 106, 107, 108, 109, 110, 111,
		 112, 113, 114, 115, 116, 117, 118, 119,
		 120, 121, 122, 123, 124, 125, 126, 127,},
	.oobfree = {
		 {.offset = 2,
		  .length = 22}}
#endif

};
int get_flash_num(void);

static int nand_get_device(struct nand_chip *chip, struct mtd_info *mtd,
			   int new_state);

static int nand_do_write_oob(struct mtd_info *mtd, loff_mtd_t to,
			     struct mtd_oob_ops *ops);

/*
 * For devices which display every fart in the system on a separate LED. Is
 * compiled away when LED support is disabled.
 */
DEFINE_LED_TRIGGER(nand_led_trigger);

/**
 * ffs_ll - find first bit set in a 64bit word.
 * @word: The word to search
 */
static inline int ffs_ll(u64 word)
{
	u32 low = word & 0xffffffff;
	u32 high = word >> 32;
	int i;

	for(i = 0; i < 32; i++) {
		if (low & 0x1)
			break;
		low >>= 1;
	}
	if (i == 32) {
		for(i = 0; i < 32; i++) {
			if (high & 0x1)
				break;
			high >>= 1;
		}
		i += 32;
	}
	if (i == 64)
		return 0;
	else
		return (i+1);
}

#if (defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_SOC_JZ4770) || defined(CONFIG_SOC_JZ4780)) && defined(CONFIG_MTD_HW_BCH_ECC)
static uint32_t calc_free_size(struct mtd_info *mtd)
{
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	uint32_t freesize;
	uint32_t pagesize = mtd->rl_writesize;
	uint32_t oobsize = mtd->oobsize;
	uint32_t eccsize = this->ecc.size;
	uint32_t eccbytes = this->ecc.bytes;
	uint32_t eccpos = this->ecc.layout->eccpos[0];

	if (((pagesize / eccsize + 1) * eccbytes + eccpos) > oobsize)
		freesize = 512;
	else
		freesize = 0;

	return freesize;
} 

static loff_mtd_t calc_to_phys(struct mtd_info *mtd, loff_mtd_t offs)
{
	if ((!g_calc_lock) && (mtd->freesize != 0))  //Cann't calculate twice in one main function.
		offs = (offs / mtd->writesize * mtd->rl_writesize) + (offs % mtd->writesize);
	return offs;
}
#else
static loff_mtd_t calc_to_phys(struct mtd_info *mtd, loff_mtd_t offs)
{
	return offs;
}
#endif


/**
 * nand_release_device - [GENERIC] release chip
 * @mtd:	MTD device structure
 *
 * Deselect, release chip lock and wake up anyone waiting on the device
 */
static void nand_release_device(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	/* De-select the NAND device */
	chip->select_chip(mtd, -1);

	/* Release the controller and the chip */
	spin_lock(&chip->controller->lock);
	chip->controller->active = NULL;
	chip->state = FL_READY;
	wake_up(&chip->controller->wq);
	spin_unlock(&chip->controller->lock);
}

/**
 * nand_read_byte - [DEFAULT] read one byte from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 8bit buswith
 */
static uint8_t nand_read_byte(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	int data;
	data = readb(chip->IO_ADDR_R);
	return data;
}

/**
 * nand_read_byte16 - [DEFAULT] read one byte endianess aware from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith with
 * endianess conversion
 */
static uint8_t nand_read_byte16(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return (uint8_t) cpu_to_le16(readw(chip->IO_ADDR_R));
}

/**
 * nand_read_word - [DEFAULT] read one word from the chip
 * @mtd:	MTD device structure
 *
 * Default read function for 16bit buswith without
 * endianess conversion
 */
static u16 nand_read_word(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	return readw(chip->IO_ADDR_R);
}

/**
 * nand_select_chip - [DEFAULT] control CE line
 * @mtd:	MTD device structure
 * @chipnr:	chipnumber to select, -1 for deselect
 *
 * Default select function for 1 chip devices.
 */
static void nand_select_chip(struct mtd_info *mtd, int chipnr)
{
	struct nand_chip *chip = mtd->priv;

	switch (chipnr) {
	case -1:
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, 0 | NAND_CTRL_CHANGE);
		break;
	case 0:
		nand_nce = NAND_NCE1;
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;
	case 1:
		nand_nce = NAND_NCE2;
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;
	case 2:
		nand_nce = NAND_NCE3;
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;
	case 3:
		nand_nce = NAND_NCE4;
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;
	case 4:
		nand_nce = NAND_NCE5;
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;
	case 5:
		nand_nce = NAND_NCE6;
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		break;

	default:
		BUG();
	}
}

/**
 * nand_write_buf - [DEFAULT] write buffer to chip
 * @mtd:	MTD device structure
 * @buf:	data buffer
 * @len:	number of bytes to write
 *
 * Default write function for 8bit buswith
 */
static void nand_write_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

#ifdef CFG_NAND_PN
	REG32(0xb3410100) = 0x03;
#endif

	for (i = 0; i < len; i++){
		writeb(buf[i], chip->IO_ADDR_W);
	}

#ifdef CFG_NAND_PN
	REG32(0xb3410100) = 0;
#endif
}

/**
 * nand_read_buf - [DEFAULT] read chip data into buffer
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 8bit buswith
 */
static void nand_read_buf(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	for (i = 0; i < len; i++){
		buf[i] = readb(chip->IO_ADDR_R);
	}
}

/**
 * nand_verify_buf - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 8bit buswith
 */
static int nand_verify_buf(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;

	for (i = 0; i < len; i++)
		if (buf[i] != readb(chip->IO_ADDR_R))
			return -EFAULT;
	return 0;
}

/**
 * nand_write_buf16 - [DEFAULT] write buffer to chip
 * @mtd:	MTD device structure
 * @buf:	data buffer
 * @len:	number of bytes to write
 *
 * Default write function for 16bit buswith
 */
static void nand_write_buf16(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		writew(p[i], chip->IO_ADDR_W);

}

/**
 * nand_read_buf16 - [DEFAULT] read chip data into buffer
 * @mtd:	MTD device structure
 * @buf:	buffer to store date
 * @len:	number of bytes to read
 *
 * Default read function for 16bit buswith
 */
static void nand_read_buf16(struct mtd_info *mtd, uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		p[i] = readw(chip->IO_ADDR_R);
}

/**
 * nand_verify_buf16 - [DEFAULT] Verify chip data against buffer
 * @mtd:	MTD device structure
 * @buf:	buffer containing the data to compare
 * @len:	number of bytes to compare
 *
 * Default verify function for 16bit buswith
 */
static int nand_verify_buf16(struct mtd_info *mtd, const uint8_t *buf, int len)
{
	int i;
	struct nand_chip *chip = mtd->priv;
	u16 *p = (u16 *) buf;
	len >>= 1;

	for (i = 0; i < len; i++)
		if (p[i] != readw(chip->IO_ADDR_R))
			return -EFAULT;

	return 0;
}

/**
 * nand_block_bad - [DEFAULT] Read bad block marker from the chip
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 *
 * Check, if the block is bad.
 */
static int nand_block_bad(struct mtd_info *mtd, loff_mtd_t ofs, int getchip)
{
	int page, page1 = 0, chipnr, res = 0;
	struct nand_chip *chip = mtd->priv;
	u16 bad;
	int calc_lock;
	
	nand_get_device(chip, mtd, FL_READING);

	calc_lock = g_calc_lock;
	ofs = calc_to_phys(mtd,ofs);
	LOCK_CALC;

	if (chip->planenum > 1) {
		page = ((int)(ofs >> chip->page_shift) * chip->planenum + CONFIG_MTD_BADBLOCK_FLAG_PAGE);
		page1 = page + mtd->rl_erasesize / mtd->rl_writesize;
		page &= chip->pagemask;
		page1 &= chip->pagemask;
	} else
		page = ((int)(ofs >> chip->page_shift) + CONFIG_MTD_BADBLOCK_FLAG_PAGE) & chip->pagemask;

	if (getchip) {
#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else
		chipnr = (int)(ofs >> chip->chip_shift);
		/* Select the NAND device */
		chip->select_chip(mtd, chipnr);
#endif
	}

	if (chip->options & NAND_BUSWIDTH_16) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos & 0xFE,
			      page);
		bad = cpu_to_le16(chip->read_word(mtd));
		if (chip->badblockpos & 0x1)
			bad >>= 8;
		if ((bad & 0xFF) != 0xff)
			res = 1;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos, page);

#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);		
#endif

		if (chip->read_byte(mtd) != 0xff)
			res = 1;
		if (chip->planenum > 1) {
			chip->cmdfunc(mtd, NAND_CMD_READOOB, chip->badblockpos, page1);
			if (chip->read_byte(mtd) != 0xff)
				res = 1;
		}

#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);		
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif

	}

	if (calc_lock != g_calc_lock)
		UNLOCK_CALC;
	
//	if (getchip)
		nand_release_device(mtd);

	return res;
}

/**
 * nand_default_block_markbad - [DEFAULT] mark a block bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 *
 * This is the default implementation, which can be overridden by
 * a hardware specific driver.
*/
static int nand_default_block_markbad(struct mtd_info *mtd, loff_mtd_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	uint8_t buf[2] = { 0, 0 };
	int block, ret;
	int calc_lock;
	
	nand_get_device(chip, mtd, FL_WRITING);
	calc_lock = g_calc_lock;
	ofs = calc_to_phys(mtd, ofs);
	LOCK_CALC;
	
	/* Get block number */
	block = (int)(ofs >> chip->bbt_erase_shift);
	if (chip->bbt)
		chip->bbt[block >> 2] |= 0x01 << ((block & 0x03) << 1);

	/* Do we have a flash based bad block table ? */
	if (chip->options & NAND_USE_FLASH_BBT)
		ret = nand_update_bbt(mtd, ofs);
	else {
		/* We write two bytes, so we dont have to mess with 16 bit
		 * access
		 */
//		nand_get_device(chip, mtd, FL_WRITING);
		ofs += mtd->oobsize;
		ofs += (CONFIG_MTD_BADBLOCK_FLAG_PAGE << chip->page_shift);
		chip->ops.len = chip->ops.ooblen = 2;
		chip->ops.datbuf = NULL;
		chip->ops.oobbuf = buf;
		chip->ops.ooboffs = chip->badblockpos & ~0x01;

		ret = nand_do_write_oob(mtd, ofs, &chip->ops);
//		nand_release_device(mtd);
	}
	if (!ret)
		mtd->ecc_stats.badblocks++;

	if(calc_lock != g_calc_lock)
		UNLOCK_CALC;
	nand_release_device(mtd);
	
	return ret;
}

/**
 * nand_check_wp - [GENERIC] check if the chip is write protected
 * @mtd:	MTD device structure
 * Check, if the device is write protected
 *
 * The function expects, that the device is already selected
 */
static int nand_check_wp(struct mtd_info *mtd)
{
	unsigned int sta;
	struct nand_chip *chip = mtd->priv;
	/* Check the WP bit */
	chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
	sta = chip->read_byte(mtd);
#endif
	sta = chip->read_byte(mtd);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
#endif
	return (sta & NAND_STATUS_WP) ? 0 : 1;
}

/**
 * nand_block_checkbad - [GENERIC] Check if a block is marked bad
 * @mtd:	MTD device structure
 * @ofs:	offset from device start
 * @getchip:	0, if the chip is already selected
 * @allowbbt:	1, if its allowed to access the bbt area
 *
 * Check, if the block is bad. Either by reading the bad block table or
 * calling of the scan function.
 */
static int nand_block_checkbad(struct mtd_info *mtd, loff_mtd_t ofs, int getchip,
			       int allowbbt)
{
	struct nand_chip *chip = mtd->priv;

	if (!chip->bbt){
		return chip->block_bad(mtd, ofs, getchip);
	}
	/* Return info from the table */
	return nand_isbad_bbt(mtd, ofs, allowbbt);
}

/*
 * Wait for the ready pin, after a command
 * The timeout is catched later.
 */
void nand_wait_ready(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	unsigned long timeo = jiffies + 2;

	led_trigger_event(nand_led_trigger, LED_FULL);
	/* wait until command is processed or timeout occures */
	do {
		if (chip->dev_ready(mtd))
			break;
		touch_softlockup_watchdog();
	} while (time_before(jiffies, timeo));
	led_trigger_event(nand_led_trigger, LED_OFF);
}
EXPORT_SYMBOL_GPL(nand_wait_ready);

void buffer_dump(uint8_t *buffer, int length, const char *comment, char *file, char *function, int line)
{
	int i;
	uint8_t *temp = buffer;
	printk("BufferDump: %s %d %s %s %d\n", comment, length, file, function, line);
	for(i = 0; i < length; i++) {
		if (i % 16 == 0) printk("\n");
		printk("%02x ", *temp++);
	}
	printk("\nOk.\n");
}

EXPORT_SYMBOL_GPL(buffer_dump);

/**
 * nand_command - [DEFAULT] Send command to NAND device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This function is used for small page
 * devices (256/512 Bytes per page)
 */
static void nand_command(struct mtd_info *mtd, unsigned int command,
			 int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;
	int ctrl = NAND_CTRL_CLE | NAND_CTRL_CHANGE;

	/*
	 * Write out the command to the device.
	 */
	if (command == NAND_CMD_SEQIN) {
		int readcmd;

		if (column >= mtd->rl_writesize) {
			/* OOB area */
			column -= mtd->rl_writesize;
			readcmd = NAND_CMD_READOOB;
		} else if (column < 256) {
			/* First 256 bytes --> READ0 */
			readcmd = NAND_CMD_READ0;
		} else {
			column -= 256;
			readcmd = NAND_CMD_READ1;
		}
		chip->cmd_ctrl(mtd, readcmd, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
	}
	chip->cmd_ctrl(mtd, command, ctrl);

	/*
	 * Address cycle, when necessary
	 */
	ctrl = NAND_CTRL_ALE | NAND_CTRL_CHANGE;
	/* Serially input address */
	if (column != -1) {
		/* Adjust columns for 16 bit buswidth */
		if (chip->options & NAND_BUSWIDTH_16)
			column >>= 1;
		chip->cmd_ctrl(mtd, column, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
	}
	if (page_addr != -1) {
		chip->cmd_ctrl(mtd, page_addr, ctrl);
		ctrl &= ~NAND_CTRL_CHANGE;
		chip->cmd_ctrl(mtd, page_addr >> 8, ctrl);
		/* One more address cycle for devices > 32MiB */
		if (chip->chipsize > (32 << 20))
			chip->cmd_ctrl(mtd, page_addr >> 16, ctrl);
	}
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);

	/*
	 * program and erase have their own busy handlers
	 * status and sequential in needs no delay
	 */
	switch (command) {

	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_STATUS:
		return;

	case NAND_CMD_RESET:
		if (chip->dev_ready)
			break;
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			       NAND_CTRL_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd,
			       NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
		while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
		return;

		/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return;
		}
	}
	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	nand_wait_ready(mtd);
}

/**
 * nand_command_lp - [DEFAULT] Send command to NAND large page device
 * @mtd:	MTD device structure
 * @command:	the command to be sent
 * @column:	the column address for this command, -1 if none
 * @page_addr:	the page address for this command, -1 if none
 *
 * Send command to NAND device. This is the version for the new large page
 * devices We dont have the separate regions as we have in the small page
 * devices.  We must emulate NAND_CMD_READOOB to keep the code compatible.
 */
static void nand_command_lp(struct mtd_info *mtd, unsigned int command,
			    int column, int page_addr)
{
	register struct nand_chip *chip = mtd->priv;

	/* Emulate NAND_CMD_READOOB */
	if (command == NAND_CMD_READOOB) {
		if (chip->planenum > 1)
			column += (mtd->rl_writesize / chip->planenum);
		else
			column += mtd->rl_writesize;
		command = NAND_CMD_READ0;
	}
#ifdef CFG_NAND_TOGGLE
	/* Command latch cycle */
	chip->cmd_ctrl(mtd, command & 0xff, NAND_CLE | NAND_CTRL_CHANGE);
#else
	chip->cmd_ctrl(mtd, command & 0xff, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
#endif

	if (column != -1 || page_addr != -1) {
#ifdef CFG_NAND_TOGGLE
		int ctrl = NAND_CTRL_CHANGE | NAND_ALE;
#else
		int ctrl = NAND_CTRL_CHANGE | NAND_ALE | NAND_NCE;
#endif

		/* Serially input address */
		if (column != -1) {
			/* Adjust columns for 16 bit buswidth */
			if (chip->options & NAND_BUSWIDTH_16)
				column >>= 1;
			chip->cmd_ctrl(mtd, column, ctrl);
#ifndef CFG_NAND_TOGGLE
			ctrl &= ~NAND_CTRL_CHANGE;
#endif
			chip->cmd_ctrl(mtd, column >> 8, ctrl);
		}
		if (page_addr != -1) {
			chip->cmd_ctrl(mtd, page_addr, ctrl);
			chip->cmd_ctrl(mtd, page_addr >> 8, ctrl);
			/* One more address cycle for devices > 128MiB */
			if (chip->chipsize > (128 << 20))
				chip->cmd_ctrl(mtd, page_addr >> 16, ctrl);
		}

#ifndef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#else
		//set io as data port
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE);
#endif
	}

	/*
	 * program and erase have their own busy handlers
	 * status, sequential in, and deplete1 need no delay
	 */
	switch (command) {

	case NAND_CMD_CACHEDPROG:
	case NAND_CMD_PAGEPROG:
	case NAND_CMD_ERASE1:
	case NAND_CMD_ERASE2:
	case NAND_CMD_SEQIN:
	case NAND_CMD_RNDIN:
	case NAND_CMD_RANDOM_READ:
	case NAND_CMD_RANDOM_READ_CONFIRM:
	case NAND_CMD_STATUS:
	case NAND_CMD_DEPLETE1:
	case 0x81: /* for two-plane page program */
	case 0x11: /* for two-plane page program */
		return;

		/*
		 * read error status commands require only a short delay
		 */
	case NAND_CMD_STATUS_ERROR:
	case NAND_CMD_STATUS_ERROR0:
	case NAND_CMD_STATUS_ERROR1:
	case NAND_CMD_STATUS_ERROR2:
	case NAND_CMD_STATUS_ERROR3:
		udelay(chip->chip_delay);
		return;

	case NAND_CMD_RESET:
		if (chip->dev_ready(mtd)){
			break;
		}
		udelay(chip->chip_delay);
		chip->cmd_ctrl(mtd, NAND_CMD_STATUS,
			       NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
		chip->cmd_ctrl(mtd, NAND_CMD_NONE,
			       NAND_NCE | NAND_CTRL_CHANGE);
#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
#endif

//		while (!(chip->read_byte(mtd) & NAND_STATUS_READY)) ;
		while(1){
#ifdef CFG_NAND_TOGGLE
			chip->read_byte(mtd);
#endif
			if(chip->read_byte(mtd) & NAND_STATUS_READY)
				break; 
		}

#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
#endif
		return;

	case NAND_CMD_READ0:
#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_CLE | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_READSTART, NAND_NCE | NAND_CLE | NAND_CTRL_CHANGE);
#endif
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE);
		break;

		/* This applies to read commands */
	default:
		/*
		 * If we don't have access to the busy pin, we apply the given
		 * command delay
		 */
		if (!chip->dev_ready) {
			udelay(chip->chip_delay);
			return;
		}
	}

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	nand_wait_ready(mtd);
}

/**
 * nand_get_device - [GENERIC] Get chip for selected access
 * @chip:	the nand chip descriptor
 * @mtd:	MTD device structure
 * @new_state:	the state which is requested
 *
 * Get the device and lock it for exclusive access
 */
static int
nand_get_device(struct nand_chip *chip, struct mtd_info *mtd, int new_state)
{
	spinlock_t *lock = &chip->controller->lock;
	wait_queue_head_t *wq = &chip->controller->wq;
	DECLARE_WAITQUEUE(wait, current);
 retry:
	spin_lock(lock);

	/* Hardware controller shared among independend devices */
	if (!chip->controller->active)
		chip->controller->active = chip;

	if (new_state == FL_PM_SUSPENDED)
	printk("%s(): chip->controller->active: 0x%p, chip: 0x%p, chip->state: 0x%d.\n", __func__, chip->controller->active, chip, chip->state);
	if (chip->controller->active == chip && chip->state == FL_READY) {
		chip->state = new_state;
		spin_unlock(lock);
		return 0;
	}
	if (new_state == FL_PM_SUSPENDED) {
		spin_unlock(lock);
		return (chip->state == FL_PM_SUSPENDED) ? 0 : -EAGAIN;
	}
	set_current_state(TASK_UNINTERRUPTIBLE);
	add_wait_queue(wq, &wait);
	spin_unlock(lock);
	schedule();
	remove_wait_queue(wq, &wait);
	goto retry;
}

/**
 * nand_wait - [DEFAULT]  wait until the command is done
 * @mtd:	MTD device structure
 * @chip:	NAND chip structure
 *
 * Wait for command done. This applies to erase and program only
 * Erase can take up to 400ms and program up to 20ms according to
 * general NAND and SmartMedia specs
 */
static int nand_wait(struct mtd_info *mtd, struct nand_chip *chip)
{

	unsigned long timeo = jiffies;
	int status, state = chip->state;

	if (state == FL_ERASING)
		timeo += (HZ * 400) / 1000;
	else
		timeo += (HZ * 20) / 1000;

	led_trigger_event(nand_led_trigger, LED_FULL);

	/* Apply this short delay always to ensure that we do wait tWB in
	 * any case on any machine. */
	ndelay(100);

	while (time_before(jiffies, timeo)) {
		if (chip->dev_ready) {
			if (chip->dev_ready(mtd))
				break;
		} else {
			if (chip->read_byte(mtd) & NAND_STATUS_READY)
				break;
		}
		cond_resched();
	}
	led_trigger_event(nand_led_trigger, LED_OFF);

	if ((state == FL_ERASING) && (chip->options & NAND_IS_AND))
		chip->cmdfunc(mtd, NAND_CMD_STATUS_MULTI, -1, -1);
	else
		chip->cmdfunc(mtd, NAND_CMD_STATUS, -1, -1);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
	status = (int)chip->read_byte(mtd);
#endif

	status = (int)chip->read_byte(mtd);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
#endif

	return status;
}

/**
 * nand_read_page_raw - [Intern] read raw page data without ecc
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * Not for syndrome calculating ecc controllers, which use a special oob layout
 */
static int nand_read_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
			      uint8_t *buf)
{
	chip->read_buf(mtd, buf, mtd->rl_writesize);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	return 0;
}

/**
 * nand_read_page_raw_syndrome - [Intern] read raw page data without ecc
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * We need a special oob layout and handling even when OOB isn't used.
 */
static int nand_read_page_raw_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
			      uint8_t *buf)
{
	int eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	uint8_t *oob = chip->oob_poi;
	int steps, size;

	for (steps = chip->ecc.steps; steps > 0; steps--) {
		chip->read_buf(mtd, buf, eccsize);
		buf += eccsize;

		if (chip->ecc.prepad) {
			chip->read_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->read_buf(mtd, oob, eccbytes);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->read_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	size = mtd->oobsize - (oob - chip->oob_poi);
	if (size)
		chip->read_buf(mtd, oob, size);

	return 0;
}

/**
 * nand_read_page_swecc - [REPLACABLE] software ecc based page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 */
static int nand_read_page_swecc(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	chip->ecc.read_page_raw(mtd, chip, buf);

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);

	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	eccsteps = chip->ecc.steps;
	p = buf;

	for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], &ecc_calc[i]);
		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}
	return 0;
}

/**
 * nand_read_subpage - [REPLACABLE] software ecc based sub-page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @data_offs:	offset of requested data within the page
 * @readlen:	data length
 * @bufpoi:	buffer to store read data
 */
static int nand_read_subpage(struct mtd_info *mtd, struct nand_chip *chip, uint32_t data_offs, uint32_t readlen, uint8_t *bufpoi)
{
	int start_step, end_step, num_steps;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint8_t *p;
	int data_col_addr, i, gaps = 0;
	int datafrag_len, eccfrag_len, aligned_len, aligned_pos;
	int busw = (chip->options & NAND_BUSWIDTH_16) ? 2 : 1;

	/* Column address wihin the page aligned to ECC size (256bytes). */
	start_step = data_offs / chip->ecc.size;
	end_step = (data_offs + readlen - 1) / chip->ecc.size;
	num_steps = end_step - start_step + 1;

	/* Data size aligned to ECC ecc.size*/
	datafrag_len = num_steps * chip->ecc.size;
	eccfrag_len = num_steps * chip->ecc.bytes;

	data_col_addr = start_step * chip->ecc.size;
	/* If we read not a page aligned data */
	if (data_col_addr != 0)
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, data_col_addr, -1);

	p = bufpoi + data_col_addr;
	chip->read_buf(mtd, p, datafrag_len);

	/* Calculate  ECC */
	for (i = 0; i < eccfrag_len ; i += chip->ecc.bytes, p += chip->ecc.size)
		chip->ecc.calculate(mtd, p, &chip->buffers->ecccalc[i]);

	/* The performance is faster if to position offsets
	   according to ecc.pos. Let make sure here that
	   there are no gaps in ecc positions */
	for (i = 0; i < eccfrag_len - 1; i++) {
		if (eccpos[i + start_step * chip->ecc.bytes] + 1 !=
			eccpos[i + start_step * chip->ecc.bytes + 1]) {
			gaps = 1;
			break;
		}
	}
	if (gaps) {
		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->rl_writesize, -1);
		chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	} else {
		/* send the command to read the particular ecc bytes */
		/* take care about buswidth alignment in read_buf */
		aligned_pos = eccpos[start_step * chip->ecc.bytes] & ~(busw - 1);
		aligned_len = eccfrag_len;
		if (eccpos[start_step * chip->ecc.bytes] & (busw - 1))
			aligned_len++;
		if (eccpos[(start_step + num_steps) * chip->ecc.bytes] & (busw - 1))
			aligned_len++;

		chip->cmdfunc(mtd, NAND_CMD_RNDOUT, mtd->rl_writesize + aligned_pos, -1);
		chip->read_buf(mtd, &chip->oob_poi[aligned_pos], aligned_len);
	}

	for (i = 0; i < eccfrag_len; i++)
		chip->buffers->ecccode[i] = chip->oob_poi[eccpos[i + start_step * chip->ecc.bytes]];

	p = bufpoi + data_col_addr;
	for (i = 0; i < eccfrag_len ; i += chip->ecc.bytes, p += chip->ecc.size) {
		int stat;

		stat = chip->ecc.correct(mtd, p, &chip->buffers->ecccode[i], &chip->buffers->ecccalc[i]);
		if (stat == -1)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}
	return 0;
}

/**
 * nand_read_page_hwecc - [REPLACABLE] hardware ecc based page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * Not for syndrome calculating ecc controllers which need a special oob layout
 */
static int nand_read_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);
	}
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	
	for (i = 0; i < chip->ecc.total; i++)
		ecc_code[i] = chip->oob_poi[eccpos[i]];

	eccsteps = chip->ecc.steps;
	p = buf;

	for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		stat = chip->ecc.correct(mtd, p, &ecc_code[i], &ecc_calc[i]);
		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;
	}
	return 0;
}

#ifdef CONFIG_MTD_HW_RS_ECC /* HW&SW Hamming ECC */

/**
 * nand_read_page_hwecc_rs - [REPLACABLE] hardware rs ecc based page read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * Not for syndrome calculating ecc controllers which need a special oob layout
 */
static int nand_read_page_hwecc_rs(struct mtd_info *mtd, struct nand_chip *chip,
				   uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	uint8_t *ecc_code = chip->buffers->ecccode;
	uint32_t *eccpos = chip->ecc.layout->eccpos;
	uint32_t page;
	uint8_t flag = 0;

	page = (buf[3]<<24) + (buf[2]<<16) + (buf[1]<<8) + buf[0];

	chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
	for (i = 0; i < chip->ecc.total; i++) {
		ecc_code[i] = chip->oob_poi[eccpos[i]];
		if (ecc_code[i] != 0xff) flag = 1;
	}

	eccsteps = chip->ecc.steps;
	p = buf;

	chip->cmdfunc(mtd, NAND_CMD_RNDOUT, 0x00, -1);
	for (i = 0 ; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;
		if (flag) {
			chip->ecc.hwctl(mtd, NAND_ECC_READ);
			chip->read_buf(mtd, p, eccsize);
			stat = chip->ecc.correct(mtd, p, &ecc_code[i], &ecc_calc[i]);
			if (stat < 0)
				mtd->ecc_stats.failed++;
			else
				mtd->ecc_stats.corrected += stat;
		}
		else {
			chip->ecc.hwctl(mtd, NAND_ECC_READ);
			chip->read_buf(mtd, p, eccsize);
		}
	}
	return 0;
}

#endif /* CONFIG_MTD_HW_RS_ECC */

/**
 * nand_read_page_syndrome - [REPLACABLE] hardware ecc syndrom based page read
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	buffer to store read data
 *
 * The hw generator calculates the error syndrome automatically. Therefor
 * we need a special oob layout and handling.
 */
static int nand_read_page_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
				   uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		int stat;

		chip->ecc.hwctl(mtd, NAND_ECC_READ);
		chip->read_buf(mtd, p, eccsize);

		if (chip->ecc.prepad) {
			chip->read_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->ecc.hwctl(mtd, NAND_ECC_READSYN);
		chip->read_buf(mtd, oob, eccbytes);
		stat = chip->ecc.correct(mtd, p, oob, NULL);

		if (stat < 0)
			mtd->ecc_stats.failed++;
		else
			mtd->ecc_stats.corrected += stat;

		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->read_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->read_buf(mtd, oob, i);

	return 0;
}

/**
 * nand_transfer_oob - [Internal] Transfer oob to client buffer
 * @chip:	nand chip structure
 * @oob:	oob destination address
 * @ops:	oob ops structure
 * @len:	size of oob to transfer
 */
static uint8_t *nand_transfer_oob(struct nand_chip *chip, uint8_t *oob,
				  struct mtd_oob_ops *ops, size_mtd_t len)
{
	switch(ops->mode) {

	case MTD_OOB_PLACE:
	case MTD_OOB_RAW:
		memcpy(oob, chip->oob_poi + ops->ooboffs, len);
		return oob + len;

	case MTD_OOB_AUTO: {
		struct nand_oobfree *free = chip->ecc.layout->oobfree;
		uint32_t boffs = 0, roffs = ops->ooboffs;
		size_mtd_t bytes = 0;

		for(; free->length && len; free++, len -= bytes) {
			/* Read request not from offset 0 ? */
			if (unlikely(roffs)) {
				if (roffs >= free->length) {
					roffs -= free->length;
					continue;
				}
				boffs = free->offset + roffs;
				bytes = min_t(size_mtd_t, len,
					      (free->length - roffs));
				roffs = 0;
			} else {
				bytes = min_t(size_mtd_t, len, free->length);
				boffs = free->offset;
			}
			memcpy(oob, chip->oob_poi + boffs, bytes);
			oob += bytes;
		}
		return oob;
	}
	default:
		BUG();
	}
	return NULL;
}

/**
 * nand_do_read_ops - [Internal] Read data with ECC
 *
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @ops:	oob ops structure
 *
 * Internal function. Called with chip held.
 */
static int nand_do_read_ops(struct mtd_info *mtd, loff_mtd_t from,
			    struct mtd_oob_ops *ops)
{
	int chipnr, page, realpage, col, bytes, aligned;
	struct nand_chip *chip = mtd->priv;
	struct mtd_ecc_stats stats;
	int blkcheck = chip->ppb - 1;//(1 << (chip->phys_erase_shift - chip->page_shift)) - 1
	int sndcmd = 1;
	int ret = 0;
	uint32_t readlen = ops->len;
	uint32_t oobreadlen = ops->ooblen;
	uint8_t *bufpoi, *oob, *buf;
		
	stats = mtd->ecc_stats;

#ifndef CFG_NAND_TOGGLE
	chipnr = (int)(from >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);
#else
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif


	//Now, the from is physcal addr by byte.
	realpage = (int)(from >> chip->page_shift);
	page = realpage & chip->pagemask;

	col = (int)(from & (mtd->rl_writesize - 1));

	buf = ops->datbuf;
	oob = ops->oobbuf;

	while(1) {
		//bytes = min(mtd->rl_writesize - col, readlen);
		//aligned = (bytes == mtd->rl_writesize);
		bytes = min(mtd->writesize - col, readlen); //here calc valid data size in one page
		aligned = (bytes == mtd->writesize);

		/* Is the current page in the buffer ? */
		if (realpage != chip->pagebuf || oob) {
			bufpoi = aligned ? buf : chip->buffers->databuf;
			global_page = page;

#if defined(CONFIG_MTD_HW_RS_ECC)
			bufpoi[0] = (uint8_t)page;
			bufpoi[1] = (uint8_t)(page >> 8);
			bufpoi[2] = (uint8_t)(page >> 16);
			bufpoi[3] = (uint8_t)(page >> 24);
#elif defined(CONFIG_SOC_JZ4730) || defined(CONFIG_SOC_JZ4780)
			if (likely(sndcmd)) {
				chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
				sndcmd = 0;
			}

#else /* for jz4750 and later chip */
			if (mtd->flags & MTD_NAND_CPU_MODE) {
				if (likely(sndcmd)) {
					chip->cmdfunc(mtd, NAND_CMD_READ0, 0x00, page);
					sndcmd = 0;
				}
			}
#endif

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
#endif

#ifdef CFG_NAND_PN
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTL_SETPN | NAND_CTRL_CHANGE);
#endif
			/* Now read the page into the buffer */
			if (unlikely(ops->mode == MTD_OOB_RAW))
				ret = chip->ecc.read_page_raw(mtd, chip, bufpoi);
			else if (!aligned && NAND_SUBPAGE_READ(chip) && !oob)
				ret = chip->ecc.read_subpage(mtd, chip, col, bytes, bufpoi);
			else
				ret = chip->ecc.read_page(mtd, chip, bufpoi);
			if (ret < 0)
				break;

#ifdef CFG_NAND_PN
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTL_CLRPN | NAND_CTRL_CHANGE);
#endif

			/* Transfer not aligned data */
			if (!aligned) {
				if (!NAND_SUBPAGE_READ(chip) && !oob)
					chip->pagebuf = realpage;
				memcpy(buf, chip->buffers->databuf + col, bytes);
			}

			buf += bytes;

			if (unlikely(oob)) {
				/* Raw mode does data:oob:data:oob */
				if (ops->mode != MTD_OOB_RAW) {
					int toread = min(oobreadlen,
						chip->ecc.layout->oobavail);
					if (toread) {
						oob = nand_transfer_oob(chip,
							oob, ops, toread);
						oobreadlen -= toread;
					}
				} else
					buf = nand_transfer_oob(chip,
						oob, ops, mtd->oobsize);
			}

			if (!(chip->options & NAND_NO_READRDY)) {
				/*
				 * Apply delay or wait for ready/busy pin. Do
				 * this before the AUTOINCR check, so no
				 * problems arise if a chip which does auto
				 * increment is marked as NOAUTOINCR by the
				 * board driver.
				 */
				if (!chip->dev_ready)
					udelay(chip->chip_delay);
				else
					nand_wait_ready(mtd);
			}
		} else {
			memcpy(buf, chip->buffers->databuf + col, bytes);
			buf += bytes;
		}

#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN | NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif

		readlen -= bytes;

		if (!readlen)
			break;

		/* For subsequent reads align to page boundary. */
		col = 0;
		/* Increment page address */
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}

		/* Check, if the chip supports auto page increment
		 * or if we have hit a block boundary.
		 */
		if (!NAND_CANAUTOINCR(chip) || !(page & blkcheck))
			sndcmd = 1;
	}

	ops->retlen = ops->len - (size_mtd_t) readlen;
	if (oob)
		ops->oobretlen = ops->ooblen - oobreadlen;

	if (ret)
		return ret;

	if (mtd->ecc_stats.failed - stats.failed)
		return -EBADMSG;

	return  mtd->ecc_stats.corrected - stats.corrected ? -EUCLEAN : 0;
}

/**
 * nand_read - [MTD Interface] MTD compability function for nand_do_read_ecc
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @len:	number of bytes to read
 * @retlen:	pointer to variable to store the number of read bytes
 * @buf:	the databuffer to put data
 *
 * Get hold of the chip and call nand_do_read
 */
static int nand_read(struct mtd_info *mtd, loff_mtd_t from, size_mtd_t len,
		     size_mtd_t *retlen, uint8_t *buf)
{
	struct nand_chip *chip = mtd->priv;
	int ret;
	int calc_lock;
	
	nand_get_device(chip, mtd, FL_READING);
	calc_lock = g_calc_lock;
	from = calc_to_phys(mtd, from);
	LOCK_CALC;

	/* Do not allow reads past end of device */
	if ((from + len) > mtd->rl_size)
	{
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return -EINVAL;
	}
	if (!len)
	{
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return 0;
	}
//	nand_get_device(chip, mtd, FL_READING);

	chip->ops.len = len;
	chip->ops.datbuf = buf;
	chip->ops.oobbuf = NULL;

	ret = nand_do_read_ops(mtd, from, &chip->ops);

	*retlen = chip->ops.retlen;

//	nand_release_device(mtd);
	
	if (calc_lock != g_calc_lock)
		UNLOCK_CALC;
	nand_release_device(mtd);

	return ret;
}

int nand_sw_bch_ops(struct mtd_info *mtd, u8 *oobdata, int ops)
{
#if defined(CONFIG_SOC_JZ4750) || defined(CONFIG_SOC_JZ4750D) || defined(CONFIG_SOC_JZ4760)
	int i, cnt, off, length;
	unsigned char *oobp = oobdata;
	struct nand_chip *chip = mtd->priv;

	off = chip->ecc.layout->oobfree[0].offset;
	length = 16;
	
	if (ops)	//encode
	{
		do_bch_encode(oobp + off, oobp + length + off, length);
	}
	else		//decode
	{
		cnt = 0;
		for (i = 0; i < 32; i++)
			if (*(oobp + i) == 0xff)
				cnt++;
	
		if (cnt < 24)
			do_bch_decode_and_correct (oobp + off, oobp + length + off, length);
	}
#endif
	return 0;
}

/**
 * nand_read_oob_std - [REPLACABLE] the most common OOB data read function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to read
 * @sndcmd:	flag whether to issue read command or not
 */
static int nand_read_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
			     int page, int sndcmd)
{
	if (sndcmd) {
		chip->cmdfunc(mtd, NAND_CMD_READOOB, 0, page);
		sndcmd = 0;
	}
	chip->read_buf(mtd, chip->oob_poi, mtd->oobsize);
//	nand_sw_bch_ops(mtd, chip->oob_poi, 0);
	
	return sndcmd;
}

/**
 * nand_read_oob_syndrome - [REPLACABLE] OOB data read function for HW ECC
 *			    with syndromes
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to read
 * @sndcmd:	flag whether to issue read command or not
 */
static int nand_read_oob_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
				  int page, int sndcmd)
{
	uint8_t *buf = chip->oob_poi;
	int length = mtd->oobsize;
	int chunk = chip->ecc.bytes + chip->ecc.prepad + chip->ecc.postpad;
	int eccsize = chip->ecc.size;
	uint8_t *bufpoi = buf;
	int i, toread, sndrnd = 0, pos;

	chip->cmdfunc(mtd, NAND_CMD_READ0, chip->ecc.size, page);
	for (i = 0; i < chip->ecc.steps; i++) {
		if (sndrnd) {
			pos = eccsize + i * (eccsize + chunk);
			if (mtd->rl_writesize > 512)
				chip->cmdfunc(mtd, NAND_CMD_RNDOUT, pos, -1);
			else
				chip->cmdfunc(mtd, NAND_CMD_READ0, pos, page);
		} else
			sndrnd = 1;
		toread = min_t(int, length, chunk);
		chip->read_buf(mtd, bufpoi, toread);
		bufpoi += toread;
		length -= toread;
	}
	if (length > 0)
		chip->read_buf(mtd, bufpoi, length);

	return 1;
}

/**
 * nand_write_oob_std - [REPLACABLE] the most common OOB data write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to write
 */
static int nand_write_oob_std(struct mtd_info *mtd, struct nand_chip *chip,
			      int page)
{
	int status = 0;
	const uint8_t *buf = chip->oob_poi;
	int length = mtd->oobsize;

	nand_sw_bch_ops(mtd, chip->oob_poi, 1);

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, mtd->rl_writesize, page);
	chip->write_buf(mtd, buf, length);
	/* Send command to program the OOB data */
	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/**
 * nand_write_oob_syndrome - [REPLACABLE] OOB data write function for HW ECC
 *			     with syndrome - only for large page flash !
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @page:	page number to write
 */
static int nand_write_oob_syndrome(struct mtd_info *mtd,
				   struct nand_chip *chip, int page)
{
	int chunk = chip->ecc.bytes + chip->ecc.prepad + chip->ecc.postpad;
	int eccsize = chip->ecc.size, length = mtd->oobsize;
	int i, len, pos, status = 0, sndcmd = 0, steps = chip->ecc.steps;
	const uint8_t *bufpoi = chip->oob_poi;

	/*
	 * data-ecc-data-ecc ... ecc-oob
	 * or
	 * data-pad-ecc-pad-data-pad .... ecc-pad-oob
	 */
	if (!chip->ecc.prepad && !chip->ecc.postpad) {
		pos = steps * (eccsize + chunk);
		steps = 0;
	} else
		pos = eccsize;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, pos, page);
	for (i = 0; i < steps; i++) {
		if (sndcmd) {
			if (mtd->rl_writesize <= 512) {
				uint32_t fill = 0xFFFFFFFF;

				len = eccsize;
				while (len > 0) {
					int num = min_t(int, len, 4);
					chip->write_buf(mtd, (uint8_t *)&fill,
							num);
					len -= num;
				}
			} else {
				pos = eccsize + i * (eccsize + chunk);
				chip->cmdfunc(mtd, NAND_CMD_RNDIN, pos, -1);
			}
		} else
			sndcmd = 1;
		len = min_t(int, length, chunk);
		chip->write_buf(mtd, bufpoi, len);
		bufpoi += len;
		length -= len;
	}
	if (length > 0)
		chip->write_buf(mtd, bufpoi, length);

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);
	status = chip->waitfunc(mtd, chip);

	return status & NAND_STATUS_FAIL ? -EIO : 0;
}

/**
 * nand_do_read_oob - [Intern] NAND read out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @ops:	oob operations description structure
 *
 * NAND read out-of-band data from the spare area
 */
static int nand_do_read_oob(struct mtd_info *mtd, loff_mtd_t from,
			    struct mtd_oob_ops *ops)
{
	int page, realpage, chipnr, sndcmd = 1;
	struct nand_chip *chip = mtd->priv;
	int blkcheck = chip->ppb - 1;//(1 << (chip->phys_erase_shift - chip->page_shift)) - 1;
	int readlen = ops->ooblen;
	int len;
	uint8_t *buf = ops->oobbuf;

	if (ops->mode == MTD_OOB_AUTO)
		len = chip->ecc.layout->oobavail;
	else
		len = mtd->oobsize;

	if (unlikely(ops->ooboffs >= len)) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_read_oob: "
			"Attempt to start read outside oob\n");
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(from >= mtd->rl_size ||
		     ops->ooboffs + readlen > ((mtd->rl_size >> chip->page_shift) -
					(from >> chip->page_shift)) * len)) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_read_oob: "
			"Attempt read beyond end of device\n");
		return -EINVAL;
	}

#ifndef CFG_NAND_TOGGLE
	chipnr = (int)(from >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);
#else 
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif

	/* Shift to get page */
	realpage = (int)(from >> chip->page_shift);
	page = realpage & chip->pagemask;

	while(1) {
		sndcmd = chip->ecc.read_oob(mtd, chip, page, sndcmd);

		len = min(len, readlen);
		buf = nand_transfer_oob(chip, buf, ops, len);

		if (!(chip->options & NAND_NO_READRDY)) {
			/*
			 * Apply delay or wait for ready/busy pin. Do this
			 * before the AUTOINCR check, so no problems arise if a
			 * chip which does auto increment is marked as
			 * NOAUTOINCR by the board driver.
			 */
			if (!chip->dev_ready)
				udelay(chip->chip_delay);
			else
				nand_wait_ready(mtd);
		}

		readlen -= len;
		if (!readlen)
			break;

		/* Increment page address */
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}

		/* Check, if the chip supports auto page increment
		 * or if we have hit a block boundary.
		 */
		if (!NAND_CANAUTOINCR(chip) || !(page & blkcheck))
			sndcmd = 1;
	}

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif

	ops->oobretlen = ops->ooblen;
	return 0;
}

/**
 * nand_read_oob - [MTD Interface] NAND read data and/or out-of-band
 * @mtd:	MTD device structure
 * @from:	offset to read from
 * @ops:	oob operation description structure
 *
 * NAND read data and/or out-of-band data
 */
static int nand_read_oob(struct mtd_info *mtd, loff_mtd_t from,
			 struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd->priv;
	int ret = -ENOTSUPP;
	int calc_lock;

	nand_get_device(chip, mtd, FL_READING);
	calc_lock = g_calc_lock;
	from = calc_to_phys(mtd, from);
	LOCK_CALC;

	ops->retlen = 0;

	/* Do not allow reads past end of device */
	if (ops->datbuf && (from + ops->len) > mtd->rl_size) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_read_oob: "
		      "Attempt read beyond end of device\n");
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return -EINVAL;
	}

	switch(ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
	case MTD_OOB_RAW:
		break;

	default:
		goto out;
	}

	//DEBUG(MTD_DEBUG_LEVEL3, "nand_read_oob: from = 0x%08Lx,ooblen= %ld,len:%ld,oobretlen:%ld,ops->mode = %d\n",
	  //    (unsigned long long)from, ops->ooblen,ops->len,ops->oobretlen,ops->mode);

	if (!ops->datbuf)
		ret = nand_do_read_oob(mtd, from, ops);
	else
		ret = nand_do_read_ops(mtd, from, ops);

 out:
	if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
	nand_release_device(mtd);

	return ret;
}

/**
 * nand_write_page_raw - [Intern] raw page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 *
 * Not for syndrome calculating ecc controllers, which use a special oob layout
 */
static void nand_write_page_raw(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf)
{
	chip->write_buf(mtd, buf, mtd->rl_writesize);
	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

/**
 * nand_write_page_raw_syndrome - [Intern] raw page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 *
 * We need a special oob layout and handling even when ECC isn't checked.
 */
static void nand_write_page_raw_syndrome(struct mtd_info *mtd, struct nand_chip *chip,
				const uint8_t *buf)
{
	int eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	uint8_t *oob = chip->oob_poi;
	int steps, size;

	for (steps = chip->ecc.steps; steps > 0; steps--) {
		chip->write_buf(mtd, buf, eccsize);
		buf += eccsize;

		if (chip->ecc.prepad) {
			chip->write_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->read_buf(mtd, oob, eccbytes);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->write_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	size = mtd->oobsize - (oob - chip->oob_poi);
	if (size)
		chip->write_buf(mtd, oob, size);
}
/**
 * nand_write_page_swecc - [REPLACABLE] software ecc based page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 */
static void nand_write_page_swecc(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	/* Software ecc calculation */
	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize)
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	chip->ecc.write_page_raw(mtd, chip, buf);
}

/**
 * nand_write_page_hwecc - [REPLACABLE] hardware ecc based page write function
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 */
static void nand_write_page_hwecc(struct mtd_info *mtd, struct nand_chip *chip,
				  const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	uint8_t *ecc_calc = chip->buffers->ecccalc;
	const uint8_t *p = buf;
	uint32_t *eccpos = chip->ecc.layout->eccpos;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {
		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);
		chip->ecc.calculate(mtd, p, &ecc_calc[i]);
	}

	for (i = 0; i < chip->ecc.total; i++)
		chip->oob_poi[eccpos[i]] = ecc_calc[i];

	chip->write_buf(mtd, chip->oob_poi, mtd->oobsize);
}

/**
 * nand_write_page_syndrome - [REPLACABLE] hardware ecc syndrom based page write
 * @mtd:	mtd info structure
 * @chip:	nand chip info structure
 * @buf:	data buffer
 *
 * The hw generator calculates the error syndrome automatically. Therefor
 * we need a special oob layout and handling.
 */
static void nand_write_page_syndrome(struct mtd_info *mtd,
				    struct nand_chip *chip, const uint8_t *buf)
{
	int i, eccsize = chip->ecc.size;
	int eccbytes = chip->ecc.bytes;
	int eccsteps = chip->ecc.steps;
	const uint8_t *p = buf;
	uint8_t *oob = chip->oob_poi;

	for (i = 0; eccsteps; eccsteps--, i += eccbytes, p += eccsize) {

		chip->ecc.hwctl(mtd, NAND_ECC_WRITE);
		chip->write_buf(mtd, p, eccsize);

		if (chip->ecc.prepad) {
			chip->write_buf(mtd, oob, chip->ecc.prepad);
			oob += chip->ecc.prepad;
		}

		chip->ecc.calculate(mtd, p, oob);
		chip->write_buf(mtd, oob, eccbytes);
		oob += eccbytes;

		if (chip->ecc.postpad) {
			chip->write_buf(mtd, oob, chip->ecc.postpad);
			oob += chip->ecc.postpad;
		}
	}

	/* Calculate remaining oob bytes */
	i = mtd->oobsize - (oob - chip->oob_poi);
	if (i)
		chip->write_buf(mtd, oob, i);
}

/**
 * nand_write_page - [REPLACEABLE] write one page
 * @mtd:	MTD device structure
 * @chip:	NAND chip descriptor
 * @buf:	the data to write
 * @page:	page number to write
 * @cached:	cached programming
 * @raw:	use _raw version of write_page
 */
static int nand_write_page(struct mtd_info *mtd, struct nand_chip *chip,
			   const uint8_t *buf, int page, int cached, int raw)
{
	int status;

	chip->cmdfunc(mtd, NAND_CMD_SEQIN, 0x00, page);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_WRITE_PREPARE | NAND_CTRL_CHANGE);
#endif

	global_page = page;
	if (chip->planenum > 1)
		chip->ecc.write_page(mtd, chip, buf);
	else {
		if (unlikely(raw))
			chip->ecc.write_page_raw(mtd, chip, buf);
		else
			chip->ecc.write_page(mtd, chip, buf);
	}

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN | NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif

	chip->cmdfunc(mtd, NAND_CMD_PAGEPROG, -1, -1);

	/*
	 * Cached progamming disabled for now, Not sure if its worth the
	 * trouble. The speed gain is not very impressive. (2.3->2.6Mib/s)
	 */
	cached = 0;

	if (!cached || !(chip->options & NAND_CACHEPRG)) {
/*
*  __nand_cmd(CMD_PAGEPROG) and __nand_sync() have been done by DMA for jz4750 and
* later chip, status should still be read by "status = chip->waitfunc(mtd, chip)"
*/
	status = chip->waitfunc(mtd, chip);

		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_WRITING, status,
					       page);

		if (status & NAND_STATUS_FAIL)
			return -EIO;
	} else {
		chip->cmdfunc(mtd, NAND_CMD_CACHEDPROG, -1, -1);
		status = chip->waitfunc(mtd, chip);
	}

#ifdef CONFIG_MTD_NAND_VERIFY_WRITE
	/* Send command to read back the data */
	chip->cmdfunc(mtd, NAND_CMD_READ0, 0, page);

#if !defined(CONFIG_SOC_JZ4760B) && !defined(CONFIG_SOC_JZ4770)
	if (chip->verify_buf(mtd, buf, mtd->rl_writesize))
#else
	if (chip->verify_buf(mtd, buf, mtd->writesize))
#endif
		return -EIO;
#endif

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif
	return 0;
}

int nand_write_2plane_pages (struct mtd_info *mtd, int page, u_char *buf_pre, u_char *buf_for, int cached)
{

	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int 	i, status;
	int	cnt = 0;

	/* FIXME: Enable cached programming */
	cached = 0;

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else 
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#endif
	/* Send command to begin auto page programming */
	this->cmdfunc (mtd, NAND_CMD_SEQIN, 0x00, page);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_WRITE_PREPARE | NAND_CTRL_CHANGE);
#endif
	/* write previos page buf */
	this->ecc.write_page(mtd, this, buf_pre);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN  | TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#endif
	/* Send command to begin 2plane pages programming */
	this->cmdfunc (mtd, NAND_CMD_PAGEPROG_PLANES0, -1, -1);
	while(!this->dev_ready(mtd));

	page += this->ppb;
	this->cmdfunc (mtd, NAND_CMD_PAGEPROG_PLANES1, 0, page);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_WRITE_PREPARE | NAND_CTRL_CHANGE);
#endif
	/* write forward page buf */
	this->ecc.write_page(mtd, this, buf_for);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#endif

	/* Send command to actually program the data */
	this->cmdfunc (mtd, cached ? NAND_CMD_CACHEDPROG : NAND_CMD_PAGEPROG, -1, -1);

	if (!cached) {
		/* call wait ready function */
		status = this->waitfunc (mtd, this);
#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif
		/* See if device thinks it succeeded */
		if (status & 0x01) {
//			DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write, page 0x%08x, ", __FUNCTION__, page);
			printk("Failed write, page 0x%08x\n", page);
			return -EIO;
		}
	} else {
		printk("Cached\n");
		/* FIXME: Implement cached programming ! */
		/* wait until cache is ready*/
		/* status = this->waitfunc (mtd, this, FL_CACHEDRPG); */
	}
	return 0;
}

int nand_read_2plane_pages (struct mtd_info *mtd, loff_t page, size_t len, u_char * buf_pre, u_char * buf_for)
{
	int ret;
	size_mtd_t retlen = 0;
	struct nand_chip *this = (struct nand_chip *)mtd->priv;
	int pagesize = mtd->rl_writesize;
	loff_mtd_t from = page * pagesize;
	len = pagesize;
	//debug
	ret = mtd->read (mtd, from, len, &retlen, buf_pre);
	from = (page+this->ppb) * pagesize;
	ret = mtd->read (mtd, from, len, &retlen, buf_for);

	return ret;
}

int nand_random_write_page (struct mtd_info *mtd, struct nand_chip *this, int page,
	u_char *buf, int cached, int offt, int size)
{
	int 	i, status;
	int	cnt = 0;

	/* FIXME: Enable cached programming */
	cached = 0;

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else 
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#endif
	/* Send command to begin auto page programming */
	this->cmdfunc (mtd, NAND_CMD_SEQIN, 0x00, page);
	/* send random write cmd and clumn addr */
	this->cmdfunc(mtd, NAND_CMD_RANDOM_PROG, offt, -1);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_WRITE_PREPARE | NAND_CTRL_CHANGE);
#endif

	this->write_buf(mtd, buf, size);
	/* Write out OOB data */
	//this->write_buf(mtd, oob_buf, mtd->oobsize);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#endif

	/* Send command to actually program the data */
	this->cmdfunc (mtd, cached ? NAND_CMD_CACHEDPROG : NAND_CMD_PAGEPROG, -1, -1);

	if (!cached) {
		/* call wait ready function */
		status = this->waitfunc (mtd, this);
#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif
		/* See if device thinks it succeeded */
		if (status & 0x01) {
//			DEBUG (MTD_DEBUG_LEVEL0, "%s: " "Failed write, page 0x%08x, ", __FUNCTION__, page);
			printk("Failed write, page 0x%08x\n", page);
			return -EIO;
		}
	} else {
		printk("Cached\n");
		/* FIXME: Implement cached programming ! */
		/* wait until cache is ready*/
		/* status = this->waitfunc (mtd, this, FL_CACHEDRPG); */
	}
	return 0;
}

int nand_random_read_page (struct mtd_info *mtd, struct nand_chip *this, int page,
	u_char *buf, int cached, int offt, int size)
{
	int 	i, status;
	int	cnt = 0;

	/* FIXME: Enable cached programming */
	cached = 0;

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else 
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#endif
	/* Send command to begin read page */
	this->cmdfunc (mtd, NAND_CMD_READ0, 0x00, page);
	/* send random read cmd and column addr	*/
	this->cmdfunc(mtd, NAND_CMD_RANDOM_READ, offt, -1);
	/* send random read confirm cmd	*/
	this->cmdfunc(mtd, NAND_CMD_RANDOM_READ_CONFIRM, -1, -1);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
#endif

	this->read_buf(mtd, buf, size);
	
	/* Write out OOB data */
	//this->write_buf(mtd, oob_buf, mtd->oobsize);

#ifdef CFG_NAND_TOGGLE
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
#else
	this->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif

	return 0;
}

/**
 * nand_fill_oob - [Internal] Transfer client buffer to oob
 * @chip:	nand chip structure
 * @oob:	oob data buffer
 * @ops:	oob ops structure
 */
static uint8_t *nand_fill_oob(struct nand_chip *chip, uint8_t *oob,
				  struct mtd_oob_ops *ops)
{
	size_mtd_t len = ops->ooblen;

	switch(ops->mode) {

	case MTD_OOB_PLACE:
	case MTD_OOB_RAW:
		memcpy(chip->oob_poi + ops->ooboffs, oob, len);
		return oob + len;

	case MTD_OOB_AUTO: {
		struct nand_oobfree *free = chip->ecc.layout->oobfree;
		uint32_t boffs = 0, woffs = ops->ooboffs;
		size_mtd_t bytes = 0;

		for(; free->length && len; free++, len -= bytes) {
			/* Write request not from offset 0 ? */
			if (unlikely(woffs)) {
				if (woffs >= free->length) {
					woffs -= free->length;
					continue;
				}
				boffs = free->offset + woffs;
				bytes = min_t(size_mtd_t, len,
					      (free->length - woffs));
				woffs = 0;
			} else {
				bytes = min_t(size_mtd_t, len, free->length);
				boffs = free->offset;
			}
			memcpy(chip->oob_poi + boffs, oob, bytes);
			oob += bytes;
		}
		return oob;
	}
	default:
		BUG();
	}
	return NULL;
}

#define NOTALIGNED(x)	(x & (chip->subpagesize - 1)) != 0

/**
 * nand_do_write_ops - [Internal] NAND write with ECC
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @ops:	oob operations description structure
 *
 * NAND write with ECC
 */
static int nand_do_write_ops(struct mtd_info *mtd, loff_mtd_t to,
			     struct mtd_oob_ops *ops)
{
	int chipnr, realpage, page, blockmask, column;
	struct nand_chip *chip = mtd->priv;
	uint32_t writelen = ops->len;
	uint8_t *oob = ops->oobbuf;
	uint8_t *buf = ops->datbuf;
	int ret, subpage;

	ops->retlen = 0;
	if (!writelen)
		return 0;

	/* reject writes, which are not page aligned */
#if !defined(CONFIG_SOC_JZ4760B) && !defined(CONFIG_SOC_JZ4770) && !defined(CONFIG_SOC_JZ4780)
	if (NOTALIGNED(to) || NOTALIGNED(ops->len)) {
#else
	if (NOTALIGNED(to)) {
#endif
		printk(KERN_NOTICE "nand_write: Attempt to write not page aligned data.\n");
		printk(KERN_NOTICE "Addr:%llu\n",to);

		return -EINVAL;
	}

	column = to & (mtd->rl_writesize - 1);

#if !defined(CONFIG_SOC_JZ4760B) && !defined(CONFIG_SOC_JZ4770) && !defined(CONFIG_SOC_JZ4780)
	subpage = column || (writelen & (mtd->rl_writesize - 1));
#else
	subpage = column || (writelen % mtd->writesize != 0);
#endif

	if (subpage && oob)
		return -EINVAL;

#ifndef CFG_NAND_TOGGLE
	chipnr = (int)(to >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);
#else
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		return -EIO;

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif

	realpage = (int)(to >> chip->page_shift);
	page = realpage & chip->pagemask;
	blockmask = chip->ppb - 1;//(1 << (chip->phys_erase_shift - chip->page_shift)) - 1;

	/* Invalidate the page cache, when we write to the cached page */
	if (to <= (chip->pagebuf << chip->page_shift) &&
	    (chip->pagebuf << chip->page_shift) < (to + ops->len))
		chip->pagebuf = -1;

	/* If we're not given explicit OOB data, let it be 0xFF */
	if (likely(!oob))
		memset(chip->oob_poi, 0xff, mtd->oobsize);

	while(1) {

#if !defined(CONFIG_SOC_JZ4760B) && !defined(CONFIG_SOC_JZ4770) && !defined(CONFIG_SOC_JZ4780)
		int bytes = mtd->rl_writesize;
#else
		int bytes = mtd->writesize;
#endif
		int cached = (writelen > bytes) && (page != blockmask);
		uint8_t *wbuf = buf;

		memset(chip->oob_poi, 0xff, mtd->oobsize);

		/* Partial page write ? */
		if (unlikely(column || writelen < (mtd->writesize - 1))) {
			cached = 0;
			bytes = min_t(int, bytes - column, (int) writelen);
			chip->pagebuf = -1;
			memset(chip->buffers->databuf, 0xff, mtd->writesize);
			memcpy(&chip->buffers->databuf[column], buf, bytes);
			wbuf = chip->buffers->databuf;
		}

		if (unlikely(oob)) {
			oob = nand_fill_oob(chip, oob, ops);
			
		}

#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_NCE | NAND_CTRL_CHANGE);
#endif

		ret = chip->write_page(mtd, chip, wbuf, page, cached,
				       (ops->mode == MTD_OOB_RAW));
		if (ret)
			break;

		writelen -= bytes;
		if (!writelen)
			break;

		column = 0;
		buf += bytes;
		realpage++;

		page = realpage & chip->pagemask;
		/* Check, if we cross a chip boundary */
		if (!page) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);
		}
	}

	ops->retlen = ops->len - writelen;
	if (unlikely(oob))
		ops->oobretlen = ops->ooblen;
	return ret;
}

/**
 * nand_write - [MTD Interface] NAND write with ECC
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @len:	number of bytes to write
 * @retlen:	pointer to variable to store the number of written bytes
 * @buf:	the data to write
 *
 * NAND write with ECC
 */
static int nand_write(struct mtd_info *mtd, loff_mtd_t to, size_mtd_t len,
			  size_mtd_t *retlen, const uint8_t *buf)
{
	struct nand_chip *chip = mtd->priv;
	int ret;
	int calc_lock;
	
	nand_get_device(chip, mtd, FL_WRITING);
	calc_lock = g_calc_lock;
	
	to = calc_to_phys(mtd,to);	

	LOCK_CALC;

	/* Do not allow reads past end of device */
	if ((to + len) > mtd->rl_size)
	{
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return -EINVAL;
	}
	if (!len)
	{
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return 0;
	}
	
	chip->ops.len = len;
	chip->ops.datbuf = (uint8_t *)buf;
	chip->ops.oobbuf = NULL;

	ret = nand_do_write_ops(mtd, to, &chip->ops);

	*retlen = chip->ops.retlen;

	if (calc_lock != g_calc_lock)
		UNLOCK_CALC;
	nand_release_device(mtd);
	
	return ret;
}

/**
 * nand_do_write_oob - [MTD Interface] NAND write out-of-band
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @ops:	oob operation description structure
 *
 * NAND write out-of-band
 */
static int nand_do_write_oob(struct mtd_info *mtd, loff_mtd_t to,
			     struct mtd_oob_ops *ops)
{
	int chipnr, page, status, len;
	struct nand_chip *chip = mtd->priv;

	DEBUG(MTD_DEBUG_LEVEL3, "nand_write_oob: to = 0x%08x, len = %i\n",
	      (unsigned int)to, (int)ops->ooblen);

	if (ops->mode == MTD_OOB_AUTO)
		len = chip->ecc.layout->oobavail;
	else
		len = mtd->oobsize;

	/* Do not allow write past end of page */
	if ((ops->ooboffs + ops->ooblen) > len) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_write_oob: "
		      "Attempt to write past end of page\n");
		return -EINVAL;
	}

	if (unlikely(ops->ooboffs >= len)) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_do_write_oob: "
			"Attempt to start write outside oob\n");
		return -EINVAL;
	}

	/* Do not allow reads past end of device */
	if (unlikely(to >= mtd->rl_size ||
		     ops->ooboffs + ops->ooblen >
			((mtd->rl_size >> chip->page_shift) -
			 (to >> chip->page_shift)) * len)) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_do_write_oob: "
			"Attempt write beyond end of device\n");
		return -EINVAL;
	}

#ifndef CFG_NAND_TOGGLE
	chipnr = (int)(to >> chip->chip_shift);
	chip->select_chip(mtd, chipnr);
#else
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif

	/* Shift to get page */
	page = (int)(to >> chip->page_shift);

	/*
	 * Reset the chip. Some chips (like the Toshiba TC5832DC found in one
	 * of my DiskOnChip 2000 test units) will clear the whole data page too
	 * if we don't do this. I have no clue why, but I seem to have 'fixed'
	 * it in the doc2000 driver in August 1999.  dwmw2.
	 */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

	/* Check, if it is write protected */
	if (nand_check_wp(mtd))
		return -EROFS;

	/* Invalidate the page cache, if we write to the cached page */
	if (page == chip->pagebuf)
		chip->pagebuf = -1;

	memset(chip->oob_poi, 0xff, mtd->oobsize);
	nand_fill_oob(chip, ops->oobbuf, ops);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_WRITE_PREPARE | NAND_CTRL_CHANGE);
#endif
	status = chip->ecc.write_oob(mtd, chip, page & chip->pagemask);
	memset(chip->oob_poi, 0xff, mtd->oobsize);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif
	if (status)
		return status;

	ops->oobretlen = ops->ooblen;

	return 0;
}

/**
 * nand_write_oob - [MTD Interface] NAND write data and/or out-of-band
 * @mtd:	MTD device structure
 * @to:		offset to write to
 * @ops:	oob operation description structure
 */
static int nand_write_oob(struct mtd_info *mtd, loff_mtd_t to,
			  struct mtd_oob_ops *ops)
{
	struct nand_chip *chip = mtd->priv;
	int ret = -ENOTSUPP;
	int calc_lock;
	
	nand_get_device(chip, mtd, FL_WRITING);

	calc_lock = g_calc_lock;
	to = calc_to_phys(mtd, to);		
	LOCK_CALC;

	ops->retlen = 0;

	/* Do not allow writes past end of device */
	if (ops->datbuf && (to + ops->len) > mtd->rl_size) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_write_oob: "
		      "Attempt write beyond end of device\n");

		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;

		nand_release_device(mtd);
		return -EINVAL;
	}


	switch(ops->mode) {
	case MTD_OOB_PLACE:
	case MTD_OOB_AUTO:
	case MTD_OOB_RAW:
		break;

	default:
		goto out;
	}

	if (!ops->datbuf){
		ret = nand_do_write_oob(mtd, to, ops);
	}else{
		ret = nand_do_write_ops(mtd, to, ops);
	}
 out:
	if (calc_lock != g_calc_lock)
		UNLOCK_CALC;
	
	nand_release_device(mtd);

	return ret;
}

/**
 * single_erease_cmd - [GENERIC] NAND standard block erase command function
 * @mtd:	MTD device structure
 * @page:	the page address of the block which will be erased
 *
 * Standard erase command for NAND chips
 */
static void single_erase_cmd(struct mtd_info *mtd, int page)
{
	struct nand_chip *chip = mtd->priv;
	/* Send commands to erase a block */
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
	chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1);
}

/**
 * multi_erease_cmd - [GENERIC] AND specific block erase command function
 * @mtd:	MTD device structure
 * @page:	the page address of the block which will be erased
 *
 * AND multi block erase command function
 * Erase 4 consecutive blocks
 */
static void multi_erase_cmd(struct mtd_info *mtd, int page)
{
	struct nand_chip *chip = mtd->priv;
	/* Send commands to erase a block */
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page++);
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page++);
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page++);
	chip->cmdfunc(mtd, NAND_CMD_ERASE1, -1, page);
	chip->cmdfunc(mtd, NAND_CMD_ERASE2, -1, -1);
}

/**
 * nand_erase - [MTD Interface] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 *
 * Erase one ore more blocks
 */
static int nand_erase(struct mtd_info *mtd, struct erase_info *instr)
{
	return nand_erase_nand(mtd, instr, 0);
}

#define BBT_PAGE_MASK	0xffffff3f

/**
 * nand_erase_nand - [Internal] erase block(s)
 * @mtd:	MTD device structure
 * @instr:	erase instruction
 * @allowbbt:	allow erasing the bbt area
 *
 * Erase one ore more blocks
 */
int nand_erase_nand(struct mtd_info *mtd, struct erase_info *instr,
		    int allowbbt)
{
	int page, status, pages_per_block, ret, chipnr;
	struct nand_chip *chip = mtd->priv;
	loff_mtd_t rewrite_bbt[NAND_MAX_CHIPS]={0};
	unsigned int bbt_masked_page = 0xffffffff;
	loff_mtd_t len;
	int calc_lock;
	
	/* Grab the lock and see if the device is available */
	nand_get_device(chip, mtd, FL_ERASING);
	calc_lock = g_calc_lock;
	instr->addr = calc_to_phys(mtd,instr->addr);
	LOCK_CALC;

	DEBUG(MTD_DEBUG_LEVEL3, "nand_erase: start = 0x%012llx, len = %llu\n",
	      (unsigned long long)instr->addr, (unsigned long long)instr->len);

	/* Start address must align on block boundary */
	if (instr->addr & ((1 << chip->phys_erase_shift) - 1)) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_erase: Unaligned address\n");
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return -EINVAL;
	}

	/* Length must align on block boundary */
//	if (instr->len & ((1 << chip->phys_erase_shift) - 1)) {
	if (mtd_mod_by_eb(instr->len, mtd)) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_erase: "
		      "Length not block aligned\n");
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return -EINVAL;
	}

	/* Do not allow erase past end of device */
	if ((instr->len + instr->addr) > mtd->rl_size) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_erase: "
		      "Erase past end of device\n");
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return -EINVAL;
	}

	instr->fail_addr = MTD_FAIL_ADDR_UNKNOWN;

	/* Shift to get first page */
	page = (int)(instr->addr >> chip->page_shift);

	chipnr = (int)(instr->addr >> chip->chip_shift);

	/* Calculate pages in each block */
	pages_per_block = chip->ppb;//1 << (chip->phys_erase_shift - chip->page_shift);

#ifndef CFG_NAND_TOGGLE
	/* Select the NAND device */
	chip->select_chip(mtd, chipnr);
#else 
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif

	/* Check, if it is write protected */
	if (nand_check_wp(mtd)) {
		DEBUG(MTD_DEBUG_LEVEL0, "nand_erase: "
		      "Device is write protected!!!\n");
		printk("nand_erase: "
		      "Device is write protected!!!\n");
		instr->state = MTD_ERASE_FAILED;
		goto erase_exit;
	}

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif

	/*
	 * If BBT requires refresh, set the BBT page mask to see if the BBT
	 * should be rewritten. Otherwise the mask is set to 0xffffffff which
	 * can not be matched. This is also done when the bbt is actually
	 * erased to avoid recusrsive updates
	 */
	if (chip->options & BBT_AUTO_REFRESH && !allowbbt)
		bbt_masked_page = chip->bbt_td->pages[chipnr] & BBT_PAGE_MASK;

	/* Loop through the pages */
	len = instr->len;

	instr->state = MTD_ERASING;

//	buffer_dump(chip->bbt,mtd->rl_size >> (chip->bbt_erase_shift + 2),"bbt",__FILE__,__func__,len);

	while (len) {
		/*
		 * check if we have a bad block, we do not erase bad blocks !
		 */
		if (nand_block_checkbad(mtd, ((loff_mtd_t) page) <<
					chip->page_shift, 0, allowbbt)) {
#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif
			printk(KERN_WARNING "nand_erase: attempt to erase a "
			       "bad block at page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			goto erase_exit;
		}

		/*
		 * Invalidate the page cache, if we erase the block which
		 * contains the current cached page
		 */
		if (page <= chip->pagebuf && chip->pagebuf < (page + pages_per_block))
			chip->pagebuf = -1;

#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#else
		chip->select_chip(mtd, chipnr);
#endif
		chip->erase_cmd(mtd, page & chip->pagemask);

		status = chip->waitfunc(mtd, chip);

#ifdef CFG_NAND_TOGGLE
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif
		/*
		 * See if operation failed and additional status checks are
		 * available
		 */
		if ((status & NAND_STATUS_FAIL) && (chip->errstat))
			status = chip->errstat(mtd, chip, FL_ERASING,
					       status, page);

		/* See if block erase succeeded */
		if (status & NAND_STATUS_FAIL) {
			DEBUG(MTD_DEBUG_LEVEL0, "nand_erase: "
			      "Failed erase, page 0x%08x\n", page);
			printk("nand_erase: "
			      "Failed erase, page 0x%08x\n", page);
			instr->state = MTD_ERASE_FAILED;
			instr->fail_addr =
				((loff_mtd_t)page << chip->page_shift);
			goto erase_exit;
		}

		/*
		 * If BBT requires refresh, set the BBT rewrite flag to the
		 * page being erased
		 */
		if (bbt_masked_page != 0xffffffff &&
		    (page & BBT_PAGE_MASK) == bbt_masked_page)
			    rewrite_bbt[chipnr] =
					((loff_mtd_t)page << chip->page_shift);

		/* Increment page address and decrement length */
		//len -= (1 << chip->phys_erase_shift);
		len -= mtd->erasesize;
		page += pages_per_block;

		/* Check, if we cross a chip boundary */
		if (len && !(page & chip->pagemask)) {
			chipnr++;
			chip->select_chip(mtd, -1);
			chip->select_chip(mtd, chipnr);

			/*
			 * If BBT requires refresh and BBT-PERCHIP, set the BBT
			 * page mask to see if this BBT should be rewritten
			 */
			if (bbt_masked_page != 0xffffffff &&
			    (chip->bbt_td->options & NAND_BBT_PERCHIP))
				bbt_masked_page = chip->bbt_td->pages[chipnr] &
					BBT_PAGE_MASK;
		}
	}
	instr->state = MTD_ERASE_DONE;

 erase_exit:

	ret = instr->state == MTD_ERASE_DONE ? 0 : -EIO;

	/* Deselect and wake up anyone waiting on the device */
//	nand_release_device(mtd);

	/* Do call back function */
	if (!ret)
		mtd_erase_callback(instr);

	/*
	 * If BBT requires refresh and erase was successful, rewrite any
	 * selected bad block tables
	 */
	if (bbt_masked_page == 0xffffffff || ret)
	{
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		nand_release_device(mtd);
		return ret;
	}
	
	for (chipnr = 0; chipnr < chip->numchips; chipnr++) {
		if (!rewrite_bbt[chipnr])
			continue;
		/* update the BBT for chip */
		DEBUG(MTD_DEBUG_LEVEL0, "nand_erase_nand: nand_update_bbt "
		      "(%d:0x%0llx 0x%0x)\n", chipnr, rewrite_bbt[chipnr],
		      chip->bbt_td->pages[chipnr]);
		nand_update_bbt(mtd, rewrite_bbt[chipnr]);
	}

	if (calc_lock != g_calc_lock)
		UNLOCK_CALC;
	
	nand_release_device(mtd);
	/* Return more or less happy */
	return ret;
}

/**
 * nand_sync - [MTD Interface] sync
 * @mtd:	MTD device structure
 *
 * Sync is actually a wait for chip ready function
 */
static void nand_sync(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	DEBUG(MTD_DEBUG_LEVEL3, "nand_sync: called\n");

	/* Grab the lock and see if the device is available */
	nand_get_device(chip, mtd, FL_SYNCING);
	/* Release it and go back */
	nand_release_device(mtd);
}

/**
 * nand_block_isbad - [MTD Interface] Check if block at offset is bad
 * @mtd:	MTD device structure
 * @offs:	offset relative to mtd start
 */
static int nand_block_isbad(struct mtd_info *mtd, loff_mtd_t offs)
{
	int calc_lock = g_calc_lock;
	offs = calc_to_phys(mtd, offs);	
	LOCK_CALC;

	/* Check for invalid offset */
	if (offs > mtd->rl_size){
		if (calc_lock != g_calc_lock)
			UNLOCK_CALC;
		return -EINVAL;
	}

	if (calc_lock != g_calc_lock)
		UNLOCK_CALC;

	return nand_block_checkbad(mtd, offs, 1, 0);
}

/**
 * nand_block_markbad - [MTD Interface] Mark block at the given offset as bad
 * @mtd:	MTD device structure
 * @ofs:	offset relative to mtd start
 */
static int nand_block_markbad(struct mtd_info *mtd, loff_mtd_t ofs)
{
	struct nand_chip *chip = mtd->priv;
	int ret;
	int calc_lock = g_calc_lock;

	ofs = calc_to_phys(mtd, ofs);	
	LOCK_CALC;

	if ((ret = nand_block_isbad(mtd, ofs))) {
		/* If it was bad already, return success and do nothing. */
	        if (calc_lock != g_calc_lock)
		  UNLOCK_CALC;

		if (ret > 0)
			return 0;
		return ret;
	}

	ret = chip->block_markbad(mtd, ofs);

	if (calc_lock != g_calc_lock)
	  UNLOCK_CALC;


	return ret;
}

/**
 * nand_suspend - [MTD Interface] Suspend the NAND flash
 * @mtd:	MTD device structure
 */
static int nand_suspend(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	return nand_get_device(chip, mtd, FL_PM_SUSPENDED);
}

/**
 * nand_resume - [MTD Interface] Resume the NAND flash
 * @mtd:	MTD device structure
 */
static void nand_resume(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

	if (chip->state == FL_PM_SUSPENDED)
		nand_release_device(mtd);
	else
		printk(KERN_ERR "nand_resume() called for a chip which is not "
		       "in suspended state\n");
}

/*
 * Set default functions
 */
static void nand_set_defaults(struct nand_chip *chip, int busw)
{
	/* check for proper chip_delay setup, set 20us if not */
	if (!chip->chip_delay)
		chip->chip_delay = 20;

	/* check, if a user supplied command function given */
	if (chip->cmdfunc == NULL){
#ifdef CFG_NAND_TOGGLE
		chip->cmdfunc = nand_command_lp;
#else
		chip->cmdfunc = nand_command;
#endif
	}
	/* check, if a user supplied wait function given */
	if (chip->waitfunc == NULL)
		chip->waitfunc = nand_wait;

	if (!chip->select_chip)
		chip->select_chip = nand_select_chip;
	if (!chip->read_byte)
		chip->read_byte = busw ? nand_read_byte16 : nand_read_byte;
	if (!chip->read_word)
		chip->read_word = nand_read_word;
	if (!chip->block_bad)
		chip->block_bad = nand_block_bad;
	if (!chip->block_markbad)
		chip->block_markbad = nand_default_block_markbad;
	if (!chip->write_buf)
		chip->write_buf = busw ? nand_write_buf16 : nand_write_buf;
	if (!chip->read_buf)
		chip->read_buf = busw ? nand_read_buf16 : nand_read_buf;
	if (!chip->verify_buf)
		chip->verify_buf = busw ? nand_verify_buf16 : nand_verify_buf;
	if (!chip->scan_bbt)
		chip->scan_bbt = nand_default_bbt;

	if (!chip->controller) {
		chip->controller = &chip->hwcontrol;
		spin_lock_init(&chip->controller->lock);
		init_waitqueue_head(&chip->controller->wq);
	}

	chip->nand_type = NAND_TYPE_TOGGLE;
}

//#ifdef CONFIG_MTD_DEBUG
#if 1
void dump_nand_and_mtd_info(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;
	printk("\nNAND & MTD Info:\n");
	printk("\nNAND Info:\n");
	printk("chip_delay:		%d\n",chip->chip_delay);
	printk("options:		%d\n",chip->options);
	printk("page_shift:		%d\n",chip->page_shift);
	printk("phys_erase_shift:	%d\n",chip->phys_erase_shift);
	printk("bbt_erase_shift:	%d\n",chip->bbt_erase_shift);
	printk("chip_shift:		%d\n",chip->chip_shift);
	printk("numchips:		%d\n",chip->numchips);
	printk("chipsize:		%lld\n",chip->chipsize);
	printk("ppb:			%d\n",chip->ppb);
	printk("eccpos:			%d\n",chip->eccpos);
	printk("pagemask:		0x%08x\n",chip->pagemask);
	printk("pagebuf:		%d\n",chip->pagebuf);
	printk("subpagesize:		%d\n",chip->subpagesize);
	printk("cellinfo:		%d\n",chip->cellinfo);
	printk("badblockpos:		%d\n",chip->badblockpos);
	printk("realplanenum:		%d\n",chip->realplanenum);

	printk("\nMTD Info:\n");

	printk("type:			%d\n",mtd->type);
	printk("flags:			%d\n",mtd->flags);
	printk("size:			%lld\n",mtd->size);
	printk("rl_size:		%lld\n",mtd->rl_size);
	printk("erasesize:		%d\n",mtd->erasesize);
	printk("rl_erasesize:		%d\n",mtd->rl_erasesize);
	printk("writesize:		%d\n",mtd->writesize);
	printk("rl_writesize:		%d\n",mtd->rl_writesize);
	printk("freesize:		%d\n",mtd->freesize);
	printk("oobsize:		%d\n",mtd->oobsize);
	printk("oobavail:		%d\n",mtd->oobavail);
#if DELETE
	printk("erasesize_shift:	%d\n",mtd->erasesize_shift);
	printk("writesize_shift:	%d\n",mtd->writesize_shift);
	printk("erasesize_mask:		0x%08x\n",mtd->erasesize_mask);
	printk("writesize_mask:		0x%08x\n",mtd->writesize_mask);
#endif
	printk("name:			%s\n",mtd->name);
	printk("index:			%d\n",mtd->index);
	printk("numeraseregions:	%d\n",mtd->numeraseregions);
	printk("subpage_sft:		%d\n",mtd->subpage_sft);
	
	printk("\nThat All.\n\n");
}

EXPORT_SYMBOL_GPL(dump_nand_and_mtd_info);
#endif

/*
 * Get the flash and manufacturer id and lookup if the type is supported
 */
static struct nand_flash_dev *nand_get_flash_type(struct mtd_info *mtd,
						  struct nand_chip *chip,
						  int busw, int *maf_id)
{
	struct nand_flash_dev *type = NULL;
	int i, dev_id, maf_idx;
	int tmp_id, tmp_manf;
	uint32_t ext_id_p = 0;
	uint8_t *ext_id = (uint8_t *)&ext_id_p;
	int flash_num;

	/* Select the device */
	chip->select_chip(mtd, 0);

	/*
	 * Reset the chip, required by some chips (e.g. Micron MT29FxGxxxxx)
	 * after power-up
	 */
	chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);

#ifdef CFG_NAND_TOGGLE
	//dqs delay line
	REG_NEMC_TGDR = 0x29;
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN | NAND_NCE | NAND_CTRL_CHANGE);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | NAND_CTRL_CHANGE);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_SET | NAND_CTRL_CHANGE);
#endif

	/* Send the command for reading device ID */
	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
#endif
	/* Read manufacturer and device IDs */
	*maf_id = chip->read_byte(mtd);
	printk(">>>maf_id is 0x%02x\n", *maf_id);
	dev_id = chip->read_byte(mtd);
	printk(">>>dev_id is 0x%02x\n", dev_id);

#ifdef CFG_NAND_TOGGLE
	dev_id = chip->read_byte(mtd);
	chip->read_byte(mtd);
	ext_id[0] = chip->read_byte(mtd);
	printk(">>>ext_id is 0x%02x\n", ext_id[0]);
	chip->read_byte(mtd);
	ext_id[1] = chip->read_byte(mtd);
	printk(">>>ext_id is 0x%02x\n", ext_id[1]);
	chip->read_byte(mtd);
	ext_id[2] = chip->read_byte(mtd);
	printk(">>>ext_id is 0x%02x\n", ext_id[2]);
	chip->read_byte(mtd);
	ext_id[3] = chip->read_byte(mtd);
	printk(">>>ext_id is 0x%02x\n", ext_id[3]);
#else
	nand_read_buf(mtd, ext_id, 4);
#endif

	chip->cellinfo = ext_id[0];

	/* Try again to make sure, as some systems the bus-hold or other
	 * interface concerns can cause random data which looks like a
	 * possibly credible NAND flash to appear. If the two results do
	 * not match, ignore the device completely.
	 */
#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
#endif

	chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);

	/* Read manufacturer and device IDs */

#ifdef CFG_NAND_TOGGLE
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, TNAND_READ_PREPARE | NAND_CTRL_CHANGE);
#endif
	tmp_manf = chip->read_byte(mtd);
	tmp_id = chip->read_byte(mtd);

#ifdef CFG_NAND_TOGGLE
	tmp_id = chip->read_byte(mtd);
	chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_FCE_CLEAN | TNAND_DAE_CLEAN | NAND_CTRL_CHANGE);
#else
		chip->cmd_ctrl(mtd, NAND_CMD_NONE, NAND_CTRL_CHANGE);
#endif

	if (tmp_manf != *maf_id || tmp_id != dev_id) {
		printk(KERN_INFO "%s: second ID read did not match "
		       "%02x,%02x against %02x,%02x\n", __func__,
		       *maf_id, dev_id, tmp_manf, tmp_id);
		return ERR_PTR(-ENODEV);
	}

	flash_num = get_flash_num();

	dev_id |= (*maf_id << 8);
	printk("dev_id:0x%08x\n",dev_id);
	printk("ext_id:0x%08x\n",ext_id_p);

	#define EXTID_MASK	0xffffffff

	/* Lookup the flash id */
	for (i = 0; i < flash_num; i++) {
		if ((dev_id == nand_flash_ids[i].id) && ((ext_id_p & EXTID_MASK) == (nand_flash_ids[i].extid & EXTID_MASK))) {
			type =  &nand_flash_ids[i];
			break;
		}
	}

	if (!type)
		return ERR_PTR(-ENODEV);

	if (!mtd->name)
		mtd->name = type->name;

	chip->chipsize = (uint64_t)type->erasesize * type->maxvalidblocks;
	
	mtd->rl_writesize = type->pagesize;
	mtd->rl_erasesize = type->erasesize;
	mtd->oobsize = type->oobsize;
	
#if defined(CONFIG_MTD_NAND_MULTI_PLANE)
	chip->realplanenum = type->realplanenum;
#else
	chip->realplanenum = 1;
#endif

	if (chip->realplanenum > 1) {     /* use muti planes mode */
		chip->planenum = 2;
		mtd->rl_writesize *= 2;  /* two pages as one page */
		mtd->rl_erasesize *= 2;  /* two blocks as one block */
		mtd->oobsize *= 2;
	} else
		chip->planenum = 1;

	/* Try to identify manufacturer */
	for (maf_idx = 0; nand_manuf_ids[maf_idx].id != 0x0; maf_idx++) {
		if (nand_manuf_ids[maf_idx].id == *maf_id)
			break;
	}

	/*
	 * Check, if buswidth is correct. Hardware drivers should set
	 * chip correct !
	 */
	if (busw != (chip->options & NAND_BUSWIDTH_16)) {
		printk(KERN_INFO "NAND device: Manufacturer ID:"
		       " 0x%02x, Chip ID: 0x%02x (%s %s)\n", *maf_id,
		       dev_id, nand_manuf_ids[maf_idx].name, mtd->name);
		printk(KERN_WARNING "NAND bus width %d instead %d bit\n",
		       (chip->options & NAND_BUSWIDTH_16) ? 16 : 8,
		       busw ? 16 : 8);
		return ERR_PTR(-EINVAL);
	}

	/* Calculate the address shift from the page size */
	chip->page_shift = ffs(mtd->rl_writesize) - 1;
	/* Convert chipsize to number of pages per chip -1. */
	chip->pagemask = (chip->chipsize >> chip->page_shift) - 1;

	chip->bbt_erase_shift = chip->phys_erase_shift =
		ffs(mtd->rl_erasesize) - 1;
	chip->chip_shift = ffs_ll(chip->chipsize) - 1;
	chip->ppb = (1 << (chip->phys_erase_shift - chip->page_shift));
	
	/* Set the bad block position */
	chip->badblockpos = mtd->rl_writesize > 512 ?
		NAND_LARGE_BADBLOCK_POS : NAND_SMALL_BADBLOCK_POS;

	/* Get chip options, preserve non chip based options */
	chip->options &= ~NAND_CHIPOPTIONS_MSK;
	chip->options |= type->options & NAND_CHIPOPTIONS_MSK;

	/*
	 * Set chip as a default. Board drivers can override it, if necessary
	 */
	chip->options |= NAND_NO_AUTOINCR;

	/* Check if chip is a not a samsung device. Do not clear the
	 * options for chips which are not having an extended id.
	 */
	if (*maf_id != NAND_MFR_SAMSUNG && !type->pagesize)
		chip->options &= ~NAND_SAMSUNG_LP_OPTIONS;

	/* Check for AND chips with 4 page planes */
	if (chip->options & NAND_4PAGE_ARRAY)
		chip->erase_cmd = multi_erase_cmd;
	else
		chip->erase_cmd = single_erase_cmd;

	/* Do not replace user supplied command function ! */
	if (mtd->rl_writesize > 512 && chip->cmdfunc == nand_command)
		chip->cmdfunc = nand_command_lp;

	printk(KERN_INFO "NAND device: Manufacturer ID:"
	       " 0x%02x, Chip ID: 0x%02x (%s %s) planenum:%d\n", *maf_id, dev_id,
	       nand_manuf_ids[maf_idx].name, type->name, chip->realplanenum);

	return type;
}

/**
 * nand_scan_ident - [NAND Interface] Scan for the NAND device
 * @mtd:	     MTD device structure
 * @maxchips:	     Number of chips to scan for
 *
 * This is the first phase of the normal nand_scan() function. It
 * reads the flash ID and sets up MTD fields accordingly.
 *
 * The mtd->owner field must be set to the module of the caller.
 */
int nand_scan_ident(struct mtd_info *mtd, int maxchips)
{
	int i, busw, nand_maf_id;
	struct nand_chip *chip = (struct nand_chip *)mtd->priv;
	struct nand_flash_dev *type;

	/* Get buswidth to select the correct functions */
	busw = chip->options & NAND_BUSWIDTH_16;

	/* Set the default functions */
	nand_set_defaults(chip, busw);

	/* Read the flash type */
	type = nand_get_flash_type(mtd, chip, busw, &nand_maf_id);

	global_mafid = nand_maf_id;
	if (IS_ERR(type)) {
		printk(KERN_WARNING "No NAND device found!!!\n");
		chip->select_chip(mtd, -1);
		return PTR_ERR(type);
	}

	/* Check for a chip array */
	for (i = 1; i < maxchips; i++) {
		chip->select_chip(mtd, i);
		/* See comment in nand_get_flash_type for reset */
		chip->cmdfunc(mtd, NAND_CMD_RESET, -1, -1);
		/* Send the command for reading device ID */
		chip->cmdfunc(mtd, NAND_CMD_READID, 0x00, -1);
		/* Read manufacturer and device IDs */
		if (nand_maf_id != chip->read_byte(mtd) ||
		    type->id != chip->read_byte(mtd))
			break;
	}
	if (i > 1)
		printk(KERN_INFO "%d NAND chips detected\n", i);

	/* Store the number of chips and calc total size for mtd */
	chip->numchips = i;
	mtd->rl_size = i * chip->chipsize;

	return 0;
}


#if defined(CONFIG_SOC_JZ4760B) && defined(CONFIG_MTD_NAND_DMA)
extern int jz4760b_nand_dma_init(struct mtd_info *);
#endif
#if defined(CONFIG_SOC_JZ4770) && defined(CONFIG_MTD_NAND_DMA)
extern int jz4770_nand_dma_init(struct mtd_info *);
#endif

/**
 * nand_scan_tail - [NAND Interface] Scan for the NAND device
 * @mtd:	    MTD device structure
 *
 * This is the second phase of the normal nand_scan() function. It
 * fills out all the uninitialized function pointers with the defaults
 * and scans for a bad block table if appropriate.
 */
int nand_scan_tail(struct mtd_info *mtd)
{
	int i, res;
	struct nand_chip *chip = mtd->priv;

	if (!(chip->options & NAND_OWN_BUFFERS))
		chip->buffers = kmalloc(sizeof(*chip->buffers), GFP_KERNEL);
	if (!chip->buffers)
		return -ENOMEM;

	/*
	 * If no default placement scheme is given, select an appropriate one
	 */
	if (!chip->ecc.layout) {
		switch (mtd->oobsize) {
		case 8:
			chip->ecc.layout = &nand_oob_8;
			break;
		case 16:
			chip->ecc.layout = &nand_oob_16;
			break;
		case 64:
			chip->ecc.layout = &nand_oob_64;
			break;
		case 128:
			if (chip->planenum > 1)
				chip->ecc.layout = &nand_oob_64;
			else
				chip->ecc.layout = &nand_oob_128;
			break;
		case 218:
			chip->ecc.layout = &nand_oob_218;
			break;
		case 256:
			if (chip->planenum > 1)
				chip->ecc.layout = &nand_oob_128;
			break;
		case 436:
			chip->ecc.layout = &nand_oob_436;
			break;
		case 1024:
			chip->ecc.layout = &nand_oob_1024;
			break;
		default:
			printk(KERN_WARNING "No oob scheme defined for "
			       "oobsize %d\n", mtd->oobsize);
			BUG();
		}
	}

	chip->eccpos = chip->ecc.layout->eccpos[0];

#if (defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_SOC_JZ4770) || defined(CONFIG_SOC_JZ4780)) && defined(CONFIG_MTD_HW_BCH_ECC)
	mtd->freesize = calc_free_size(mtd);
#else
	mtd->freesize = 0;
#endif

	mtd->writesize = mtd->rl_writesize - mtd->freesize;
	//mtd->erasesize = mtd->rl_erasesize - (mtd->freesize * (mtd->rl_erasesize / mtd->rl_writesize));
	//mtd->size = ((mtd->rl_size / mtd->rl_erasesize) * mtd->erasesize);
	mtd->erasesize = mtd->rl_erasesize - (mtd->freesize * chip->ppb);
	mtd->size = mtd->rl_size - ((1 << (chip->chip_shift - chip->page_shift)) * mtd->freesize);
	/* Set the internal oob buffer location, just after the page data */
	chip->oob_poi = chip->buffers->databuf + mtd->writesize;

	if (!chip->write_page)
		chip->write_page = nand_write_page;


	/*
	 * check ECC mode, default to software if 3byte/512byte hardware ECC is
	 * selected and we have 256 byte pagesize fallback to software ECC
	 */

	switch (chip->ecc.mode) {
	case NAND_ECC_HW:
		/* Use standard hwecc read page function ? */
		if (!chip->ecc.read_page) {
#ifndef CONFIG_MTD_HW_RS_ECC
			chip->ecc.read_page = nand_read_page_hwecc;
#else
			chip->ecc.read_page = nand_read_page_hwecc_rs;
#endif
		}
		if (!chip->ecc.write_page)
			chip->ecc.write_page = nand_write_page_hwecc;
		if (!chip->ecc.read_page_raw)
			chip->ecc.read_page_raw = nand_read_page_raw;
		if (!chip->ecc.write_page_raw)
			chip->ecc.write_page_raw = nand_write_page_raw;
		if (!chip->ecc.read_oob)
			chip->ecc.read_oob = nand_read_oob_std;
		if (!chip->ecc.write_oob)
			chip->ecc.write_oob = nand_write_oob_std;

	case NAND_ECC_HW_SYNDROME:
		if ((!chip->ecc.calculate || !chip->ecc.correct ||
		     !chip->ecc.hwctl) &&
		    (!chip->ecc.read_page ||
		     chip->ecc.read_page == nand_read_page_hwecc ||
		     !chip->ecc.write_page ||
		     chip->ecc.write_page == nand_write_page_hwecc)) {
			printk(KERN_WARNING "No ECC functions supplied, "
			       "Hardware ECC not possible\n");
			BUG();
		}
		/* Use standard syndrome read/write page function ? */
		if (!chip->ecc.read_page)
			chip->ecc.read_page = nand_read_page_syndrome;
		if (!chip->ecc.write_page)
			chip->ecc.write_page = nand_write_page_syndrome;
		if (!chip->ecc.read_page_raw)
			chip->ecc.read_page_raw = nand_read_page_raw_syndrome;
		if (!chip->ecc.write_page_raw)
			chip->ecc.write_page_raw = nand_write_page_raw_syndrome;
		if (!chip->ecc.read_oob)
			chip->ecc.read_oob = nand_read_oob_syndrome;
		if (!chip->ecc.write_oob)
			chip->ecc.write_oob = nand_write_oob_syndrome;

		if (mtd->rl_writesize >= chip->ecc.size)
			break;
		printk(KERN_WARNING "%d byte HW ECC not possible on "
		       "%d byte page size, fallback to SW ECC\n",
		       chip->ecc.size, mtd->rl_writesize);
		chip->ecc.mode = NAND_ECC_SOFT;

	case NAND_ECC_SOFT:
		chip->ecc.calculate = nand_calculate_ecc;
		chip->ecc.correct = nand_correct_data;
		chip->ecc.read_page = nand_read_page_swecc;
		chip->ecc.read_subpage = nand_read_subpage;
		chip->ecc.write_page = nand_write_page_swecc;
		chip->ecc.read_page_raw = nand_read_page_raw;
		chip->ecc.write_page_raw = nand_write_page_raw;
		chip->ecc.read_oob = nand_read_oob_std;
		chip->ecc.write_oob = nand_write_oob_std;
		chip->ecc.size = 256;
		chip->ecc.bytes = 3;
		break;

	case NAND_ECC_NONE:
		printk(KERN_WARNING "NAND_ECC_NONE selected by board driver. "
		       "This is not recommended !!\n");
		chip->ecc.read_page = nand_read_page_raw;
		chip->ecc.write_page = nand_write_page_raw;
		chip->ecc.read_oob = nand_read_oob_std;
		chip->ecc.read_page_raw = nand_read_page_raw;
		chip->ecc.write_page_raw = nand_write_page_raw;
		chip->ecc.write_oob = nand_write_oob_std;
		chip->ecc.size = mtd->rl_writesize;
		chip->ecc.bytes = 0;
		break;

	default:
		printk(KERN_WARNING "Invalid NAND_ECC_MODE %d\n",
		       chip->ecc.mode);
		BUG();
	}

	/*
	 * The number of bytes available for a client to place data into
	 * the out of band area
	 */
	chip->ecc.layout->oobavail = 0;
	for (i = 0; chip->ecc.layout->oobfree[i].length
			&& i < ARRAY_SIZE(chip->ecc.layout->oobfree); i++)
		chip->ecc.layout->oobavail +=
			chip->ecc.layout->oobfree[i].length;
	mtd->oobavail = chip->ecc.layout->oobavail;

	/*
	 * Set the number of read / write steps for one page depending on ECC
	 * mode
	 */
#if !defined(CONFIG_SOC_JZ4760B) && !defined(CONFIG_SOC_JZ4770) && !defined(CONFIG_SOC_JZ4780)
	chip->ecc.steps = mtd->rl_writesize / chip->ecc.size;
	if(chip->ecc.steps * chip->ecc.size != mtd->rl_writesize) {
#else
	chip->ecc.steps = mtd->writesize / chip->ecc.size;
	if(chip->ecc.steps * chip->ecc.size != mtd->writesize) {
#endif
		printk(KERN_WARNING "Invalid ecc parameters\n");
		BUG();
	}
	chip->ecc.total = chip->ecc.steps * chip->ecc.bytes;

	/*
	 * Allow subpage writes up to ecc.steps. Not possible for MLC
	 * FLASH.
	 */
	if (!(chip->options & NAND_NO_SUBPAGE_WRITE) &&
	    !(chip->cellinfo & NAND_CI_CELLTYPE_MSK)) {
		switch(chip->ecc.steps) {
		case 2:
			mtd->subpage_sft = 1;
			break;
		case 4:
		case 8:
		case 16:
			mtd->subpage_sft = 2;
			break;
		}
	}
	chip->subpagesize = mtd->rl_writesize >> mtd->subpage_sft;

	/* Initialize state */
	chip->state = FL_READY;

	/* De-select the device */
	chip->select_chip(mtd, -1);

	/* Invalidate the pagebuffer reference */
	chip->pagebuf = -1;

	/* Fill in remaining MTD driver data */
	mtd->type = MTD_NANDFLASH;
	mtd->flags = MTD_CAP_NANDFLASH;
	mtd->erase = nand_erase;
	mtd->point = NULL;
	mtd->unpoint = NULL;
	mtd->read = nand_read;
	mtd->write = nand_write;
	mtd->read_oob = nand_read_oob;
	mtd->write_oob = nand_write_oob;
	mtd->sync = nand_sync;
	mtd->lock = NULL;
	mtd->unlock = NULL;
	mtd->suspend = NULL;
	mtd->resume = NULL;
	mtd->block_isbad = nand_block_isbad;
	mtd->block_markbad = nand_block_markbad;

	/* propagate ecc.layout to mtd_info */
	mtd->ecclayout = chip->ecc.layout;

	/* Check, if we should skip the bad block table scan */
	if (chip->options & NAND_SKIP_BBTSCAN)
		return 0;

#if defined(CONFIG_SOC_JZ4760B) && defined(CONFIG_MTD_NAND_DMA)
	jz4760b_nand_dma_init(mtd);
#endif
#if defined(CONFIG_SOC_JZ4770) && defined(CONFIG_MTD_NAND_DMA)
	jz4770_nand_dma_init(mtd);
#endif

        /* Create jz_mtd1 for one plane operation if the NAND support multiple
	   planes operation, because some partitions will only use one plane. */
	if ((chip->planenum == 2) && !all_use_planes) {
		int i, len, numblocks;
		struct nand_chip *this = (struct nand_chip *) (&jz_mtd1[1]);
	
		memcpy(jz_mtd1, mtd, sizeof(*mtd));
		jz_mtd1->priv = this;
		memcpy(this, chip, sizeof(*chip));

		this->planenum = 1;
		jz_mtd1->rl_writesize >>= 1;
		jz_mtd1->writesize >>= 1;
		jz_mtd1->oobsize >>= 1;
		jz_mtd1->rl_erasesize >>= 1;
		jz_mtd1->erasesize >>= 1;

		this->page_shift = chip->page_shift - 1;
		this->pagemask = (this->chipsize >> this->page_shift) - 1;
		this->bbt_erase_shift = this->phys_erase_shift =
			chip->phys_erase_shift - 1;

		this->ecc.steps >>= 1;
		this->ecc.total = this->ecc.steps * this->ecc.bytes;

		this->subpagesize = jz_mtd1->rl_writesize;

		this->erase_cmd = single_erase_cmd;
#if defined(CONFIG_MTD_HW_RS_ECC)
		this->ecc.read_page = nand_read_page_hwecc_rs;
		this->ecc.write_page = nand_write_page_hwecc;
#endif
		this->ecc.read_oob = nand_read_oob_std;
		this->ecc.write_oob = nand_write_oob_std;
		this->write_buf = nand_write_buf;
		this->read_buf = nand_read_buf;

		/* Firstly, build bad block table as one plane */
		res = this->scan_bbt(jz_mtd1);

		/* Secondly, build bad block table as 2 plane based on bbt of jz_mtd1 */

		numblocks =  chip->chipsize >> (chip->bbt_erase_shift - 1);   /* = (real numblocks * 2) */
		len = mtd->rl_size >> (chip->bbt_erase_shift + 2);

		chip->bbt = kzalloc(len, GFP_KERNEL);

#define isbad_2plane(block)  (((this->bbt[(block) >> 3] >> ((block) & 0x06)) 	\
			      | (this->bbt[((block)+2) >> 3] >> (((block)+2) & 0x06))) & 0x03)

		for (i = 0; i < numblocks; i += 2) {
			if (isbad_2plane(2*i))
				chip->bbt[i >> 3] |= 0x03 << (i & 0x6);
		}
	} else {
		res = chip->scan_bbt(mtd);
	}

#if defined(CONFIG_ALLOCATE_MTDBLOCK_JZ_EARLY) && !defined(CONFIG_SOC_JZ4730)
        /* Allocate a block cache for every partitions which works over mtdblock-jz */
	{
		printk("CONFIG_ALLOCATE_MTDBLOCK_JZ_EARLY.\n");
		extern int nr_partitions;

		jz_mtdblock_cache = kzalloc(nr_partitions * sizeof(unsigned char *), GFP_KERNEL);

		for (i = 0; i < nr_partitions; i++) {
			if (!(partition_info[i].mtdblock_jz_invalid) && !(partition_info[i].cpu_mode)) {
				if (partition_info[i].use_planes) {
					jz_mtdblock_cache[i] = kmalloc(mtd->erasesize, GFP_KERNEL);
					printk("Allocate 0x%x bytes for jz_mtdblock%d at address:0x%p.\n", mtd->erasesize, i, jz_mtdblock_cache[i]);
				} else {
					jz_mtdblock_cache[i] = kmalloc(jz_mtd1->erasesize, GFP_KERNEL);
					printk("Allocate 0x%x bytes for jz_mtdblock%d at address:0x%p.\n", jz_mtd1->erasesize, i, jz_mtdblock_cache[i]);
				}
				if (!jz_mtdblock_cache[i])
					printk("Cannot allocate memory for jz_mtdblock%d!\n", i);
			}
		}
	}
#endif

//#ifdef CONFIG_MTD_DEBUG
#if 1
	dump_nand_and_mtd_info(mtd);
#endif
	return res;
}

/* is_module_text_address() isn't exported, and it's mostly a pointless
   test if this is a module _anyway_ -- they'd have to try _really_ hard
   to call us from in-kernel code if the core NAND support is modular. */
#ifdef MODULE
#define caller_is_module() (1)
#else
#define caller_is_module() \
	is_module_text_address((unsigned long)__builtin_return_address(0))
#endif

/**
 * nand_scan - [NAND Interface] Scan for the NAND device
 * @mtd:	MTD device structure
 * @maxchips:	Number of chips to scan for
 *
 * This fills out all the uninitialized function pointers
 * with the defaults.
 * The flash ID is read and the mtd/chip structures are
 * filled with the appropriate values.
 * The mtd->owner field must be set to the module of the caller
 *
 */
int nand_scan(struct mtd_info *mtd, int maxchips)
{
	int ret;

	/* Many callers got this wrong, so check for it for a while... */
	if (!mtd->owner && caller_is_module()) {
		printk(KERN_CRIT "nand_scan() called with NULL mtd->owner!\n");
		BUG();
	}

	ret = nand_scan_ident(mtd, maxchips);
	if (!ret)
		ret = nand_scan_tail(mtd);
	return ret;
}

/**
 * nand_release - [NAND Interface] Free resources held by the NAND device
 * @mtd:	MTD device structure
*/
void nand_release(struct mtd_info *mtd)
{
	struct nand_chip *chip = mtd->priv;

#ifdef CONFIG_MTD_PARTITIONS
	/* Deregister partitions */
	del_mtd_partitions(mtd);
#endif
	/* Deregister the device */
	del_mtd_device(mtd);

	/* Free bad block table memory */
	kfree(chip->bbt);
	if (!(chip->options & NAND_OWN_BUFFERS))
		kfree(chip->buffers);
}

EXPORT_SYMBOL_GPL(nand_scan);
EXPORT_SYMBOL_GPL(nand_scan_ident);
EXPORT_SYMBOL_GPL(nand_scan_tail);
EXPORT_SYMBOL_GPL(nand_release);

static int __init nand_base_init(void)
{
	led_trigger_register_simple("nand-disk", &nand_led_trigger);
	return 0;
}

static void __exit nand_base_exit(void)
{
	led_trigger_unregister_simple(nand_led_trigger);
}

module_init(nand_base_init);
module_exit(nand_base_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Steven J. Hill <sjhill@realitydiluted.com>, Thomas Gleixner <tglx@linutronix.de>");
MODULE_DESCRIPTION("Generic NAND flash driver code");
