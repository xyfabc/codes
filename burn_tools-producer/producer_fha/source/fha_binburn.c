/**
 * @FILENAME: fha_binburn.c
 * @BRIEF xx
 * Copyright (C) 2011 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR luqiliu
 * @DATE 2009-11-19
 * @VERSION 1.0
 * @modified lu 2011-11-03
 * 
 * @REF
 */

#include "nand_info.h"
#include "sdburn.h"
#include "spiburn.h"
#include "nandburn.h"
#include "nanddataget.h"
#include "fha_asa.h"

typedef T_U32  (*FHA_WRITE_BIN_BEGIN)(T_PFHA_BIN_PARAM bin_param);
typedef T_U32  (*FHA_WRITE_BIN)(const T_U8 *pData,  T_U32 data_len);
typedef T_U32  (*FHA_WRITE_BOOT_BEGIN)(T_U32 bin_len);
typedef T_U32  (*FHA_WRITE_BOOT)(const T_U8 *pData,  T_U32 data_len);
typedef T_U32  (*FHA_SET_RESV_ZONE_INFO)(T_U32  nSize, T_BOOL bErase);
typedef T_U32  (*FHA_SET_FS_PART)(const T_U8 *pInfoBuf, T_U32 buf_len);
typedef T_U32  (*FHA_SET_LIB_VERSION)(T_LIB_VER_INFO *lib_info, T_U32 lib_cnt);
typedef T_U32  (*FHA_CLOSE)(T_VOID);
typedef T_U32  (*FHA_GET_LAST_POS)(T_VOID);
typedef T_U32  (*FHA_GET_FS_PART)(T_U8 *pInfoBuf, T_U32 buf_len);
typedef T_U32  (*FHA_SET_BIN_RESV_SIZE)(T_U32 bin_size);
typedef T_U32  (*FHA_READ_BIN_BEGIN)(T_PFHA_BIN_PARAM bin_param);
typedef T_U32  (*FHA_READ_BIN)(T_U8 *pData,  T_U32 data_len);
typedef T_U32  (*FHA_GET_BIN_NUM)(T_U32 *cnt);
typedef T_U32  (*FHA_GET_BOOT_BEGIN)(T_PFHA_BIN_PARAM bin_param);
typedef T_U32  (*FHA_GET_BOOT)(T_U8 *pData,  T_U32 data_len);


typedef struct
{
    FHA_WRITE_BIN_BEGIN     write_begin;
    FHA_WRITE_BIN           write_bin;
    FHA_READ_BIN_BEGIN      read_begin;
    FHA_READ_BIN            read_bin;
    FHA_GET_BIN_NUM         bin_num;    
    FHA_WRITE_BOOT_BEGIN    boot_begin;
    FHA_WRITE_BOOT          write_boot;
    FHA_SET_RESV_ZONE_INFO  set_resv;
    FHA_SET_FS_PART         set_fs;
    FHA_GET_FS_PART         get_fs;
    FHA_SET_LIB_VERSION     set_ver;
    FHA_CLOSE               close;
    FHA_GET_LAST_POS        get_last;
    FHA_SET_BIN_RESV_SIZE   set_bin_size;
    FHA_GET_BOOT_BEGIN      get_boot_begin;
    FHA_GET_BOOT            get_boot;
}T_FHA_BURN_FUNC, *T_PFHA_BURN_FUNC;   

T_PFHA_BURN_FUNC m_fha_burn_func = AK_NULL;
#ifdef SUPPORT_NAND
extern T_U32 asa_write_file(T_U8 file_name[], const T_U8 dataBuf[], T_U32 dataLen, T_U8 mode);
#endif

#ifdef SUPPORT_SD
#ifdef SUPPORT_SPIFLASH

