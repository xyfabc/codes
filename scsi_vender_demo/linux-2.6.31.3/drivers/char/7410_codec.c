/*
 *  linux/drivers/sound/jzcodec.c
 *
 *  JzSOC internal audio driver.
 *
 */
#include <linux/init.h>
#include <linux/interrupt.h>
//#include <sound/driver.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/i2c.h>

#include <asm/jzsoc.h>
#include <linux/fs.h>

#include <linux/random.h>

static struct i2c_client * codec_i2c = NULL;
int codec_write_reg(unsigned int reg, u8 value) {
	int ret = -1;
	unsigned char buf[2];

	if (codec_i2c == NULL)
		return -EINVAL;

	buf[0] = reg;
	buf[1] = value;

	ret = i2c_master_send(codec_i2c, buf, 2);
	if ( ret != 2)
		printk("7410 write failed!, ret = %d\n", ret);

	return ret;
}
unsigned short codec_read_reg16(unsigned char reg) {
	int ret = -1, tmp;
	unsigned char buf[2];
	unsigned short retval;

	if (codec_i2c == NULL)
		return -EINVAL;

	ret = i2c_master_send(codec_i2c, &reg, 1);
	if ( ret != 1){
		printk("7410 write failed!, ret = %d\n", ret);
		return ret;
	}
	ret = i2c_master_recv(codec_i2c, buf, 2);
        if (ret != 2) {
                printk("%s: ret = %d\n", __FUNCTION__, ret);
                return ret;
        }
	retval = (buf[0] << 8) | buf[1];
	return retval;
}
int codec_write_reg16(unsigned int reg, unsigned short value) {
	int ret = -1, tmp;
	unsigned char buf[3];

	if (codec_i2c == NULL)
		return -EINVAL;

	buf[0] = reg;
	buf[1] = (value >> 8) & 0xff;
	buf[2] = value & 0xff;

	ret = i2c_master_send(codec_i2c, buf, 3);
	if ( ret != 3)
		printk("7410 write failed!, ret = %d\n", ret);

	return ret;
}


int codec_write_regn(unsigned int reg, unsigned short *data, int len) {
	int i, data_len = 1;
	int ret = -1, tmp;
	unsigned char *buff = (unsigned char *)kmalloc((len * 2 + 1), GFP_KERNEL);
	if(!buff){
		printk("Can not malloc enough place..\n");
		return -1;
	}
	if (codec_i2c == NULL)
		return -EINVAL;

	buff[0] = reg;
	for(i = 0; i < len; i++){
		buff[data_len++] = (data[i] >> 8) & 0xff;
		buff[data_len++] = data[i] & 0xff;
	}

/*
	printk("start data...\n");
	for(i = 0; i < len; i++){
		printk("0x%04x  ", data[i]);
		if(i % 8 == 7)
			printk("\n");
	}
	printk("after data .....\n");
	for(i = 0; i < data_len; i++){
		if(i == 0)	
			printk("addr = 0x%02x\n", buff[i]);
		else{
			printk("0x%02x  ", buff[i]);
			if(i % 8 == 7)
				printk("\n");
		}
	
	}
*/
	ret = i2c_master_send(codec_i2c, buff, data_len);
	if ( ret != data_len)
		printk("7410 write failed!, ret = %d\n", ret);

	return ret;
}
unsigned short codec_read_regn(unsigned char reg, unsigned short *data, int len) {
	int i, j = 0;
	int ret = -1, data_len = (2 * len);
	unsigned short retval;
	unsigned char *buff = (unsigned char *)kmalloc((len * 2), GFP_KERNEL);
	if(!buff){
		printk("Can not malloc enough read place...\n");
		return -1;
	}

	if (codec_i2c == NULL)
		return -EINVAL;
	ret = i2c_master_send(codec_i2c, &reg, 1);
	if ( ret != 1){
		printk("7410 write failed!, ret = %d\n", ret);
		return ret;
	}
	ret = i2c_master_recv(codec_i2c, buff, data_len);
	if (ret != data_len) {
		printk("%s: ret = %d\n", __FUNCTION__, ret);
		return ret;
	}
	for(i = 0; i < data_len; i = (i + 2))
		data[j++] = ((buff[i] << 8) | buff[i + 1]);

	return ret;
}

