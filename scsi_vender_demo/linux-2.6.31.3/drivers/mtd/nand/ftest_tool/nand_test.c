/*
 * linux/drivers/mtd/nand/nand_test.c
 * 
 * JZ4775 NAND test
 * 
 * Copyright(c)2005-20012IngenicSemiconductorInc.
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/mtd/mtd.h>
#include <linux/mtd/nand.h>
#include <linux/random.h>

#include <asm/io.h>
#include <asm/jzsoc.h>


#define DEBUG_LINE	0
#if DEBUG_LINE
#define DEBG	printk("%s:L%d\n", __FUNCTION__, __LINE__)
#else
#define DEBG
#endif

unsigned char *databuf = NULL;
unsigned char *tempbuf = NULL;
unsigned char *oobbuf = NULL;
unsigned char *tempbuf2 = NULL;


/******************************************************************************
 * TEST LIBRARY FUNCTIONS
 ******************************************************************************/
/*
 * get_random_range get one random number between start and end
 * check if the random num generated is same with the value in buf
 * */
static inline int get_random_range(int start, int end, int pos, unsigned int *buf)
{
	unsigned int num = 0;
	int i, ret = 1;

	while (ret) {
		ret = 0;
		num = start + random32() % (end-start);
		for (i=0; i<pos; i++) {
			if (buf[i] == num) {
				ret = 1;
				break;
			}
		}
	}
	//printk("%s random num:%d\n", __func__, ret);
	return (int)num;
}

/*
 * buf_init	init a buffer with regularly data
 * */
static inline void buf_init(uint8_t *buffer, int len)
{
	u8 *buf = buffer;
	int i;

#if 0
	for(i=0; i<len; i++){
		switch(((i/1024)&0x3)) {
			case 0:
				buf[i] = i % 256;
				break;
			case 1:
				buf[i] = ~(i % 256);
				break;
			case 2:
				buf[i] = (i + 128) % 256;
				break;
			case 3:
				buf[i] = ~((i + 128) % 256 );
				break;
			default:
				printk("buf_init:BUG\n");
		}
	}
#else
	for(i=0; i<len; i++){
		buf[i] = i%256;
		//buf[i] = 0x5a;
	}
#endif
#if 0
	buf = oobbuf;
	for(i=0; i<OOBBUF_SIZE; i++)
	{
		buf[i] = i % 256;
	}
#endif
}

/*
 * buf_init_random	init a buffer with regularly data
 * */
static inline void buf_init_random(uint8_t *buffer, int len)
{
	int i;

	for(i=0; i<len; i++){
		buffer[i] = get_random_range(0, 256, 0, NULL);
	}
}
/*
 * mem_dump print a buf with col & rowh eadlines
 * */
static void mem_dump(uint8_t *buf, int len)
{
	int line = len/16;
	int i, j;

	if (len & 0xF)
		line++;

	printk("\n<0x%08x>Length:%dBytes\n", (unsigned int)buf, len);
	printk("--------| ");
	for(i=0; i<16; i++)
		printk("%02d ", i);
	printk("\n");

	for(i=0; i<line; i++) {
		printk("%08x: ", i<<4);

		if(i<(len/16)) {
			for(j=0; j<16; j++)
				printk("%02x ", buf[(i<<4)+j]);
		} else {
			for(j=0; j<(len&0xF); j++)
				printk("%02x ", buf[(i<<4)+j]);
		}

		printk("\n");
	}
	printk("\n");
}

/*
 * buf_check check if two buffer's contents same,like memcmp()
 * */
static int buf_check(u8* org, u8* obj, int size)
{
	int i, ecnt=0;

	for(i=0; i<size; i++) {
		if(org[i] != obj[i]) {
			printk("%s--pos:%d %02x<-->%02x\n", __func__, i, org[i], obj[i]);
			ecnt++;
			if (ecnt > 64) {
				printk("More than 64 bit error, stop check.\n");
				break;
			}
		}
	}
	if (ecnt) {
		printk("%s--Totalerrorcnt:%d\n", __func__, ecnt);
	} else
		printk("%s--bufcheck:NoError!\n", __func__);
	return ecnt;
}

static inline int check_ff(u8 *buf, u8 val, int size)
{
	int i = 0, ret = 0;

	for (i=0; i<size; i++) {
		if (buf[i] != val) {
			printk("%s--pos:%d val:%d", __func__, i, buf[i]);
			ret ++;
		}
	}
	if (!ret)	
		printk("%s--All value are valid.", __func__);
	return ret;
}

int serial_tstc(void)
{
	volatile u8 *uart_lsr = (volatile u8 *)(UART_BASE + OFF_LSR);

	if(*uart_lsr & UART_LSR_DR) {
		/* Datainrfifo */
		return (1);
	}
	return 0;
}

int serial_getc(void)
{
	volatile u8 *uart_rdr = (volatile u8 *)(UART_BASE + OFF_RDR);

	while (!serial_tstc());

	return *uart_rdr;
}


#define CFG_WITH_BCH 1
/******************************************************************************
 * BCH TEST FUNCTIONS
 ******************************************************************************/
//#define BCH_TEST_ENABLE 1
#if defined(BCH_TEST_ENABLE) || defined(CFG_WITH_BCH)
//#define BCH_ALLF_TEST 1

#define BCH_DEBUG 1
#ifdef BCH_DEBUG
#define bch_debug(str,args...)	printk(str,##args)
#else
#define bch_debug(str,args...)
#endif
#define bch_debug2(str,args...)

//#define REG_BCH_FF REG32((BCH_BASE + 0x198)) /* BCH Interrupt Set register */

enum{
	BCH_DR_BYTE = 0,
	BCH_DR_HWORD,
	BCH_DR_WORD
};

enum{
	ERR_IN_DATA = 0,
	ERR_IN_PARITY,
	ERR_IN_BOTH
};

//#define ARRAY_SIZE(a) (sizeof(a)/sizeof((a)[0]))

#ifdef CFG_TEST_FULL
//static int bch_datalen[] = {24,256,512,1024};
static int bch_err_check[65] = {0};
static int bch_datalen[] = {1024,512,256,24};
static int eccbit[] = {4,8,12,16,20,24,28,32,36,40,44,48,52,56,60,64};
static int bch_dr_mode[] = {BCH_DR_BYTE,BCH_DR_HWORD,BCH_DR_WORD};
static int bch_errpos[] = {ERR_IN_DATA,ERR_IN_PARITY,ERR_IN_BOTH};
static int result[ARRAY_SIZE(bch_datalen)][ARRAY_SIZE(eccbit)][ARRAY_SIZE(bch_dr_mode)][ARRAY_SIZE(bch_errpos)][3];
#else
static int bch_err_check[65] = {0};
static int bch_datalen[] = {1024};
static int eccbit[] = {24,40};
static int bch_dr_mode[] = {BCH_DR_BYTE};
static int bch_errpos[] = {ERR_IN_DATA,ERR_IN_PARITY};
static int result[ARRAY_SIZE(bch_datalen)][ARRAY_SIZE(eccbit)][ARRAY_SIZE(bch_dr_mode)][ARRAY_SIZE(bch_errpos)][3];
#endif

