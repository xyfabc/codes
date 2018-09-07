/*
 * Virtual Video driver - This code emulates a real video device with v4l2 api
 *
 * Copyright (c) 2006 by:
 *      Mauro Carvalho Chehab <mchehab--a.t--infradead.org>
 *      Ted Walther <ted--a.t--enumera.com>
 *      John Sokol <sokol--a.t--videotechnology.com>
 *      http://v4l.videotechnology.com/
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the BSD Licence, GNU General Public License
 * as published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version
 */
#include <linux/module.h>
#include <linux/delay.h>
#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/mm.h>
#include <linux/ioport.h>
#include <linux/init.h> 
#include <linux/sched.h>
#include <linux/pci.h> 
#include <linux/random.h>
#include <linux/version.h>
#include <linux/mutex.h>
#include <linux/videodev2.h>
#include <linux/dma-mapping.h>
#ifdef CONFIG_VIDEO_V4L1_COMPAT
/* Include V4L1 specific functions. Should be removed soon */
#include <linux/videodev.h>
#endif
#include <linux/interrupt.h>
#include <media/videobuf-vmalloc.h>

#include <linux/kthread.h>
#include <linux/highmem.h>
#include <linux/freezer.h>
#include <asm/system.h>
#include <linux/spinlock.h>
#include <asm/irq.h>
#include <asm/uaccess.h>
#include <asm/jzsoc.h>

#include <linux/videodev.h>
#include <media/v4l2-common.h>
//#include <linux/video_decoder.h>


/* add 2010_04_15*/
#include "cim_driver_single.h"
#include <linux/i2c.h>


static unsigned int dmacmd_intr_flag = 0
#ifdef CIM_INTR_SOF_EN
	| CIM_CMD_SOFINT
#endif
#ifdef CIM_INTR_EOF_EN
	| CIM_CMD_EOFINT
#endif
#ifdef CIM_INTR_EEOF_EN
	| CIM_CMD_EEOFINT
#endif
	;
static int cim_id_init = 1;
static int cim_tran_buf_id;	/*cim dma current transfer buffer ID*/
static int data_ready_buf_id;	/*data ready for yuv convert buffer ID*/
static int last_data_buf_id;	/*last buf of yuv convert, if it is equal to data_ready_buf_id, wait eof intr*/
static int data_buf_reading_flag;

//#define FRM_BUF_NUM 6
#define FRM_BUF_NUM 3

static unsigned char *my_frm_buf[FRM_BUF_NUM];	/*current trans buffer pointer*/

#define FRM_BUF_LENGTH		(640 * 480 * 2)

#define GOLF_SHOW_RGB	1
#define GOLF_SHOW_YUV	0
#define GOLF_FORMAL_PRJ
#undef GOLF_FORMAL_PRJ


#define CIM_NULL		0
#define CIM_GC0308 	1

#define IMINOR_NULL 24
#define IMINOR_GC0308 23

static wait_queue_head_t          wq_out_rgb;
static wait_queue_head_t          wq_out_yuv;
static struct semaphore SENSOR_REG_mutex;
static spinlock_t fresh_lock;
static spinlock_t yuv_lock;
static int yuv_read_index = 2;
static int yuv_write_index = 0;
static bool yuv_updated = false;
static unsigned int frame_index = 0;
static struct JZ_GC0308_dmaqueue  dma_que;

#define  GET_NEWEST_INDEX(read_index,write_index) (3- (read_index) - (write_index))

#define CIM_DMA_INTERRUPT
#define TRUE    1
#define FALSE   0
//#define AECH 0X08
//#define AECL 0X10

#define AECH 0X0f    //for 7740
#define AECL 0X10    //for 7740
#define AGCL 0X00    //for 7740
#define AGCH 0x15    //for 7740
#define PRE_AGC 100
#define MIN_EXP 15
#define MIN_STEP 15


/*
 * CIM DMA descriptor
 */

struct cim_desc {
	u32 nextdesc;   // Physical address of next desc
#if defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_SOC_JZ4770) || defined(CONFIG_SOC_JZ4775)
	u32 frameid;    // Frame ID
	u32 framebuf;   // Physical address of frame buffer, when SEP=1, it's y framebuffer
#else
	u32 framebuf;   // Physical address of frame buffer, when SEP=1, it's y framebuffer
	u32 frameid;    // Frame ID
#endif
	u32 dmacmd;     // DMA command, when SEP=1, it's y cmd
	/* only used when SEP = 1 */
	u32 cb_frame;
	u32 cb_len;
	u32 cr_frame;
	u32 cr_len;
};


#define DEBUG_MESG_GOLF
#undef DEBUG_MESG_GOLF

static int sensor_init_i2c(struct i2c_client *client, unsigned char st_num);
static int get_sensor_reg(int selct_cim, unsigned char reg,unsigned char* date);
static int set_sensor_reg(int selct_cim, unsigned char reg, unsigned char data);
extern void jz_wdt_ping(void);


//static struct cim_desc *cim_frame_desc[FRM_BUF_NUM] __attribute__ ((aligned (32)));
static struct cim_desc *cim_frame_desc[FRM_BUF_NUM] __attribute__ ((aligned (sizeof(struct cim_desc))));

//static struct cim_desc *first_cim_desc __attribute__ ((aligned (32)));
static struct cim_desc *first_cim_desc __attribute__ ((aligned (sizeof(struct cim_desc))));

// extern unsigned char *lcd_frame0;	// del, yjt, 20130624
// extern unsigned char *test_lcd_frame0;	// add, yjt, 20130624

#define SENSOR_READ_TEST
#undef SENSOR_READ_TEST

/* end 2010_04_15*/

/* Wake up at about 30 fps */
#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001
#define BUFFER_TIMEOUT     msecs_to_jiffies(1000) 

#include "font.h"

#define JZ_GC0308_MAJOR_VERSION 0
#define JZ_GC0308_MINOR_VERSION 4
#define JZ_GC0308_RELEASE 0
#define JZ_GC0308_VERSION KERNEL_VERSION(JZ_GC0308_MAJOR_VERSION, JZ_GC0308_MINOR_VERSION, JZ_GC0308_RELEASE)

/* Declare static vars that will be used as parameters */
static unsigned int vid_limit = 16;	/* Video memory limit, in Mb */
static struct video_device JZ_GC0308_RGB;	/* Video device */
static struct video_device JZ_GC0308_YUV;	/* Video device */
static int video_nr = -1;		/* /dev/videoN, -1 for autodetect */

