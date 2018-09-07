/*
 * OV9650 CMOS Camera Sensor Initialization
 */
#include "../../jz_sensor.h"
#include "../../jz_cim_core.h"

/*
 *;09032004
 *;OV9650
 *;VGA YUV
 *;12.5fps when 24MHz input clock
 *;Device Address(Hex)/Register(Hex)/Value(Hex)
 *;
 */
static void ov9650_vga(struct i2c_client *client)
{
	sensor_write_reg(client, 0x12, 0x80);
	sensor_write_reg(client, 0x11, 0x81);
//	sensor_write_reg(client, 0x11, 0x80);
	sensor_write_reg(client, 0x6b, 0x0a);
	sensor_write_reg(client, 0x6a, 0x3e);
	sensor_write_reg(client, 0x3b, 0x09);
//	sensor_write_reg(client, 0x3b, 0x81);
	sensor_write_reg(client, 0x13, 0xe0);
	sensor_write_reg(client, 0x01, 0x80);
	sensor_write_reg(client, 0x02, 0x80);
	sensor_write_reg(client, 0x00, 0x00);
	sensor_write_reg(client, 0x10, 0x00);
	sensor_write_reg(client, 0x13, 0xe5);
	sensor_write_reg(client, 0x39, 0x43);// ;50 for 30fps
	sensor_write_reg(client, 0x38, 0x12);// ;92 for 30fps
	sensor_write_reg(client, 0x37, 0x00);
	sensor_write_reg(client, 0x35, 0x91);// ;81 for 30fps
	sensor_write_reg(client, 0x0e, 0x20);
//	sensor_write_reg(client, 0x1e, 0x24);
	sensor_write_reg(client, 0x1e, 0x04);

	sensor_write_reg(client, 0xA8, 0x80);
	sensor_write_reg(client, 0x12, 0x40);
#ifndef CONFIG_JZ_CIM_IN_FMT_ITU656
	sensor_write_reg(client, 0x04, 0x00);
	sensor_write_reg(client, 0x40, 0xc1);
#else
	sensor_write_reg(client, 0x04, 0x40);
	sensor_write_reg(client, 0x40, 0x80);	//output range: 0x01~0xFE
#endif
	sensor_write_reg(client, 0x0c, 0x04);
	sensor_write_reg(client, 0x0d, 0x80);
	sensor_write_reg(client, 0x18, 0xc5);//;c6
	sensor_write_reg(client, 0x17, 0x25);//;26
	sensor_write_reg(client, 0x32, 0xad);
	sensor_write_reg(client, 0x03, 0x00);
	sensor_write_reg(client, 0x1a, 0x3d);
	sensor_write_reg(client, 0x19, 0x01);
	sensor_write_reg(client, 0x3f, 0xa6);
	sensor_write_reg(client, 0x14, 0x1a);
	sensor_write_reg(client, 0x15, 0x02);
	sensor_write_reg(client, 0x41, 0x02);
	sensor_write_reg(client, 0x42, 0x08);

	sensor_write_reg(client, 0x1b, 0x00);
	sensor_write_reg(client, 0x16, 0x06);
	sensor_write_reg(client, 0x33, 0xc0);//;e2 ;c0 for internal regulator
	sensor_write_reg(client, 0x34, 0xbf);
	sensor_write_reg(client, 0x96, 0x04);
	sensor_write_reg(client, 0x3a, 0x00);
	sensor_write_reg(client, 0x8e, 0x00);

	sensor_write_reg(client, 0x3c, 0x77);
	sensor_write_reg(client, 0x8B, 0x06);
	sensor_write_reg(client, 0x94, 0x88);
	sensor_write_reg(client, 0x95, 0x88);
	sensor_write_reg(client, 0x29, 0x2f);//;3f ;2f for internal regulator
	sensor_write_reg(client, 0x0f, 0x42);

	sensor_write_reg(client, 0x3d, 0x92);
	sensor_write_reg(client, 0x69, 0x40);
	sensor_write_reg(client, 0x5C, 0xb9);
	sensor_write_reg(client, 0x5D, 0x96);
	sensor_write_reg(client, 0x5E, 0x10);
	sensor_write_reg(client, 0x59, 0xc0);
	sensor_write_reg(client, 0x5A, 0xaf);
	sensor_write_reg(client, 0x5B, 0x55);
	sensor_write_reg(client, 0x43, 0xf0);
	sensor_write_reg(client, 0x44, 0x10);
	sensor_write_reg(client, 0x45, 0x68);
	sensor_write_reg(client, 0x46, 0x96);
	sensor_write_reg(client, 0x47, 0x60);
	sensor_write_reg(client, 0x48, 0x80);
	sensor_write_reg(client, 0x5F, 0xe0);
	sensor_write_reg(client, 0x60, 0x8c);// ;0c for advanced AWB (related to lens)
	sensor_write_reg(client, 0x61, 0x20);
	sensor_write_reg(client, 0xa5, 0xd9);
	sensor_write_reg(client, 0xa4, 0x74);
#if defined(CONFIG_JZ4780_F4780)
	sensor_write_reg(client, 0x8d, 0x12);	//color bar test mode
#else
	sensor_write_reg(client, 0x8d, 0x02);
#endif
	sensor_write_reg(client, 0x13, 0xe7);

	sensor_write_reg(client, 0x4f, 0x3a);
	sensor_write_reg(client, 0x50, 0x3d);
	sensor_write_reg(client, 0x51, 0x03);
	sensor_write_reg(client, 0x52, 0x12);
	sensor_write_reg(client, 0x53, 0x26);
	sensor_write_reg(client, 0x54, 0x38);
	sensor_write_reg(client, 0x55, 0x40);
	sensor_write_reg(client, 0x56, 0x40);
	sensor_write_reg(client, 0x57, 0x40);
	sensor_write_reg(client, 0x58, 0x0d);

	sensor_write_reg(client, 0x8C, 0x23);
	sensor_write_reg(client, 0x3E, 0x02);
	sensor_write_reg(client, 0xa9, 0xb8);
	sensor_write_reg(client, 0xaa, 0x92);
	sensor_write_reg(client, 0xab, 0x0a);

	sensor_write_reg(client, 0x8f, 0xdf);
	sensor_write_reg(client, 0x90, 0x00);
	sensor_write_reg(client, 0x91, 0x00);
	sensor_write_reg(client, 0x9f, 0x00);
	sensor_write_reg(client, 0xa0, 0x00);
	sensor_write_reg(client, 0x3A, 0x01);
	sensor_write_reg(client, 0x24, 0x70);//;80 for CSTN
	sensor_write_reg(client, 0x25, 0x64);//;74 for CSTN
	sensor_write_reg(client, 0x26, 0xc3);//;d4 for CSTN
//;gamma
	sensor_write_reg(client, 0x6c, 0x40);
	sensor_write_reg(client, 0x6d, 0x30);
	sensor_write_reg(client, 0x6e, 0x4b);
	sensor_write_reg(client, 0x6f, 0x60);
	sensor_write_reg(client, 0x70, 0x70);
	sensor_write_reg(client, 0x71, 0x70);
	sensor_write_reg(client, 0x72, 0x70);
	sensor_write_reg(client, 0x73, 0x70);
	sensor_write_reg(client, 0x74, 0x60);
	sensor_write_reg(client, 0x75, 0x60);
	sensor_write_reg(client, 0x76, 0x50);
	sensor_write_reg(client, 0x77, 0x48);
	sensor_write_reg(client, 0x78, 0x3a);
	sensor_write_reg(client, 0x79, 0x2e);
	sensor_write_reg(client, 0x7a, 0x28);
	sensor_write_reg(client, 0x7b, 0x22);
	sensor_write_reg(client, 0x7c, 0x04);
	sensor_write_reg(client, 0x7d, 0x07);
	sensor_write_reg(client, 0x7e, 0x10);
	sensor_write_reg(client, 0x7f, 0x28);
	sensor_write_reg(client, 0x80, 0x36);
	sensor_write_reg(client, 0x81, 0x44);
	sensor_write_reg(client, 0x82, 0x52);
	sensor_write_reg(client, 0x83, 0x60);
	sensor_write_reg(client, 0x84, 0x6c);
	sensor_write_reg(client, 0x85, 0x78);
	sensor_write_reg(client, 0x86, 0x8c);
	sensor_write_reg(client, 0x87, 0x9e);
	sensor_write_reg(client, 0x88, 0xbb);
	sensor_write_reg(client, 0x89, 0xd2);
	sensor_write_reg(client, 0x8a, 0xe6);

	sensor_write_reg(client, 0x11, 0x81);//;for 24M clock
//	sensor_write_reg(client, 0x11, 0x80);//;for 24M clock
	sensor_write_reg(client, 0x2a, 0x10);//;for 24M clock
	sensor_write_reg(client, 0x2b, 0x40);//;for 24M clock

//;	sensor_write_reg(client, 0x11, 0x81);;for 26M clock
//;	sensor_write_reg(client, 0x2a, 0x10);;for 26M clock
//;	sensor_write_reg(client, 0x2b, 0xe0);;for 26M clock

//;	sensor_write_reg(client, 0x11, 0x81);;for 32M clock
//;	sensor_write_reg(client, 0x2a, 0x30);;for 32M clock
//;	sensor_write_reg(client, 0x2b, 0xc0);;for 32M clock

//;	sensor_write_reg(client, 0x66, 0x01);
//;	sensor_write_reg(client, 0x64, 0x08);
//;	sensor_write_reg(client, 0x65, 0x50);
	sensor_write_reg(client, 0x62, 0x00);
	sensor_write_reg(client, 0x63, 0x00);
	sensor_write_reg(client, 0x64, 0x06);
	sensor_write_reg(client, 0x65, 0x40);
	sensor_write_reg(client, 0x9d, 0x06);
	sensor_write_reg(client, 0x9e, 0x08);
	sensor_write_reg(client, 0x66, 0x05);

  /*vga-sxga*/
/*
	sensor_write_reg(client, 0x11, 0x80);
	sensor_write_reg(client, 0x12, 0x00);
	sensor_write_reg(client, 0x0C, 0x00);
	sensor_write_reg(client, 0x0D, 0x00);
	sensor_write_reg(client, 0x18, 0xbe);;bd
	sensor_write_reg(client, 0x17, 0x1e);;1d
	sensor_write_reg(client, 0x32, 0xbf);
	sensor_write_reg(client, 0x03, 0x12);
	sensor_write_reg(client, 0x1a, 0x81);
	sensor_write_reg(client, 0x19, 0x01);

	sensor_write_reg(client, 0x2a, 0x10);;24M clock
	sensor_write_reg(client, 0x2b, 0x34);;24M clock
	sensor_write_reg(client, 0x6a, 0x41);;24M clock

;	sensor_write_reg(client, 0x2a, 0x10);;26M clock
;	sensor_write_reg(client, 0x2b, 0xcc);;26M clock
;	sensor_write_reg(client, 0x6a, 0x41);;26M clock
*/

}
static void ov9650_sxga(struct i2c_client *client)
{
	sensor_write_reg(client, 0x12, 0x80);
	sensor_write_reg(client, 0x11, 0x81);
//	sensor_write_reg(client, 0x11, 0x80);
	sensor_write_reg(client, 0x6b, 0x0a);
	sensor_write_reg(client, 0x6a, 0x3e);
	sensor_write_reg(client, 0x3b, 0x09);
	sensor_write_reg(client, 0x13, 0xe0);
	sensor_write_reg(client, 0x01, 0x80);
	sensor_write_reg(client, 0x02, 0x80);
	sensor_write_reg(client, 0x00, 0x00);
	sensor_write_reg(client, 0x10, 0x00);
	sensor_write_reg(client, 0x13, 0xe5);

	sensor_write_reg(client, 0x39, 0x43);// ;50 for 30fps
	sensor_write_reg(client, 0x38, 0x12);// ;92 for 30fps
	sensor_write_reg(client, 0x37, 0x00);
	sensor_write_reg(client, 0x35, 0x91);// ;81 for 30fps
	sensor_write_reg(client, 0x0e, 0x20);
//	sensor_write_reg(client, 0x1e, 0x24); //mirror
	sensor_write_reg(client, 0x1e, 0x04);

	sensor_write_reg(client, 0xA8, 0x80);
	sensor_write_reg(client, 0x12, 0x40);
#ifndef CONFIG_JZ_CIM_IN_FMT_ITU656
	sensor_write_reg(client, 0x04, 0x00);
	sensor_write_reg(client, 0x40, 0xc1);
#else
	sensor_write_reg(client, 0x04, 0x40);
	sensor_write_reg(client, 0x40, 0x80);
#endif
	sensor_write_reg(client, 0x0c, 0x04);
	sensor_write_reg(client, 0x0d, 0x80);
	sensor_write_reg(client, 0x18, 0xc5);//;c6
	sensor_write_reg(client, 0x17, 0x25);//;26
	sensor_write_reg(client, 0x32, 0xad);
	sensor_write_reg(client, 0x03, 0x00);
	sensor_write_reg(client, 0x1a, 0x3d);
	sensor_write_reg(client, 0x19, 0x01);
	sensor_write_reg(client, 0x3f, 0xa6);
	sensor_write_reg(client, 0x14, 0x1a);
	sensor_write_reg(client, 0x15, 0x02);
	sensor_write_reg(client, 0x41, 0x02);
	sensor_write_reg(client, 0x42, 0x08);

	sensor_write_reg(client, 0x1b, 0x00);
	sensor_write_reg(client, 0x16, 0x06);
	sensor_write_reg(client, 0x33, 0xc0);//;e2 ;c0 for internal regulator
	sensor_write_reg(client, 0x34, 0xbf);
	sensor_write_reg(client, 0x96, 0x04);
	sensor_write_reg(client, 0x3a, 0x00);
	sensor_write_reg(client, 0x8e, 0x00);

	sensor_write_reg(client, 0x3c, 0x77);
	sensor_write_reg(client, 0x8B, 0x06);
	sensor_write_reg(client, 0x94, 0x88);
	sensor_write_reg(client, 0x95, 0x88);
	sensor_write_reg(client, 0x29, 0x2f);//;3f ;2f for internal regulator
	sensor_write_reg(client, 0x0f, 0x42);

	sensor_write_reg(client, 0x3d, 0x92);
	sensor_write_reg(client, 0x69, 0x40);
	sensor_write_reg(client, 0x5C, 0xb9);
	sensor_write_reg(client, 0x5D, 0x96);
	sensor_write_reg(client, 0x5E, 0x10);
	sensor_write_reg(client, 0x59, 0xc0);
	sensor_write_reg(client, 0x5A, 0xaf);
	sensor_write_reg(client, 0x5B, 0x55);
	sensor_write_reg(client, 0x43, 0xf0);
	sensor_write_reg(client, 0x44, 0x10);
	sensor_write_reg(client, 0x45, 0x68);
	sensor_write_reg(client, 0x46, 0x96);
	sensor_write_reg(client, 0x47, 0x60);
	sensor_write_reg(client, 0x48, 0x80);
	sensor_write_reg(client, 0x5F, 0xe0);
	sensor_write_reg(client, 0x60, 0x8c);// ;0c for advanced AWB (related to lens)
	sensor_write_reg(client, 0x61, 0x20);
	sensor_write_reg(client, 0xa5, 0xd9);
	sensor_write_reg(client, 0xa4, 0x74);
#if defined(CONFIG_JZ4780_F4780)
	sensor_write_reg(client, 0x8d, 0x12);	//color bar test mode
#else
	sensor_write_reg(client, 0x8d, 0x02);
#endif
	sensor_write_reg(client, 0x13, 0xe7);

	sensor_write_reg(client, 0x4f, 0x3a);
	sensor_write_reg(client, 0x50, 0x3d);
	sensor_write_reg(client, 0x51, 0x03);
	sensor_write_reg(client, 0x52, 0x12);
	sensor_write_reg(client, 0x53, 0x26);
	sensor_write_reg(client, 0x54, 0x38);
	sensor_write_reg(client, 0x55, 0x40);
	sensor_write_reg(client, 0x56, 0x40);
	sensor_write_reg(client, 0x57, 0x40);
	sensor_write_reg(client, 0x58, 0x0d);

	sensor_write_reg(client, 0x8C, 0x23);
	sensor_write_reg(client, 0x3E, 0x02);
	sensor_write_reg(client, 0xa9, 0xb8);
	sensor_write_reg(client, 0xaa, 0x92);
	sensor_write_reg(client, 0xab, 0x0a);

	sensor_write_reg(client, 0x8f, 0xdf);
	sensor_write_reg(client, 0x90, 0x00);
	sensor_write_reg(client, 0x91, 0x00);
	sensor_write_reg(client, 0x9f, 0x00);
	sensor_write_reg(client, 0xa0, 0x00);
	sensor_write_reg(client, 0x3A, 0x01);
	sensor_write_reg(client, 0x24, 0x70);//;80 for CSTN
	sensor_write_reg(client, 0x25, 0x64);//;74 for CSTN
	sensor_write_reg(client, 0x26, 0xc3);//;d4 for CSTN
//;gamma
	sensor_write_reg(client, 0x6c, 0x40);
	sensor_write_reg(client, 0x6d, 0x30);
	sensor_write_reg(client, 0x6e, 0x4b);
	sensor_write_reg(client, 0x6f, 0x60);
	sensor_write_reg(client, 0x70, 0x70);
	sensor_write_reg(client, 0x71, 0x70);
	sensor_write_reg(client, 0x72, 0x70);
	sensor_write_reg(client, 0x73, 0x70);
	sensor_write_reg(client, 0x74, 0x60);
	sensor_write_reg(client, 0x75, 0x60);
	sensor_write_reg(client, 0x76, 0x50);
	sensor_write_reg(client, 0x77, 0x48);
	sensor_write_reg(client, 0x78, 0x3a);
	sensor_write_reg(client, 0x79, 0x2e);
	sensor_write_reg(client, 0x7a, 0x28);
	sensor_write_reg(client, 0x7b, 0x22);
	sensor_write_reg(client, 0x7c, 0x04);
	sensor_write_reg(client, 0x7d, 0x07);
	sensor_write_reg(client, 0x7e, 0x10);
	sensor_write_reg(client, 0x7f, 0x28);
	sensor_write_reg(client, 0x80, 0x36);
	sensor_write_reg(client, 0x81, 0x44);
	sensor_write_reg(client, 0x82, 0x52);
	sensor_write_reg(client, 0x83, 0x60);
	sensor_write_reg(client, 0x84, 0x6c);
	sensor_write_reg(client, 0x85, 0x78);
	sensor_write_reg(client, 0x86, 0x8c);
	sensor_write_reg(client, 0x87, 0x9e);
	sensor_write_reg(client, 0x88, 0xbb);
	sensor_write_reg(client, 0x89, 0xd2);
	sensor_write_reg(client, 0x8a, 0xe6);

	sensor_write_reg(client, 0x11, 0x81);
//	sensor_write_reg(client, 0x11, 0x80);//;for 24M clock
	sensor_write_reg(client, 0x2a, 0x10);//;for 24M clock
	sensor_write_reg(client, 0x2b, 0x40);//;for 24M clock

//;	sensor_write_reg(client, 0x11, 0x81);;for 26M clock
//;	sensor_write_reg(client, 0x2a, 0x10);;for 26M clock
//;	sensor_write_reg(client, 0x2b, 0xe0);;for 26M clock

//;	sensor_write_reg(client, 0x11, 0x81);;for 32M clock
//;	sensor_write_reg(client, 0x2a, 0x30);;for 32M clock
//;	sensor_write_reg(client, 0x2b, 0xc0);;for 32M clock

//;	sensor_write_reg(client, 0x66, 0x01);
//;	sensor_write_reg(client, 0x64, 0x08);
//;	sensor_write_reg(client, 0x65, 0x50);
	sensor_write_reg(client, 0x62, 0x00);
	sensor_write_reg(client, 0x63, 0x00);
	sensor_write_reg(client, 0x64, 0x06);
	sensor_write_reg(client, 0x65, 0x40);
	sensor_write_reg(client, 0x9d, 0x06);
	sensor_write_reg(client, 0x9e, 0x08);
	sensor_write_reg(client, 0x66, 0x05);

  /*vga-sxga*/

//	sensor_write_reg(client, 0x11, 0x80);
	sensor_write_reg(client, 0x11, 0x81);
	sensor_write_reg(client, 0x12, 0x00);
	sensor_write_reg(client, 0x0C, 0x00);
	sensor_write_reg(client, 0x0D, 0x00);
	sensor_write_reg(client, 0x18, 0xbe);//;bd
	sensor_write_reg(client, 0x17, 0x1e);//;1d
	sensor_write_reg(client, 0x32, 0xbf);
	sensor_write_reg(client, 0x03, 0x12);
	sensor_write_reg(client, 0x1a, 0x81);
	sensor_write_reg(client, 0x19, 0x01);

	sensor_write_reg(client, 0x2a, 0x10);//;24M clock
	sensor_write_reg(client, 0x2b, 0x34);//;24M clock
	sensor_write_reg(client, 0x6a, 0x41);//;24M clock

//;	sensor_write_reg(client, 0x2a, 0x10);;26M clock
//;	sensor_write_reg(client, 0x2b, 0xcc);;26M clock
//;	sensor_write_reg(client, 0x6a, 0x41);;26M clock

}

