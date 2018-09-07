#include <linux/module.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/ioctl.h>
#include <linux/major.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/signal.h>
#include <asm/io.h>
#include <asm/pgtable.h>
#include <asm/page.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <asm/uaccess.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/bcd.h>
#include <linux/rtc.h>

#include <linux/utsname.h>
#include <linux/device.h>

//#include "hym8563.h"

#define HYM8563_DEBUG
#undef HYM8563_DEBUG

#define hym8563_print(fmt, ...) 	printk(KERN_ALERT fmt, ##__VA_ARGS__)

#if defined(HYM8563_DEBUG)
#define hym8563_debug(fmt, ...) \
	printk(KERN_ALERT fmt, ##__VA_ARGS__)
#else
#define hym8563_debug(fmt, ...) \
	({ if (0) printk(KERN_ALERT fmt, ##__VA_ARGS__); 0; })
#endif

//#define RTC_HYM8563_IOC_MAGIC	0xd7

#define HYM8563_MAJOR	0

static DEFINE_MUTEX(rtc_lock); /* Protect state etc */

static const unsigned char days_in_month[] =
	{ 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

struct hym8563_dev
{
	struct cdev cdev;
	dev_t devno;
	struct semaphore sem;
	struct i2c_client *client;
	struct rtc_device *rtc;
};

struct hym8563_dev *hym8563_devp;

static const struct i2c_device_id i2c_hym8563[] = {
	{"hym8563", 0},
};

static struct i2c_driver i2c_driver_hym8563;

static struct class *dev_class;

static int dev_create_device(dev_t devno, char* dev_name, char* class_name)
{
	dev_class = class_create(THIS_MODULE, class_name);
	device_create(dev_class, NULL, devno, dev_name, "hym8563");
	return 0;
}

static int hym8563_open(struct inode *inode, struct file *filp);
static int hym8563_release(struct inode *inode, struct file *filp);
static ssize_t hym8563_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos);
static ssize_t hym8563_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos);
static int hym8563_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg);

static int hym8563_open(struct inode *inode, struct file *filp)
{
	hym8563_debug("hym8563_open: hym8563 dev opened!\n");
	filp->private_data = container_of(inode->i_cdev, struct hym8563_dev, cdev);
	return 0;
}

static int hym8563_release(struct inode *inode, struct file *filp)
{
	hym8563_debug("hym8563_release: exit\n");
	return 0;
}

static ssize_t hym8563_read(struct file *filp, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

static ssize_t hym8563_write(struct file *filp, const char __user *buf, size_t count, loff_t *ppos)
{
	return 0;
}

#define HYM8563_IIC_WRITE		1
#define HYM8563_IIC_READ		0
#define HYM8563_IIC_RW_BUFFER_SIZE	32

static int hym8563_iic_rw(struct i2c_client *client, u8 rw, u8 subaddr, u8 *buf, u8 len)
{
	struct i2c_msg msg[2];
	u8 msg_buf[HYM8563_IIC_RW_BUFFER_SIZE];
	s32 status = 0;
	s32 i;

	if(rw == 0)	//read
	{
		if(len > HYM8563_IIC_RW_BUFFER_SIZE)
		{
			return -EFAULT;
		}
		msg_buf[0] = subaddr;				//sub_addr
		msg[0].addr = client->addr;	//slave addr
		msg[0].flags = 0;				//write
		msg[0].buf = msg_buf;
		msg[0].len = 1;
		
		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].buf = msg_buf;
		msg[1].len = len;
		
		status = i2c_transfer(client->adapter, msg, 2);
		if(2 == status)
		{
			status = 0;	
			for(i=0;i<len;i++)
			{
				buf[i] = msg_buf[i];
			}
		}
		else
		{
			status = -EFAULT;	
		}
	}
	else			//write
	{
		if(len >= HYM8563_IIC_RW_BUFFER_SIZE)
		{
			return -EFAULT;
		}
		msg_buf[0] = subaddr;
		for(i=0;i<len;i++)
		{
			msg_buf[i+1] = buf[i];
		}
		msg[0].addr = client->addr;	//slave addr
		msg[0].flags = 0;				//write
		msg[0].buf = msg_buf;
		msg[0].len = len + 1;
		
		status = i2c_transfer(client->adapter, msg, 1);
		if(1 == status)
		{
			status = 0;	
		}
		else
		{
			status = -EFAULT;	
		}
	}

	return status;
}

static int hym8563_get_rtc_time(struct i2c_client *client, struct rtc_time *tm)
{
	u8 msg_buf[10];
	int i = 0;
        static unsigned long old_time = 0;
        unsigned long new_time = 0;
        unsigned char InvalidTimeFlag = 0;
        const unsigned char trycnt = 20;
        const unsigned char time_delay = 3;
        unsigned char cnt = 0;
  do{

	InvalidTimeFlag = 0;	  
 
	mutex_lock(&rtc_lock);

	if(hym8563_iic_rw(client, HYM8563_IIC_READ, 0, msg_buf, 9))
	{
		hym8563_print("read hwclock I2C failed\n");
		//mutex_unlock(&rtc_lock);
		//return -EFAULT;
	}

	mutex_unlock(&rtc_lock);
	
	tm->tm_sec = msg_buf[2] & 0x7f;
	tm->tm_min = msg_buf[3]	& 0x7f;
	tm->tm_hour = msg_buf[4] & 0x3f;

	tm->tm_wday = msg_buf[6] & 0x07;
	tm->tm_mday = msg_buf[5] & 0x3f;
	tm->tm_mon = msg_buf[7] & 0x1f;
	tm->tm_year = msg_buf[8];
	
	//hym8563_print("read: %02x-%02x-%02x, %02x, %02x:%02x:%02x\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	tm->tm_sec = bcd2bin(tm->tm_sec);
	tm->tm_min = bcd2bin(tm->tm_min);
	tm->tm_hour = bcd2bin(tm->tm_hour);
	tm->tm_mday = bcd2bin(tm->tm_mday);
	tm->tm_mon = bcd2bin(tm->tm_mon) - 1;
	tm->tm_year = bcd2bin(tm->tm_year) + 100;		//LINUX, year base: 1900, now we need year after 2000

	rtc_tm_to_time(tm,(unsigned long *)&new_time);
	
//	if(rtc_valid_tm(tm) < 0)
	if((rtc_valid_tm(tm) < 0) || (new_time > time_delay + old_time) || (old_time > time_delay + new_time))
	{
		hym8563_print("Read clock failed,data format invalid!time is:");
		//return -EFAULT;
		InvalidTimeFlag = 1;
	//hym8563_print("read: %02x-%02x-%02x, %02x, %02x:%02x:%02x\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour, tm->tm_min, tm->tm_sec);
	hym8563_print("%d-%d-%d  %d  %d:%d:%d\n",tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_wday,tm->tm_hour,tm->tm_min,tm->tm_sec);
	}

        old_time = new_time;
	schedule_timeout_interruptible(1);//sleep 10ms

 }while(InvalidTimeFlag && ++cnt < trycnt);
	//tm->tm_year += 1900;
        if(cnt >= trycnt)
        {
		printk("Get RTC time error!!!\n");
	        return -EFAULT;
        }
	else	
	{
		hym8563_print("Get RTC clock done!time is:");
	hym8563_print("%d-%d-%d  %d  %d:%d:%d\n",tm->tm_year - 100, tm->tm_mon + 1, tm->tm_mday, tm->tm_wday,tm->tm_hour,tm->tm_min,tm->tm_sec);
		return 0;
	}
}

static int hym8563_set_rtc_time(struct i2c_client *client, struct rtc_time *tm)
{
	u8 msg_buf[10];

	//tm->tm_year -= 1900;
	
	if(rtc_valid_tm(tm) < 0)
	{
		hym8563_print("write clock failed, data is invalid\n");
		return -EFAULT;
	}

	tm->tm_year %= 100;
	tm->tm_mon++;
	
	tm->tm_year = bin2bcd(tm->tm_year);
	tm->tm_mon = bin2bcd(tm->tm_mon);
	tm->tm_mday = bin2bcd(tm->tm_mday);
	tm->tm_hour = bin2bcd(tm->tm_hour);
	tm->tm_min = bin2bcd(tm->tm_min);
	tm->tm_sec = bin2bcd(tm->tm_sec);
	
	hym8563_print("write: %02x-%02x-%02x, %02x, %02x:%02x:%02x\n", tm->tm_year, tm->tm_mon, tm->tm_mday, tm->tm_wday, tm->tm_hour, tm->tm_min, tm->tm_sec);

	mutex_lock(&rtc_lock);		

	//write time
	msg_buf[0] = tm->tm_sec;
	msg_buf[1] = tm->tm_min;
	msg_buf[2] = tm->tm_hour;
	msg_buf[4] = tm->tm_wday;
	msg_buf[3] = tm->tm_mday;
	msg_buf[5] = tm->tm_mon;
	msg_buf[6] = tm->tm_year;
	if(hym8563_iic_rw(client, HYM8563_IIC_WRITE, 2, msg_buf, 7))
	{
		mutex_unlock(&rtc_lock);
		return -EFAULT;
	}

	mutex_unlock(&rtc_lock);

	return 0;
}

static int hym8563_ioctl(struct inode *inodep, struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct hym8563_dev *dev = (struct hym8563_dev *)filp->private_data;
	struct i2c_msg msg[2];
	struct i2c_client *client = dev->client;
	struct rtc_time tm;
	s32 status = 0;

	u8 buffer[15];
	u8 len;

	memset(msg, 0, sizeof(msg));
	
	switch(cmd)
	{
	case RTC_RD_TIME:
		
		if(hym8563_get_rtc_time(client, &tm))
		{
			return -EFAULT;
		}
		if(copy_to_user((struct rtc_time *)arg, &tm, sizeof(tm)))
		{
			return -EFAULT;
		}

		break;
	case RTC_SET_TIME:
		if (!capable(CAP_SYS_TIME))
		{
			return -EPERM;
		}

		if (copy_from_user(&tm, (struct rtc_time *)arg, sizeof(tm)))
		{
			return -EFAULT;
		}

		if(hym8563_set_rtc_time(client, &tm))
		{
			return -EFAULT;
		}
		
		break;
	default:
		status = -EINVAL;
		break;
	}

	return status;
}

static int hym8563_rtc_read_time(struct device *dev, struct rtc_time *tm)
{
	return hym8563_get_rtc_time(to_i2c_client(dev), tm);
}

static int hym8563_rtc_set_time(struct device *dev, struct rtc_time *tm)
{
	return hym8563_set_rtc_time(to_i2c_client(dev), tm);
}

static const struct rtc_class_ops hym8563_rtc_ops = {
	.read_time	= hym8563_rtc_read_time,
	.set_time	= hym8563_rtc_set_time,
};

static struct file_operations hym8563_fops = {
	.owner = THIS_MODULE,
	.open = hym8563_open,
	.release = hym8563_release,
	.read = hym8563_read,
	.write = hym8563_write,
	.ioctl = hym8563_ioctl,
};

static s32 hym8563_setup_cdev(struct hym8563_dev *dev)
{
	s32 err;
	
	cdev_init(&dev->cdev, &hym8563_fops);
	dev->cdev.owner = hym8563_fops.owner;

	err = cdev_add(&dev->cdev, dev->devno, 1);
	
	if(err < 0)
	{
		hym8563_debug("hym8563_setup_cdev: faitmp100 to add cdev\n");
	}
/*
	else
	{
		hym8563_debug("hym8563_setup_cdev: add cdev ok, devno: %d, %d\n", MAJOR(dev->devno), MINOR(dev->devno));
	}
*/	
	return err;
}


static int hym8563_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int err;
	dev_t devno = MKDEV(HYM8563_MAJOR, 0);
	
	hym8563_debug("hym8563_probe: start\n");
	
	if(HYM8563_MAJOR)
	{
		err = register_chrdev_region(devno, 1, "hym8563");	
	}
	else
	{
		err = alloc_chrdev_region(&devno, 0, 1, "hym8563");	
	}
	if(err < 0)
	{
		hym8563_debug("hym8563_probe: failed to alloc dev num\n");
		return err;	
	}
	
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		hym8563_debug("hym8563_probe: didn't support I2C_FUNC_I2C\n");
		return -EPFNOSUPPORT;
	}
	
	hym8563_devp = kzalloc(sizeof(struct hym8563_dev), GFP_KERNEL);
	if(NULL == hym8563_devp)
	{
		err = -ENOMEM;
		goto alloc_dev_fail;
	}

	hym8563_devp->rtc = rtc_device_register(i2c_driver_hym8563.driver.name,
				&client->dev, &hym8563_rtc_ops, THIS_MODULE);
	if(IS_ERR(hym8563_devp->rtc))
	{
		err = PTR_ERR(hym8563_devp->rtc);
		goto setup_cdev_fail;
	}
	
	hym8563_devp->devno = devno;
	hym8563_devp->client = client;
	
	err = hym8563_setup_cdev(hym8563_devp);
	
	if(err)
	{
		goto setup_cdev_fail;	
	}
	
	dev_create_device(hym8563_devp->devno, "hym8563", "class_hym8563");

	i2c_set_clientdata(client, hym8563_devp);

	hym8563_print("hym8563_probe: done\n");
	
	return 0;
	
setup_cdev_fail:
	kfree(hym8563_devp);
alloc_dev_fail:
	unregister_chrdev_region(devno, 1);
	return err;
}