/* supported controls */
#define dprintk(level,fmt, arg...)					\
			printk( "JZ_GC0308: " fmt , ## arg);	
/* ------------------------------------------------------------------
	Basic structures
   ------------------------------------------------------------------*/


struct JZ_GC0308_dmaqueue {
	struct list_head       active;
	struct list_head       queued;
	struct timer_list      timeout;

	/* thread for generating video stream*/
	struct task_struct         *kthread;
	wait_queue_head_t          wq;
	/* Counters to control fps rate */
	int                        frame;
	int                        ini_jiffies;
};

static LIST_HEAD(JZ_GC0308_devlist);

struct JZ_GGC0308_dev {
	struct list_head           JZ_GC0308_devlist;

	struct mutex               lock;

	int                        users;

	/* various device info */
	struct video_device        *vfd;

	struct JZ_GC0308_dmaqueue       vidq;

	/* Several counters */
	int                        h,m,s,us,jiffies;
	char                       timestr[13];

	struct i2c_client *client;

};

struct JZ_GGC0308_dev gGgc0308dev_rgb;
struct JZ_GGC0308_dev gGgc0308dev_yuv;

/* ------------------------------------------------------------------
	DMA and thread functions
   ------------------------------------------------------------------*/
struct cim_config {
	int mclk;/* Master clock output to the sensor */
	int vsp; /* VSYNC Polarity:0-rising edge is active,1-falling edge is active */
	int hsp; /* HSYNC Polarity:0-rising edge is active,1-falling edge is active */
	int pcp; /* PCLK Polarity:0-rising edge sampling,1-falling edge sampling */
	int dsm; /* Data Sampling Mode:0-CCIR656 Progressive Mode, 1-CCIR656 Interlace Mode,2-Gated Clock Mode,3-Non-Gated Clock Mode */
	int pack; /* Data Packing Mode: suppose received data sequence were 0x11 0x22 0x33 0x44, then packing mode will rearrange the sequence in the RXFIFO:
0: 0x11 0x22 0x33 0x44
1: 0x22 0x33 0x44 0x11
2: 0x33 0x44 0x11 0x22
3: 0x44 0x11 0x22 0x33
4: 0x44 0x33 0x22 0x11
5: 0x33 0x22 0x11 0x44
6: 0x22 0x11 0x44 0x33
7: 0x11 0x44 0x33 0x22
			   */
	int inv_dat;    /* Inverse Input Data Enable */
	int frc;        /* Frame Rate Control:0-15 */
	int trig;   /* RXFIFO Trigger: 
0: 4
1: 8
2: 12
3: 16
4: 20
5: 24
6: 28
7: 32
				 */
};

static struct cim_config cim_cfg[] = {
/*
	{20000000, 1, 0, 0, 2, 2, 0, 0, 1},
	{20000000, 1, 0, 1, 2, 2, 0, 1, 1}
*/
//	{24000000, 0, 0, 0, 2, 4, 0, 0, 1},//yuv  {24000000, 0, 0, 0, 2, 4, 0, 0, 1},
//	{24000000, 0, 0, 0, 2, 4, 0, 0, 1}//rgb  {24000000, 0, 0, 0, 2, 4, 0, 0, 1},  save rgb {24000000, 0, 0, 0, 2, 2, 0, 0, 1}
	{22000000, 0, 0, 0, 2, 4, 0, 0, 1},//yuv  {24000000, 0, 0, 0, 2, 4, 0, 0, 1},
	{22000000, 0, 0, 0, 2, 4, 0, 0, 1}//rgb  {24000000, 0, 0, 0, 2, 4, 0, 0, 1},  save rgb {24000000, 0, 0, 0, 2, 2, 0, 0, 1}

};

#if 0
#else
typedef struct{
	unsigned int cfg;
	unsigned int ctrl;
	unsigned int size;
	unsigned int offs;
	unsigned int packed:1,
		fmt_422:1,
		reserved:30;
} cim_config_t;
#endif

//extern void cim_power_on(void);
extern void cim_scan_sensor(void);
extern void sensors_make_default(void);
static void sensor_set_i2c_speed(struct i2c_client *, unsigned long);

static int sensor_read_reg2(struct i2c_client *client, unsigned char reg,unsigned char *value);

static int sensor_read_all_regs(void);

static int sensor_write_reg2(struct i2c_client *client, unsigned char reg, unsigned char val);

#ifdef FRAMERATE_TEST
void ShowRGBVideo(int Index);
void ShowYUVVideo(int Index);
#endif
/*==========================================================================
 * CIM print operations
 *========================================================================*/
static void cim_print_regs(int id)
{
	printk("REG_CIM_CFG \t= \t0x%08x\n", REG_CIM_CFG(id));
	printk("REG_CIM_CTRL \t= \t0x%08x\n", REG_CIM_CTRL(id));
#ifdef CONFIG_SOC_JZ4760B
	printk("REG_CIM_CTRL2 \t= \t0x%08x\n", REG_CIM_CTRL2);
#endif
	printk("REG_CIM_STATE \t= \t0x%08x\n", REG_CIM_STATE(id));
	printk("REG_CIM_IID \t= \t0x%08x\n", REG_CIM_IID(id));
	printk("REG_CIM_DA \t= \t0x%08x\n", REG_CIM_DA(id));
	printk("REG_CIM_FA \t= \t0x%08x\n", REG_CIM_FA(id));
	printk("REG_CIM_FID \t= \t0x%08x\n", REG_CIM_FID(id));
	printk("REG_CIM_CMD \t= \t0x%08x\n", REG_CIM_CMD(id));
	printk("REG_CIM_SIZE \t= \t0x%08x\n", REG_CIM_SIZE(id));
	printk("REG_CIM_OFFSET \t= \t0x%08x\n", REG_CIM_OFFSET(id));
#ifdef CONFIG_SOC_JZ4760B
	printk("REG_CIM_YFA \t= \t%#08x\n", REG_CIM_YFA);
	printk("REG_CIM_YCMD \t= \t%#08x\n", REG_CIM_YCMD);
	printk("REG_CIM_CBFA \t= \t%#08x\n", REG_CIM_CBFA);
	printk("REG_CIM_CBCMD \t= \t%#08x\n", REG_CIM_CBCMD);
	printk("REG_CIM_CRFA \t= \t%#08x\n", REG_CIM_CRFA);
	printk("REG_CIM_CRCMD \t= \t%#08x\n", REG_CIM_CRCMD);
#endif
}


/* CIM start capture data */
static void thcim_start(void)
{
	__cim_disable(0);
	//__cim_set_da((unsigned int)virt_to_phys((void *)cim_frame_desc[cim_tran_buf_id]));
	__cim_set_da(0, virt_to_phys(cim_frame_desc[0]));
	//__cim_clear_state();	// clear state register
/*
	REG_CIM_STATE(0) = 0x0;

	//__cim_reset_rxfifo();	// resetting rxfifo
	REG_CIM_CTRL(0) |= CIM_CTRL_DMA_EN;
	REG_CIM_CTRL(0) |= CIM_CTRL_RXF_RST;
	REG_CIM_CTRL(0) &= ~CIM_CTRL_DMA_EN;

	//__cim_unreset_rxfifo();
	REG_CIM_CTRL(0) |= CIM_CTRL_DMA_EN;
	REG_CIM_CTRL(0) &= ~CIM_CTRL_RXF_RST;
	REG_CIM_CTRL(0) &= ~CIM_CTRL_DMA_EN;
	
	//__cim_enable_dma();	// enable dma
	REG_CIM_CTRL(0) |= CIM_CTRL_DMA_EN;
	REG_CIM_CTRL(0) &= ~CIM_CTRL_RXF_RST;
	REG_CIM_CTRL(0) |= CIM_CTRL_DMA_EN;

	__cim_enable(0);
	//printk("CIM start!!!!\n");
*/
	__cim_clear_state(0);	// clear state register
	__cim_reset_rxfifo(0);	// resetting rxfifo
	__cim_unreset_rxfifo(0);
	__cim_enable_dma(0);
	__cim_enable(0);

}

static void cim_stop(void)
{
	__cim_disable(0);
	__cim_clear_state(0);
}

static struct cim_desc *get_desc_list(void)
{
	int i;
	struct cim_desc *desc_list_head __attribute__ ((aligned (sizeof(struct cim_desc))));
	struct cim_desc *desc_list_tail __attribute__ ((aligned (sizeof(struct cim_desc))));
	struct cim_desc *p_desc __attribute__ ((aligned (sizeof(struct cim_desc))));
	int frm_id = 0;

	desc_list_head = desc_list_tail = NULL;

	/*Prepare CIM_BUF_NUM-1 link descriptors for getting sensor data*/

	cim_frame_desc[0] = (struct cim_desc *)kmalloc(sizeof(struct cim_desc), GFP_KERNEL);
	cim_frame_desc[0]->framebuf = virt_to_phys(my_frm_buf[0]);
	cim_frame_desc[0]->nextdesc = (unsigned int)NULL;
	cim_frame_desc[0]->frameid = frm_id;
	cim_frame_desc[0]->dmacmd = (FRM_BUF_LENGTH >> 2) | CIM_CMD_EOFINT | CIM_CMD_STOP;

	frm_id++;
	desc_list_head = cim_frame_desc[0];
	cim_frame_desc[0]->nextdesc = virt_to_phys(desc_list_head);
	
	for (i = 1; i < FRM_BUF_NUM-1; i++) 
	{
		p_desc = (struct cim_desc *)kmalloc(sizeof(struct cim_desc), GFP_KERNEL);
		if (NULL == p_desc){
			return NULL;
		}
	
		if (desc_list_head == NULL)
		{
			printk("Page_list_head returns NULL\n");
			desc_list_head = p_desc;
		}
		else
			cim_frame_desc[i-1]->nextdesc = virt_to_phys(p_desc);

		desc_list_tail = p_desc;
		desc_list_tail->nextdesc = virt_to_phys(desc_list_head);
		desc_list_tail->framebuf = virt_to_phys(my_frm_buf[i]);
		desc_list_tail->frameid = frm_id;
		desc_list_tail->dmacmd = FRM_BUF_LENGTH >> 2;
		desc_list_tail->dmacmd = FRM_BUF_LENGTH >> 2;

		desc_list_tail->dmacmd |= CIM_CMD_EOFINT | CIM_CMD_STOP;
		cim_frame_desc[i] = desc_list_tail;
		frm_id++;
	}

	if (FRM_BUF_NUM>1)
	{
		/*Prepare one non-linked descriptor for application reading*/
		cim_frame_desc[FRM_BUF_NUM-1] = (struct cim_desc *)kmalloc(sizeof(struct cim_desc), GFP_KERNEL);
		cim_frame_desc[FRM_BUF_NUM-1]->framebuf = virt_to_phys(my_frm_buf[FRM_BUF_NUM-1]);
		cim_frame_desc[FRM_BUF_NUM-1]->nextdesc = virt_to_phys(desc_list_head);
		cim_frame_desc[FRM_BUF_NUM-1]->frameid = frm_id;
/*		cim_frame_desc[FRM_BUF_NUM-1]->dmacmd = (FRM_BUF_LENGTH >> 2) | CIM_CMD_EOFINT | CIM_CMD_STOP;*/
			cim_frame_desc[FRM_BUF_NUM-1]->dmacmd = (FRM_BUF_LENGTH >> 2) | CIM_CMD_EOFINT | CIM_CMD_STOP;
		cim_frame_desc[FRM_BUF_NUM-2]->nextdesc = virt_to_phys(cim_frame_desc[FRM_BUF_NUM-1]);
	}

	for (i = 0; i < FRM_BUF_NUM; i++)
	{
		dma_cache_wback((unsigned long)(cim_frame_desc[i]), sizeof(struct cim_desc));
	}
	return desc_list_head;
}
static int thcim_fb_alloc(void)
{
	//printk("cim_fb_alloc!\n");

	first_cim_desc = get_desc_list();
	
	if (first_cim_desc == NULL)
	{
		printk("The first cim_desc not exit!\n");
		return -ENOMEM;
	}

	return 0;
}


#define FRAMERATE_TEST
#undef FRAMERATE_TEST

#ifdef FRAMERATE_TEST
void ShowYUVVideo(int Index);
struct timer_list timer_frameratetest;
unsigned char gRGB_frame = 0;
unsigned char gYUV_frame = 0;
static void frameratetest_timeout(void)
{
	printk("RGB frames is:%d and YUV frame is%d\n",gRGB_frame,gYUV_frame);
	gYUV_frame = 0;
	gRGB_frame = 0;
	
	mod_timer(&timer_frameratetest, jiffies + BUFFER_TIMEOUT);
}
static int frameratetest_timer_init(void)
{
	timer_frameratetest.function = frameratetest_timeout;

	init_timer(&timer_frameratetest);

	mod_timer(&timer_frameratetest, jiffies + BUFFER_TIMEOUT);

	return 0;
}

#define _R_16(x) (((x&0x00F8)<<16))
#define _B_16(x) (((x&0x1F00)>>5))
#define _G_16(x) ((((x&0x0007)<<13)|((x&0xE000)>>3)))

#define _RGB565to888(x) ( (_R_16(x))|(_G_16(x))|(_B_16(x)) )

void ShowRGBVideo(int Index)
{
    int i,j;
	
	unsigned short *src = NULL;
	unsigned int *dst = NULL;

    src = (unsigned short *)my_frm_buf[Index * 2];
    dst = (unsigned int *)test_lcd_frame0;

	for(i = 0;i < 240;i ++)
		for(j = 0;j < 320;j ++)
			dst[320 * i + j] = _RGB565to888(src[640 * i * 2  + j * 2]);
}
void ShowYUVVideo(int Index)
{
    int i,j;
	
	unsigned short *src = NULL;
	unsigned int *dst = NULL;
	unsigned char tmp = 0;

    src = (unsigned short *)my_frm_buf[Index * 2 + 1];
    dst = (unsigned int *)test_lcd_frame0;

	for(i = 0;i < 240;i ++)
		for(j = 0;j < 320;j ++)
		{
			tmp = src[640 * i * 2  + j * 2];
			dst[320 * i + j] = tmp|(tmp<<8)|(tmp<<16);
		}
}
#endif

#ifdef CIM_DMA_INTERRUPT

static irqreturn_t cim_irq_handler(int irq, void *dev_id)
{
	static int sensor_select = CIM_NULL;
	int i;
	unsigned int state = REG_CIM_STATE(0);
	unsigned long flags;
	if (state & CIM_STATE_DMA_EOF) 
	{	
	    	spin_lock(&fresh_lock);
		yuv_updated = true;
		frame_index++;												
		//printk("write_index1 = %d read_index1 = %d\n",yuv_write_index,yuv_read_index);
		yuv_write_index = GET_NEWEST_INDEX(yuv_read_index,yuv_write_index);
		//printk("write_index2 = %d read_index2 = %d\n",yuv_write_index,yuv_read_index);
		spin_unlock(&fresh_lock);			
		wake_up_interruptible(&wq_out_yuv);
		__cim_disable(0);						
		__cim_set_da(0, virt_to_phys(cim_frame_desc[yuv_write_index]));					
		REG_CIM_STATE(0) &= ~CIM_STATE_DMA_EOF;
	}
	else if ( state & CIM_STATE_RXF_OF)
	{
		printk("OverFlow interrupt!\n");

		__cim_disable(0);
	}

	for (i = 0; i < FRM_BUF_NUM; i++)
		dma_cache_wback((unsigned long)(cim_frame_desc[i]), sizeof(struct cim_desc));

	__cim_reset_rxfifo(0);
	__cim_unreset_rxfifo(0);
	__cim_clear_state(0);
	__cim_enable(0);

	return IRQ_HANDLED;
}

#endif

static int put_free_buf(unsigned char * buf,int length) 
{
	static int order =0;
	order=get_order(length);

	free_pages((unsigned long)buf, order);
	
	return 0;
}
static int JZ_GGC0308_open(struct file *file)
{
	int minor = iminor(file->f_dentry->d_inode);
	printk( "JZ_GGC0308: open called minor: %d\n", minor);
/*
	//Just can be enabled once!
	if(minor == 21)
		enable_irq(IRQ_CIM);
*/
	return 0;
}
static int JZ_GGC0308_release(struct file *file)
{
	int minor = iminor(file->f_dentry->d_inode);	
	return 0;
}
static int JZ_GGC0308_mmap( struct file *file, struct vm_area_struct * vma)
{
	static int yuv_buf_index = 0;
	unsigned long jz_off;
	printk("!!!!!!!!!!!!!!!!JZ_GGC0308_mmap !!!!iminor(file) %d\n",iminor(file->f_dentry->d_inode));	
 	jz_off = virt_to_phys(my_frm_buf[yuv_buf_index]);
	yuv_buf_index=(yuv_buf_index+1)%FRM_BUF_NUM;
	vma->vm_pgoff = jz_off >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	printk("JZ_GGC0308_mmap !!!!\n");
	if(remap_pfn_range(vma, vma->vm_start, jz_off >> PAGE_SHIFT,
					vma->vm_end - vma->vm_start,
					vma->vm_page_prot))
	return -EAGAIN;
}
#define ERROR_TF_EMPTY_TIMEOUT -111
static int set_sensor_reg(int selct_cim, unsigned char reg, unsigned char data)
{
	int ret = 0;
	unsigned char cnt = 0;
	//unsigned char cnt1 = 0;
	
	const unsigned char trytimes = 20;

	if(selct_cim==IMINOR_GC0308)
	{
		while(cnt ++  < trytimes)
		{
		   ret = sensor_write_reg2(gGgc0308dev_yuv.client, reg, data);

			if(!ret)
				break;
				
			else if(ERROR_TF_EMPTY_TIMEOUT == ret)
				{
					//jz_wdt_ping();
					printk("Reg is:0x%x,and data is:0x%x\n",reg,data);
					
					sensor_init_i2c(gGgc0308dev_yuv.client, 0);
					mdelay(100);

					return 0;

				 }
			else
				{				
					//erro_times++;   //add by dy
					//printk("erro_times=%d\n",erro_times);  //add by dy
					mdelay(10);
				}
		}

	}
	else if(selct_cim==IMINOR_NULL)
	{
		while(cnt ++  < trytimes)
		{
			if(!(ret = sensor_write_reg2(gGgc0308dev_rgb.client, reg, data)))
				break;
			else
				mdelay(10);
		}

	}
	if(!ret)
		ret = 0;
	
	else
		{
			ret = -1;
			printk("Error!!!Try times is :%d\n",cnt);
			printk("Reg is:0x%x,and data is:0x%x\n",reg,data);
		}

	return ret;
}
static int get_sensor_reg(int selct_cim, unsigned char reg,unsigned char* date)
{
		unsigned char *regVal = date;
		int ret = 0;
		unsigned char cnt = 0;
		const unsigned char trytimes = 20;
				
		if(selct_cim==IMINOR_GC0308)
		{
			while(cnt ++  < trytimes)
			{
				if(!(ret = sensor_read_reg2(gGgc0308dev_yuv.client, reg, regVal)))
					break;
				else
					mdelay(1);
			}
		
		}
		else if(selct_cim==IMINOR_NULL)
		{		
			while(cnt ++  < trytimes)
			{
				if(!(ret = sensor_read_reg2(gGgc0308dev_rgb.client, reg, regVal)))
					break;
				else
					mdelay(1);
			}
		
		}
		
		if(!ret)
			ret = 0;
		else
			{
				ret = -1;
				printk("try times is :%d\n",cnt);
			}

		return ret;
}

static int JZ_GGC0308_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
//	printk("KDebug: %s, cmd = 0x%x, arg = 0x%x \n", __func__, cmd, arg);
	switch (cmd) {
	case VIDIOC_G_FMT:
	{
		struct v4l2_format *fmt = (struct v4l2_format *)arg;
		int type = fmt->type;

		printk( " VIDIOC_G_FMT\n" );

		memset(fmt, 0, sizeof(*fmt));
		fmt->type = type;
		return 12;

	}
	break;
	case VIDIOC_S_SENSOR_REG:
	{
		unsigned long flag;
		int ret = 0;
		unsigned short tmp = 0;
		unsigned char reg_addr,reg_value;
		
		get_user(tmp,(unsigned short *)arg);

		reg_addr = (unsigned char)((tmp & 0xff00)>>8);
		reg_value = (unsigned char)(tmp & 0xff);

        //printk("set sensor:addr is 0x%x value is 0x%x\n",reg_addr,reg_value);
		
		spin_lock_irqsave(&fresh_lock,flag);
		
		ret = set_sensor_reg(iminor(file->f_dentry->d_inode),reg_addr, reg_value);
		
		spin_unlock_irqrestore(&fresh_lock,flag);

		return ret;

	}
	break;
	case VIDIOC_G_SENSOR_REG:
	{
		unsigned long flag;
		int ret = 0;
		unsigned short tmp = 0;
		unsigned char reg_addr,reg_value;
		
		get_user(tmp,(unsigned short *)arg);

		reg_addr = (unsigned char)((tmp & 0xff00)>>8);
		
		spin_lock_irqsave(&fresh_lock,flag);
		
		ret = get_sensor_reg(iminor(file->f_dentry->d_inode), reg_addr, &reg_value);
		
		spin_unlock_irqrestore(&fresh_lock,flag);

		tmp = (reg_addr << 8) | reg_value;

		put_user(tmp,(unsigned short *)arg);

		return ret;

	}
	break;

	case VIDIOC_S_EXPOSURE:
	{	
		unsigned short exposure = 0;
		int ret = 0;
		unsigned char h_data, l_data;
	    unsigned long flag;	
		
		get_user(exposure,(unsigned short *)arg);

#if 1		// add #if ... #endif, yjt, 20130624
		if(exposure<MIN_EXP)
			exposure=MIN_EXP;
		else
			exposure= ((exposure+(MIN_STEP/2))/MIN_STEP)*MIN_STEP;
#endif
		//printk("expusure=%d\n",exposure);
		
		h_data = (unsigned char)((exposure & 0xff00)>>8);
		l_data = (unsigned char)(exposure & 0xff);
		
		//down(&SENSOR_REG_mutex);
	    	spin_lock_irqsave(&fresh_lock,flag);
		ret = set_sensor_reg(iminor(file->f_dentry->d_inode), AECH, h_data);
		//mdelay(30);
		ret = set_sensor_reg(iminor(file->f_dentry->d_inode), AECL, l_data);
		spin_unlock_irqrestore(&fresh_lock,flag);
	
		return ret;
	}
	case VIDIOC_QUERYBUF:
	{
		printk("JZ_GGC0308_ioctl VIDIOC_QUERYBUF \n");
		return 13;
	}
	break;
	case VIDIOC_QBUF:
	{
		return 14;
	}
	break;

	case VIDIOC_DQBUF:
	{
		//int *arg_num = (int *)arg;
		long time=2;	
		int retindex;
		unsigned long flag;
	    	spin_lock_irqsave(&fresh_lock,flag);
		if(yuv_updated)
			retindex = yuv_read_index = GET_NEWEST_INDEX(yuv_read_index,yuv_write_index);
	
		else
			retindex = -1;
		yuv_updated = false;
		spin_unlock_irqrestore(&fresh_lock,flag);		
		if(retindex<0)
		{
			time = interruptible_sleep_on_timeout(&wq_out_yuv,1000);
	
                	if (time >= 1 )
			{
	    			spin_lock_irqsave(&fresh_lock,flag);
				if(yuv_updated)
					retindex = yuv_read_index = GET_NEWEST_INDEX(yuv_read_index,yuv_write_index);
			
				else
					retindex = -1;
				yuv_updated = false;
				spin_unlock_irqrestore(&fresh_lock,flag);	
			}
		}
	  	put_user(retindex,(int *)arg);		
		if(retindex<0)
			return -1;
		else
			return 15;
	}
	case VIDIOC_INIT_ALLSENOSER:
		printk( " VIDIOC_RESET_SENSOR \n");
		sensor_init_i2c(gGgc0308dev_yuv.client, 0);						
		break;					
	case VIDIOC_G_EXPOSURE:
		{
			unsigned char h_data = 0;
			unsigned char l_data = 0;
			unsigned short exposureValue = 0;
			unsigned long flag;

     	   //printk("Getting sensor Exposure Value!\n");

		    spin_lock_irqsave(&fresh_lock,flag);

			get_sensor_reg(iminor(file->f_dentry->d_inode), AECL, &l_data);
			get_sensor_reg(iminor(file->f_dentry->d_inode), AECH, &h_data);
			
			spin_unlock_irqrestore(&fresh_lock,flag);			
			
			exposureValue = ((unsigned short)(h_data) << 8) | (unsigned short)(l_data);
			
			put_user(exposureValue,(unsigned short *)arg);
			printk("Exposure Register value L is:0x%x and H is :0x%x\n",l_data,h_data);						
		
	   }
   			break;

	default:
		printk( "%d: UNKNOWN ioctl cmd: 0x%x\n",(unsigned int)arg, cmd);
		printk("VIDIOC_DQBUF: 0x%x, 0x%x\n", VIDIOC_DQBUF, _IOWR('V', 17, struct v4l2_buffer));
		return -ENOIOCTLCMD;
	break;

	}	
	return 0;
}

