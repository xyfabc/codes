// #include <linux/config.h>
#include <linux/utsname.h>
#include <linux/kernel.h>
#include <linux/major.h>
#include <linux/string.h>
#include <linux/fcntl.h>
#include <linux/slab.h>
#include <linux/timer.h>
#include <linux/sched.h>
#include <linux/tty.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/fs.h>
#include <linux/wait.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/interrupt.h>
#include <linux/device.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include "kbd_BF6911AQ22.h"

MODULE_AUTHOR("shecun feng <fsc0@163.com>");
MODULE_DESCRIPTION("key boards Driver");
MODULE_LICENSE("GPL");

#define DEBUG_MESG_GOLF 
//#undef DEBUG_MESG_GOLF

#define KEY_TEST
#undef KEY_TEST

#ifdef KEY_TEST
	//#define BUFFER_TIMEOUT     msecs_to_jiffies(1000)  /* 1 seconds */
	#define BUFFER_TIMEOUT     HZ * 10 
	static int key_test_flag = 0;
	static unsigned char key_test_buf[12] = {0x11,0x11,0x34,0x41,0x13,0x41,0x42,0x43,0x13,0x11,0x31,0x11,0x11,0x11};
	static unsigned char key_buf_index = 0;
    static struct timer_list key_timer;
#endif

unsigned char kbd_buf = 0xFF;
static int   kbd_flag = 0;
static wait_queue_head_t wq;

#ifdef KEY_TEST
	static void key_test_timeout()
{
	kbd_flag = 1;

	mod_timer(&key_timer, jiffies + BUFFER_TIMEOUT);

	printk("now here is:%s",__FUNCTION__);
}

	static int timer_init(void)
{	
	key_timer.function = key_test_timeout;

	init_timer(&key_timer);

	mod_timer(&key_timer, jiffies + BUFFER_TIMEOUT);

	printk("now here is:%s",__FUNCTION__);

	return 0;
}
#endif

/*  I2C  */
//static unsigned int i2c_addr = 0x70;
//static unsigned int i2c_addr = 0x20;
static unsigned int i2c_clk = 100000;
#define I2C_READ	1
#define I2C_WRITE	0
#define TIMEOUT         1000
struct _spls_dev
{
	struct cdev cdev;
	dev_t devno;
	struct semaphore sem;
	struct i2c_client *client;
};

struct _spls_dev /* *pspls_dev, */ spls_dev;


static struct tasklet_struct tlet;


static void kkbd_enable_irq(void)
{
	//__intc_unmask_irq(IRQ_GPIO3);
	__gpio_unmask_irq(KBD_IRQ_PIN);
	//enable_irq(IRQ_KBD);
}


static void kkbd_disable_irq(void)
{

	__gpio_mask_irq(KBD_IRQ_PIN);
	//disable_irq(IRQ_GPIO3);
	//disable_irq(IRQ_KBD);
}


static struct i2c_client *gclient = NULL;

#if 0
static inline int kbd_i2c_master_send(struct i2c_client *client, const char *buf, int count)
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	printk("client->addr:%x\n", client->addr);
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;
	ret = i2c_transfer(adap, &msg, 1);
//	printk("ret = i2c_transfer(adap, &msg, 1):%x\n", ret);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

