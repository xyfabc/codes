#include <linux/i2c.h>
#include <media/jz_cim.h>
#include <asm/jzsoc.h>

#define CIM0_SENSOR_NAME	"ov3640-0"
#define CIM1_SENSOR_NAME	"ov3640-1"

#ifdef OV3640_DEBUG
#define dprintk(x...)   do{printk("OV3640---\t");printk(x);printk("\n");}while(0)
#else
#define dprintk(x...)
#endif

static struct resolution_info ov3640_resolution_table[] = {
	{2048, 1536, 16},
	{1600, 1200, 16},
	{1280, 1024, 16},
	{1024, 768, 16,},
	{800, 600, 16},
	{800, 480, 16},
	{640, 480, 16},
	{480, 320, 16},
	{352, 288, 16},
	{320, 240, 16},
	{176, 144, 16},
};

static void init_set(struct i2c_client *client)
{
	/*** VGA preview (640X480) 30fps 24MCLK input ***********/

	printk("%s %s()\n", __FILE__, __func__);

	sensor_write_reg16(client,0x3012,0x80);
	sensor_write_reg16(client,0x304d,0x45);
	sensor_write_reg16(client,0x30a7,0x5e);
	sensor_write_reg16(client,0x3087,0x16);
	sensor_write_reg16(client,0x309C,0x1a);
	sensor_write_reg16(client,0x30a2,0xe4);
	sensor_write_reg16(client,0x30aa,0x42);
	sensor_write_reg16(client,0x30b0,0xff);
	sensor_write_reg16(client,0x30b1,0xff);
	sensor_write_reg16(client,0x30b2,0x10);
	sensor_write_reg16(client,0x300e,0x39);
	sensor_write_reg16(client,0x300f,0x21);
	sensor_write_reg16(client,0x3010,0x20);
	sensor_write_reg16(client,0x304c,0x81);
	sensor_write_reg16(client,0x30d7,0x10);
	sensor_write_reg16(client,0x30d9,0x0d);
	sensor_write_reg16(client,0x30db,0x08);
	sensor_write_reg16(client,0x3016,0x82);
	sensor_write_reg16(client,0x3018,0x48);
	sensor_write_reg16(client,0x3019,0x40);
	sensor_write_reg16(client,0x301a,0x82);
	sensor_write_reg16(client,0x307d,0x00);
	sensor_write_reg16(client,0x3087,0x02);
	sensor_write_reg16(client,0x3082,0x20);
	//sensor_write_reg16(client,0x3015,0x12);//zi dong zeng yi
	sensor_write_reg16(client,0x3015,0x11);
	sensor_write_reg16(client,0x3014,0x84);
	sensor_write_reg16(client,0x3013,0xf7);
	sensor_write_reg16(client,0x303c,0x08);
	sensor_write_reg16(client,0x303d,0x18);
	sensor_write_reg16(client,0x303e,0x06);
	sensor_write_reg16(client,0x303F,0x0c);
	sensor_write_reg16(client,0x3030,0x62);
	sensor_write_reg16(client,0x3031,0x26);
	sensor_write_reg16(client,0x3032,0xe6);
	sensor_write_reg16(client,0x3033,0x6e);
	sensor_write_reg16(client,0x3034,0xea);
	sensor_write_reg16(client,0x3035,0xae);
	sensor_write_reg16(client,0x3036,0xa6);
	sensor_write_reg16(client,0x3037,0x6a);
	sensor_write_reg16(client,0x3104,0x02);
	sensor_write_reg16(client,0x3105,0xfd);
	sensor_write_reg16(client,0x3106,0x00);
	sensor_write_reg16(client,0x3107,0xff);
	sensor_write_reg16(client,0x3300,0x13);
	sensor_write_reg16(client,0x3301,0xde);
	sensor_write_reg16(client,0x3302,0xcf);
	sensor_write_reg16(client,0x3312,0x26);
	sensor_write_reg16(client,0x3314,0x42);
	sensor_write_reg16(client,0x3313,0x2b);
	sensor_write_reg16(client,0x3315,0x42);
	sensor_write_reg16(client,0x3310,0xd0);
	sensor_write_reg16(client,0x3311,0xbd);
	sensor_write_reg16(client,0x330c,0x18);
	sensor_write_reg16(client,0x330d,0x18);
	sensor_write_reg16(client,0x330e,0x56);
	sensor_write_reg16(client,0x330f,0x5c);
	sensor_write_reg16(client,0x330b,0x1c);
	sensor_write_reg16(client,0x3306,0x5c);
	sensor_write_reg16(client,0x3307,0x11);
	sensor_write_reg16(client,0x336a,0x52);
	sensor_write_reg16(client,0x3370,0x46);
	sensor_write_reg16(client,0x3376,0x38);
	sensor_write_reg16(client,0x30b8,0x20);
	sensor_write_reg16(client,0x30b9,0x17);
	sensor_write_reg16(client,0x30ba,0x04);
	sensor_write_reg16(client,0x30bb,0x08);
	sensor_write_reg16(client,0x3507,0x06);
	sensor_write_reg16(client,0x350a,0x4f);
	sensor_write_reg16(client,0x3100,0x02);
	sensor_write_reg16(client,0x3301,0xde);
	sensor_write_reg16(client,0x3304,0xfc);
	sensor_write_reg16(client,0x3400,0x00);
	sensor_write_reg16(client,0x3404,0x00);
	sensor_write_reg16(client,0x3600,0xc0);
	sensor_write_reg16(client,0x3088,0x08);
	sensor_write_reg16(client,0x3089,0x00);
	sensor_write_reg16(client,0x308a,0x06);
	sensor_write_reg16(client,0x308b,0x00);
	sensor_write_reg16(client,0x308d,0x04);
	sensor_write_reg16(client,0x3086,0x03);
	sensor_write_reg16(client,0x3086,0x00);
	sensor_write_reg16(client,0x30a9,0xbd);
	sensor_write_reg16(client,0x3317,0x04);
	sensor_write_reg16(client,0x3316,0xf8);
	sensor_write_reg16(client,0x3312,0x17);
	sensor_write_reg16(client,0x3314,0x30);
	sensor_write_reg16(client,0x3313,0x23);
	sensor_write_reg16(client,0x3315,0x3e);
	sensor_write_reg16(client,0x3311,0x9e);
	sensor_write_reg16(client,0x3310,0xc0);
	sensor_write_reg16(client,0x330c,0x18);
	sensor_write_reg16(client,0x330d,0x18);
	sensor_write_reg16(client,0x330e,0x5e);
	sensor_write_reg16(client,0x330f,0x6c);
	sensor_write_reg16(client,0x330b,0x1c);
	sensor_write_reg16(client,0x3306,0x5c);
	sensor_write_reg16(client,0x3307,0x11);
	sensor_write_reg16(client,0x3308,0x25);
	sensor_write_reg16(client,0x3340,0x20);
	sensor_write_reg16(client,0x3341,0x50);
	sensor_write_reg16(client,0x3342,0x18);
	sensor_write_reg16(client,0x3343,0x23);
	sensor_write_reg16(client,0x3344,0xad);
	sensor_write_reg16(client,0x3345,0xd0);
	sensor_write_reg16(client,0x3346,0xb8);
	sensor_write_reg16(client,0x3347,0xb4);
	sensor_write_reg16(client,0x3348,0x04);
	sensor_write_reg16(client,0x3349,0x98);
	sensor_write_reg16(client,0x3355,0x02);
	sensor_write_reg16(client,0x3358,0x44);
	sensor_write_reg16(client,0x3359,0x44);
	sensor_write_reg16(client,0x3300,0x13);
	sensor_write_reg16(client,0x3367,0x23);
	sensor_write_reg16(client,0x3368,0xBB);
	sensor_write_reg16(client,0x3369,0xD6);
	sensor_write_reg16(client,0x336A,0x2A);
	sensor_write_reg16(client,0x336B,0x07);
	sensor_write_reg16(client,0x336C,0x00);
	sensor_write_reg16(client,0x336D,0x23);
	sensor_write_reg16(client,0x336E,0xC3);
	sensor_write_reg16(client,0x336F,0xDE);
	sensor_write_reg16(client,0x3370,0x2b);
	sensor_write_reg16(client,0x3371,0x07);
	sensor_write_reg16(client,0x3372,0x00);
	sensor_write_reg16(client,0x3373,0x23);
	sensor_write_reg16(client,0x3374,0x9e);
	sensor_write_reg16(client,0x3375,0xD6);
	sensor_write_reg16(client,0x3376,0x29);
	sensor_write_reg16(client,0x3377,0x07);
	sensor_write_reg16(client,0x3378,0x00);
	sensor_write_reg16(client,0x332a,0x1d);
	sensor_write_reg16(client,0x331b,0x08);
	sensor_write_reg16(client,0x331c,0x16);
	sensor_write_reg16(client,0x331d,0x2d);
	sensor_write_reg16(client,0x331e,0x54);
	sensor_write_reg16(client,0x331f,0x66);
	sensor_write_reg16(client,0x3320,0x73);
	sensor_write_reg16(client,0x3321,0x80);
	sensor_write_reg16(client,0x3322,0x8c);
	sensor_write_reg16(client,0x3323,0x95);
	sensor_write_reg16(client,0x3324,0x9d);
	sensor_write_reg16(client,0x3325,0xac);
	sensor_write_reg16(client,0x3326,0xb8);
	sensor_write_reg16(client,0x3327,0xcc);
	sensor_write_reg16(client,0x3328,0xdd);
	sensor_write_reg16(client,0x3329,0xee);
	sensor_write_reg16(client,0x332e,0x04);
	sensor_write_reg16(client,0x332f,0x04);
	sensor_write_reg16(client,0x3331,0x02);
	sensor_write_reg16(client,0x3012,0x10);
	sensor_write_reg16(client,0x3023,0x06);
	sensor_write_reg16(client,0x3026,0x03);
	sensor_write_reg16(client,0x3027,0x04);
	sensor_write_reg16(client,0x302a,0x03);
	sensor_write_reg16(client,0x302b,0x10);
	sensor_write_reg16(client,0x3075,0x24);
	sensor_write_reg16(client,0x300d,0x01);
	sensor_write_reg16(client,0x30d7,0x90);
	sensor_write_reg16(client,0x3069,0x04);
	sensor_write_reg16(client,0x303e,0x00);
	sensor_write_reg16(client,0x303f,0xc0);
	sensor_write_reg16(client,0x3302,0xef);
	sensor_write_reg16(client,0x335f,0x34);
	sensor_write_reg16(client,0x3360,0x0c);
	sensor_write_reg16(client,0x3361,0x04);
	sensor_write_reg16(client,0x3362,0x34);
	sensor_write_reg16(client,0x3363,0x08);
	sensor_write_reg16(client,0x3364,0x04);
	sensor_write_reg16(client,0x3403,0x42);
	sensor_write_reg16(client,0x3088,0x04);
	sensor_write_reg16(client,0x3089,0x00);
	sensor_write_reg16(client,0x308a,0x03);
	sensor_write_reg16(client,0x308b,0x00);
	sensor_write_reg16(client,0x300e,0x32);
	sensor_write_reg16(client,0x300f,0x21);
	sensor_write_reg16(client,0x3010,0x20);
	sensor_write_reg16(client,0x304c,0x82);
	sensor_write_reg16(client,0x3302,0xef);
	sensor_write_reg16(client,0x335f,0x34);
	sensor_write_reg16(client,0x3360,0x0c);
	sensor_write_reg16(client,0x3361,0x04);
	sensor_write_reg16(client,0x3362,0x12);
	sensor_write_reg16(client,0x3363,0x88);
	sensor_write_reg16(client,0x3364,0xe4);
	sensor_write_reg16(client,0x3403,0x42);
	sensor_write_reg16(client,0x3088,0x12);
	sensor_write_reg16(client,0x3089,0x80);
	sensor_write_reg16(client,0x308a,0x01);
	sensor_write_reg16(client,0x308b,0xe0);
	sensor_write_reg16(client,0x304c,0x85);
	sensor_write_reg16(client,0x300e,0x39);
	sensor_write_reg16(client,0x300f,0xa1);//12.5fps -> 25fps
#if defined(CONFIG_JZ4810_F4810) || defined(CONFIG_JZ4775_F4775) || defined(CONFIG_JZ4780_F4780)
	sensor_write_reg16(client,0x3011,0x5); /* default 0x00, change ov3640 PCLK DIV here --- by Lutts*/
#else
	sensor_write_reg16(client,0x3011,0x0); /* default 0x00, change ov3640 PCLK DIV here --- by Lutts*/
#endif
	sensor_write_reg16(client,0x3010,0x81);
	sensor_write_reg16(client,0x302e,0xA0);
	sensor_write_reg16(client,0x302d,0x00);
	sensor_write_reg16(client,0x3071,0x82);
	sensor_write_reg16(client,0x301C,0x05);
}