/************************************************************************
 * NAME:     fha_write_SD_SPI_bin_begin
 * FUNCTION  set write bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_write_SD_SPI_bin_begin(T_PFHA_BIN_PARAM bin_param)
{
    if (FHA_FAIL == SDBurn_BinStartOne(bin_param))
        return FHA_FAIL;
    
    return Sflash_BurnBinStart(bin_param);
}

/************************************************************************
 * NAME:     fha_write_SD_SPI_bin
 * FUNCTION  write bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_write_SD_SPI_bin(const T_U8 * pData,  T_U32 data_len)
{
    if (FHA_FAIL == SDBurn_WriteBinData(pData, data_len))
        return FHA_FAIL;
    
    return Sflash_BurnWriteData(pData, data_len);    
}

/************************************************************************
 * NAME:     FHA_set_fs_part
 * FUNCTION  set fs partition info to medium 
 * PARAM:    [in] pInfoBuf-----need to write fs info data pointer addr
 *           [in] data_len--need to write fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_set_SD_SPI_fs_part(const T_U8 *pInfoBuf , T_U32 buf_len)
{
    if (FHA_FAIL == SDBurn_SetFSPart(pInfoBuf, buf_len))
        return FHA_FAIL;
    
    return Sflash_SetFSPart(pInfoBuf, buf_len);    
}

/************************************************************************
 * NAME:     FHA_close
 * FUNCTION  flush all need to save data to medium, and free all fha malloc ram
 * PARAM:    NULL
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_SD_SPI_close(T_VOID)
{
    if (FHA_FAIL == SDBurn_WriteConfig())
        return FHA_FAIL;
    
    return Sflash_BurnWriteConfig();    
}
#endif
#endif

/************************************************************************
 * NAME:     FHA_burn_init
 * FUNCTION  Initial FHA callback function, init fha init info para
 * PARAM:    [in] pInit----burn platform init struct pointer
 *           [in] pCB------callback function struct pointer
 *           [in] pPhyInfo-input medium struct pointer
 *                         NAND-------T_NAND_PHY_INFO*
 *                         SPIFLASH---T_PSPI_INIT_INFO
 *                         EMMC-------AK_NULL
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_burn_init(T_PFHA_INIT_INFO pInit, T_PFHA_LIB_CALLBACK pCB, T_pVOID pPhyInfo)
{ 
    T_U32 ret = FHA_SUCCESS;
    
    if (AK_NULL == pInit || AK_NULL == pCB 
        || ((MEDIUM_NAND == pInit->eMedium || MEDIUM_SPIFLASH == pInit->eMedium) && AK_NULL == pPhyInfo))
    {
        //gFHAf.Printf("PR_NB FHA_burn_init para is NULL\r\n");
        return FHA_FAIL;
    }
    
    if (!fha_CallbackInit(pInit, pCB))
        return FHA_FAIL;

    if (MODE_UPDATE_SELF == pInit->eMode)
    {
        gFHAf.Printf("FHA_S not support MODE_UPDATE_SELF (burn init)\r\n");
        return FHA_FAIL;
    }
    
#ifdef SUPPORT_NAND
    if (MEDIUM_NAND == pInit->eMedium)
    {
        ret =  fha_burn_nand_init(pInit, pCB, (T_NAND_PHY_INFO *)pPhyInfo);        
        if (FHA_FAIL == ret)
        {
            return FHA_FAIL;
        }
    }
#endif    

#ifdef SUPPORT_SPIFLASH
    if (MEDIUM_SPIFLASH == pInit->eMedium 
        || MEDIUM_SPI_EMMC == pInit->eMedium
        || MEDIUM_EMMC_SPIBOOT == pInit->eMedium)
    {
        ret = Sflash_BurnInit(pPhyInfo);
        if (FHA_FAIL == ret)
        {
            return FHA_FAIL;
        }
    }
#endif    

#ifdef SUPPORT_SD
    if (MEDIUM_EMMC == pInit->eMedium 
        || MEDIUM_SPI_EMMC == pInit->eMedium
        || MEDIUM_EMMC_SPIBOOT == pInit->eMedium)
    {
        ret = SDBurn_Init();
        if (FHA_FAIL == ret)
        {
            return FHA_FAIL;
        }
    }    
#endif

    m_fha_burn_func = gFHAf.RamAlloc(sizeof(T_FHA_BURN_FUNC));

    if (AK_NULL == m_fha_burn_func)
    {
        return FHA_FAIL;
    }

    gFHAf.MemSet(m_fha_burn_func, 0, sizeof(T_FHA_BURN_FUNC));
    
    switch (g_burn_param.eMedium)
    {
        case MEDIUM_NAND:
#ifdef SUPPORT_NAND            
            m_fha_burn_func->write_begin    = fha_write_nandbin_begin; 
            m_fha_burn_func->write_bin      = fha_write_nandbin;
            m_fha_burn_func->read_begin     = fha_GetBinStart; 
            m_fha_burn_func->read_bin       = fha_GetBinData;
            m_fha_burn_func->bin_num        = fha_GetBinNum;
            m_fha_burn_func->boot_begin     = fha_write_nandboot_begin; 
            m_fha_burn_func->write_boot     = fha_write_nandboot;
            m_fha_burn_func->close          = fha_nand_burn_close;
            m_fha_burn_func->set_resv       = fha_set_nand_resv_zone_info;
            m_fha_burn_func->set_fs         = fha_set_nand_fs_part;
            m_fha_burn_func->get_fs         = fha_GetFSPart;
            m_fha_burn_func->set_ver        = fha_WriteNandLibVerion;
            m_fha_burn_func->get_last       = fha_get_nand_last_pos;
            m_fha_burn_func->set_bin_size   = fha_set_bin_resv_size;
            m_fha_burn_func->get_boot_begin = fha_get_nandboot_begin;
            m_fha_burn_func->get_boot       = fha_get_nandboot;
            
#endif                
            break;
        case MEDIUM_SPIFLASH:
#ifdef SUPPORT_SPIFLASH
            m_fha_burn_func->write_begin  = Sflash_BurnBinStart; 
            m_fha_burn_func->write_bin    = Sflash_BurnWriteData;
            m_fha_burn_func->read_begin   = Sflash_GetBinStart; 
            m_fha_burn_func->read_bin     = Sflash_GetBinData;
            m_fha_burn_func->bin_num      = Sflash_GetBinNum;
            m_fha_burn_func->boot_begin   = Sflash_BurnBootStart; 
            m_fha_burn_func->write_boot   = Sflash_BurnWriteData;
            m_fha_burn_func->close        = Sflash_BurnWriteConfig;
            m_fha_burn_func->set_resv     = AK_NULL;
            m_fha_burn_func->set_fs       = Sflash_SetFSPart;
            m_fha_burn_func->get_fs       = Sflash_GetFSPart;
            m_fha_burn_func->set_ver      = AK_NULL;
            m_fha_burn_func->get_last     = Sflash_get_nand_last_pos;
            m_fha_burn_func->get_boot_begin = Sflash_GetBootStart;
            m_fha_burn_func->get_boot       = Sflash_GetBootData;
            m_fha_burn_func->set_bin_size   = Sflash_set_bin_resv_size;

#endif 
            break;
        case MEDIUM_EMMC:
#ifdef SUPPORT_SD
            m_fha_burn_func->write_begin  = SDBurn_BinStartOne; 
            m_fha_burn_func->write_bin    = SDBurn_WriteBinData;
            m_fha_burn_func->read_begin   = SDBurn_GetBinStart; 
            m_fha_burn_func->read_bin     = SDBurn_GetBinData;
            m_fha_burn_func->bin_num      = SDBurn_GetBinNum;
            m_fha_burn_func->boot_begin   = SDBurn_BurnBootStart; 
            m_fha_burn_func->write_boot   = SDBurn_WriteBinData;
            m_fha_burn_func->close        = SDBurn_WriteConfig;
            m_fha_burn_func->set_resv     = SDBurn_SetResvSize;
            m_fha_burn_func->set_fs       = SDBurn_SetFSPart;
            m_fha_burn_func->get_fs       = SDBurn_GetFSPart;
            m_fha_burn_func->set_ver      = SDBurn_WriteLibVerion;
            m_fha_burn_func->get_last     = SDBurn_get_last_pos;
            m_fha_burn_func->set_bin_size = SDBurn_set_bin_resv_size;
            m_fha_burn_func->get_boot_begin = SDBurn_GetBootStart;
            m_fha_burn_func->get_boot       = SDBurn_GetBootData;
#endif
            break;
        case MEDIUM_SPI_EMMC:      
        case MEDIUM_EMMC_SPIBOOT: 
#ifdef SUPPORT_SD
#ifdef SUPPORT_SPIFLASH            
            m_fha_burn_func->write_begin  = fha_write_SD_SPI_bin_begin; 
            m_fha_burn_func->write_bin    = fha_write_SD_SPI_bin;
            m_fha_burn_func->close        = fha_SD_SPI_close;
            m_fha_burn_func->set_fs       = fha_set_SD_SPI_fs_part;
#endif
#endif

#ifdef SUPPORT_SD                
            m_fha_burn_func->set_resv     = SDBurn_SetResvSize;
            m_fha_burn_func->set_ver      = SDBurn_WriteLibVerion;
            m_fha_burn_func->get_last     = SDBurn_get_last_pos;
            m_fha_burn_func->get_fs       = SDBurn_GetFSPart;
            m_fha_burn_func->set_bin_size = SDBurn_set_bin_resv_size;
#endif

#ifdef SUPPORT_SPIFLASH
            m_fha_burn_func->boot_begin   = Sflash_BurnBootStart; 
            m_fha_burn_func->write_boot   = Sflash_BurnWriteData;
#endif                
            break;

        }       


    return ret;
}

/************************************************************************
 * NAME:     FHA_set_bin_resv_size
 * FUNCTION  set write bin reserve size(unit byte)
 * PARAM:    [in] bin_size--reserve bin size
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_set_bin_resv_size(T_U32 bin_size)
{
    T_U32 ret = FHA_SUCCESS;

    if (AK_NULL != m_fha_burn_func->set_bin_size)
    {
        ret = m_fha_burn_func->set_bin_size(bin_size);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
    
}

/************************************************************************
 * NAME:     FHA_write_bin_begin
 * FUNCTION  set write bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_write_bin_begin(T_PFHA_BIN_PARAM bin_param)
{
    T_U32 ret = FHA_SUCCESS;
    
    gFHAf.Printf("FHA_B file len:%d\r\n",  bin_param->data_length);
    gFHAf.Printf("FHA_B file name:%s\r\n", bin_param->file_name);
    gFHAf.Printf("FHA_B ld addr:0x%x\r\n", bin_param->ld_addr);
    gFHAf.Printf("FHA_B backup:%d, check:%d, updateself:%d\r\n", 
                bin_param->bBackup, bin_param->bCheck, bin_param->bUpdateSelf);

    if (fha_str_len(bin_param->file_name) > (sizeof(bin_param->file_name) -1))
    {
        gFHAf.Printf("FHA_B file name overlong!\r\n");
        return FHA_FAIL;
    }

    if (AK_NULL != m_fha_burn_func->write_begin)
    {
        ret = m_fha_burn_func->write_begin(bin_param);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
}

/************************************************************************
 * NAME:     FHA_write_bin
 * FUNCTION  write bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_write_bin(const T_U8 * pData,  T_U32 data_len)
{
    T_U32  ret = FHA_SUCCESS;

    if (AK_NULL != m_fha_burn_func->write_bin)
    {
        ret = m_fha_burn_func->write_bin(pData, data_len);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
}

/************************************************************************
 * NAME:     FHA_get_last_pos
 * FUNCTION  get fs start position
 * PARAM:    NULL
 * RETURN:   success return fs start block, fail retuen 0
**************************************************************************/

