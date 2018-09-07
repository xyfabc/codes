/*
 * usb device -- driver for Hanvon USB Device;
 * Features
 * AUTHOR eric.xi
 * 16-09-09
 */

#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/slab.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <asm/uaccess.h>
#include <linux/module.h>
#include <linux/device.h>

#include <linux/delay.h>
#include <asm/io.h>





#define DEVICE_NAME "usbd"

#define HANVON_SCSI_STALL		_IO('U', 0xf0) // set stall feature command for producer 
#define  HANVON_SCSI_STATUS		_IO('U', 0xf1) // set CSW status command
#define  HANVON_SCSI_GET_CHANNEL_ID			_IOR('U', 0xf2, __u32) //get channel id command

//#define CHANNEL_ID_ADDR (CONFIG_RAM_BASE + SZ_1M)  /* the address for abtain channel id for burntool */

unsigned int sense_data; // the CSW status
struct semaphore sense_data_lock; // synchronization sense_data

EXPORT_SYMBOL(sense_data);
EXPORT_SYMBOL(sense_data_lock);

static int major = 0;
struct usbd_dev {
	struct cdev cdev;
	wait_queue_head_t rq_rbuf, wq_rbuf;	/* read and write queues for rbuf */
	wait_queue_head_t rq_wbuf, wq_wbuf;	/* read and write queues for wbuf */
	void *rbuf;	/* This buffer is used for user reading */
	size_t rlen; /* The length of rbuf */
	void *wbuf;	/* This buffer is used for user writing */
	size_t wlen; /* The length of wbuf */
	struct semaphore r_sem, w_sem;	/* mutual exclusion semaphore for rbuf and wbuf; */
	int r_stall; /* The flag of stop of rbuf */
	int w_stall; /* The flag of sotp of wbuf */
} *b_dev;
struct class *hanvon_scsi_class;

static int hanvon_scsi_open(struct inode *inode, struct file *file)
{
	printk("hanvon scsi  device is open\n");
	
	return 0;
}

static int hanvon_scsi_close(struct inode *inode, struct file *file)
{
	printk("hanvon scsi  device is closed\n");

	return 0;
}

/* the read function for the producer */
ssize_t hanvon_scsi_read(struct file *filp, char __user *buf,
				size_t count, loff_t *f_pos)
{
	//HANVON_DBG("begin");

	/*
	 * count is used to record the number of data to read;
	 * count1 is used to record the total number of data been read,
	 * readn is used to record the number of data been read each time.
	 */
	size_t count1 = 0, readn;

	while (count != 0) /* when the all data has been read, return. */
	{
		down(&b_dev->r_sem); /* before access the rbuf member of b_dev, down the r_sem */
		while (b_dev->rbuf == NULL) { /* when there is no data to read, sleep to wait. */
			up(&b_dev->r_sem); /* up the r_sem */
			wait_event(b_dev->rq_rbuf, b_dev->rbuf != NULL); /* to sleep until some data is coming. */
			down(&b_dev->r_sem);
		}
		readn = min(count, b_dev->rlen);

		/* copy the data to user space */
		if (copy_to_user(buf + count1, b_dev->rbuf, readn)) {
			up(&b_dev->r_sem);
			return -EFAULT;
		}
		count1 += readn;
		count -= readn;

		/* the number of data which has been read is less than b_dev->rlen */
		if (readn < b_dev->rlen) {
			b_dev->rbuf += readn;
			b_dev->rlen -= readn;
			up(&b_dev->r_sem);
		} else { /* wake up the wq_rbuf if the all data in rbuf has been read. */
			b_dev->rbuf = NULL;
			up(&b_dev->r_sem);
			wake_up(&b_dev->wq_rbuf);

			/* if the r_stall is set, return */
			if (b_dev->r_stall == 1) {
				b_dev->r_stall = 0;
				break;
			}
		}
	}
	//HANVON_DBG("done");
	return count1;
}