static void enable_test_pattern(struct i2c_client *client)
{
	unsigned char value;

	/* test pattern(color bar) */
	value = sensor_read_reg16(client, 0x307b);
	value &= ~0x3;
	sensor_write_reg16(client, 0x307b, value | 0x2);

	value = sensor_read_reg16(client, 0x306c);
	sensor_write_reg16(client, 0x306c, value & ~0x10);

	value = sensor_read_reg16(client, 0x307d);
	sensor_write_reg16(client,0x307d,value | 0x80);

//	value = sensor_read_reg16(client, 0x3080);
//	sensor_write_reg16(client,0x3080,value | 0x80);
}

static void capture_reg_set(struct i2c_client *client)
{
	sensor_write_reg16(client,0x3012,0x00);
	sensor_write_reg16(client,0x3020,0x01);
	sensor_write_reg16(client,0x3021,0x1d);
	sensor_write_reg16(client,0x3022,0x00);
	sensor_write_reg16(client,0x3023,0x0a);
	sensor_write_reg16(client,0x3024,0x08);
	sensor_write_reg16(client,0x3025,0x18);
	sensor_write_reg16(client,0x3026,0x06);
	sensor_write_reg16(client,0x3027,0x0c);
	sensor_write_reg16(client,0x302a,0x06);
	sensor_write_reg16(client,0x302b,0x20);
	sensor_write_reg16(client,0x3075,0x44);
	sensor_write_reg16(client,0x300d,0x00);
	sensor_write_reg16(client,0x30d7,0x10);
	sensor_write_reg16(client,0x3069,0x40);
	sensor_write_reg16(client,0x303e,0x01);
	sensor_write_reg16(client,0x303f,0x80);
	sensor_write_reg16(client,0x3302,0xef);
	sensor_write_reg16(client,0x335f,0x68);
	sensor_write_reg16(client,0x3360,0x18);
	sensor_write_reg16(client,0x3361,0x0c);
	sensor_write_reg16(client,0x3362,0x68);
	sensor_write_reg16(client,0x3363,0x08);
	sensor_write_reg16(client,0x3364,0x04);
	sensor_write_reg16(client,0x3403,0x42);
	sensor_write_reg16(client,0x3088,0x08);
	sensor_write_reg16(client,0x3089,0x00);
	sensor_write_reg16(client,0x308a,0x06);
	sensor_write_reg16(client,0x308b,0x00);
	sensor_write_reg16(client,0x300e,0x39);
	sensor_write_reg16(client,0x300f,0x21);
	sensor_write_reg16(client,0x3010,0x20);
	sensor_write_reg16(client,0x304c,0x81);
	sensor_write_reg16(client,0x3366,0x10);
#if defined(CONFIG_JZ4810_F4770) || defined(CONFIG_JZ4775_F4775) || defined(CONFIG_JZ4780_F4780)
	sensor_write_reg16(client,0x3011,0x5); /* default 0x00, change ov3640 PCLK DIV here --- by lutts */
#else
	sensor_write_reg16(client,0x3011,0x0); /* default 0x00, change ov3640 PCLK DIV here --- by lutts */
#endif
	sensor_write_reg16(client,0x3f00,0x02);//disable overlay

//	enable_test_pattern(client);	//ylyuan
}

