/*
 * @FILENAME transc.c
 * @BRIEF handle all burn transc
 * Copyright (C) 2010 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR Zhang Jingyuan
 * @DATE 2010-10-10
 * @version 1.0
 */

#include "fha.h"
#include "fha_asa.h"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <errno.h>
#include <mntent.h>

#include "usbburn.h"
#include "file.h"
#if (defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
#include "spi_char_interface.h"
#else
#include "nand_char_interface.h"
#endif
#include "burnimage.h"

#define AK_USBBURN_STALL		_IO('U', 0xf0)
#define AK_USBBURN_STATUS		_IO('U', 0xf1)
#define AK_GET_CHANNEL_ID		_IOR('U', 0xf2, unsigned long)
#define MAX_PARTS_COUNT			26
#define LED_SYS_PATH "/sys/devices/platform/ak98_producer_led/"
#define POWEROFF_GPIO_SYS "/sys/devices/platform/ak98_poweroff/"
#define BEGIN_DELAY 5
#define DELAY 3

#define MAXLINE		64
#define DEV_PATH	"/dev/"
#define DEV_NAME	"mtdblock"
#define MTD_BLOCK_MAJOR	31

#define SPI_FLASH_BIN_PAGE_START 	(558)

typedef struct
{
    unsigned int  chip_id;
    unsigned int    total_size;             ///< flash total size in bytes
    unsigned int  page_size;       ///< total bytes per page
    unsigned int  program_size;    ///< program size at 02h command
    unsigned int  erase_size;      ///< erase size at d8h command 
    unsigned int  clock;           ///< spi clock, 0 means use default clock 
    
    //chip character bits:
    //bit 0: under_protect flag, the serial flash under protection or not when power on
    //bit 1: fast read flag    
    unsigned char  flag;            ///< chip character bits
    unsigned char  protect_mask;    ///protect mask bits in status register:BIT2:BP0, 
                             //BIT3:BP1, BIT4:BP2, BIT5:BP3, BIT7:BPL
    unsigned char    reserved1;    //used for partition align K
    unsigned char    reserved2;
    char   des_str[32];      //描述符                                    
}T_SFLASH_PHY_INFO;
static unsigned int align_size = 64 * 1024;
static unsigned char buf[85536];
static unsigned char *mounted = NULL;
static unsigned long *ori_mbyte = NULL;
unsigned char *userpart = NULL;
static unsigned char mtd_mount = 0;
static T_BURNED_PARAM burned_param;
static T_PARTION_INFO *pc_parts = NULL;

const static unsigned char anyka_confirm_resp[16] = {
	0x41, 0x4E, 0x59, 0x4B, 0x41, 0x20, 0x44, 0x45, 
	0x53, 0x49, 0x47, 0x4e, 0x45
};

static T_FHA_BIN_PARAM bin_param;
static T_FHA_LIB_CALLBACK callback;
T_FHA_INIT_INFO init_info;

T_SFLASH_PHY_INFO spi_phy_info;
#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
T_SPI_INIT_INFO spi_info;
#endif

static T_IMG_INFO imginfo;

static int fd;	/* the usb gadget buffer deivce fd */

static int cmd_ret = 0;
static unsigned long detect = 0;
static pthread_t ntid, alrm_ntid;
static int led_valid = 0;

static T_DL_CONTROL m_dl_control;
static T_GPIO_PARAM gpio_param;
static unsigned char *part_info = NULL;
static T_CHIP_SELECT_DATA nand_ce_data;
unsigned long chip_page_num;
static int kernel_update_flag = 1;

static void *getpartion(void *data);
static T_BOOL Transc_GetPartion2();
static void umount_all(void);

T_U8 bufbin[64 * 1024];
/*
 * @BREIF    transc of test connection
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @PARAM    [out] handle result
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_SwitchUsb(unsigned long data[], unsigned long len)
{
	ioctl(fd, AK_USBBURN_STATUS, 0);
	system("rmmod g_file_storage");
	system("rmmod ak98_udc_full");
	system("insmod /root/ak98_udc.ko");
	system("insmod /root/g_file_storage.ko file=/dev/ram0 removable=1 stall=0 product=0x038e vendor=0x04d6");

	return AK_TRUE;
}

/*
 * @BREIF    transc of test connection
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @PARAM    [out] handle result
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_TestConnection(unsigned long data[], unsigned long len)
{
	unsigned long *pData = data;

	if('B' == pData[0] && 'T' == pData[1])
		return AK_TRUE;

	return AK_FALSE;
}

/*
 * @BREIF    transc of set burn mode
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_SetMode(unsigned long data[], unsigned long len)
{
	init_info.eMode = data[0] >> 16;

	return AK_TRUE;
}

/*
 * @BREIF    transc of Get flash ID
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_GetFlashID(unsigned long data[], unsigned long len)
{
	T_CHIP_INFO chip_info;


#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)

	//Get flash ID and chip count
	memset(&chip_info, 0, sizeof(T_CHIP_INFO));
	/* get nandflash id and chip_cnt*/
	chip_info.chip_ID = SPIFlash_get_id(0);
	chip_info.chip_cnt = init_info.nChipCnt;
	if (write(fd, &chip_info, sizeof(T_CHIP_INFO)) 
		!= sizeof(T_CHIP_INFO)) {
		printf("send nand chip info error");
		return AK_FALSE;
	}
#else

	//Get flash ID and chip count
	memset(&chip_info, 0, sizeof(T_CHIP_INFO));
	/* get nandflash id and chip_cnt*/
	chip_info.chip_ID = get_nand_id(0);
	chip_info.chip_cnt = init_info.nChipCnt;
	if (write(fd, &chip_info, sizeof(T_CHIP_INFO)) 
		!= sizeof(T_CHIP_INFO)) {
		printf("send nand chip info error");
		return AK_FALSE;
	}
#endif

	return AK_TRUE;
}

/*
 * @BREIF    transc of Initial sec area
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   void *
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_InitSecArea(void *data)
{
	/*if init sec area fail, format the area */
	if (AK_FALSE == FHA_asa_scan(AK_TRUE))
	{
		if (AK_TRUE == FHA_asa_format(*(unsigned long *)data)) {
			//printf("FHA_asa_format success!\n");
			cmd_ret = 1;
			return AK_TRUE;
		}
		else {
			printf("FHA_asa_format fail!\n");
			cmd_ret = 2;
			return AK_FALSE;
		}
	}
	//printf("FHA_asa_scan success!\n");

	cmd_ret = 1;
	return AK_TRUE;
}