static void cim_init_config60(cim_config_t *cim_cfg)
{
	cim_cfg->cfg = 0x00024c26; // cur_desc->cfg_info.configure_register;
	cim_cfg->cfg &=~CIM_CFG_DMA_BURST_TYPE_MASK;
#if defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_SOC_JZ4770) || defined(CONFIG_SOC_JZ4775)
	cim_cfg->cfg |= CIM_CFG_DMA_BURST_INCR32;// (n+1)*burst = 2*16 = 32 <64
	cim_cfg->ctrl =  CIM_CTRL_DMA_SYNC | CIM_CTRL_FRC_1;
#elif defined(CONFIG_SOC_JZ4760)
	cim_cfg->cfg |= CIM_CFG_DMA_BURST_INCR16;
	cim_cfg->ctrl =  CIM_CTRL_DMA_SYNC | CIM_CTRL_FRC_1 | (1<<CIM_CTRL_RXF_TRIG_BIT);// (n+1)*burst = 2*16 = 32 <64
#else
	cim_cfg->cfg |= CIM_CFG_DMA_BURST_INCR8;
	cim_cfg->ctrl = CIM_CTRL_FRC_1 | CIM_CTRL_RXF_TRIG_8 | CIM_CTRL_FAST_MODE; // 16 < 32
#endif

	printk("ctrl = %x\n", cim_cfg->ctrl);
	return;
}

