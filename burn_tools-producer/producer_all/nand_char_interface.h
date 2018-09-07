/**
 * @filename mtd_char_interface.h
 * @brief AK880x download tool used
 * Copyright (C) 2010 Anyka (Guangzhou) Software Technology Co., LTD
 * @author zhangzheng
 * @modify
 * @date 
 * @version 1.0
 * @ref Please refer toÂ¡Â­
 */

#ifndef _NAND_CHAR_INTERFACE_H_
#define _NAND_CHAR_INTERFACE_H_

#define MTD_WRITEABLE		0x400	/* Device is writeable */
#define MTD_BIT_WRITEABLE	0x800	/* Single bits can be flipped */
#define MTD_NO_ERASE		0x1000	/* No erase necessary */
#define MTD_POWERUP_LOCK	0x2000	/* Always locked after reset */

#define MTDPART_OFS_NXTBLK	(-2)
#define MTDPART_OFS_APPEND	(-1)
#define MTDPART_SIZ_FULL	(0)

#define MTD_PART_NAME_LEN   (4)

#define SZ_1K               0x00000400
#define SZ_4K               0x00001000
#define SZ_8K               0x00002000
#define SZ_16K              0x00004000
#define SZ_32K				0x00008000
#define SZ_64K              0x00010000
#define SZ_128K             0x00020000
#define SZ_256K             0x00040000
#define SZ_512K             0x00080000

#define SZ_1M               0x00100000
#define SZ_2M               0x00200000
#define SZ_4M               0x00400000
#define SZ_8M               0x00800000
#define SZ_16M              0x01000000
#define SZ_26M				0x01a00000
#define SZ_32M              0x02000000
#define SZ_64M              0x04000000
#define SZ_128M             0x08000000
#define SZ_256M             0x10000000
#define SZ_512M             0x20000000

#define SZ_1G               0x40000000
#define SZ_2G               0x80000000

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
	unsigned int Size;
	unsigned int EnlargeSize;         // set this value if enlarge capacity,otherwise set 0 
    unsigned int HideStartBlock;      //hide disk start
    unsigned int FSType;
    unsigned int resv[1];				
}__attribute__((packed)) T_PARTION_INFO;



T_U32 mc_interface_init(void);
T_U32 mc_destroy(void);
T_U32 nand_erase(T_U32 chip_num,  T_U32 startpage);
T_U32 nand_read(T_U32 chip_num,	T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType);
T_U32 nand_write(T_U32 chip_num, T_U32 page_num, T_U8 *data, T_U32 data_len,  T_U8 *oob, T_U32 oob_len, E_FHA_DATA_TYPE eDataType);
T_U32 nand_read_bytes(T_U32 chip_num, T_U32 row_addr, T_U32 clm_addr, T_U8 *data, T_U32 data_len);
T_U32 get_nand_id(T_U32 chip_num);
T_U32 mount_mtd_partitions(unsigned long chip_nr);
T_U32 send_nand_para(T_NAND_PHY_INFO *npi);
T_U32 set_nand_ce(T_U32 chip_num, T_U32 ce2_gpio, T_U32 ce3_gpio);


#endif