/*
 * @BREIF    transc of create init secarea thread
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   void *
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static void *initsecarea(void *data)
{
	Transc_InitSecArea(data);
	
	return NULL;
}

/*
 * @BREIF    transc of start download bin
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_StartDLBin(unsigned long data[], unsigned long len)
{
   int ret ;
#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
	unsigned long reserved_size;
#endif
	
	read(fd, &bin_param, len);
	m_dl_control.file_total_len = bin_param.data_length;
	m_dl_control.data_total = 0;

#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
	bin_param.bBackup = AK_FALSE;
	reserved_size = spi_phy_info.total_size - (spi_info.BinPageStart / spi_info.PagesPerBlock + 2) * (spi_info.PageSize * spi_info.PagesPerBlock);
	if (bin_param.data_length > reserved_size) {
		printf("Error: bin size large than remanent flash size\n");
		return AK_FALSE;
	}
#endif

	if (FHA_SUCCESS == FHA_write_bin_begin(&bin_param)) {

		if (init_info.eMode == MODE_UPDATE)
			{
				ret = Transc_GetPartion2();
				if(AK_TRUE == ret)	
				{
					printf("Transc_GetPartion2 success\n");
					return AK_TRUE;
				}
				else
				{
				printf("Transc_GetPartion2 failed\n");
				return AK_FALSE;
				}
			} 

		return AK_TRUE;
	}
	else {
		printf("FHA_write_bin_begin fail\n");
		return AK_FALSE;
	}
}

/*
 * @BREIF    transc of download bin data
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_DLBin(unsigned long data[], unsigned long len)
{
	T_BOOL ret= AK_TRUE;
	
	//check length of data
	m_dl_control.data_total += len;
	if (m_dl_control.data_total > m_dl_control.file_total_len
		|| (0 == len) && (m_dl_control.data_total < m_dl_control.file_total_len)) {
		return AK_FALSE;
	}

	if (len > 0) {
		if (read(fd, buf, len) != len) {
			printf("read bin data error\n");
			return AK_FALSE;
		}
		ret = FHA_write_bin(buf, len);
	}

	return ret;
}

/*
 * @BREIF    transc of start download boot
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_StartDLBoot(unsigned long data[], unsigned long len)
{
	read(fd, buf, len);
	m_dl_control.file_total_len = (*(unsigned long *)buf);
	m_dl_control.data_total = 0;
	
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	if (init_info.eMode == MODE_UPDATE)
		if (FHA_FAIL == FHA_set_fs_part(part_info, sizeof(int) 
			+ partition_count * sizeof(parts[0]) + partition_count * sizeof(ori_mbyte[0])))
			return AK_FALSE;
#endif		

	if (FHA_write_boot_begin(
			m_dl_control.file_total_len) == AK_TRUE) {
		//printf("FHA_write_boot_begin success\n");
    	return AK_TRUE;
	}
	else {
		printf("FHA_write_boot_begin fail\n");
		return AK_FALSE;
	}
}

/*
 * @BREIF    transc of download boot data
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_DLBoot(unsigned long data[], unsigned len)
{
	//check length of data
	m_dl_control.data_total += len;
	if (m_dl_control.data_total > m_dl_control.file_total_len
		|| (0 == len) && (m_dl_control.data_total < m_dl_control.file_total_len)) {
		return AK_FALSE;
	}

	if (len > 0) {
		if (read(fd, buf, len) != len) {
			printf("read boot data error\n");
			return AK_FALSE;
		}
		if (FHA_write_boot(buf, len) == AK_TRUE) {
			//printf("FHA_write_boot success\n");
			return AK_TRUE;
		}
		else {
			printf("FHA_write_boot fail\n");
			return AK_FALSE;
		}
	}

	return AK_TRUE;
}

static void free_mem()
{
	free(part_info);
	part_info = NULL;
	free(parts);
	parts = NULL;
	free(ori_mbyte);
	ori_mbyte = NULL;
	free(mounted);
	mounted = NULL;
	free(userpart);
	userpart = NULL;
	free(pc_parts);
	pc_parts = NULL;
}

/*
 * @BREIF    transc of close to write config information
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_Close(unsigned long data[], unsigned long len)
{	
	if ((init_info.eMode == MODE_UPDATE)&&(kernel_update_flag == 1))
	{
		if (FHA_SUCCESS == FHA_set_fs_part(part_info,spi_info.PageSize ))
		{
			printf("FHA_set_fs_part update success!\n");
		} 
		else 
		{
			printf("FHA_set_fs_part update failed !\n");
		}
	}
	umount_all();
	free_mem();
	return FHA_close();
}

/*
 * @BREIF    transc of get disk info
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
//T_BOOL Transc_GetDiskInfo(T_U8 data[], T_U32 len, T_CMD_RESULT *result)
//{
//	m_transc_state = TRANS_GET_DISK_INFO;

//	FHA_get_fs_part(buf, len);
	// 
//	write(fd, buf, len);

//	return AK_TRUE;
//}

#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
static unsigned long long mbytes_to_bytes(float x)
{
	unsigned long i, f;
	//unsigned int block_size = spi_info.PageSize * spi_info.PagesPerBlock;

	i = x;
	i = i * 1024 * 1024;
	f = x * 1000;
	f = f % 1000;
	f = f * 1024 * 1024;
	f = f / 1000;

	if ((i + f) % align_size)
		return ((i + f) / align_size + 1) * align_size;
	else
		return (i + f);
}
#endif

/*
 * @BREIF    transc of create file system partion
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   void *
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_CreatePartion()
{

#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))

	int i, j, tmp, blknum, start_blk;

	/* get the user space start block */
	start_blk = FHA_get_last_pos() + 1;

	/* erase the user space of the nand */
	for (i = start_blk; i < init_info.nChipCnt * nand_info.blk_num; i++)
	{
		/* if the block is bad, go to next */
		if (FHA_check_bad_block(i) == AK_TRUE)
			continue;
		/* if erase the block fail, marker the block bad*/
		if (nand_erase(0, i * nand_info.page_per_blk) == AK_FALSE)
			FHA_set_bad_block(i);
	}
	
	partition_count = *(unsigned long *)(buf + sizeof(unsigned long));
	parts = malloc(partition_count * sizeof(*parts));
	ori_mbyte = malloc(partition_count * sizeof(*ori_mbyte));
	mounted = malloc(partition_count);
	userpart = malloc(partition_count);
	pc_parts = malloc(partition_count * sizeof(*pc_parts));
	if (parts == NULL || mounted == NULL
		|| ori_mbyte == NULL || userpart == NULL || pc_parts == NULL) {
		cmd_ret = 2;
		return AK_FALSE;
	}
	memset(ori_mbyte, 0, partition_count * sizeof(*ori_mbyte));
	memset(mounted, 0, partition_count);
	memset(userpart, 0, partition_count);
	memcpy(pc_parts, buf + 2 * sizeof(unsigned long), 
		partition_count * sizeof(*pc_parts));
	parts[0].name[0] = pc_parts[0].Disk_Name;
	parts[0].name[1] = '\0';

	/* calculate the blknum of the well blocks to allocate */
	ori_mbyte[0] = pc_parts[0].Size;
	printf("parts[0] mbyte size = %d\n", ori_mbyte[0]);
	blknum =  (unsigned long long)ori_mbyte[0] * SZ_1M / (nand_info.page_size * nand_info.page_per_blk)
		   	* (1 + BLOCK_RESERVED_RATE);		/* multiplied by the rate */
	parts[0].offset = (unsigned long long)start_blk * (unsigned long long)nand_info.page_size * (unsigned long long)nand_info.page_per_blk;

	parts[0].mask_flags = 0x0;

	/* to allocate blknum of well blocks, thus the bad block overleap */ 
	for (i = start_blk, tmp = blknum; i < init_info.nChipCnt * nand_info.blk_num; i++)
	{
		/* if the block is bad, go to next */
		if (FHA_check_bad_block(i) == AK_FALSE)
			tmp--;
		if (0 == tmp) {
			blknum = i + 1 - start_blk;
			start_blk = i + 1;
			break;
		}
	}
	if (i >= init_info.nChipCnt * nand_info.blk_num) {
		printf("There is no enough space to create partition 1\n");
		cmd_ret = 2;
		return AK_FALSE;
	}
	parts[0].size = (unsigned long long)blknum * ((unsigned long long)nand_info.page_size)
		* ((unsigned long long)nand_info.page_per_blk);
	
	for (j = 1; j < partition_count; j++)
	{
		parts[j].name[0] = pc_parts[j].Disk_Name;
		parts[j].name[1] = '\0';
		parts[j].offset = parts[j - 1].offset + parts[j - 1].size;
		parts[j].mask_flags = 0x0;
		ori_mbyte[j] = pc_parts[j].Size;
		printf("parts[%d] mbyte size = %d\n", j, ori_mbyte[j]);
		blknum = (unsigned long long)ori_mbyte[j] * SZ_1M / (nand_info.page_size * nand_info.page_per_blk)
		   	* (1 + BLOCK_RESERVED_RATE);

		if (j < partition_count - 1)
		{
			/* to allocate blknum of well blocks, thus the bad block overleap */ 
			for (i = start_blk, tmp = blknum; i < init_info.nChipCnt * nand_info.blk_num - 1; i++)
			{
				/* if the block is bad, go to next */
				if (FHA_check_bad_block(i) == AK_FALSE)
					tmp--;
				if (0 == tmp) {
					blknum = i + 1 - start_blk;
					start_blk = i + 1;
					break;
				}
			}
			if (i >= init_info.nChipCnt * nand_info.blk_num - 1) {
				printf("there is no enough space to create partition\n");
				cmd_ret = 2;
				return AK_FALSE;
			}
			parts[j].size = ((unsigned long long)blknum) 
				* ((unsigned long long)nand_info.page_size) 
				* ((unsigned long long)nand_info.page_per_blk);
		}
		else
                        parts[j].size = ((unsigned long long)((unsigned long)nand_info.blk_num * init_info.nChipCnt - start_blk)) * ((unsigned long long)nand_info.page_size) * ((unsigned long long)nand_info.page_per_blk);
	}
