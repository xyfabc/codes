/**
 * @filename mtd_char_interface.h
 * @brief AK880x download tool used
 * Copyright (C) 2010 Anyka (Guangzhou) Software Technology Co., LTD
 * @author She shaohua
 * @modify
 * @date 
 * @version 1.0
 * @ref Please refer toÂ¡Â­
 */

#ifndef _SPI_CHAR_INTERFACE_H_
#define _SPI_CHAR_INTERFACE_H_

#define MTD_WRITEABLE		0x400	/* Device is writeable */
#define MTD_BIT_WRITEABLE	0x800	/* Single bits can be flipped */
#define MTD_NO_ERASE		0x1000	/* No erase necessary */
#define MTD_POWERUP_LOCK	0x2000	/* Always locked after reset */

#define MTDPART_OFS_NXTBLK	(-2)
#define MTDPART_OFS_APPEND	(-1)
#define MTDPART_SIZ_FULL	(0)

#define MTD_PART_NAME_LEN   (4)

struct partitions
{
	char name[MTD_PART_NAME_LEN]; 
	unsigned long long size;			           
	unsigned long long offset;
	unsigned int mask_flags;
}__attribute__((packed));

typedef struct
{
	char Disk_Name;				//ÅÌ·ûÃû
    char bOpenZone;				//
    char ProtectType;			//	
    char ZoneType;				//
	float Size;
	unsigned int EnlargeSize;         // set this value if enlarge capacity,otherwise set 0 
    unsigned int HideStartBlock;      //hide disk start
    unsigned int FSType;
    unsigned int resv[1];				
}__attribute__((packed)) T_PARTION_INFO;



T_U32 mtd_interface_init(void);
T_U32 mtd_destroy(void);
T_U32 SPIFlash_SetEraseSize(T_U32 es);
T_U32 SPIFlash_erase(T_U32 chip_num,  T_U32 startpage);
T_U32 SPIFlash_write(T_U32 chip_num, T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType);
T_U32 SPIFlash_read(T_U32 chip_num,	T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType);
T_U32 SPIFlash_read_bytes(T_U32 chip_num, T_U32 row_addr, T_U32 clm_addr, T_U8 *data, T_U32 data_len);


#endif
