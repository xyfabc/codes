#ifndef        _NANDDATAGET_H_
#define        _NANDDATAGET_H_

/************************************************************************
 * NAME:     fha_GetBinStart
 * FUNCTION  set read bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 fha_GetBinStart(T_PFHA_BIN_PARAM bin_param);

/************************************************************************
 * NAME:     fha_GetBinData
 * FUNCTION  read bin to buf from medium
 * PARAM:    [out]pData-----need to read bin data buf pointer addr
 *           [in] data_len--need to read bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 fha_GetBinData(T_U8* pBuf, T_U32 data_len);
T_U32 fha_GetFSPart(T_U8 *pInfoBuf, T_U32 buf_len);
T_U32 fha_GetNandPara(T_NAND_PHY_INFO *pNandPhyInfo);
T_U32 fha_GetResvZoneInfo(T_U16 *start_block, T_U16 *block_cnt);
T_U32 fha_GetMaplist(T_U8 file_name[], T_U16 *map_data, T_U32 *file_len, T_BOOL bBackup);
T_U32 fha_GetBinNum(T_U32 *cnt);
T_U32 fha_GetNandLibVerion(T_LIB_VER_INFO *lib_info);
T_U32 fha_Get_fs_last_pos(T_VOID);
T_U32 fha_get_close(T_VOID);

/************************************************************************
 * NAME:     fha_get_read_bin_begin
 * FUNCTION  set read bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 fha_get_read_bin_begin(T_PFHA_BIN_PARAM bin_param);

/************************************************************************
 * NAME:     fha_get_read_bin
 * FUNCTION  read bin to buf from medium
 * PARAM:    [out]pData-----need to read bin data buf pointer addr
 *           [in] data_len--need to read bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 fha_get_read_bin(T_U8 *pData,  T_U32 data_len);
T_U32 fha_get_read_bin_num(T_U32 *cnt);
T_BOOL fha_CallbackInit(T_PFHA_INIT_INFO pInit, T_PFHA_LIB_CALLBACK pCB);
T_U32  fha_get_burn_fs_part(T_U8 *pInfoBuf, T_U32 buf_len);
T_U32  fha_get_nandboot_begin(T_PFHA_BIN_PARAM bin_param);
T_U32  fha_get_nandboot(T_U8 *pData,  T_U32 data_len);

#endif    //_FHA_NANDDATAGET_H_