#if 0
	for (i = 0; i < partition_count; i++) {
		printf("parts[%d].name = %s\n", i, parts[i].name);
		printf("parts[%d].size = %lx\n", i, parts[i].size);
		printf("parts[%d].offset = %lx\n", i, parts[i].offset);
		printf("parts[%d].flag = %x\n", i, parts[i].mask_flags);
	}
#endif
	memcpy(buf, &partition_count, sizeof(unsigned long));
	memcpy(buf + sizeof(unsigned long), parts, partition_count * sizeof(parts[0]));
	memcpy(buf + sizeof(unsigned long) + partition_count * sizeof(parts[0]),
		ori_mbyte, partition_count * sizeof(ori_mbyte[0]));

#if 0
	for (i = 0; i < sizeof(int) + partition_count * sizeof(parts[0]); i++)
		printf("buf[%d] = %2x\n", i, buf[i]);
#endif

	/* save the partitions infomation */
	if (FHA_SUCCESS == FHA_set_fs_part(buf, sizeof(unsigned long) + 
			partition_count * sizeof(parts[0]) + partition_count * sizeof(ori_mbyte[0]))) {
		//printf("FHA_set_fs_part success!\n");
		cmd_ret = 1;
		return AK_TRUE;
	} else {
		printf("FHA_set_fs_part fail!\n");
		cmd_ret = 2;
		return AK_FALSE;
	}

#if 0
	memset(buf, 0, 85536);
	if (FHA_SUCCESS == FHA_get_fs_part(buf, sizeof(int) + partition_count * sizeof(parts[0]))) {
		printf("FHA_get_fs_part success!\n");
	} else {
		printf("FHA_get_fs_part fail!\n");
	}

	for (i = 0; i < sizeof(int) + partition_count * sizeof(parts[0]); i++)
		printf("***buf[%d] = %2x\n", i, buf[i]);
#endif
#else
	int i;
	unsigned int start_page;
	unsigned int start_block;
	unsigned int end_block;
	unsigned int x, y;

	start_page = FHA_get_last_pos();
#if 0
	if (start_page % spi_info.PagesPerBlock) {
		start_block = start_page / spi_info.PagesPerBlock + 1;
	} else {
		start_block = start_page / spi_info.PagesPerBlock;
	}
#else
	x = align_size / spi_phy_info.page_size;
	y = ((start_page / x) + 1) * x;
	start_block = y / spi_info.PagesPerBlock;
#endif
	printf("===>start_page:%u\n", start_page);
	printf("===>start_block:%u\n", start_block);
	
	partition_count = *(unsigned long *)(buf + sizeof(unsigned long));
	printf("partition_count:%d\n", partition_count);

	parts = malloc(partition_count * sizeof(*parts));
	mounted = malloc(partition_count + 1);
	userpart = malloc(partition_count);
	pc_parts = malloc(partition_count * sizeof(*pc_parts));
	if (!parts || !pc_parts || !mounted || !userpart) {
		printf("Allocate memory failed\n");
		cmd_ret = 2;
	}
	memset(mounted, 0, partition_count + 1);
	memset(userpart, 0, partition_count);
	memset(parts, 0, partition_count*sizeof(*parts));
	memset(pc_parts, 0, partition_count*sizeof(*pc_parts));

	memcpy(pc_parts, buf + 2 * sizeof(unsigned long), partition_count * sizeof(*pc_parts));

	parts[0].name[0] = pc_parts[0].Disk_Name;
	parts[0].name[1] = '\0';
	parts[0].size = mbytes_to_bytes(pc_parts[0].Size);
	parts[0].offset = start_block * spi_info.PagesPerBlock * spi_info.PageSize;
	if (parts[0].offset + parts[0].size > spi_phy_info.total_size) {
		printf("Partition %c large than remain size %u bytes of flash\n", 
					pc_parts[0].Disk_Name, spi_phy_info.total_size - parts[0].offset);
		cmd_ret = 2;
		return AK_FALSE;
	}

	for (i = 1; i < partition_count; i++) {
		parts[i].name[0] = pc_parts[i].Disk_Name;
		parts[i].name[1] = '\0';
		parts[i].size = mbytes_to_bytes(pc_parts[i].Size);
		parts[i].offset = parts[i-1].offset + parts[i-1].size;
		if (parts[i].offset + parts[i].size > spi_phy_info.total_size) {
			printf("Partition %c large than remain size %u bytes of flash\n", 
					pc_parts[i].Disk_Name, spi_phy_info.total_size - parts[0].offset);
			cmd_ret = 2;
			return AK_FALSE;
		}
	}

	end_block = (parts[i-1].offset + parts[i-1].size) / spi_info.PageSize / spi_info.PagesPerBlock;

	printf("===>end_block:%u\n", end_block);
	for (i = 0; i < partition_count; i++) {
		printf("Name:%s\n", parts[i].name);
		printf("Offset:%llu\n", parts[i].offset);
		printf("Size:%llu\n", parts[i].size);
	}
			
	for (i = start_block; i < end_block; i++) {
		if (SPIFlash_erase(0, i * spi_info.PagesPerBlock) == FHA_FAIL) {
			printf("block %u erase failed\n", i);
			cmd_ret = 2;
			return AK_FALSE;
		} 
	}

	memcpy(buf, &partition_count, sizeof(unsigned long));
	memcpy(buf + sizeof(unsigned long), parts, partition_count * sizeof(parts[0]));
	printf("Sizeof fs info %u\n", sizeof(unsigned long) + partition_count * sizeof(parts[0]));
	if (FHA_SUCCESS == FHA_set_fs_part(buf, sizeof(unsigned long) + partition_count * sizeof(parts[0]))) {
		printf("FHA_set_fs_part success!\n");
		cmd_ret = 1;
		return AK_TRUE;
	} else {
		printf("FHA_set_fs_part fail!\n");
		cmd_ret = 2;
		return AK_FALSE;
	}

	cmd_ret = 1;
#endif
	return AK_TRUE;
}

/*
 * @BREIF    transc of create partition thread
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   void *
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static void *createpartion(void *data)
{
	Transc_CreatePartion();

	return NULL;
}



static T_BOOL Transc_GetPartion2()
{

	int i, ret;
	unsigned int start_block;
	unsigned int end_block;
	unsigned ori_parts_count;

	part_info = malloc(spi_info.PageSize);
	if (part_info == NULL) {
		printf("malloc part_info error\n");
		cmd_ret = 2;
		return AK_FALSE;
	}
	ret = FHA_get_fs_part(part_info, spi_info.PageSize);
	if (AK_FALSE == ret) {
		fprintf(stderr, "get fs partition info failed\n");
		cmd_ret = 2;
		return AK_FALSE;
	}
	return AK_TRUE;	
}

/*
 * @BREIF    transc of create file system partion
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   void *
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_GetPartion()
{
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))

	int ret, size, i, j;
	unsigned long ori_parts_count;

	/* get the old part_info */
	size = sizeof(partition_count) + sizeof(*parts) * MAX_PARTS_COUNT +
			sizeof(*ori_mbyte) * MAX_PARTS_COUNT;
	part_info = malloc(size);
	if (part_info == NULL) {
		printf("malloc part_info error\n");
		cmd_ret = 2;
		return AK_FALSE;
	}
	ret = FHA_get_fs_part(part_info, size);
	if (AK_FALSE == ret) {
		cmd_ret = 2;
		return AK_FALSE;
	}		
	
	partition_count = *(unsigned long *)(part_info);
	ori_parts_count = *(unsigned long *)(buf + 4);
	if (partition_count != ori_parts_count) {
		printf("orignal partition count %d is not consistent with burntool count %d\n", 
				partition_count, ori_parts_count);
		cmd_ret = 2;
		return AK_FALSE;
	}
	
	parts = malloc(partition_count * sizeof(*parts));
	ori_mbyte = malloc(partition_count * sizeof(*ori_mbyte));
	mounted = malloc(partition_count);
	userpart = malloc(partition_count);
	pc_parts = malloc(partition_count * sizeof(*pc_parts));
	if (parts == NULL || ori_mbyte == NULL 
		|| mounted == NULL || userpart == NULL || pc_parts == NULL) {
		cmd_ret = 2;
		return AK_FALSE;
	}
	memset(mounted, 0, partition_count);
	memset(ori_mbyte, 0, partition_count * sizeof(*ori_mbyte));
	memset(userpart, 0, partition_count);
	memcpy(parts, part_info + sizeof(partition_count), partition_count * sizeof(*parts));
	memcpy(ori_mbyte, part_info + sizeof(partition_count) + partition_count * sizeof(*parts),
		partition_count * sizeof(*ori_mbyte));
	memcpy(pc_parts, buf + 2 * sizeof(unsigned long), 
		partition_count * sizeof(*pc_parts));
	for (i = 0; i < partition_count - 1; i++)
	{
		if (ori_mbyte[i] != pc_parts[i].Size) {
			printf("the part[%d] original size %d is not consistent with burntool %d\n", 
				i, ori_mbyte[i], pc_parts[i].Size);
			cmd_ret = 2;
			return AK_FALSE;
		}
	}

	for (i = 0; i < partition_count; i++)
	{
		if (pc_parts[i].bOpenZone == 0) {
			printf("erase the system partiton part[%d]\n", i);
			userpart[i] = 0;
			startblock = parts[i].offset / (nand_info.page_per_blk * nand_info.page_size);
			endblock = startblock + parts[i].size / (nand_info.page_per_blk * nand_info.page_size);
			for(j = startblock; j < endblock; j++)
			{
				//can't erase bad block
				if (FHA_check_bad_block(j) == AK_TRUE)
					continue;
				if (nand_erase(0, j * nand_info.page_per_blk) == AK_FALSE) {
					printf("check membbt,blkno[%d] is bad,skip to next block\n",i);
				//marker the block is bad
					FHA_set_bad_block(j);
					break;
				}
			}
		} else
			userpart[i] = 1;
	}
	cmd_ret = 1;

