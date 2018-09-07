
/*
 * I2C adapter for the INGENIC I2C bus access.
 *
 * Copyright (C) 2006 - 2009 Ingenic Semiconductor Inc.
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/i2c-id.h>
#include <linux/init.h>
#include <linux/time.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/platform_device.h>
#include <linux/kthread.h>

#include <asm/irq.h>
#include <asm/io.h>
#include <linux/module.h>
#include <asm/addrspace.h>

#include <asm/jzsoc.h>
#include "i2c-jz4775.h"

/* I2C protocol */
#define I2C_READ_CMD	(1 << 8)
#define I2C_WRITE_CMD	(0 << 8)
#define TIMEOUT         0xffff
#define I2C_CLIENT_NUM  20

//#define DEBUG
#ifdef DEBUG
#define dprintk(x...)	printk(x)
#else
#define dprintk(x...) do{}while(0)
#endif

#define DMA_ID_I2C3_RX    10
#define DMA_ID_I2C3_TX	  11
#define DMA_ID_I2C4_RX	  12
#define DMA_ID_I2C4_TX	  13
#define DANGERS_WAIT_ON(cond, nsec)					\
	({								\
		 unsigned long m_start_time = jiffies;			\
		 int __nsec_i = 0;					\
									 \
		 while (cond) {						\
			 if (time_after(jiffies, m_start_time + HZ)) {	\
			 printk("WARNING: %s:%d condition never becomes to zero\n", __func__, __LINE__); \
			 __nsec_i ++;				\
			 if (__nsec_i >= (int)((nsec)))		\
				 break;				\
			 m_start_time = jiffies;			\
		 	}						\
		 }							\
		 (__nsec_i == (int)((nsec)));				\
	 })

static int dma_write_by_cpu = 0;
static int is_transmit = 0;
static int is_receive = 0;
jz_dma_desc_8word *read_desc = NULL;
#if 1
struct jz_i2c_dma_info {
	int chan;
	volatile atomic_t is_waiting;
	//struct completion comp;
	int i2c_id;
	int dma_id;
	unsigned int dma_req;
	char name[12];
	int use_dma;
};

/* .dma_req: need check DMA spec */
static struct jz_i2c_dma_info rx_dma_info[] = {
	{
		.chan = -1,
		.i2c_id = 0,
		.dma_id = DMA_ID_I2C0_RX,
		.dma_req = 0x25,
		.name = "i2c0 read",
#ifdef CONFIG_JZ_I2C0_USE_DMA
		.use_dma = 1,
#else
		.use_dma = 0,
#endif
	},
	{
		.chan = -1,
		.i2c_id = 1,
		.dma_id = DMA_ID_I2C1_RX,
		.dma_req = 0x27,
		.name = "i2c1 read",
#ifdef CONFIG_JZ_I2C1_USE_DMA
		.use_dma = 1,
#else
		.use_dma = 0,
#endif
	},
	{
		.chan = -1,
		.i2c_id = 2,
		.dma_id = DMA_ID_I2C2_RX,
		.dma_req = 0x29,
		.name = "i2c2 read",
#ifdef CONFIG_JZ_I2C2_USE_DMA
		.use_dma = 1,
#else
		.use_dma = 0,
#endif
	},

};

static struct jz_i2c_dma_info tx_dma_info[] = {
	{
		.chan = -1,
		.i2c_id = 0,
		.dma_id = DMA_ID_I2C0_TX,
		.dma_req = 0x24,
		.name = "i2c0 write",
	},
	{
		.chan = -1,
		.i2c_id = 1,
		.dma_id = DMA_ID_I2C1_TX,
		.dma_req = 0x26,
		.name = "i2c1 write",
	},
	{
		.chan = -1,
		.i2c_id = 2,
		.dma_id = DMA_ID_I2C2_TX,
		.dma_req = 0x28,
		.name = "i2c2 write",
	},
};
#endif

struct jz_i2c {
	int                     id;
	unsigned int            irq;
	struct i2c_adapter	adap;
	int (*write)(unsigned char device, unsigned char *buf,
			int length, struct jz_i2c *i2c, int restart);
	int (*read)(unsigned char device, unsigned char *buf,
			int length, struct jz_i2c *i2c, int restart);
	int (*read_offset)(unsigned char device,
			unsigned char *offset, int off_len,
			unsigned char *buf, int read_len,
			struct jz_i2c *i2c);
};

#define PRINT_REG_WITH_ID(reg_name, id) \
	printk("" #reg_name "(%d) = 0x%08x\n", id, reg_name(id))

#define DEBUG
#ifdef DEBUG
static void jz_dump_i2c_regs(int i2c_id, int line) {
	printk("***** i2c%d regs, line = %d *****\n", i2c_id, line);
	PRINT_REG_WITH_ID(REG_I2C_CTRL, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_TAR, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_SAR, i2c_id);
	//PRINT_REG_WITH_ID(REG_I2C_DC, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_SHCNT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_SLCNT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_FHCNT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_FLCNT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_INTST, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_INTM, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_RXTL, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_TXTL, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CINTR, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CRXUF, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CRXOF, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CTXOF, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CRXREQ, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CTXABRT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CRXDONE, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CACT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CSTP, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CSTT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_CGC, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_ENB, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_STA, i2c_id);
	//PRINT_REG_WITH_ID(REG_I2C_TXFLR, i2c_id);
	//PRINT_REG_WITH_ID(REG_I2C_RXFLR, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_TXABRT, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_DMACR, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_DMATDLR, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_DMARDLR, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_SDASU, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_ACKGC, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_ENSTA, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_SDAHD, i2c_id);
	PRINT_REG_WITH_ID(REG_I2C_FLT, i2c_id);
}
#else  /* !DEBUG */
static void jz_dump_i2c_regs(int i2c_id, int line) {
}
#endif	/* DEBUG */

struct i2c_speed {
	unsigned int speed;
	unsigned char slave_addr;
};
static  struct i2c_speed jz4760_i2c_speed[I2C_CLIENT_NUM];
static unsigned char current_device[6] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0};
static int client_cnt = 0;

void i2c_jz_setclk(struct i2c_client *client,unsigned long i2cclk)
{
	if (i2cclk > 0 && i2cclk <= 400000) {
		jz4760_i2c_speed[client_cnt].slave_addr = client->addr;
		jz4760_i2c_speed[client_cnt].speed      = i2cclk / 1000;
	} else if (i2cclk <= 0) {
		jz4760_i2c_speed[client_cnt].slave_addr = client->addr;
		jz4760_i2c_speed[client_cnt].speed      = 100;
	} else {
		jz4760_i2c_speed[client_cnt].slave_addr = client->addr;
		jz4760_i2c_speed[client_cnt].speed      = 400;
	}

	client_cnt++;
}
EXPORT_SYMBOL_GPL(i2c_jz_setclk);

