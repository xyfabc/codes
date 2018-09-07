/**
 * @filename mtd_char_interface.c
 * @brief AK880x download tool used
 * Copyright (C) 2010 Anyka (Guangzhou) Software Technology Co., LTD
 * @author zhangzheng
 * @modify
 * @date 
 * @version 1.0
 * @ref Please refer to¡­
 */
 
#include <fcntl.h>
#include <stdio.h>
#include "fha.h"
#include "nand_char_interface.h"

#define AK_NAND_PHY_ERASE               0xa0 //erase command
#define AK_NAND_PHY_READ                0xa1 // read command
#define AK_NAND_PHY_WRITE               0xa2 // write command
#define AK_NAND_GET_CHIP_ID             0xa3 // get nand id command
#define AK_MOUNT_MTD_PART               0xa4 // mount command
#define AK_NAND_READ_BYTES              0xa5 // read without ecc command
#define AK_GET_NAND_PARA                0xa6 // set nand param command
#define AK_SET_NAND_CE                	0xad // set nand chips info command


#define AK_NAND_CHAR_NODE               "/dev/nand_char"

/* erase block info */
struct erase_para
{
    T_U32 chip_num; //which chip the block is in
    T_U32 startpage; // the block's first page number
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
    T_U32 *nand_id;
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
  * open the nand char 
  */
T_U32 mc_interface_init(void)
{
	file = AK_NAND_CHAR_NODE;
	printf("zz mc_interface_init() file name=%s\n", file);
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
T_U32 mc_destroy(void)
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

/* 
  * erase the block
  * @PARAM    [chip_num] chip num 
  * @PARAM    [startpage]  page num
  */ 
T_U32 nand_erase(T_U32 chip_num,  T_U32 startpage)
{
	struct erase_para ep;

	if (init_info.nChipCnt > 1)
	{
		ep.chip_num = startpage / chip_page_num;
		ep.startpage = startpage % chip_page_num;
	}
	else
	{
		ep.chip_num = chip_num;
		ep.startpage = startpage;
	}
	
	if(fd)
	{
		printf("e");
		if (0 != ioctl(fd, AK_NAND_PHY_ERASE, &ep))
		{
			printf("nand_erase startpage:%d failed!\n", startpage);
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
T_U32 nand_write(T_U32 chip_num, T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType)
{	
    int ret = 0;
	struct rw_para wp;

	if (init_info.nChipCnt > 1) 
	{
		wp.chip_num = page_num / chip_page_num;
		wp.page_num = page_num % chip_page_num;
	}
	else
	{
		wp.chip_num = chip_num;
		wp.page_num = page_num;
	}

	wp.data = data;
	wp.data_len = data_len;
	wp.oob = oob;
	wp.oob_len = oob_len;
	wp.eDataType = eDataType;
	
	if(fd)
	{
		ret = ioctl(fd, AK_NAND_PHY_WRITE, &wp);
		if (ret != 0)
		{
           printf("ioctl AK_NAND_PHY_WRITE ret err!\n");
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
  * read the page
  * @PARAM    [chip_num] chip num 
  * @PARAM    [page_num]  page num
  * @PARAM    [data]  the buffer to store data
  * @PARAM    [data_len]  the buffer length
  * @PARAM    [eDataType]  data type
  */ 
T_U32 nand_read(T_U32 chip_num,	T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType)
{
    int ret = 0;
	struct rw_para rp;

	if (init_info.nChipCnt > 1) 
	{
		rp.chip_num = page_num / chip_page_num;
		rp.page_num = page_num % chip_page_num;
	}
	else
	{
		rp.chip_num = chip_num;
		rp.page_num = page_num;
	}

	rp.data = data;
	rp.data_len = data_len;
	rp.oob = oob;
	rp.oob_len = oob_len;
	rp.eDataType = eDataType;

	if(fd)
	{
		ret = ioctl(fd, AK_NAND_PHY_READ, &rp);
		if (ret != 0)
		{
           printf("ioctl AK_NAND_PHY_READ ret err!\n");
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
T_U32 nand_read_bytes(T_U32 chip_num, T_U32 row_addr, T_U32 clm_addr, T_U8 *data, T_U32 data_len)
{
	struct rbytes_para rbp;
	rbp.chip_num = chip_num;
	rbp.row_addr = row_addr;
	rbp.clm_addr = clm_addr;
	rbp.data     = data;
	rbp.data_len = data_len;
	
	if(fd)
	{	
		ioctl(fd, AK_NAND_READ_BYTES, &rbp);
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}
	return FHA_SUCCESS;
}

/* 
  * get the nand id 
  */
T_U32 get_nand_id(T_U32 chip_num)
{
	T_U32 id = 0;
	int ret = 0;
	struct get_id_para gip;
	gip.chip_num = chip_num;
	gip.nand_id = &id;

	if(fd)
	{
		printf("get_nand_id\n");
		ret = ioctl(fd, AK_NAND_GET_CHIP_ID, &gip);
		printf(".");
		if (ret != 0)
		{
           printf("ioctl AK_NAND_GET_CHIP_ID ret err!\n");
           return FHA_FAIL;
		}
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}
	return *gip.nand_id;
}

/*
  * if we need mount nand partition, we must call it first 
  * @PARAM    [chip_nr] the total number of chips 
  */
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

/*
  * set nand param for burn
  */
T_U32 send_nand_para(T_NAND_PHY_INFO *npi)
{
    int ret =0;
    
	if(fd)
	{
		printf("send_nand_para\n");
		ret = ioctl(fd, AK_GET_NAND_PARA, npi);
		printf(".");
		if (ret != 0)
		{
           printf("ioctl AK_GET_NAND_PARA ret err!\n");
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
  * set the nand chip and chip select info
  * @PARAM    [chip_num] the total number of chips
  * @PARAM    [ce2_gpio] the gpio number of chip select 2
  * @PARAM    [ce3_gpio] the gpio number of chip select 3
  */
T_U32 set_nand_ce(T_U32 chip_num, T_U32 ce2_gpio, T_U32 ce3_gpio)
{
	int ret = 0;
	struct ce_para ce;

	ce.chip_nr = chip_num;
	ce.ce2_gpio = ce2_gpio;
	ce.ce3_gpio = ce3_gpio;
	
	if (fd)
	{
		ret = ioctl(fd, AK_SET_NAND_CE, &ce);
		if (ret == 0 || ret == -1)
		{
			printf("set ce parameter failed\n");
			return FHA_FAIL;
		}
		else
		{
			init_info.nChipCnt = ret;
		}
	}
	else
	{
		printf("Open %s failed!\n", file);
		return FHA_FAIL;
	}

	return FHA_SUCCESS;
}