/*
;09032004
;OV9650
;QVGA YUV
;30fps when 24MHz input clock
;Device Address(Hex)/Register(Hex)/Value(Hex)
 */
static void ov9650_qvga(struct i2c_client *client)
{
	sensor_write_reg(client, 0x12, 0x80);
	sensor_write_reg(client, 0x11, 0x81);
//	sensor_write_reg(client, 0x11, 0x80);
	sensor_write_reg(client, 0x6b, 0x0a);
	sensor_write_reg(client, 0x6a, 0x1f);
	sensor_write_reg(client, 0x3b, 0x09);
//	sensor_write_reg(client, 0x3b, 0x00);
	sensor_write_reg(client, 0x13, 0xe0);
	sensor_write_reg(client, 0x01, 0x80);
	sensor_write_reg(client, 0x02, 0x80);
	sensor_write_reg(client, 0x00, 0x00);
	sensor_write_reg(client, 0x10, 0x00);
	sensor_write_reg(client, 0x13, 0xe5);

	sensor_write_reg(client, 0x39, 0x43);
	sensor_write_reg(client, 0x38, 0x12);
	sensor_write_reg(client, 0x37, 0x00);
	sensor_write_reg(client, 0x35, 0x91);
	sensor_write_reg(client, 0x0e, 0xa0);
	sensor_write_reg(client, 0x1e, 0x04);

	sensor_write_reg(client, 0xA8, 0x80);
	sensor_write_reg(client, 0x12, 0x10);
//#ifndef CONFIG_JZ_CIM_IN_FMT_ITU656
//	sensor_write_reg(client, 0x04, 0x00);
//	sensor_write_reg(client, 0x40, 0xc1);
//#else
	sensor_write_reg(client, 0x04, 0x40);	//CCIR656
	sensor_write_reg(client, 0x40, 0x80);	//output range: 0x01~0xFE
//#endif
	sensor_write_reg(client, 0x0c, 0x04);
	sensor_write_reg(client, 0x0d, 0x80);
	sensor_write_reg(client, 0x18, 0xc6);
	sensor_write_reg(client, 0x17, 0x26);
	sensor_write_reg(client, 0x32, 0xa4);
	sensor_write_reg(client, 0x03, 0x36);
	sensor_write_reg(client, 0x1a, 0x1e);
	sensor_write_reg(client, 0x19, 0x00);
	sensor_write_reg(client, 0x3f, 0xa6);
	sensor_write_reg(client, 0x14, 0x2e);
	sensor_write_reg(client, 0x15, 0x02);
	sensor_write_reg(client, 0x41, 0x02);
	sensor_write_reg(client, 0x42, 0x08);

	sensor_write_reg(client, 0x1b, 0x00);
	sensor_write_reg(client, 0x16, 0x06);
	sensor_write_reg(client, 0x33, 0xe2);//c0 for internal regulator
	sensor_write_reg(client, 0x34, 0xbf);
	sensor_write_reg(client, 0x96, 0x04);
	sensor_write_reg(client, 0x3a, 0x00);
	sensor_write_reg(client, 0x8e, 0x00);

	sensor_write_reg(client, 0x3c, 0x77);
	sensor_write_reg(client, 0x8B, 0x06);
	sensor_write_reg(client, 0x94, 0x88);
	sensor_write_reg(client, 0x95, 0x88);
	sensor_write_reg(client, 0x29, 0x3f);// ;2f for internal regulator
	sensor_write_reg(client, 0x0f, 0x42);

	sensor_write_reg(client, 0x3d, 0x92);
	sensor_write_reg(client, 0x69, 0x40);
	sensor_write_reg(client, 0x5C, 0xb9);
	sensor_write_reg(client, 0x5D, 0x96);
	sensor_write_reg(client, 0x5E, 0x10);
	sensor_write_reg(client, 0x59, 0xc0);
	sensor_write_reg(client, 0x5A, 0xaf);
	sensor_write_reg(client, 0x5B, 0x55);
	sensor_write_reg(client, 0x43, 0xf0);
	sensor_write_reg(client, 0x44, 0x10);
	sensor_write_reg(client, 0x45, 0x68);
	sensor_write_reg(client, 0x46, 0x96);
	sensor_write_reg(client, 0x47, 0x60);
	sensor_write_reg(client, 0x48, 0x80);
	sensor_write_reg(client, 0x5F, 0xe0);
	sensor_write_reg(client, 0x80, 0x8c);// ;0c for advanced AWB (related to lens)
	sensor_write_reg(client, 0x61, 0x20);
	sensor_write_reg(client, 0xa5, 0xd9);
	sensor_write_reg(client, 0xa4, 0x74);
#if defined(CONFIG_JZ4780_F4780)
	sensor_write_reg(client, 0x8d, 0x12);	//color bar test mode
#else
	sensor_write_reg(client, 0x8d, 0x02);
#endif
	sensor_write_reg(client, 0x13, 0xe7);

	sensor_write_reg(client, 0x4f, 0x3a);
	sensor_write_reg(client, 0x50, 0x3d);
	sensor_write_reg(client, 0x51, 0x03);
	sensor_write_reg(client, 0x52, 0x12);
	sensor_write_reg(client, 0x53, 0x26);
	sensor_write_reg(client, 0x54, 0x38);
	sensor_write_reg(client, 0x55, 0x40);
	sensor_write_reg(client, 0x56, 0x40);
	sensor_write_reg(client, 0x57, 0x40);
	sensor_write_reg(client, 0x58, 0x0d);

	sensor_write_reg(client, 0x8C, 0x23);
	sensor_write_reg(client, 0x3E, 0x02);
	sensor_write_reg(client, 0xa9, 0xb8);
	sensor_write_reg(client, 0xaa, 0x92);
	sensor_write_reg(client, 0xab, 0x0a);

	sensor_write_reg(client, 0x8f, 0xdf);
	sensor_write_reg(client, 0x90, 0x00);
	sensor_write_reg(client, 0x91, 0x00);
	sensor_write_reg(client, 0x9f, 0x00);
	sensor_write_reg(client, 0xa0, 0x00);
	sensor_write_reg(client, 0x3A, 0x01); //yuyv
//	sensor_write_reg(client, 0x3A, 0x05); //yvyu
//	sensor_write_reg(client, 0x3A, 0x0d); //uyvy
//	sensor_write_reg(client, 0x3A, 0x09); //vyuy

	sensor_write_reg(client, 0x24, 0x70);
	sensor_write_reg(client, 0x25, 0x64);
	sensor_write_reg(client, 0x26, 0xc3);
	sensor_write_reg(client, 0x2a, 0x00); //;10 for 50Hz
	sensor_write_reg(client, 0x2b, 0x00);// ;40 for 50Hz
//;gamma
	sensor_write_reg(client, 0x6c, 0x40);
	sensor_write_reg(client, 0x6d, 0x30);
	sensor_write_reg(client, 0x6e, 0x4b);
	sensor_write_reg(client, 0x6f, 0x60);
	sensor_write_reg(client, 0x70, 0x70);
	sensor_write_reg(client, 0x71, 0x70);
	sensor_write_reg(client, 0x72, 0x70);
	sensor_write_reg(client, 0x73, 0x70);
	sensor_write_reg(client, 0x74, 0x60);
	sensor_write_reg(client, 0x75, 0x60);
	sensor_write_reg(client, 0x76, 0x50);
	sensor_write_reg(client, 0x77, 0x48);
	sensor_write_reg(client, 0x78, 0x3a);
	sensor_write_reg(client, 0x79, 0x2e);
	sensor_write_reg(client, 0x7a, 0x28);
	sensor_write_reg(client, 0x7b, 0x22);
	sensor_write_reg(client, 0x7c, 0x04);
	sensor_write_reg(client, 0x7d, 0x07);
	sensor_write_reg(client, 0x7e, 0x10);
	sensor_write_reg(client, 0x7f, 0x28);
	sensor_write_reg(client, 0x80, 0x36);
	sensor_write_reg(client, 0x81, 0x44);
	sensor_write_reg(client, 0x82, 0x52);
	sensor_write_reg(client, 0x83, 0x60);
	sensor_write_reg(client, 0x84, 0x6c);
	sensor_write_reg(client, 0x85, 0x78);
	sensor_write_reg(client, 0x86, 0x8c);
	sensor_write_reg(client, 0x87, 0x9e);
	sensor_write_reg(client, 0x88, 0xbb);
	sensor_write_reg(client, 0x89, 0xd2);
	sensor_write_reg(client, 0x8a, 0xe6);
}

void preview_set(struct i2c_client *client)
{
	ov9650_qvga(client);	//320x240
}

void capture_set(struct i2c_client *client)
{
	ov9650_qvga(client);	//320x240
}

void init_set(struct i2c_client *client)
{}

