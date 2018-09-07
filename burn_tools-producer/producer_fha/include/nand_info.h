#ifndef        _GFHA_H_
#define        _GFHA_H_

#include "fha.h"


/*Version:library name VmainVersion.sub1Version.sub2Version, 
fha.a only detect the string before the last version separator(.)
which must is the same string, otherwise the version is not matching. 
So, you may modify the string of sub2Version of library version generally, 
if the modification is so fatal that the version is not compatible with your last verion, 
pls modify library name VmainVersion.sub1Version.
*/

#define     FHA_LIB_VER                 "fhalib V1.0.25"

#define     FS_PART_INFO_BACK_PAGE      1
#define     NAND_CONFIG_INFO_BACK_PAGE  2
#define     FILE_CONFIG_INFO_BACK_PAGE  3
#define     NAND_BOOT_PAGE_FIRST        472
#define     NAND_BOOT_PAGE_FIRST_10L    402   //snowbirdLоƬ


#define        BOOT_NAME            "BOOTBIN"


//bin nand struct info
typedef struct
{
    T_U32 bin_page_size;          //bin page size to write
    T_U32 page_per_block;         //pages per block
    T_U32 block_per_plane;        //blocks per plane
    T_U16 boot_page_size;         //boot page size to write
}T_BIN_NAND_INFO;

typedef struct
{
    T_U32 file_length;
    T_U32 ld_addr;
    T_U32 map_index;
    T_U32 run_pos;          //BIN run position
    T_U32 update_pos;       //BIN update position
    T_U8  file_name[16];
}T_FILE_CONFIG;

typedef struct tag_NandConfig{
    T_U8  Resv[4];
    T_U32 BinFileCount;
    T_U32 ResvStartBlock;
    T_U32 ResvBlockCount;
    T_U32 BootLength;
    T_U32 BinStartBlock;
    T_U32 BinEndBlock;
    T_U32 ASAStartBlock;
    T_U32 ASAEndBlock;
    T_U8  Resv_ex[28];
}T_NAND_CONFIG;

T_BOOL Prod_NandEraseBlock(T_U32 block_index);
T_BOOL Prod_NandWriteBinPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[]);
T_BOOL Prod_NandReadBinPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[]);
T_BOOL Prod_NandWriteASAPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[]);
T_BOOL Prod_NandReadASAPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[]);
T_BOOL Prod_NandWriteBootPage(T_U32 page, T_U8 data[]);
T_BOOL Prod_NandReadBootPage(T_U32 page, T_U8 data[]);
T_BOOL Prod_IsBadBlock(T_U32 block_index);

T_VOID fha_init_nand_param(T_U16 phypagesize, T_U16 pagesperblock, T_U16 blockperplane);
T_U32 fha_str_len(const char *string);
int fha_str_cmp(const char *s1, const char *s2);
T_VOID fha_set_asa_end_block(T_U32 blk_end);

T_U32 Prod_LB2PB(T_U32 LogBlock);
T_U32 Prod_PB2LB(T_U32 PhyBlock);

extern T_FHA_INIT_INFO    g_burn_param;
extern T_FHA_LIB_CALLBACK gFHAf;
extern T_BIN_NAND_INFO    m_bin_nand_info;
extern T_NAND_PHY_INFO    g_nand_phy_info;

#endif