static void preview_set(struct i2c_client *client)
{
	unsigned char value;

	printk("%s %s()\n", __FILE__, __func__);

	sensor_write_reg16(client,0x3012,0x10);
	sensor_write_reg16(client,0x3023,0x06);
	sensor_write_reg16(client,0x3026,0x03);
	sensor_write_reg16(client,0x3027,0x04);
	sensor_write_reg16(client,0x302a,0x03);
	sensor_write_reg16(client,0x302b,0x10);
	sensor_write_reg16(client,0x3075,0x24);
	sensor_write_reg16(client,0x300d,0x01);
	sensor_write_reg16(client,0x30d7,0x90);
	sensor_write_reg16(client,0x3069,0x04);
	sensor_write_reg16(client,0x303e,0x00);
	sensor_write_reg16(client,0x303f,0xc0);
	sensor_write_reg16(client,0x3302,0xef);
	sensor_write_reg16(client,0x335f,0x34);
	sensor_write_reg16(client,0x3360,0x0c);
	sensor_write_reg16(client,0x3361,0x04);
	sensor_write_reg16(client,0x3362,0x12);
	sensor_write_reg16(client,0x3363,0x88);
	sensor_write_reg16(client,0x3364,0xe4);
	sensor_write_reg16(client,0x3403,0x42);
	sensor_write_reg16(client,0x3088,0x12);
	sensor_write_reg16(client,0x3089,0x80);
	sensor_write_reg16(client,0x308a,0x01);
	sensor_write_reg16(client,0x308b,0xe0);

	sensor_write_reg16(client,0x304c,0x85);
	//sensor_write_reg16(client,0x3366,0x10);

	sensor_write_reg16(client,0x300e,0x39);
	sensor_write_reg16(client,0x300f,0xa1);//12.5fps -> 25fps
	sensor_write_reg16(client,0x3010,0x81);
#if defined(CONFIG_JZ4810_F4810) || defined(CONFIG_JZ4775_F4775) || defined(CONFIG_JZ4780_F4780)
	sensor_write_reg16(client,0x3011,0x5); /* default: 0x00, change ov3640 PCLK here --- by Lutts */
#else
	sensor_write_reg16(client,0x3011,0x0); /* default: 0x00, change ov3640 PCLK here --- by Lutts */
#endif
	sensor_write_reg16(client,0x302e,0xa0);
	sensor_write_reg16(client,0x302d,0x00);
	sensor_write_reg16(client,0x3071,0x82);
	sensor_write_reg16(client,0x301C,0x05);

	sensor_write_reg16(client,0x3100,0x02);
	sensor_write_reg16(client,0x3301,0xde);
	sensor_write_reg16(client,0x3304,0xfc);

	/* reg 0x3404 */
#define YUV422_YUYV 0x00
#define YUV422_YVYU 0X01
#define YUV422_UYVY 0x02
#define YUV422_VYUY 0x03

#define Y8	   0x0D

#define YUV444_YUV 0x0E
#define YUV444_YVU 0x0F
#define YUV444_UYV 0x1C
#define YUV444_VYU 0x1D
#define YUV444_UVY 0x1E
#define YUV444_VUY 0x1F

	sensor_write_reg16(client,0x3400,0x00); /* source */
	//sensor_write_reg16(client,0x3404,0x00); /* fmt control */
	sensor_write_reg16(client,0x3404,YUV422_YUYV);
	//sensor_write_reg16(client,0x3404, YUV422_YVYU);
	//sensor_write_reg16(client,0x3404, YUV422_UYVY);
	//sensor_write_reg16(client,0x3404, YUV422_VYUY);

#ifdef CONFIG_VIDEO_CIM_IN_FMT_YUV444
	sensor_write_reg16(client,0x3404, YUV444_YUV);
#endif
	//sensor_write_reg16(client,0x3404, YUV444_YVU);
	//sensor_write_reg16(client,0x3404, YUV444_UVY);
	//sensor_write_reg16(client,0x3404, YUV444_VUY);

	//sensor_write_reg16(client,0x3404, Y8);

	sensor_write_reg16(client,0x3600,0xc0);

	sensor_write_reg16(client,0x3013,0xf7);

//	enable_test_pattern(client);	//ylyuan
}