static inline int kbd_i2c_master_recv(struct i2c_client *client, char *buf, int count)
{
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
	printk("client->addr:%x\n", client->addr);
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = buf;
	ret = i2c_transfer(adap, &msg, 1);
//	printk("ret = i2c_transfer(adap, &msg, 1):%x\n", ret);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

#endif
 
/*add by dy */

static unsigned char spls_write_reg(struct i2c_client *client, unsigned char reg,unsigned char val)
{

	int ret;
	unsigned char buf[2]={reg,val};
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 2;

	msg.buf = buf;
	ret = i2c_transfer(adap, &msg, 1);
	
	return (ret == 1) ? 0 : ret;

}



static unsigned char spls_read_reg(struct i2c_client *client, unsigned char reg)
{
/*
	int ret;
	unsigned char retval;

	//ret = kbd_i2c_master_send(client, &reg, 1);

	//if (ret < 0)
	//	return ret;
	//if (ret != 1)
	//	return -EIO;

	ret = kbd_i2c_master_recv(client, &retval, 1);
	if (ret < 0) {
		printk("%s: ret < 0\n", __FUNCTION__);
		return ret;
	}
	if (ret != 1) {
		printk("%s: ret != 1\n", __FUNCTION__);
		return -EIO;
	}
	
	return retval;
*/

	int ret;
	int status = 0;
	unsigned char retval;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg[2];

		//msg[0] = client->addr;				//sub_addr
		msg[0].addr = client->addr;	//slave addr
		msg[0].flags = 0;				//write
		msg[0].buf = &reg;
		msg[0].len = 1;
		
		msg[1].addr = client->addr;
		msg[1].flags = I2C_M_RD;
		msg[1].buf = &retval;
		msg[1].len = 1;

	status = i2c_transfer(client->adapter, msg, 2);
	int i;
	if(2 == status)
		{
			status = 0;	
		printk("retval=%x --\n", retval);
			return retval;
		}
		else
		{
			status = -EFAULT;	
		}
		
}

static unsigned char spls_read_val(struct i2c_client *client)
{
	
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;
	int ret;
	unsigned char retval;

	msg.addr = client->addr;
	//printk("client->addr:%x\n", client->addr);
	msg.flags = client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = 1;
	msg.buf = &retval;
	ret = i2c_transfer(adap, &msg, 1);
	//printk("ret = i2c_transfer(adap, &msg, 1):%x\n", ret);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? retval : ret;

}

static unsigned char spls_send_command(struct i2c_client *client,unsigned char command)
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
	msg.flags = 0;
	msg.len = 1;

	msg.buf = &command;
	ret = i2c_transfer(adap, &msg, 1);
	
	return (ret == 1) ? 0 : ret;
}


static void remap_keyval(unsigned char *buf)
{
	int i;
	for(i=0;i<sizeof(d_key_remap)/sizeof(d_key_remap[0]); i++)
	{
		if(*buf == d_key_remap[i].val)	
		{
			*buf=d_key_remap[i].reval;
			break;
		}
	} 
}

static void kbd_tasklet_fn(unsigned long arg)
{
	int i = 0;

	//kbd_buf = spls_read_reg(spls_dev.client, 0);
	kbd_buf=spls_read_val(spls_dev.client);	//add by dy

#ifdef KEY_TEST
	if(0x41 == kbd_buf)
	{
		key_test_flag = 1;
		timer_init();
		printk("now key_test_flag is 1!!!!!\n");
	}
#endif


#ifndef DEBUG_MESG_GOLF
	printk("!!-- %x --\n", kbd_buf);
#endif

	if( kbd_buf /*spls_read_reg(spls_dev.client, 0)*/)
	{
		//printk("KEY VALUE: %08X\n", kbd_buf);
#if 0
		for(i=0; i<sizeof(g_key_remap)/sizeof(g_key_remap[0]); i++)
		{
			if(0 == g_key_remap[i].val)
			{
				kbd_buf = -1;
				break;
			}
			if(kbd_buf == g_key_remap[i].val)
			{
				kbd_buf = g_key_remap[i].reval;
				break;
			}
		}
#endif

#ifdef DEBUG_MESG_GOLF
		printk("KEY VALUE: %x\n", kbd_buf);
#endif
		// ÓÐÐ§Öµ
		if(0x55 != kbd_buf)
		{ 
			remap_keyval(&kbd_buf); //add by dy

			kbd_flag = 1;
			wake_up_interruptible(&wq);
		}
	}

}

