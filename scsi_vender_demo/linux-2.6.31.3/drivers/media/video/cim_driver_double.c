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

#include <linux/proc_fs.h>


/* add 2013_02_21*/
#include "cim_driver_double.h"
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

#define FRM_BUF_NUM 6
static unsigned char *my_frm_buf[FRM_BUF_NUM];	/*current trans buffer pointer*/

#define FRM_BUF_LENGTH		(640 * 480 * 2)

#define GOLF_SHOW_RGB	1
#define GOLF_SHOW_YUV	0
#define GOLF_FORMAL_PRJ
#undef GOLF_FORMAL_PRJ


#define CIM_RGB		0
#define CIM_YUV 	1

#define IMINOR_RGB 22
#define IMINOR_YUV 21

static wait_queue_head_t          wq_out_rgb;
static wait_queue_head_t          wq_out_yuv;

#define CIM_DMA_INTERRUPT
#undef CIM_DMA_INTERRUPT

#ifdef CIM_DMA_INTERRUPT

static struct semaphore SENSOR_REG_mutex;

static unsigned char rgb_read_index=0;
static unsigned char yuv_read_index=0;

static unsigned char gRGBImageBufIndex = 0;
static unsigned char gYUVImageBufIndex = 0;

int bRGBImageReady = 0;
int bYUVImageReady = 0;


#define ImageBufIndex_MAX 2
#define ImageBufIndex_MIN 0

#endif


#define TRUE    1
#define FALSE   0

//#define AECH 0X08
//#define AECL 0X10

#define reg_320_240  1
#undef reg_320_240
#ifdef  reg_320_240
frm_rgb_buf_length = 320*240*2;	
//frm_rgb_buf_length = 640*480*2;	
#endif


#define AECH 0X0f    //for 7740
#define AECL 0X10    //for 7740
#define AGCL 0X00    //for 7740
#define AGCH 0x15    //for 7740
#define PRE_AGC 50   //450
#define MIN_EXP 0
#define MIN_STEP 15

static spinlock_t fresh_lock;

/*
 * CIM DMA descriptor
 */
#if 0 
struct cim_desc {
	u32 nextdesc;   /* Physical address of next desc */
	u32 framebuf;   /* Physical address of frame buffer */
	u32 frameid;    /* Frame ID */ 
	u32 dmacmd;     /* DMA command */
	u32 pagenum;
	u32 reserve0;
	u32 reserve1;
	u32 reserve2;
};
#else
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
#endif

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

// extern unsigned char *lcd_frame0;  // del, yjt, 20130718

#define SENSOR_READ_TEST
#undef SENSOR_READ_TEST

/* end 2010_04_15*/

/* Wake up at about 30 fps */
#define WAKE_NUMERATOR 30
#define WAKE_DENOMINATOR 1001
#define BUFFER_TIMEOUT     msecs_to_jiffies(1000) 

#include "font.h"

#define JZ_OV7725_MAJOR_VERSION 0
#define JZ_OV7725_MINOR_VERSION 4
#define JZ_OV7725_RELEASE 0
#define JZ_OV7725_VERSION KERNEL_VERSION(JZ_OV7725_MAJOR_VERSION, JZ_OV7725_MINOR_VERSION, JZ_OV7725_RELEASE)

/* Declare static vars that will be used as parameters */
static unsigned int vid_limit = 16;	/* Video memory limit, in Mb */
static struct video_device JZ_OV7725_RGB;	/* Video device */
static struct video_device JZ_OV7725_YUV;	/* Video device */
static int video_nr = -1;		/* /dev/videoN, -1 for autodetect */

/* supported controls */

