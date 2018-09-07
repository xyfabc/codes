/**
 * @FILENAME: fha_dataget.c
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
#include "nanddataget.h"
#include "spiburn.h"
#include "fha_asa.h"

typedef T_U32  (*FHA_READ_BIN_BEGIN)(T_PFHA_BIN_PARAM bin_param);
typedef T_U32  (*FHA_READ_BIN)(T_U8 *pData,  T_U32 data_len);
typedef T_U32  (*FHA_GET_BIN_NUM)(T_U32 *cnt);
typedef T_U32  (*FHA_GET_RESV_ZONE_INFO)(T_U16 *start_block, T_U16 *block_cnt);
typedef T_U32  (*FHA_GET_FS_PART)(T_U8 *pInfoBuf, T_U32 buf_len);
typedef T_U32  (*FHA_GET_LIB_VERSION)(T_LIB_VER_INFO *lib_info);
typedef T_U32  (*FHA_GET_CLOSE)(T_VOID);
typedef T_U32  (*FHA_GET_WRITE_BIN_BEGIN)(T_PFHA_BIN_PARAM bin_param);
typedef T_U32  (*FHA_GET_WRITE_BIN)(const T_U8 *pData,  T_U32 data_len);

typedef struct
{
    FHA_GET_WRITE_BIN_BEGIN write_begin;
    FHA_GET_WRITE_BIN       write_bin;    
    FHA_READ_BIN_BEGIN  read_begin;
    FHA_READ_BIN        read_bin;
    FHA_GET_BIN_NUM     bin_num;
    FHA_GET_RESV_ZONE_INFO get_resv;
    FHA_GET_FS_PART     get_fs;
    FHA_GET_LIB_VERSION get_ver;
    FHA_GET_CLOSE       get_close;
}T_FHA_GET_FUNC, *T_PFHA_GET_FUNC;   

static T_PFHA_GET_FUNC m_fha_get_func = AK_NULL;

T_FHA_LIB_CALLBACK  gFHAf = {0};
T_FHA_INIT_INFO     g_burn_param = {0};

static T_U32  get_close_null(T_VOID);
#ifdef SUPPORT_NAND
extern T_U32 asa_read_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen);
#endif
T_U32 FHA_check_lib_version(T_LIB_VER_INFO *lib_info)
{
    T_LIB_VER_INFO get_lib_info;
    T_U32 lib_len, i, dot_num = 0;

    if (AK_NULL == lib_info)
    {
        return FHA_FAIL;
    }  

    lib_len = fha_str_len(lib_info->lib_version);

    if ((0 == lib_len) || (lib_len > 40))
    {
        return FHA_FAIL;
    }    

    gFHAf.MemCpy(get_lib_info.lib_name, lib_info->lib_name, sizeof(lib_info->lib_name));

    if (FHA_SUCCESS != FHA_get_lib_verison(&get_lib_info))
    {
        gFHAf.Printf("FHA get LIB %s fail\n", lib_info->lib_name);
        return FHA_FAIL;
    }  

    for (i=0; i<lib_len; i++)
    {
        if (lib_info->lib_version[i] != get_lib_info.lib_version[i])
        {
            gFHAf.Printf("lib version:%s, read:%s\n", lib_info->lib_version, get_lib_info.lib_version);
            return FHA_FAIL;
        } 

        if ('.' == lib_info->lib_version[i])
        {
            dot_num++;
            if (2 == dot_num)
            {
                /*防止前十个命名不标准为".x", 而不是".0x"*/
                if (lib_info->lib_version[i+2] != 0 && get_lib_info.lib_version[i+2] == 0)
                {
                    /*这个情况两位数的大于一位数的，认为是正确的*/
                    return FHA_SUCCESS;
                }
                
                if (lib_info->lib_version[i+2] == 0 && get_lib_info.lib_version[i+2] != 0)
                {
                    /*这个情况两位数的小于一位数的，认为是错误的*/
                    return FHA_FAIL;
                }

                if (lib_info->lib_version[i+1] > get_lib_info.lib_version[i+1])
                {
                    return FHA_SUCCESS;
                }
                else if (lib_info->lib_version[i+1] < get_lib_info.lib_version[i+1])
                {
                    return FHA_FAIL;
                }
                else if (lib_info->lib_version[i+2] < get_lib_info.lib_version[i+2])
                {
                    return FHA_FAIL;
                }
                else
                {
                    return FHA_SUCCESS;
                }
                
            }
        }
    } 
    
    return FHA_SUCCESS;
} 