static void cim_config60(cim_config_t *c)
{

	//REG_CIM_CFG(0)  = c->cfg;
	REG_CIM_CTRL(0) = c->ctrl;
//	REG_CIM_SIZE(0) = c->size;
//	REG_CIM_OFFSET(0) = c->offs;
	REG_CIM_CFG(0)	= 0x00020c46/*0x00024c56*/;  // 0,1,2,3,4,5,6,7
//	REG_CIM_CTRL(0)	= 0x00004984;
	REG_CIM_SIZE(0)	= 0x011001e0;
	REG_CIM_OFFSET(0)	= 0x00680050;

	printk("REG_CIM_CTRL = %x, ctrl = %x \n", REG_CIM_CTRL(0), c->ctrl);
	printk("REG_CIM_CFG = %x\n", REG_CIM_CFG(0));
	printk("REG_CIM_SIZE = %x\n", REG_CIM_SIZE(0));
	printk("REG_CIM_OFFSET = %x\n", REG_CIM_OFFSET(0));

#if defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_SOC_JZ4770) || defined(CONFIG_SOC_JZ4775)

	__cim_enable_bypass_func(0);

	__cim_enable_auto_priority(0);

	__cim_enable_emergency(0);

	/* 0, 1, 2, 3
	 * 0: highest priority
	 * 3: lowest priority
	 * 1 maybe best for SEP=1
	 * 3 maybe best for SEP=0
	 */
	//__cim_set_opg(0);
	__cim_set_opg(0, 1);
	//__cim_set_opg(2);
	//__cim_set_opg(3);

	__cim_enable_priority_control(0);
	//__cim_disable_priority_control();


	c->packed = 1;
	if (c->packed) {
		__cim_disable_sep(0);
	} else {
		__cim_enable_sep(0);
	} 
#endif

	__cim_enable_dma(0);

#ifdef CIM_SAFE_DISABLE
	__cim_enable_vdd_intr(0);
#else
	__cim_disable_vdd_intr(0);
#endif

#ifdef CIM_INTR_SOF_EN
	__cim_enable_sof_intr(0);
#else
	__cim_disable_sof_intr(0);
#endif

#define CIM_INTR_EOF_EN 
#ifdef CIM_INTR_EOF_EN
	__cim_enable_eof_intr(0);
#else
	__cim_disable_eof_intr(0);
#endif

#ifdef CIM_INTR_STOP_EN
	__cim_enable_stop_intr(0);
#else
	__cim_disable_stop_intr(0);
#endif

/*
#ifdef CIM_INTR_TRIG_EN
	__cim_enable_trig_intr(0);
#else
	__cim_disable_trig_intr(0);
#endif
*/

#define CIM_INTR_OF_EN 
#ifdef CIM_INTR_OF_EN
	__cim_enable_rxfifo_overflow_intr(0);
#else
	__cim_disable_rxfifo_overflow_intr(0);
#endif

#ifdef CIM_INTR_EEOF_EN
	__cim_set_eeof_line(0, 100);
	__cim_enable_eeof_intr(0);
#else
#if !defined(CONFIG_SOC_JZ4750)
	__cim_set_eeof_line(0, 0);
	__cim_disable_eeof_intr(0);
#endif
#endif

#ifdef CONFIG_VIDEO_CIM_FSC
	__cim_enable_framesize_check_error_intr(0);
#else
	__cim_disable_framesize_check_error_intr(0);
#endif
}