#else
	int i,j, ret;
	unsigned int start_block;
	unsigned int end_block;
	unsigned ori_parts_count;


	printf("Transc_GetPartion FHA_burn_init\n");
	if(part_info == NULL)
	{
		kernel_update_flag = 0;
		part_info = malloc(spi_info.PageSize);
		if (part_info == NULL) {
			printf("malloc part_info error\n");
			cmd_ret = 2;
			return AK_FALSE;
		}
		FHA_burn_init(&init_info, &callback, &spi_info);
		ret = FHA_get_fs_part(part_info, spi_info.PageSize);
		if (AK_FALSE == ret) {
			fprintf(stderr, "get fs partition info failed\n");
			cmd_ret = 2;
			return AK_FALSE;
		}
	}
    else
	{
	  kernel_update_flag = 1;
	  FHA_burn_init(&init_info, &callback, &spi_info);
	}
	
	partition_count = *(unsigned long *)(part_info);
	ori_parts_count = *(unsigned long *)(buf + 4);
	fprintf(stderr, "partition_count:%d, ori_parts_count:%d\n", partition_count, ori_parts_count);
	if (partition_count != ori_parts_count) {
		printf("orignal partition count %d is not consistent with burntool count %d\n", 
				partition_count, ori_parts_count);
		cmd_ret = 2;
		return AK_FALSE;
	}

	pc_parts = malloc(partition_count * sizeof(*pc_parts));
	parts = malloc(ori_parts_count * sizeof(*parts));
	mounted = malloc(partition_count + 1);
	userpart = malloc(partition_count);
	memset(mounted, 0, partition_count + 1);
	memset(userpart, 0, partition_count);

	memcpy(pc_parts, buf + 2 * sizeof(unsigned long), partition_count * sizeof(*pc_parts));
	memcpy(parts, part_info + sizeof(unsigned long), ori_parts_count * sizeof(*parts));
	for (i = 0; i < partition_count; i++) {
		if (mbytes_to_bytes(pc_parts[i].Size) != parts[i].size) {
			fprintf(stderr, "part:%s size %llu different from burntool %llu\n", parts[i].name, 
					parts[i].size, mbytes_to_bytes(pc_parts[i].Size));
			cmd_ret = 2;
			return AK_FALSE;
		}
	}
	if (init_info.eMode == MODE_NEWBURN)
		{	
			for (i = 0; i < partition_count; i++) {	
				if (pc_parts[i].bOpenZone == 0) {
					printf("erase the system partiton part[%d]\n", i);
					userpart[i] = 0;
					start_block = parts[i].offset / (spi_info.PageSize * spi_info.PagesPerBlock);
					end_block = start_block + parts[i].size / (spi_info.PageSize * spi_info.PagesPerBlock);
					for (j = start_block; j < end_block; j++) {			
						if (SPIFlash_erase(0, j * spi_info.PagesPerBlock) == FHA_FAIL) {
							printf("block %u erase failed\n", j);
							cmd_ret = 2;
							return AK_FALSE;
						} 
					}
				} else {
					fprintf(stderr, "part:%s is user partition\n", parts[i].name);
					userpart[i] = 1;
				}
			}
		}
	cmd_ret = 1;
#endif
	return AK_TRUE;
}

/*
 * @BREIF    transc of get partition thread
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   void *
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static void *getpartion(void *data)
{
	Transc_GetPartion();

	return NULL;
}

/*
 * @BREIF    transc of mount driver
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_MountDriver(unsigned long data[], unsigned long len)
{
	if (read(fd, buf, len) <= 0) {
		printf("read mount data error\n");
		return AK_FALSE;
	}
	
	return AK_TRUE;
}

/*
 * @BREIF    transc of format driver
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_FormatDriver(unsigned long data[], unsigned long len)
{
//	char command[] = "mkyaffs /mnt/mtd0";
	int symbol;

	if (read(fd, buf, len) <= 0) {
		printf("read format data error\n");
		return AK_FALSE;
	}
//	symbol = buf[4] - 'A';	
//	command[19] = '0' + symbol;

	return AK_TRUE;
}

/*
 * @BREIF    transc of start download image
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_StartDLImg(unsigned long data[], unsigned long len)
{
	int i, index;

	if (read(fd, buf, len) != len) {
		printf("read img info error");
		return AK_FALSE;
	}
	memcpy(&imginfo, buf, sizeof(imginfo));
	m_dl_control.file_total_len = image_size = imginfo.data_length;
	m_dl_control.data_total = 0;

	for (i = 0; i < partition_count; i++) {
		if (parts[i].name[0] == imginfo.DriverName) {
			index = i;
			break;
		}
	}
	if (i == partition_count)
		return AK_FALSE;

#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	if (FHA_FAIL == burn_yaffs2_image_start(index)) {
		printf("burn_yaffs2_image_start fail\n");
		return AK_FALSE;
	}
#else
	if (burn_jffs2_image_start(index)) {
		printf("burn jffs2 image failed\n");
		return AK_FALSE;
	}
#endif	
	return AK_TRUE;
}
/*
 * @BREIF    transc of download image data
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_DLImg(unsigned long data[], unsigned long len)
{
	// T_U32 ack = BT_SUCCESS;
	static unsigned long rsv_len = 0;
	unsigned long total_len;
	
	//check length of data
	m_dl_control.data_total += len;
	if (m_dl_control.data_total > m_dl_control.file_total_len
		|| (0 == len) && (m_dl_control.data_total < m_dl_control.file_total_len)) {
		return AK_FALSE;
	}

	//last packet to close file handle
    if(0 == len) {
		if (rsv_len == 0) {
			//printf("Transc_DLImg success!\n");
			return AK_TRUE;
		}
		else {
			printf("Transc_DLImg fail!\n");
			return AK_FALSE;
		}
	}

	if (read(fd, buf + rsv_len, len) != len) {
		printf("read img data error");
		return AK_FALSE;
	}
	total_len = len + rsv_len;
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	rsv_len = total_len % (nand_info.page_size + yaffs_tag_size);
	burn_yaffs2_image(imginfo.DriverName - 'A', buf, total_len - rsv_len);
	memcpy(buf, buf + total_len - rsv_len, rsv_len);
#else
	if (burn_jffs2_image(imginfo.DriverName - 'A', buf, total_len))
		return AK_FALSE;
#endif
	return AK_TRUE;
}

/*
 * @BREIF    transc of create all mtd device
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static void create_mtd_device(void)
{
	int ret;
	char *name;
	char pathname[MAXLINE/2] = DEV_PATH;
	char buf[MAXLINE] = {0};
	FILE *fp;
	int major, minor;
	dev_t dev;

	fp = fopen("/proc/partitions", "r");
	if (!fp) {
		fprintf(stderr, "open /proc/partitons failed\n");
		return;
	}
	while(fgets(buf, MAXLINE, fp) != NULL) {
		name = strstr(buf, DEV_NAME);
		if (name) {
			memcpy(pathname + strlen(DEV_PATH), name, strlen(name) - 1);
			fprintf(stderr, "pathname:%s\n", pathname);
			fprintf(stderr, "major:%d\n", MTD_BLOCK_MAJOR);
			fprintf(stderr, "minor:%d\n", atoi(name + strlen(DEV_NAME)));
			dev = makedev(MTD_BLOCK_MAJOR, atoi(name + strlen(DEV_NAME)));
			if(mknod(pathname, S_IFBLK, dev)) {
				if (errno == EEXIST)
					fprintf(stderr, "device node %s already exists\n", pathname);
				else
					fprintf(stderr, "device node %s create failed\n", pathname);
			} else {
				fprintf(stderr, "create %s\n", pathname);
			}
		}
	}
	fclose(fp);
}

/*
 * @BREIF    transc of start download udisk file
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_StartDLFile()
{
	T_UDISK_FILE_INFO* pFileInfo;
	int i, index, ret;
	//char command1[] = "mkdir /mnt/mtd0";
	char command2[] = "mount /dev/mtdblock0  /mnt/mtd00";
	char path[ANYKA_MAX_PATH];
	int isdir;

	pFileInfo = (T_UDISK_FILE_INFO *)buf;
	printf("file_mode:%x\n", pFileInfo->file_mode);
	
	/* see whether the file is the directory */ 
	if(pFileInfo->file_mode == ANYKA_DIR_TYPE)
		isdir = 1;
	else
		isdir = 0;
	
	m_dl_control.file_total_len = pFileInfo->file_length;
	m_dl_control.data_total = 0;

	printf("%s\n", pFileInfo->apath);
	if (pFileInfo->apath[1] != ':' || pFileInfo->apath[2] != '\\') {
		cmd_ret = 2;
		return AK_FALSE;
	}
