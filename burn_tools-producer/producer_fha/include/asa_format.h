#ifndef        _ASA_FORMAT_H_
#define        _ASA_FORMAT_H_
/** @file
 * @brief Define the register operator for system
 *
 * Copyright (C) 2006 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @author 
 * @date 2006-01-16
 * @version 1.0
 */

#define HEAD_SIZE 8               //asa head info size

#define ASA_BLOCK_COUNT 10       //define asa block max count
#define ASA_BUFFER_SIZE 8192

#define ASA_MAX_BLOCK_TRY 50    //define asa block max try use
     
//general
#define SA_SUCCESS 0
#define SA_ERROR 2

typedef struct {
 T_U8 head_str[8];
 T_U32 verify[2];
 T_U32 item_num;
 T_U32 info_end;
}
T_ASA_HEAD;

typedef struct
{
    T_U16 page_start;
    T_U16 page_count;
    T_U16 info_start;
    T_U16 info_len;
 }T_ASA_ITEM;

typedef struct
{
    T_U8 file_name[8];
    T_U32 file_length;
    T_U32 start_page;
    T_U32 end_page;
}T_ASA_FILE_INFO;

typedef struct tag_ASA_Param
{
    T_U16   PagePerBlock;       
    T_U16   BytesPerPage;
    T_U16   BlockNum;           //blocks of one chip 
}T_ASA_PARAM;

typedef struct tag_ASA_Block
{
    T_U8  asa_blocks[ASA_BLOCK_COUNT];   //保存安全区块表的数组
    T_U8  asa_count;                     //初始化以后用于安全区的block个数，仅指安全区可用块
    T_U8  asa_head;                      //安全区块数组中数据最新的块的索引 
    T_U8  asa_block_cnt;                 //安全区所有块个数，包含坏块
    T_U32 write_time;                    //块被擦写次数
}T_ASA_BLOCK;

typedef enum
{
    NAND_TYPE_UNKNOWN,
    NAND_TYPE_SAMSUNG,
    NAND_TYPE_HYNIX,
    NAND_TYPE_TOSHIBA,
    NAND_TYPE_TOSHIBA_EXT,
    NAND_TYPE_MICRON,
    NAND_TYPE_ST,
    NAND_TYPE_MICRON_4K,
    NAND_TYPE_MIRA
}E_NAND_TYPE;

extern T_ASA_PARAM m_fha_asa_param; //
extern T_ASA_BLOCK m_fha_asa_block; //

#endif   //_FHA_ASA_FORMAT_H_