//#defineBCH_IRQ
#ifdef BCH_IRQ
DECLARE_WAIT_QUEUE_HEAD(nand_bch_wait_queue);
static volatile int dma_ack = 0;

/*
 * bch_irq_sync wait for event(dma_ack = 1,bch encode/decode finish)
 * */
static void bch_irq_sync(void)
{
	int err;

	do{
		bch_debug("bchwait.\n");
#if 1
		err = wait_event_interruptible_timeout(nand_bch_wait_queue,dma_ack,3*HZ);
#else
		err = wait_event_interruptible(nand_bch_wait_queue,dma_ack);
#endif
		//		dbch_debug("exit.\n");
	} while (err == -ERESTARTSYS);

	if (!err)
		bch_debug("%s----IRQerr:%d\n",__func__,err);
}

/*
 * nand_bch_irq interrupt process func
 * when interrupt happens, bch encode/decode finished,
 * so set dma_ack = 1 and wakeup &nand_bch_wait_queue.
 * */
static irqreturn_t nand_bch_irq(int irq, void *dev_id)
{
	bch_debug("\nBCH IRQ...\n");
	bch_debug("reg intc:%08x\n", REG_INTC_ISR(0));
	bch_debug("mcu intc:%08x\n", REG_INTC_DSR0);
	if (irq == IRQ_BCH) {
		/* clear mbint  */
		bch_debug("BCH INTE:%08x\n", REG_BCH_INTE);

		REG_BCH_INTEC = 0X3F;
		bch_debug("pdma bch irq, wakeup----\n");
		dma_ack = 1;
		wake_up_interruptible(&nand_bch_wait_queue);
	}
	bch_debug("reg intc:%08x\n", REG_INTC_ISR(0));
	bch_debug("mcu intc:%08x\n", REG_INTC_DSR0);

	return IRQ_HANDLED;
}

/*
 * nand_bch_irq_init request bch irq
 *  */
static int nand_bch_irq_init(void)
{
	int err;

	bch_debug("%s----request bch irq\n", __func__);
	err = request_irq(IRQ_BCH, nand_bch_irq, IRQF_DISABLED, "nand_bch", NULL);
	if (err) {
		bch_debug("can't reqeust BCH.\n");
		return -1;
	}
	return 0;
}
#endif /* #ifdef BCH_IRQ */

/*
 * bch_test_encode bch encode datas
 * data_buf:datas to be encoded
 * par_buf:parity datas to be stored
 * datasize: datasize in bytes
 * eccbit:bch correctiable error bits
 * bch_dr_mode:send data to bch data reg in bytes/hal fwords/words
 * */
static void bch_test_encode(unsigned char *data_buf, unsigned char *par_buf,
		int datasize, int eccbit, int bch_dr_mode)
{
	int eccbytes = eccbit *14/8;
	volatile u8 *paraddr = (volatile u8 *)BCH_PAR0;
	int i;

#ifdef BCH_IRQ
	bch_debug("regintc:%08x\n", REG_INTC_ISR(0));
	REG_BCH_INTES = 0x1F;
	dma_ack = 0;
#endif
	//bch_debug("%s datasize:%d, eccbit:%d, eccbytes:%d\n", __func__, datasize, eccbit, eccbytes);
	__bch_cnt_set(datasize, eccbytes);
	__bch_encoding(eccbit);
	//bch_debug("BCHCR:%08x BCHCNT:%08x BCHINTS:%08x REG_BCH_BHEA:%08x\n",
	//REG_BCH_CR, REG_BCH_CNT, REG_BCH_INTS, REG_BCH_BHEA);

	if (bch_dr_mode == BCH_DR_BYTE) {/* WritedatatoREG_BCH_DR8 */
		for (i = 0; i<datasize; i++)
			REG_BCH_DR8 = data_buf[i];
	} else if (bch_dr_mode == BCH_DR_HWORD) {/* WritedatatoREG_BCH_DR16 */
		for (i = 0; (i<<1)<datasize; i++)
			REG_BCH_DR16 = ((unsigned short *)data_buf)[i];
	} else {/* WritedatatoREG_BCH_DR32 */
		for (i=0; (i<<2)<datasize; i++)
			REG_BCH_DR32 = ((unsigned int *)data_buf)[i];
	}

#ifdef BCH_IRQ
	bch_irq_sync();
	bch_debug("BCHINTS%08x\n", REG_BCH_INTS);
#else
	//bch_debug("BCHCR:%08x BCHCNT:%08x BCHINTS:%08x REG_BCH_BHEA:%08x\n",
	//REG_BCH_CR, REG_BCH_CNT, REG_BCH_INTS, REG_BCH_BHEA);
	//bch_debug("%s syncing...\n", __func__);
	__bch_encode_sync();
#endif

	for (i=0; i<eccbytes; i++)
		par_buf[i] =  *paraddr++;

	bch_debug("%s BCHCR:%08x BCHCNT:%08x BCHINTS:%08x REG_BCH_BHEA:%08x\n",
			__func__, REG_BCH_CR, REG_BCH_CNT, REG_BCH_INTS, REG_BCH_BHEA);
	__bch_encints_clear();
	__bch_disable();
}

/*
 * bch_error_correct correct bch errors
 * */
static void bch_error_correct(unsigned short *data_buf,int err_bit)
{
	unsigned short err_mask;
	int idx;/*  indicates an error half_word  */

	idx = (REG_BCH_ERR(err_bit)&BCH_ERR_INDEX_MASK)>>BCH_ERR_INDEX_BIT;
	err_mask = (REG_BCH_ERR(err_bit)&BCH_ERR_MASK_MASK)>>BCH_ERR_MASK_BIT;

	data_buf[idx] ^= err_mask;
}

/*
 * bch_test_decode bch decode datas
 * data_buf:datas to be decoded
 * par_buf:parity datas to be stored
 * data_size:datasize in bytes
 * eccbit: bch correctiable error bits
 * bch_dr_mode:send data to bch data reg in bytes/halfwords/words
 * 
 * ret:0 success,num num of error bits,-1 Uncorrectiable error,-2 ALL FF data
 * */