static void size_switch(struct i2c_client *client, int width, int height)
{
	char value;

	if(width == 2048 && height == 1536)
	{
		sensor_write_reg16(client,0x304c, 0x81);
	}
	else if(width == 1600 && height == 1200)
	{
		sensor_write_reg16(client,0x304c, 0x81);
	}
	else if(width == 1280 && height == 1024)
	{
		sensor_write_reg16(client,0x304c, 0x82);
	}
	else if(width == 1024 && height == 768)
	{
		sensor_write_reg16(client,0x304c, 0x82);
	}
	else if(width == 800 && height == 600)
	{
		sensor_write_reg16(client,0x304c, 0x82);
	}
	else if(width == 800 && height == 480)
	{
		sensor_write_reg16(client,0x304c,0x82);
	}
	else if(width == 640 && height == 480)
	{
		sensor_write_reg16(client,0x304c, 0x82);
	}
	else if(width == 480 && height == 320)
	{
		sensor_write_reg16(client,0x304c,0x84);
	}
	else if(width == 352 && height == 288)
	{
		sensor_write_reg16(client,0x304c, 0x84);
	}
	else if(width == 320 && height == 240)
	{
		sensor_write_reg16(client,0x304c, 0x84);
	}
	else if(width == 176 && height == 144)
	{
		sensor_write_reg16(client,0x304c, 0x84);
	}
	else
		return;

	value = sensor_read_reg16(client,0x3012);

	if ( (value & 0x70) == 0 ) //FULL MODE
	{
		sensor_write_reg16(client,0x3302, 0xef);
		sensor_write_reg16(client,0x335f, 0x68);
		sensor_write_reg16(client,0x3360, 0x18);
		sensor_write_reg16(client,0x3361, 0x0c);
	}
	else
	{
		sensor_write_reg16(client,0x3302, 0xef);
		sensor_write_reg16(client,0x335f, 0x34);
		sensor_write_reg16(client,0x3360, 0x0c);
		sensor_write_reg16(client,0x3361, 0x04);
	}

	sensor_write_reg16(client,0x3362, (unsigned char)((((width+8)>>8) & 0x0F) + (((height+4)>>4)&0x70)) );
	sensor_write_reg16(client,0x3363, (unsigned char)((width+8) & 0xFF) );
	sensor_write_reg16(client,0x3364, (unsigned char)((height+4) & 0xFF) );

	sensor_write_reg16(client,0x3403, 0x42);

	sensor_write_reg16(client,0x3088, (unsigned char)((width>>8)&0xFF) );
	sensor_write_reg16(client,0x3089, (unsigned char)(width & 0xFF) );
	sensor_write_reg16(client,0x308a, (unsigned char)((height>>8)&0xFF) );
	sensor_write_reg16(client,0x308b, (unsigned char)(height & 0xFF) );

	sensor_write_reg16(client,0x3f00,0x12);//update zone
}