extern void i2c_jz_setclk(struct i2c_client *client,unsigned long i2cclk);
static __devinit int codec_i2c_probe(struct i2c_client *i2c,
		const struct i2c_device_id *id)
{
	int retval;
	codec_i2c = i2c;
	i2c_jz_setclk(i2c, 80*1000);

	msleep(1000);

	

	return 0;
}

static const struct i2c_device_id codec_i2c_id[] = {
	{ "jz7410", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, codec_i2c_id);

static struct i2c_driver codec_i2c_driver = {
	.driver = {
		.name = "7410_codec",
		.owner = THIS_MODULE,
	},
	.probe =    codec_i2c_probe,
	.id_table = codec_i2c_id,
};

static inline unsigned int new_rand(void)
{
        return get_random_int();
}

static struct timer_list codec_timer;

static int  codec_i2c_test(void)
{

	int i;
	unsigned short *buff = (unsigned short *)kmalloc((sizeof(unsigned short) * 256), GFP_KERNEL);
	if(!buff)
	{
		printk("Can not malloc enough place...\n");
		return -1;
	}
	unsigned short *read_data = (unsigned short *)kmalloc((sizeof(unsigned short) * 256), GFP_KERNEL);
	if(!read_data)
	{
		printk("Can not malloc enough data place ....\n");
		return -1;
	
	}
	memset(read_data, 0, sizeof(unsigned short) * 256);

	for(i =0; i <= 256; i++)
		buff[i]	= new_rand() & 0xffff;

	//test 0x00 - 0x1e address write/read end once
	printk("write data form 0x%02x\n", 0x0);
	for(i = 0; i <= 30; i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7)
			printk("\n");
	}
	codec_write_regn(0x0, buff, 31);

	codec_read_regn(0x0, read_data, 31);
	printk("Read data form 0x%02x .....\n", 0x00);
	for(i = 0; i <= 30; i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3)
			printk("\n");
	}

	printk("\n\n");

	//read /write  0x20 to 0xff register once
	printk("write data form 0x%02x\n", 32);
	for(i = 0x20; i <= 0xff; i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7)
			printk("\n");
	}
	codec_write_regn(0x20, (buff + 0x20), (0xff - 0x20 + 1));

	codec_read_regn(0x20, (read_data + 0x20), (0xff - 0x20 + 1));
	printk("Read data from 0x%02x .....\n", 0x20);
	for(i = 0x20; i <= 0xff; i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3)
			printk("\n");
	}

	kfree(buff);
	kfree(read_data);
	printk("\n\n===========================================\n\n");

}
static void i2c_test_func(unsigned long data) {

	codec_i2c_test();
	codec_timer.expires = jiffies + HZ;
	add_timer(&codec_timer);
}

static int i2c_test(void) {

	printk("====start test on i2c%d=====\n", __LINE__);

	init_timer(&codec_timer);
	codec_timer.function = i2c_test_func;
	codec_timer.expires = jiffies + HZ;

	add_timer(&codec_timer);

	return 0;
}
#if 1
//for wlyang codec register check

#define CODEC_BASE          0x80

#define PM_DAC_L            (02 + CODEC_BASE)
#define PM_DAC_H            (03 + CODEC_BASE)
#define PM_ADC_L            (04 + CODEC_BASE)
#define PM_ADC_H            (05 + CODEC_BASE)
#define FCR                 (06 + CODEC_BASE)
#define CR_MUTE1_L          (16 + CODEC_BASE)
#define CR_MUTE1_H          (17 + CODEC_BASE)
#define CR_MUTE2            (24 + CODEC_BASE)
#define CR_BIAS             (25 + CODEC_BASE)
#define ODS                 (26 + CODEC_BASE)
#define GCR_L               (28 + CODEC_BASE)
#define GCR_H               (29 + CODEC_BASE)
#define OTHER1_L            (18 + CODEC_BASE)
#define OTHER1_H            (19 + CODEC_BASE)
#define OTHER2_L            (20 + CODEC_BASE)
#define OTHER2_H            (21 + CODEC_BASE)

#define PM_DAC_L_DEF        0x0000
#define PM_DAC_H_DEF        0x0000
#define PM_ADC_L_DEF        0x0400
#define PM_ADC_H_DEF        0x0000
#define FCR_DEF             0x0077
#define CR_MUTE1_L_DEF      0xf3f3
#define CR_MUTE1_H_DEF      0x00d0
#define CR_MUTE2_DEF        0x1bfc
#define CR_BIAS_DEF         0x36db
#define OD_DEFS             0x0490
#define GCR_L_DEF           0xd1a3
#define GCR_H_DEF           0x698c
#define OTHER1_L_DEF        0x0000
#define OTHER1_H_DEF        0x0004
#define OTHER2_L_DEF        0x0000
#define OTHER2_H_DEF        0x0000