T_U8 *FHA_get_version(T_VOID)
{
    return FHA_LIB_VER;
}

int fha_str_cmp(const char *s1, const char *s2)
{
    int i;

    for(i=0; s1[i] && s2[i]; i++)
    {
        if (s1[i] != s2[i])
        {
            return -1;
        }
    }

    if (s1[i] != s2[i])
        return -1;
    else
        return 0;
}

T_U32 fha_str_len(const char *string)
{
    T_U32 i = 0;

    while (string[i] != 0)
    {
        i++;
        if (i>10000)
            break;
    }
    return i;
} 

/**
 * @BREIF    Initial burn callback function
 * @AUTHOR   Lu_Qiliu
 * @DATE     2010-10-13
 * @PARAM    [in] callback function struct pointer
 * @RETURN   T_VOID
 */
T_BOOL fha_CallbackInit(T_PFHA_INIT_INFO pInit, T_PFHA_LIB_CALLBACK pCB)
{
    if (AK_NULL == pInit || AK_NULL == pCB) 
    {
        return AK_FALSE;
    }
    
    gFHAf.Erase = pCB->Erase;
    gFHAf.Write = pCB->Write;
    gFHAf.Read  = pCB->Read;
    gFHAf.ReadNandBytes = pCB->ReadNandBytes;
    gFHAf.RamAlloc = pCB->RamAlloc;
    gFHAf.RamFree  = pCB->RamFree;
    gFHAf.MemSet   = pCB->MemSet;        
    gFHAf.MemCpy   = pCB->MemCpy;
    gFHAf.MemCmp   = pCB->MemCmp;
    gFHAf.Printf   = pCB->Printf;

    g_burn_param.nChipCnt   = pInit->nChipCnt;
    g_burn_param.nBlockStep = pInit->nBlockStep;
    g_burn_param.eAKChip    = pInit->eAKChip;
    g_burn_param.ePlatform  = pInit->ePlatform;
    g_burn_param.eMedium    = pInit->eMedium;
    g_burn_param.eMode      = pInit->eMode;

    if (AK_NULL == gFHAf.Write
        || AK_NULL == gFHAf.Read
        ||(MEDIUM_NAND == pInit->eMedium && (AK_NULL == gFHAf.ReadNandBytes || AK_NULL == gFHAf.Erase))
        || AK_NULL == gFHAf.RamAlloc
        || AK_NULL == gFHAf.RamFree
        || AK_NULL == gFHAf.MemSet
        || AK_NULL == gFHAf.MemCpy
        || AK_NULL == gFHAf.MemCmp
        || AK_NULL == gFHAf.Printf)
    {
        if (AK_NULL == gFHAf.Printf)
        {
            //printf("FHA_CallbackInit(): some callback function not init!\r\n");
        }
        else
        {
            gFHAf.Printf("FHA_CallbackInit(): some callback function not init!\r\n");
        }
        
        return AK_FALSE;
    }

    gFHAf.Printf("%s\r\n", FHA_LIB_VER);

    return AK_TRUE;
}
/************************************************************************
 * NAME:     FHA_mount
 * FUNCTION  Initial FHA mount callback function, init fha mount init info para
 * PARAM:    [in] pInit----burn platform init struct pointer
 *           [in] pCB------callback function struct pointer
 *           [in] pPhyInfo-input medium struct pointer
 *                         NAND-------T_NAND_PHY_INFO*, linux platform == AK_NULL
 *                         SPIFLASH---T_PSPI_INIT_INFO
 *                         EMMC-------AK_NULL
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/


T_U32 FHA_mount(T_PFHA_INIT_INFO pInit, T_PFHA_LIB_CALLBACK pCB, T_pVOID pPhyInfo)
{
    if (!fha_CallbackInit(pInit, pCB))
        return FHA_FAIL;

    if (MODE_UPDATE_SELF == pInit->eMode
        && g_burn_param.eMedium != MEDIUM_SPIFLASH)
    {
        gFHAf.Printf("FHA_S not support MODE_UPDATE_SELF(mount init), eMedium must equal to MEDIUM_SPIFLASH\r\n");
        return FHA_FAIL;
    }

    m_fha_get_func = pCB->RamAlloc(sizeof(T_FHA_GET_FUNC));

    if (AK_NULL == m_fha_get_func)
    {
        return FHA_FAIL;
    } 

    gFHAf.MemSet(m_fha_get_func, 0, sizeof(T_FHA_GET_FUNC));
    m_fha_get_func->get_close = get_close_null;
    
    switch (g_burn_param.eMedium)
    {
        case MEDIUM_NAND:
#ifdef SUPPORT_NAND            
            m_fha_get_func->read_begin   = fha_GetBinStart; 
            m_fha_get_func->read_bin     = fha_GetBinData;
            m_fha_get_func->bin_num      = fha_GetBinNum;
            m_fha_get_func->get_resv     = fha_GetResvZoneInfo;
            m_fha_get_func->get_fs       = fha_GetFSPart;
            m_fha_get_func->get_ver      = fha_GetNandLibVerion;
#endif                
            break;
        case MEDIUM_SPIFLASH:
        case MEDIUM_SPI_EMMC:  
#ifdef SUPPORT_SPIFLASH
            m_fha_get_func->write_begin   = Sflash_BurnBinStart; 
            m_fha_get_func->write_bin     = Sflash_BurnWriteData;
            m_fha_get_func->read_begin   = Sflash_GetBinStart; 
            m_fha_get_func->read_bin     = Sflash_GetBinData;
            m_fha_get_func->bin_num      = Sflash_GetBinNum;
            m_fha_get_func->get_resv     = AK_NULL;
            m_fha_get_func->get_fs       = Sflash_GetFSPart;
            m_fha_get_func->get_ver      = AK_NULL;;
#endif 
            break;
        case MEDIUM_EMMC:
        case MEDIUM_EMMC_SPIBOOT: 
#ifdef SUPPORT_SD
            m_fha_get_func->read_begin   = SDBurn_GetBinStart; 
            m_fha_get_func->read_bin     = SDBurn_GetBinData;
            m_fha_get_func->bin_num      = SDBurn_GetBinNum;
            m_fha_get_func->get_resv     = SDBurn_GetResvZoneInfo;
            m_fha_get_func->get_fs       = SDBurn_GetFSPart;
            m_fha_get_func->get_ver      = SD_GetLibVerion;
#endif
            break;
    }       

#ifdef SUPPORT_NAND
    if (MEDIUM_NAND == g_burn_param.eMedium)
    {
	    if ((PLAT_LINUX == g_burn_param.ePlatform) || (AK_NULL == pPhyInfo))
	    {
	        if (FHA_SUCCESS != fha_GetNandPara(&g_nand_phy_info))
	        {
	            return FHA_FAIL;
	        }
	    }
	    else
	    {
	        gFHAf.MemCpy(&g_nand_phy_info, pPhyInfo, sizeof(g_nand_phy_info));
	    }    

	    fha_init_nand_param(g_nand_phy_info.page_size, g_nand_phy_info.page_per_blk, g_nand_phy_info.plane_blk_num);
    }
#endif 

#ifdef SUPPORT_SPIFLASH
    if (MEDIUM_SPIFLASH == g_burn_param.eMedium
		|| MEDIUM_SPI_EMMC == g_burn_param.eMedium
		|| MEDIUM_EMMC_SPIBOOT == g_burn_param.eMedium)
    {   
	    if (FHA_SUCCESS != Sflash_MountInit(pPhyInfo))
	    {
	        return FHA_FAIL;
	    }
    }
#endif

    return FHA_SUCCESS;
}

#ifdef SUPPORT_NAND

/************************************************************************
 * NAME:     FHA_get_nand_para
 * FUNCTION  get nand para
 * PARAM:    [out] pNandPhyInfo--nand info struct pointer
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_get_nand_para(T_NAND_PHY_INFO *pNandPhyInfo)
{ 
     if (AK_NULL == pNandPhyInfo)
        return FHA_FAIL;

     gFHAf.MemCpy(pNandPhyInfo, &g_nand_phy_info, sizeof(g_nand_phy_info));
     
     return FHA_SUCCESS;
}

/************************************************************************
 * NAME:     FHA_get_maplist
 * FUNCTION  get block address map of bin. (only nand)
 * PARAM:    [in] file_name---need to get bin's file name
 *           [out]map_data----need to get bin's block map buf pointer addr
 *           [out]file_len----need to get bin's file length 
 *           [in] bBackup-----if AK_TRUE == bBackup, get backup block map, or else get origin block map
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 FHA_get_maplist(T_U8 file_name[], T_U16 *map_data, T_U32 *file_len, T_BOOL bBackup)
{
    return fha_GetMaplist(file_name, map_data, file_len, bBackup);
}    
#endif

/************************************************************************
 * NAME:     FHA_get_resv_zone_info
 * FUNCTION  get reserve area
 * PARAM:    [out] start_block--get reserve area start block
 *           [out] block_cnt----get reserve area block count
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 FHA_get_resv_zone_info(T_U16 *start_block, T_U16 *block_cnt)
{
    if (AK_NULL == start_block || AK_NULL == block_cnt)
    {
        return FHA_FAIL;
    }

    if (AK_NULL != m_fha_get_func->get_resv)
    {
        return m_fha_get_func->get_resv(start_block, block_cnt);
    }
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        return FHA_FAIL;
    }    
}
/************************************************************************
 * NAME:     FHA_read_bin_begin
 * FUNCTION  set read bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_get_read_bin_begin(T_PFHA_BIN_PARAM bin_param)
{
    if (AK_NULL == bin_param)
    {
        return FHA_FAIL;
    }

    if (AK_NULL != m_fha_get_func->read_begin)
    {
        return m_fha_get_func->read_begin(bin_param);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        return FHA_FAIL;
    } 
}
/************************************************************************
 * NAME:     FHA_read_bin
 * FUNCTION  read bin to buf from medium
 * PARAM:    [out]pData-----need to read bin data buf pointer addr
 *           [in] data_len--need to read bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_get_read_bin(T_U8 *pData,  T_U32 data_len)
{
    if (AK_NULL == pData)
    {
        return FHA_FAIL;
    }

    if (AK_NULL != m_fha_get_func->read_bin)
    {
        return m_fha_get_func->read_bin(pData, data_len);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        return FHA_FAIL;
    }   
}
/************************************************************************
 * NAME:     FHA_get_fs_part
 * FUNCTION  get fs partition info
 * PARAM:    [out]pInfoBuf---need to get fs info data pointer addr
 *           [in] data_len---need to get fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_get_fs_part(T_U8 *pInfoBuf, T_U32 buf_len)
{
     if (AK_NULL == pInfoBuf)
    {
        return FHA_FAIL;
    }

    if ((AK_NULL != m_fha_get_func) && (AK_NULL != m_fha_get_func->get_fs))
    {
        return m_fha_get_func->get_fs(pInfoBuf, buf_len);
    }
    else
    {
        return fha_get_burn_fs_part(pInfoBuf, buf_len);
    }  
}

/************************************************************************
 * NAME:     FHA_get_bin_num
 * FUNCTION  get bin file number
 *           [out] cnt----bin file count in medium
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_get_read_bin_num(T_U32 *cnt)
{
    if (AK_NULL == cnt)
    {
        return FHA_FAIL;
    } 
    
    if (AK_NULL != m_fha_get_func->bin_num)
    {
        return m_fha_get_func->bin_num(cnt);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        return FHA_FAIL;
    }      
}

/************************************************************************
 * NAME:     FHA_get_lib_verison
 * FUNCTION  get burn lib version by input lib_info->lib_name
 * PARAM:    [in-out] lib_info--input lib_info->lib_name, output lib_info->lib_version
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_get_lib_verison(T_LIB_VER_INFO *lib_info)
{
    if (AK_NULL != m_fha_get_func->get_ver)
    {
        return m_fha_get_func->get_ver(lib_info);
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        return FHA_FAIL;
    }
}

/************************************************************************
 * NAME:     FHA_get_last_pos
 * FUNCTION  get fs start position
 * PARAM:    NULL
 * RETURN:   success return fs start block, fail retuen 0
**************************************************************************/