static int i2c_disable(int i2c_id)
{
	int timeout = TIMEOUT;

	__i2c_disable(i2c_id);
	while(__i2c_is_enable(i2c_id) && (timeout > 0)) {
		udelay(1);
		timeout--;
	}
	if(timeout)
		return 0;
	else
		return 1;
}
static int i2c_enable(int i2c_id)
{
	int timeout = TIMEOUT;
	__i2c_enable(i2c_id);
	while((!__i2c_is_enable(i2c_id)) && (timeout > 0)){
		udelay(1);
		timeout--;
	}
	if(timeout)
		return 0;
	else
		return 1;

}

static int i2c_set_clk(int i2c_clk, int i2c_id)
{
	int dev_clk_khz = cpm_get_clock(CGU_PCLK) / 1000;
	//int dev_clk_khz = 24000;
	int cnt_high = 0;       /* HIGH period count of the SCL clock */
	int cnt_low = 0;        /* LOW period count of the SCL clock */
	int cnt_period = 0;     /* period count of the SCL clock */
	int setup_time = 0;
	int hold_time = 0;

	if (i2c_clk <= 0 || i2c_clk > 400)
		goto set_clk_err;

	/* 1 I2C cycle equals to cnt_period PCLK(i2c_clk) */
	cnt_period = dev_clk_khz / i2c_clk;
	if (i2c_clk <= 100) {
		/* i2c standard mode, the min LOW and HIGH period are 4700 ns and 4000 ns */
		cnt_high = (cnt_period * 4000) / (4700 + 4000);
	} else {
		/* i2c fast mode, the min LOW and HIGH period are 1300 ns and 600 ns */
		cnt_high = (cnt_period * 600) / (1300 + 600);
	}

	cnt_low = cnt_period - cnt_high;

//	printk("dev_clk = %d, i2c_clk = %d cnt_period = %d, cnt_high = %d, cnt_low = %d, \n", dev_clk_khz, i2c_clk, cnt_period, cnt_high, cnt_low);

	if (i2c_clk <= 100) {
		REG_I2C_CTRL(i2c_id) = 0x63;      /* standard speed mode*/
		REG_I2C_SHCNT(i2c_id) = I2CSHCNT_ADJUST(cnt_high);
		REG_I2C_SLCNT(i2c_id) = I2CSLCNT_ADJUST(cnt_low);
 	} else {
		REG_I2C_CTRL(i2c_id) = 0x65;       /* high speed mode*/
		REG_I2C_FHCNT(i2c_id) = I2CFHCNT_ADJUST(cnt_high);
		REG_I2C_FLCNT(i2c_id) = I2CFLCNT_ADJUST(cnt_low);
	}

	/*
	 * a i2c device must internally provide a hold time at least 300ns
	 * tHD:DAT
	 *	Standard Mode: min=300ns, max=3450ns
	 *	Fast Mode: min=0ns, max=900ns
	 * tSU:DAT
	 *	Standard Mode: min=250ns, max=infinite
	 *	Fast Mode: min=100(250ns is recommanded), max=infinite
	 *
	 * 1i2c_clk = 10^6 / dev_clk_khz
	 * on FPGA, dev_clk_khz = 12000, so 1i2c_clk = 1000/12 = 83ns
	 * on Pisces(1008M), dev_clk_khz=126000, so 1i2c_clk = 1000 / 126 = 8ns
	 *
	 * The actual hold time is (SDAHD + 1) * (i2c_clk period).
	 *
	 * The length of setup time is calculated using (SDASU - 1) * (ic_clk_period)
	 *
	 */

	if (i2c_clk <= 100) { /* standard mode */
		setup_time = 300;
		hold_time = 400;
	} else {
		setup_time = 300;
		hold_time = 0;
	}

	hold_time = (hold_time / (1000000 / dev_clk_khz) - 1);
	setup_time = (setup_time / (1000000 / dev_clk_khz) + 1);

	if (setup_time > 255)
		setup_time = 255;
	if (setup_time <= 0)
		setup_time = 1;

	if (hold_time > 255)
		hold_time = 255;
	if(hold_time <= 0)
		hold_time = 1;

	__i2c_set_hold_time(i2c_id, hold_time);
	__i2c_set_setup_time(i2c_id, setup_time);

//	printk("tSU:DAT = %d tHD:DAT = %d\n",
//		REG_I2C_SDASU(i2c_id) & 0xff, REG_I2C_SDAHD(i2c_id) & 0xff);
	return 0;

set_clk_err:
	printk("i2c set sclk faild,i2c_clk=%d KHz,dev_clk=%dKHz.\n", i2c_clk, dev_clk_khz);
	return -1;
}

static int i2c_set_target(unsigned char address,int i2c_id)
{

//	printk("=====> enter %s\n", __func__);
	int val;
	/* wait 2s for T FIFO is empty and i2c master in idel mode */
	int res = DANGERS_WAIT_ON((!__i2c_txfifo_is_empty(i2c_id) || __i2c_master_active(i2c_id)), 2);
	if (res) {
		printk("WARNING: i2c%d failed to set slave address!\n", i2c_id);
		return -ETIMEDOUT;
	}
	val = REG_I2C_TAR(i2c_id);
	val &= ~(0x3ff << 0);
	val |= address;
	REG_I2C_TAR(i2c_id) = val;  /* slave id needed write only once */
	return 0;
}

static int i2c_init_as_master(int i2c_id,unsigned char device)
{

	int i, ret;
	volatile unsigned short value = 0;
	unsigned int speed = 20; /* default to 100k */

//	printk("======>enter   %s\n", __func__);
	if(i2c_disable(i2c_id)) {
		printk("i2c not disable, check if any transfer pending!\n");
		return -ETIMEDOUT;
	}

	for (i = 0; i < I2C_CLIENT_NUM; i++) {
		if(device == jz4760_i2c_speed[i].slave_addr) {
			speed = jz4760_i2c_speed[i].speed;
			break;
		}
	}
	i2c_set_clk(speed,i2c_id);

	REG_I2C_TAR(i2c_id) &= ~(1 << 12); //7 bit mode 
	REG_I2C_INTM(i2c_id) = 0x0; /*mask all interrupt*/
	REG_I2C_TXTL(i2c_id) = 0xff;
	REG_I2C_RXTL(i2c_id) = 0;

	ret = i2c_set_target(device, i2c_id);
	if (ret)
		return ret;
	if(i2c_enable(i2c_id)){
		printk("I2c not enable...\n");
		return -ETIMEDOUT;
	}

#if 0
#if 0
	/* Test no restart mode when read and write */
	REG_I2C_CTRL(i2c_id) = 0x43;
	if(i2c_enable(i2c_id)){
		printk("I2c not enable...\n");
		return -ETIMEDOUT;
	}
#if 0
	/* read after write and no restart */
	__i2c_write(0xc3 | I2C_WRITE_CMD, i2c_id);
	__i2c_write(I2C_READ_CMD | I2C_DC_STP, i2c_id);
#else
	/*write after read and no restart */
	__i2c_write(I2C_READ_CMD, i2c_id);
	__i2c_write((0xc3 | I2C_WRITE_CMD | I2C_DC_STP), i2c_id);
#endif
#else
	/* Test has restart mode when read and write */
	REG_I2C_CTRL(i2c_id) = 0x63;
	if(i2c_enable(i2c_id)){
		printk("I2c not enable...\n");
		return -ETIMEDOUT;
	}
#if 1
	/* read after write , have  restart */
	__i2c_write(0xc3 | I2C_WRITE_CMD, i2c_id);
	__i2c_write(I2C_READ_CMD | I2C_DC_STP, i2c_id);
#else
	/* write after write, have restart */
	__i2c_write(I2C_READ_CMD, i2c_id);
	__i2c_write((0xc3 | I2C_WRITE_CMD | I2C_DC_STP), i2c_id);

#endif
	while(1);
#endif
#endif
#if 0
	/* Test if we done allow trans stop, whether trans stop when TX fifo is empty */
	if(i2c_enable(i2c_id)){
		printk("I2c not enable...\n");
		return -ETIMEDOUT;
	}
	__i2c_write(0xc3, i2c_id);
#endif
	return 0;
}
#if 1
static irqreturn_t jz_i2c_dma_callback(int irq, void *devid)
{
	struct jz_i2c_dma_info *dma_info = (struct jz_i2c_dma_info *)devid;
	int chan = dma_info->chan;

//	printk("====>enter %s\n", __func__);
	disable_dma(chan);
	if (__dmac_channel_address_error_detected(chan)) {
		printk("%s: DMAC address error.\n",
				
				__FUNCTION__);
		__dmac_channel_clear_address_error(chan);
	}
	if (__dmac_channel_transmit_end_detected(chan)) {
		__dmac_channel_clear_transmit_end(chan);
	}

	if (atomic_read(&dma_info->is_waiting)) {
		atomic_set(&dma_info->is_waiting, 0);
		wmb();
	}

	return IRQ_HANDLED;
}