//static int hym8563_detach_client(struct i2c_client *client)
static int hym8563_remove(struct i2c_client *client)
{
	//struct hym8563_data *hym8563;
	dev_t devno;

	if (hym8563_devp->rtc)
		rtc_device_unregister(hym8563_devp->rtc);

	cdev_del(&hym8563_devp->cdev);
	
	devno = hym8563_devp->devno;
	
	device_destroy(dev_class, devno);
	class_destroy(dev_class);
	
	kfree(hym8563_devp);
	unregister_chrdev_region(devno, 1);
	
	i2c_set_clientdata(client, NULL);
	
	hym8563_print("hym8563_remove: driver removed!\n");

	return 0;
}

static struct i2c_driver i2c_driver_hym8563 = {
	.driver = {
		.name = "hym8563",
		.owner = THIS_MODULE,
	},
	.id_table = i2c_hym8563,
	.probe = hym8563_probe,
	.remove = hym8563_remove,
};

static int __init hym8563_init(void)
{
	hym8563_debug("hym8563_init: init\n");
	return i2c_add_driver(&i2c_driver_hym8563);
}

static void __exit hym8563_exit(void)
{
	hym8563_debug("hym8563_exit: init\n");
	i2c_del_driver(&i2c_driver_hym8563);
}

module_init(hym8563_init);
module_exit(hym8563_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("HYM8563 RTC driver.");
MODULE_AUTHOR("YuanJuntao.yjttust@163.com");


