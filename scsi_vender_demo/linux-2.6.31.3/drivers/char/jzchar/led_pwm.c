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
#include <linux/proc_fs.h>


//#include "led_pwm.h"
#include <asm/jzsoc.h>
#include <linux/device.h>
MODULE_AUTHOR("shecun feng <fsc0@163.com>");
MODULE_DESCRIPTION("led pwm Driver"); 
MODULE_LICENSE("GPL");
#define JZ_led_pwm_DEBUG 1
#undef JZ_led_pwm_DEBUG 

#ifdef JZ_led_pwm_DEBUG
#define dbg(format, arg...) printk( ": " format "\n" , ## arg)
#else
#define dbg(format, arg...) do {} while (0)
#endif
#define err(format, arg...) printk(": " format "\n" , ## arg)
#define info(format, arg...) printk(": " format "\n" , ## arg)
#define warn(format, arg...) printk(": " format "\n" , ## arg)

int led_pwm_major = 55;
int LED_OPENED = 0;

/*
//static struct timer_list pwm_timer;
static struct hrtimer pwm_timer;
*/
static volatile int pwm_state = 0;
static volatile int duty_ticks = 75;
static int period_ticks = 100;

#define GPIO_LED_PWM	(32*4 + 2) // PE2, PWM, modify, yjt, 20130821
// #define GPIO_LED_SMALL	(32*1 + 30) // PB30	// del, yjt, 20130821
#define GPIO_LCD_BL     (32 * 1 + 30)  //modify, yjt, 20130821, PE2 -> PC9 -> PB30

static int led_PWM_control(int n);
static int lcd_backlight_control(int n);

void print_rtc_reg(void)
{
	printk("REG_RTC_RTCCR = 0x%08x\n", REG_RTC_RTCCR);
	printk("REG_RTC_RTCSR = 0x%08x\n", REG_RTC_RTCSR);
	printk("REG_RTC_RTCSAR = 0x%08x\n", REG_RTC_RTCSAR);
	printk("REG_RTC_RTCGR = 0x%08x\n", REG_RTC_RTCGR);
	printk("INTC_ICMR = 0x%08x\n", INTC_ICMR(1));
	printk("REG_INTC_IPR = 0x%08x\n", REG_INTC_IPR(1));
}

static void jz_set_nc1hz(void)
{
	rtc_write_reg(RTC_RTCGR, 10);
//	rtc_set_reg(RTC_RTCCR, 1 << 5);
}

void rtc_update_alarm(int alarm)
{
	uint32_t rtc_rtcsr = 0, rtc_rtcsar = 0;

	rtc_rtcsar = rtc_read_reg(RTC_RTCSAR); /* second alarm register */
	rtc_rtcsr = rtc_read_reg(RTC_RTCSR); /* second register */

	rtc_write_reg(RTC_RTCSAR, (rtc_rtcsr + alarm));
	rtc_set_reg(RTC_RTCCR, 0x3 << 2); /* alarm enable, alarm interrupt enable */
//	printk("%s: rtc_rtcsar = %d, rtc_rtcsr = %d\n", __func__, rtc_rtcsar, rtc_rtcsr);

	return;
}

#define IS_RTC_IRQ(x,y)  (((x) & (y)) == (y))
static irqreturn_t jz47xx_rtc_interrupt(int irq, void *dev_id)
{
	unsigned int rtccr, rtcsar;

	rtccr = rtc_read_reg(RTC_RTCCR);
	rtcsar = rtc_read_reg(RTC_RTCSAR);

	if(IS_RTC_IRQ(rtccr, RTC_RTCCR_AF)) {
//		printk("----------- jz4760b_rtc_interrupt: RTC_RTCCR = 0x%x ---------- \n", rtccr);
		rtc_clr_reg(RTC_RTCCR, RTC_RTCCR_AF);
		if(pwm_state) {
			__gpio_as_output0(GPIO_LED_PWM);	//(32*6 + 12); yjt, 20130510, change
			pwm_state = 0;
			rtc_update_alarm(duty_ticks);
		}
		else {
			__gpio_as_output1(GPIO_LED_PWM);	// (32*6 + 12); yjt, 20130510, change
			pwm_state = 1;
			rtc_update_alarm(period_ticks - duty_ticks);
		}
	}

	return IRQ_HANDLED;
}

static void jz47xx_alarm_start(void)
{
	__gpio_as_output1(GPIO_LED_PWM);	// (32*6 + 12); yjt, 20130510, change

	rtc_update_alarm(duty_ticks);
	OUTREG32(INTC_ICMCR(1), 0x1);
	jz_set_nc1hz();
	__intc_ack_irq(IRQ_RTC);
	__intc_unmask_irq(IRQ_RTC); //unmask rtc irq
	rtc_clr_reg(RTC_RTCCR,RTC_RTCCR_1HZ);
//	print_rtc_reg();

	printk("---------------------- Start RTC PWM -----------------\n");
}
static void jz47xx_alarm_stop(void)
{
	__intc_mask_irq(IRQ_RTC);
	REG_RTC_RTCCR &= ~(1 << 2);
	__gpio_as_output0(GPIO_LED_PWM);	// (32*6 + 12); 20130510, yjt, change

//	printk("---------------------- Stop RTC PWM -----------------\n");
}