static int jz_i2c_dma_init_as_write(unsigned short *buf,int length, int bus_id, int need_wait)
{
	int i;
	struct jz_i2c_dma_info *dma_info = tx_dma_info + bus_id;

//	printk("====>enter %s\n", __func__);
	__i2c_set_dma_td_level(bus_id, 32); // half FIFO depth, 64/2
	__i2c_dma_td_enable(bus_id);


	dma_cache_wback((unsigned long)buf, length * 2);
	if (need_wait)
		atomic_set(&dma_info->is_waiting, 1);
	else
		atomic_set(&dma_info->is_waiting, 0);
	/* Init DMA module */
	REG_DMAC_DCCSR(dma_info->chan) = 0;
	REG_DMAC_DRSR(dma_info->chan) = dma_info->dma_req;
	REG_DMAC_DSAR(dma_info->chan) = CPHYSADDR(buf);
	REG_DMAC_DTAR(dma_info->chan) = CPHYSADDR(I2C_DC(bus_id));
	REG_DMAC_DTCR(dma_info->chan) = (length * 2); // length * 2 byte unit
	REG_DMAC_DCMD(dma_info->chan) = DMAC_DCMD_SAI | DMAC_DCMD_SWDH_16 | DMAC_DCMD_DWDH_16 | DMAC_DCMD_DS_AUTO | DMAC_DCMD_TIE;
	__dmac_channel_set_rdil(dma_info->chan, 8); //32 * 16bit = 64 BYTE 
	REG_DMAC_DCCSR(dma_info->chan) = DMAC_DCCSR_NDES | DMAC_DCCSR_EN;
	REG_DMAC_DMACR |= DMAC_DMACR_DMAE; /* global DMA enable bit */
	if (need_wait) {
		int res = DANGERS_WAIT_ON(atomic_read(&dma_info->is_waiting), 2);
		__i2c_dma_td_disable(dma_info->i2c_id);
		if (res) {
			printk("WARNING: i2c%d write error, maybe I2C_CLK or I2C_SDA is pull down to zero by some one!\n", bus_id);
			jz_stop_dma(dma_info->chan);
			return -ETIMEDOUT;
		}
	}

	return 0;
}