T_U32  FHA_get_last_pos(T_VOID)
{
    T_U32 ret = FHA_SUCCESS;

    if (AK_NULL != m_fha_burn_func && AK_NULL != m_fha_burn_func->get_last)
    {
        ret = m_fha_burn_func->get_last();
    }    
    else
    {
        #ifdef SUPPORT_NAND 
        return fha_Get_fs_last_pos();
        #endif        
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
}

/************************************************************************
 * NAME:     FHA_write_boot_begin
 * FUNCTION  set write boot start init para
 * PARAM:    [in] bin_len--boot length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_write_boot_begin(T_U32 bin_len)
{
    T_U32 ret = FHA_SUCCESS;

    if (AK_NULL != m_fha_burn_func->boot_begin)
    {
        ret = m_fha_burn_func->boot_begin(bin_len);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
}    

/************************************************************************
 * NAME:     FHA_write_boot
 * FUNCTION  write boot to medium
 * PARAM:    [in] pData-----need to write boot data pointer addr
 *           [in] data_len--need to write boot data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/


T_U32  FHA_write_boot(const T_U8 *pData,  T_U32 data_len)
{
    T_U32 ret = FHA_SUCCESS;

    if (AK_NULL != m_fha_burn_func->write_boot)
    {
        ret = m_fha_burn_func->write_boot(pData, data_len);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
}    

/************************************************************************
 * NAME:     FHA_set_fs_part
 * FUNCTION  set fs partition info to medium 
 * PARAM:    [in] pInfoBuf-----need to write fs info data pointer addr
 *           [in] data_len--need to write fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_set_fs_part(const T_U8 *pInfoBuf , T_U32 buf_len)
{
    T_U32 ret = FHA_SUCCESS;

    if (AK_NULL != m_fha_burn_func->set_fs)
    {
        ret = m_fha_burn_func->set_fs(pInfoBuf, buf_len);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
}

/************************************************************************
 * NAME:     FHA_set_resv_zone_info
 * FUNCTION  set reserve area
 * PARAM:    [in] nSize--set reserve area size(unit Mbyte)
 *           [in] bErase-if == 1 ,erase reserve area, or else not erase 
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 FHA_set_resv_zone_info(T_U32  nSize, T_BOOL bErase)
{
    T_U32 ret = FHA_SUCCESS;

    if (AK_NULL != m_fha_burn_func->set_resv)
    {
        ret = m_fha_burn_func->set_resv(nSize, bErase);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;

}    

/************************************************************************
 * NAME:     FHA_close
 * FUNCTION  flush all need to save data to medium, and free all fha malloc ram
 * PARAM:    NULL
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_close(T_VOID)
{
    T_U32 ret = FHA_SUCCESS;
 
    if (AK_NULL != m_fha_burn_func && AK_NULL != m_fha_burn_func->close)
    {
        ret = m_fha_burn_func->close();
        gFHAf.RamFree(m_fha_burn_func);
        m_fha_burn_func = AK_NULL;
    }    
    else
    {
        ret = fha_get_close();
    } 
 
    return ret;
}

/************************************************************************
 * NAME:     FHA_set_lib_version
 * FUNCTION  set burn all lib version
 * PARAM:    [in] lib_info--all lib version struct pointer addr
 *           [in] lib_cnt---lib count
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_set_lib_version(T_LIB_VER_INFO *lib_info,  T_U32 lib_cnt)
{
    T_U32 ret = FHA_SUCCESS;
    
    if (AK_NULL != m_fha_burn_func->set_ver)
    {
        ret = m_fha_burn_func->set_ver(lib_info, lib_cnt);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 

    return ret;
}   

/************************************************************************
 * NAME:     fha_get_burn_fs_part
 * FUNCTION  get fs partition info
 * PARAM:    [out]pInfoBuf---need to get fs info data pointer addr
 *           [in] data_len---need to get fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_get_burn_fs_part(T_U8 *pInfoBuf, T_U32 buf_len)
{
    if (AK_NULL == pInfoBuf)
    {
        return FHA_FAIL;
    }

    if (AK_NULL != m_fha_burn_func->get_fs)
    {
        return m_fha_burn_func->get_fs(pInfoBuf, buf_len);
    } 
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        return FHA_FAIL;
    }  
}


  
  /************************************************************************
   * NAME:     FHA_asa_write_file
   * FUNCTION  write important infomation to security area, data_len can't too large
   * PARAM:    [in] file_name -- asa file name
   *           [in] pData ------ buffer used to store information data
   *           [in] data_len --- need to store data length
   *           [in] mode-------- operation mode 
   *                             ASA_MODE_OPEN------open  
   *                             ASA_MODE_CREATE----create
   * RETURN:   success return ASA_FILE_SUCCESS, fail retuen ASA_FILE_FAIL
  **************************************************************************/
T_U32  FHA_asa_write_file(T_U8 file_name[], const T_U8 *pData, T_U32 data_len, T_U8 mode)
{
#ifdef SUPPORT_NAND
  if (MEDIUM_NAND == g_burn_param.eMedium)
  {
      return asa_write_file(file_name, pData, data_len, mode);
  }
#endif

#ifdef SUPPORT_SD
  if (MEDIUM_EMMC == g_burn_param.eMedium
      || MEDIUM_SPI_EMMC == g_burn_param.eMedium
      || MEDIUM_EMMC_SPIBOOT == g_burn_param.eMedium)
  {
      return SDBurn_asa_write_file(file_name, pData, data_len, mode);
  }
#endif


#ifdef SUPPORT_SPIFLASH
    if (MEDIUM_SPIFLASH == g_burn_param.eMedium)
    {
         return  SPIBurn_asa_write_file(file_name, pData, data_len, mode);
    }
#endif

  return ASA_FILE_FAIL;
}



T_U32  FHA_read_boot_begin(T_PFHA_BIN_PARAM bin_param)
{
    if (AK_NULL == bin_param)
    {
        return FHA_FAIL;
    }
    
    if (m_fha_burn_func != AK_NULL && m_fha_burn_func->read_begin != AK_NULL)
    {
        
        return m_fha_burn_func->get_boot_begin(bin_param);
    }
    else
    {
        gFHAf.Printf("#FHA not support FHA_read_boot_begin#\r\n");
        return FHA_FAIL;
    }
}

T_U32  FHA_read_boot(T_U8 *pData,  T_U32 data_len)
{
    if (AK_NULL == pData)
    {
        return FHA_FAIL;
    }

    if (m_fha_burn_func != AK_NULL && m_fha_burn_func->read_bin != AK_NULL)
    {
        return m_fha_burn_func->get_boot(pData, data_len);
    }
    else
    {
        gFHAf.Printf("#FHA not support FHA_read_boot#\r\n");
        return FHA_FAIL;
    }

}



T_U32  FHA_read_bin_begin(T_PFHA_BIN_PARAM bin_param)
{
    if (AK_NULL == bin_param)
    {
        return FHA_FAIL;
    }

    if (m_fha_burn_func != AK_NULL && m_fha_burn_func->read_begin != AK_NULL)
    {
        return m_fha_burn_func->read_begin(bin_param);
    }    
    else
    {
        return fha_get_read_bin_begin(bin_param);
    } 
}

T_U32  FHA_read_bin(T_U8 *pData,  T_U32 data_len)
{
    if (AK_NULL == pData)
    {
        return FHA_FAIL;
    }

    if (m_fha_burn_func != AK_NULL && m_fha_burn_func->read_bin != AK_NULL)
    {
        return m_fha_burn_func->read_bin(pData, data_len);
    }    
    else
    {
        return fha_get_read_bin(pData, data_len);
    }   
}

T_U32  FHA_get_bin_num(T_U32 *cnt)
{
    if (AK_NULL == cnt)
    {
        return FHA_FAIL;
    } 
    
    if (m_fha_burn_func != AK_NULL && m_fha_burn_func->bin_num != AK_NULL)
    {
        return m_fha_burn_func->bin_num(cnt);
    }    
    else
    {
        return fha_get_read_bin_num(cnt);
    }      
}

#ifdef SUPPORT_SPIFLASH
/************************************************************************
 * NAME:     FHA_write_bin_begin
 * FUNCTION  set write bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  FHA_write_AllDatat_begin(T_PFHA_BIN_PARAM bin_param)
{
     return Sflash_BurnWrite_AllData_start(bin_param);
}

/************************************************************************
 * NAME:     FHA_write_bin
 * FUNCTION  write bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  FHA_write_AllDatat(const T_U8 * pData,  T_U32 data_len)
{
    return Sflash_BurnWrite_AllData(pData, data_len);
}

/************************************************************************
 * NAME:     FHA_write_bin_begin
 * FUNCTION  set write bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  FHA_write_MTD_begin(T_PFHA_BIN_PARAM bin_param)
{
     return Sflash_BurnWrite_MTD_start(bin_param);
}

/************************************************************************
 * NAME:     FHA_write_bin
 * FUNCTION  write bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  FHA_write_MTD(const T_U8 * pData,  T_U32 data_len)
{
    return Sflash_BurnWrite_MTD(pData, data_len);

}
/************************************************************************
 * NAME:     FHA_read_AllDatat_begin
 * FUNCTION  set read bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  FHA_read_AllDatat_begin(T_PFHA_BIN_PARAM bin_param)
{
    return Sflash_BurnRead_AllData_start(bin_param);
}

/************************************************************************
 * NAME:     FHA_read_AllDatat
 * FUNCTION  read bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32  FHA_read_AllDatat(const T_U8 * pData,  T_U32 data_len)
{
    return Sflash_BurnRead_AllData(pData, data_len);
}

#endif