#if 0
	for (i = 0; i < strlen(pFileInfo->apath); i++) {
		if (pFileInfo->apath[i] == ' ')
			return AK_FALSE;
	}
#endif

#ifndef CONFIG_AK39XX
	if (mtd_mount == 0 && mount_mtd_partitions(init_info.nChipCnt) == AK_FALSE) {
		printf("mount mtd partitions fail\n");
		cmd_ret = 2;
		return AK_FALSE;
	}
#endif
	mtd_mount = 1;

	for (i = 0; i < partition_count; i++) {
		if (pFileInfo->apath[0] == parts[i].name[0]) {
			index = i;
			break;
		}
	}
	if (i == partition_count) {
		cmd_ret = 2;
		return AK_FALSE;
	}

	create_mtd_device();

#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
	index += 1;
#endif
	if (mounted[index] == 0) {
		//command1[14] = '0' + index;
		if (index < 10)
			command2[19] = command2[31] = '0' + index;
		else if (index >= 10 && index < 20) {
			command2[19] = '1';
			command2[20] = index - 10 + '0';
			command2[30] = '1';
			command2[31] = index - 10 + '0';
		} else {
			command2[19] = '2';
			command2[20] = index - 20 + '0';
			command2[30] = '2';
			command2[31] = index - 20 + '0';
		}		
		//system(command1);

		if (ret = system(command2)) {
			printf("\n\n%d\n", ret);
			cmd_ret = 2;
			return AK_FALSE;
		}
		mounted[index] = 1;
	}

	strcpy(path, "/mnt/mtd00");

	if (index < 10)
		path[9] = '0' + index;
	else if (index >= 10 && index < 20) {
		path[8] = '1';
		path[9] = index - 10 + '0';
	} else {
		path[8] = '2';
		path[9] = index - 20 + '0';
	}
		
	strcat(path, pFileInfo->apath + 2);
	for (i = 0; i < strlen(path); i++) {
		if (path[i] == '\\')
			path[i] = '/';
	}
	if (start_burn_file(path, isdir) == AK_TRUE) {
		//printf("start_burn_file success!\n");
		cmd_ret = 1;
		return AK_TRUE;
	} else {
		printf("start_burn_file fail!\n");
		cmd_ret = 2;
		return AK_FALSE;
	}
}