static int jz_i2c_dma_init_as_read(unsigned char *buf,int length, int bus_id, int need_wait)
{
	int i = 0, len = length;
	unsigned int div, next, run_time = 0, offset;
	jz_dma_desc_8word *dma_desc = read_desc;
	/* DMA request throhold don't set 64 */
	unsigned char trig_value[] = {0, 0, 0, 0, 0};
	unsigned int burst_len[] = {32, 16, 4, 2, 1};
	unsigned int burst_val[] = {0x4, 0x3, 0x0, 0x2, 0x1};
	struct jz_i2c_dma_info *dma_info = rx_dma_info + bus_id;
//	printk("=====>enter %s\n", __func__);

	__i2c_dma_rd_enable(bus_id);
	memset((void *)buf, 0, length * sizeof(unsigned char));
	if (need_wait)
		atomic_set(&dma_info->is_waiting, 1);
	else
		atomic_set(&dma_info->is_waiting, 0);

	dma_cache_wback_inv((unsigned long)buf, length * sizeof(unsigned char));
	REG_DMAC_DCCSR(dma_info->chan) |= DMAC_DCCSR_DES8;
	REG_DMAC_DCCSR(dma_info->chan) &= ~DMAC_DCCSR_NDES;

	while(len > 0)
	{
		div = len / burst_len[i];
		offset = length - len;
		len = len % burst_len[i];
		if(div == 0){
			i++;
			continue;
		}
		trig_value[i] = (unsigned char)burst_len[i] - 1;
		dma_desc->dsadr = CPHYSADDR(&(trig_value[i]));
		dma_desc->dtadr = CPHYSADDR(I2C_DMARDLR(bus_id));
		dma_desc->dreqt = 0x08;
		dma_desc->dcmd =  DMAC_DCMD_SWDH_8 | DMAC_DCMD_DWDH_8 | DMAC_DCMD_DS_8BIT | DMAC_DCMD_LINK;
		next = CPHYSADDR(((jz_dma_desc_8word *)dma_desc + 1)) >> 4;
		dma_desc->ddadr = (next << 24) | 1;
		run_time++;
		dma_desc++;

		dma_desc->dsadr = CPHYSADDR(I2C_DC(bus_id)); 
		dma_desc->dtadr = CPHYSADDR(((unsigned char *)buf + offset));
		dma_desc->dreqt = dma_info->dma_req;
		dma_desc->dcmd =  DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 | DMAC_DCMD_DWDH_8 | (burst_val[i] << DMAC_DCMD_DS_BIT) | DMAC_DCMD_LINK;
		next = CPHYSADDR(((jz_dma_desc_8word *)dma_desc + 1)) >> 4;
		dma_desc->ddadr = (next << 24) | div;
		run_time++;
		dma_desc++;
	}
	dma_desc--;
	dma_desc->dcmd |= DMAC_DCMD_TIE;
	dma_desc->dcmd &= ~DMAC_DCMD_LINK;
	dma_desc->ddadr &= ~(0xff << 24);
	dma_cache_wback_inv((unsigned long)read_desc, run_time * sizeof(jz_dma_desc_8word));
	dma_cache_wback_inv((unsigned long)trig_value, ARRAY_SIZE(trig_value)*sizeof(unsigned int));

#if 0 /* DEBUG */
	dma_desc = read_desc;
	printk("================================\n");
	for(i = 0 ; i< run_time; i++){
		printk("******************************\n");
		printk("desc[%d] addr is 0x%08x\n",i, (unsigned int)dma_desc);
		printk("desc[%d] saddr is 0x%08x\n", i, dma_desc->dsadr);
		printk("desc[%d] taddr is 0x%08x\n", i, dma_desc->dtadr);
		printk("desc[%d] dreqt is 0x%08x\n", i, dma_desc->dreqt);
		printk("desc[%d] dcmd  is 0x%08x\n", i, dma_desc->dcmd);
		printk("desc[%d] ddadr is 0x%08x\n", i, dma_desc->ddadr);
		dma_desc++;
	}
#endif
	REG_DMAC_DDA(dma_info->chan) = CPHYSADDR(read_desc);
	REG_DMAC_DMADBSR = 1 << dma_info->chan;
	REG_DMAC_DCCSR(dma_info->chan) |=  DMAC_DCCSR_EN;
	REG_DMAC_DMACR |= DMAC_DMACR_DMAE; /* global DMA enable bit */

	if (need_wait) {
		int res = DANGERS_WAIT_ON(atomic_read(&dma_info->is_waiting), 2);
		__i2c_dma_rd_disable(bus_id);
		if (res) {
			printk("WARNNING: i2c%d read error, did not receive enough data from i2c slave!\n", bus_id);
			jz_stop_dma(dma_info->chan);
			return -ETIMEDOUT;
		}
	}

#if 0  /* old code */
	__i2c_set_dma_rd_level(bus_id, 3);
	__i2c_dma_rd_enable(bus_id);
	memset((void *)buf, 0, length * sizeof(unsigned char));
	dma_cache_wback_inv((unsigned long)buf, length * sizeof(unsigned char));

	if (need_wait)
		atomic_set(&dma_info->is_waiting, 1);
	else
		atomic_set(&dma_info->is_waiting, 0);

	/* Init DMA module */
	REG_DMAC_DCCSR(dma_info->chan) = 0;
	REG_DMAC_DRSR(dma_info->chan) = dma_info->dma_req;
	REG_DMAC_DSAR(dma_info->chan) = CPHYSADDR(I2C_DC(bus_id));
	REG_DMAC_DTAR(dma_info->chan) = CPHYSADDR(buf);
	REG_DMAC_DTCR(dma_info->chan) = length;
	REG_DMAC_DCMD(dma_info->chan) = DMAC_DCMD_DAI | DMAC_DCMD_SWDH_8 | DMAC_DCMD_DWDH_8 | DMAC_DCMD_DS_8BIT | DMAC_DCMD_TIE;

	//dump_jz_dma_channel(dma_info->chan);
	REG_DMAC_DCCSR(dma_info->chan) = DMAC_DCCSR_NDES | DMAC_DCCSR_EN;
	REG_DMAC_DMACR |= DMAC_DMACR_DMAE; /* global DMA enable bit */

	if (need_wait) {
		int res = DANGERS_WAIT_ON(atomic_read(&dma_info->is_waiting), 2);
		__i2c_dma_rd_disable(bus_id);
		if (res) {
			printk("WARNNING: i2c%d read error, did not receive enough data from i2c slave!\n", bus_id);
			jz_stop_dma(dma_info->chan);
			return -ETIMEDOUT;
		}
	}
#endif

	return dma_info->chan;
}

static int dma_send_cmd_pio(unsigned char device, int length, struct jz_i2c *i2c)
{
	int timeout;
	int i2c_id = i2c->id;
	int i = 0;
	int ret = 0;
//	printk("=====>enter %s\n", __func__);
	for (i = 0; i < length; i++) {
		DANGERS_WAIT_ON((!__i2c_txfifo_not_full(i2c_id)), 1);
		if(i == (length - 1))
			__i2c_write((I2C_READ_CMD | I2C_DC_STP), i2c_id);
		else
			__i2c_write(I2C_READ_CMD, i2c_id);
	}

	timeout = TIMEOUT;
	while((!(REG_I2C_STA(i2c_id) & I2C_STA_TFE)) && --timeout){
		udelay(10);
	}
	if (!timeout){
		printk("Send cmd by pio: i2c device 0x%2x failed: wait TF buff empty timeout.\n",device);
		ret = -ETIMEDOUT;
		jz_dump_i2c_regs(i2c_id, __LINE__);
		goto out;
	}
	if ((REG_I2C_INTST(i2c_id) & I2C_INTST_TXABT) ||
			REG_I2C_TXABRT(i2c_id)) {
		volatile int intr;
		printk("Send cmd by pio: i2c device 0x%2x failed: device no ack or abort.\n",device);
		__i2c_clear_interrupts(intr,i2c_id);
		ret = -ETIMEDOUT;
		goto out;
	}
out:
	return ret;
}

static int xfer_read(unsigned char device, unsigned char *buf,
		int length, struct jz_i2c *i2c, int restart)
{
	int timeout;
	int i2c_id = i2c->id;
	int i, chan;
	int ret = 0;
	unsigned short *rbuf;

	REG_I2C_INTM(i2c_id) = 0;

//	printk("=====>enter %s\n", __func__);
	rbuf = (unsigned short *)kzalloc(sizeof(unsigned short) * length,GFP_KERNEL);
	for (i = 0; i < length; i++){
		rbuf[i] = I2C_READ_CMD;
		if(i == length - 1)
			rbuf[i] |= I2C_DC_STP;
	}
	memset((void *)buf, 0, length * sizeof(unsigned char));

	chan = jz_i2c_dma_init_as_read(buf,length,i2c_id, 0);
	atomic_set(&rx_dma_info[i2c_id].is_waiting, 1);

#if 1
	if(dma_write_by_cpu == 1){
		ret = dma_send_cmd_pio(device, length, i2c);
		if(ret)
			goto out;
	}else
		jz_i2c_dma_init_as_write(rbuf,length,i2c_id, 0);

#endif
	ret = DANGERS_WAIT_ON(atomic_read(&rx_dma_info[i2c_id].is_waiting), 2);
	__i2c_dma_rd_disable(i2c_id);
	__i2c_dma_td_disable(i2c_id);
	if (ret) {
		printk("WARNING: i2c%d did not receive enough data from slave!\n", i2c_id);
		jz_stop_dma(tx_dma_info[i2c_id].chan);
		jz_stop_dma(rx_dma_info[i2c_id].chan);

		goto out;
	}

	timeout = TIMEOUT;
	while ((REG_I2C_STA(i2c_id) & I2C_STA_MSTACT) && timeout) {
		timeout --;
		udelay(10);
	}
	if (!timeout) {
		printk("WARNING(%s:%d): i2c%d wait master inactive failed!\n", __func__, __LINE__, i2c_id);
		ret = -ETIMEDOUT;
	}

out:
	kfree(rbuf);
	return ret;
}
static int xfer_write(unsigned char device, unsigned char *buf,
		int length, struct jz_i2c *i2c, int restart)
{
	int timeout;
	int i2c_id = i2c->id;
	int i = 0;
	int ret = 0;
	unsigned short *wbuf;

	REG_I2C_INTM(i2c_id) = 0;
	wbuf = (unsigned short *)kzalloc(sizeof(unsigned short) * length,GFP_KERNEL);
	for (i = 0; i < length; i++){
		wbuf[i] = I2C_WRITE_CMD | buf[i];
		if((i == (length - 1)) && !restart)
			wbuf[i] |= I2C_DC_STP;
	}
	ret = jz_i2c_dma_init_as_write(wbuf,length,i2c_id, 1);
	if (ret)
		goto out;

	timeout = TIMEOUT;
	while((!(REG_I2C_STA(i2c_id) & I2C_STA_TFE)) && timeout){
		timeout --;
		udelay(10);
	}
	if (!timeout){
		printk("Write i2c device 0x%2x failed, wait TF buff empty timeout.\n",device);
		ret = -ETIMEDOUT;
		goto out;
	}

	if (!restart) {
		timeout = TIMEOUT;
		while (__i2c_master_active(i2c_id) && timeout)
			timeout--;
		if (!timeout){
			printk("Write i2c device 0x%2x failed, wait master inactive timeout.\n",device);
			ret = -ETIMEDOUT;
			goto out;
		}
	}

	msleep(1); /* the TXABRT bit seems not immediatly seted when error happen */
	if ((REG_I2C_INTST(i2c_id) & I2C_INTST_TXABT) ||
			REG_I2C_TXABRT(i2c_id)) {
		volatile int tmp;
		printk("Write i2c device 0x%2x failed: device no ack or abort.\n",device);
		__i2c_clear_interrupts(tmp,i2c_id);
		ret = -ECANCELED;
		goto out;
	}


out:
	kfree(wbuf);
	return ret;
}
#endif