static void cim_print_regs60(int id)
{
	printk("REG_CIM_CFG \t= \t0x%08x\n", REG_CIM_CFG(id));
	printk("REG_CIM_CTRL \t= \t0x%08x\n", REG_CIM_CTRL(id));
#ifdef CONFIG_SOC_JZ4760B
	printk("REG_CIM_CTRL2 \t= \t0x%08x\n", REG_CIM_CTRL2);
#endif
	printk("REG_CIM_STATE \t= \t0x%08x\n", REG_CIM_STATE(id));
	printk("REG_CIM_IID \t= \t0x%08x\n", REG_CIM_IID(id));
	printk("REG_CIM_DA \t= \t0x%08x\n", REG_CIM_DA(id));
	printk("REG_CIM_FA \t= \t0x%08x\n", REG_CIM_FA(id));
	printk("REG_CIM_FID \t= \t0x%08x\n", REG_CIM_FID(id));
	printk("REG_CIM_CMD \t= \t0x%08x\n", REG_CIM_CMD(id));
	printk("REG_CIM_SIZE \t= \t0x%08x\n", REG_CIM_SIZE(id));
	printk("REG_CIM_OFFSET \t= \t0x%08x\n", REG_CIM_OFFSET(id));
#ifdef CONFIG_SOC_JZ4760B
	printk("REG_CIM_YFA \t= \t%#08x\n", REG_CIM_YFA);
	printk("REG_CIM_YCMD \t= \t%#08x\n", REG_CIM_YCMD);
	printk("REG_CIM_CBFA \t= \t%#08x\n", REG_CIM_CBFA);
	printk("REG_CIM_CBCMD \t= \t%#08x\n", REG_CIM_CBCMD);
	printk("REG_CIM_CRFA \t= \t%#08x\n", REG_CIM_CRFA);
	printk("REG_CIM_CRCMD \t= \t%#08x\n", REG_CIM_CRCMD);
#endif
}

