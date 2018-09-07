/*
 *  linux/drivers/mmc/host/jz_mmc/dma/jz_mmc_dma.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * Copyright (c) Ingenic Semiconductor Co., Ltd.
 */


#include <linux/dma-mapping.h>
#include <linux/scatterlist.h>
#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/random.h>
#include <asm/jzsoc.h>
#include "include/jz_mmc_dma.h"
#include "include/jz_mmc_host.h"

#define JZMMC_BUFFER_NEEDS_BOUNCE(buffer)  (((unsigned long)(buffer) & 0x3) || !virt_addr_valid((buffer)))

extern void jz_mmc_dump_regs(int msc_id, int line) ; 

void jz_mmc_stop_dma(struct jz_mmc_host *host)
{
	u32 old_counter = REG_MSC_DMALEN(host->pdev_id);
	u32 cur_counter;

	//WARN(1, "mmc%d called %s\n", host->pdev_id, __FUNCTION__);
	/* wait for the counter not change */
	while (1) {		     /* wait forever, even when the card is removed */
		schedule_timeout(3); /* 30ms */
		cur_counter = REG_MSC_DMALEN(host->pdev_id);
		if (cur_counter == old_counter)
			break;
		old_counter = cur_counter;
	}

	/*disable dma function*/
	REG_MSC_DMAC(host->pdev_id) = 0;
	REG_MSC_DMANDA(host->pdev_id) = 0;
	REG_MSC_DMADA(host->pdev_id) = 0;
	REG_MSC_DMALEN(host->pdev_id) = 0;
	REG_MSC_DMACMD(host->pdev_id) = 0;
}

static void dump_msc_desc(struct jz4780_msc_dma_desc *desc, int line)
{
	printk("--------------line:%d-----------------\n",line);
	printk("next_desc:%x\n",desc->next_desc);
	printk("data_addr:%x\n",desc->data_addr);
	printk("len      :%d\n",desc->len);
	printk("cmd      :%x\n",desc->cmd);
}

void jz_set_align(struct jz_mmc_host *host, unsigned int paddr)
{
	REG_MSC_DMAC(host->pdev_id) &= ~(MSC_DMAC_AOFST_MASK);
	switch (paddr & 0x3) {
		case 0:
			REG_MSC_DMAC(host->pdev_id) &= ~(MSC_DMAC_ALIGNED);
			break;
		case 1:
			REG_MSC_DMAC(host->pdev_id) |= (MSC_DMAC_ALIGNED);
			REG_MSC_DMAC(host->pdev_id) |= (MSC_DMAC_AOFST_01);
			break;
		case 2:
			REG_MSC_DMAC(host->pdev_id) |= (MSC_DMAC_ALIGNED);
			REG_MSC_DMAC(host->pdev_id) |= (MSC_DMAC_AOFST_10);
			break;

		case 3:
			REG_MSC_DMAC(host->pdev_id) |= (MSC_DMAC_ALIGNED);
			REG_MSC_DMAC(host->pdev_id) |= (MSC_DMAC_AOFST_11);	
			break;
	}
}

static int sg_to_desc(struct scatterlist *sgentry, JZ_MSC_DMA_DESC *first_desc,
		      int *desc_pos /* IN OUT */, int mode, int ctrl_id,
		      struct jz_mmc_host *host) 
{
	struct jz4780_msc_dma_desc*desc = NULL;
	int pos = *desc_pos;
	unsigned int next;
	unsigned int dma_len;
	unsigned int blklen = REG_MSC_BLKLEN(host->pdev_id);	
	dma_addr_t dma_addr = 0;

	dma_addr_t dma_desc_phys_addr = CPHYSADDR((unsigned long)first_desc);

	/* if the start address is not aligned to word */
	if (sg_phys(sgentry) & 0x3) {
		printk("MSC SDMA address not aligned. phy addr = 0x%08x\n", (unsigned int)sg_phys(sgentry));
		jz_set_align(host, (unsigned int)sg_phys(sgentry));
	}	

	dma_len = sg_dma_len(sgentry);

	dma_addr = sg_phys(sgentry);

	if ((dma_addr & 0x3) && (dma_len > blklen)) {	
		int times = dma_len / blklen;	/* ??? */
		int i = pos;

		while (pos < (i + times)) {
			desc = first_desc + pos;

			desc->cmd = pos << MSC_DMACMD_ID_BIT;
			desc->cmd |=  DMAC_DCMD_LINK;

			desc->data_addr = dma_addr + blklen * pos;

			next = (dma_desc_phys_addr + (pos + 1) * (sizeof(struct jz4780_msc_dma_desc)));	
			desc->next_desc = next;

			desc->len = blklen;

			pos ++;
		}

		if (dma_len % blklen) {
			desc = first_desc + pos;

			desc->cmd = pos << MSC_DMACMD_ID_BIT;
			desc->cmd |=  DMAC_DCMD_LINK;

			desc->data_addr = dma_addr + blklen * pos;

			next = (dma_desc_phys_addr + (pos + 1) * (sizeof(struct jz4780_msc_dma_desc)));	
			desc->next_desc = next;

			desc->len = dma_len % blklen;

			pos ++;
		}
	} else {
		desc = first_desc + pos;

		desc->cmd = pos << MSC_DMACMD_ID_BIT;
		desc->cmd |=  DMAC_DCMD_LINK;

		desc->data_addr = dma_addr;

		next = (dma_desc_phys_addr + (pos + 1) * (sizeof(struct jz4780_msc_dma_desc)));	
		desc->next_desc = next;

		desc->len = dma_len;

		pos ++;
	}

	*desc_pos = pos;
	return 0;
}