/* the write function for the usb file_storage */
int hanvon_usbd_write(void *buf, size_t count)
{
	down(&b_dev->r_sem); // down the r_sem before access the rbuf member of b_dev. 
	b_dev->rbuf = buf;
	b_dev->rlen = count;

	// sleep until the all data in rbuf has been read. 
	while (b_dev->rbuf != NULL) {
		up(&b_dev->r_sem);
		wake_up(&b_dev->rq_rbuf);
		wait_event(b_dev->wq_rbuf, b_dev->rbuf == NULL);
		down(&b_dev->r_sem);
	}
	up(&b_dev->r_sem);

	return count;
}
EXPORT_SYMBOL(hanvon_usbd_write);

/* the write function for the producer */
ssize_t hanvon_scsi_write(struct file *filp, const char __user *buf,
				size_t count, loff_t *f_pos)
{
	/*
	 * count is used to record the number of data to write;
	 * count1 is used to record the total number of data been written,
	 * written is used to record the number of data been written each time.
	 */
	size_t count1 = 0, written;

	while (count != 0) {
		down(&b_dev->w_sem); /* before access the wbuf member of b_dev, down the w_sem. */
		while (b_dev->wbuf == NULL) {
			up(&b_dev->w_sem); /* up the w_sem. */
			wait_event(b_dev->wq_wbuf, b_dev->wbuf != NULL); /* sleep until wbuf can be written. */
			down(&b_dev->w_sem);
		}
		written = min(count, b_dev->wlen);

		/* copy the user space buffer to the kernel. */
		if (copy_from_user(b_dev->wbuf, buf + count1, written)) {
			up(&b_dev->w_sem);
			return -EFAULT;
		}
		count1 += written;
		count -= written;

		/* if written if less than b_dev->wlen. */
		if (written < b_dev->wlen) {
			b_dev->wbuf += written;
			b_dev->wlen -= written;

			/* if w_stall is set, wake up the rq_buf and return. */
			if (b_dev->w_stall == 1) {
				b_dev->wbuf = NULL;
				b_dev->wlen = written;
				b_dev->w_stall = 0;
				up(&b_dev->w_sem);
				wake_up(&b_dev->rq_wbuf);
				break;
			}
			up(&b_dev->w_sem);
		} else { /* wake up rq_wbuf if the wbuf is full written. */
			b_dev->wbuf = NULL;
			up(&b_dev->w_sem);
			wake_up(&b_dev->rq_wbuf);
		}
	}

	return count1;
}

/* the read function for the usb file_storage */
int hanvon_usbd_read(void *buf, size_t count)
{
	down(&b_dev->w_sem);
	b_dev->wbuf = buf;
	b_dev->wlen = count;

	//sleep until wbuf has been written. 
	while (b_dev->wbuf != NULL) {
		up(&b_dev->w_sem);
		wake_up(&b_dev->wq_wbuf);
		wait_event(b_dev->rq_wbuf, b_dev->wbuf == NULL);
		down(&b_dev->w_sem);
	}
	count = b_dev->wlen;
	up(&b_dev->w_sem);
	return count;
}
EXPORT_SYMBOL(hanvon_usbd_read);

long hanvon_scsi_ioctl(struct file *filp, unsigned int cmd,
				unsigned long arg)
{
	unsigned long *channel_id;

	switch (cmd) {
	case  HANVON_SCSI_STALL:
		down(&b_dev->w_sem);
		b_dev->w_stall = 1; /*set the w_stall flag for wbuf. */
		up(&b_dev->w_sem);
		break;
	case  HANVON_SCSI_STATUS:
		sense_data = arg;
		up(&sense_data_lock);
		break;
	case  HANVON_SCSI_GET_CHANNEL_ID:
		/*
		channel_id = ioremap(CHANNEL_ID_ADDR, sizeof (*channel_id));
		if (channel_id == NULL) {
			printk("ioremap error\n");
			return -1;
		}
		if (copy_to_user((void __user *)arg, channel_id, sizeof (*channel_id))) {
			printk("copy id to user space failed!\n");
			iounmap(channel_id);
			return -1;
		}
		iounmap(channel_id);*/
		break;
	default:
		return -1;
	}
	
	return 0;
}

