#include <linux/utsname.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/sched.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <asm/io.h>
#include <asm/system.h>
#include <asm/uaccess.h>
#include <linux/poll.h>
#include <linux/irq.h>
#include <asm/irq.h>
#include <linux/interrupt.h>
#include <linux/gpio.h>
#include <linux/device.h>

#include <asm/jzsoc.h>
#include "ic_id_card.h"

#define IC_ID_DEBUG
#undef IC_ID_DEBUG

#if defined(IC_ID_DEBUG)
#define ic_id_debug(fmt, ...) \
	printk(KERN_ALERT fmt, ##__VA_ARGS__)
#else
#define ic_id_debug(fmt, ...) \
	({ if (0) printk(KERN_ALERT fmt, ##__VA_ARGS__); 0; })
#endif

#define IC_ID_MAJOR	0

#define UART_NUM 0

static int uart_puts(char *str, int len);

// ************************************************************************
#define		CMD_SET_ANNTENA			0x010C
#define		CMD_REQUEST_CARD		0x0201
#define		CMD_ANTI_COLLISION		0x0202
#define		CMD_SELECT				0x0203

#define Uint32	unsigned int
#define Uint8	unsigned char


struct ic_id_dev
{
	struct cdev cdev;
	dev_t devno;

	unsigned char rx_buffer[64];
	unsigned int rx_cnt;
};


static struct ic_id_dev *ic_id_devp;
/*
	drvICcmd
		Uint32 nodeid
		int funcode
		Uint8 *data
		Uint32 datalen
	return:
		int		<0 indicates an error
*/
static int drvICcmd(Uint32 nodeid, int funcode, Uint8 *data, Uint32 datalen)
{
	int		i;
	int err;
	Uint8	xor = (char)0;
	Uint8	tmpbuf[32];

	tmpbuf[0] = (Uint8)0xAA;
	tmpbuf[1] = (Uint8)0xBB;
	tmpbuf[2] = (Uint8)((datalen + 5) & 0xFF);
	tmpbuf[3] = (Uint8)((datalen + 5) >> 8);
	tmpbuf[4] = (Uint8)(nodeid & 0xFF);
	tmpbuf[5] = (Uint8)(nodeid >>8);
	tmpbuf[6] = (Uint8)(funcode & 0xFF);
	tmpbuf[7] = (Uint8)(funcode >> 8);

	for(i=0;i<datalen;i++)
	{
		tmpbuf[8+i] = *data++;
	}

	for (i = 0; i < (datalen + 4); i ++)
	{
		xor ^= tmpbuf[i + 4];
	}

	tmpbuf[datalen + 8] = xor;

	err = uart_puts(tmpbuf, datalen + 9);
	
	return err;
}

/*
	drvICInit
		void
	return
		int		0: OK
				-1: set anntena failed
				-2: request card failed
*/
static int drvICInit(void)
{
	int err;
	Uint8	buf[4];

	buf[0] = 1;
	err = drvICcmd(0, CMD_SET_ANNTENA, buf, 1);
	if(err < 0)
	{
		printk("rfcard cmd error:CMD_SET_ANNTENA\n");
		return -1;
	}
	
	schedule_timeout_interruptible(5);
	
	buf[0] = 0x52;
	err = drvICcmd(0, CMD_REQUEST_CARD, buf, 1);
	if(err < 0)
	{
		printk("rfcard cmd error:CMD_REQUEST_CARD\n");
		return -2;
	}

	return 0;
}

static int drvICReceiveCardNum_Cmd(void)
{
	int err;
	Uint8	buf[4];

	buf[0] = 0x52;
	err = drvICcmd(0, CMD_REQUEST_CARD, buf, 1);
	if(err < 0)
	{
		printk("RF Card CMD_REQUEST_CARD CMD ERROR\n!");
		return -2;
	}
	
	schedule_timeout_interruptible(2);
	
	err = drvICcmd(0, CMD_ANTI_COLLISION, 0, 0);
	if(err < 0)
	{
		printk("RF Card CMD_ANTI_COLLISION CMD ERROR\n!");
		return -3;
	}
	
	schedule_timeout_interruptible(20);
	

	return 0;
}  
// ************************************************************************//

static int ic_id_open(struct inode *inode, struct file *filp);
static int ic_id_release(struct inode *inode, struct file *filp);
//static ssize_t ic_id_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
static int ic_id_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg);
//static unsigned int ic_id_poll(struct file *filp, poll_table *wait);

static struct file_operations ic_id_fops = {
	.owner = THIS_MODULE,
	.open = ic_id_open,
	.release = ic_id_release,
	.ioctl = ic_id_ioctl,
	//.poll = ic_id_poll,
};

/*	
	uart_putchar
	char send:	ch to send via uart
	return: int		 0: ok
					-1: time out err
*/
static int uart_putchar(char send)
{
	unsigned int time_out = 0;
	int status = 1;
	do{
		status = __uart_transmit_end(UART_NUM);
		time_out++;
		if(time_out > 0x003fffff)
		{
			break;
		}
	}while(0 == status);
	if(time_out > 0x003fffff)
	{
		printk("\n__uart_transmit_ timeout!!!\n\n");
		return -1;
	}
	__uart_transmit_char(UART_NUM, send);
	return 0;
}

/*
	uart_puts
		char *str:	datas to be send
		int len:	indicate how many datas
	return: 
		int		<0 	indicates an error
				>0	return how many data has send
*/
static int uart_puts(char *str, int len)
{
	int err;
	int i;
	for(i=0;i<len;i++)
	{
		err = uart_putchar(*str++);
		if(err)
		{
			return err;
		}
	}
	return i;
}

static irqreturn_t ic_id_interrupt_handler(int irq, void *dev_id);
static int get_card_id(unsigned int * data);

static int ic_id_open(struct inode *inode, struct file *filp)
{
	int err, retry = 5;
	filp->private_data = ic_id_devp;

	__uart_disable(UART_NUM);
	__gpio_as_uart0();
	__uart_disable_transmit_irq(UART_NUM);
	__uart_disable_loopback(UART_NUM);
	__uart_set_8n1(UART_NUM);
	__uart_set_baud(UART_NUM, 24000000, 19200);		// modify, yjt, 20130722, 12M -> 24M
	__uart_enable(UART_NUM);
	
drvICInit_retry:
	err = drvICInit();
	if(err)
	{
		retry--;
		if(retry)
		{
			goto drvICInit_retry;
		}
	}
	
	return err;
}

static int ic_id_release(struct inode *inode, struct file *filp)
{	
	__uart_disable_receive_irq(UART_NUM);
	__uart_disable(UART_NUM);
	free_irq(IRQ_UART0, NULL);
	ic_id_debug("ic_id_release: device closed\n");
	return 0;
}
static int Uart_RX_Enable_Status =0; 
static int ic_id_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
	unsigned int card_num = 0;
	switch(cmd)
	{
	case IC_ID_CARD_REQUEST_CARD_START:		
		__uart_enable_receive_irq(UART_NUM);
		Uart_RX_Enable_Status = 1;
		break;
	case IC_ID_CARD_REQUEST_CARD_STOP:
		__uart_disable_receive_irq(UART_NUM);
		Uart_RX_Enable_Status = 0;
		break;
	case IC_ID_CARD_GET_CARD_ID:

		if(!Uart_RX_Enable_Status)
		{
			printk("Uart recv is disable!!!\n");
			break;
		}
		if(!get_card_id(&card_num))
			put_user(card_num,(unsigned long *)arg);
			
		break;

	default:
		return -EFAULT;
	}
	return 0;
}
/*
static unsigned int ic_id_poll(struct file *filp, poll_table *wait)
{
	struct ic_id_dev *dev = (struct ic_id_dev *)filp->private_data;
	unsigned int mask = 0;

	
	return mask;
}
*/
static irqreturn_t ic_id_interrupt_handler(int irq, void *dev_id)
{

	ic_id_devp->rx_buffer[ic_id_devp->rx_cnt % 0x3F] = __uart_receive_char(UART_NUM);
	ic_id_devp->rx_cnt += 1;
	

	return IRQ_RETVAL(IRQ_HANDLED);
}


static int get_card_id(unsigned int * data)
{
	unsigned char *pt = ic_id_devp->rx_buffer;
	int i = 0;
	Uint8	buf[4];
	int index;
	int ret = 0;

  memset(pt,0xff,64);
  ic_id_devp->rx_cnt = 0;

	if(drvICReceiveCardNum_Cmd() < 0)
	{
				printk("Get RFCardNum error!!\n");
				return -1;
	}

	if((pt[18] == 0x02) && (pt[19] == 0x02) && (pt[20] == 0x00))	// it is a legal card
	{
		//num = (pt[9] << 24) | (pt[10] << 16) | (pt[11] << 8) | pt[12];			
		index = 21;
			
		for(i = 0;i < 4;i ++)
		{
			buf[i] = pt[index];
			if((buf[i] == 0xaa) && (pt[index + 1] == 0x00))
				index += 2;
			else
				index += 1;
		}
	  *data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];	
	  //if(0x16A4AE00 != *data)
	  //ic_id_debug("\nget a new card: 0x%08X\n", *data);
	  
	  ret = 0;

	}
	else
		ret = -1;	
		 
  return ret;
}

struct class *ic_id_card_class;

static __init int ic_id_init(void)
{
	int err;

	dev_t devno = MKDEV(IC_ID_MAJOR, 0);
	
	if(IC_ID_MAJOR)
	{
		err = register_chrdev_region(devno, 1, "ic_id_card");	
	}
	else
	{
		err = alloc_chrdev_region(&devno, 0, 1, "ic_id_card");	
	}
	if(err < 0)
	{
		ic_id_debug("alloc dev number failed.\n");
		return err;	
	}
	
	ic_id_debug("ic_id_init: alloc ic_id_devp mem\n");
	
	ic_id_devp = kmalloc(sizeof(struct ic_id_dev), GFP_KERNEL);
	if(NULL == ic_id_devp)
	{
		err = -ENOMEM;
		ic_id_debug("ic_id_init: alloc mem failed\n");
		goto alloc_mem_fail;	
	}
	memset(ic_id_devp, 0, sizeof(struct ic_id_dev));
	
	ic_id_devp->devno = devno;

	//setup cdev
	cdev_init(&ic_id_devp->cdev, &ic_id_fops);
	ic_id_devp->cdev.owner = ic_id_fops.owner;

	err = cdev_add(&ic_id_devp->cdev, ic_id_devp->devno, 1);
	
	if(err < 0)
	{
		ic_id_debug("ic_id_init: failed to add cdev\n");
		goto setup_cdev_fail;
	}
	else
	{
		ic_id_debug("ic_id_init: add cdev ok, devno: %d, %d\n", MAJOR(ic_id_devp->devno), MINOR(ic_id_devp->devno));
	}


	// enable uart clock, after hw reset, all UARTs are in disable status, and clock for UARTs is also disabled
	cpm_start_clock(CGM_UART0);

	ic_id_card_class = class_create(THIS_MODULE, "ic_id_card_class");
	if(IS_ERR(ic_id_card_class))
	{
		goto setup_cdev_fail;
	}
	device_create(ic_id_card_class, NULL, ic_id_devp->devno, "ic_id_card", "ic_id_card");
	
	ic_id_debug("ic_id_init: all done\n");

#if 0
	err = request_irq(IRQ_UART0, ic_id_interrupt_handler,IRQF_DISABLED, "ic_id_card", NULL);
	if(err != 0)
	{
		ic_id_debug("%s: request irq failed and return-value is:%d\n", __FUNCTION__,err);		
		return -EFAULT;
	}
	else
	{
		__uart_enable_receive_irq(UART_NUM);
		ic_id_debug("%s: request irq done\n", __FUNCTION__);
	}
#endif
     
	printk("RF Card Module Init Done!\n");
	return 0;
	
setup_cdev_fail:
	kfree(ic_id_devp);
alloc_mem_fail:
	unregister_chrdev_region(devno, 1);
	return err;
}

static __exit void ic_id_exit(void)
{
	dev_t devno;

	__uart_disable(UART_NUM);
	
	device_destroy(ic_id_card_class, ic_id_devp->devno);
	class_destroy(ic_id_card_class);
	
	cdev_del(&ic_id_devp->cdev);
	
	devno = ic_id_devp->devno;
	kfree(ic_id_devp);
	unregister_chrdev_region(devno, 1);
	
	ic_id_debug("ic_id_exit: driver removed!\n");
}

MODULE_AUTHOR("Yuan Juntao. yjttust@163.com");
MODULE_LICENSE("Dual BSD/GPL");

module_init(ic_id_init);
module_exit(ic_id_exit);



