/**
 * @filename mtd_char_interface.c
 * @brief AK880x download tool used
 * Copyright (C) 2010 Anyka (Guangzhou) Software Technology Co., LTD
 * @author She Shaohua
 * @modify
 * @date 
 * @version 1.0
 * @ref Please refer to¡­
 */
 
#include <fcntl.h>
#include <stdio.h>
#include "fha.h"
#include "spi_char_interface.h"

#define AK_SPIFLASH_PHY_ERASE               0x80
#define AK_SPIFLASH_PHY_READ                0x81
#define AK_SPIFLASH_PHY_WRITE               0x82
#define AK_SPIFLASH_GET_CHIP_ID             0x83
#define AK_MOUNT_MTD_PART		    0x84 

#define AK_MTD_CHAR_NODE               "/dev/mtd0"

static T_U32 erasesize = 0;

/* erase block info */
struct erase_para
{
    T_U32 chip_num; //which chip the block is in
    T_U32 startpage; // the block's first page number
	T_U32 erasesize;	
};

/* write page info */
struct rw_para
{
    T_U32 chip_num;
    T_U32 page_num;
    T_U8 *data;
    T_U32 data_len;
    T_U8 *oob;
    T_U32 oob_len;
	T_U32 eDataType;
};

struct rbytes_para
{
	T_U32 chip_num; 
	T_U32 row_addr; 
	T_U32 clm_addr; 
	T_U8 *data;
	T_U32 data_len;
};

/* the total number of chips and nand id info */
struct get_id_para
{
    T_U32 chip_num;
#if 0
    T_U32 *nand_id;
#else
	T_U32 *spiflash_id;
#endif
};

/* chip total count and select gpio info */ 
struct ce_para
{
        int chip_nr;
        int ce2_gpio;
        int ce3_gpio;
};

static int fd = 0;
static char *file;
extern T_FHA_INIT_INFO init_info;
extern unsigned long chip_page_num;

/* 
  * open the Spi flash 
  */
T_U32 mtd_interface_init(void)
{
	file = AK_MTD_CHAR_NODE;
	printf("mtd_interface_init() file name=%s\n", file);
	fd = open(file, O_RDWR);
	if(fd != -1)
	{
		printf("mtd char device interface initialize successed!\n");
		return FHA_SUCCESS;
	}
	else
	{
		printf("mtd char device interface initialize failed!\n");
		return FHA_FAIL;
	}
}

/* 
  * close the nand char 
  */
T_U32 mtd_destroy(void)
{
	if(fd)
	{
		close(fd);
		return FHA_SUCCESS;
	}
	else
	{
		return FHA_FAIL;
	}
}


T_U32 SPIFlash_SetEraseSize(T_U32 es)
{
	erasesize = es;
	return 0;
}


/* 
  * erase special sector of spi flash.
  * @PARAM    [chip_num] chip num 
  * @PARAM    [startpage]  page num
  */ 
T_U32 SPIFlash_erase(T_U32 chip_num,  T_U32 startpage)
{
	struct erase_para ep;

	if (init_info.nChipCnt > 1)
	{
		printf("%s, ERROR: SPI Flash doesn't support multi-chip select.\n", __func__);
		return FHA_FAIL;
	}
	else
	{
		ep.chip_num = chip_num;
		ep.startpage = startpage;
		ep.erasesize = erasesize;
	}
	
	if(fd)
	{
		printf("e");
		if (0 != ioctl(fd, AK_SPIFLASH_PHY_ERASE, &ep))
		{
			printf("SPIFlash_erase startpage:%d failed!\n", startpage);
			return  FHA_FAIL;
		}
		printf(".");
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}
	return FHA_SUCCESS;
}


/* 
  * write the page
  * @PARAM    [chip_num] chip num 
  * @PARAM    [page_num]  page num
  * @PARAM    [data]  the buffer with data to write
  * @PARAM    [data_len]  the buffer length
  * @PARAM    [eDataType]  data type
  */ 
