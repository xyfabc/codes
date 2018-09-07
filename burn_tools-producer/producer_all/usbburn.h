#ifndef __USB_BURN__
#define __USB_BURN__

#define 	MAX_PATH					260

#define     BT_FAIL						0
#define     BT_SUCCESS					1
#define     USB_STATUS_SUCCESS  BT_SUCCESS
#define     USB_STATUS_FAILE    BT_FAIL

#define    TRANS_NULL  						0
#define    TRANS_SWITCH_USB					1		//切换USB速度为High Speed
#define    TRANS_TEST_CONNECT				2		//测试USb通讯
#define    TRANS_SET_MODE					3 		//设置烧录模式，完整烧录或是升级或是SPI烧录
#define    TRANS_GET_FLASHID				4 		//获取nandflash或spi flash id
#define    TRANS_SET_NANDPARAM				5 		//设置nandflash的参数
#define    TRANS_DETECT_NANDPARAM			6 		//测nandflash参数，对于不在nand列表里的flash使用
#define    TRANS_INIT_SECAREA				7		//初始化安全区
#define    TRANS_SET_RESV					8		//设置保留区大小
#define    TRANS_CREATE_PARTITION			9		//创建分区
#define    TRANS_FORMAT_DRIVER				10		//格式化分区
#define    TRANS_MOUNT_DRIVER				11		//挂载分区
#define    TRANS_DOWNLOAD_BOOT_START		12		//开始下载boot
#define    TRANS_DOWNLOAD_BOOT_DATA			13		//发送boot数据
#define    TRANS_COMPARE_BOOT_START		    14		//开始比较boot
#define    TRANS_COMPARE_BOOT_DATA			15		//发送比较boot数据
#define    TRANS_DOWNLOAD_BIN_START			16		//开始下载bin文件
#define    TRANS_DOWNLOAD_BIN_DATA			17		//发送bin文件数据
#define    TRANS_COMPARE_BIN_START			18		//开始比较bin文件
#define    TRANS_COMPARE_BIN_DATA			19		//发送比较bin文件数据
#define    TRANS_DOWNLOAD_IMG_START			20		//开始下载IMAGE文件
#define    TRANS_DOWNLOAD_IMG_DATA			21		//发送IMAGE数据
#define    TRANS_COMPARE_IMG_START			22		//开始比较IMAGE文件
#define    TRANS_COMPARE_IMG_DATA			23		//发送比较IMAGE数据
#define    TRANS_DOWNLOAD_FILE_START		24		//开始下载文件系统文件
#define    TRANS_DOWNLOAD_FILE_DATA			25		//发送文件系统文件数据
#define    TRANS_COMPARE_FILE_START		    26		//开始比较文件系统文件
#define    TRANS_COMPARE_FILE_DATA			27		//发送比较文件系统文件数据
#define    TRANS_UPLOAD_BIN_START			28	    //开始上传BIN文件
#define    TRANS_UPLOAD_BIN_DATA			29	    //上传BIN文件
#define    TRANS_UPLOAD_FILE_START			30      //开始上传文件系统文件
#define    TRANS_UPLOAD_FILE_DATA			31		//上传文件系统文件数据
#define    TRANS_SET_GPIO					32 		//设置GPIO
#define    TRANS_RESET						33		//重启设备端
#define    TRANS_CLOSE						34		//Close
#define    TRANS_SET_REG					35
#define    TRANS_DOWNLOAD_PRODUCER_START	36
#define    TRANS_DOWNLOAD_PRODUCER_DATA		37
#define    TRANS_RUN_PRODUCER				38
#define    TRANS_GET_DISK_INFO		        39		//获取分区信息
#define    TRANS_UPDATESELF_BIN_START		40		//开始下载自升级数据
#define    TRANS_UPDATESELF_BIN_DATA		41		//传自升级数据
#define    TRANS_UPLOAD_BIN_LEN				43	    //上传BIN长度
#define	   TRANS_WRITE_ASA_FILE				44	    //写安全区文件

#define    TRANS_DOWNLOAD_CLIENT_BOOT_START			45		//开始下载客户BOOT文件
#define    TRANS_DOWNLOAD_CLIENT_BOOT_DATA			46		//发送客户BOOT文件数据
#define    TRANS_COMPARE_CLIENT_BOOT_START			47		//开始比较客户BOOT文件
#define    TRANS_COMPARE_CLIENT_BOOT_DATA			48		//发送比较
#define    TRANS_SET_SPIPARAM					49
#define    TRANS_UPLOAD_SPIDATA_START			    53	    //开始上传SPIDATA
#define    TRANS_UPLOAD_SPIDATA_DATA			    54	    //开始上传SPIDATA

#define	   TRANS_GET_CHANNEL_ID						100
#define	   TRANS_GET_SCSI_STATUS 					102	    //
#define    TRANS_SET_BURNEDPARAM                    104     //
#define		 TRANS_SET_NANDFLASH_CHIP_SEL             105
#define    TRANS_READ_ASA_FILE				106

#define	   TRANS_SET_ERASE_NAND_MODE			    111	    //
#define	   TRANS_SET_BIN_RESV_SIZE			        112	    //

#define	   TRANS_UPLOAD_BOOT_START				    107	    //
#define	   TRANS_UPLOAD_BOOT_DATA				    108	    //
#define	   TRANS_UPLOAD_BOOT_LEN				    109	    //

#define    ANYKA_UFI_CONFIRM_ID 			0x01
#define	   ANYKA_DIR_TYPE					0x4000000
#define    ANYKA_MAX_PATH					255

typedef struct
{
	unsigned long chip_ID;
	unsigned long chip_cnt;
}T_CHIP_INFO;

typedef struct
{
	T_U32 file_length;
	T_U32 file_mode;
	T_U32 file_time;
    T_U8 bCheck;
	T_U8 resv[3];
    T_U8 apath[MAX_PATH+1];
}T_UDISK_FILE_INFO;

typedef struct tag_ImgInfo
{
    T_U32   data_length;
    T_U8    DriverName;
	T_U8    bCheck;
	T_U8	resv[2];
}T_IMG_INFO;


typedef struct DownLoadControl
{
	unsigned long file_total_len;
	unsigned long data_total;
}T_DL_CONTROL;

typedef struct
{
    T_U32	num;
    T_U8	dir;
    T_U8	level;
	T_U8	Pullup;
	T_U8	Pulldown;
}__attribute__((packed)) T_GPIO_PARAM;

typedef struct  
{
    T_U32  burned_mode;
    T_U32  bGpio;
    T_GPIO_PARAM  pwr_gpio_param;
    T_U32  bWakup;
    T_U32  rtc_wakup_level;
}T_BURNED_PARAM;

#define BURNED_NONE      1
#define BURNED_RESET     2
#define BURNED_POWER_OFF 3

typedef struct
{
        T_U8 chip_sel_num;                                      //Ƭѡ˽
        T_U8 gpio_chip_2;                                       //Ƭѡ2 ce
        T_U8 gpio_chip_3;                                       //Ƭѡ3 ce
}__attribute__((packed)) T_CHIP_SELECT_DATA;

#endif