static int xfer_read_pio(unsigned char device, unsigned char *buf,
		int length, struct jz_i2c *i2c, int restart)
{
	int timeout;
	int i2c_id = i2c->id;
	int cnt;
	unsigned long start_time;
	int ret = 0;

	if (length  > 64)     /* FIFO depth is 16 */
		return -1;

	for (cnt = 0; cnt < length; cnt++) {
		DANGERS_WAIT_ON((!__i2c_txfifo_not_full(i2c_id)), 1);
		if(cnt == (length - 1))
			__i2c_write((I2C_READ_CMD | I2C_DC_STP), i2c_id);
		else
			__i2c_write(I2C_READ_CMD, i2c_id);
	}
	udelay(100);
	memset((void *)buf, 0, length * sizeof(unsigned char));

	for (cnt = 0; cnt < length; cnt++) {
		start_time = jiffies;
		while (!(REG_I2C_STA(i2c_id) & I2C_STA_RFNE)) {
			if ((REG_I2C_INTST(i2c_id) & I2C_INTST_TXABT) ||
					REG_I2C_TXABRT(i2c_id)) {
				volatile int tmp;
				printk("Read i2c device 0x%2x failed: i2c abort.\n",device);
				__i2c_clear_interrupts(tmp,i2c_id);
				ret = -ECANCELED;
				goto out;
			}
			if (time_after(jiffies, start_time + HZ)) {
				printk("WARNING: i2c%d: data not received after 1 second!\n", i2c_id);
				ret = -ETIMEDOUT;
				goto out;
			}
			udelay(10);
		}
		buf[cnt] = __i2c_read(i2c_id);
	}

	timeout = TIMEOUT;
	while ((REG_I2C_STA(i2c_id) & I2C_STA_MSTACT) && --timeout)
		udelay(10);
	if (!timeout){
		printk("Read i2c device 0x%2x failed: wait master inactive timeout.\n",device);
		ret = -ETIMEDOUT;
	}

out:
	return ret;
}

static int xfer_read_offset_pio(unsigned char device,
		unsigned char *offset, int off_len,
		unsigned char *buf, int read_len,
		struct jz_i2c *i2c)
{
	int timeout;
	int i2c_id = i2c->id;
	int i;
	int ret = 0;
	unsigned long start_time;
	int total_len = off_len + read_len;
	unsigned short *wbuf = (unsigned short *)kmalloc(sizeof(unsigned short) * total_len, GFP_KERNEL);;
	if(!wbuf){
		printk("%s can not malloc space\n", __func__);
		goto out; 
	}
	if (read_len  > 64)     /* FIFO depth is 64 */
		return -EINVAL;

	if (total_len > 80)     /* offset len max 16 */
		return -EINVAL;

	for (i = 0; i < off_len; i++) {
		wbuf[i] = I2C_WRITE_CMD | *offset;
		offset++;
	}
	for (i = 0; i < read_len; i++){
		wbuf[i + off_len] = I2C_READ_CMD;
		if(i == (read_len - 1))
			wbuf[i + off_len] |= I2C_DC_STP;
	}

#if 0
	/*test start stop rxfl rxof txof and rxuf interrupt interrupt */
	REG_I2C_RXTL(i2c_id) = 0;
	REG_I2C_INTM(i2c_id) = I2C_NORMAL_MASK; // NOTE : please set int the interrupt handler too

	__i2c_read(i2c_id); // test rxuf interrupt 

	for(i = 0; i < 1000; i++){
		if(i == 0)
			__i2c_write((I2C_WRITE_CMD | 0x00), i2c_id);
		else if(i == 999)	
			__i2c_write(I2C_READ_CMD | I2C_DC_STP, i2c_id);
		else
			__i2c_write(I2C_READ_CMD, i2c_id);
	}

	msleep(5000);
#else
	/*set normal mask */
//	REG_I2C_INTM(i2c_id) = I2C_NORMAL_MASK;
	REG_I2C_INTM(i2c_id) = 0;
        is_receive = 1;
#endif

	for (i = 0; i < total_len; i++) {
		DANGERS_WAIT_ON((!__i2c_txfifo_not_full(i2c_id)), 1);
		__i2c_write(wbuf[i], i2c_id);
	}
	udelay(100);
	timeout = TIMEOUT;
	while((!(REG_I2C_STA(i2c_id) & I2C_STA_TFE)) && --timeout){
		udelay(100);
	}
	if (!timeout){
		printk("%s:%d: wait tx fifi empty timedout! dev_addr = 0x%02x\n",
				__func__, __LINE__, device);
		ret = -ETIMEDOUT;
		jz_dump_i2c_regs(i2c_id, __LINE__);
		goto out;
	}
 
	/* TXABRT 0 bit: master is not recive a ack */
	if ((REG_I2C_INTST(i2c_id) & I2C_INTST_TXABT) ||
			REG_I2C_TXABRT(i2c_id)) {
		int tmp;
		printk("%s:%d: TX abart, dev(0x%02x0 no ack!\n",
				__func__, __LINE__, device);
		__i2c_clear_interrupts(tmp,i2c_id);
		ret = -ECANCELED;
		goto out;
	}