T_U32 SPIFlash_write(T_U32 chip_num, T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType)
{	
    int ret = 0;
	struct rw_para wp;

	if (init_info.nChipCnt > 1) 
	{
		printf("%s, ERROR: SPI Flash doesn't support multi-chip select.\n", __func__);
		return FHA_FAIL;
	}
	else
	{
		wp.chip_num = chip_num;
		wp.page_num = page_num;
	}

	wp.data = data;
	wp.data_len = data_len;
	wp.eDataType = eDataType;
	
	if(fd)
	{
		ret = ioctl(fd, AK_SPIFLASH_PHY_WRITE, &wp);
		if (ret != 0)
		{
           printf("ioctl AK_SPIFLASH_PHY_WRITE ret err!\n");
           return FHA_FAIL;
		}
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}
	return FHA_SUCCESS;

}



/* 
  * read the page of SPI Flash.
  * @PARAM    [chip_num] chip num 
  * @PARAM    [page_num]  page num
  * @PARAM    [data]  the buffer to store data
  * @PARAM    [data_len]  the buffer length
  * @PARAM    [eDataType]  data type
  */ 
T_U32 SPIFlash_read(T_U32 chip_num,	T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType)
{
    int ret = 0;
	struct rw_para rp;

	if (init_info.nChipCnt > 1) 
	{
		printf("ERROR: SPI Flash doesn't support multi-chip select.\n");
		return FHA_FAIL;
	}
	else
	{
		rp.chip_num = chip_num;
		rp.page_num = page_num;
	}

	rp.data = data;
	rp.data_len = data_len;
	rp.eDataType = eDataType;

	if(fd)
	{
		ret = ioctl(fd, AK_SPIFLASH_PHY_READ, &rp);
		if (ret != 0)
		{
           printf("ioctl AK_SPIFLASH_PHY_READ ret err!\n");
           return FHA_FAIL;
		}
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}
	return FHA_SUCCESS;

}


/* 
  * read data_len bytes in the specified addr
  * @PARAM    [chip_num] chip num 
  * @PARAM    [row_addr]  row addr
  * @PARAM    [clm_addr]  column addr
  * @PARAM    [data]  the buffer to store data
  * @PARAM    [data_len]  the buffer length
  */ 
T_U32 SPIFlash_read_bytes(T_U32 chip_num, T_U32 row_addr, T_U32 clm_addr, T_U8 *data, T_U32 data_len)
{
	struct rbytes_para rbp;
	rbp.chip_num = chip_num;
	rbp.row_addr = row_addr;
	rbp.clm_addr = clm_addr;
	rbp.data     = data;
	rbp.data_len = data_len;

	while(1)
	{
		printf("SPIFlash_read_bytes not be support.\n");
	}
	
	/*if(fd)
	{	
		ioctl(fd, AK_NAND_READ_BYTES, &rbp);
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}*/
	return FHA_SUCCESS;
}


/* 
  * get the SPI Flash id 
  */
T_U32 SPIFlash_get_id(T_U32 chip_num)
{
	T_U32 id = 0;
	int ret = 0;
	struct get_id_para gip;
	gip.chip_num = chip_num;
	gip.spiflash_id = &id;

	if(fd)
	{
		printf("SPIFlash_get_id\n");
		ret = ioctl(fd, AK_SPIFLASH_GET_CHIP_ID, &gip);
		printf(".");
		if (ret != 0)
		{
           printf("ioctl AK_SPIFLASH_GET_CHIP_ID ret err!\n");
           return FHA_FAIL;
		}
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}
	return *gip.spiflash_id;
}

T_U32 mount_mtd_partitions(unsigned long chip_nr)
{
    int ret= 0;
	unsigned long chip_cnt = chip_nr;
    
	if(fd)
	{
		printf("mount_mtd_partitions");
		ret = ioctl(fd, AK_MOUNT_MTD_PART, &chip_cnt);
		printf(".");
		if (ret != 0)
		{
           printf("ioctl AK_MOUNT_MTD_PART ret err!\n");
           return FHA_FAIL;
		}
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}
	return FHA_SUCCESS;
}