static void jz47xx_rtc_init(void)
{
	int ret = 0;

	/* Disable RTC Alarm interrupt */
	rtc_clr_reg(RTC_RTCCR, RTC_RTCCR_AF);

	ret = request_irq(IRQ_RTC, jz47xx_rtc_interrupt, IRQF_DISABLED,
                                "rtc 1Hz and alarm", NULL);
        if (ret) {
                printk(KERN_ERR"IRQ %d already in use.\n", IRQ_RTC);
        }

	jz47xx_alarm_start();

	return;
}

/*
//static void pwm_function(unsigned long data)
static enum hrtimer_restart pwm_function(struct hrtimer *t)
{
	ktime_t tnew;

	if(pwm_state) {
		__gpio_as_output0(32*6 + 12);
		pwm_state = 0;
		tnew = ktime_set(0, 10000000*duty_ticks);
	}
	else {
		__gpio_as_output1(32*6 + 12);
		pwm_state = 1;
		tnew = ktime_set(0, 10000000*(period_ticks - duty_ticks));
	}

	hrtimer_start(&pwm_timer, tnew, HRTIMER_MODE_REL);

	return HRTIMER_NORESTART;
}
*/

static int my_led_open(void)
{
	__gpio_as_output1(GPIO_LED_PWM);	// add, yjt, 20130822

//	jz47xx_alarm_start();

//	__gpio_clear_pin(GPIO_LED_SMALL);	// DEL, yjt, 20130821

	return 0;
}

static int my_led_close(void)
{
	__gpio_as_output(GPIO_LED_PWM);
	__gpio_clear_pin(GPIO_LED_PWM);
	
//	jz47xx_alarm_stop();

//	__gpio_set_pin(GPIO_LED_SMALL);		// del, yjt, 20130821

	return 0;
}

int old_pwm_level = -1;
#define PWM_CHN 2    /* pwm channel */	// yjt, 20130510, change

/*
	open led / 577mA
6led,12V;( n / led current ):
	1 / 485 mA
	2 / 451 mA
	3 / 416 mA
	4 / 381 mA
	5 / 347 mA
	6 / 312 mA
	7 / 277 mA
	8 / 243 mA
	9 / 208 mA
	10/ 173 mA
	11/ 139 mA
	12/ 104	mA
	13/ 69  mA
	14/ 35	mA
*/

static int led_PWM_control(int n)
{
	int i =0;
	__gpio_as_pwm(2);		// modify, yjt, 20130821

	if(15 == n)
		n = 14;
	if(old_pwm_level != n)
	{
		i = (100/16) *(15-n);
		duty_ticks = i / 2 + 50;
		printk("lcd_PWM_control  n lever is %d, duty = %d \n", n, duty_ticks);
//		printk("lcd_PWM_control  15-n lever is %d \n",15-n);
		printk("Infrared_LED_PWM_control  lever is %d \n",i);
		//__gpio_as_pwm(2);
	    __tcu_disable_pwm_output(PWM_CHN);               
	    __tcu_stop_counter(PWM_CHN);                     
	    __tcu_init_pwm_output_high(PWM_CHN);             
	    __tcu_set_pwm_output_shutdown_abrupt(PWM_CHN);   
	    __tcu_select_clk_div1(PWM_CHN);                  
	    __tcu_mask_full_match_irq(PWM_CHN);              
	    __tcu_mask_half_match_irq(PWM_CHN);              
	    __tcu_set_count(PWM_CHN,0);                      
	    __tcu_set_full_data(PWM_CHN,__cpm_get_extalclk()/1000);           
	    __tcu_set_half_data(PWM_CHN,__cpm_get_extalclk()/1000*i/100);     
	    __tcu_enable_pwm_output(PWM_CHN);                
	    __tcu_select_extalclk(PWM_CHN);                  
	    __tcu_start_counter(PWM_CHN);                    
		old_pwm_level = n;
	}
	
//	__gpio_as_output(32*6 + 12);
//	__gpio_set_pin(32*6 + 12);
//	__gpio_clear_pin(GPIO_LED_SMALL);	// del, yjt, 20130821

	return n;
}

static int LED_Close(struct inode * inode, struct file * file)
{
	return 0;
}
  