static irqreturn_t get_key_value(int irq, void *dev_id)
{
	int i=0;

#ifdef DEBUG_MESG_GOLF
	printk("4760 kbd intrrupt signal ... \n");
#endif

	kkbd_disable_irq();

	if(1/*kbd_i2c_read(pspls_dev->client , &kbd_buf)*/)
	{
		//printk("KEY VALUE: %08X\n", kbd_buf);
#if 1
		// kbd_buf = Kkey_transition_Value(kbd_buf);
#else
		for(i=0; i<sizeof(g_key_remap)/sizeof(g_key_remap[0]); i++)
		{
			if(0 == g_key_remap[i].val)
			{
				kbd_buf = -1;
				break;
			}
			if(kbd_buf == g_key_remap[i].val)
			{
				kbd_buf = g_key_remap[i].reval;
				break;
			}
		}
#endif
		//printk("KEY VALUE: %08X\n", kbd_buf);
	//	kbd_flag=1;
	//	wake_up_interruptible(&wq);
	}

	kkbd_enable_irq();

	tasklet_schedule(&tlet);
	
	return 0; //IRQ_HANDLED;
}

static int keypad_Open(void)
{
	int i;
	int retval;
	
	printk("open kbd device!!\n");

	/* interrupt keyboard */
	printk("KBD_Open IRQ %d !!! \n", IRQ_KBD);

	__gpio_unmask_irq(KBD_IRQ_PIN);
//	__gpio_as_irq_rise_edge(KBD_IRQ_PIN);
	__gpio_as_irq_fall_edge(KBD_IRQ_PIN);
//	__gpio_as_irq_high_level(KBD_IRQ_PIN);

	retval = request_irq(IRQ_KBD, get_key_value, IRQF_DISABLED/*SA_INTERRUPT*/,  "kbd_jz", "0"); 
	if(retval)
	{
		err("unable torequest IRQ %d", IRQ_KBD);
		return retval;
	}

	return 0;
}
static int KBD_Open(struct inode *inode, struct file *file)
{
	keypad_Open();

	return 0;
}

static int KBD_Read(struct file *fp, unsigned int *buf, size_t count)
{
	//if(wait_event_interruptible(wq,kbd_flag!=0))
	//	return -1;
#ifndef KEY_TEST

	kbd_flag = 0;	
	copy_to_user(buf, &kbd_buf, 1);

		return count;	

#else

	if(1 == key_test_flag)
	{
		if(key_buf_index < 12)
			copy_to_user(buf, &key_test_buf[key_buf_index ++], 1);
		else
			key_buf_index = 0;
	}
	return count;

#endif

   

}

static int KBD_Poll(struct file * filp, poll_table * wait)
{
	poll_wait(filp, &wq, wait);
	if(kbd_flag)
		return POLLIN | POLLRDNORM;		
	else 
		return 0;
}

static int KBD_release(struct inode *inode, struct file *file)
{
#ifdef DEBUG_MESG_GOLF
	dbg("release  kbd device");
#endif
	free_irq(IRQ_KBD,"0");

	return 0;
}

static int KBD_Ioctl(struct inode *inode,struct file *file,unsigned int cmd, unsigned long arg)
{

	return 0;
}

struct file_operations kbd_spls_fops = 
{
	.owner		=	THIS_MODULE,
	.open 		=	KBD_Open, 
	.ioctl 		=	KBD_Ioctl, 
	.read 		=	KBD_Read, 
	.poll 		=	KBD_Poll,
	.release 	=	KBD_release,
};

static struct class *mthy_class;

static int kbd_create_device(int major_num, char *dev_name, char *class_name)
{
	mthy_class = class_create(THIS_MODULE, class_name);
	device_create(mthy_class, NULL, MKDEV(KEYBOARD_MAJOR, 0), dev_name, "kbd_jz", 0);
	
	return 0;
}

static int spls_setup_cdev(struct _spls_dev *dev)
{
	int result;
	
	cdev_init(&dev->cdev, &kbd_spls_fops);
	dev->cdev.owner = kbd_spls_fops.owner;

	result = cdev_add(&dev->cdev, dev->devno, 1);
	
	if(result < 0)
	{
		printk("spls_setup_cdev: faitmp100 to add cdev\n");
	}
	else
	{
		printk("spls_setup_cdev: add cdev ok, devno: %d, %d\n", MAJOR(dev->devno), MINOR(dev->devno));
	}
	
	return result;
}