/*
#define dprintk(level,fmt, arg...)					\
	do {								\
		if (JZ_OV7725.debug >= (level))				\
			printk(KERN_DEBUG "JZ_OV7725: " fmt , ## arg);	\
	} while (0)
*/
#define dprintk(level,fmt, arg...)					\
			printk( "JZ_OV7725: " fmt , ## arg);	
/* ------------------------------------------------------------------
	Basic structures
   ------------------------------------------------------------------*/


struct JZ_OV7725_dmaqueue {
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

static LIST_HEAD(JZ_OV7725_devlist);

struct JZ_GOV7725_dev {
	struct list_head           JZ_OV7725_devlist;

	struct mutex               lock;

	int                        users;

	/* various device info */
	struct video_device        *vfd;

	struct JZ_OV7725_dmaqueue       vidq;

	/* Several counters */
	int                        h,m,s,us,jiffies;
	char                       timestr[13];

	struct i2c_client *client;

};

struct JZ_GOV7725_dev gGov7725dev_rgb;
struct JZ_GOV7725_dev gGov7725dev_yuv;

/* ------------------------------------------------------------------
	DMA and thread functions
   ------------------------------------------------------------------*/


/* !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! */

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
typedef struct
{
	u32 cfg;
	u32 ctrl;
	u32 mclk;
} cim_config_t;
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

// extern void cim_power_on(void);		// del, yjt, 20130718, this func exist in this file
extern void cim_scan_sensor(void);
static void cim_power_off(void);
extern void sensors_make_default(void);
static void sensor_set_i2c_speed(struct i2c_client *, unsigned long);

static int sensor_read_reg2(struct i2c_client *client, unsigned char reg,unsigned char *value);

static int sensor_read_all_regs(void);

static int sensor_write_reg2(struct i2c_client *client, unsigned char reg, unsigned char val);

static int cim_select_cpu(void);

void ShowRGBVideo(int Index);

void ShowYUVVideo(int Index);

/*==========================================================================
 * CIM print operations
 *========================================================================*/
static void cim_print_regs(void)
{
	printk("REG_CIM_CFG \t= \t0x%08x\n", REG_CIM_CFG(1));
	printk("REG_CIM_CTRL \t= \t0x%08x\n", REG_CIM_CTRL(1));
#ifdef CONFIG_SOC_JZ4760B
	printk("REG_CIM_CTRL2 \t= \t0x%08x\n", REG_CIM_CTRL2);
#endif
	printk("REG_CIM_STATE \t= \t0x%08x\n", REG_CIM_STATE(1));
	printk("REG_CIM_IID \t= \t0x%08x\n", REG_CIM_IID(1));
	printk("REG_CIM_DA \t= \t0x%08x\n", REG_CIM_DA(1));
	printk("REG_CIM_FA \t= \t0x%08x\n", REG_CIM_FA(1));
	printk("REG_CIM_FID \t= \t0x%08x\n", REG_CIM_FID(1));
	printk("REG_CIM_CMD \t= \t0x%08x\n", REG_CIM_CMD(1));
	printk("REG_CIM_SIZE \t= \t0x%08x\n", REG_CIM_SIZE(1));
	printk("REG_CIM_OFFSET \t= \t0x%08x\n", REG_CIM_OFFSET(1));
#ifdef CONFIG_SOC_JZ4760B
	printk("REG_CIM_YFA \t= \t%#08x\n", REG_CIM_YFA);
	printk("REG_CIM_YCMD \t= \t%#08x\n", REG_CIM_YCMD);
	printk("REG_CIM_CBFA \t= \t%#08x\n", REG_CIM_CBFA);
	printk("REG_CIM_CBCMD \t= \t%#08x\n", REG_CIM_CBCMD);
	printk("REG_CIM_CRFA \t= \t%#08x\n", REG_CIM_CRFA);
	printk("REG_CIM_CRCMD \t= \t%#08x\n", REG_CIM_CRCMD);
#endif
}


static int rgb_read_system=0;
static int rgb_read_cim=1;
static int rgb_read_bak=2;
static spinlock_t rgb_lock;

static int yuv_read_system=0;
static int yuv_read_cim=1;
static int yuv_read_bak=2;
static spinlock_t yuv_lock;

//spinlock_t exp_lock; //add by dy 20130325
//DECLARE_MUTEX(exp_sem);//add by dy 20130325
	
#define TIMEOUT     msecs_to_jiffies(10)  

static struct JZ_OV7725_dmaqueue  dma_que;


static void JZ_OV7725_vid_timeout(unsigned long data)
{
	mod_timer(&dma_que.timeout, jiffies + TIMEOUT);

	cim_select_cpu();
}
static int timer_init(void)
{
	printk("\n cim_driver ov7725 timer_init \n");

	dma_que.timeout.function = JZ_OV7725_vid_timeout;

	init_timer(&dma_que.timeout);

	mod_timer(&dma_que.timeout, jiffies + BUFFER_TIMEOUT);

	return 0;
}

/*ZYL,2011.12.07*/
static int cim_select_cpu(void)
{
	static int sim_slecte = 1;  //add by dy 20130325
	//static int cim_tran_id = 0;
	int i;
	unsigned long flag;
	//u32 state = REG_CIM_STATE;
		
	if (REG_CIM_STATE(1) & CIM_STATE_DMA_EOF) 
	{	

		if (sim_slecte == CIM_RGB)/*for RGB*/
		{
	
			spin_lock(yuv_lock);
			
			yuv_read_bak = yuv_read_cim;
			yuv_read_cim =3-yuv_read_system-yuv_read_cim;
			
			spin_unlock(yuv_lock);

			golf_pin_cim_yuv();    //add by dy 20130607
			wake_up_interruptible(&wq_out_yuv);

#ifdef GOLF_FORMAL_PRJ

			ShowRGBVideo(yuv_read_cim);

#endif

			__cim_disable(1);

//up(&exp_sem);//add by dy 20130325

			//golf_pin_cim_yuv();

			__cim_set_da(1, virt_to_phys(cim_frame_desc[rgb_read_cim*2+1]));

		}
		else/*for YUV*/
		{		
			spin_lock(rgb_lock);
				
			rgb_read_bak = rgb_read_cim;
			rgb_read_cim =3-rgb_read_system- rgb_read_cim;
			
			spin_unlock(rgb_lock);

			golf_pin_cim_rgb();	//add by dy 20130607
			wake_up_interruptible(&wq_out_rgb);

#ifdef GOLF_FORMAL_PRJ

			ShowYUVVideo(rgb_read_cim);

#endif
			__cim_disable(1);

//down(&exp_sem);  //add by dy 20130325

			//golf_pin_cim_rgb();	
	
			__cim_set_da(1, virt_to_phys(cim_frame_desc[yuv_read_cim*2]));
		}


		REG_CIM_STATE(1) &= ~CIM_STATE_DMA_EOF;

		golf_cim_reset_rxfifo(1);	// resetting rxfifo
		golf_cim_unreset_rxfifo(1);
		golf_cim_enable_dma(1);	// enable dma
		__cim_enable(1);
		
		sim_slecte = (sim_slecte+1)%2;

		for (i = 0; i < FRM_BUF_NUM; i++)
			dma_cache_wback((unsigned long)(cim_frame_desc[i]), sizeof(struct cim_desc));


		REG_CIM_STATE(1) &= ~CIM_STATE_DMA_EOF;

	}

	return 0;
}


/* CIM start capture data */
static void thcim_start(void)
{

	if (cim_id_init == 1) {
		cim_tran_buf_id = 0;
		last_data_buf_id = data_ready_buf_id = 2;
		data_buf_reading_flag = 0;
	//	cim_id_init = 0;
	}
	__cim_disable(1);
	//__cim_set_da((unsigned int)virt_to_phys((void *)cim_frame_desc[cim_tran_buf_id]));
	__cim_set_da(1, virt_to_phys(cim_frame_desc[0]));
	//__cim_clear_state();	// clear state register
	REG_CIM_STATE(1) = 0x0;

	//__cim_reset_rxfifo();	// resetting rxfifo
	REG_CIM_CTRL(1) |= CIM_CTRL_DMA_EN;
	REG_CIM_CTRL(1) |= CIM_CTRL_RXF_RST;
	REG_CIM_CTRL(1) &= ~CIM_CTRL_DMA_EN;

	//__cim_unreset_rxfifo();
	REG_CIM_CTRL(1) |= CIM_CTRL_DMA_EN;
	REG_CIM_CTRL(1) &= ~CIM_CTRL_RXF_RST;
	REG_CIM_CTRL(1) &= ~CIM_CTRL_DMA_EN;
	
	//__cim_enable_dma();	// enable dma
	REG_CIM_CTRL(1) |= CIM_CTRL_DMA_EN;
	REG_CIM_CTRL(1) &= ~CIM_CTRL_RXF_RST;
	REG_CIM_CTRL(1) |= CIM_CTRL_DMA_EN;

//	mdelay(5000);
	__cim_enable(1);
	printk("CIM start!!!!\n");
//	mdelay(5000);
//	__cim_disable();

}

static void cim_stop(void)
{
	__cim_disable(1);
	__cim_clear_state(1);
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
			printk("Page_list_head\n");
			desc_list_head = p_desc;
		}
		else
			cim_frame_desc[i-1]->nextdesc = virt_to_phys(p_desc);

		desc_list_tail = p_desc;
		desc_list_tail->nextdesc = virt_to_phys(desc_list_head);
		desc_list_tail->framebuf = virt_to_phys(my_frm_buf[i]);
		desc_list_tail->frameid = frm_id;
		desc_list_tail->dmacmd = FRM_BUF_LENGTH >> 2;
#ifdef  reg_320_240
		if((i%2) == 1)
			desc_list_tail->dmacmd = frm_rgb_buf_length >> 2;
		else
			desc_list_tail->dmacmd = FRM_BUF_LENGTH >> 2;
#else
		desc_list_tail->dmacmd = FRM_BUF_LENGTH >> 2;
#endif

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
#ifdef  reg_320_240
		if(((FRM_BUF_NUM-1)%2) == 1)
			cim_frame_desc[FRM_BUF_NUM-1]->dmacmd = (frm_rgb_buf_length >> 2) | CIM_CMD_EOFINT | CIM_CMD_STOP;
		else
			cim_frame_desc[FRM_BUF_NUM-1]->dmacmd = (FRM_BUF_LENGTH >> 2) | CIM_CMD_EOFINT | CIM_CMD_STOP;
#else
			cim_frame_desc[FRM_BUF_NUM-1]->dmacmd = (FRM_BUF_LENGTH >> 2) | CIM_CMD_EOFINT | CIM_CMD_STOP;
#endif

		cim_frame_desc[FRM_BUF_NUM-2]->nextdesc = virt_to_phys(cim_frame_desc[FRM_BUF_NUM-1]);


	}

	for (i = 0; i < FRM_BUF_NUM; i++)
	{
		dma_cache_wback((unsigned long)(cim_frame_desc[i]), sizeof(struct cim_desc));
	}
	return desc_list_head;
}
#if 0
static struct cim_desc *get_desc_list(void)
{
	int i;	
	struct cim_desc *desc_list_head __attribute__ ((aligned (sizeof(struct cim_desc))));
	struct cim_desc *desc_list_tail __attribute__ ((aligned (sizeof(struct cim_desc))));
	struct cim_desc *p_desc __attribute__ ((aligned (sizeof(struct cim_desc))));
	int frm_id = FRM_BUF_NUM - 1;

