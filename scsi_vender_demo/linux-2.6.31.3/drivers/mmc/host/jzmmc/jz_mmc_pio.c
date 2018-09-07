#include <linux/mmc/core.h>
#include <linux/mmc/host.h>
#include <linux/scatterlist.h>
#include <linux/kthread.h>
#include <asm/jzsoc.h>
#include "include/jz_mmc_dma.h"
#include "include/jz_mmc_host.h"

/* FIXME: how to decide the max timeout */
#if defined(CONFIG_FPGA)
#define WAIT_FIFO_MAX_TIMEOUT	0xe0000
#else
//extern unsigned long loops_per_jiffy; /* 10ms */
//#define WAIT_FIFO_MAX_TIMEOUT	(loops_per_jiffy * 10)
#define WAIT_FIFO_MAX_TIMEOUT	0xffffffff
#endif
 
static int jz_mmc_pio_read(struct jz_mmc_host *host, unsigned char *buf, size_t len) {
	unsigned int *buf_v = (unsigned int *)buf;
	size_t len_v = (len >> 2);

	unsigned char *buf_o = buf + (len & (~0x3));
	size_t len_o = len & 0x3;
	size_t i = 0;
	unsigned int timeout;

	for (i = 0; i < len_v; i++) {
		timeout = WAIT_FIFO_MAX_TIMEOUT;
		while (timeout) {
			if(!(REG_MSC_STAT(host->pdev_id) & (MSC_STAT_DATA_FIFO_EMPTY))){
				break;
			}
			else if((REG_MSC_STAT(host->pdev_id) & (MSC_STAT_TIME_OUT_READ))){
				printk("read timeout\n");
				return -ETIMEDOUT;
			}
			else if((REG_MSC_STAT(host->pdev_id) & (MSC_STAT_CRC_READ_ERROR))){
				printk("read crc\n");
				return -ETIMEDOUT;
			}
			else if(host->eject) {
				printk("card eject\n");
				return -ENOMEDIUM;
			}
			udelay(100);
			timeout--;
			/* printk("===> stat:%x\n",REG_MSC_STAT(host->pdev_id)); */
			/* printk("   > rtcnt:%x\n",REG_MSC_RTCNT(host->pdev_id)); */
			/* printk("   > debug:%x at %x\n",REG_MSC_DEBUG(host->pdev_id), MSC_DEBUG(host->pdev_id)); */
			/* printk("   > timeout = %d\n", timeout); */
		}

		if (!timeout) {
			printk("wait fifo not empty timedout!\n");
			return -ETIMEDOUT;
		}

		*buf_v = REG_MSC_RXFIFO(host->pdev_id);
		buf_v++;
	}

	if (len_o) {
		unsigned int temp = 0;
		unsigned char *c = (unsigned char *)&temp;

		timeout = WAIT_FIFO_MAX_TIMEOUT;
		while (timeout) {
			if(!(REG_MSC_STAT(host->pdev_id) & (MSC_STAT_DATA_FIFO_EMPTY))){
				break;
			}
			else if((REG_MSC_STAT(host->pdev_id) & (MSC_STAT_TIME_OUT_READ))){
				printk("read timeout\n");
				return -ETIMEDOUT;
			}
			else if((REG_MSC_STAT(host->pdev_id) & (MSC_STAT_CRC_READ_ERROR))){
				printk("read crc\n");
				return -ETIMEDOUT;
			}
			else if(host->eject) {
				printk("card eject\n");
				return -ENOMEDIUM;
			}
			udelay(100);
			timeout--;
		}

		if (!timeout) {
			printk("wait fifo not empty timedout!\n");
			return -ETIMEDOUT;
		}
		temp = REG_MSC_RXFIFO(host->pdev_id);

		for (i = 0; i < len_o; i++)
			buf_o[i] = c[i];
	}

	return 0;
}

static int jz_mmc_pio_write(struct jz_mmc_host *host, unsigned char *buf, size_t len) {
	unsigned int *buf_v = (unsigned int *)buf;
	size_t len_v = (len >> 2);

	unsigned char *buf_o = buf + (len & (~0x3));
	size_t len_o = (len & 0x3);

	size_t i = 0;
	unsigned int timeout;

	for (i = 0; i < len_v; i++) {
		timeout = WAIT_FIFO_MAX_TIMEOUT;
		while (timeout && ((REG_MSC_STAT(host->pdev_id) & MSC_STAT_DATA_FIFO_FULL) && !host->eject)) {
			udelay(10);
			timeout --;
		}

		if (!timeout) {
			printk("wait fifo not full timedout.\n");
			return -ETIMEDOUT;
		}

		if (host->eject) {
			printk("card ejected.\n");
			return -ENOMEDIUM;
		}

		REG_MSC_TXFIFO(host->pdev_id) = *buf_v;
		buf_v++;
	}

	if (len_o) {
		unsigned int temp;
		unsigned char *c = (unsigned char *)&temp;

		for (i = 0; i < len_o; i++)
			c[i] = buf_o[i];

		timeout = WAIT_FIFO_MAX_TIMEOUT;
		while (timeout && ((REG_MSC_STAT(host->pdev_id) & MSC_STAT_DATA_FIFO_FULL) && !host->eject)) {
			udelay(10);
			timeout --;
		}

		if (!timeout) {
			printk("%d, wait fifo not full timedout.\n", __LINE__);
			return -ETIMEDOUT;
		}

		if (host->eject) {
			printk("card ejected.\n");
			return -ENOMEDIUM;
		}

		REG_MSC_TXFIFO(host->pdev_id) = temp;;
	}

	return 0;
}

void jz_mmc_stop_pio(struct jz_mmc_host *host) {
	int pri = 1;

	while(!host->transfer_end) {
		if (pri) {
			printk("===> wait transfer not end... \n");
			pri = 0;
		}
	}
}

static int jz_mmc_data_transfer(void *arg) {
	struct jz_mmc_host *host = (struct jz_mmc_host *)arg;
	struct mmc_data *data = host->curr_mrq->data;
	int is_write = data->flags & MMC_DATA_WRITE;
	struct scatterlist *sgentry = NULL;
	unsigned char *buf = NULL;
	int ret = 0, len = 0, i = 0;

	host->transfer_mode = JZ_TRANS_MODE_PIO;

	/* For badly aligned access */
	for_each_sg(data->sg, sgentry, data->sg_len, i) {
		buf = sg_virt(sgentry);
		len = sg_dma_len(sgentry);
//		printk("*** buf = %p, len = %d ***\n", buf, len);

		if (is_write)
			ret = jz_mmc_pio_write(host, buf, len);
		else
			ret = jz_mmc_pio_read(host, buf, len);
		if (ret) {
			data->error = ret;
			break;
		}
	}

	if (is_write) {
		if (ret) {
			host->data_ack = 0;
			wake_up_interruptible(&host->data_wait_queue);
		}
		/* else, DATA_TRANS_DONE interrupt will raise */
	} else {
		if (!ret)
			host->data_ack = 1;
		else
			host->data_ack = 0;
		wake_up_interruptible(&host->data_wait_queue);
	}

	host->transfer_end = 1;

	return 0;
}

void jz_mmc_start_pio(struct jz_mmc_host *host) {
	host->cmdat &= ~(MSC_CMDAT_DATA_EN);
	host->transfer_end = 0;
	kthread_run(jz_mmc_data_transfer, (void *)host, "msc pio transfer");
}