	memset((void *)buf, 0, read_len * sizeof(unsigned char));
	for (i = 0; i < read_len; i++) {
		start_time = jiffies;
		while (!(REG_I2C_STA(i2c_id) & I2C_STA_RFNE)) {

			if ((REG_I2C_INTST(i2c_id) & I2C_INTST_TXABT) ||
					REG_I2C_TXABRT(i2c_id)) {
				int tmp;
				jz_dump_i2c_regs(i2c_id, __LINE__);
				__i2c_clear_interrupts(tmp,i2c_id);
				printk("%s:%d: i2c transfer aborted, dev_addr = 0x%02x, intr = 0x%08x.\n",
						__func__, __LINE__, device, tmp);
				ret = -ECANCELED;
				goto out;

			}
			if (time_after(jiffies, start_time + HZ)) {
				printk("WARNING: i2c%d: data not received after 1 second!\n", i2c_id);
				ret = -ETIMEDOUT;
				goto out;
			}
			udelay(10);
		}
		buf[i] = __i2c_read(i2c_id);
	}

	is_receive = 0;
	timeout = TIMEOUT;
	while ((REG_I2C_STA(i2c_id) & I2C_STA_MSTACT) && --timeout)
		udelay(10);
	if (!timeout){
		printk("%s:%d: waite master inactive timeout, dev_addr = 0x%02x\n",
				__func__, __LINE__, device);
		ret = -ETIMEDOUT;
		goto out;
	}
out:
	kfree(wbuf); //  by zjj 2014.10.21
	return ret;
}

static int xfer_write_pio(unsigned char device, unsigned char *buf,
		int length, struct jz_i2c *i2c, int restart)
{
	int timeout;
	int i2c_id = i2c->id;
	int i = 0;
	int ret = 0;
//	printk("=====>enter %s\n", __func__);
	unsigned short *wdata = (unsigned short *)kmalloc(sizeof(unsigned short) * length,GFP_KERNEL); 
	if(!wdata){
		printk("%s can not malloc space\n", __func__);
		goto out; 
	}

#if 0
	/*test after write a data has a stop every times */
	for(i = 0; i < length; i++){
		wdata[i] = I2C_DC_STP | I2C_WRITE_CMD | *buf++;
	}
#else
	for(i = 0; i < length; i++){
		wdata[i] = I2C_WRITE_CMD | *buf++;
		if(i == (length - 1))
			wdata[i] |= I2C_DC_STP;
	}
#endif 
#if 0
	/* test TXEMP, active txabrt interruput */

	/*first test TXEMP interrupt */
	REG_I2C_INTM(i2c_id) = (1 << 4);
	msleep(2000);


	/*second test activety interrupt when tx fifo has data and normal interrrupt*/
	/* set normal mask interrupt */
	REG_I2C_INTM(i2c_id) = I2C_NORMAL_MASK | (1 << 8);
	is_transmit = 1;
#endif

	REG_I2C_INTM(i2c_id) = 0;
	for (i = 0; i < length; i++) {
		DANGERS_WAIT_ON((!__i2c_txfifo_not_full(i2c_id)), 1);
		__i2c_write(wdata[i], i2c_id);
	}

	timeout = TIMEOUT;
	while((!(REG_I2C_STA(i2c_id) & I2C_STA_TFE)) && --timeout){
		udelay(10);
	}
	if (!timeout){
		printk("Write i2c device 0x%2x failed: wait TF buff empty timeout.\n",device);
		ret = -ETIMEDOUT;
		jz_dump_i2c_regs(i2c_id, __LINE__);
		goto out;
	}
	
	timeout = TIMEOUT;
	while (__i2c_master_active(i2c_id) && --timeout);
	if (!timeout){
		printk("Write i2c device 0x%2x failed: wait master inactive timeout.\n",device);
		ret = -ETIMEDOUT;
		jz_dump_i2c_regs(i2c_id, __LINE__);
		goto out;
	}
	is_transmit = 0;
	if ((REG_I2C_INTST(i2c_id) & I2C_INTST_TXABT) ||
			REG_I2C_TXABRT(i2c_id)) {
		volatile int intr;
		printk("Write i2c device 0x%2x failed: device no ack or abort.\n",device);
		__i2c_clear_interrupts(intr,i2c_id);
		ret = -ETIMEDOUT;
		goto out;
	}

out:
	kfree(wdata); //  by zjj 2014.10.21
	return ret;
}

static int i2c_jz_xfer(struct i2c_adapter *adap, struct i2c_msg *pmsg, int num)
{
	int ret, i;
	struct jz_i2c *i2c = adap->algo_data;
	__u16 addr = pmsg->addr;

	BUG_ON(in_irq());     /* we can not run in hardirq */

	if (num > 2)	      /* sorry, our driver currently can not support more than two message */
		return -EINVAL;

	if (pmsg->addr != current_device[i2c->id]) {
		current_device[i2c->id] = pmsg->addr;
		if (i2c_init_as_master(i2c->id, addr) < 0)
			return -EBUSY;
	}

	if ((num > 1) &&
			i2c->read_offset &&
			((pmsg[0].flags & I2C_M_RD) == 0) &&
			(pmsg[1].flags & I2C_M_RD)) {
		ret = i2c->read_offset(pmsg[0].addr,
				pmsg[0].buf, pmsg[0].len,
				pmsg[1].buf, pmsg[1].len,
				i2c);
		if (ret) {
			i2c_init_as_master(i2c->id, addr);
			return ret;
		} else
			return num;
	}

	for (i = 0; i < num; i++) {
		if (likely(pmsg->len && pmsg->buf)) {	/* sanity check */
			if (pmsg->flags & I2C_M_RD){
				ret = i2c->read(pmsg->addr, pmsg->buf, pmsg->len, i2c, (num > 1));
			} else {
				ret = i2c->write(pmsg->addr, pmsg->buf, pmsg->len, i2c, (num > 1));
			}
			if (ret) {
				i2c_init_as_master(i2c->id, addr);
				return ret;
			}
		}
		pmsg++;		/* next message */
	}

	return i;
}

#define I2C_TEST
#ifdef I2C_TEST

#define EEPROM_PAGE_SIZE 16
#include <linux/random.h>

static inline unsigned int new_rand(void)
{
	return get_random_int();
}

static int i2c_write_test(struct jz_i2c *i2c, unsigned char dev_addr,
		unsigned char start_addr, int len) {
	struct i2c_msg msg;
	unsigned char *buf = NULL;
	int j = 0;
	int ret = 0;

	printk("write 0x%02x at 0x%02x, len = %d......\n", dev_addr, start_addr, len);
	buf = (unsigned char *)kmalloc(len + 1, GFP_KERNEL);
	if (!buf) {
		printk("malloc buf failed\n");
		return -ENOMEM;
	}

	buf[0] = start_addr;
	for (j = 0; j < len; j++)
		buf[j + 1] = new_rand() & 0xff;

	msg.addr = dev_addr;
	msg.buf = buf;
	msg.len = len + 1;
	msg.flags = 0;

	printk("write data:\n");
	for (j = 0; j < len; j++) {
		printk("0x%02x ", buf[j + 1]);
		if ((j != 0) && (j % 8 == 7))
			printk("\n");
	}
	printk("\n");

	ret = i2c_transfer(&i2c->adap, &msg, 1);
	if ( ret != 1)
		printk("write failed!, ret = %d\n", ret);

	kfree(buf);
	return ret;
}