static int bch_test_decode(unsigned char*data_buf,unsigned char *par_buf,
		int data_size,int eccbit,int bch_dr_mode)
{
	unsigned int err_cnt, stat;
	int ecc_bytes = eccbit*14/8;
	int i, report = 0;

	//mem_dump(data_buf, data_size);
	//mem_dump(par_buf, ecc_bytes);
#ifdef BCH_IRQ
	bch_debug("regintc:%08x\n", REG_INTC_ISR(0));
	REG_BCH_INTES = 0x1F;
	dma_ack = 0;
#endif
	__bch_cnt_set(data_size, ecc_bytes);
	__bch_decoding(eccbit);

	//bch_debug("BCHCR:%08x BCHCNT:%08x BCHINTS:%08x REG_BCH_BHEA:%08x report:%x\n",
	//REG_BCH_CR, REG_BCH_CNT, REG_BCH_INTS, REG_BCH_BHEA, report);
	if (bch_dr_mode == BCH_DR_BYTE) {	/* Write data and parites to REG_BCH_DR8 */
		for (i = 0; i<data_size; i++)
			REG_BCH_DR8 = data_buf[i];
		for (i=0; i<ecc_bytes; i++)
			REG_BCH_DR8 = par_buf[i];
	} else if (bch_dr_mode == BCH_DR_HWORD) {	/*  Write data and parities to REG_BCH_DR16 */
		for (i = 0; (i<<1)<data_size; i++)
			REG_BCH_DR16 = ((unsigned short*)data_buf)[i];
		for (i=0; (i<<1)<ecc_bytes; i++)
			REG_BCH_DR16 = ((unsigned short *)par_buf)[i];
	} else {	/* Write data and parities to REG_BCH_DR32  */
		for (i=0; (i<<2)<data_size; i++)
			REG_BCH_DR32 = ((unsigned int *)data_buf)[i];
		for (i=0; (i<<2)<ecc_bytes; i++)
			REG_BCH_DR32 = ((unsigned int *)par_buf)[i];
	}

#ifdef BCH_IRQ
	bch_irq_sync();
#else
	__bch_decode_sync();
#endif

	/* Checkd ecoding */
	stat = REG_BCH_INTS;
	//bch_debug("BCHINTS:%08x\n", stat);
	if (stat&BCH_INTS_UNCOR) {	/* Uncorrectiable Error occurred */
		bch_debug("%s Uncorrectiable error occurs\n",__func__);
		report = -1;
	} else if (stat&BCH_INTS_ERR) {
		err_cnt = (stat&BCH_INTS_ERRC_MASK)>>BCH_INTS_ERRC_BIT;
		report = (stat & BCH_INTS_TERRC_MASK) >> BCH_INTS_TERRC_BIT;

		bch_debug("%s find %d error bits, %d err halfwords in data, correcting...\n", __func__, report, err_cnt);
		for (i=0; i<err_cnt; i++)
			bch_error_correct((unsigned short*)data_buf, i);
	} else if (stat&BCH_INTS_ALLf) {
		report = -2;
	} else
		report = 0;

	bch_debug2("%s BCHCR:%08x BCHCNT:%08x BCHINTS:%08x REG_BCH_BHEA:%08x report:%x\n",
			__func__, REG_BCH_CR, REG_BCH_CNT, REG_BCH_INTS, REG_BCH_BHEA, report);
	__bch_decints_clear();
	__bch_disable();
	return report;
}


/*
 * bch_test test bch encode & decode according to the conndition.
 * datalen:length of data to be calculated
 * eccbit:bits bch can correct
 * bch_dr_mode:send data to data register in reg8/reg16/reg32
 * errcount:number of error bits
 * errpos:0 error at data area, 1 error at parity area, 2 data at both data and parity area
 * 
 * ret:0 if pass, 1 if failed
 * */