extern void  i2c_jz_setclk(struct i2c_client *client,unsigned long i2cclk);
/* It's seems ok without this function */
static void sensor_set_i2c_speed(struct i2c_client *client,unsigned long speed)
{
#if defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_JZ4760_F4760) || defined(CONFIG_JZ4810_F4810)
	i2c_jz_setclk(client, speed);
#endif
	//printk("set sensor i2c write read speed = %d hz\n",speed);
}

static int spls_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int result;
	dev_t devno;
	struct _spls_dev *p_dev = &spls_dev;

	
	init_waitqueue_head(&wq);

//	devno = MKDEV(KEYBOARD_MAJOR, 0);

	sensor_set_i2c_speed(client, 100000/* 200000 bad */);//set i2c speed : 200khz

#if 1
	result = register_chrdev(KEYBOARD_MAJOR, "kbd_jz", &kbd_spls_fops);
	if(result<0)
	{
		printk("\nFALLED: Cannot register kbd_driver! \n");
		
		return result;
	}
	else
	{
		printk("Register kbd_driver OK!\n");
	}
	
#else
	if(KEYBOARD_MAJOR){
		
		result = register_chrdev_region(devno, 1, "spls");	
	} else {
	
		result = alloc_chrdev_region(&devno, 0, 1, "spls");	
	}
	if(result < 0)
	{
		printk("spls_probe: faitmp100 to alloc dev num\n");
		return result;	
	}
#endif

#if 0
	if(!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) 
	{
		printk("spls_probe: didn't support I2C_FUNC_I2C\n");
		return -EPFNOSUPPORT;
	}
#endif

#if 0
	pspls_dev = kzalloc(sizeof(struct _spls_dev), GFP_KERNEL);
	if(NULL == pspls_dev)
	{
		result = -ENOMEM;
		goto alloc_dev_fail;
	}

	pspls_dev->devno = devno;
//	spls_dev.devno = devno;
	pspls_dev->client = client;
//	spls_dev.client =  client;
//	gclient = client;
//	printk("gclient: %p\n", gclient);
#endif

	p_dev->devno = devno;
	p_dev->client = client;


#if 0	
	result = spls_setup_cdev(pspls_dev);
	
	if(result)
	{
		goto setup_cdev_fail;	
	}
#endif

	kbd_create_device(p_dev->devno, "kbd_jz", "class_kbd");


#ifdef DEBUG_MESG_GOLF

	int i = 0;
	for(i = 0; i<1; i++){
		printk("-- %x --\n", spls_read_val(spls_dev.client));
		mdelay(1000);
//		break;
	}
#endif

	spls_send_command(client,0x37);   //reset KBD
	printk("spls_probe: done\n");

	tasklet_init(&tlet, kbd_tasklet_fn, 0);

//	keypad_Open();
	
	return 0;

#if 0
setup_cdev_fail:
	kfree(pspls_dev);
alloc_dev_fail:
	unregister_chrdev_region(devno, 1);
#endif

	return result;
}

static int spls_remove(struct i2c_client *client)
{
#if 0
	//cdev_del(&pspls_dev->cdev);
	
	kfree(pspls_dev);

	unregister_chrdev(pspls_dev->devno, "spls");
	device_destroy(mthy_class, MKDEV(pspls_dev->devno, 0));
	class_destroy(mthy_class);

	i2c_set_clientdata(client, NULL);
#endif

#ifdef DEBUG_MESG_GOLF
	printk("spls_remove: driver removed!\n");
#endif

	return 0;
}

static struct i2c_device_id spls_id[] = {
	{ "spls", 0 },
	{ }
};

static struct i2c_driver spls_driver = {
	.driver = {
		.name = "spls",
		.owner = THIS_MODULE,
	},
	.probe = spls_probe,
	.remove = spls_remove,
	.id_table = spls_id,
};

//#define PIN_LED_GOLF (32*5 + 11)// PF11
static int __init HW_SPLS_Init(void)
{
	//__gpio_as_output(PIN_LED_GOLF);
	//__gpio_clear_pin(PIN_LED_GOLF);

	return i2c_add_driver(&spls_driver);
}

static void __exit HW_SPLS_Exit(void)
{

	i2c_del_driver(&spls_driver);
}


module_init(HW_SPLS_Init);
module_exit(HW_SPLS_Exit);
 
