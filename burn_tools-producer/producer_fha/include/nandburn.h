#ifndef _NAND_BURN_H_
#define _NAND_BURN_H_

#define     NAND_BIN_COUNT             10
#define     BIN_MAX_BLOCK_COUNT        1000 
#define     BIN_RESV_FLAG              0xFFFFDDDD

#define     MAP_INFO_FLAG               0x12121212
#define     BIN_FILE_INFO_FLAG          0x34343434
#define     NAND_CONFIG_FLAG            0x56565656
#define     ZONE_INFO_FLAG              0x5A5A5A5A

T_U32  fha_burn_nand_init(T_PFHA_INIT_INFO pInit, T_PFHA_LIB_CALLBACK pCB, T_NAND_PHY_INFO *pNandPhyInfo);

/************************************************************************
 * NAME:     fha_write_nandbin_begin
 * FUNCTION  set write bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  fha_write_nandbin_begin(T_PFHA_BIN_PARAM bin_param);
/************************************************************************
 * NAME:     fha_write_nandbin
 * FUNCTION  write bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  fha_write_nandbin(const T_U8 * pData,  T_U32 data_len);

/************************************************************************
 * NAME:     FHA_write_boot_begin
 * FUNCTION  set write boot start init para
 * PARAM:    [in] bin_len--boot length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  fha_write_nandboot_begin(T_U32 bin_len);

/************************************************************************
 * NAME:     FHA_write_boot
 * FUNCTION  write boot to medium
 * PARAM:    [in] pData-----need to write boot data pointer addr
 *           [in] data_len--need to write boot data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  fha_write_nandboot(const T_U8 *pData,  T_U32 data_len);
T_U32  fha_nand_burn_close(T_VOID);
T_U32  fha_set_nand_resv_zone_info(T_U32  nSize, T_BOOL bErase);
T_U32  fha_set_nand_fs_part(const T_U8 *pInfoBuf , T_U32 buf_len);
T_U32  fha_WriteNandLibVerion(T_LIB_VER_INFO *lib_info,  T_U32 lib_cnt);
T_U32  fha_get_nand_last_pos(T_VOID);

/************************************************************************
 * NAME:     fha_set_bin_resv_size
 * FUNCTION  set bin resvsize
 * PARAM:    [in] nSize-----bin resvsize
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 fha_set_bin_resv_size(T_U32 nSize);


#endif