int jz_mmc_start_scatter_dma(struct jz_mmc_host *host, struct scatterlist *sg, unsigned int sg_len, int mode) {
	int i = 0;
	int desc_pos = 0;
	dma_addr_t dma_desc_phy_addr = 0;
	struct mmc_data *data = host->curr_mrq->data;
	struct scatterlist *sgentry;
	struct jz4780_msc_dma_desc*desc;
	struct jz4780_msc_dma_desc*desc_first;
	unsigned long flags;
	unsigned long start_time = jiffies;
	int ret = 0;

	while (REG_MSC_STAT(host->pdev_id) & MSC_STAT_DMAEND) {
		if (jiffies - start_time > 10) { /* 100ms */
			printk("MSC(%d) DMA unavailable! REG_MSC_STAT = 0x%08x\n", host->pdev_id, REG_MSC_STAT(host->pdev_id));
			jz_mmc_stop_dma(host);
			break;
		}
	}
	desc = host->dma_desc;
	desc_first = desc;

	dma_desc_phy_addr  = CPHYSADDR((unsigned long)desc);

	memset(desc, 0, PAGE_SIZE);

	desc_pos = 0;
	flags = claim_dma_lock();
	//REG_MSC_DMAC(host->pdev_id) &= ~(MSC_DMAC_INCR_MASK);  /* 16 burst */
	//REG_MSC_DMAC(host->pdev_id) |= MSC_DMAC_INCR32;        /* 32 burst */
	REG_MSC_DMAC(host->pdev_id) |= MSC_DMAC_INCR64;          /* 64 burst */
	REG_MSC_DMAC(host->pdev_id) &= ~(MSC_DMAC_AOFST_MASK);
	REG_MSC_DMAC(host->pdev_id) &= ~(MSC_DMAC_ALIGNED);

	for_each_sg(data->sg, sgentry, host->dma.len, i) {
		ret = sg_to_desc(sgentry, desc, &desc_pos, mode, host->pdev_id, host);
		if (ret < 0)
			goto out;
	}

	desc = desc + (desc_pos - 1);
	desc->cmd |= MSC_DMACMD_ENDI;                   /*enable dmaend interrupt*/
	desc->cmd &= ~MSC_DMACMD_LINK;                  /*the last one unlink*/

	dma_cache_wback_inv((unsigned long)desc_first, desc_pos * sizeof(struct jz4780_msc_dma_desc));
	//printk("====> DMAC = %d, should 64burst, [3,2] = 2'b10\n", REG_MSC_DMAC(host->pdev_id));

	REG_MSC_DMANDA(host->pdev_id) = dma_desc_phy_addr;

out:
	release_dma_lock(flags);
	return ret;
}

#ifdef CONFIG_HIGHMEM
static void jz_mmc_highmem_dma_map_sg(struct scatterlist *sgl, unsigned int nents, int is_write)
{
	struct sg_mapping_iter miter;
	unsigned long flags;
	unsigned int sg_flags = SG_MITER_ATOMIC;

	if (is_write)
		sg_flags |= SG_MITER_FROM_SG;
	else
		sg_flags |= SG_MITER_TO_SG;

	sg_miter_start(&miter, sgl, nents, sg_flags);

	local_irq_save(flags);

	while (sg_miter_next(&miter)) {
		if (is_write)
			dma_cache_wback_inv((unsigned long)miter.addr, miter.length);
		else
			dma_cache_inv((unsigned long)miter.addr, miter.length);
	}

	sg_miter_stop(&miter);
	local_irq_restore(flags);
}
#endif

void jz_mmc_enable_sdma(struct jz_mmc_host *host){
	/*enable dma function*/
	REG_MSC_DMAC(host->pdev_id) |= (MSC_DMAC_EN);
}

int jz_mmc_start_sdma(struct jz_mmc_host *host) 
{
	struct mmc_data *data = host->curr_mrq->data;
	int mode;
	int ret = 0;

	host->transfer_mode = JZ_TRANS_MODE_DMA;

	if (data->flags & MMC_DATA_WRITE) {
		mode = DMA_MODE_WRITE;
		host->dma.dir = DMA_TO_DEVICE;
	} else {
		mode = DMA_MODE_READ;
		host->dma.dir = DMA_FROM_DEVICE;
	}

#ifndef CONFIG_HIGHMEM
	host->dma.len =
		dma_map_sg(mmc_dev(host->mmc), data->sg, data->sg_len,
				host->dma.dir);
#else
	jz_mmc_highmem_dma_map_sg(data->sg, data->sg_len, (mode == DMA_MODE_WRITE));
	host->dma.len = data->sg_len;
#endif

	ret = jz_mmc_start_scatter_dma(host, data->sg, host->dma.len, mode);

	if (ret < 0) {
		printk("---> unmap sg\n ");
		dma_unmap_sg(mmc_dev(host->mmc), data->sg, host->dma.len,
				host->dma.dir);
		host->transfer_mode = JZ_TRANS_MODE_NULL;
	}

	jz_mmc_enable_sdma(host);

	return ret;
}

static int jz_mmc_init_dma(struct jz_mmc_host *host)
{
	if (host->dma_id < 0)
		return 0;     /* not use dma */

	host->dma_desc = (struct jz4780_msc_dma_desc*)__get_free_pages(GFP_KERNEL, 0);

	/*select dma mode*/
	REG_MSC_DMAC(host->pdev_id) &= ~(MSC_DMAC_SEL_COMMON);  

	return 0;
}

static void jz_mmc_deinit_dma(struct jz_mmc_host *host)
{
	jz_free_dma(host->dma.channel);
}

int jz_mmc_dma_register(struct jz_mmc_dma *dma)
{
	if(dma == NULL)
		return -ENOMEM;

	dma->init = jz_mmc_init_dma;
	dma->deinit = jz_mmc_deinit_dma;

	return 0;
}
