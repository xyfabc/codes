

#include <asm/jzsoc.h>
#include <linux/i2c.h>

#include "../../jz_cim_core.h"
#include "../../jz_sensor.h"

#include "ov9650_camera.h"
#include "ov9650_set.h"

#define OV9650_DEBUG
//#undef DEBUG

#ifdef OV9650_DEBUG
#define dprintk(x...)   do{printk("OV9650---\t");printk(x);printk("\n");}while(0)
#else
#define dprintk(x...)
#endif

struct camera_sensor_desc ov9650_sensor_desc;

/* gpio init */
#if defined(CONFIG_JZ4750_APUS) || defined(CONFIG_JZ4750D_FUWA1) || defined(CONFIG_JZ4750_AQUILA)/* board APUS */
#define GPIO_CAMERA_RST         (32*4+8) /* MCLK as reset */
#elif defined(CONFIG_JZ4760_ALTAIR)
#define GPIO_CAMERA_RST         (32*4+13) /*GPE13*/
#elif defined(CONFIG_JZ4760_LEPUS) || defined(CONFIG_JZ4760B_LEPUS)
#define GPIO_CAMERA_RST         (32*1 + 26) /* GPB26 */
#elif defined(CONFIG_JZ4760_F4760) || defined(CONFIG_JZ4810_F4810)/* JZ4760 FPGA */
#define GPIO_CAMERA_RST         (32*1+9) /* CIM_MCLK as reset */
#elif defined(CONFIG_JZ4770_F4770)
#define GPIO_CAMERA_RST		GPB(9)
#elif defined(CONFIG_JZ4770_PISCES)
#define GPIO_CAMERA_RST		GPE(3)
#elif defined(CONFIG_JZ4780_F4780)
#define GPIO_CAMERA_RST		(32*1+9)	/* GPB9 */
#else
#error "ov9650/ov9650_camera.c , please define camera for your board."
#endif

void ov9650_power_down(void)
{
	printk("=======%s: L%d\n", __FUNCTION__, __LINE__);
#if defined(CONFIG_JZ4750_AQUILA)
	__gpio_as_output(4*32+23);
	__gpio_set_pin(4*32+23);
#elif defined(CONFIG_JZ4760_ALTAIR)
	__gpio_as_output(0*32+27);                                    /* GPA27 */
	__gpio_set_pin(0*32+27);
#elif defined(CONFIG_JZ4760_LEPUS) || defined(CONFIG_JZ4760B_LEPUS)
	__gpio_as_output(32 * 1 + 27); /* GPB27 */
	__gpio_set_pin(32 * 1 + 27);
#elif defined(CONFIG_JZ4770_PISCES)
	__gpio_as_output1(GPE(4));
#endif
	mdelay(5);
}

void ov9650_power_up(void)
{
	printk("=======%s: L%d\n", __FUNCTION__, __LINE__);
#if defined(CONFIG_JZ4750_AQUILA)
	__gpio_as_output(4*32+23);
	__gpio_clear_pin(4*32+23);
#elif defined(CONFIG_JZ4760_ALTAIR)
	__gpio_as_output(0*32+27);
	__gpio_clear_pin(0*32+27);
#elif defined(CONFIG_JZ4760_LEPUS) || defined(CONFIG_JZ4760B_LEPUS)
	__gpio_as_output(32 * 1 + 27); /* GPB27 */
	__gpio_clear_pin(32 * 1 + 27);
#elif defined(CONFIG_JZ4770_PISCES)
	__gpio_as_output0(GPE(4));
#endif
	mdelay(5);
}


int ov9650_set_mclk(unsigned int mclk)
{
	/* Set the master clock output */
	/* If use pll clock, enable it */
	// __cim_set_master_clk(__cpm_get_hclk(), c->mclk);
	return 0;//in this board use the extra osc - -20MHZ
}