static int LED_Open(struct inode * inode, struct file * file)
{
	LED_OPENED = 1;
	//dbg("open led_pwm device minor is %d \n",MINOR(inode->i_rdev));
        	
	printk("open Infrared_LED_PWM_control device minor is %d \n",MINOR(inode->i_rdev));
	return 0;
}
static int lcd_backlight_control(int n)
{
	int i = 0;
	
    if(n < 0 || n > 15)
    {
    	printk("Invalid BL level!!!\n");
		return old_pwm_level;
    }
	else 
	{
		__gpio_as_output(GPIO_LCD_BL);	
		__gpio_clear_pin(GPIO_LCD_BL);	
		mdelay(4);
		__gpio_set_pin(GPIO_LCD_BL); //add by dy 20130411
		mdelay(1);//add by dy 20130411

		for(i=0; i<n; i++)
		{
			__gpio_set_pin(GPIO_LCD_BL);
			udelay(5); 
			__gpio_clear_pin(GPIO_LCD_BL);	
			udelay(5);
		}
		__gpio_set_pin(GPIO_LCD_BL);
		
		old_pwm_level = n;

		printk("LCD_BL_Control level:%d\n", n);
		
	}

	return old_pwm_level;
	
}

static ssize_t LED_Read(struct file *fp, char __user *buf, size_t count, loff_t *ppos)
{
	return 0;

}
 
int LED_release(struct inode *inode, struct file *file)
{
	my_led_close();

	//dbg("release  led_pwm device");
	printk("release  led_pwm device");
        //free_irq(IRQ_led_pwm,"0");
	return 0;
}

static int LED_Ioctl(struct inode *inode,struct file *file,unsigned int cmd, unsigned long arg)
{ 
	switch(cmd)
  { 		
		case 0: 
			return my_led_close();

		case 1: 
			return my_led_open(); 

		case 2: 
			return lcd_backlight_control((int)arg);
			return 0;

	    case 3:
			//return led_PWM_control((int)arg);
			__gpio_as_output1(GPIO_LED_PWM);		// yjt, 20130822, maxim output
			return 0;

		default:                        
			printk("Unkown  Command ID.\n");
			return -1;
	}     
	return 0; 

}

struct file_operations LED_fops = 
{
	open:	 LED_Open, 
	ioctl:	  LED_Ioctl, 
	release:    LED_Close, 
	read:	  LED_Read, 
	release:    LED_release,
};

static int led_backlight_proc_write(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int l = 0;

        l = simple_strtoul(buffer, 0, 10);
	printk("Set backlight to %d\n", l);
	lcd_backlight_control(l);

        return count;
}

struct class *my_led_class;

static int MY_create_device(int major_num, char *dev_name, char *class_name)
{
   	my_led_class = class_create(THIS_MODULE, class_name);
//	device_create(my_led_class, NULL, MKDEV(major_num, 0), "led_pwm%d", 0);

	device_create(my_led_class, NULL, MKDEV(major_num, 0), dev_name, "led_pwm0");

	return 0;

	return 0;
}

int __init LED_Init(void)
{
	int result;
	struct proc_dir_entry *res;

	result = register_chrdev(led_pwm_major, "led_pwm", &LED_fops);
	
	if (result<0)
	{
		printk(KERN_INFO"[FALLED: Cannot register led_driver!!]\n");
		return result;
	}

	MY_create_device(led_pwm_major, "led_pwm", "class_led_pwm");

	res = create_proc_entry("led_backlight", 0644, NULL);
	if (res) {
		res->read_proc = NULL;
		res->write_proc = led_backlight_proc_write;
		res->data = NULL;
	}

	/* init small led ,add by dy */
//	__gpio_as_output(GPIO_LED_SMALL);	// del, yjt, 20130821
//	__gpio_set_pin(GPIO_LED_SMALL);		// del, yjt, 20130821

	__gpio_as_output(GPIO_LCD_BL);
	__gpio_set_pin(GPIO_LCD_BL);
	
	__gpio_as_output0(GPIO_LED_PWM);	// add, yjt, 20130517
//	__gpio_as_output0(GPIO_LED_SMALL);	// add, yjt, 20130517	// del, yjt, 20130821

/*
	init_timer(&pwm_timer);
	pwm_timer.function = pwm_function;
	pwm_timer.expires = jiffies + msecs_to_jiffies(10);
	add_timer (&pwm_timer);
*/
/*
	hrtimer_init(&pwm_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	pwm_timer.function = pwm_function;

	pwm_function(&pwm_timer);
*/

	// jz47xx_rtc_init();		// yjt, 20130510, del

	return 0;

}

void __exit LED_Exit(void)
{
	unregister_chrdev(led_pwm_major, "led_pwm");
	device_destroy(my_led_class, MKDEV(led_pwm_major, 0));
	class_destroy(my_led_class);
	//	free(my_led_class);
}

EXPORT_SYMBOL(LED_OPENED);
module_init(LED_Init);
module_exit(LED_Exit);