static int cim_device_config60(void)
{
	cim_config_t cim_cfg;
	
	cim_init_config60(&cim_cfg);
	cim_config60(&cim_cfg);
//	cim_print_regs60(0);
	
	return 0;
}


/*ZYL,2011.11.16*/
static int sensor_init_i2c(struct i2c_client *client, unsigned char st_num)
{
	int i = 0;
	int j = 0;

#if 0
	printk("Initalize RGB CMOS CAMMERA...... \n");
	

	// v7725reg
	for(i=0; i<sizeof(v7725reg)/sizeof(v7725reg[0]); i++)
	{
		if(0 != set_sensor_reg(IMINOR_NULL,v7725reg[i].reg,v7725reg[i].val))
			printk("RGB write reg error:0x%x\n",v7725reg[i].reg);
		
		
		if(v7725reg[i].reg == 0x12 || v7725reg[i].reg == 0x15 || v7725reg[i].reg == 0x0c)
			for(j=0;j<0x100000;j++);
		else
			for(j=0;j<0x10000;j++);
	}

	printk("Initalize YUV CMOS CAMMERA...... \n");
	
#endif

	// ov7740YUVreg

	for(i=0; i<sizeof(v7725YUVreg)/sizeof(v7725YUVreg[0]); i++)
	{
		//printk("reg=%x\n",v7725YUVreg[i].reg);
		if(0 != set_sensor_reg(IMINOR_GC0308,v7725YUVreg[i].reg,v7725YUVreg[i].val))
			printk("YUV write reg error:0x%x\n",v7725YUVreg[i].reg);


		//if(v7725YUVreg[i].reg == 0x12 || v7725YUVreg[i].reg == 0x15 || v7725YUVreg[i].reg == 0x0c)
		if(v7725YUVreg[i].reg == 0x12)
			//for(j=0;j<0x100000;j++);
			mdelay(3);
		else
			//for(j=0;j<0x10000;j++);
			udelay(100);
	}

#if 1
	//init AGC,add by dy	
	unsigned char temp1;
	mdelay(30);
	temp1= (unsigned char)(PRE_AGC&0xff);
	//printk("0x00=%x\n",temp1);
	set_sensor_reg(IMINOR_GC0308,AGCL, temp1);
	mdelay(30);
	unsigned char temp;
	get_sensor_reg(IMINOR_GC0308, AGCH, &temp);
	temp=temp|(PRE_AGC>>8&0x03);
	mdelay(30);
	set_sensor_reg(IMINOR_GC0308,AGCH, temp);
	//printk("0x15=%d\n",temp);
#endif
	
#ifdef SENSOR_READ_TEST	
	sensor_read_all_regs();
#endif

	return 0;
}
static int cim_start_first(void)
{
	int i;
	cim_device_config60();
	//golf_pin_cim_yuv();	
	//sensor_init_i2c(gGgc0308dev_rgb.client, 0);
	sensor_init_i2c(gGgc0308dev_yuv.client, 0);

	for (i = 0; i < FRM_BUF_NUM; i ++)	
	{
		static int jz_order = 0;		
		printk( "get_alloc_buf  %d\n", i);		
		jz_order=get_order(FRM_BUF_LENGTH);
		printk(" memory can be alloc!order is %x \n", jz_order);
		my_frm_buf[i] = (unsigned char *)__get_free_pages(GFP_KERNEL, jz_order);
		printk(" cim memory can be allocated and order is %x buf is %x \n", jz_order, (unsigned int)my_frm_buf[i]);
		mdelay(10);
	}
	thcim_fb_alloc();
	thcim_start();
	return 0;
}