void ov9650_reset(void)
{
#if  defined(CONFIG_JZ4750_AQUILA)
	__gpio_as_output(5*32+23); /* GPIO_IO_SWETCH_EN */
	__gpio_clear_pin(5*32+23);
	__gpio_as_output(2*32+31); /* GPIO_BOOTSEL1 for camera rest*/
	__gpio_set_pin(2*32+31);
	mdelay(5);
	__gpio_clear_pin(2*32+31);
	mdelay(5);
	__gpio_set_pin(2*32+31);
#else
#if defined(CONFIG_JZ4810_F4810) || defined(CONFIG_JZ4770_F4770) || defined(CONFIG_JZ4770_PISCES) || defined(CONFIG_JZ4780_F4780)
	printk("=======%s: L%d\n", __FUNCTION__, __LINE__);
	//while(1)
	{
		__gpio_as_output1(GPIO_CAMERA_RST);	//active high
		mdelay(50);
		__gpio_as_output0(GPIO_CAMERA_RST);
		mdelay(50);
	}
#else
	__gpio_clear_pin(GPIO_CAMERA_RST);
	mdelay(50);
	__gpio_set_pin(GPIO_CAMERA_RST);
	mdelay(50);
#endif
#endif

	printk("REG_GPIO_PXPIN(%d)=0x%08x\n", 1, REG_GPIO_PXPIN(1));
	printk("REG_GPIO_PXINT(%d)=0x%08x\n", 1, REG_GPIO_PXINT(1));
	printk("REG_GPIO_PXMASK(%d)=0x%08x\n", 1, REG_GPIO_PXMASK(1));
	printk("REG_GPIO_PXPAT1(%d)=0x%08x\n", 1, REG_GPIO_PXPAT1(1));
	printk("REG_GPIO_PXPAT0(%d)=0x%08x\n", 1, REG_GPIO_PXPAT0(1));
}

int ov9650_set_balance(balance_flag_t balance_flag,int arg)
{
	dprintk("ov9650_set_balance");
	switch(balance_flag)
	{
		case WHITE_BALANCE_AUTO:
			dprintk("WHITE_BALANCE_AUTO");
			break;
		case WHITE_BALANCE_INCANDESCENT :
			dprintk("WHITE_BALANCE_INCANDESCENT ");
			break;
		case WHITE_BALANCE_FLUORESCENT ://ying guang
			dprintk("WHITE_BALANCE_FLUORESCENT ");
			break;
		case WHITE_BALANCE_WARM_FLUORESCENT :
			dprintk("WHITE_BALANCE_WARM_FLUORESCENT ");
			break;
		case WHITE_BALANCE_DAYLIGHT ://ri guang
			dprintk("WHITE_BALANCE_DAYLIGHT ");
			break;
		case WHITE_BALANCE_CLOUDY_DAYLIGHT ://ying tian
			dprintk("WHITE_BALANCE_CLOUDY_DAYLIGHT ");
			break;
		case WHITE_BALANCE_TWILIGHT :
			dprintk("WHITE_BALANCE_TWILIGHT ");
			break;
		case WHITE_BALANCE_SHADE :
			dprintk("WHITE_BALANCE_SHADE ");
			break;
	}

	return 0;
}
int ov9650_set_antibanding(antibanding_flag_t antibanding_flag,int arg)
{
	dprintk("ov9650_set_antibanding");
	switch(antibanding_flag)
	{
		case ANTIBANDING_AUTO :
			dprintk("ANTIBANDING_AUTO ");
			break;
		case ANTIBANDING_50HZ :
			dprintk("ANTIBANDING_50HZ ");
			break;
		case ANTIBANDING_60HZ :
			dprintk("ANTIBANDING_60HZ ");
			break;
		case ANTIBANDING_OFF :
			dprintk("ANTIBANDING_OFF ");
			break;
	}
	return 0;
}

int ov9650_set_flash_mode(flash_mode_flag_t flash_mode_flag,int arg)
{
	dprintk("ov9650_set_flash_mode");
	switch(flash_mode_flag)
	{
		case FLASH_MODE_OFF :
			dprintk("FLASH_MODE_OFF");
			break;
		case FLASH_MODE_AUTO :
			dprintk("FLASH_MODE_AUTO  ");
			break;
		case FLASH_MODE_ON :
			dprintk("FLASH_MODE_ON ");
			break;
		case FLASH_MODE_RED_EYE:
			dprintk("FLASH_MODE_RED_EYE ");
			break;
		case FLASH_MODE_TORCH:
			dprintk("FLASH_MODE_TORCH ");
			break;
	}
	return 0;

}

