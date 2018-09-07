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

#include <asm/jzsoc.h>
#include <linux/device.h>
MODULE_AUTHOR("shecun feng <fsc0@163.com>");
MODULE_DESCRIPTION("led red_green Driver"); 
MODULE_LICENSE("GPL");
#define JZ_led_pwm_DEBUG 1


#ifdef JZ_led_pwm_DEBUG
#define dbg(format, arg...) printk( ": " format "\n" , ## arg)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) printk(": " format "\n" , ## arg)
#define info(format, arg...) printk(": " format "\n" , ## arg)
#define warn(format, arg...) printk(": " format "\n" , ## arg)

int led_red_green_major = 56;


#define GPIO_LED_RED	(32*6 + 13) // PG10
#define GPIO_LED_GREEN	(32*6 + 10) // PG13

#define ledopen     1
#define ledclose     2

#define redcmd      1
#define greencmd   2
#define yellowcmd  3


static int red_ioctrl(int n)
{
	switch(n)
		{
		case ledopen:
			__gpio_clear_pin(GPIO_LED_GREEN);
			__gpio_set_pin(GPIO_LED_RED);
			break;
		case ledclose:
			__gpio_clear_pin(GPIO_LED_GREEN);
			__gpio_clear_pin(GPIO_LED_RED);
			break;
		default:
			return -1;
		}
	return 0;
	
}

static int green_ioctrl(int n)
{
	switch(n)
		{
		case ledopen:
			__gpio_clear_pin(GPIO_LED_RED);
			__gpio_set_pin(GPIO_LED_GREEN);
			break;
		case ledclose:
			__gpio_clear_pin(GPIO_LED_RED);
			__gpio_clear_pin(GPIO_LED_GREEN);
			break;
		default:
			return -1;
		}
	return 0;

}

static int yellow_ioctrl(int n)
{
	switch(n)
		{
		case ledopen:
			__gpio_set_pin(GPIO_LED_RED);
			__gpio_set_pin(GPIO_LED_GREEN);
			break;
		case ledclose:
			__gpio_clear_pin(GPIO_LED_RED);
			__gpio_clear_pin(GPIO_LED_GREEN);
			break;
		default:
			return -1;
		}
	return 0;

}

static int LED_red_green_Close(struct inode * inode, struct file * file)
{
	return 0;
}
  
static int LED_red_green_Open(struct inode * inode, struct file * file)
{

	dbg("open led_red_green device minor is %d \n",MINOR(inode->i_rdev));

	__gpio_as_output(GPIO_LED_RED);
	__gpio_as_output(GPIO_LED_GREEN);

	__gpio_clear_pin(GPIO_LED_RED);
	__gpio_clear_pin(GPIO_LED_GREEN);

	
	return 0;
}


static int LED_red_green_Read(struct file *fp, unsigned int *buf, size_t count)
{
	return 0;

}
 
int LED_red_green_release(struct inode *inode, struct file *file)
{
	dbg("release  led_red_green device");
        //free_irq(IRQ_led_pwm,"0");
	return 0;
}

static int LED_red_green_Ioctl(struct inode *inode,struct file *file,unsigned int cmd, unsigned long arg)
{ 
	//dbg("cmd is %d ,arg is %d\n",cmd,(int)arg);

	switch(cmd)  	
		{ 		
		case redcmd: 
			return red_ioctrl((int)arg);  	
			//break;
		case greencmd: 
			return green_ioctrl((int)arg);  	
			//break;
		case yellowcmd: 
			return yellow_ioctrl((int)arg);  	
			//break;
		default:                        
			printk("Unkown  Command ID.\n");  
			return -1;
	}     
	return 0; 

}

 
struct file_operations LED_red_green_fops = 
{
	open:	 LED_red_green_Open, 
	ioctl:	  LED_red_green_Ioctl, 
	release:    LED_red_green_Close, 
	read:	  LED_red_green_Read, 
	release:    LED_red_green_release,
};


struct class *my_led_red_green_class;

static int MY_create_device(int major_num, char *dev_name, char *class_name)
{
   	my_led_red_green_class = class_create(THIS_MODULE, class_name);
//	device_create(my_led_class, NULL, MKDEV(major_num, 0), "led_pwm%d", 0);

	device_create(my_led_red_green_class, NULL, MKDEV(major_num, 0), dev_name, "led_red_green", 0);

	return 0;

	return 0;
}

int __init LED_red_green_Init(void)
{
	int result;

	result = register_chrdev(led_red_green_major, "led_red_green", &LED_red_green_fops);
	
	if (result<0)
	{
		printk(KERN_INFO"[FALLED: Cannot register led_red_green_driver!]\n");
		return result;
	}

	MY_create_device(led_red_green_major, "led_red_green", "class_led_red_green");

	return 0;

}

void __exit LED_red_green_Exit(void)
{
	unregister_chrdev(led_red_green_major, "led_red_green");
	device_destroy(my_led_red_green_class, MKDEV(led_red_green_major, 0));
	class_destroy(my_led_red_green_class);
	//	free(my_led_class);
}

//EXPORT_SYMBOL(LED_OPENED);
module_init(LED_red_green_Init);
module_exit(LED_red_green_Exit);