static void capture_set(struct i2c_client *client)
{
	uint8_t reg0x3001;
	uint8_t reg0x3002,reg0x3003,reg0x3013;
	uint8_t reg0x3028,reg0x3029;
	uint8_t reg0x302a,reg0x302b;
	uint8_t reg0x302d,reg0x302e;
	uint16_t shutter;
	uint16_t extra_lines, Preview_Exposure;
	uint16_t Preview_Gain16;
	uint16_t Preview_dummy_pixel;
	uint16_t Capture_max_gain16, Capture_banding_Filter;
	uint16_t Preview_line_width,Capture_line_width,Capture_maximum_shutter;
	uint16_t Capture_Exposure;

	uint8_t Mclk =24; //MHz
	uint8_t Preview_PCLK_frequency, Capture_PCLK_frequency;
	uint32_t Gain_Exposure,Capture_Gain16;
	uint8_t Gain;

	uint8_t capture_max_gain = 31;// parm,from 4* gain to 2* gain
	uint8_t Default_Reg0x3028 = 0x09;
	uint8_t Default_Reg0x3029 = 0x47;
	uint8_t Cap_Default_Reg0x302a = 0x06;
	uint8_t Cap_Default_Reg0x302b = 0x20;
	uint16_t capture_Dummy_pixel = 0;
	uint16_t capture_dummy_lines = 0;
	//uint16_t Default_XGA_Line_Width = 1188;
	uint16_t Default_QXGA_Line_Width = 2376;
	uint16_t Default_QXGA_maximum_shutter = 1563;

	printk("%s %s()\n", __FILE__, __func__);

	// 1. Stop Preview
	//Stop AE/AG
	reg0x3013 = sensor_read_reg16(client,0x3013);
	reg0x3013 = reg0x3013 & 0xf8;
	sensor_write_reg16(client,0x3013,reg0x3013);

	//Read back preview shutter
	reg0x3002 = sensor_read_reg16(client,0x3002);
	reg0x3003 = sensor_read_reg16(client,0x3003);
	shutter = (reg0x3002 << 8) + reg0x3003;

	//Read back extra line
	reg0x302d = sensor_read_reg16(client,0x302d);
	reg0x302e = sensor_read_reg16(client,0x302e);
	extra_lines = reg0x302e + (reg0x302d << 8);
	Preview_Exposure = shutter + extra_lines;

	//Read Back Gain for preview
	reg0x3001 = sensor_read_reg16(client,0x3001);
	Preview_Gain16 = (((reg0x3001 & 0xf0)>>4) + 1) * (16 + (reg0x3001 & 0x0f));

	//Read back dummy pixels
	reg0x3028 = sensor_read_reg16(client,0x3028);
	reg0x3029 = sensor_read_reg16(client,0x3029);
	Preview_dummy_pixel = (((reg0x3028 - Default_Reg0x3028) & 0xf0)<<8) + reg0x3029-Default_Reg0x3029;

	Preview_PCLK_frequency = (64 - 50) * 1 * Mclk / 1.5 / 2 / 2;
	Capture_PCLK_frequency = (64 - 57) * 1 * Mclk / 1.5 / 2 / 3;  // 7.5fps 56MHz

	// 2.Calculate Capture Exposure
	Capture_max_gain16 = capture_max_gain;

	//In common, Preview_dummy_pixel,Preview_dummy_line,Capture_dummy_pixel and
	//Capture_dummy_line can be set to zero.
	Preview_line_width = Default_QXGA_Line_Width + Preview_dummy_pixel ;

	Capture_line_width = Default_QXGA_Line_Width + capture_Dummy_pixel;
	if(extra_lines>5000)
	{
		Capture_Exposure = 16*11/10*Preview_Exposure * Capture_PCLK_frequency/Preview_PCLK_frequency *Preview_line_width/Capture_line_width;
	}
	else if(extra_lines>2000)
	{
		Capture_Exposure = 16*18/10*Preview_Exposure * Capture_PCLK_frequency/Preview_PCLK_frequency *Preview_line_width/Capture_line_width;
	}
	else if(extra_lines>100)
	{
		Capture_Exposure = 16*20/10*Preview_Exposure * Capture_PCLK_frequency/Preview_PCLK_frequency *Preview_line_width/Capture_line_width;
	}
	else
	{
		Capture_Exposure = 16*22/10*Preview_Exposure * Capture_PCLK_frequency/Preview_PCLK_frequency *Preview_line_width/Capture_line_width;
	}
	if(Capture_Exposure == 0)
	{
		Capture_Exposure =1 ;
	}

	//Calculate banding filter value
	//If (50Hz) {Capture_banding_Filter = Capture_PCLK_Frequency/ 100/ (2*capture_line_width);
	//else {(60Hz)Capture_banding_Filter = Capture_PCLK_frequency /120 /(2*capture_line_width);
	Capture_banding_Filter = (uint16_t)((float)Capture_PCLK_frequency * 1000000 / 100/ (2*Capture_line_width)+0.5);

	Capture_maximum_shutter = (Default_QXGA_maximum_shutter + capture_dummy_lines)/Capture_banding_Filter;
	Capture_maximum_shutter = Capture_maximum_shutter * Capture_banding_Filter;

	//redistribute gain and exposure
	Gain_Exposure = Preview_Gain16 * Capture_Exposure/16;
	if( Gain_Exposure ==0)
	{
		Gain_Exposure =1;
	}

	if (Gain_Exposure < (Capture_banding_Filter * 16))
	{
		// Exposure < 1/100
		Capture_Exposure = Gain_Exposure /16;
		Capture_Gain16 = (Gain_Exposure*2 + 1)/Capture_Exposure/2;
	}
	else

	{
		if (Gain_Exposure > Capture_maximum_shutter * 16)
		{
			// Exposure > Capture_Maximum_Shutter
			Capture_Exposure = Capture_maximum_shutter;
			Capture_Gain16 = (Gain_Exposure*2 + 1)/Capture_maximum_shutter/2;

			if (Capture_Gain16 > Capture_max_gain16)
			{
				// gain reach maximum, insert extra line
				Capture_Exposure = Gain_Exposure*11/10/Capture_max_gain16;
				// For 50Hz, Exposure = n/100; For 60Hz, Exposure = n/120
				Capture_Exposure = Capture_Exposure/Capture_banding_Filter;
				Capture_Exposure =Capture_Exposure * Capture_banding_Filter;
				Capture_Gain16 = (Gain_Exposure *2+1)/ Capture_Exposure/2;
			}
		}
		else
		{
			// 1/100(120) < Exposure < Capture_Maximum_Shutter, Exposure = n/100(120)
			Capture_Exposure = Gain_Exposure/16/Capture_banding_Filter;
			Capture_Exposure = Capture_Exposure * Capture_banding_Filter;
			Capture_Gain16 = (Gain_Exposure*2 +1) / Capture_Exposure/2;
		}
	}

	// 3.Switch to QXGA
	/*// Write registers, change to QXGA resolution.*/
/***********************************************************************/

	capture_reg_set(client);
	// 4.Write Registers
	//write dummy pixels
	reg0x3029 = sensor_read_reg16(client,0x3029);
	reg0x3029 = reg0x3029 + (capture_Dummy_pixel & 0x00ff);

	reg0x3028 = sensor_read_reg16(client,0x3028);
	reg0x3028 = (reg0x3028 & 0x0f) | ((capture_Dummy_pixel & 0x0f00)>>4);


	sensor_write_reg16(client,0x3028, reg0x3028);
	sensor_write_reg16(client,0x3029, reg0x3029);

	//Write Dummy Lines
	reg0x302b = (capture_dummy_lines & 0x00ff ) + Cap_Default_Reg0x302b;
	reg0x302a =( capture_dummy_lines >>8 ) + Cap_Default_Reg0x302a;
	sensor_write_reg16(client,0x302a, reg0x302a);
	sensor_write_reg16(client,0x302b, reg0x302b);

	//Write Exposure
	if (Capture_Exposure > Capture_maximum_shutter)
	{
		shutter = Capture_maximum_shutter;
		extra_lines = Capture_Exposure - Capture_maximum_shutter;
	}
	else
	{
		shutter = Capture_Exposure;
		extra_lines = 0;
	}

	reg0x3003 = shutter & 0x00ff;
	reg0x3002 = (shutter>>8) & 0x00ff;
	sensor_write_reg16(client,0x3003, reg0x3003);
	sensor_write_reg16(client,0x3002, reg0x3002);

	// Write extra line
	reg0x302e= extra_lines & 0x00ff;
	reg0x302d= extra_lines >> 8;
	sensor_write_reg16(client,0x302d, reg0x302d);
	sensor_write_reg16(client,0x302e, reg0x302e);

	// Write Gain
	Gain = 0;
	if (Capture_Gain16 > 31)
	{
		Capture_Gain16 = Capture_Gain16 /2;
		Gain = 0x10;
	}
	if (Capture_Gain16 > 31)
	{
		Capture_Gain16 = Capture_Gain16 /2;
		Gain = Gain| 0x20;
	}
	if (Capture_Gain16 > 31)
	{
		Capture_Gain16 = Capture_Gain16 /2;
		Gain = Gain | 0x40;
	}
	if (Capture_Gain16 > 31)
	{
		Capture_Gain16 = Capture_Gain16 /2;
		Gain = Gain | 0x80;
	}
	if (Capture_Gain16 > 16)
	{
		Gain = Gain | ((uint32_t)Capture_Gain16 -16);
	}

	sensor_write_reg16(client,0x3001, Gain+0x05);
//	mdelay(500);
//	sensor_write_reg16(client,0x3001, Gain);
}