void hanvon_usbd_ioctl(void)
{
	down(&b_dev->r_sem);
	b_dev->r_stall = 1; /*set the r_stall flag for rbuf. */
	up(&b_dev->r_sem);
}
EXPORT_SYMBOL(hanvon_usbd_ioctl);

/* The file operation for the usbburn device.*/
static struct file_operations hanvon_scsi_fops = {
	.owner = THIS_MODULE,
	.read = hanvon_scsi_read,
	.write = hanvon_scsi_write,
	.unlocked_ioctl = hanvon_scsi_ioctl,
	.open = hanvon_scsi_open,     
	.release = hanvon_scsi_close,
};

int __init hanvon_scsi_init(void)
{
	int ret = 0;
	struct device *device;
	dev_t dev = MKDEV(major, 0);
	
	/* allocate the burn device. */
	b_dev = kmalloc(sizeof(*b_dev), GFP_KERNEL);
	if (unlikely (!b_dev)) {
		ret = -ENOMEM;
		printk("error, out of memory\n");
		goto out1;
	}
	memset(b_dev, 0, sizeof(*b_dev));

	/* Register device major, and accept a dynamic number. */
	if (major)
		ret = register_chrdev_region(dev, 1, DEVICE_NAME);
	else {
		ret = alloc_chrdev_region(&dev, 0, 1, DEVICE_NAME);
		major = MAJOR(dev);
	};
	if (ret < 0) {
		printk("register chrdev major and minor number failed\n");
		goto out1;
	}
	
	cdev_init(&b_dev->cdev, &hanvon_scsi_fops);
	b_dev->cdev.owner = THIS_MODULE;
	ret = cdev_add(&b_dev->cdev, dev, 1);
	/* Fail gracefully if need be */
	if (ret) {
		printk("Error %d adding akudc_usbburn_dev", ret);
		goto out1;
	}

	/* create the hanvon_scsi_class char device's class */
	hanvon_scsi_class = class_create(THIS_MODULE, DEVICE_NAME);
	if (IS_ERR(hanvon_scsi_class)) {
		ret = PTR_ERR(hanvon_scsi_class);
		printk("create hanvon_scsi_class class failed\n");
		goto out2;
	}
	
	/* create the hanvon_scsi_class char device node in /dev/akudc_usbburn */
	device = device_create(hanvon_scsi_class, NULL, dev, NULL,DEVICE_NAME);
	if (IS_ERR(device)) {
		printk("hanvon_scsi_class chrdev create failed! %x\n", ret);
		ret = PTR_ERR(device);
		goto out2;
	}
	else
		printk("hanvon_scsi_class chrdev create success!\n");

	/* init the waitqueue head for the rbuf and wbuf */
	init_waitqueue_head(&b_dev->rq_rbuf);
	init_waitqueue_head(&b_dev->wq_rbuf);
	init_waitqueue_head(&b_dev->rq_wbuf);
	init_waitqueue_head(&b_dev->wq_wbuf);

	/* init the r_sem and w_sem with 1 */
	sema_init(&b_dev->r_sem, 1);
	sema_init(&b_dev->w_sem, 1);

	/* init the sense_data_lock with 0 */
	memset(&sense_data_lock, 0, sizeof(sense_data_lock));
	sema_init(&sense_data_lock, 0);

	return ret;
out2:
	cdev_del(&b_dev->cdev);
out1:
	kfree(b_dev);

	return ret;
}

void __exit hanvon_scsi_exit(void)
{
	/* delete the device node */
	device_destroy(hanvon_scsi_class, MKDEV(major, 0));
	/* delete the class for usbburn */
	class_destroy(hanvon_scsi_class);
	cdev_del(&b_dev->cdev);
	kfree(b_dev);
	unregister_chrdev_region(MKDEV(major, 0), 1);
}

module_init(hanvon_scsi_init);
module_exit(hanvon_scsi_exit);

MODULE_DESCRIPTION(" usb device driver");
MODULE_AUTHOR("eric.xi");
MODULE_LICENSE("GPL");

