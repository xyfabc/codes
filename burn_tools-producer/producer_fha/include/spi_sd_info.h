#ifndef _SD_SPI_INFO_H_
#define _SD_SPI_INFO_H_
#include "fha.h"

#define SD_PART_TABLE_SECTOR      0
#define SD_DISK_INFO_SECTOR       1
#define SD_STRUCT_INFO_SECTOR     2

#ifdef PROTECT_SD //±£»¤sd
#define SD_BOOT_DATA_START        3 
#define SD_BIN_INFO_SECTOR        3
#define SD_BIN_INFO_SECTOR_CNT    1
#define SD_DATA_START_SECTOR      16
#else
#define SD_BOOT_DATA_START        3 
#define SD_BIN_INFO_SECTOR        80
#define SD_BIN_INFO_SECTOR_CNT    1
#define SD_DATA_START_SECTOR      128
#endif

#define SD_LIB_VER_SECTOR         127
#define SD_USER_INFO_SECTOR_START 88
#define SD_USER_INFO_SECTOR_END   126
#define SD_SECTOR_SIZE            512
 
typedef struct
{
    T_U32 file_length;          //file data length
    T_U32 ld_addr;              //link address
    T_U32 start_sector;         //origin data start sector
    T_U32 backup_sector;        //backup data start sector
    T_U8  file_name[16];        //name
}T_SD_FILE_INFO;

typedef struct
{
    T_U32 file_cnt;             //count of bin file
    T_U32 data_start;           //reserve zone start sector, bin data start sector if resv_size is zero
    T_U32 resv_size;            //size of reserve zone
    T_U32 bin_info_pos;         //start sector of bin file info
    T_U32 bin_end_pos;          //end sector of bin file data 
}T_SD_STRUCT_INFO;   

typedef struct
{
    T_U32 file_length;
    T_U32 ld_addr;
    T_U32 start_page;
    T_U32 backup_page;        //backup data start page
    T_U8  file_name[16];
}T_SPI_FILE_CONFIG;

#endif//_SD_SPI_INFO_H_  