static void ov3640_power_down(int pd_pin)
{
	/* Power Down: active high */
#if defined(CONFIG_JZ4760_LEPUS) || defined(CONFIG_JZ4760B_LEPUS)
	__gpio_as_output(pd_pin);
	__gpio_set_pin(pd_pin);
#elif defined(CONFIG_JZ4770_PISCES)
	__gpio_as_output1(pd_pin);
#endif
	mdelay(5);
}

static void ov3640_power_up(int pd_pin)
{
#if defined(CONFIG_JZ4760_LEPUS) || defined(CONFIG_JZ4760B_LEPUS)
	__gpio_as_output(pd_pin);
	__gpio_clear_pin(pd_pin);
#elif defined(CONFIG_JZ4770_PISCES)
	__gpio_as_output0(pd_pin);
#endif
	mdelay(5);
}

static int ov3640_set_mclk(unsigned int mclk)
{
	/*
	 * It has a 24MHz oscillator to supply Mclk on RD47xx_Camera_Board.
	 */

	// __cim_set_master_clk(__cpm_get_hclk(), c->mclk);
	return 0;
}

static void ov3640_reset(int rst_pin)
{
	/* Reset: active-low */
#if defined(CONFIG_JZ4770_F4770) || defined(CONFIG_JZ4770_PISCES) || defined(CONFIG_JZ4775_F4775) || defined(CONFIG_JZ4780_F4780)
	__gpio_as_output0(rst_pin);
	mdelay(50);
	__gpio_as_output1(rst_pin);
	mdelay(50);
#else
	__gpio_clear_pin(rst_pin);
	mdelay(50);
	__gpio_set_pin(rst_pin);
	mdelay(50);
#endif
}

