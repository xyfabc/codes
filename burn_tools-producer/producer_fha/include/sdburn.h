#ifndef _SD_BURN_H_
#define _SD_BURN_H_
#include "fha.h"

T_U32 SDBurn_Init();
T_U32 SDBurn_BinStartOne(T_PFHA_BIN_PARAM pBinFile);
T_U32 SDBurn_WriteBinData(const T_U8 *pData, T_U32 data_len);
T_U32 SDBurn_BurnBootStart(T_U32 boot_len);
T_U32 SDBurn_SetResvSize(T_U32  nSize, T_BOOL bErase);
T_U32 SDBurn_SetFSPart(const T_U8 *pInfoBuf, T_U32 buf_len);
T_U32 SDBurn_WriteConfig(T_VOID);
T_U32 SDBurn_get_last_pos(T_VOID);

T_U32 SDBurn_GetBinStart(T_PFHA_BIN_PARAM bin_param);
T_U32 SDBurn_GetBinData(T_U8* pBuf, T_U32 data_len);
T_U32 SDBurn_GetFSPart(T_U8 *pInfoBuf, T_U32 buf_len);
T_U32 SDBurn_GetBinNum(T_U32 *cnt);
T_U32 SDBurn_GetResvZoneInfo(T_U16 *start_block, T_U16 *block_cnt);
T_U32 SDBurn_WriteLibVerion(T_LIB_VER_INFO *lib_info,  T_U32 lib_cnt);
T_U32 SD_GetLibVerion(T_LIB_VER_INFO *lib_info);

/************************************************************************
 * NAME:     SDBurn_GetSDLastPosPara
 * FUNCTION  get fs start position
 * PARAM:    NULL
 * RETURN:   success return fs start block, fail retuen 0
**************************************************************************/
T_U32 SDBurn_GetSDLastPosPara(T_VOID);

/************************************************************************
 * NAME:     SDBurn_set_bin_resv_size
 * FUNCTION  set write bin reserve size(unit byte)
 * PARAM:    [in] bin_size--reserve bin size
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 SDBurn_set_bin_resv_size(T_U32 bin_size);

/************************************************************************
 * NAME:     SDBurn_asa_write_file
 * FUNCTION  write important infomation to security area, data_len can't too large
 * PARAM:    [in] file_name -- asa file name
 *           [in] pData ------ buffer used to store information data
 *           [in] data_len --- need to store data length
 *           [in] mode-------- operation mode 
 *                             ASA_MODE_OPEN------open  
 *                             ASA_MODE_CREATE----create
 * RETURN:   success return ASA_FILE_SUCCESS, fail retuen ASA_FILE_FAIL
**************************************************************************/
T_U32 SDBurn_asa_write_file(T_U8 file_name[], const T_U8 dataBuf[], T_U32 dataLen, T_U8 mode);

/************************************************************************
 * NAME:     SDBurn_asa_read_file
 * FUNCTION  get infomation from security area by input file name
 * PARAM:    [in] file_name -- asa file name
 *           [out]pData ------ buffer used to store information data
 *           [in] data_len --- need to get data length
 * RETURN:   success return ASA_FILE_SUCCESS, fail retuen ASA_FILE_FAIL
**************************************************************************/
T_U32 SDBurn_asa_read_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen);
T_U32 SDBurn_GetBootStart(T_PFHA_BIN_PARAM bin_param);
T_U32 SDBurn_GetBootData(T_U8* pBuf, T_U32 data_len);


#endif