#define i2c_rd(x)	codec_read_reg16(x)

int TEST_NAME(void)
{
	unsigned short read_data;
	unsigned int error = 0;
	printk("Start!\n");
	read_data = i2c_rd(PM_DAC_L  ); if(read_data != PM_DAC_L_DEF  ){printk("Read PM_DAC_L    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(PM_DAC_H  ); if(read_data != PM_DAC_H_DEF  ){printk("Read PM_DAC_H    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(PM_ADC_L  ); if(read_data != PM_ADC_L_DEF  ){printk("Read PM_ADC_L    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(PM_ADC_H  ); if(read_data != PM_ADC_H_DEF  ){printk("Read PM_ADC_H    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(FCR       ); if(read_data != FCR_DEF       ){printk("Read FCR         register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(CR_MUTE1_L); if(read_data != CR_MUTE1_L_DEF){printk("Read CR_MUTE1_L  register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(CR_MUTE1_H); if(read_data != CR_MUTE1_H_DEF){printk("Read CR_MUTE1_H  register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(CR_MUTE2  ); if(read_data != CR_MUTE2_DEF  ){printk("Read CR_MUTE2    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(CR_BIAS   ); if(read_data != CR_BIAS_DEF   ){printk("Read CR_BIAS     register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(ODS       ); if(read_data != OD_DEFS       ){printk("Read ODS         register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(GCR_L     ); if(read_data != GCR_L_DEF     ){printk("Read GCR_L       register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(GCR_H     ); if(read_data != GCR_H_DEF     ){printk("Read GCR_H       register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(OTHER1_L  ); if(read_data != OTHER1_L_DEF  ){printk("Read OTHER1_L    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(OTHER1_H  ); if(read_data != OTHER1_H_DEF  ){printk("Read OTHER1_H    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(OTHER2_L  ); if(read_data != OTHER2_L_DEF  ){printk("Read OTHER2_L    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}
	read_data = i2c_rd(OTHER2_H  ); if(read_data != OTHER2_H_DEF  ){printk("Read OTHER2_H    register default is ERROR! Read data is 0x%4x\n",read_data); error = 1;}

	return error;
}



void svmain_itbench()
{
	int error;
	error = TEST_NAME();
	printk("\n>>>>>>> ------------------- >>>>>>>>>>\n"); 
	if(error == 0)
		printk("\tSimulation is PASS!\n");
	else
		printk("\n\tSimulation is FILE!\n");
	printk("<<<<<<< ------------------- <<<<<<<<<<\n\n");
}



#endif
static int i2c_test_dma(void)
{
	int i;
	int test_len;
	unsigned short *buff = (unsigned short *)kmalloc((sizeof(unsigned short) * 256), GFP_KERNEL);
	if(!buff)
	{
		printk("Can not malloc enough place...\n");
		return -1;
	}
	unsigned short *read_data = (unsigned short *)kmalloc((sizeof(unsigned short) * 256), GFP_KERNEL);
	if(!read_data)
	{
		printk("Can not malloc enough data place ....\n");
		return -1;
	
	}
	memset(read_data, 0, sizeof(unsigned short) * 256);

	for(i =0; i <= 256; i++)
		buff[i]	= new_rand() & 0xffff;


	test_len = 7;
	printk("=====>write data, test_len = %d\n", (test_len * 2));
	for(i = 0; i < test_len; i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 0)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(0x0, buff, test_len);
	codec_read_regn(0x0, read_data, test_len);
	printk("=====>Check data....\n");
	for(i = 0; i < test_len; i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3 && i != 0)
			printk("\n");
	}
	printk("\n************************\n\n");

	test_len = 9;
	printk("=====>Write data, test_len = %d\n", (test_len * 2));
	for(i = 7; i < (test_len + 7); i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 7)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(7, (buff + 7), test_len);
	codec_read_regn(7, (read_data + 7), test_len);
	printk("======>Check data ...\n");
	for(i = 7; i < (test_len + 7); i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3 && i != 7)
			printk("\n");
	}
	printk("\n*************************\n\n");

	test_len = 15;
	printk("=====>Write data, test_len = %d\n",(test_len * 2));
	for(i = 0x20; i < (test_len + 0x20); i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 0x20)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(0x20, (buff + 0x20), test_len);
	codec_read_regn(0x20, (read_data + 0x20), test_len);
	printk("Check data .....\n");
	for(i = 0x20; i < (test_len + 0x20); i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3 && i != 0x20)
			printk("\n");
	}
	printk("\n*****************************\n\n");


	test_len = 17;
	printk("Write data, test_len = %d\n",(test_len * 2));
	for(i = 0x30; i < (test_len + 0x30); i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 0x30)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(0x30, (buff + 0x30), test_len);
	codec_read_regn(0x30, (read_data + 0x30), test_len);
	printk("Check data ....\n");
	for(i = 0x30; i < (test_len + 0x30); i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3)
			printk("\n");
	}
	printk("\n*****************************\n\n");

	test_len = 31;
	printk("Write data, test_len = %d\n",(test_len * 2));
	for(i = 0x42; i < (test_len + 0x42); i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 0x42)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(0x42, (buff + 0x42), test_len);
	codec_read_regn(0x42, (read_data + 0x42), test_len);
	printk("Check data ....\n");
	for(i = 0x42; i < (test_len + 0x42); i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3 && i != 0x42)
			printk("\n");
	}
	printk("\n*****************************\n\n");


	test_len = 33;
	printk("Write data, test_len = %d\n",(test_len * 2));
	for(i = 0x63; i < (test_len + 0x63); i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 0x63)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(0x63, (buff + 0x63), test_len);
	codec_read_regn(0x63, (read_data + 0x63), test_len);
	printk("Check data ....\n");
	for(i = 0x63; i < (test_len + 0x63); i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3 && i != 0x63)
			printk("\n");
	}
	printk("\n*****************************\n\n");

	test_len = 63;
	printk("Write data, test_len = %d\n",(test_len * 2));
	for(i = 0x7b; i < (test_len + 0x7b); i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 0x7b)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(0x7b, (buff + 0x7b), test_len);
	codec_read_regn(0x7b, (read_data + 0x7b), test_len);
	printk("Check data ....\n");
	for(i = 0x7b; i < (test_len + 0x7b); i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3 && i != 0x7b)
			printk("\n");
	}
	printk("\n*****************************\n\n");

	test_len = 65;
	printk("Write data, test_len = %d\n",(test_len * 2));
	for(i = 0xbb; i < (test_len + 0xbb); i++){
		printk("0x%04x  ", buff[i]);
		if(i % 8 == 7 && i != 0xbb)
			printk("\n");
	}
	printk("\n");
	codec_write_regn(0xbb, (buff + 0xbb), test_len);
	codec_read_regn(0xbb, (read_data + 0xbb), test_len);
	printk("Check data ....\n");
	for(i = 0xbb; i < (test_len + 0xbb); i++){
		printk("addr(0x%02x) = 0x%04x ", i, read_data[i]);
		if(read_data[i] != buff[i]){
			printk("error  ");
			while(1);
		}
		if(i % 4 == 3 && i != 0xbb)
			printk("\n");
	}
	printk("\n*****************************\n\n");
	kfree(read_data);
	kfree(buff);
	return 0;
}
static int __init init_jzcodec(void)
{
	int ret = -1;
	unsigned short retval;
	unsigned short data;
	int i;
	int reg_addr = 0x80;

	ret = i2c_add_driver(&codec_i2c_driver);
	if (ret != 0) {
		printk(KERN_ERR "Failed to register 7410 I2C driver: %d\n", ret);
		return ret;
	}

#if 1
	codec_write_reg16(0x0, 0x4567);
	retval = codec_read_reg16(0x0);
	printk("read 0x0 value is 0x%04x\n", retval);

	codec_write_reg16(0x01, 0xa5a5);
	retval = codec_read_reg16(0x01);
	printk("read 0x1 value is 0x%04x\n", retval);

	while(1){
		printk("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
		i2c_test_dma();
		printk("\nXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX\n\n");
		msleep(1000);
	}

#endif
	printk("\n\n#####7410 codec.c-------%s  line=%d\n\n",__FUNCTION__,__LINE__);
	return 0;
}


static void __exit cleanup_jzcodec(void)
{
	i2c_del_driver(&codec_i2c_driver);
}

module_init(init_jzcodec);
module_exit(cleanup_jzcodec);