int ov9650_set_scene_mode(scene_mode_flag_t scene_mode_flag,int arg)
{
	dprintk("ov9650_set_scene_mode");
	switch(scene_mode_flag)
	{
		case SCENE_MODE_AUTO :
			dprintk("SCENE_MODE_AUTO ");
			break;
		case SCENE_MODE_ACTION :
			dprintk("SCENE_MODE_ACTION ");
			break;
		case SCENE_MODE_PORTRAIT   :
			dprintk("SCENE_MODE_PORTRAIT   ");
			break;
		case SCENE_MODE_LANDSCAPE  :
			dprintk("SCENE_MODE_LANDSCAPE  ");
			break;
		case SCENE_MODE_NIGHT     :
			dprintk("SCENE_MODE_NIGHT     ");
			break;
		case SCENE_MODE_NIGHT_PORTRAIT   :
			dprintk("SCENE_MODE_NIGHT_PORTRAIT   ");
			break;
		case SCENE_MODE_THEATRE  :
			dprintk("SCENE_MODE_THEATRE  ");
			break;
		case SCENE_MODE_BEACH   :
			dprintk("SCENE_MODE_BEACH   ");
			break;
		case SCENE_MODE_SNOW    :
			dprintk("SCENE_MODE_SNOW    ");
			break;
		case SCENE_MODE_SUNSET    :
			dprintk("SCENE_MODE_SUNSET    ");
			break;
		case SCENE_MODE_STEADYPHOTO   :
			dprintk("SCENE_MODE_STEADYPHOTO   ");
			break;
		case SCENE_MODE_FIREWORKS    :
			dprintk("SCENE_MODE_FIREWORKS    ");
			break;
		case SCENE_MODE_SPORTS    :
			dprintk("SCENE_MODE_SPORTS    ");
			break;
		case SCENE_MODE_PARTY   :
			dprintk("SCENE_MODE_PARTY   ");
			break;
		case SCENE_MODE_CANDLELIGHT    :
			dprintk("SCENE_MODE_CANDLELIGHT    ");
			break;
	}
	return 0;
}

int ov9650_set_focus_mode(focus_mode_flag_t flash_mode_flag,int arg)
{
	dprintk("ov9650_set_focus_mode");
	switch(flash_mode_flag)
	{
		case FOCUS_MODE_AUTO:
			dprintk("FOCUS_MODE_AUTO");
			break;
		case FOCUS_MODE_INFINITY:
			dprintk("FOCUS_MODE_INFINITY");
			break;
		case FOCUS_MODE_MACRO:
			dprintk("FOCUS_MODE_MACRO");
			break;
		case FOCUS_MODE_FIXED:
			dprintk("FOCUS_MODE_FIXED");
			break;
	}

	return 0;
}

int ov9650_set_power(int state)
{
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	switch (state)
	{
		case 0:
			/* hardware power up first */
			ov9650_power_up();
			/* software power up later if it implemented */
			break;
		case 1:
			ov9650_power_down();
			break;
		case 2:
			break;
		default:
			printk("%s : EINVAL! \n",__FUNCTION__);
	}
	return 0;
}

/* sensor_set_function use for init preview or capture.
 * there may be some difference between preview and capture.
 * so we divided it into two sequences.
 * param: function indicated which function
 * 0: preview
 * 1: capture
 * 2: recording
 */
int ov9650_set_effect(effect_flag_t effect_flag,int arg)
{
	dprintk("ov9650_set_effect");
	switch(effect_flag)
	{
		case EFFECT_NONE:
			dprintk("EFFECT_NONE");
			break;
		case EFFECT_MONO :
			dprintk("EFFECT_MONO ");
			break;
		case EFFECT_NEGATIVE :
			dprintk("EFFECT_NEGATIVE ");
			break;
		case EFFECT_SOLARIZE ://bao guang
			dprintk("EFFECT_SOLARIZE ");
			break;
		case EFFECT_SEPIA :
			dprintk("EFFECT_SEPIA ");
			break;
		case EFFECT_POSTERIZE ://se diao fen li
			dprintk("EFFECT_POSTERIZE ");
			break;
		case EFFECT_WHITEBOARD :
			dprintk("EFFECT_WHITEBOARD ");
			break;
		case EFFECT_BLACKBOARD :
			dprintk("EFFECT_BLACKBOARD ");
			break;
		case EFFECT_AQUA  ://qian lv se
			dprintk("EFFECT_AQUA  ");
			break;
		case EFFECT_PASTEL:
			dprintk("EFFECT_PASTEL");
			break;
		case EFFECT_MOSAIC:
			dprintk("EFFECT_MOSAIC");
			break;
		case EFFECT_RESIZE:
			dprintk("EFFECT_RESIZE");
			break;
	}

	return 0;
}