T_U32 fha_Get_fs_last_pos(T_VOID)
{
#ifdef SUPPORT_NAND
    if (MEDIUM_NAND == g_burn_param.eMedium)
    {
        T_U32 spare;
        T_NAND_CONFIG *pNandCfg = AK_NULL;
        T_U32 page_size;
        T_U32 pos = 0;
        T_U8* pbuf = AK_NULL;
        
        page_size = m_bin_nand_info.bin_page_size;
        
        pbuf = gFHAf.RamAlloc(page_size);
        if(AK_NULL == pbuf)
        {
            gFHAf.Printf("FHA_M G_F_L_P alloc fail\r\n");
            return 0;
        }

        if(!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - NAND_CONFIG_INFO_BACK_PAGE, pbuf, (T_U8 *)&spare))
        {
            gFHAf.RamFree(pbuf);
            return 0;
        } 

        pNandCfg = (T_NAND_CONFIG *)pbuf;

        pos = pNandCfg->BinEndBlock;

        gFHAf.RamFree(pbuf);
        
        return pos;
    }
#endif

    return 0;    
}

/************************************************************************
 * NAME:     FHA_asa_read_file
 * FUNCTION  get infomation from security area by input file name
 * PARAM:    [in] file_name -- asa file name
 *           [out]pData ------ buffer used to store information data
 *           [in] data_len --- need to get data length
 * RETURN:   success return ASA_FILE_SUCCESS, fail retuen ASA_FILE_FAIL
**************************************************************************/