//static const struct file_operations JZ_GC0308_fops = {
static const struct v4l2_file_operations JZ_GC0308_fops = {
	.owner		= THIS_MODULE,
	.open           = JZ_GGC0308_open,
	.release        = JZ_GGC0308_release,
//	.read           = JZ_GC0308_read,
//	.poll		= JZ_GC0308_poll,
	.ioctl          = JZ_GGC0308_ioctl,//video_ioctl2, /* V4L2 ioctl handler */
	.mmap           = JZ_GGC0308_mmap,
//	.llseek         = no_llseek,
};

static struct video_device JZ_GC0308_RGB = {
	.name		= "JZ_GC0308_RGB",
	//.type		= VID_TYPE_CAPTURE,
	.vfl_type	= VFL_TYPE_GRABBER,
	.fops           = &JZ_GC0308_fops,
	.minor		= IMINOR_NULL,
};

static struct video_device JZ_GC0308_YUV = {
	.name		= "JZ_GC0308_YUV",
	//.type		= VID_TYPE_CAPTURE,
	.vfl_type	= VFL_TYPE_GRABBER,//VID_TYPE_CAPTURE,
	.fops           = &JZ_GC0308_fops,
	.minor		= IMINOR_GC0308,
};


//#include "../../jz_cim_core.h"
#include "../../misc/jz_cim/jz_sensor.h"

static inline int sensor_i2c_master_send(struct i2c_client *client,const char *buf ,int count)
{
	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	msg.addr = client->addr;
//	printk("client->addr:%x\n", client->addr);
	msg.flags = client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = (char *)buf;
	ret = i2c_transfer(adap, &msg, 1);
//	printk("ret = i2c_transfer(adap, &msg, 1):%x\n", ret);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? count : ret;
}

static inline int sensor_i2c_master_recv(struct i2c_client *client, char *buf ,int count)
{
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;
	int ret;

	msg.addr = client->addr;
//	printk("client->addr:%x\n", client->addr);
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

static int sensor_write_reg2(struct i2c_client *client, unsigned char reg, unsigned char val)
{

	int ret;
	struct i2c_adapter *adap=client->adapter;
	struct i2c_msg msg;

	unsigned char buf[2] = {reg,val};

	msg.addr = client->addr;

//printk("addr=%x\n",client->addr);
	msg.flags = 0;
	msg.len = 2;
	msg.buf = buf;

	ret = i2c_transfer(adap, &msg, 1);

	/* If everything went ok (i.e. 1 msg transmitted), return #bytes
	   transmitted, else error code. */
	return (ret == 1) ? 0 : ret;

}

static int sensor_read_reg2(struct i2c_client *client, unsigned char reg,unsigned char *value)
{
	int ret;
	unsigned char *retval = value;


	ret = sensor_i2c_master_send(client,&reg,1);

	if (ret < 0)
	{
		//printk("%s: ret < 0   sensor_i2c_master_send ERR!\n", __FUNCTION__);
		return ret;		
	}

	if (ret != 1)
	{
		//printk("%s: ret != 1   sensor_i2c_master_send ERR!\n", __FUNCTION__);
		return -EIO;		
	}


	ret = sensor_i2c_master_recv(client, retval, 1);
	if (ret < 0) {
		//printk("%s: ret < 0\n", __FUNCTION__);
		return ret;
	}
	if (ret != 1) {
		//printk("%s: ret != 1\n", __FUNCTION__);
		return -EIO;
	}

	return 0;
}
static int sensor_read_all_regs()
{
	int i = 0;
	unsigned char regVal = 0;
	int ret = 0;
#if 0	
	printk("Below is RGB sensor' all register:\n");
	for(i=0; i<sizeof(v7725reg)/sizeof(v7725reg[0]); i++)
	{
		if(0 != (ret = get_sensor_reg(IMINOR_NULL, v7725reg[i].reg,&regVal)))
			printk("get sensor reg Err!  0x%x\n",v7725reg[i].reg);
			
		printk("0x%x,0x%x\n",v7725reg[i].reg,regVal);		
	}
#endif

	printk("Below is YUV sensor' all register:\n");
	for(i=0; i<sizeof(v7725YUVreg)/sizeof(v7725YUVreg[0]); i++)
	{
		if(0 != (ret = get_sensor_reg(IMINOR_GC0308, v7725YUVreg[i].reg,&regVal)))
			printk("get sensor reg Err!  0x%x\n",v7725YUVreg[i].reg);
			
		printk("0x%x,0x%x\n",v7725YUVreg[i].reg,regVal);		
	}

	return ret;
}
extern void  i2c_jz_setclk(struct i2c_client *client,unsigned long i2cclk);
/* It's seems ok without this function */
static void sensor_set_i2c_speed(struct i2c_client *client,unsigned long speed)
{
#if defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_JZ4760_F4760) || defined(CONFIG_JZ4810_F4810) || defined(CONFIG_SOC_JZ4770) || defined(CONFIG_SOC_JZ4775)
	i2c_jz_setclk(client,speed);
#endif
	//printk("set sensor i2c write read speed = %d hz\n",speed);
}


#include <asm/jzsoc.h> 

static void cim_power_off(void)
{
	cpm_stop_clock(CGM_CIM0);
}

static void cim_power_on(void)
{
	cpm_stop_clock(CGM_CIM0);
	cpm_set_clock(CGU_CIMCLK, 24000000);	// modify, yjt, 20130828, 48M -> 24M
	cpm_start_clock(CGM_CIM0);
}

static int gc03085_rgb_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct JZ_GGC0308_dev *dev = &gGgc0308dev_rgb;

	printk("in function %s\n", __FUNCTION__);

	//__gpio_as_output1(32*6+7);	// SENSOR RESET PIN
	reset_pin_cim_reset_off();
	
	sensor_set_i2c_speed(client, 100000);//set i2c speed : 100khz

//	__gpio_as_cim();
	__gpio_as_cim_8bit(0);

	cim_power_on();

//	CIM_SELECT_CTR_GPIO_INIT();

	dev->client = client;

	list_add_tail(&dev->JZ_GC0308_devlist, &JZ_GC0308_devlist);

	/* init video dma queues */
	init_waitqueue_head(&wq_out_rgb);
	//init_waitqueue_head(&wq_out_yuv);

	/* callbacks */
	JZ_GC0308_RGB.release = video_device_release;

	dev->vfd = &JZ_GC0308_RGB;

	ret = video_register_device(dev->vfd, VFL_TYPE_GRABBER, IMINOR_NULL);
	if(ret != 0){
		printk("21 video_register_device %d error\n\n", IMINOR_NULL);
	}else{
		printk("21 video_register_device %d OK\n\n", IMINOR_NULL);
		mdelay(100);
	}
		
	//cim_start_first();

	return ret;

}