static int i2c_read_test(struct jz_i2c *i2c, unsigned char dev_addr,
		unsigned char start_addr, int len) {
	unsigned char msg1[4] = { 0 };
	unsigned char *msg2 = NULL;
	int j = 0;
	int ret = 0;
	struct i2c_msg msgs[2];

	printk("read 0x%02x at 0x%02x, len = %d......\n", dev_addr, start_addr, len);
	msg2 = (unsigned char *)kmalloc(len, GFP_KERNEL);
	if (!msg2) {
		printk("malloc msg2 failed!\n");
		return -ENOMEM;
	}

	msg1[0] = start_addr;
	msgs[0].addr = dev_addr;
	msgs[0].buf = msg1;
	msgs[0].len = 1;
	msgs[0].flags = 0;

	memset(msg2, 0, len);
	msgs[1].addr = dev_addr;
	msgs[1].buf = msg2;
	msgs[1].len = len;
	msgs[1].flags = I2C_M_RD;

	ret = i2c_transfer(&i2c->adap, msgs, 2);

	printk("ret = %d data: \n", ret);
	if (ret == 2) {
		for (j = 0; j < len; j++) {
			printk("0x%02x ", msg2[j]);
			if ((j != 0) && (j % 8 == 7))
				printk("\n");
		}

		printk("\n");
	}

	kfree(msg2);
	return ret;
}

static struct timer_list i2c_test_timer[6];

static void i2c_test_func(unsigned long data) {
	struct jz_i2c *i2c = (struct jz_i2c *)data;

	i2c_write_test(i2c, 0x50, 5, 12);
	i2c_read_test(i2c, 0x50, 0, 12);

	i2c_test_timer[i2c->id].expires = jiffies + HZ;
	add_timer(&i2c_test_timer[i2c->id]);
}

static int i2c_test(void *data) {
	struct jz_i2c *i2c = (struct jz_i2c *)data;

	printk("====start test on i2c%d=====\n", i2c->id);

	init_timer(&i2c_test_timer[i2c->id]);
	i2c_test_timer[i2c->id].function = i2c_test_func;
	i2c_test_timer[i2c->id].expires = jiffies + HZ;
	i2c_test_timer[i2c->id].data = (unsigned long)i2c;

	add_timer(&i2c_test_timer[i2c->id]);

	return 0;
}
#else
#define i2c_test(i) do {  } while(0)

#endif

static irqreturn_t jz_i2c_irq(int irqno, void *dev_id)
{
	volatile int ret;
	unsigned short clear_flag;
	struct jz_i2c * i2c =(struct jz_i2c *)dev_id;
	int i2c_id = i2c->id;
	unsigned short abrt =  REG_I2C_TXABRT(i2c_id);
	unsigned int irq_flag =  REG_I2C_INTST(i2c_id); // get irq flag

	REG_I2C_INTM(i2c_id) = 0; // mask all irq

	printk("====>enter  %s\n", __func__);
	printk("init flags value is 0x%04x\n", irq_flag);
#if 1
	if(irq_flag & I2C_INTST_IACT){
		printk("Note : I2c is activety ....\n")	;
		clear_flag = REG_I2C_CACT(i2c_id);
	}
	if(irq_flag & I2C_INTST_TXEMP){
		printk("Note : transmit fifo level is  below the I2C_TXTL value\n");
	}
	if(irq_flag & I2C_INTST_RXFL){
		printk("Note : receive fifo level is above the I2C_RXTL value\n");
	}
#endif
	if(irq_flag & I2C_INTST_RXUF){
		printk("Note: receive fifo is empty, Can not read data\n");
		clear_flag = REG_I2C_CRXUF(i2c_id);
	}
	if(irq_flag & I2C_INTST_RXOF){
		printk("Note: receive fifo is full, and another data is received still\n");
		clear_flag = REG_I2C_CRXOF(i2c_id);
	}
	if(irq_flag & I2C_INTST_TXOF){
		printk("Note: transmit fifo is full, and another is send to fifo still\n");
		clear_flag = REG_I2C_CTXOF(i2c_id);
	}
	if((irq_flag & I2C_INTST_TXABT) || abrt){
		printk("ABRT: REG_I2C_TXABRT(%d) = 0x%04x\n", i2c_id, abrt);
		if(is_transmit)
			printk("Write i2c device 0x%2x failed: device no ack or abort.\n", current_device[i2c_id]);
		if(is_receive)
			printk("Read i2c device 0x%2x failed: i2c abort.\n", current_device[i2c_id]);
		clear_flag = REG_I2C_CTXABRT(i2c_id);
	}
	if(irq_flag & I2C_INTST_ISTP){
		printk("Note:  Stop signal is send\n");
		clear_flag = REG_I2C_CSTP(i2c_id);
	}
	if(irq_flag & I2C_INTST_ISTT){
		printk("Note: Start signal is send\n");
	}

	__i2c_clear_interrupts(ret, i2c_id);
//	REG_I2C_INTM(i2c_id) |= I2C_NORMAL_MASK;
	REG_I2C_INTM(i2c_id) = I2C_INTST_RXUF;


	return IRQ_HANDLED;
}

static u32 i2c_jz_functionality(struct i2c_adapter *adap)
{
	return I2C_FUNC_I2C | I2C_FUNC_SMBUS_EMUL;
}

static const struct i2c_algorithm i2c_jz_algorithm = {
	.master_xfer	= i2c_jz_xfer,
	.functionality	= i2c_jz_functionality,
};