static int ov3640_set_power(struct jz_sensor_desc *desc, int state)
{
	switch (state)
	{
		case 0:
			ov3640_power_up(desc->pd_pin);
			break;
		case 1:
			ov3640_power_down(desc->pd_pin);
			break;
		case 2:
			break;
		default:
			printk("%s : invalid state %d! \n", __func__, state);
			return -EINVAL;
	}
	return 0;
}

static int ov3640_sensor_init(struct jz_sensor_desc *desc)
{
	ov3640_set_mclk(desc->mclk);
	ov3640_reset(desc->rst_pin);

	init_set(desc->client);
	return 0;
}

static int ov3640_sensor_probe(struct jz_sensor_desc *desc)
{
	int id = 0;

	ov3640_power_up(desc->pd_pin);
	ov3640_reset(desc->rst_pin);
	mdelay(10);

	id = sensor_read_reg16(desc->client, 0x300a);	//read product id MSBs
	ov3640_power_down(desc->pd_pin);

	printk("%s L%d: id = 0x%02x\n", __func__, __LINE__, id);

	if(id != 0x36) {
		printk("==>%s: error! pid is 0x%02x, should be 0x36!\n", __func__, id);
		return -1;
	}

	return 0;
}

static int ov3640_set_resolution(struct jz_sensor_desc *desc, struct resolution_info *res)
{
	size_switch(desc->client, res->width, res->height);
	return 0;
}