static int gc03085_yuv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct JZ_GGC0308_dev *dev = &gGgc0308dev_yuv;

	printk("in function %s\n", __FUNCTION__);
	
	//__gpio_as_output1(32*6+7);	// SENSOR RESET PIN
	reset_pin_cim_reset_off();
	
	sensor_set_i2c_speed(client, 100000);//set i2c speed : 100khz

//	__gpio_as_cim();
	__gpio_as_cim_8bit(0);

	cim_power_on();

//	CIM_SELECT_CTR_GPIO_INIT();

	dev->client = client;

	list_add_tail(&dev->JZ_GC0308_devlist, &JZ_GC0308_devlist);

	/* init video dma queues */
	//init_waitqueue_head(&wq_out_rgb);
	init_waitqueue_head(&wq_out_yuv);

	//init_MUTEX(&SENSOR_REG_mutex);	


	JZ_GC0308_YUV.release = video_device_release;
	dev->vfd = &JZ_GC0308_YUV;
	ret = video_register_device(dev->vfd, VFL_TYPE_GRABBER, IMINOR_GC0308);
	if(ret != 0){
		printk("22 video_register_device %d error\n\n", IMINOR_GC0308);
	}else{
		printk("22 video_register_device %d OK\n\n", IMINOR_GC0308);
		mdelay(100);
	}	
		
	cim_start_first();
	
#ifdef CIM_DMA_INTERRUPT

	if(ret = request_irq(IRQ_CIM,cim_irq_handler,IRQF_DISABLED,"CIM",0))
	{
		printk("IRQ_CIM request failed!\n");
		return ret;
	}
	//disable_irq(IRQ_CIM);	
	//enable_irq(IRQ_CIM);
	spin_lock_init(&fresh_lock);
#endif

	// frameratetest_timer_init();
	
	return ret;

}

static const struct i2c_device_id gc0308_id[] = {
	/* name, private data to driver */
	{ "gc0308_rgb", 0},
	{ }	/* Terminating entry */
};

static const struct i2c_device_id gc0308_yuv_id[] = {
	{"gc0308_yuv", 0},
};


//MODULE_DEVICE_TABLE(i2c, gc0308_id);
#if 0
static struct i2c_driver gc0308_rgb_driver = {
	.probe		= gc03085_rgb_probe,
	/*.remove		= gc03085_remove,*/
	.id_table	= gc0308_id,
	.driver	= {
		.name = "gc0308_rgb",
		.owner = THIS_MODULE,
	},
};
#endif
	
static struct i2c_driver gc0308_yuv_driver = {
	.probe		= gc03085_yuv_probe,
	/*.remove		= gc03085_remove,*/
	.id_table	= gc0308_yuv_id,
	.driver	= {
		.name = "gc0308_yuv",
		.owner = THIS_MODULE,
	},
};

/* -----------------------------------------------------------------
	Initialization and module stuff
  ------------------------------------------------------------------*/

#if 1
static int __init JZ_GC0308_init(void)
{
	int err;
/*
#define LIGHT1_PIN	GPF(2)
	__gpio_as_output(LIGHT1_PIN);
	__gpio_clear_pin(LIGHT1_PIN);

	__gpio_as_output0(32*6+7);	// SENSOR RESET PIN
	mdelay(5);
*/

/*
	err = i2c_add_driver(&gc0308_rgb_driver);
	if(err)
	{
		printk("add gc0308_rgb failed\n");
		return err;
	}
*/
	//__gpio_as_output1(32 * 0 + 27);	// add, yjt, 20130624
	reset_pin_cim_reset_on();

	return i2c_add_driver(&gc0308_yuv_driver);

}

#else

static int __init JZ_GC0308_init(void)
{
#define LIGHT1_PIN	GPF(2)
	__gpio_as_output(LIGHT1_PIN);
	__gpio_clear_pin(LIGHT1_PIN);

	//__gpio_as_output0(32*6+7);	// SENSOR RESET PIN
	reset_pin_cim_reset_on();
	mdelay(5);

	return i2c_add_driver(&gc0308_rgb_driver);
}

static int __init JZ_GC0308_yuv_init(void)
{
	return i2c_add_driver(&gc0308_yuv_driver);

}

#endif

static void __exit JZ_GC0308_exit(void)
{
	printk("Now we're exit form:%s\n",__FUNCTION__);

	//video_unregister_device(&JZ_GC0308_RGB);
       video_unregister_device(&JZ_GC0308_YUV);
	kfree(cim_frame_desc);
	
	//i2c_del_driver(&gc0308_rgb_driver);
	i2c_del_driver(&gc0308_yuv_driver);
}


//module_init(JZ_GC0308_yuv_init);
module_init(JZ_GC0308_init);
module_exit(JZ_GC0308_exit);

MODULE_LICENSE("GPL");