	desc_list_head = desc_list_tail = NULL;

	for(i = FRM_BUF_NUM - 1;i >= 0;i --)
	{
		if(NULL == (p_desc = (struct cim_desc *)kmalloc(sizeof(struct cim_desc),GFP_KERNEL)))
		{
			printk("cim_desc malloc error!\n");			
			return NULL;
		}

		p_desc->framebuf = virt_to_phys(my_frm_buf[i]);
		p_desc->nextdesc = virt_to_phys(desc_list_tail);
		p_desc->frameid = frm_id;
		p_desc->dmacmd = (FRM_BUF_LENGTH >> 2) | CIM_CMD_EOFINT | CIM_CMD_STOP;

		cim_frame_desc[i] = desc_list_tail = p_desc;
		
		frm_id -- ;
	}

	desc_list_head = cim_frame_desc[0];
	desc_list_tail = cim_frame_desc[FRM_BUF_NUM - 1];
	desc_list_tail->nextdesc = virt_to_phys(desc_list_head);

	for (i = 0; i < FRM_BUF_NUM; i++)
	{
		dma_cache_wback((unsigned long)(cim_frame_desc[i]), sizeof(struct cim_desc));
	}

	
	return desc_list_head;
}
#endif
static int thcim_fb_alloc(void)
{
	printk("cim_fb_alloc!\n");

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

#endif
//#define SENSOR_OUT_FORMAT_RGB565
#define SENSOR_OUT_FORMAT_RAWRGB

#ifdef SENSOR_OUT_FORMAT_RGB565 

#define _R_16(x) (((x&0x00F8)<<16))
#define _B_16(x) (((x&0x1F00)>>5))
#define _G_16(x) ((((x&0x0007)<<13)|((x&0xE000)>>3)))
#define _RGB565to888(x) ( (_R_16(x))|(_G_16(x))|(_B_16(x)) )

void ShowRGBVideo(int Index)
{
#if 0
    int i,j;
	
	unsigned short *src = NULL;
	unsigned int *dst = NULL;

    src = (unsigned short *)my_frm_buf[Index * 2];
    dst = (unsigned int *)lcd_frame0;

	for(i = 0;i < 240;i ++)
		for(j = 0;j < 320;j ++)
			dst[320 * i + j] = _RGB565to888(src[640 * i * 2  + j * 2]);
#endif
}
#endif

#ifdef SENSOR_OUT_FORMAT_RAWRGB

void ShowRGBVideo(int Index)
{
#if 0
    int i,j;
	
    unsigned short *src = NULL;
    unsigned int *dst = NULL;

    src = (unsigned short *)my_frm_buf[Index * 2];
    dst = (unsigned int *)lcd_frame0;

    unsigned char r = 0;
    unsigned char g = 0;
    unsigned char b = 0;

    unsigned int w = 640;
    unsigned int h = 480;

    for(i = 0;i < 320;i ++)
	for(j = 0;j < 240;j ++)
	{
		r = src[w * (j + 1) * 2  + i * 2];
		g = src[w * j * 2  + i * 2];
		b = src[w * j * 2  + i * 2 + 1];
		dst[240 * i + j] = r|(g<<8)|(b<<16);
	}
#endif
}
#endif
void ShowYUVVideo(int Index)
{
#if 0
    int i,j;
	
    unsigned short *src = NULL;
    unsigned int *dst = NULL;
    unsigned char tmp = 0;

    src = (unsigned short *)my_frm_buf[Index * 2 + 1];
    dst = (unsigned int *)lcd_frame0;
    for(i = 0;i < 320;i ++)
	for(j = 0;j < 240;j ++)
	{
	   tmp = src[i * 2  + j * 2 * 640];
	   dst[240 * i + j] = tmp|(tmp<<8)|(tmp<<16);
	}
#endif
}
#ifdef CIM_DMA_INTERRUPT

static irqreturn_t cim_irq_handler(int irq, void *dev_id)
{
	static int sensor_select = CIM_RGB;
	int i;
	unsigned int state = REG_CIM_STATE(1);
	unsigned long flags;
	unsigned char fresh_id  = 0;


	if (state & CIM_STATE_DMA_EOF) 
	{

		if (sensor_select == CIM_RGB)/*for RGB*/
		{
			bRGBImageReady = TRUE;

			wake_up_interruptible(&wq_out_rgb);

#ifdef GOLF_FORMAL_PRJ

			ShowRGBVideo(gRGBImageBufIndex);
#endif

	        spin_lock_irqsave(&fresh_lock,flags);

				rgb_read_index = gRGBImageBufIndex;

			spin_unlock_irqrestore(&fresh_lock,flags);

			if(unlikely(ImageBufIndex_MAX == gRGBImageBufIndex))
				gRGBImageBufIndex = 0;
			else
				gRGBImageBufIndex ++;
			

			__cim_disable();

			golf_pin_cim_yuv();


			__cim_set_da(virt_to_phys(cim_frame_desc[gRGBImageBufIndex * 2 + 1]));


			sensor_select = CIM_YUV;

		}
		else if(sensor_select == CIM_YUV)/*for YUV*/
		{				
			bYUVImageReady = TRUE;	
			
			wake_up_interruptible(&wq_out_yuv);

#ifdef GOLF_FORMAL_PRJ

		ShowYUVVideo(gYUVImageBufIndex);

#endif	
	        spin_lock_irqsave(&fresh_lock,flags);

				yuv_read_index = gYUVImageBufIndex;

			spin_unlock_irqrestore(&fresh_lock,flags);

			if(unlikely(ImageBufIndex_MAX == gYUVImageBufIndex))
				gYUVImageBufIndex = 0;
			else
				gYUVImageBufIndex ++;
				
			__cim_disable();

			golf_pin_cim_rgb();
		
			__cim_set_da(virt_to_phys(cim_frame_desc[gYUVImageBufIndex * 2]));


			sensor_select = CIM_RGB;
		}
		
	REG_CIM_STATE &= ~CIM_STATE_DMA_EOF;
	
	}
	else if ( state & CIM_STATE_RXF_OF)
	{
		printk("OverFlow interrupt!\n");

		__cim_disable();
	}

	for (i = 0; i < FRM_BUF_NUM; i++)
		dma_cache_wback((unsigned long)(cim_frame_desc[i]), sizeof(struct cim_desc));

	__cim_reset_rxfifo();
	__cim_unreset_rxfifo();
	__cim_clear_state();
	__cim_enable();

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
static int JZ_GOV7725_open(struct file *file)
{
	int minor = iminor(file->f_dentry->d_inode);
	printk( "JZ_GOV7725: open called minor: %d\n", minor);
/*
	//Just can be enabled once!
	if(minor == 21)
		enable_irq(IRQ_CIM);
*/

	return 0;
}
static int JZ_GOV7725_release(struct file *file)
{
	int minor = iminor(file->f_dentry->d_inode);	
	return 0;
}

static int JZ_GOV7725_mmap( struct file *file, struct vm_area_struct * vma)
{
	static int mmap_num_one = CIM_RGB;
	static int mmap_num_two = CIM_YUV;

	unsigned long jz_off;
	printk("!!!!!!!!!!!!!!!!JZ_GOV7725_mmap !!!!iminor(file) %d\n",iminor(file->f_dentry->d_inode));
	if(iminor(file->f_dentry->d_inode)==IMINOR_RGB)
	{	
		jz_off = virt_to_phys(my_frm_buf[mmap_num_one]);

		mmap_num_one=(mmap_num_one+2)%FRM_BUF_NUM;

	}
	else
	{
		jz_off = virt_to_phys(my_frm_buf[mmap_num_two]);

		mmap_num_two=(mmap_num_two+2)%FRM_BUF_NUM;

	}

	vma->vm_pgoff = jz_off >> PAGE_SHIFT;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

	printk("JZ_GOV7725_mmap !!!!\n");

	if(remap_pfn_range(vma, vma->vm_start, jz_off >> PAGE_SHIFT,
					vma->vm_end - vma->vm_start,
					vma->vm_page_prot))
	return -EAGAIN;

	return 0;
}

/*ZYL,2011.1012*/
#define ERROR_TF_EMPTY_TIMEOUT -111
static int set_sensor_reg(int selct_cim, unsigned char reg, unsigned char data)
{
	int ret = 0;
	unsigned char cnt = 0;
	//unsigned char cnt1 = 0;
	
	const unsigned char trytimes = 20;
	
	//printk("%s, Write config register\n", __FUNCTION__);

	if(selct_cim==IMINOR_YUV)
	{
		//printk("IMINOR YUV\n");
		while(cnt ++  < trytimes)
		{
		   ret = sensor_write_reg2(gGov7725dev_yuv.client, reg, data);

			if(!ret)
				break;
			
			else if(ERROR_TF_EMPTY_TIMEOUT == ret)
				{
					//jz_wdt_ping();
					printk("Reg is:0x%x,and data is:0x%x\n",reg,data);
					
					sensor_init_i2c(gGov7725dev_yuv.client, 0);
					mdelay(100);

					return 0;

				 }
			else
				mdelay(10);
				
		}

	}
	else if(selct_cim==IMINOR_RGB)
	{
		//printk("IMINOR RGB\n");
		while(cnt ++  < trytimes)
		{
			if(!(ret = sensor_write_reg2(gGov7725dev_rgb.client, reg, data)))
				break;
			else
				mdelay(10);
		}

	}
	
	//printk("%s, check point\n", __FUNCTION__);
	if(!ret)
		ret = 0;
	
	else
		{
			ret = -1;
			printk("Error!!!Try times is :%d\n",cnt);
			printk("Reg is:0x%x,and data is:0x%x\n",reg,data);
		}
	//printk("Reg is:%x0x,and data is:%x0x\n",reg,data);
	//printk("Try times is :%d\n",cnt);

	return ret;
}

#if 0

static int set_sensor_reg(int selct_cim, unsigned char reg, unsigned char data)
{
	int ret = 0;
	unsigned char cnt = 0;
	const unsigned char trytimes = 20;
	unsigned char data_back = 0;

	if(selct_cim==IMINOR_YUV)
		
		golf_I2C_cim_yuv();
	
	else if(selct_cim==IMINOR_RGB)
		
		golf_I2C_cim_rgb();

	do
	{
		sensor_write_reg2(gGov7735dev.client, reg, data);
		mdelay(2);
		sensor_read_reg2(gGov7735dev.client, reg, &data_back);
	
	}while(data != data_back && ++ cnt  < trytimes);
	
	if(cnt < trytimes)
		ret = 0;
	else
		{
			ret = -1;
			printk("Error!!!Try times is :%d\n",cnt);
		}

	return ret;
}
#endif
static int get_sensor_reg(int selct_cim, unsigned char reg,unsigned char* date)
{
		unsigned char *regVal = date;
		int ret = 0;
		unsigned char cnt = 0;
		const unsigned char trytimes = 20;
				
		if(selct_cim==IMINOR_YUV)
		{
			while(cnt ++  < trytimes)
			{
				if(!(ret = sensor_read_reg2(gGov7725dev_yuv.client, reg, regVal)))
					break;
				else
					mdelay(1);
			}
		
		}
		else if(selct_cim==IMINOR_RGB)
		{		
			while(cnt ++  < trytimes)
			{
				if(!(ret = sensor_read_reg2(gGov7725dev_rgb.client, reg, regVal)))
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

static int JZ_GOV7725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
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
	case VIDIOC_S_EXPOSURE:
	{
//#if 0
		unsigned short exposure = 0;
		int ret = 0;
		//unsigned int cnt = 200;
		unsigned char h_data, l_data;
		unsigned long flag;

		get_user(exposure,(unsigned short *)arg);
		
		if(exposure<MIN_EXP)
			exposure=MIN_EXP;
		else
			exposure= ((exposure+(MIN_STEP/2))/MIN_STEP)*MIN_STEP;
		//printk("expusure=%d\n",exposure);
		
		h_data = (unsigned char)((exposure & 0xff00)>>8);
		l_data = (unsigned char)(exposure & 0xff);
		
		//down(&SENSOR_REG_mutex);
//down(&exp_sem);//add by dy 20130325
		//golf_pin_cim_yuv();    //add by dy 20130325
		long time;
		time = interruptible_sleep_on_timeout(&wq_out_yuv,1000);  //add by dy 20130517

		i2c_pin_cim_yuv();	// add, yjt, 20130725
		mdelay(5);			// add, yjt, 20130725
		
		ret = set_sensor_reg(iminor(file->f_dentry->d_inode),AECH, h_data);
		//mdelay(30);
		ret = set_sensor_reg(iminor(file->f_dentry->d_inode),AECL, l_data);
//up(&exp_sem); //add by dy 20130325

		//up(&SENSOR_REG_mutex);

		printk( " VIDIOC_S_EXPOSURE exposure 0x%x, 0x%x \n", h_data, l_data);

		return ret;
//#endif
//		return 0;
	}
	
	case VIDIOC_QUERYBUF:
	{
		printk("JZ_GOV7725_ioctl VIDIOC_QUERYBUF \n");
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
		
		if(iminor(file->f_dentry->d_inode) == IMINOR_RGB)
		{
			if(yuv_read_bak!= 0xff)
			{
				spin_lock(yuv_lock);
				yuv_read_system =yuv_read_bak;
				yuv_read_bak = 0xff;
				spin_unlock(yuv_lock);
			}
			else
			{
				time = interruptible_sleep_on_timeout(&wq_out_yuv,1000);
				if(yuv_read_bak!= 0xff)
				{
					spin_lock(yuv_lock);
					yuv_read_system =yuv_read_bak;
					yuv_read_bak = 0xff;
					spin_unlock(yuv_lock);
				}				
			}

			put_user(yuv_read_system,(int *)arg);

			//gRGB_frame ++;

			if (time < 1 )
			{
				printk("interruptible_sleep_on_timeout IMINOR_RGB time is:%ul\n",time);

				//printk( " VIDIOC_RESET_SENSOR!!!\n");
				
				//sensor_init_i2c(gGov7735dev.client, 0);
				
				return -1;				
			}


		}
		if (iminor(file->f_dentry->d_inode)==IMINOR_YUV)
		{
			if(rgb_read_bak!= 0xff)
			{
				spin_lock(rgb_lock);
				rgb_read_system = rgb_read_bak;
				rgb_read_bak = 0xff;
				spin_unlock(rgb_lock);
			}
			else
			{
				time = interruptible_sleep_on_timeout(&wq_out_rgb,1000);
				if(rgb_read_bak!= 0xff)
				{
					spin_lock(rgb_lock);
					rgb_read_system = rgb_read_bak;
					rgb_read_bak = 0xff;
					spin_unlock(rgb_lock);
				}				
			}

			put_user(rgb_read_system,(int *)arg);

			//gYUV_frame ++;

			if (time < 1 )
			{
				printk("interruptible_sleep_on_timeout IMINOR_YUV time is:%ul\n",time);

				//printk( " VIDIOC_RESET_SENSOR!!!\n");
				
				//sensor_init_i2c(gGov7735dev.client, 0);
					
				return -1;				
			}


		}
		return 15;
	}

#if 0

	case VIDIOC_DQBUF:
	{
		int ret_time = 0;
		unsigned long flags;		
		
		if(iminor(inode) == IMINOR_RGB)
		{
			ret_time = wait_event_interruptible_timeout(wq_out_rgb,bRGBImageReady,1000);
			
	        spin_lock_irqsave(&fresh_lock,flags);

				put_user(rgb_read_index,(int *)arg);

			spin_unlock_irqrestore(&fresh_lock,flags);
	
			bRGBImageReady = FALSE;

			//gRGB_frame ++;

			if (ret_time <= 0 )
			{
				printk("wait_event_interruptible_timeout IMINOR_RGB ret is:%d\n",ret_time);
				
				//down(&SENSOR_REG_mutex);
				
				sensor_init_i2c(gGov7735dev.client, 0);
				
				//up(&SENSOR_REG_mutex);
				
				return -1;				
			}
		}
		else if(iminor(inode)==IMINOR_YUV)
		{
			ret_time = wait_event_interruptible_timeout(wq_out_yuv,bYUVImageReady,1000);
			
	        spin_lock_irqsave(&fresh_lock,flags);	
			
				put_user(yuv_read_index,(int *)arg);
			
			spin_unlock_irqrestore(&fresh_lock,flags);
			
			bYUVImageReady = FALSE;	

			//gYUV_frame ++;

			if (ret_time <= 0 )
			{
				printk("wait_event_interruptible_timeout IMINOR_YUV ret is:%d\n",ret_time);
				
				//down(&SENSOR_REG_mutex);
				
				sensor_init_i2c(gGov7735dev.client, 0);
				
				//up(&SENSOR_REG_mutex);
				
				return -1;				
			}
		}

		return 15;
	}
	
#endif

	case VIDIOC_INIT_ALLSENOSER:

		printk( " VIDIOC_RESET_SENSOR \n");

		//down(&SENSOR_REG_mutex);
		
		sensor_init_i2c(gGov7725dev_rgb.client, 0);
		sensor_init_i2c(gGov7725dev_yuv.client, 0);
		
		//up(&SENSOR_REG_mutex);
		
		break;					
	case VIDIOC_G_EXPOSURE:
		{
	
			unsigned char h_data = 0;
			unsigned char l_data = 0;
			unsigned short exposureValue = 0;
			unsigned long flag;

     		printk("Getting sensor Exposure Value!\n");

			//down(&SENSOR_REG_mutex);
			
//down(&exp_sem);//add by dy 20130325
			get_sensor_reg(iminor(file->f_dentry->d_inode), AECL, &l_data);
			//mdelay(10);
			get_sensor_reg(iminor(file->f_dentry->d_inode), AECH, &h_data);
//up(&exp_sem); //add by dy 20130325
			
			//up(&SENSOR_REG_mutex);
			
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
//	if(cim_cfg == NULL)
//		return;

	//memset(&jz_cim->cim_cfg, 0, sizeof(jz_cim->cim_cfg));

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

#if 0 // \B2\BB\C3\F7
	cim_cfg.packed = jz_cim->ginfo.packed;
	/* Just for test */
	cim_cfg.fmt_422 = jz_cim->ginfo.fmt_422;
#else
	cim_cfg->packed = 0; // =================== golf ==================
	/* Just for test */
//	cim_cfg.fmt_422 = jz_cim->ginfo.fmt_422;
#endif //
	return;
}

static void cim_config60(cim_config_t *c)
{

	//REG_CIM_CFG(1)  = c->cfg;
	REG_CIM_CTRL(1) = c->ctrl;
//	REG_CIM_SIZE(1) = c->size;
//	REG_CIM_OFFSET(1) = c->offs;
	REG_CIM_CFG(1)		= 0x00020c46/*0x00024c56*/;  // 0,1,2,3,4,5,6,7
//	REG_CIM_CTRL(1)	= 0x00004984;
	REG_CIM_SIZE(1)	= 0x011001e0;
	REG_CIM_OFFSET(1)	= 0x00680050;

#if defined(CONFIG_SOC_JZ4760B) || defined(CONFIG_SOC_JZ4770) || defined(CONFIG_SOC_JZ4775)

		__cim_enable_bypass_func(1);
		//__cim_disable_bypass_func(1);

		__cim_enable_auto_priority(1);
		//__cim_disable_auto_priority(1);

		__cim_enable_emergency(1);
		//__cim_disable_emergency(1);

		/* 0, 1, 2, 3
		 * 0: highest priority
		 * 3: lowest priority
		 * 1 maybe best for SEP=1
		 * 3 maybe best for SEP=0
		 */
		//__cim_set_opg(1);
		__cim_set_opg(1, 1);
		//__cim_set_opg(2);
		//__cim_set_opg(3);

		__cim_enable_priority_control(1);
		//__cim_disable_priority_control();

c->packed = 1;
	if (c->packed) {
		__cim_disable_sep(1);
	} else {
		__cim_enable_sep(1);
	}
#endif

	__cim_enable_dma(1);

#ifdef CIM_SAFE_DISABLE
	__cim_enable_vdd_intr(1);
#else
	__cim_disable_vdd_intr(1);
#endif

#ifdef CIM_INTR_SOF_EN
	__cim_enable_sof_intr(1);
#else
	__cim_disable_sof_intr(1);
#endif

#define CIM_INTR_EOF_EN 
#ifdef CIM_INTR_EOF_EN
	__cim_enable_eof_intr(1);
#else
	__cim_disable_eof_intr(1);
#endif

#ifdef CIM_INTR_STOP_EN
	__cim_enable_stop_intr(1);
#else
	__cim_disable_stop_intr(1);
#endif

/*
#ifdef CIM_INTR_TRIG_EN
	__cim_enable_trig_intr(1);
#else
	__cim_disable_trig_intr(1);
#endif
*/

#define CIM_INTR_OF_EN 
#ifdef CIM_INTR_OF_EN
	__cim_enable_rxfifo_overflow_intr(1);
#else
	__cim_disable_rxfifo_overflow_intr(1);
#endif

#ifdef CIM_INTR_EEOF_EN
	__cim_set_eeof_line(1, 100);
	__cim_enable_eeof_intr(1);
#else
#if !defined(CONFIG_SOC_JZ4750)
	__cim_set_eeof_line(1, 0);
	__cim_disable_eeof_intr(1);
#endif
#endif

#ifdef CONFIG_VIDEO_CIM_FSC
	__cim_enable_framesize_check_error_intr(1);
#else
	__cim_disable_framesize_check_error_intr(1);
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
	cim_print_regs60(1);
	
	return 0;
}


////////////////////////////////////////////// add by zjj 2013.8.6
static int i2c_pin_cim_select(struct file *file, const char *buffer, unsigned long count, void *data)
{
	int l = 0;

    l = simple_strtoul(buffer, 0, 10);
    if(l==0)
    {
        i2c_pin_cim_rgb();
      	printk("i2c_pin_cim_select %d --- rgb.\n", l);
    }
    else
    {
        i2c_pin_cim_yuv();
	    printk("i2c_pin_cim_select %d --- yuv.\n", l);
	}

    return count;
}
//////////////////////////////////////////////

/*ZYL,2011.11.16*/
static int sensor_init_i2c(struct i2c_client *client, unsigned char st_num)
{
	int i = 0;
	int j = 0;
    struct proc_dir_entry *res;

	printk("Initalize RGB CMOS CAMMERA...... \n");
	
	// golf_pin_cim_rgb(); //add by dy		// del, yjt, 20130725
	i2c_pin_cim_rgb();		// add, yjt, 20130725
	
	mdelay(10);	// add, yjt, 20130723
	// v7740reg
	for(i=0; i<sizeof(v7725reg)/sizeof(v7725reg[0]); i++)
	{
		if(0 != set_sensor_reg(IMINOR_RGB,v7725reg[i].reg,v7725reg[i].val))
			printk("RGB write reg error:0x%x\n",v7725reg[i].reg);
		
		
		if(v7725reg[i].reg == 0x12 || v7725reg[i].reg == 0x15 || v7725reg[i].reg == 0x0c)
			mdelay(3);
		else
			udelay(100);
	}
	
	mdelay(10);	// add, yjt, 20130723

	printk("Initalize YUV CMOS CAMMERA...... \n");

	
	// v7740YUVreg
	// golf_pin_cim_yuv();    //add by dy	// del, yjt, 20130725
	i2c_pin_cim_yuv();		// add, yjt, 20130725
	
	mdelay(10);	// add, yjt, 20130723

	for(i=0; i<sizeof(v7725YUVreg)/sizeof(v7725YUVreg[0]); i++)
	{

		if(0 != set_sensor_reg(IMINOR_YUV,v7725YUVreg[i].reg,v7725YUVreg[i].val))
			printk("YUV write reg error:0x%x\n",v7725YUVreg[i].reg);


		if(v7725YUVreg[i].reg == 0x12 || v7725YUVreg[i].reg == 0x15 || v7725YUVreg[i].reg == 0x0c)
			mdelay(3);
		else
			udelay(100);
	}

#if 1
	//init AGC,add by dy	
	unsigned char temp1;
	mdelay(30);
	temp1= (unsigned char)(PRE_AGC&0xff);
	//printk("0x00=%x\n",temp1);
	set_sensor_reg(IMINOR_YUV,AGCL, temp1);
	mdelay(30);
	unsigned char temp;
	get_sensor_reg(IMINOR_YUV, AGCH, &temp);
	temp=temp|(PRE_AGC>>8&0x03);
	mdelay(30);
	set_sensor_reg(IMINOR_YUV,AGCH, temp);
	//printk("0x15=%d\n",temp);
#endif

#ifdef SENSOR_READ_TEST	
	sensor_read_all_regs();
#endif
////////////////////////////////////////////// add by zjj 2013.8.6
	res = create_proc_entry("i2c_pin_cim", 0644, NULL);
	if (res) {
		res->read_proc = NULL;
		res->write_proc = i2c_pin_cim_select;
		res->data = NULL;
	}
//////////////////////////////////////////////
	return 0;
}



static int cim_start_first(void)
{
	int i;

	cim_device_config60();
	
	//golf_pin_cim_yuv();
	
	sensor_init_i2c(gGov7725dev_rgb.client, 0);

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

//static const struct file_operations JZ_OV7725_fops = {
static const struct v4l2_file_operations JZ_OV7725_fops = {
	.owner		= THIS_MODULE,
	.open           = JZ_GOV7725_open,
	.release        = JZ_GOV7725_release,
//	.read           = JZ_OV7725_read,
//	.poll		= JZ_OV7725_poll,
	.ioctl          = JZ_GOV7725_ioctl,//video_ioctl2, /* V4L2 ioctl handler */
	.mmap           = JZ_GOV7725_mmap,
//	.llseek         = no_llseek,
};

static struct video_device JZ_OV7725_RGB = {
	.name		= "JZ_OV7725_RGB",
	//.type		= VID_TYPE_CAPTURE,
	.vfl_type	= VFL_TYPE_GRABBER,
	.fops           = &JZ_OV7725_fops,
	.minor		= IMINOR_RGB,
};

static struct video_device JZ_OV7725_YUV = {
	.name		= "JZ_OV7725_YUV",
	//.type		= VID_TYPE_CAPTURE,
	.vfl_type	= VID_TYPE_CAPTURE,
	.fops           = &JZ_OV7725_fops,
	.minor		= IMINOR_YUV,
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
	
	printk("Below is RGB sensor' all register:\n");
	for(i=0; i<sizeof(v7725reg)/sizeof(v7725reg[0]); i++)
	{
		if(0 != (ret = get_sensor_reg(IMINOR_RGB, v7725reg[i].reg,&regVal)))
			printk("get sensor reg Err!  0x%x\n",v7725reg[i].reg);
			
		printk("0x%x,0x%x\n",v7725reg[i].reg,regVal);		
	}
		
	printk("Below is YUV sensor' all register:\n");
	for(i=0; i<sizeof(v7725YUVreg)/sizeof(v7725YUVreg[0]); i++)
	{
		if(0 != (ret = get_sensor_reg(IMINOR_YUV, v7725YUVreg[i].reg,&regVal)))
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
	cpm_stop_clock(CGM_CIM1);
}

static void cim_power_on(void)
{
	cpm_stop_clock(CGM_CIM1);
	cpm_set_clock(CGU_CIM1CLK, 36000000);	// modify, yjt, 20130724, 48M -> 22M
	cpm_start_clock(CGM_CIM1);
}

static int ov77255_rgb_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct JZ_GOV7725_dev *dev = &gGov7725dev_rgb;

	printk("in function %s\n", __FUNCTION__);

	//__gpio_as_output1(32*6 + 7);	// SENSOR RESET PIN  by dy	// modify, yjt, 20130718, PD24 -> PG7
	reset_pin_cim_reset_off();
	
	sensor_set_i2c_speed(client, 100000);//set i2c speed : 100khz

//	__gpio_as_cim();
	__gpio_as_cim_8bit(1);

	cim_power_on();

	CIM_SELECT_CTR_GPIO_INIT();

	client->addr=0x21;  // add by dy ,it is true ov7740 i2c addr
	dev->client = client;

	list_add_tail(&dev->JZ_OV7725_devlist, &JZ_OV7725_devlist);

	/* init video dma queues */
	init_waitqueue_head(&wq_out_rgb);
	//init_waitqueue_head(&wq_out_yuv);

	/* callbacks */
	JZ_OV7725_RGB.release = video_device_release;

	dev->vfd = &JZ_OV7725_RGB;

	ret = video_register_device(dev->vfd, VFL_TYPE_GRABBER, IMINOR_RGB);
	if(ret != 0){
		printk("21 video_register_device %d error\n\n", IMINOR_RGB);
	}else{
		printk("21 video_register_device %d OK\n\n", IMINOR_RGB);
		mdelay(100);
	}
		
	//cim_start_first();
	
	//timer_init();

	return ret;

}

static int ov77255_yuv_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret;
	struct JZ_GOV7725_dev *dev = &gGov7725dev_yuv;

	printk("in function %s\n", __FUNCTION__);
	
	//__gpio_as_output1(32*6+7);	// SENSOR RESET PIN
	reset_pin_cim_reset_off();
	
	sensor_set_i2c_speed(client, 100000);//set i2c speed : 100khz

//	__gpio_as_cim();
	__gpio_as_cim_8bit(1);

	cim_power_on();

//	CIM_SELECT_CTR_GPIO_INIT();

	dev->client = client;

	list_add_tail(&dev->JZ_OV7725_devlist, &JZ_OV7725_devlist);

	/* init video dma queues */
	//init_waitqueue_head(&wq_out_rgb);
	init_waitqueue_head(&wq_out_yuv);

	//init_MUTEX(&SENSOR_REG_mutex);	


	JZ_OV7725_YUV.release = video_device_release;
	dev->vfd = &JZ_OV7725_YUV;
	ret = video_register_device(dev->vfd, VFL_TYPE_GRABBER, IMINOR_YUV);
	if(ret != 0){
		printk("22 video_register_device %d error\n\n", IMINOR_YUV);
	}else{
		printk("22 video_register_device %d OK\n\n", IMINOR_YUV);
		mdelay(100);
	}	
		
	cim_start_first();
	
#ifdef CIM_DMA_INTERRUPT
//printk("irq request!\n");
	if(ret = request_irq(IRQ_CIM,cim_irq_handler,IRQF_DISABLED,"CIM",0))
	{
		printk("IRQ_CIM request failed!\n");
		return ret;
	}
	
	disable_irq(IRQ_CIM);	

	spin_lock_init(&fresh_lock);
	
#endif

	//frameratetest_timer_init();
	timer_init();

	return ret;

}

static const struct i2c_device_id ov7725_id[] = {
	/* name, private data to driver */
	{ "ov7725_rgb", 0},
	{ }	/* Terminating entry */
};

static const struct i2c_device_id ov7725_yuv_id[] = {
	{"ov7725_yuv", 0},
};


//MODULE_DEVICE_TABLE(i2c, ov7725_id);

static struct i2c_driver ov7725_rgb_driver = {
	.probe		= ov77255_rgb_probe,
	/*.remove		= ov77255_remove,*/
	.id_table	= ov7725_id,
	.driver	= {
		.name = "ov7725_rgb",
		.owner = THIS_MODULE,
	},
};

static struct i2c_driver ov7725_yuv_driver = {
	.probe		= ov77255_yuv_probe,
	/*.remove		= ov77255_remove,*/
	.id_table	= ov7725_yuv_id,
	.driver	= {
		.name = "ov7725_yuv",
		.owner = THIS_MODULE,
	},
};

/* -----------------------------------------------------------------
	Initialization and module stuff
  ------------------------------------------------------------------*/

#if 1
static int __init JZ_OV7725_init(void)
{
	int err;
/*
	#define LIGHT1_PIN	GPF(2)
	__gpio_as_output(LIGHT1_PIN);
	__gpio_clear_pin(LIGHT1_PIN);

	__gpio_as_output0(GPD(10));	// SENSOR RESET PIN
	mdelay(5);
*/

	//__gpio_as_output0(32*6 + 7);		// add, yjt, 20130723, reset sensors.
	reset_pin_cim_reset_on();

	err = i2c_add_driver(&ov7725_rgb_driver);
	
	if(err)
	{
		printk("add ov7725_rgb failed\n");
		return err;
	}

	return i2c_add_driver(&ov7725_yuv_driver);

}

#else

static int __init JZ_OV7725_init(void)
{
#define LIGHT1_PIN	GPF(2)
	__gpio_as_output(LIGHT1_PIN);
	__gpio_clear_pin(LIGHT1_PIN);

	//__gpio_as_output0(32*6 + 7);	// SENSOR RESET PIN  by dy	// modify, yjt, 20130718, PD10 -> PG7
	reset_pin_cim_reset_on();
	mdelay(5);

	return i2c_add_driver(&ov7725_rgb_driver);
}

static int __init JZ_OV7725_yuv_init(void)
{
	return i2c_add_driver(&ov7725_yuv_driver);

}

#endif

static void __exit JZ_OV7725_exit(void)
{
	printk("Now we're exit form:%s\n",__FUNCTION__);

	video_unregister_device(&JZ_OV7725_RGB);
    video_unregister_device(&JZ_OV7725_YUV);
	kfree(cim_frame_desc);
	
	i2c_del_driver(&ov7725_rgb_driver);
	i2c_del_driver(&ov7725_yuv_driver);
}


//module_init(JZ_OV7725_yuv_init);
module_init(JZ_OV7725_init);
module_exit(JZ_OV7725_exit);

MODULE_LICENSE("GPL");