/*
 * @BREIF    transc of start download file thread
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static void *startdlfile(void *data)
{
	Transc_StartDLFile();

	return NULL;
}

/*
 * @BREIF    transc of umount all device
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static void umount_all(void)
{
	FILE *fp;
	struct mntent *mtpair;

	fp = setmntent("/proc/mounts", "r");
	if (!fp) {
		fprintf(stderr, "open /proc/mounts failed\n");
		return;
	}

	while(mtpair = getmntent(fp)) {
		if(strstr(mtpair->mnt_fsname, DEV_NAME)) {
			fprintf(stderr, "umount dev:%s, dir:%s\n", mtpair->mnt_fsname, mtpair->mnt_dir);
			umount(mtpair->mnt_dir);
		}
	}
}

/*
 * @BREIF    transc of download udisk file
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_DLFile(unsigned long data[], unsigned long len)
{
	//T_U32 ack = BT_SUCCESS;
	T_BOOL ret= AK_TRUE;
	
	//check length of data
	m_dl_control.data_total += len;
	if (m_dl_control.data_total > m_dl_control.file_total_len
		|| (0 == len) && (m_dl_control.data_total < m_dl_control.file_total_len)) {
		return AK_FALSE;
	}
	//last packet to close file handle
    if(0 == len) {
		if (close_file() == AK_FALSE) {
			printf("close file fail!\n");
			return AK_FALSE;
		}
		return AK_TRUE;
	}

	if (read(fd, buf, len) != len) {
		printf("read file data error");
		return AK_FALSE;
	}
	if (len == burn_file(buf, len)) {
		//printf("burn_file success!\n");
		return AK_TRUE;
	}
	else {
		printf("burn_file fail!\n");
		return AK_FALSE;
	}
}	

/*
 * @BREIF    transc of set nand param
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_SetNandParam(unsigned long data[], unsigned long len)
{
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	memset(&nand_info, 0, sizeof(nand_info));
	if (read(fd, &nand_info, len) != len) {
		printf("read nand param error");
		return AK_FALSE;
	}
	if (AK_FALSE == send_nand_para(&nand_info)) {
		printf("send_nand_para fail!\n");
		return AK_FALSE;
	}
	if (nand_info.page_size < 2048)
		yaffs_tag_size = YAFFS_TAG_SIZE_512_1024;
	else
		yaffs_tag_size = YAFFS_TAG_SIZE;

	chip_page_num = nand_info.page_per_blk * nand_info.blk_num;
	
	return FHA_burn_init(&init_info, &callback, &nand_info);
#else
    return AK_TRUE;
#endif
}

/*
 * @BREIF    transc of set SPI Flash param
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_SetSPIParam(unsigned long data[], unsigned long len)
{
	if (read(fd, &spi_phy_info, len) != len) {
		printf("read nand param error");
		return AK_FALSE;
	}

	if(spi_phy_info.reserved1 == 0)
	{
		printf("align size == 0, use default %u\n", align_size);
	}
	else if(spi_phy_info.reserved1 * 1024 % spi_phy_info.erase_size != 0)
	{
		printf("align size \% erase size !=0 use default %u\n", align_size);
	}
	else
	{
		align_size = spi_phy_info.reserved1 * 1024;
		printf("align size = %u\n", align_size);
	}

	SPIFlash_SetEraseSize(spi_phy_info.erase_size);
	spi_info.PageSize = spi_phy_info.page_size;
	spi_info.PagesPerBlock = spi_phy_info.erase_size / spi_phy_info.page_size;
	FHA_burn_init(&init_info, &callback, &spi_info);

	return AK_TRUE;
}

/*
 * @BREIF    transc of channel_id
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_GetChannelId(unsigned long data[], unsigned long len)
{
	unsigned long id = -1, ret = -1;

	if ((ret = ioctl(fd, AK_GET_CHANNEL_ID, &id)) < 0) {
		id = 0xffffffff;
		perror("get channel id error");
	}
	if ((ret = ioctl(fd, AK_USBBURN_STALL, 0)) < 0) {
		printf("fatal error: ioctl stall failed");
		while (1)
			;
	}
	printf("channel id = %ld\n", id);
	if (write(fd, &id, sizeof(id)) != sizeof(id))
		return AK_FALSE;

	return AK_TRUE;
}

/*
 * @BREIF    transc of nand chip count
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_SetNandChipCount(unsigned long data[], unsigned long len)
{
	int ret;
        if (read(fd, buf, len) != len && len < sizeof (nand_ce_data)) {
                printf("read nand chip data error");
                return AK_FALSE;
        }
        memcpy(&nand_ce_data, buf, sizeof(nand_ce_data));
        printf("chip_num = %d, gpio_chip_2 = %d, gpio_chip_3 = %d\n",
                nand_ce_data.chip_sel_num, nand_ce_data.gpio_chip_2, nand_ce_data.gpio_chip_3);
        ret = set_nand_ce(nand_ce_data.chip_sel_num, nand_ce_data.gpio_chip_2, nand_ce_data.gpio_chip_3);

        return ret;
}

/*
 * @BREIF    transc of channel_id
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_LedSignal(unsigned long data[], unsigned long len)
{
	unsigned long id = -1;
	char cmd[80];

	if (read(fd, &gpio_param, len) != len) {
		printf("read led gpio_param error\n");
		return AK_FALSE;
	}
	led_valid = 1;

	printf("gpio = %d\n", gpio_param.num);

	sprintf(cmd, "echo \"%d\" > \"%sled_gpio\"", gpio_param.num, LED_SYS_PATH);
	printf("%s\n", cmd);
	system(cmd);
	sprintf(cmd, "echo \"1\" > \"%sled_flash\"", LED_SYS_PATH);
	system(cmd);
	sprintf(cmd, "echo \"1000\" > \"%sled_cycle\"", LED_SYS_PATH);
	system(cmd);

	return AK_TRUE;
}

static void LedErrSignal()
{
	char cmd[80];

#ifdef CONFIG_AK98	
	if (led_valid) {
		sprintf(cmd, "echo \"100\" > \"%sled_cycle\"", LED_SYS_PATH);
		system(cmd);
	}
#endif	
}

static void BurnFailFun(int sig)
{
	LedErrSignal();
	//system("killall -SIGALRM error_flash");
}

/*
 * @BREIF    transc of burnover_param
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_BurnOverParam(unsigned long data[], unsigned long len)
{
	char cmd[80];
	
	if (read(fd, buf, len) != len) {
		printf("read burnover param error");
		return AK_FALSE;
	}
	memset(&burned_param, 0, sizeof(burned_param));
	memcpy(&burned_param, buf, sizeof(burned_param));
	if (burned_param.burned_mode == BURNED_POWER_OFF) {
		if (burned_param.bGpio) {
			sprintf(cmd, "echo \"%d\" > \"%spoweroff_gpio_number\"", burned_param.pwr_gpio_param.num, POWEROFF_GPIO_SYS);
			printf("%s\n",cmd);
			if (system(cmd) != 0) {
				printf("gpio number error\n");
				return AK_FALSE;
			}	
			if (burned_param.pwr_gpio_param.Pulldown != 255) {
				sprintf(cmd, "echo \"%d\" > \"%spoweroff_gpio_pulldown\"", burned_param.pwr_gpio_param.Pulldown, POWEROFF_GPIO_SYS);
				system(cmd);
			}
			else if (burned_param.pwr_gpio_param.Pullup != 255) {
				sprintf(cmd, "echo \"%d\" > \"%spoweroff_gpio_pullup\"", burned_param.pwr_gpio_param.Pullup, POWEROFF_GPIO_SYS);
				system(cmd);
			}
			sprintf(cmd, "echo \"%d\" > \"%spoweroff_gpio_level\"", burned_param.pwr_gpio_param.level, POWEROFF_GPIO_SYS);
			system(cmd);
		}
		if (burned_param.bWakup) {
			sprintf(cmd, "echo \"%d\" > \"%spoweroff_rtc_wakeup\"", burned_param.rtc_wakup_level, POWEROFF_GPIO_SYS);
			system(cmd);
		}
	}
	return AK_TRUE;
}	

/*
 * @BREIF    transc of set nand param
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_Default(unsigned long data[], unsigned long len)
{
	read(fd, buf, len);

	return AK_TRUE;
}

/*
 * @BREIF    transc of read ASA file
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_ReadASAFile(unsigned long data[], unsigned long len)
{
	static char filename[8] = {0};

	if (*data == 0) {
		if (read(fd, filename, len) != len) {
			printf("get asa file name failed when checking\n");
			return AK_FALSE;
		}

		if (FHA_asa_read_file(filename, buf, 1) == AK_FALSE) {
			printf("no asa file %s\n", buf);
			return AK_FALSE;
		}
	} else {
		if (FHA_asa_read_file(filename, buf, len) == AK_FALSE) {
			printf("read asa file %s failed\n", filename);
			return AK_FALSE;
		}
		
		if (write(fd, buf, len) != len) {
			printf("send %s failed\n", filename);
			return AK_FALSE;
		}
	}	

	return AK_TRUE;
}

/*
 * @BREIF    transc of write ASA file
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_WriteASAFile(unsigned long data[], unsigned long len)
{
	static char filename[8] = {0};
	unsigned char file_len[4] = {0};
	int i;

	if (data[0] == 0) {
		if (read(fd, filename, len) != len) {
			printf("get asa file name failed\n");
			return AK_FALSE;
		}
	} else {
		if (read(fd, buf, len) != len) {
			printf("get asa file content failed\n");
			return AK_FALSE;
		}

		if (FHA_asa_write_file(filename, buf, len, data[1]) == AK_FALSE) {
			printf("write asa file %s failed\n", filename);
			return AK_FALSE;
		}
	}

	return AK_TRUE;
}

/*
 * @BREIF    transc of upload bin start
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_UploadBinStart(unsigned long data[], unsigned long len)
{
	static char filename[16] = {0};
#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
	FHA_burn_init(&init_info, &callback, &spi_info);
#endif
	if (read(fd, filename, len) != len) {
		printf("get file name failed when read\n");
		return AK_FALSE;
	}
	
	memset(&bin_param, 0, sizeof(bin_param));
	strcpy(bin_param.file_name, filename);
	
	return FHA_read_bin_begin(&bin_param);
}

/*
 * @BREIF    transc of spidata start
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_UploadSPIDataStart(unsigned long data[], unsigned long len)
{
	unsigned int buflen;
	if (read(fd, &buflen, len) != len) {
		printf("get spi len failed when read\n");
		return AK_FALSE;
	}
	
	T_FHA_BIN_PARAM spi_param = {0};
	spi_param.data_length = buflen;
	printf("bin_param.data_length = %d \r\n", spi_param.data_length);
	return FHA_read_AllDatat_begin(&spi_param);
}

static T_FHA_BIN_PARAM bootParam;

/*
 * @BREIF    transc of upload boot start
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_UploadBootStart(unsigned long data[], unsigned long len)
{
	static char filename[16] = {0};
	
	memset(&bootParam, 0 , sizeof(bootParam));
	if (read(fd, filename, len) != len) {
		printf("get file name failed when read\n");
		return AK_FALSE;
	}
	strcpy(bootParam.file_name, filename);
#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
	bootParam.data_length = spi_info.BinPageStart - 1;
#endif
	return FHA_read_boot_begin(&bootParam);
}

/*
 * @BREIF    transc of set erase nand mode
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Transc_SetEraseNandMode(viod)
{
    T_U32 i, j;
    T_BOOL is_used = AK_FALSE;
    T_U8 *pBuf = AK_NULL;
    T_U32 badblock_page_cnt = 0;
    T_U32 spare = 0;
    T_U32 chip = init_info.nChipCnt;
    T_U32 blcok_num = nand_info.blk_num;
    T_U32 page_size = nand_info.page_size;
    T_U32 page_per_blk = nand_info.page_per_blk;
    T_U32 byte_loc, byte_offset;
    
    // EraseBlock when in the MODE_ERASE 
    badblock_page_cnt = (blcok_num - 1) / (page_size * 8) + 1;
    pBuf = malloc(badblock_page_cnt*page_size);
    if(AK_NULL == pBuf)
    {
        printf("Fwl_Erase_block malloc fail\r\n");
        return AK_FALSE;
    }
    //判断是否使用过。
    printf("*************************************************\r\n");
    if(FHA_check_Nand_isused(pBuf, badblock_page_cnt*page_size))
    {
        is_used = AK_TRUE;
        printf("is_used :%d\r\n", is_used);
    }
    memset(pBuf, 0, badblock_page_cnt*page_size);
    printf("chip:%d\r\n", chip);
    //此nand没有被使用过
    if(AK_FALSE == is_used)
    {
        printf("Nandflash format\r\n");
        for(j = chip; j >= 1; j--)
        {
            for (i = 0; i < blcok_num; i++)
            {
                printf("chip:%d,  block:%d\r\n", chip-j, i);
                
                if(FHA_Nand_check_block(chip - j, i))
                {
                    printf("eb:%d\r\n", i);
                	if(nand_erase(chip - j, i * page_per_blk) != FHA_SUCCESS)
                	{
                		printf("********erase fail\n");		
                	}
                }
                else
                {
                    printf("factory bad block:%d\r\n", i);
                }
            }
        }
        return AK_TRUE;
    }
 
   //判断第一个块是否出厂坏块
   if(!FHA_Get_factory_badblock_Buf(pBuf, badblock_page_cnt))
   {
        printf("FHA_Get_factory_badblock_Buf return fail\r\n");
        return AK_FALSE;
   }
    
    //此nand已使用过
    if(AK_TRUE == is_used)
    {
        for(j = chip; j >= 1; j--)
        {
            for (i = 0; i < blcok_num; i++)
            {
                //判断是否出厂坏块
                byte_loc = i / 8;
                byte_offset = 7 - i % 8;
 
                if(pBuf[byte_loc] & (1 << byte_offset))
                {
                    printf("bad block:%d\n", i);
                    continue;
                }
                //擦块
                printf("eb:%d, %d\n", chip-j, i);
                if(nand_erase(chip - j, i * page_per_blk) != FHA_SUCCESS)
                {
                	printf("********erase fail\n");		
                }
            }
        }
    }
    printf("erase blk_num: %d, %d\n", i, blcok_num);
    return AK_TRUE;
}

/*
 * @BREIF    transc of set bin resver size
 * @PARAM    [in] data buffer
 * @PARAM    [in] length of data
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL Trans_SetBinResvSize(unsigned long data[], unsigned long len)
{
	return FHA_set_bin_resv_size(*data);
}
static void usb_anyka_command(unsigned char *scsi_data, unsigned long size)
{
	volatile unsigned char cmd;
	unsigned long params[3];
	unsigned long params_tmp;
	unsigned long send_size;
	T_U32 tmpLen;
	int ret, i;
	char mkcmd[] = "mkdir /mnt/mtd00";

	cmd = *(scsi_data + 1);
	params[0] = (( *(scsi_data + 5) ) 
				| ( *(scsi_data + 6) << 8 ) 
				| ( *(scsi_data + 7) << 16 ) 
				| ( *(scsi_data + 8) << 24 ));
	params[1] = (( *(scsi_data + 9) ) 
				| ( *(scsi_data + 10) << 8 ) 
				| ( *(scsi_data + 11) << 16 ) 
				| ( *(scsi_data + 12) << 24 ));

	if (cmd >= 0x80) {
		cmd -= 0x80;
		if (TRANS_SWITCH_USB == cmd) {
			Transc_SwitchUsb(params, size);
			return;
		}		
		switch (cmd)
		{
		case TRANS_TEST_CONNECT:
			ret = Transc_TestConnection(params, size);
			break;
		case TRANS_SET_MODE:
			ret = Transc_SetMode(params, size);
			break;
		case TRANS_GET_FLASHID:
			ret = Transc_GetFlashID(params, size);
			break;
		case TRANS_INIT_SECAREA:
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
			params_tmp = params[0];
			cmd_ret = 0;
			ret = pthread_create(&ntid, NULL, initsecarea, &params_tmp);
			if (ret == 0) {
				printf("create init secarea pthread success!\n");
				ret = AK_TRUE;
			} else {
				printf("create init secarea pthread errror %d\n", ret);
				ret = AK_FALSE;
			}
#else
			ret = AK_TRUE;
#endif		
			break;
		case TRANS_GET_SCSI_STATUS:	
			if (ioctl(fd, AK_USBBURN_STALL, 0) < 0) {
				printf("fatal error: ioctl stall failed");
				while (1)
					;
			}
			if (write(fd, &cmd_ret, 1) != 1) {
				printf("write status info error");
				ret = AK_FALSE;
				break;
			}
			ret = AK_TRUE;
			break;
		case TRANS_DOWNLOAD_BIN_START:
			ret = Transc_StartDLBin(params, size);
			break;
		case TRANS_DOWNLOAD_BIN_DATA:
			ret = Transc_DLBin(params, size);
			break;
		case TRANS_DOWNLOAD_BOOT_START:
			ret = Transc_StartDLBoot(params, size);
			break;
		case TRANS_DOWNLOAD_BOOT_DATA:
			ret = Transc_DLBoot(params, size);
			break;
		case TRANS_CLOSE:
			ret = Transc_Close(params, size);
			break;
		/*
		case TRANS_GET_DISK_INFO:
			Transc_GetDiskInfo(params, size);
			*/
		case TRANS_CREATE_PARTITION:		
			if (read(fd, buf, size) != size) {
				printf("read create partition info error\n");
				ret = AK_FALSE;
				break;
			}
			partition_count = *(unsigned long *)(buf + sizeof(unsigned long));