int ov9650_set_output_format(pixel_format_flag_t pixel_format_flag,int arg)
{
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	switch(pixel_format_flag)
	{
		case PIXEL_FORMAT_JPEG:
			printk("ov9650 set output format to jepg");
			break;
		case PIXEL_FORMAT_YUV422SP:
			printk("ov9650 set output format to yuv422sp");
			break;
		case PIXEL_FORMAT_YUV420SP:
			printk("ov9650 set output format to yuv420sp");
			break;
		case PIXEL_FORMAT_YUV422I:
			printk("ov9650 set output format to yuv422i");
			break;
		case PIXEL_FORMAT_RGB565:
			printk("ov9650 set output format to rgb565");
			break;
	}
	return 0;
}

#if 0
int ov9650_af_init(struct sensor_af_arg *info)
{
	unsigned short *reg  = info->buf;
	unsigned char *value = info->buf+2;
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	if(info->len % 3 != 0)
		return -1;
	while(value <= info->buf+info->len)
	{
		//printk("--------------------------------%4x %2x\n",*reg,*value);
		sensor_write_reg(ov9650_sensor_desc.client,*reg,*value);
		reg =(unsigned short*)((unsigned char *)reg + 3);
		value += 3;
	}

//	sensor_write_reg(ov9650_sensor_desc.client,0x3f00,0x01);//focus overlay
	return 0;
}
#endif

int ov9650_set_function(int function,void *cookie)
{
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	switch (function)
	{
	case 0:
		preview_set(ov9650_sensor_desc.client);
		break;
	case 1:
		capture_set(ov9650_sensor_desc.client);
		break;
	case 2:
		break;
	case 3:
		break;
	case 4:
		break;
	case 5:
		break;
	}

	return 0;
}

int ov9650_set_fps(int fps)
{
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	dprintk("set fps : %d",fps);
	return 0;
}

int ov9650_set_night_mode(int enable)
{
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	if(enable)
		dprintk("nightshot_mode enable!");
	else
		dprintk("nightshot_mode disable!");

	//__ov9650_set_night_mode(ov9650_sensor_desc.client,enable);

	return 0;
}

int ov9650_set_luma_adaptation(int arg)
{
	dprintk("luma_adaptation : %d",arg);
	return 0;
}

int ov9650_set_parameter(int cmd, int mode, int arg)
{
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	switch(cmd)
	{
		case CPCMD_SET_BALANCE :
    			ov9650_set_balance(mode,arg);
			break;
		case CPCMD_SET_EFFECT :
    			ov9650_set_effect(mode,arg);
			break;
		case CPCMD_SET_ANTIBANDING :
    			ov9650_set_antibanding(mode,arg);
			break;
		case CPCMD_SET_FLASH_MODE :
    			ov9650_set_flash_mode(mode,arg);
			break;
		case CPCMD_SET_SCENE_MODE :
   			ov9650_set_scene_mode(mode,arg);
			break;
		case CPCMD_SET_PIXEL_FORMAT :
    			ov9650_set_output_format(mode,arg);
			break;
		case CPCMD_SET_FOCUS_MODE :
    			ov9650_set_focus_mode(mode,arg);
			break;
		case CPCMD_SET_PREVIEW_FPS:
    			ov9650_set_fps(arg);
			break;
		case CPCMD_SET_NIGHTSHOT_MODE:
			ov9650_set_night_mode(arg);
			break;
		case CPCMD_SET_LUMA_ADAPTATION:
			ov9650_set_luma_adaptation(arg);
			break;
	}
	return 0;
}

int ov9650_sensor_init(void)
{
	dprintk("=======%s:%d\n", __FUNCTION__, __LINE__);
	ov9650_set_mclk(0);
	ov9650_reset();

	init_set(ov9650_sensor_desc.client);
	return 0;
}


int ov9650_sensor_probe(void)
{
	int retval = 0;

	ov9650_power_up();
	ov9650_reset();
	mdelay(10);

#if 0
	while (1) {
	retval = sensor_read_reg(ov9650_sensor_desc.client, 0x0a);	//read product id MSBs
	printk("%s L%d: retval = 0x%02x\n", __func__, __LINE__, retval);
	}
#endif
	retval = sensor_read_reg(ov9650_sensor_desc.client, 0x0a);	//read product id MSBs
	ov9650_power_down();

	printk("%s L%d: retval = 0x%02x\n", __func__, __LINE__, retval);

	if(retval == 0x96)	//ov9650 product id is 0x96
		return 0;
	else {
		printk("==>%s: error! pid is 0x%02x, should be 0x96!\n", __func__, retval);
		return -1;
	}
}