static int ov3640_set_function(struct jz_sensor_desc *desc, enum sensor_mode_t mode)
{
	switch (mode)
	{
	case SENSOR_MODE_PREVIEW:
		preview_set(desc->client);
		break;
	case SENSOR_MODE_CAPTURE:
		capture_set(desc->client);
		break;
	default:
		printk("%s: invalid function!\n", __func__);
		break;
	}

	return 0;
}

static struct jz_sensor_ops ov3640_sensor_ops = {
	.sensor_init		= ov3640_sensor_init,
	.sensor_probe		= ov3640_sensor_probe,
	.sensor_set_resolution	= ov3640_set_resolution,
	.sensor_set_power	= ov3640_set_power,
	.sensor_set_function	= ov3640_set_function,
};

static int ov3640_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct jz_sensor_desc *desc = NULL;
	struct jz_sensor_platform_data *priv = client->dev.platform_data;

	if (!id)
		return -EINVAL;

	desc = kzalloc(sizeof(*desc), GFP_KERNEL);
	if (!desc) {
		printk(KERN_ERR "%s: kzalloc failed!\n", __func__);
		return -ENOMEM;
	}
	desc->client = client;
	strcpy(desc->name, id->name);
	
	desc->cim_id = priv->cim_id;	/* attached to cim 0/1 */
	desc->rst_pin = priv->rst_pin;
	desc->pd_pin = priv->pd_pin;

	desc->mclk = 24000000;
	desc->i2c_clk = 400000;		/* set i2c speed: 400KHZ */
	desc->bus_width = 8;
	desc->wait_frames = 2;
#ifdef CONFIG_VIDEO_CIM_IN_FMT_YUV444
	desc->fourcc = V4L2_PIX_FMT_YUV444;
#else
	desc->fourcc = V4L2_PIX_FMT_YUYV;
#endif
	desc->ops = &ov3640_sensor_ops;
	/* max capture resolution: 2048 x 1536 x 16 bits */
	memcpy(&desc->cap_res, &ov3640_resolution_table[0], sizeof(struct resolution_info));
	/* max preview resolution: 1024 x 768 x 16 bits */
	memcpy(&desc->pre_res, &ov3640_resolution_table[3], sizeof(struct resolution_info));
	desc->resolution_table	= ov3640_resolution_table;
	desc->table_nr = ARRAY_SIZE(ov3640_resolution_table);

	sensor_set_i2c_speed(client, desc->i2c_clk);

	/* attaching sensor to CIM host controller */
	jz_sensor_register(desc);

	i2c_set_clientdata(client, desc);

	printk(KERN_INFO "%s: %s probed\n", __func__, desc->name);

	return 0;
}

static int ov3640_remove(struct i2c_client *client)
{
//	struct jz_sensor_desc *desc = container_of(client, struct jz_sensor_desc, client);
	struct jz_sensor_desc *desc = i2c_get_clientdata(client);

	printk(KERN_INFO "%s: %s probed\n", __func__, desc->name);

	i2c_set_clientdata(client, NULL);
	kfree(desc);

	return 0;
}

/*
 * i2c_device_id.name must be same with i2c_board_info.type !
 */
static const struct i2c_device_id ov3640_id[] = {
	{ CIM0_SENSOR_NAME, 0 },
	{ CIM1_SENSOR_NAME, 0 },
	{ }	/* Terminating entry */
};
MODULE_DEVICE_TABLE(i2c, ov3640_id);

static struct i2c_driver ov3640_driver = {
	.probe		= ov3640_probe,
	.remove		= ov3640_remove,
	.id_table	= ov3640_id,
	.driver	= {
		.name = "ov3640",
	},
};

static int __init ov3640_init(void)
{
	return i2c_add_driver(&ov3640_driver);
}

static void __exit ov3640_exit(void)
{
	i2c_del_driver(&ov3640_driver);
}

module_init(ov3640_init);
module_exit(ov3640_exit);
