#ifndef _SFLASH_BURN_H_
#define _SFLASH_BURN_H_

#include "fha.h"

#define     FILE_MAX_NUM    9

T_U32 Sflash_BurnInit(T_pVOID SpiInfo);
T_U32 Sflash_BurnBootStart(T_U32 boot_len);
/************************************************************************
 * NAME:     Sflash_BurnBinStart
 * FUNCTION  set write bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_BurnBinStart(T_FHA_BIN_PARAM *pBinFile);

/************************************************************************
 * NAME:     Sflash_BurnWriteData
 * FUNCTION  write bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_BurnWriteData(const T_U8 *pData, T_U32 data_len);

T_U32 Sflash_SetFSPart(const T_U8 *pInfoBuf, T_U32 buf_len);
T_U32 Sflash_BurnWriteConfig(T_VOID);

T_U32 Sflash_BurnWritePage(const T_U8 *pData, T_U32 startPage, T_U32 PageCnt);
T_U32 Sflash_BurnReadPage(T_U8* buf, T_U32 startPage, T_U32 PageCnt);


/************************************************************************
 * NAME:     Sflash_GetBinStart
 * FUNCTION  set read bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_GetBinStart(T_PFHA_BIN_PARAM bin_param);

/************************************************************************
 * NAME:     Sflash_GetBinData
 * FUNCTION  read bin to buf from medium
 * PARAM:    [out]pData-----need to read bin data buf pointer addr
 *           [in] data_len--need to read bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_GetBinData(T_U8* pBuf, T_U32 data_len);
T_U32 Sflash_GetFSPart(T_U8 *pInfoBuf, T_U32 buf_len);
T_U32 Sflash_GetBinNum(T_U32 *cnt);
T_U32 Sflash_MountInit(T_pVOID SpiInfo);
T_U32 Sflash_GetBootStart(T_PFHA_BIN_PARAM bin_param);
T_U32 Sflash_GetBootData(T_U8* pBuf, T_U32 data_len);
T_U32  Sflash_get_nand_last_pos(T_VOID);

/************************************************************************
 * NAME:     Sflash_BurnWrite_AllData_start
 * FUNCTION  write all data into spiflash start
 * PARAM:    T_FHA_BIN_PARAM *pBinFile
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_BurnWrite_AllData_start(T_FHA_BIN_PARAM *pBinFile);
/************************************************************************
 * NAME:     Sflash_set_bin_resv_size
 * FUNCTION  set bin spare
 * PARAM: 
 *           [in] T_U32 bin_size-- size
 * RETURN:    return  bin_size
**************************************************************************/
T_U32 Sflash_set_bin_resv_size(T_U32 bin_size);

/************************************************************************
* NAME:     Sflash_BurnWrite_AllData
* FUNCTION  write all data into spiflash
* PARAM:    [out]pData-----need to read bin data buf pointer addr
*           [in] data_len--need to read bin data length
* RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_BurnWrite_AllData(const T_U8 *pData, T_U32 data_len);


/************************************************************************
 * NAME:     Sflash_BurnWrite_MTD_start
 * FUNCTION  write all data into spiflash start
 * PARAM:    T_FHA_BIN_PARAM *pBinFile
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_BurnWrite_MTD_start(T_FHA_BIN_PARAM *pBinFile);


/************************************************************************
* NAME:     Sflash_BurnWrite_MTD
* FUNCTION  write all data into spiflash
* PARAM:    [out]pData-----need to read bin data buf pointer addr
*           [in] data_len--need to read bin data length
* RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_BurnWrite_MTD(const T_U8 *pData, T_U32 data_len);


T_U32 SPIBurn_asa_write_file(T_U8 file_name[], const T_U8 dataBuf[], T_U32 dataLen, T_U8 mode);
T_U32 SPIBurn_asa_read_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen);


#endif