T_U32  FHA_asa_read_file(T_U8 file_name[], T_U8 *pData, T_U32 data_len)
{
#ifdef SUPPORT_NAND
    if (MEDIUM_NAND == g_burn_param.eMedium)
    {
        return asa_read_file(file_name, pData, data_len);
    }
#endif

#ifdef SUPPORT_SD
    if (MEDIUM_EMMC == g_burn_param.eMedium
        || MEDIUM_SPI_EMMC == g_burn_param.eMedium
        || MEDIUM_EMMC_SPIBOOT == g_burn_param.eMedium)
    {
        return SDBurn_asa_read_file(file_name, pData, data_len);
    }
#endif

#ifdef SUPPORT_SPIFLASH
        if (MEDIUM_SPIFLASH == g_burn_param.eMedium)
        {
            return SPIBurn_asa_read_file(file_name, pData, data_len);
        }
#endif


    return ASA_FILE_FAIL;    
}

/************************************************************************
 * NAME:     get_close_null
 * FUNCTION  flush all need to save data to medium, and free all fha malloc ram
 * PARAM:    NULL
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

static T_U32  get_close_null(T_VOID)
{
    return FHA_SUCCESS;
}
/************************************************************************
 * NAME:     fha_get_close
 * FUNCTION  flush all need to save data to medium, and free all fha malloc ram
 * PARAM:    NULL
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_get_close(T_VOID)
{
    T_U32 ret = FHA_SUCCESS;
 
    if (AK_NULL != m_fha_get_func && AK_NULL != m_fha_get_func->get_close)
    {
         ret = m_fha_get_func->get_close();
         gFHAf.RamFree(m_fha_get_func);
         m_fha_get_func = AK_NULL;
    }    
    else
    {
        gFHAf.Printf("#FHA not support#\r\n");
        ret = FHA_FAIL;
    } 
 
    return ret;
}