#if defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX)
			partition_count += 1;
#endif
			for (i = 0; i < partition_count; i++) {
				if (i < 10)
					mkcmd[15] = '0' + i;
				else if (i >= 10 && i < 20) {
					mkcmd[14] = '1';
					mkcmd[15] = i - 10 + '0';
				} else {
					mkcmd[14] = '2';
					mkcmd[15] = i - 20 + '0';
				}				
				system(mkcmd);
			}
			cmd_ret = 0;
			if (init_info.eMode == MODE_NEWBURN)
				ret = pthread_create(&ntid, NULL, createpartion , NULL);
			else 
				ret = pthread_create(&ntid, NULL, getpartion, NULL);
			if (ret == 0) {
				printf("create init secarea pthread success!\n");
				ret = AK_TRUE;
			} else {
				printf("create init secarea pthread errror %d\n", ret);
				ret = AK_FALSE;
			}
			break;
		case TRANS_MOUNT_DRIVER:
			ret = Transc_MountDriver(params, size); 
			break;
		case TRANS_FORMAT_DRIVER:
			ret = Transc_FormatDriver(params, size);
			break;
		case TRANS_DOWNLOAD_IMG_START:
			ret = Transc_StartDLImg(params, size);
			break;
		case TRANS_DOWNLOAD_IMG_DATA:
			ret = Transc_DLImg(params, size);
			break;
		case TRANS_DOWNLOAD_FILE_START:
			if (read(fd, buf, size) != size) {
				printf("read file info error");
				ret = AK_FALSE;
				break;
			}
			cmd_ret = 0;
			ret = pthread_create(&ntid, NULL, startdlfile, NULL);
			if (ret == 0) {
				printf("create init secarea pthread success!\n");
				ret = AK_TRUE;
			} else {
				printf("create init secarea pthread errror %d\n", ret);
				ret = AK_FALSE;
			}
			break;
		case TRANS_DOWNLOAD_FILE_DATA:
			ret = Transc_DLFile(params, size);
			break;
		case TRANS_SET_NANDPARAM:
			ret = Transc_SetNandParam(params, size);
			break;
		case TRANS_SET_NANDFLASH_CHIP_SEL:
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
			ret = Transc_SetNandChipCount(params, size);
#else
			ret = AK_TRUE;
#endif			
			break;
		case TRANS_GET_CHANNEL_ID:
			ret = Transc_GetChannelId(params, size);
			break;
		case TRANS_SET_GPIO:
//#ifdef CONFIG_AK98
			ret = Transc_LedSignal(params, size);
//#endif
			break;
		case TRANS_SET_BURNEDPARAM:
//#ifdef CONFIG_AK98
			ret = Transc_BurnOverParam(params, size);
//#endif
			break;			
		case TRANS_READ_ASA_FILE:
			ret = Transc_ReadASAFile(params, size);
			break;
		case TRANS_WRITE_ASA_FILE:
			ret = Transc_WriteASAFile(params, size);
			break;

		case TRANS_SET_SPIPARAM:
			ret = Transc_SetSPIParam(params, size);
			break;
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
		case TRANS_SET_ERASE_NAND_MODE:
			ret = Transc_SetEraseNandMode();
			break;
#endif
		case TRANS_SET_BIN_RESV_SIZE:
			ret = Trans_SetBinResvSize(params, size);
			break;
		case TRANS_UPLOAD_BOOT_START:
			ret = Transc_UploadBootStart(params, size);
			break;
		case TRANS_UPLOAD_BOOT_DATA:
			if(FHA_SUCCESS == FHA_read_boot(bufbin, size))
			{
				if(write(fd, bufbin, size) != size)
				{
					printf("write bin data error\n");
					ret = 0;	
				}
				else
				{
					printf("write bin data success\n");
					ret = 1;	
				}
			}
			else
			{	
				ret = 0;
			}
			break;
		case TRANS_UPLOAD_BOOT_LEN:
			tmpLen = bootParam.data_length;
			if(write(fd, &tmpLen, sizeof(tmpLen)) != sizeof(tmpLen))
			{
				printf("write boot data len error\n");
				ret = 0;	
			}
			else
			{
				printf("write boot data len success\n");
				ret = 1;	
			}
			break;
		case TRANS_UPLOAD_BIN_START:
			ret = Transc_UploadBinStart(params, size);
			break;
		case TRANS_UPLOAD_BIN_DATA:
			if(FHA_SUCCESS == FHA_read_bin(bufbin, size))
			{
				if(write(fd, bufbin, size) != size)
				{
					printf("write bin data error\n");
					ret = 0;	
				}
				else
				{
					printf("write bin data success\n");
					ret = 1;	
				}
			}
			else
			{	
				ret = 0;
			}

			break;
		case TRANS_UPLOAD_BIN_LEN:
			tmpLen = bin_param.data_length;
			if(write(fd, &tmpLen, sizeof(tmpLen)) != sizeof(tmpLen))
			{
				printf("write bin data len error\n");
				ret = 0;	
			}
			else
			{
				printf("write bin data len success\n");
				ret = 1;	
			}
			break;
		case TRANS_UPLOAD_SPIDATA_START:
		{
			ret = Transc_UploadSPIDataStart(params, size);
		}
		break;
		case  TRANS_UPLOAD_SPIDATA_DATA:
		{
			if(FHA_SUCCESS == FHA_read_AllDatat(bufbin, size))
			{
				if(write(fd, bufbin, size) != size)
				{
					printf("write spi data error\n");
					ret = 0;	
				}
				else
				{
					printf("write spi data success\n");
					ret = 1;	
				}
			}
			else
			{	
				ret = 0;
			}		
		}
		
		break;
		default:
			ret = Transc_Default(params, size);
			break;
		}
		printf(".\n");
		if (AK_TRUE == ret) {
			if (ioctl(fd, AK_USBBURN_STATUS, 0) < 0) {
				printf("fatal error: ioctl stall failed");
				while (1)
					;
			}
			if (cmd_ret == 2) 
				BurnFailFun(0);
		} else {
			printf("\nERRORS!!!!\n");
			if (ioctl(fd, AK_USBBURN_STATUS, 1) < 0) {
				printf("fatal error: ioctl stall failed");
				while (1)
					;
			}
			//system("poweroff -f");
			//exit(1);
			BurnFailFun(0);
		}
		if (cmd == TRANS_CLOSE) {
			if (burned_param.burned_mode == BURNED_POWER_OFF)
				system("poweroff -f");
			else if (burned_param.burned_mode == BURNED_RESET)
				system("reboot");
			//exit(0);
		}
	} else {
		switch (cmd)
		{
		case ANYKA_UFI_CONFIRM_ID:
			if (ioctl(fd, AK_USBBURN_STALL, 0) < 0) {
				printf("fatal error: ioctl stall failed");
				while (1)
					;
			}
			if (write(fd, anyka_confirm_resp, 13) != 13) {
				printf("write anyka confirm data error");
			}
			if (ioctl(fd, AK_USBBURN_STATUS, 0) < 0) {
				printf("fatal error: ioctl stall failed");
				while (1)
					;
			}
			break;
		default:
			break;
		}
		printf(".\n");
	}
}

static void *alrm_detect(void *data)
{
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))

	signal(SIGALRM, BurnFailFun);
	alarm(BEGIN_DELAY);
	while (1) { 
		if (detect == 1) {
			alarm(0);
			alarm(DELAY);
			detect = 0;
		}
		sleep(1);
	}
#endif
}
	
int main()
{
	int length, ret;
	unsigned char buf[26] = "";
	/*init the callback */
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	callback.Erase = (FHA_Erase)nand_erase;
	callback.Write = (FHA_Write)nand_write;
	callback.Read = (FHA_Read)nand_read;
	callback.ReadNandBytes = (FHA_ReadNandBytes)nand_read_bytes;
#else
	callback.Erase = (FHA_Erase)SPIFlash_erase;
	callback.Write = (FHA_Write)SPIFlash_write;
	callback.Read = (FHA_Read)SPIFlash_read;
	callback.ReadNandBytes = (FHA_ReadNandBytes)SPIFlash_read_bytes;
#endif
	callback.RamAlloc = (FHA_RamAlloc)malloc;
	callback.RamFree = (FHA_RamFree)free;
	callback.MemSet = (FHA_MemSet)memset;
	callback.MemCpy = (FHA_MemCpy)memcpy;
	callback.MemCmp = (FHA_MemCmp)memcmp;
	callback.Printf = (FHA_Printf)printf;
	
	/* init the init_info */
	init_info.nBlockStep = 1;
#ifdef CONFIG_AK98
	init_info.eAKChip = FHA_CHIP_980X;
#endif
#ifdef CONFIG_AK39XX
	init_info.eAKChip = FHA_CHIP_39XX;
#else
	init_info.eAKChip = FHA_CHIP_37XX;
#endif
	init_info.ePlatform = PLAT_LINUX;

#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	init_info.eMedium = MEDIUM_NAND;
#else
	init_info.eMedium = MEDIUM_SPIFLASH;
	init_info.nChipCnt = 1;
	init_info.eMode = MODE_NEWBURN;
	spi_info.BinPageStart = SPI_FLASH_BIN_PAGE_START; 	// 544; //32;
	spi_info.PageSize = 256;
	/* FIXME: This is one big bug if SPI flash don`t support
	 * 4k sector erase. but is is safe enough now */
	spi_info.PagesPerBlock = 16;
#endif

	/* open the usb gadget buffer */
	fd = open("/dev/akudc_usbburn", O_RDWR); 
	if (fd < 0) {
		perror("cannot open\n");
		exit(1);
	}

#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	/* open the nandflash char device */
	if (mc_interface_init() == FHA_FAIL) {
#else
	if (mtd_interface_init() == FHA_FAIL) {
#endif
		exit(1);
	}

	ret = pthread_create(&alrm_ntid, NULL, alrm_detect, NULL);
	if (ret == 0) {
		printf("create alarm pthread success!\n");
	} else {
		printf("create alarm pthread errror %d\n", ret);
		exit(1);
	}
	
	while (1)
	{
		/* read the command including its params and the data length */
		if (read(fd, buf, 24) != 24) {
			printf("fatal error: ioctl stall failed");
			while (1)
				;
		}
		detect = 1;
		length = buf[20] | (buf[21] << 8) | (buf[22] << 16) | (buf[23] << 24);
		buf[16] = '\0';
		/* execute the command */
		usb_anyka_command(buf, length);
	}
	close(fd);
	
#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
	mc_destroy();
#else
	mtd_destroy();
#endif

	return 0;
}