int ov9650_set_resolution(int width,int height,int bpp,pixel_format_flag_t fmt,camera_mode_t mode)
{
	dprintk("ov9650_set_resolution %d x %d",width,height);
	return 0;
}


static int ov9650_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	ov9650_sensor_desc.client = client;
#ifndef CONFIG_JZ4810_F4810
	sensor_set_i2c_speed(client,400000);// set ov9650 i2c speed : 400khz
#else
	sensor_set_i2c_speed(client,5000);// F4760: set ov9650 i2c speed : 5khz
#endif
	camera_sensor_register(&ov9650_sensor_desc);

	printk("OV9650 probed.\n");
	return 0;
}

struct camera_sensor_ops ov9650_sensor_ops = {
	.sensor_init = ov9650_sensor_init,
	.sensor_set_function =  ov9650_set_function,
	.sensor_set_resolution = ov9650_set_resolution,
	.sensor_set_parameter = ov9650_set_parameter,
	.sensor_set_power = ov9650_set_power,

	.camera_sensor_probe = ov9650_sensor_probe,
};


struct resolution_info ov9650_resolution_table[] = {
	{800,600,16,PIXEL_FORMAT_YUV422I},	//SVGA
	{640,480,16,PIXEL_FORMAT_YUV422I},	//VGA
	{352,288,16,PIXEL_FORMAT_YUV422I},	//CIF
	{320,240,16,PIXEL_FORMAT_YUV422I},	//QVGA
	{176,144,16,PIXEL_FORMAT_YUV422I},	//QCIF
};


struct camera_sensor_desc ov9650_sensor_desc = {
	.name = "ov9650",
	.camera_clock = CAM_CLOCK,
	.wait_frames = 2,
	.client = NULL,

	.scale = NULL,
	.ops = &ov9650_sensor_ops,

	.resolution_table = ov9650_resolution_table,
	.resolution_table_nr=ARRAY_SIZE(ov9650_resolution_table),

	.capture_parm = {800, 600, 16,PIXEL_FORMAT_YUV422I},
	.max_capture_parm = {800, 600, 16,PIXEL_FORMAT_YUV422I},

	.preview_parm = {640, 480, 16,PIXEL_FORMAT_YUV422I},
	.max_preview_parm = {640 , 480, 16,PIXEL_FORMAT_YUV422I},

	.cfg_info = {
		.configure_register= 0x0
			| CIM_CFG_PACK_2		/* pack mode : 3 2 1 4*/
#if !defined(CONFIG_SOC_JZ4780)
			| CIM_CFG_BYPASS		/* Bypass Mode */
#endif
			//|CIM_CFG_VSP             	/* VSYNC Polarity:1-falling edge active */
			//|CIM_CFG_PCP             	/* PCLK working edge:1-falling */
#ifndef CONFIG_JZ_CIM_IN_FMT_ITU656
			| CIM_CFG_DSM_GCM,		/* Gated Clock Mode */
#else
			| CIM_CFG_DF_ITU656
			| CIM_CFG_DSM_CIM, 		/* CCIR656 Interlace Mode */
#endif
	},

	.flags = {
		.focus_mode_flag = ~0x0,//FOCUS_MODE_INFINITY | FOCUS_MODE_AUTO,
		//.scene_mode_flag = ~0x0,
		.antibanding_flag = ~0x0,

		.effect_flag =		EFFECT_NONE
		       			|EFFECT_MONO
	       				|EFFECT_NEGATIVE
					|EFFECT_SEPIA
					|EFFECT_AQUA,

		.balance_flag = WHITE_BALANCE_AUTO | WHITE_BALANCE_CLOUDY_DAYLIGHT | WHITE_BALANCE_DAYLIGHT,
	},
};


static const struct i2c_device_id ov9650_id[] = {
	{ "ov9650", 0 },
	{ }	/* Terminating entry */
};
MODULE_DEVICE_TABLE(i2c, ov9650_id);

static struct i2c_driver ov9650_driver = {
	.probe		= ov9650_probe,
	.id_table	= ov9650_id,
	.driver	= {
		.name = "ov9650",
	},
};

static int __init ov9650_register(void)
{
	return i2c_add_driver(&ov9650_driver);
}

module_init(ov9650_register);