static int i2c_jz_probe(struct platform_device *pdev)
{
	int i;
	unsigned int address = 0x50;
	struct jz_i2c *i2c;
	struct i2c_jz_platform_data *plat = pdev->dev.platform_data;

	struct jz_i2c_dma_info *tx_dma = tx_dma_info + pdev->id;
	struct jz_i2c_dma_info *rx_dma = rx_dma_info + pdev->id;
	int ret;
	pdev->id = pdev->id >=0 ? pdev->id : 0;
	if (rx_dma->use_dma) {

		rx_dma->chan = jz_request_dma(rx_dma->dma_id, rx_dma->name,
				jz_i2c_dma_callback, IRQF_DISABLED, rx_dma);
		printk("i2c%d: rx chan = %d\n", pdev->id, rx_dma->chan);
		if (rx_dma->chan < 0) {
			printk("i2c%d: request RX dma failed\n", pdev->id);
			return -ENODEV;
		}

		tx_dma->chan = jz_request_dma(tx_dma->dma_id, tx_dma->name,
				jz_i2c_dma_callback, IRQF_DISABLED, tx_dma);
		printk("i2c%d: tx chan = %d\n", pdev->id, tx_dma->chan);
		if (tx_dma->chan < 0) {
			printk("i2c%d: request TX dma failed have not DMA chan to use\n", pdev->id);
			tx_dma->chan = -1;
			dma_write_by_cpu = 1;
		//	return -ENODEV;
		}
//		dma_write_by_cpu = 1; // when test use
	}

	i2c = kzalloc(sizeof(struct jz_i2c), GFP_KERNEL);
	if (!i2c) {
		printk("i2c%d: alloc jz_i2c failed!\n", pdev->id);
		ret = -ENOMEM;
		goto emalloc;
	}
#if 0
	printk("+++++++++++++++++++++\n");
	{
		int i;
		for(i = 0; i < 10; i++){
			__gpio_as_output0(32 * 4 + 0);
			__gpio_as_output0(32 * 4 + 3);
			msleep(100);
			__gpio_as_output1(32 * 4 + 0);
			__gpio_as_output1(32 * 4 + 3);
			msleep(100);
		}
	}
	
	msleep(3000);
#endif
	switch(pdev->id) {
		case 0:
			cpm_start_clock(CGM_I2C0);
//			__gpio_as_i2c(pdev->id);
			__gpio_as_func0(32 * 3 + 30);
			__gpio_as_func0(32 * 3 + 31);
			break;
		case 1:
			cpm_start_clock(CGM_I2C1);
			//__gpio_as_i2c(pdev->id);
			__gpio_as_func0(32 * 4 + 30);
			__gpio_as_func0(32 * 4 + 31);
			break;
		case 2:
			cpm_start_clock(CGM_I2C2);
			//__gpio_as_i2c2();
			__gpio_as_func1(32 * 4 + 0);
			__gpio_as_func1(32 * 4 + 3);

			break;
		default:
			;
	}
	if (rx_dma->use_dma) {
		if(dma_write_by_cpu == 1)
			i2c->write = xfer_write_pio;
		else
			i2c->write = xfer_write;
		i2c->read = xfer_read;
		i2c->read_offset = NULL;
	} else 
	{
		i2c->write = xfer_write_pio;
		i2c->read = xfer_read_pio;
		i2c->read_offset = xfer_read_offset_pio;
	}

	i2c->id            = pdev->id;
	i2c->adap.owner   = THIS_MODULE;
	i2c->adap.algo    = &i2c_jz_algorithm;
	i2c->adap.retries = 5;
	sprintf(i2c->adap.name, "jz_i2c-i2c.%u", pdev->id);
	i2c->adap.algo_data = i2c;
	i2c->adap.dev.parent = &pdev->dev;

	read_desc = (jz_dma_desc_8word *)__get_free_pages(GFP_KERNEL, 0);
	if(!read_desc){
		printk("Can not get dma descripter space...\n");
		goto emalloc;
	}
	read_desc = (unsigned int)read_desc | 0xa0000000;

	i2c_init_as_master(i2c->id, 0xff);
	if (plat)
		i2c->adap.class = plat->class;

	i2c->irq = platform_get_irq(pdev, 0);
	ret = request_irq(i2c->irq, jz_i2c_irq, IRQF_DISABLED,
			dev_name(&pdev->dev), i2c);
	if (ret != 0) {
		dev_err(&pdev->dev, "cannot claim IRQ %d\n", i2c->irq);
		goto irq_err;
	}

	i2c->adap.nr = pdev->id;

	ret = i2c_add_numbered_adapter(&i2c->adap);
	if (ret < 0) {
		printk(KERN_INFO "i2c%d: Failed to add bus\n", pdev->id);
		goto eadapt;
	}

	platform_set_drvdata(pdev, i2c);
	dev_info(&pdev->dev, "JZ4775 i2c bus driver.\n");


#if 1
	/* test interrupt */
//	i2c_write_test(i2c, address, 0, 15);
//	i2c_read_test(i2c, address, 0, 15);
//	while(1);
#endif


#ifdef CONFIG_JZ_I2C_TEST 
	while(1){

		//i2c_write_test(i2c, address, 0, EEPROM_PAGE_SIZE);
		//i2c_read_test(i2c, address, 0, EEPROM_PAGE_SIZE);
		i2c_write_test(i2c, address, 0, 15);
		i2c_read_test(i2c, address, 0, 15);
		if(address == 0x57)
			address = 0x50;
		else
			address += 0x1;
		printk("===================================================================================\n");
	}
#endif
	return 0;

eadapt:
	free_irq(i2c->irq, i2c);
emalloc:
	jz_free_dma(tx_dma->chan);
	jz_free_dma(rx_dma->chan);
	kfree(read_desc);
irq_err:
	kfree(i2c);
	return ret;
}

static int i2c_jz_remove(struct platform_device *pdev)
{
	struct jz_i2c *i2c = platform_get_drvdata(pdev);
	struct i2c_adapter *adapter = &i2c->adap;
	int rc;

	rc = i2c_del_adapter(adapter);
	platform_set_drvdata(pdev, NULL);

	if (rx_dma_info[i2c->id].chan >= 0)
		jz_free_dma(rx_dma_info[i2c->id].chan);
	if (tx_dma_info[i2c->id].chan >= 0)
		jz_free_dma(tx_dma_info[i2c->id].chan);
	return rc;
}

#ifdef  CONFIG_JZ_I2C0
static struct platform_driver i2c_0_jz_driver = {
	.probe		= i2c_jz_probe,
	.remove		= i2c_jz_remove,
	.driver		= {
		.name	= "jz_i2c0",
	},
};
#endif

#ifdef  CONFIG_JZ_I2C1
static struct platform_driver i2c_1_jz_driver = {
	.probe		= i2c_jz_probe,
	.remove		= i2c_jz_remove,
	.driver		= {
		.name	= "jz_i2c1",
	},
};
#endif
#ifdef  CONFIG_JZ_I2C2
static struct platform_driver i2c_2_jz_driver = {
	.probe		= i2c_jz_probe,
	.remove		= i2c_jz_remove,
	.driver		= {
		.name	= "jz_i2c2",
	},
};
#endif

static int __init i2c_adap_jz_init(void)
{
	int ret = 0;

#ifdef  CONFIG_JZ_I2C0
	ret = platform_driver_register(&i2c_0_jz_driver);
#endif
#ifdef  CONFIG_JZ_I2C1
	ret = platform_driver_register(&i2c_1_jz_driver);
#endif
#ifdef  CONFIG_JZ_I2C2
	ret = platform_driver_register(&i2c_2_jz_driver);
#endif
	return ret;
}

static void __exit i2c_adap_jz_exit(void)
{
#ifdef  CONFIG_JZ_I2C0
	platform_driver_unregister(&i2c_0_jz_driver);
#endif
#ifdef  CONFIG_JZ_I2C1
	platform_driver_unregister(&i2c_1_jz_driver);
#endif
#ifdef  CONFIG_JZ_I2C2
	platform_driver_unregister(&i2c_2_jz_driver);
#endif

}

MODULE_LICENSE("GPL");
subsys_initcall(i2c_adap_jz_init);
module_exit(i2c_adap_jz_exit);