static int bch_test(int datalen, int eccbit, int bch_dr_mode, int errcount, int errpos)
{
	int i, report, num_in_data;
	int parity_len = eccbit*14/8;

	buf_init(databuf, datalen);
	memcpy(tempbuf, databuf, datalen);
	//buf_init(tempbuf,datalen);

	/* encoding */
	bch_debug("\n---bch encode\n");
	bch_test_encode(databuf, oobbuf, datalen, eccbit, bch_dr_mode);
	memcpy(tempbuf2, oobbuf, parity_len);
	//mem_dump(databuf, datalen>256?256:datalen);
	//mem_dump(oobbuf, parity_len);

	if (errcount != 0) {
		/* make error bit */
		num_in_data = 0;
		//bch_debug("---ERR_IN_BOTH errcount:%d, err in data:%d\n", errcount, num_in_data);
		switch(errpos) {
			default:
				bch_debug("***Warning,err pos invalid, default to error in data area\n");
			case ERR_IN_DATA:
				bch_debug("---ERR_IN_DATA errcount:%d\n", errcount);
				for (i=0; i<errcount; i++) {
					bch_err_check[i] = get_random_range(0, datalen*8, i, bch_err_check);
					databuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
				//errcount = buf_check(tempbuf, databuf, datalen);
				buf_check(tempbuf, databuf, datalen);
				break;
			case ERR_IN_PARITY:
				bch_debug("---ERR_IN_PARITY errcount:%d\n", errcount);
				for (i=0; i<errcount; i++) {
					bch_err_check[i] = get_random_range(0, parity_len*8, i, bch_err_check);
					oobbuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
				//errcount = buf_check(tempbuf2, oobbuf, parity_len);
				buf_check(tempbuf2, oobbuf, parity_len);
				break;
			case ERR_IN_BOTH:
				num_in_data = get_random_range(1, errcount-1, 0, NULL);
				bch_debug("---ERR_IN_BOTH errcount:%d, err in data:%d\n", errcount, num_in_data);
				for (i=0; i<num_in_data; i++) {
					bch_err_check[i] = get_random_range(0, datalen*8, i, bch_err_check);
					databuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
				//errcount = buf_check(tempbuf, databuf, datalen);
				buf_check(tempbuf, databuf, datalen);
				bch_debug("data make error finished:%d\n", num_in_data);
				for (i=0; i<errcount-num_in_data; i++) {
					bch_err_check[i] = get_random_range(0, parity_len*8, i, bch_err_check);
					oobbuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
				//errcount += buf_check(tempbuf2, oobbuf, datalen);
				buf_check(tempbuf2, oobbuf, parity_len);
				bch_debug("parity make error finished\n");
				break;
		}
		bch_debug("Total err at last:%d, num in data:%d\n", errcount, num_in_data);
		//bch_debug("---make error, errornum:%d, errpos:%d\n", errcount, errpos);
		//buf_check(tempbuf2, oobbuf, parity_len);
		//mem_dump(databuf, datalen>256?256:datalen);
	}

#if 0
	datalen = 256;
	for (i = 0; i<datalen; i++) {
		databuf[i] = get_random_range(0, 256);
	}
	for (i=0; i<parity_len; i++) {
		oobbuf[i] = get_random_range(0, 256);
	}
#endif
	/* decoding */
	bch_debug("---bch decode\n");
	report = bch_test_decode(databuf, oobbuf, datalen, eccbit, bch_dr_mode);

	//mem_dump((unsigned char*)BCH_ERR0, 64*4);
	//mem_dump(databuf, datalen>256?256:datalen);
	buf_check(tempbuf, databuf, datalen);
	if (errcount == report || (errcount>eccbit&&report == -1)) {/* -1forUncorrectiableerror */
		//bch_debug("BCH TEST OK\n");
		return 0;
	} else {
		bch_debug("BCH TEST FAILED,report:%d,errcount:%d\n", report, errcount);
		return 1;
	}
}

#ifdef BCH_ALLF_TEST 
/*
 * BCH all 0xFF data test
 * */
#define FF_BUF_SIZE 512
#define FF_MAX_BITS 32
static int bch_test_ff(void)
{
	//int eccbit[] = {4, 64};
	int eccbit[] = {64};
	int zerobits;
	int datalen, test_end0 = 1;
	int ret, t1, t2, i;

	printk("+++++++++++++++++++++++++ BCH ALL FF TEST +++++++++++++++++++++++++++++\n");
	for (t1=1; t1<FF_MAX_BITS; t1++) {
		for (t2=0; t2<ARRAY_SIZE(eccbit); t2++) {
			test_end0 = 1;
test_start:
			//printk("Tolerate:%d zero bits:%d", t1, t1);
			datalen = FF_BUF_SIZE + eccbit[t2]*14/8;
			memset(databuf, 0xFF, datalen);
			if (test_end0) {
				for (i=0; i<t1-1; i++) {
					bch_err_check[i] = get_random_range(0, (datalen-1)*8, i, bch_err_check);
					databuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
				databuf[datalen-1] ^= 0x1;
			} else {
				for (i=0; i<t1; i++) {
					bch_err_check[i] = get_random_range(0, (datalen-1)*8, i, bch_err_check);
					databuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
			}
			__bch_bhea_set(t1);
			/* -2 for ALL FF data detect */
			ret = bch_test_decode(databuf, &databuf[FF_BUF_SIZE], FF_BUF_SIZE, eccbit[t2], 0);
			zerobits = __bch_bhea_zb();
			if (ret==-2 && zerobits == t1) { /* test pass */
				printk("ECC level:%d(make %d zero) tolerate:%d zero bits:%d test_end0:%d ==> success.\n",
						eccbit[t2], t1, t1, zerobits, test_end0);
			} else { /* test failed */
				printk("ECC level:%d(make %d zero) tolerate:%d zero bits:%d test_end0:%d ==> failed.\n",
						eccbit[t2], t1, t1, zerobits, test_end0);
			}
			printk("\n");

			memset(databuf, 0xFF, datalen);
			if (test_end0) {
				for (i=0; i<t1; i++) {
					bch_err_check[i] = get_random_range(0, (datalen-1)*8, i, bch_err_check);
					databuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
				databuf[datalen-1] ^= 0x1;
			} else {
				for (i=0; i<t1+1; i++) {
					bch_err_check[i] = get_random_range(0, (datalen-1)*8, i, bch_err_check);
					databuf[bch_err_check[i]/8] ^= 0x1<<(bch_err_check[i]%8);
				}
			}
			__bch_bhea_set(t1);
			/* -2 for ALL FF data detect */
			ret = bch_test_decode(databuf, &databuf[FF_BUF_SIZE], FF_BUF_SIZE, eccbit[t2], 0);
			if (ret==-1) { /* test pass */
				printk("ECC level:%d(make %d zero) tolerate:%d test_end0:%d ==> success.\n",
						eccbit[t2], t1+1, t1, test_end0);
			} else { /* test failed */
				if (ret == -2) {
					zerobits = __bch_bhea_zb();
					printk("ECC level:%d (make %d zero)tolerate:%d zero bits:%d test_end0:%d ==> failed.\n",
							eccbit[t2], t1+1, t1, zerobits, test_end0);
				} else {
					printk("WARNING ALL FF data but correctiable++++++++++++++++++++++++++++\n");
				}
			}
			printk("\n");
			if (test_end0) {
				test_end0 = 0;
				goto test_start;
			}
			printk("******************************************************\n");
		}
	}
	return ret;
}
#endif /* #ifdef BCH_ALLF_TEST */
/*
 * bch_test_case test chip's bch module functions
 * case: will test 4/8/12...64 bit correct for data in 24/256/512/1024 byte
 * func: write data to bch in reg8/reg16/reg32
 * datas: no error/error in data/error in parity/all 0xff data
 * errors: correctiable/uncorrectiable errors
 * */
int bch_test_case(void)
{
	int t1, t2, t3, t4, t5;
	int ret = 0, intr = 1;  /* interrupt when test failed */

	printk("##############################################################################\n");
	printk("Entering bch test case\n");
#ifdef BCH_IRQ
	bch_debug("---USE BCH irq\n");
	nand_bch_irq_init();
#endif

	for (t1=0; t1<ARRAY_SIZE(bch_datalen); t1++) {
		for (t2=0; t2<ARRAY_SIZE(eccbit); t2++) {
			for (t3=0; t3<ARRAY_SIZE(bch_dr_mode); t3++) {
				for (t4=0; t4<ARRAY_SIZE(bch_errpos); t4++) {
					for (t5=0; t5<3; t5++) {
						result[t1][t2][t3][t4][t5] = -1;
					}
				}
			}
		}
	}
	for (t1=0; t1<ARRAY_SIZE(bch_datalen); t1++) {
		for (t2=0; t2<ARRAY_SIZE(eccbit); t2++) {
			for (t3=0; t3<ARRAY_SIZE(bch_dr_mode); t3++) {
				for (t4=0; t4<ARRAY_SIZE(bch_errpos); t4++) {
					printk("++++++++++++++++++++++++++++++++++++++++++++++++++++++\n"
							"Test datalen:%d eccbit:%d bch_dr_mode:%d errpos:%d===>\n",
							bch_datalen[t1], eccbit[t2], bch_dr_mode[t3], bch_errpos[t4]);
					result[t1][t2][t3][t4][0] =
						bch_test(bch_datalen[t1], eccbit[t2], bch_dr_mode[t3], 0, bch_errpos[t4]);
					if (result[t1][t2][t3][t4][0] && intr) {
						goto test_f;
					}
					if (!(t1|t2|t3|t4))
						printk("result[%d][%d][%d][%d][%d]:%d\n", t1, t2, t3, t4, t5, result[0][0][0][0][0]);
					result[t1][t2][t3][t4][1] =
						bch_test(bch_datalen[t1], eccbit[t2], bch_dr_mode[t3], eccbit[t2], bch_errpos[t4]);
					if (result[t1][t2][t3][t4][1] && intr) {
						goto test_f;
					}
					result[t1][t2][t3][t4][2] =
						bch_test(bch_datalen[t1], eccbit[t2], bch_dr_mode[t3], eccbit[t2]+1, bch_errpos[t4]);
					if (result[t1][t2][t3][t4][2] && intr) {
						goto test_f;
					}
				}
			}
		}
	}
	printk("result[%d][%d][%d][%d][%d]:%d\n", t1, t2, t3, t4, t5, result[0][0][0][0][0]);
test_f:
#ifdef BCH_ALLF_TEST
	bch_test_ff();
#endif
#ifdef BCH_IRQ
	/* free irq */
	bch_debug("---free bch irq!\n");
	free_irq(IRQ_BCH, NULL);
#endif
	printk(" =================================BCH TEST RESULT=================================\n");
	for (t1=0; t1<ARRAY_SIZE(bch_datalen); t1++) {
		for (t2=0; t2<ARRAY_SIZE(eccbit); t2++) {
			for (t3=0; t3<ARRAY_SIZE(bch_dr_mode); t3++) {
				for (t4=0; t4<ARRAY_SIZE(bch_errpos); t4++) {
					for (t5=0; t5<3; t5++) {
						if (result[t1][t2][t3][t4][t5]) {
							printk("Test datalen:%d eccbit:%d bch_dr_mode:%d errpos:%d errcount:%d===>result:%d(%s)\n",
									bch_datalen[t1], eccbit[t2], bch_dr_mode[t3], bch_errpos[t4], t5, result[t1][t2][t3][t4][t5], "Failed");
							ret = -1;
							goto endt;
						} else {
							printk("Test datalen:%d eccbit:%d bch_dr_mode:%d errpos:%d errcount:%d===>result:%s\n",
									bch_datalen[t1], eccbit[t2], bch_dr_mode[t3], bch_errpos[t4], t5, "Success");
						}
					}
				}
			}
		}
	}
endt:
	printk("Leaving bch test case\n");
	printk("##############################################################################\n");
	return ret;
}
#endif /* #ifdef BCH_TEST_ENABLE */

/******************************************************************************
 * NEMC TEST FUNCTIONS
 ******************************************************************************/
#define NEMC_TEST_ENABLE 1
#ifdef NEMC_TEST_ENABLE

/*
 * NAND TEST CONFIGRATION
 * */
#define CFG_NAND_16BIT 1
//#define CFG_DUAL_CS 1
//#define CFG_BCH_ENABLE 1
//#defile CFG_MAKE_ERR 1
//#define CFG_SIMPLE 1
//#define CFG_TEST_REG 1
#define CFG_WITH_BCH 1
#ifdef CFG_WITH_BCH
#define BCH_DATA_SIZE	1024
#define BCH_ECC_BIT	8//CONFIG_MTD_HW_BCH_BIT
#define PAR_SIZE	(14*BCH_ECC_BIT/8)
#define ECC_POS	24
#define BCH_DR_MODE	0
#endif

#define NEMC_DEBUG 1
#ifdef NEMC_DEBUG
#define nemc_debug(str,args...)	printk(str,##args)
#else
#define nemc_debug(str,args...)
#endif
#define nemc_debug2(str,args...)

#define NAND_CMD_OFFSET		0x00400000
#define NAND_ADDR_OFFSET	0x00800000
#define NAND_BANK  1
#define NAND_DATAPORT	NEMC_CS1
#define NAND_COMMPORT	(NAND_DATAPORT|NAND_CMD_OFFSET)
#define NAND_ADDRPORT	(NAND_DATAPORT|NAND_ADDR_OFFSET)

#ifndef CFG_NAND_16BIT
#define __nand_data()	REG8(NAND_DATAPORT)
#else
#define __nand_data()	REG16(NAND_DATAPORT)
#endif

#define __nand_cmd(cmd)	(REG8(NAND_COMMPORT) = (cmd))
#define __nand_addr(addr)	(REG8(NAND_ADDRPORT) = (addr))
#define __nand_status(state)	((state) = REG8(NAND_DATAPORT))

#ifndef CONFIG_MTD_NAND_TOGGLE
#define __nand_enable()	(REG_NEMC_NFCSR = NEMC_NFCSR_NFE(NAND_BANK)|NEMC_NFCSR_NFCE(NAND_BANK))
#define __nand_disable()	(REG_NEMC_NFCSR &= ~(NEMC_NFCSR_NFE(NAND_BANK)|NEMC_NFCSR_NFCE(NAND_BANK)))
#else
#define __nand_enable()	__tnand_enable(NAND_BANK)
#define __nand_disable()	__tnand_disable(NAND_BANK)
#endif

#define __snand_dphtd_sync()	while (!(REG_NEMC_TGPD&NEMC_TGPD_DPHTD1))
#define __snand_enable()\
	do {\
		REG_NEMC_NFCSR = NEMC_NFCSR_TNFE1 | NEMC_NFCSR_NFE1;\
		__snand_dphtd_sync();\
		REG_NEMC_NFCSR |= NEMC_NFCSR_NFCE1 | NEMC_NFCSR_DAEC;\
	} while (0)

#define __snand_disable()\
	do {\
		REG_NEMC_NFCSR &= ~NEMC_NFCSR_NFCE1;\
		__snand_dphtd_sync();\
		REG_NEMC_NFCSR = 0;\
	} while (0)

#define pn_enable() \
	do { \
		REG_NEMC_PNCR = 0x3; \
	} while(0)
#define pn_disable() \
	do { \
		REG_NEMC_PNCR = 0x0; \
	} while(0)


#define test_start_blk	5
#define test_blk_num	2
#define test_rw_pnum	128
#define rw_pnum	1
int use_pn = 0;

#ifndef CFG_DUAL_CS
static void __nand_sync(void)
{
	volatile unsigned int timeout = 1200;

	while ((REG_GPIO_PXPIN(0)&0x00100000) && timeout--);
	while (!(REG_GPIO_PXPIN(0)&0x00100000));
}
#else /* for NAND with two chip select */
static void __nand_sync(void)
{
	volatile unsigned int timeout = 200;

	while ((REG_GPIO_PXPIN(0)&0x00800000) && timeout--);
	while (!(REG_GPIO_PXPIN(0)&0x00800000));
}
#endif /* #ifdef CFG_DUAL_CS */

static inline void nand_reset(void)
{
	__nand_enable();

	nemc_debug("--NAND Reset\n");
	__nand_cmd(0xff);
	__nand_sync();

	__nand_disable();
}

static int nand_erase_simple(int pageaddr)
{
	int i, state;

	nemc_debug("%s pageaddr:%d\n", __func__, pageaddr);
	/* erase block to be writed */
	__nand_enable();

	__nand_cmd(0x60);
	for (i=0; i<3; i++) {
		__nand_addr(pageaddr & 0xff);
		pageaddr >>= 8;
	}
	__nand_cmd(0xd0);
	__nand_sync();

	__nand_cmd(0x70);
#ifdef CONFIG_MTD_NAND_TOGGLE
	__tnand_datard_perform();
	__nand_status(i);
#endif
	__nand_status(state);

	return state&0x01?-1:0;
}

#ifdef CFG_SIMPLE 
/*
 * nand_read_simple simple nand read function without kernel's nand driver
 * @data_buf:buf to store data readed
 * @pageaddr:read page address
 * @size:should be length of one page
 * */
static int nand_read_simple(struct mtd_info *mtd, unsigned char *data_buf, int pageaddr, int size)
{
	int i,j;

	__nand_enable();

	__nand_cmd(0x00);
	__nand_addr(mtd->writesize & 0xff);
	__nand_addr((mtd->writesize>>8) & 0xff);
	//__nand_addr(0x00);
	//__nand_addr(0x00);
	for(i=0; i<3; i++) {
		__nand_addr(pageaddr & 0xff);
		pageaddr >>= 8;
	}
	__nand_cmd(0x30);

	__nand_sync();
#ifdef CONFIG_MTD_NAND_TOGGLE
	__tnand_datard_perform();
#endif
	if (use_pn) {
		nemc_debug("Enabling pn...\n");
		pn_enable();
	}
#ifdef CFG_WITH_BCH

#ifndef CFG_NAND_16BIT
	for(i=0; i<mtd->oobsize; i++){
		oobbuf[i] = __nand_data();
	}
#else
	for(i=0; (i<<1)<mtd->oobsize; i++){
		((u16*)oobbuf)[i] = __nand_data();
	}
	mem_dump(oobbuf, 64);
	mem_dump(data_buf, 2048);
	for(i=0; i<size/BCH_DATA_SIZE; i++){
		bch_test_decode(&data_buf[i*BCH_DATA_SIZE], &oobbuf[ECC_POS+i*PAR_SIZE],
			   	BCH_DATA_SIZE, BCH_ECC_BIT, BCH_DR_MODE);
	}
#endif
	__nand_cmd(0x05);
	__nand_addr(0x00);
	__nand_addr(0x00);
	__nand_cmd(0xE0);
	__nand_sync();
#ifndef CFG_NAND_16BIT
	for(i=0; i<size/BCH_DATA_SIZE; i++){
		for(j=0; j<BCH_DATA_SIZE; j++)
			data_buf[i*BCH_DATA_SIZE+j] = __nand_data();
	}
#else
	for(i=0; i<size/BCH_DATA_SIZE; i++){
		for(j=0; (j<<1)<BCH_DATA_SIZE; j++)
			((u16 *)data_buf)[i*BCH_DATA_SIZE>>1+j] = __nand_data();
	}
#endif
#else
#ifndef CFG_NAND_16BIT
	for(i=0; i<size/1024; i++){
		for(j=0; j<1024; j++)
			data_buf[i*1024+j] = __nand_data();
	}
#else
	for(i=0; i<size/1024/2; i++){
		for(j=0; (j<<1)<1024; j++)
			((u16*)data_buf)[i*1024>>1+j] = __nand_data();
	}
#endif
#endif
	if (use_pn)
		pn_disable();

	__nand_disable();
	return 0;
}

/*
 * nand_write_simple simple nand write function without kernel's nand driver
 * @data_buf:buf to write
 * @pageaddr:page address
 * @size:should be size of one page
 * */
static int nand_write_simple(struct mtd_info *mtd ,unsigned char *data_buf, int pageaddr, int size)
{
	int i, j, state, num_err;
#if 0
	int blk = (pageaddr/128)*128;

	/* erase block to be writed */
	__nand_enable();
	__nand_cmd(0x60);
	for (i=0; i<3; i++) {
		__nand_addr(blk & 0xff);
		blk >>= 8;
	}
	__nand_cmd(0xd0);
	__nand_sync();
	__nand_disable();
#endif

	/* send write cmd */
	__nand_enable();

	__nand_cmd(0x80);
	__nand_addr(0x0);
	__nand_addr(0x0);
	for (i=0; i<3; i++) {
		__nand_addr(pageaddr & 0xff);
		pageaddr >>= 8;
	}
#ifdef CONFIG_MTD_NAND_TOGGLE
	__tnand_datawr_perform(NAND_BANK);
#endif
	if (use_pn) {
		nemc_debug("Enabling pn...\n");
		pn_enable();
	}
#ifdef CFG_WITH_BCH
	for (i=0; i<ECC_POS; i++) {
		oobbuf[i] = 0xFF;
	}
	for(i=0; i<size/BCH_DATA_SIZE; i++){
		bch_test_encode(&data_buf[i*BCH_DATA_SIZE], &oobbuf[ECC_POS+i*PAR_SIZE],
			   	BCH_DATA_SIZE, BCH_ECC_BIT, BCH_DR_MODE);
#ifdef CFG_MAKE_ERR
		/* make some error bits */
		num_err = get_random_range(0, BCH_ECC_BIT+2, 0, NULL);
		nemc_debug("%s make %d error bits\n", __func__, num_err);
		for (j=0; j<num_err; j++) {
			bch_err_check[j] = get_random_range(0, BCH_DATA_SIZE*8, j, bch_err_check);
			nemc_debug2("%s error bit(%d):%d\n", __func__, j, bch_err_check[j]);
			data_buf[i*BCH_DATA_SIZE+bch_err_check[j]/8] ^= 0x1<<(bch_err_check[j]%8);
		}
#endif
#ifndef CFG_NAND_16BIT
		for(j=0; j<BCH_DATA_SIZE; j++) {
		for(j=0; j<BCH_DATA_SIZE; j++) {
			__nand_data() = data_buf[i*BCH_DATA_SIZE+j];
		}
#else
		for(j=0; (j<<1)<BCH_DATA_SIZE; j++) {
			__nand_data() = ((u16*)data_buf)[i*BCH_DATA_SIZE>>1+j];
		}
#endif
	}
#ifndef CFG_NAND_16BIT
	for (i=0; i<mtd->oobsize; i++) {
		__nand_data() = oobbuf[i];
	}
#else
	for (i=0; (i<<1)<mtd->oobsize; i++) {
		__nand_data() = ((u16 *)oobbuf)[i];
	}
#endif
#else
	for(i=0; i<size/1024; i++){
#ifndef CFG_NAND_16BIT
		for(j=0; j<1024; j++) {
			__nand_data() = data_buf[i*1024+j];
		}
#else
		for(j=0; (j<<1)<1024; j++) {
			__nand_data() = ((u16 *)data_buf)[i*1024>>1+j];
		}
#endif
	}
#endif
	if (use_pn)
		pn_disable();

#ifdef CONFIG_MTD_NAND_TOGGLE
	__tnand_fce_clear(NAND_BANK);
	__tnand_fce_set(NAND_BANK);
#endif
	__nand_cmd(0x10);
	__nand_sync();
	__nand_cmd(0x70);

#ifdef CONFIG_MTD_NAND_TOGGLE
	__tnand_datard_perform();
	__nand_status(i);
#endif
	__nand_status(state);

	__nand_disable();

	return state&0x01?-1:0;
}
#endif /* #ifdef CFG_SIMPLE */


static int nand_test_read(struct mtd_info *mtd, int pageaddr, uint8_t *buf, int num)
{
#ifdef CFG_SIMPLE
	int i, ret = 0;

	nemc_debug("%s:L%d, page = %d, num:%d\n", __FUNCTION__, __LINE__, pageaddr, num);
	for (i=0; i<num; i++) {
		ret = nand_read_simple(mtd, buf+i*mtd->writesize, pageaddr+i, mtd->writesize);
		if (ret) {
			nemc_debug("Read failed, pageaddr:%d\n", pageaddr + i);
			break;
		}
	}
	return ret;
#else
	int pagesize = mtd->writesize;
	loff_mtd_t from = pageaddr*pagesize;
	size_mtd_t len = num*pagesize;
	size_mtd_t retlen = 0;
	nemc_debug("%s:L%d, page = %d\n", __FUNCTION__, __LINE__, pageaddr);

	DEBG;
	return mtd->read(mtd, from, len, &retlen, buf);
#endif
}

static int nand_test_write(struct mtd_info *mtd, int pageaddr, uint8_t *buf, int num)
{
#ifdef CFG_SIMPLE
	int i, ret = 0;

	nemc_debug("%s:L%d, page = %d, num:%d\n", __FUNCTION__, __LINE__, pageaddr, num);
	for (i=0; i<num; i++) {
		ret = nand_write_simple(mtd, buf+i*mtd->writesize, pageaddr+i, mtd->writesize);
		if (ret) {
			nemc_debug("Programe failed, pageaddr:%d\n", pageaddr + i);
			break;
		}
	}
	return ret;
#else
	int pagesize = mtd->writesize;
	loff_mtd_t to = pageaddr*pagesize;
	size_mtd_t len = num*pagesize;
	size_mtd_t retlen = 0;
	int ret = 0;

	nemc_debug("%s:L%d, page = %d\n", __FUNCTION__, __LINE__, pageaddr);

	ret = mtd->write(mtd, to, len, &retlen, buf);
	nemc_debug("%s write complete, ret:%d, retlen:%d\n", __func__, ret, (int)retlen);
	return ret;
#endif
}

/*
 * nand_test_erase erase nand blocks
 * mtd:
 * pageaddr:erase start address
 * num:erase block number
 * */
static int nand_test_erase(struct mtd_info *mtd, int pageaddr, int num)
{
#if 1//def CFG_SIMPLE
	int i, ret = 0;
	int ppb = mtd->erasesize / mtd->writesize;

	pageaddr = (pageaddr / ppb) * ppb;
	nemc_debug("%s:L%d, page = %d, num:%d\n", __FUNCTION__, __LINE__, pageaddr, num);
	for (i=0; i<num; i++) {
		ret = nand_erase_simple(pageaddr + i * ppb);
		if (ret) {
			nemc_debug("Erase failed, pageaddr:%d\n", pageaddr + i * ppb);
			break;
		}
	}
	return ret;
#else
	struct erase_info erase;
	int ppb = mtd->erasesize/mtd->writesize;
	int blk = pageaddr/ppb;

	memset(&erase, 0, sizeof(struct erase_info));
	erase.mtd = mtd;
	erase.len = mtd->erasesize*num;
	erase.addr = blk*mtd->erasesize;
	nemc_debug("%s:L%d, pageaddr = %d, addr=%llu\n", __FUNCTION__, __LINE__, pageaddr, erase.addr);

	return mtd->erase(mtd, &erase);
#endif
}

static void dump_nand_info(struct mtd_info *mtd)
{
	printk("\n[NandInfo]\n");
	printk("size:		%llu(%d MB)\n", mtd->size, (int)(mtd->size/1024/1024));
	printk("erasesize:	%u(%d KB)\n", mtd->erasesize, mtd->erasesize/1024);
	printk("writesize:	%u(%d KB)\n", mtd->writesize, mtd->writesize/1024);
	printk("oobsize:	%d\n", mtd->oobsize);
	return;
}

#ifdef CFG_TEST_REG
#define REG_NEMC_SMCR(n)	REG32(NEMC_SMCR1+((n)-1)*0x4)
#define REG_NEMC_TGCR(n)	REG32(NEMC_TGCR1+((n)-1)*0x4)
static void nand_test_reg(void)
{
	int i;
	u32 reg;

	for (i = 1; i<=6; i++) {
		reg = REG_NEMC_SMCR(i);
		printk("-->REG_NEMC_SMCR%d:%08x set to 0x3FFFFFC7 and 0x0\n", i, REG_NEMC_SMCR(i));
		REG_NEMC_SMCR(i)=0xffffffff;
		printk("---SMCR%d:%08x\n", i, REG_NEMC_SMCR(i));
		REG_NEMC_SMCR(i) = 0x0;
		printk("---SMCR%d:%08x\n", i, REG_NEMC_SMCR(i));
		REG_NEMC_SMCR(i) = reg;
	}

	for (i=1; i<=6; i++) {
		reg = REG_NEMC_NFCSR;
		printk("\n-->REG_NEMC_NFCSR:%08x set to 0x003f0fff\n", REG_NEMC_NFCSR);
		REG_NEMC_NFCSR = NEMC_NFCSR_NFCE(i)|NEMC_NFCSR_NFE(i)|NEMC_NFCSR_TNFE(i);
		printk("---NFCSR%d:%08x\n", i, REG_NEMC_NFCSR);
		REG_NEMC_NFCSR &= ~(NEMC_NFCSR_NFCE(i)|NEMC_NFCSR_NFE(i)|NEMC_NFCSR_TNFE(i));
		printk("---NFCSR%d:%08x\n", i, REG_NEMC_NFCSR);
		REG_NEMC_NFCSR = reg;
	}

	for (i=1; i<=6; i++) {
		reg = REG_NEMC_TGCR(i);
		printk("-->REG_NEMC_TGCR%d:%08x\n", i, REG_NEMC_TGCR(i));
		REG_NEMC_TGCR(i) = 0xffffffff;
		printk("---TGCR%d:%08x\n", i, REG_NEMC_TGCR(i));
		REG_NEMC_TGCR(i) = 0xffffffff;
		printk("---TGCR%d:%08x\n", i, REG_NEMC_TGCR(i));
		REG_NEMC_TGCR(i) = reg;
	}
}
#endif /* CFG_TEST_REG */

static inline void dump_nemc_reg(void)
{
	printk("---REG_NEMC_SACR1: 0x%08x\n", REG_NEMC_SACR1);
	printk("---REG_NEMC_NFCSR: 0x%08x\n", REG_NEMC_NFCSR);
	printk("---REG_NEMC_PNCR:  0x%08x\n", REG_NEMC_PNCR);
	printk("---REG_NEMC_PNDR:  0x%08x\n", REG_NEMC_PNDR);
	printk("---REG_NEMC_BITCNT:0x%08x\n", REG_NEMC_BITCNT);
	printk("---REG_NEMC_SMCR1: 0x%08x\n", REG_NEMC_SMCR1);
	printk("---REG_NEMC_TGWE:  0x%08x\n", REG_NEMC_TGWE);
	printk("---REG_NEMC_TGCR1: 0x%08x\n", REG_NEMC_TGCR1);
	printk("---REG_NEMC_TGSR:  0x%08x\n", REG_NEMC_TGSR);
	printk("---REG_NEMC_TGFL:  0x%08x\n", REG_NEMC_TGFL);
	printk("---REG_NEMC_TGCL:  0x%08x\n", REG_NEMC_TGCL);
	printk("---REG_NEMC_TGPD:  0x%08x\n", REG_NEMC_TGPD);
	printk("---REG_NEMC_TGSL:  0x%08x\n", REG_NEMC_TGSL);
	printk("---REG_NEMC_TGRR:  0x%08x\n", REG_NEMC_TGRR);
	printk("---REG_NEMC_TGDR:  0x%08x\n", REG_NEMC_TGDR);
}

//TEST_SELECT
int nemc_test_case(struct mtd_info *mtd)
{
	int ppb = mtd->erasesize / mtd->writesize;
	int rw_size = rw_pnum * mtd->writesize;
	int i, j, ret;
	int start;

	printk("##############################################################################\n");
	dump_nand_info(mtd);
	printk("Entering NEMC TEST routine...\n");
#ifdef CFG_TEST_REG
	printk("============================== NEMC REG TEST ==============================\n");
	nand_test_reg();
	serial_getc();
#endif
	printk("============================== NEMC NAND TEST ==============================\n");
	use_pn = 0;
with_pn:
	nemc_debug("Write and read nand page with data:\n");

#if 0
	REG_NEMC_SMCR1 = 0x0FFF770A;
	REG_NEMC_TGCR1 = 0x0FF8770A;
	__tnand_dqsdelay_probe();
	if (__tnand_dqsdelay_checkerr()) {
		printk("Toggle nand dqsdelay detect failed, set to default:0x9.\n");
		__tnand_dqsdelay_init(0x9);
	}
#endif

#if 0 /* test only one page */
#if 1 /* erase,write and read */
	/* Test erase functions */
	nemc_debug("Erasing block start %d num %d...\n", test_start_blk, test_blk_num);
	ret = nand_test_erase(mtd, ppb*test_start_blk, test_blk_num);
	if (ret) {
		printk("WARNING!!! NAND ERASE ERROR");
		goto error;
	}

	while (1) {
		printk("Press any key to program.\n");
		serial_getc();
		buf_init(databuf, rw_size);
		ret = nand_test_write(mtd, test_start_blk*ppb, databuf, rw_pnum);
		if (ret) {
			printk("WARNING!!! NAND WRITE ERROR");
		}
#else  /* only read */
	while (1) {
#endif
		buf_init(databuf, rw_size);
		printk("Press any key to read.\n");
		serial_getc();
		ret = nand_test_read(mtd, test_start_blk*ppb, tempbuf, rw_pnum);
		if (ret) {
			printk("WARNING!!! NAND READ ERROR");
			printk("Data writed in:\n");
			//mem_dump(databuf, 512);
		}
		buf_check(databuf, tempbuf, rw_size);
		mem_dump(tempbuf, 512);
	}
	while (1);
#endif

	nemc_debug("Erasing block start %d num %d...\n", test_start_blk, test_blk_num);
	ret = nand_test_erase(mtd, ppb*test_start_blk, test_blk_num);
	if (ret) {
		printk("WARNING!!! NAND ERASE ERROR");
		goto error;
	}
	for (i=test_start_blk; i<test_start_blk+test_blk_num; i++) {
		start = 0;//= get_random_range(0, ppb-test_rw_pnum, 0, NULL);
		//for (j=start; j<start+test_rw_pnum; j+=rw_pnum) {
		for (j=start; j<ppb; j+=rw_pnum) {
			nemc_debug("Writing page %d(%d) in block %d withdata...\n", j, ppb, i);
			//buf_init_random(databuf, rw_size);
			buf_init(databuf, rw_size);

			printk("Press any key to program.\n");
			//serial_getc();
			ret = nand_test_write(mtd, i*ppb+j, databuf, rw_pnum);
			if (ret) {
				printk("WARNING!!! NAND WRITE ERROR");
				goto error;
			}
			buf_init(databuf, rw_size);

			ret = nand_test_read(mtd, i*ppb+j, tempbuf, rw_pnum);
#if 1 //ndef CFG_SIMPLE   /* if CFG_SIMPLE, always check the buf */
			if (ret) {
				printk("WARNING!!! NAND READ ERROR");
#endif
				printk("Data writed in:\n");
				mem_dump(databuf, 256);
				printk("Data read out:\n");
				mem_dump(tempbuf, 256);
#if 1//ndef CFG_SIMPLE
				goto error;
			}
			buf_check(databuf, tempbuf, rw_size);
#endif
			printk("\n\n");
		}
	}
	/* with pn */
	use_pn = 1;
	if (!use_pn) {
		use_pn = 1;
		nemc_debug("***********************************************************==>");
		nemc_debug("will test with use_pn...\n");
		goto with_pn;
	}
error:
	printk("Leaving NEMC TEST routine...\n");
	printk("##############################################################################\n");

	return ret;
}
#endif /* #ifdef NEMC_TEST_ENABLE */


#define PAGESIZE	mtd->writesize
#define OOBBUF_SIZE	(1024)
#define DATABUF_SIZE	PAGESIZE
int nemc_bch_test(struct mtd_info *mtd)
{
	int ret = 0;
	/* get memories */
	databuf = kmalloc(DATABUF_SIZE, GFP_KERNEL);
	if (databuf == NULL) {
		printk("%s:kmalloc failed databuf\n", __FUNCTION__);
		return -1;
	}

	tempbuf = kmalloc(DATABUF_SIZE, GFP_KERNEL);
	if (tempbuf == NULL) {
		printk("%s:kmalloc failed tempbuf\n", __FUNCTION__);
		ret = -1;
		goto err3;
	}

	oobbuf = kmalloc(OOBBUF_SIZE, GFP_KERNEL);
	if (oobbuf == NULL) {
		printk("%s:kmalloc failed oobbuf\n", __FUNCTION__);
		ret = -1;
		goto err2;
	}
	tempbuf2 = kmalloc(OOBBUF_SIZE, GFP_KERNEL);
	if (tempbuf2 == NULL) {
		printk("%s:kmalloc failed tempbuf2\n", __FUNCTION__);
		ret = -1;
		goto err1;
	}

#ifdef BCH_TEST_ENABLE
	/* test bch functions */
	ret = bch_test_case();
	if (ret) {
		printk("WARNING!!! BCH TEST FAILED!!!\n");
		goto err0;
	}
	/* wait for user's confirm */
	serial_getc();
#endif
#ifdef NEMC_TEST_ENABLE
	/* test nemc functions */
	ret = nemc_test_case(mtd);
	if (ret) {
		printk("WARNING!!! NEMC TEST FAILED!!!\n");
		goto err0;
	}
	serial_getc();
#endif
	printk("Congratulations, All test passed\n");
	serial_getc();

err0:
	kfree(tempbuf2);
err1:
err2:
	kfree(tempbuf);
err3:
	kfree(databuf);
	return 0;
}
EXPORT_SYMBOL(nemc_bch_test);
