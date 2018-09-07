/**
 * @FILENAME: sdburn.c
 * @BRIEF xx
 * Copyright (C) 2011 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR luqiliu
 * @DATE 2009-11-19
 * @VERSION 1.0
 * @modified lu 2011-11-03
 * 
 * @REF
 */

#include "anyka_types.h"
#include "sdburn.h"
#include "spi_sd_info.h"
#include "nand_info.h"
#include "asa_format.h"
#include "fha_asa.h"

#define SD_SPARE_RATIO      10

#define     ONEMSIZE            (1024*1024)
#define     SD_USER_INFO_FLAG_CNT 8
T_U32 g_sdboot_startpage = SD_BOOT_DATA_START;


static const T_U8  m_anyka_user_info[SD_USER_INFO_FLAG_CNT] = 
{'A','N','Y','K','A','H','E','D'};

/**
 * @BREIF    SD_BurnCompareData
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE :     fail
 */

static T_BOOL SD_BurnCompareData(T_U32 start_sector, T_U32 data_len, const T_U8 *pData);

/**
 * @BREIF    SDBurn_WriteBinEnd
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN  VOID
 * @retval  
 * @retval  
 */

static T_VOID SDBurn_WriteBinEnd();

typedef struct
{
    T_BOOL bBootData;             //true is write boot data
    T_BOOL bBackup;               //true is backup
    T_BOOL bCheck;                //true is check
    T_BOOL bUpdateSelf;           //true is update self
    T_U32  BootDataSectorIndex;   //boot data sector index
    T_U32  StartSector;           //start sector
    T_U32  BackupSector;          //backup sector
    T_U32  SectorCnt;             //need to write data sector len
    T_U32  RemainSectorCnt;       //
    T_U32  FileTotalCnt;          //
    T_U32  FileIndex;             //
    T_U32  ResvSize;              //
    T_U32  bin_info_pos;          //
    T_U32  bin_end_pos;           //
    T_U32  extern_size;           //
    T_SD_FILE_INFO *pFileInfo;    //
}T_BURN_SD;

static T_BURN_SD m_burn_sd_info = {0};
static T_U32 m_GetBin_SD_Page_Start = 0;
//sd write
static T_U32 Prod_SDWrite(const T_U8 data[], T_U32 sector, T_U32 sectorcnt)
{
    return gFHAf.Write(0, sector, data, sectorcnt, 0, 0, MEDIUM_EMMC);
}

//sd read
static T_U32 Prod_SDRead(T_U8 data[], T_U32 sector, T_U32 sectorcnt)
{
    return gFHAf.Read(0, sector, data, sectorcnt, 0, 0, MEDIUM_EMMC);
}

/**
 * @BREIF    sd_get_sd_update_param
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL sd_get_sd_update_param(T_U8 strName[], T_U32 file_len)
{
    T_U32 i;
    T_U32 fileIndex = 0;

    /*
    search bin file config, suppose bin file config  was recovered by call function  
    RecoverBinFileInfo in BurnBin_init
    */
    for(i = 0; i < m_burn_sd_info.FileTotalCnt; i++)
    {            
        if(0 == fha_str_cmp(strName, m_burn_sd_info.pFileInfo[i].file_name))
        {
             fileIndex = i;
             break;
        }     
    }
    
    if (i == m_burn_sd_info.FileTotalCnt)
    {
        return AK_FALSE;
    }
    else
    {
        T_U32 file_sector_cnt;
        T_U32 space_sector_cnt;

        m_burn_sd_info.FileIndex = fileIndex;  //index of this bin in buffer of m_pBin_info

        if (T_U32_MAX == m_burn_sd_info.pFileInfo[fileIndex].backup_sector)
        {
            /*没有备份区*/
            gFHAf.Printf("FHA sd_get_sd_update_param: BIN file no backup area\r\n");
            return AK_FALSE;
        }

        if (m_burn_sd_info.bUpdateSelf)
        {
            T_U32 tmp = m_burn_sd_info.pFileInfo[fileIndex].start_sector;

            m_burn_sd_info.pFileInfo[fileIndex].start_sector = m_burn_sd_info.pFileInfo[fileIndex].backup_sector;
            m_burn_sd_info.pFileInfo[fileIndex].backup_sector = tmp;   
        }  
        
        m_burn_sd_info.StartSector = m_burn_sd_info.pFileInfo[fileIndex].start_sector;
        m_burn_sd_info.BackupSector = m_burn_sd_info.pFileInfo[fileIndex].backup_sector;

        if (m_burn_sd_info.BackupSector > m_burn_sd_info.StartSector)
        {
            space_sector_cnt = m_burn_sd_info.BackupSector - m_burn_sd_info.StartSector;
        }
        else
        {
            space_sector_cnt = m_burn_sd_info.StartSector - m_burn_sd_info.BackupSector;
        } 

        m_burn_sd_info.SectorCnt = space_sector_cnt;

        file_sector_cnt = (file_len + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE;

        if (file_sector_cnt > space_sector_cnt)
        {
            gFHAf.Printf("FHA_SD update data size is too large\r\n");
            return AK_FALSE;
        }    
    }

    return AK_TRUE; 
}

/**
 * @BREIF    extend_sector_cnt
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   extend sector count
 * @retval   
 */

T_U32 extend_sector_cnt(T_U32 bin_len)
{
    T_U32 ret = 0; 
    
    if (0 == m_burn_sd_info.extern_size)
    {
        return ((bin_len * SD_SPARE_RATIO / 100 + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE);
    }
    else
    {
        //此大小可以得长bin文件的大小是一个设定的大小
        if(m_burn_sd_info.extern_size <= (bin_len + bin_len * SD_SPARE_RATIO / 100))
        {
            ret = (bin_len * SD_SPARE_RATIO / 100 + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE;
        }
        else
        {
            ret =((m_burn_sd_info.extern_size - bin_len) + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE;
        }
        m_burn_sd_info.extern_size = 0;
        gFHAf.Printf("extend_sector_cnt=%d\r\n", ret);
        return ret;
    }
}  

/**
 * @BREIF    SD_Read_Update_Info
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE :     fail
 */

T_BOOL SD_Read_Update_Info()
{
    if (MODE_UPDATE == g_burn_param.eMode)
    {
        gFHAf.Printf("FHA_SD read bin info\r\n");

        if (FHA_SUCCESS == Prod_SDRead((T_U8 *)(m_burn_sd_info.pFileInfo), SD_STRUCT_INFO_SECTOR, 1))
        {
            T_SD_STRUCT_INFO struct_info;

            gFHAf.MemCpy(&struct_info, m_burn_sd_info.pFileInfo, sizeof(struct_info));
            m_burn_sd_info.FileTotalCnt = struct_info.file_cnt;
            m_burn_sd_info.ResvSize = struct_info.resv_size;
            m_burn_sd_info.bin_info_pos = struct_info.bin_info_pos;
            m_burn_sd_info.bin_end_pos  = struct_info.bin_end_pos;

            if (m_burn_sd_info.FileTotalCnt > (SD_BIN_INFO_SECTOR_CNT * SD_SECTOR_SIZE) / sizeof(T_SD_FILE_INFO))
            {
                gFHAf.Printf("FHA_SD filecnt:%d, suppose data is err\r\n");
                return AK_FALSE;
            }    
        }
        else
        {
            gFHAf.Printf("FHA_SD read struct info fail\r\n");
            return AK_FALSE;
        }    
        
        if (FHA_SUCCESS != Prod_SDRead((T_U8 *)(m_burn_sd_info.pFileInfo), m_burn_sd_info.bin_info_pos, SD_BIN_INFO_SECTOR_CNT))
        {
            gFHAf.Printf("FHA_SD read bin info fail\r\n");
            return AK_FALSE;
        } 
    }

    return AK_TRUE;
} 


/**
 * @BREIF    SDBurn_Init
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32 SDBurn_Init()
{
    m_burn_sd_info.bBootData = AK_FALSE;
    m_burn_sd_info.bBackup   = AK_FALSE; 
    m_burn_sd_info.bUpdateSelf = AK_FALSE;
    m_burn_sd_info.BootDataSectorIndex = SD_BOOT_DATA_START;
    m_burn_sd_info.StartSector = SD_DATA_START_SECTOR;
    m_burn_sd_info.SectorCnt = 0;
    m_burn_sd_info.FileIndex = 0;
    m_burn_sd_info.FileTotalCnt = 0;
    m_burn_sd_info.ResvSize = 0;
    m_burn_sd_info.pFileInfo = AK_NULL;
    m_burn_sd_info.extern_size = 0;
    
    //malloc space for binfile info
    m_burn_sd_info.pFileInfo = (T_SD_FILE_INFO*)gFHAf.RamAlloc(SD_SECTOR_SIZE * SD_BIN_INFO_SECTOR_CNT);
    if(AK_NULL== m_burn_sd_info.pFileInfo)
    {
        return FHA_FAIL;
    }

    if(!SD_Read_Update_Info())
    {
        gFHAf.RamFree((T_U8 *)m_burn_sd_info.pFileInfo);
        m_burn_sd_info.pFileInfo = AK_NULL;
        return FHA_FAIL;
    }    
    
    return FHA_SUCCESS;
}

/************************************************************************
 * NAME:     FHA_write_bin_begin
 * FUNCTION  set write bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 SDBurn_BinStartOne(T_PFHA_BIN_PARAM pBinFile)
{
    T_SD_FILE_INFO *pfile_info;

    m_burn_sd_info.bBootData = AK_FALSE;
    m_burn_sd_info.bBackup = pBinFile->bBackup;
    m_burn_sd_info.bCheck  = pBinFile->bCheck;
    m_burn_sd_info.bUpdateSelf = pBinFile->bUpdateSelf;

    if ((MODE_NEWBURN == g_burn_param.eMode) && m_burn_sd_info.bUpdateSelf)
    {
        m_burn_sd_info.bUpdateSelf = AK_FALSE;
    }    
 
    if (MODE_NEWBURN != g_burn_param.eMode)
    {
        if (!sd_get_sd_update_param(pBinFile->file_name, pBinFile->data_length))
        {
            gFHAf.Printf("FHA_SD get update param fail\r\n");
            return FHA_FAIL;
        }    
        pfile_info = m_burn_sd_info.pFileInfo + m_burn_sd_info.FileIndex;
    }  
    else
    {
         //space to keep bin info
         T_U32 SectorCnt;

         pfile_info = m_burn_sd_info.pFileInfo + m_burn_sd_info.FileIndex;
         SectorCnt = (pBinFile->data_length + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE
                                    + extend_sector_cnt(pBinFile->data_length);

         pfile_info->start_sector = m_burn_sd_info.StartSector;

         if(m_burn_sd_info.bBackup)
         {
            pfile_info->backup_sector = pfile_info->start_sector + SectorCnt;
            m_burn_sd_info.BackupSector = m_burn_sd_info.StartSector + SectorCnt;
         }
         else
         {
            pfile_info->backup_sector = T_U32_MAX;
         }

         gFHAf.MemCpy(pfile_info->file_name, pBinFile->file_name, 16);

         
         m_burn_sd_info.SectorCnt = SectorCnt;
    }  

    pfile_info->file_length = pBinFile->data_length;
    pfile_info->ld_addr = pBinFile->ld_addr;
    
    m_burn_sd_info.RemainSectorCnt = (pBinFile->data_length + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE;

    gFHAf.Printf("FHA_SD bin lenth:%d\r\n", pfile_info->file_length);
    gFHAf.Printf("FHA_SD start:%d\r\n", pfile_info->start_sector);
    gFHAf.Printf("FHA_SD backup:%d\r\n", pfile_info->backup_sector);
        
    return FHA_SUCCESS;

}

/************************************************************************
 * NAME:     FHA_write_bin
 * FUNCTION  write bin to medium
 * PARAM:    [in] pData-----need to write bin data pointer addr
 *           [in] data_len--need to write bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 SDBurn_WriteBinData(const T_U8 *pData, T_U32 data_len)
{
    T_U32 sector_cnt;    
    T_U32* pStartPage;

    //page of starting to write 
    pStartPage = m_burn_sd_info.bBootData ? &(m_burn_sd_info.BootDataSectorIndex)
                       : &(m_burn_sd_info.StartSector);

    //sector of starting to write 
    sector_cnt = (data_len + SD_SECTOR_SIZE -1) / SD_SECTOR_SIZE;

    gFHAf.Printf("FHA_SD start:%d\r\n", *pStartPage);
    if (sector_cnt > m_burn_sd_info.RemainSectorCnt)
    {
        gFHAf.Printf("FHA_SD write SD bin data_len is too large\r\n");
        return FHA_FAIL;
    }
    else if (sector_cnt < m_burn_sd_info.RemainSectorCnt)
    {
        if (data_len != SD_SECTOR_SIZE * sector_cnt)
        {
            gFHAf.Printf("FHA_SD write SD bin data_len is not page multiple\r\n");
            return FHA_FAIL;
        }
    }
    else
    {
        if (!m_burn_sd_info.bBootData)
        {
            T_SD_FILE_INFO* file_cfg;
        
            file_cfg = m_burn_sd_info.pFileInfo + m_burn_sd_info.FileIndex;
      
            if ((data_len % SD_SECTOR_SIZE) != (file_cfg->file_length % SD_SECTOR_SIZE))
            {
                gFHAf.Printf("FHA_SD write SD bin data_len is error!\r\n");
                return FHA_FAIL;
            }
        }    
    }

    if(FHA_SUCCESS != Prod_SDWrite(pData, *pStartPage, sector_cnt))
    {
        gFHAf.Printf("FHA_SD write err\r\n");
        return FHA_FAIL;
    }
    else
    {
        if (m_burn_sd_info.bCheck)
        {
            if (!SD_BurnCompareData(*pStartPage, data_len, pData))
            {
                return FHA_FAIL;
            }
        }

        *pStartPage += sector_cnt;
        m_burn_sd_info.RemainSectorCnt -= sector_cnt;
    }    

    //write backup data
    if (m_burn_sd_info.bBackup && !m_burn_sd_info.bUpdateSelf)
    {       
        if(FHA_SUCCESS != Prod_SDWrite(pData, m_burn_sd_info.BackupSector, sector_cnt))
        {
            gFHAf.Printf("FHA_SD write backup err\r\n");
            return FHA_FAIL;
        }
        else
        {
            if (m_burn_sd_info.bCheck)
            {
                if (!SD_BurnCompareData(m_burn_sd_info.BackupSector, data_len, pData))
                {
                    return FHA_FAIL;
                } 
            }    

            m_burn_sd_info.BackupSector += sector_cnt;
        } 
    }

    if (0 == m_burn_sd_info.RemainSectorCnt && !m_burn_sd_info.bBootData)
        SDBurn_WriteBinEnd();
    
    return FHA_SUCCESS;
}

/**
 * @BREIF    SDBurn_WriteBinEnd
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN  VOID
 * @retval  
 * @retval  
 */

static T_VOID SDBurn_WriteBinEnd()
{
    T_SD_FILE_INFO *pfile_info = m_burn_sd_info.pFileInfo + m_burn_sd_info.FileIndex;
   
    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        m_burn_sd_info.StartSector = pfile_info->start_sector + m_burn_sd_info.SectorCnt;

        if (m_burn_sd_info.bBackup)
        {
            m_burn_sd_info.StartSector += m_burn_sd_info.SectorCnt;
        } 

        m_burn_sd_info.bin_end_pos = m_burn_sd_info.StartSector;
    }

    m_burn_sd_info.FileIndex++;
}    

/************************************************************************
 * NAME:     SDBurn_SetResvSize
 * FUNCTION  set reserve area
 * PARAM:    [in] nSize--set reserve area size(unit Mbyte)
 *           [in] bErase-if == 1 ,erase reserve area, or else not erase 
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 SDBurn_SetResvSize(T_U32  nSize, T_BOOL bErase)
{
    T_U32 oneM_size;
    oneM_size = ONEMSIZE/ SD_SECTOR_SIZE;

    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        m_burn_sd_info.StartSector += (nSize * oneM_size);
        m_burn_sd_info.ResvSize = nSize;
        m_burn_sd_info.bin_end_pos = m_burn_sd_info.StartSector;
    } 

    return FHA_SUCCESS;
}

/**
 * @BREIF    SD_BurnCompareData
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE :     fail
 */
static T_BOOL SD_BurnCompareData(T_U32 start_sector, T_U32 data_len, const T_U8 *pData)
{
    T_U32 sector_cnt;
    T_U8* pBuf = AK_NULL;

    sector_cnt = (data_len + SD_SECTOR_SIZE -1) / SD_SECTOR_SIZE;

     //malloc buffer to read data
    pBuf = gFHAf.RamAlloc(sector_cnt * SD_SECTOR_SIZE);

    if(AK_NULL == pBuf)
    {
        return AK_FALSE;
    }

    if(FHA_SUCCESS != Prod_SDRead(pBuf, start_sector, sector_cnt))
    {
        gFHAf.Printf("FHA_SD read err\r\n");
        gFHAf.RamFree(pBuf);
        return AK_FALSE;
   }

    if(0 != gFHAf.MemCmp(pData, pBuf, data_len))
    {
        gFHAf.Printf("FHA_SD compare fail\r\n");
        gFHAf.RamFree(pBuf);
        return AK_FALSE;
    }

    gFHAf.RamFree(pBuf);
    return AK_TRUE;
}

/************************************************************************
 * NAME:     FHA_write_boot_begin
 * FUNCTION  set write boot start init para
 * PARAM:    [in] bin_len--boot length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 SDBurn_BurnBootStart(T_U32 boot_len)
{
    //flag of boot area
    m_burn_sd_info.bBootData = AK_TRUE;
    m_burn_sd_info.bBackup   = AK_FALSE;
    m_burn_sd_info.bCheck    = AK_TRUE;
    m_burn_sd_info.bUpdateSelf = AK_FALSE;
    m_burn_sd_info.RemainSectorCnt = (boot_len + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE;
    
    return FHA_SUCCESS;
}

/**
 * @BREIF    SDBurn_WriteConfig
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32 SDBurn_WriteConfig(T_VOID)
{
    T_U32 info_sec_cnt;

    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        T_U8 *tmpBuf;

        tmpBuf = gFHAf.RamAlloc(SD_SECTOR_SIZE);

        if (AK_NULL == tmpBuf)
        {
            gFHAf.Printf("FHA_SD W_C malloc err\r\n");
            return FHA_FAIL;
        }
  
        m_burn_sd_info.FileTotalCnt = m_burn_sd_info.FileIndex;

        if (m_burn_sd_info.FileTotalCnt > 0)
        {  
            T_SD_STRUCT_INFO struct_info;

            gFHAf.MemSet(tmpBuf, 0, SD_SECTOR_SIZE);

            gFHAf.Printf("FHA_SD write fileinfo, count:%d\r\n", m_burn_sd_info.FileTotalCnt);

            struct_info.file_cnt = m_burn_sd_info.FileTotalCnt;
            struct_info.data_start = SD_DATA_START_SECTOR;
            struct_info.resv_size = m_burn_sd_info.ResvSize;
            struct_info.bin_info_pos = SD_BIN_INFO_SECTOR;
            struct_info.bin_end_pos  = m_burn_sd_info.bin_end_pos;
            gFHAf.MemCpy(tmpBuf, &struct_info, sizeof(T_SD_STRUCT_INFO));

            if(FHA_SUCCESS != Prod_SDWrite(tmpBuf, SD_STRUCT_INFO_SECTOR, 1))
            {
                gFHAf.RamFree(tmpBuf);
                gFHAf.Printf("FHA_SD write structinfo err\r\n");
                return FHA_FAIL;
            }

            m_burn_sd_info.bin_info_pos = struct_info.bin_info_pos;
        }
        
        gFHAf.RamFree(tmpBuf);
    }    

    if (m_burn_sd_info.FileTotalCnt > 0)
    {    
        info_sec_cnt = (m_burn_sd_info.FileTotalCnt * sizeof(T_SD_FILE_INFO) - 1) / SD_SECTOR_SIZE + 1;
    
        if (FHA_SUCCESS != Prod_SDWrite((T_U8 *)m_burn_sd_info.pFileInfo, m_burn_sd_info.bin_info_pos, info_sec_cnt))
        {
            gFHAf.Printf("FHA_SD write fileinfo err\r\n");
            return FHA_FAIL;
        }          
    }

    if (m_burn_sd_info.pFileInfo != AK_NULL)
    {
        gFHAf.RamFree(m_burn_sd_info.pFileInfo);
        m_burn_sd_info.pFileInfo = AK_NULL;
    }
    
    gFHAf.Printf("FHA_SD write cfg end\r\n");

    return FHA_SUCCESS;   
}

/************************************************************************
 * NAME:     FHA_get_last_pos
 * FUNCTION  get fs start position
 * PARAM:    NULL
 * RETURN:   success return fs start block, fail retuen 0
**************************************************************************/

T_U32 SDBurn_get_last_pos(T_VOID)
{
    gFHAf.Printf("m_burn_sd_info.bin_end_pos:%d\r\n", m_burn_sd_info.bin_end_pos);
    return m_burn_sd_info.bin_end_pos;    
}   

/************************************************************************
 * NAME:     SDBurn_GetBinStart
 * FUNCTION  set read bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/



//开始获取boot
T_U32 SDBurn_GetBootStart(T_PFHA_BIN_PARAM bin_param)
{
    if(0 == fha_str_cmp(bin_param->file_name, BOOT_NAME))
    {
        bin_param->data_length  = (SD_BIN_INFO_SECTOR - SD_BOOT_DATA_START)*SD_SECTOR_SIZE;
    }
    return FHA_SUCCESS;  
}

//获取boot数据
T_U32 SDBurn_GetBootData(T_U8* pBuf, T_U32 data_len)
{
    T_U32 page_cnt;
    T_U32 page_size = SD_SECTOR_SIZE;
   

    page_cnt = (data_len + page_size - 1) / page_size;

    if (FHA_SUCCESS != Prod_SDRead(pBuf, g_sdboot_startpage, page_cnt))
    {
        gFHAf.Printf("FHA_SD read fail at page:%d\r\n", m_GetBin_SD_Page_Start);
        return FHA_FAIL;
    }    
    g_sdboot_startpage = g_sdboot_startpage + page_cnt;
    
    return FHA_SUCCESS;  
}




T_U32 SDBurn_GetBinStart(T_PFHA_BIN_PARAM bin_param)
{
    T_U32 i;
    T_U32 fileIndex = 0;
    T_SD_FILE_INFO *pFileInfo;
    T_SD_STRUCT_INFO *struct_info;
    T_U32 file_count, pos;
    T_U8  *pbuf = AK_NULL;
    
    pbuf = gFHAf.RamAlloc(SD_SECTOR_SIZE);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    }        

    if (FHA_SUCCESS != Prod_SDRead(pbuf, SD_STRUCT_INFO_SECTOR, 1))
    {
        gFHAf.Printf("FHA_SD read file cnt fail\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    struct_info = (T_SD_STRUCT_INFO *)pbuf;
    file_count = struct_info->file_cnt;
    m_burn_sd_info.bin_end_pos = struct_info->bin_end_pos;
     gFHAf.Printf("m_burn_sd_info.bin_end_pos:%d\r\n", m_burn_sd_info.bin_end_pos);
    if (file_count == 0)
    {
        gFHAf.Printf("FHA_SD G_B_S file num is 0\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;        
    }
    
    pos = struct_info->bin_info_pos;

    if (FHA_SUCCESS != Prod_SDRead(pbuf, struct_info->bin_info_pos, 1))
    {
        gFHAf.Printf("FHA_SD read file info fail\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    pFileInfo = (T_SD_FILE_INFO *)(pbuf);
    
    for(i = 0; i < file_count; i++)
    {   
        //gFHAf.Printf("name[0]:%d, cnt:%d\r\n", bin_param->file_name[0], file_count);
		//gFHAf.Printf("bin_name[0]:%s, file_name:%s\r\n", bin_param->file_name, pFileInfo[i].file_name);
        if (bin_param->file_name[0] < file_count)
        {
            if (i == bin_param->file_name[0])
            {
                gFHAf.MemSet(bin_param, 0, sizeof(T_FHA_BIN_PARAM));
                gFHAf.MemCpy(bin_param->file_name, pFileInfo[i].file_name, sizeof(bin_param->file_name));
                fileIndex = i;
                break;
            }
        }
        else if(0 == fha_str_cmp(bin_param->file_name, pFileInfo[i].file_name))
        {
            fileIndex = i;
            break;
        }     
    }

    if (i == file_count)
    {
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    bin_param->data_length = pFileInfo[fileIndex].file_length;
    bin_param->ld_addr     = pFileInfo[fileIndex].ld_addr;
    //gFHAf.Printf("bin_param->data_length:%d\r\n", bin_param->data_length);
    //gFHAf.Printf("bin_param->ld_addr:%d\r\n", bin_param->ld_addr);
    //gFHAf.Printf("bin_param->bBackup:%d\r\n", bin_param->bBackup);
    //gFHAf.Printf("pFileInfo[%d].start_sector:%d\r\n", fileIndex, pFileInfo[fileIndex].start_sector);
    if (bin_param->bBackup)
    {
        m_GetBin_SD_Page_Start = pFileInfo[fileIndex].backup_sector;
    }
    else
    {
        m_GetBin_SD_Page_Start = pFileInfo[fileIndex].start_sector;
    }
    gFHAf.RamFree(pbuf);

    //gFHAf.Printf("FHA_SD G_B_S start page:%d\r\n", m_GetBin_SD_Page_Start);
    if (m_GetBin_SD_Page_Start != 0)
    {
        return FHA_SUCCESS;
    }
    else
    {
        return FHA_FAIL;
    }
}

/************************************************************************
 * NAME:     SDBurn_GetBinData
 * FUNCTION  read bin to buf from medium
 * PARAM:    [out]pData-----need to read bin data buf pointer addr
 *           [in] data_len--need to read bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 SDBurn_GetBinData(T_U8* pBuf, T_U32 data_len)
{
    T_U32 page_cnt;
    T_U32 i =0;
    T_U32 page_size = SD_SECTOR_SIZE;

    page_cnt = (data_len + page_size - 1) / page_size;
    
    //gFHAf.Printf("startpage:%d\r\n", m_GetBin_SD_Page_Start);
    
    if (FHA_SUCCESS != Prod_SDRead(pBuf, m_GetBin_SD_Page_Start, page_cnt))
    {
        gFHAf.Printf("FHA_SD read fail at page:%d\r\n", m_GetBin_SD_Page_Start);
        return FHA_FAIL;
    }   
    /*
    for(i = 0; i < data_len; i++)
    {
        if(i%16 == 0)
        {
            gFHAf.Printf("\n");
        }
        gFHAf.Printf("%x ", pBuf[i]);
    }*/

    m_GetBin_SD_Page_Start +=page_cnt;
    
    return FHA_SUCCESS; 
}

T_U32 SDBurn_GetFSPart(T_U8 *pInfoBuf, T_U32 buf_len)
{
    T_U32 SecCnt;
    T_U8 *pBuf = AK_NULL;

    if (0 == buf_len)
    {
        return FHA_SUCCESS;
    }

    SecCnt = ((buf_len + SD_SECTOR_SIZE - 1)/ SD_SECTOR_SIZE);
    pBuf = gFHAf.RamAlloc(SecCnt * SD_SECTOR_SIZE);
    if (AK_NULL == pBuf)
    {
        return FHA_FAIL;
    }
    
    if (FHA_SUCCESS != Prod_SDRead(pBuf, SD_DISK_INFO_SECTOR, SecCnt))
    {
        gFHAf.Printf("FHA_SD S_F_P read err\r\n");
        gFHAf.RamFree(pBuf);
        return FHA_FAIL;
    } 

    gFHAf.MemCpy(pInfoBuf, pBuf, buf_len);
    gFHAf.RamFree(pBuf);
    
    return FHA_SUCCESS;
}

T_U32 SDBurn_SetFSPart(const T_U8 *pInfoBuf, T_U32 buf_len)
{
    T_U32 SecCnt;

    SecCnt = ((buf_len + SD_SECTOR_SIZE - 1)/ SD_SECTOR_SIZE);
    if (FHA_SUCCESS != Prod_SDWrite(pInfoBuf, SD_DISK_INFO_SECTOR, SecCnt))
    {
        gFHAf.Printf("FHA_SD S_F_P write err\r\n");
        return FHA_FAIL;
    }  

    return FHA_SUCCESS;
}

T_U32 SDBurn_GetBinNum(T_U32 *cnt)
{
    T_SD_STRUCT_INFO *struct_info;
    T_U32 file_count;
    T_U8  *pbuf = AK_NULL;
    
    pbuf = gFHAf.RamAlloc(SD_SECTOR_SIZE);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    }        

    if (FHA_SUCCESS != Prod_SDRead(pbuf, SD_STRUCT_INFO_SECTOR, 1))
    {
        gFHAf.Printf("FHA_SD G_B_N read fail\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    struct_info = (T_SD_STRUCT_INFO *)pbuf;
    file_count = struct_info->file_cnt;
    
    if (file_count == 0)
    {
        gFHAf.Printf("FHA_SD G_B_N file num is 0\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;        
    }
    
    *cnt = file_count;
    gFHAf.RamFree(pbuf);
    
    return FHA_SUCCESS;
}

T_U32 SDBurn_GetResvZoneInfo(T_U16 *start_block, T_U16 *block_cnt)
{
    T_U8  *pbuf = AK_NULL;
    T_U32 ret   = FHA_FAIL;

    if (AK_NULL == start_block || AK_NULL == block_cnt)
    {
        return FHA_FAIL;
    }
    
    pbuf = gFHAf.RamAlloc(SD_SECTOR_SIZE);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    }

    if (FHA_SUCCESS == Prod_SDRead(pbuf, SD_STRUCT_INFO_SECTOR, 1))
    {
        T_SD_STRUCT_INFO struct_info;
        T_U32 oneM_size = ONEMSIZE/ SD_SECTOR_SIZE;

        gFHAf.MemCpy(&struct_info, pbuf, sizeof(struct_info));
        *block_cnt   = (T_U16)(struct_info.resv_size * oneM_size);
        *start_block = (T_U16)struct_info.bin_end_pos - *block_cnt;
        ret = FHA_SUCCESS;
    }
        
    gFHAf.RamFree(pbuf); 

    return ret;
}

T_U32 SDBurn_WriteLibVerion(T_LIB_VER_INFO *lib_info,  T_U32 lib_cnt)
{
    if (AK_NULL == lib_info)
    {
        return FHA_FAIL;
    }

    if (sizeof(T_LIB_VER_INFO) * lib_cnt > SD_SECTOR_SIZE)
    {
        gFHAf.Printf("FHA_SD SET VER: cnt too large\r\n");
        return FHA_FAIL;
    }

    if (FHA_SUCCESS != Prod_SDWrite((T_U8 *)lib_info, SD_LIB_VER_SECTOR, 1))
    {
        gFHAf.Printf("FHA_SD SET VER write err\r\n");
        return FHA_FAIL;
    }  
    
    return FHA_SUCCESS;    
} 

T_U32 SD_GetLibVerion(T_LIB_VER_INFO *lib_info)
{
    T_U32 pos, size;
    T_U8 *buf = AK_NULL;
    T_LIB_VER_INFO *info;

    if (AK_NULL == lib_info)
    {
        return FHA_FAIL;
    }

    size = SD_SECTOR_SIZE;
    buf = gFHAf.RamAlloc(size);
    if(AK_NULL == buf)
    {
        gFHAf.Printf("FHA_SD GET VER: malloc fail\r\n");
        return FHA_FAIL;
    }

    if (FHA_SUCCESS != Prod_SDRead(buf, SD_LIB_VER_SECTOR, 1))
    {
        gFHAf.Printf("FHA_SD GET VER: read file fail\r\n");
        gFHAf.RamFree(buf);
        return FHA_FAIL;
    }

    pos = 0;
    info = (T_LIB_VER_INFO *)buf;
    while (pos < size)
    {
        if(0 == fha_str_cmp(info->lib_name, lib_info->lib_name))
        {
            gFHAf.MemCpy(lib_info->lib_version, info->lib_version, sizeof(lib_info->lib_version));            
            gFHAf.RamFree(buf);
            return FHA_SUCCESS;
        }
        info++;
        pos += sizeof(T_LIB_VER_INFO);
    }

    gFHAf.RamFree(buf);
    return FHA_FAIL;    
} 

T_U32 SDBurn_set_bin_resv_size(T_U32 bin_size)
{
    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        if(PLAT_LINUX == g_burn_param.ePlatform)
        {
            gFHAf.Printf("bin_size(k)=%d\r\n", bin_size);
            m_burn_sd_info.extern_size = bin_size*1024;  //以k为单位
        }
        else
        {
            gFHAf.Printf("bin_size(M)=%d\r\n", bin_size);
            m_burn_sd_info.extern_size = bin_size*ONEMSIZE;  //以m为单位
        }
        gFHAf.Printf("m_burn_sd_info.extern_size=%d\r\n", m_burn_sd_info.extern_size);
    }

    return FHA_SUCCESS;
}

T_U32 SDBurn_asa_write_file(T_U8 file_name[], const T_U8 dataBuf[], T_U32 dataLen, T_U8 mode)
{
    T_U8 *pBuf = AK_NULL;
    T_ASA_HEAD *pHeadInfo;
    T_ASA_FILE_INFO *pFileInfo = AK_NULL;
    T_U32 i,j;
    T_U32 file_num;
    T_U32 file_num_MAX;
    T_BOOL bMatch = AK_FALSE; //must false
    T_U32 page_cnt = (dataLen + SD_SECTOR_SIZE -1)/SD_SECTOR_SIZE;
    
    if(AK_NULL == file_name || AK_NULL == dataBuf)
    {
        return ASA_FILE_FAIL;
    }

    if (fha_str_len(file_name) > sizeof(pFileInfo->file_name) -1)
    {
        gFHAf.Printf("SDBurn_write_file(): file name overlong!\r\n");        
        return ASA_FILE_FAIL;
    }

    file_num_MAX = (SD_SECTOR_SIZE - sizeof(T_ASA_HEAD)) / sizeof(T_ASA_FILE_INFO);
    
    pBuf = gFHAf.RamAlloc(SD_SECTOR_SIZE);
    if( AK_NULL == pBuf)
    {
        return ASA_FILE_FAIL;
    }

    //read SD_USER_INFO_SECTOR_START to get file info
    if(FHA_SUCCESS != Prod_SDRead(pBuf, SD_USER_INFO_SECTOR_START, 1))
    {
        goto EXIT;
    }

    //head info
    pHeadInfo = (T_ASA_HEAD *)pBuf;

    if (gFHAf.MemCmp(pHeadInfo->head_str, m_anyka_user_info, SD_USER_INFO_FLAG_CNT))
    {
        gFHAf.MemCpy(pHeadInfo->head_str, m_anyka_user_info, SD_USER_INFO_FLAG_CNT);
        gFHAf.MemSet(pHeadInfo->verify, 0, 2);
        pHeadInfo->item_num = 0;
        pHeadInfo->info_end = SD_USER_INFO_SECTOR_START + 1;
    }
    
    //asa file info
    pFileInfo = (T_ASA_FILE_INFO *)(pBuf + sizeof(T_ASA_HEAD));

    file_num = pHeadInfo->item_num;

    //search asa file name
    for(i=0; i<file_num; i++)
    {
        bMatch = AK_TRUE;
        for(j=0; j < 8; j++)
        {
            if(file_name[j] != (pFileInfo->file_name)[j])
            {
                bMatch = AK_FALSE;
                break;
            } 

            if (0 == file_name[j])
                break;
        }

        //exist
        if(bMatch)
        {
            //return if open mode write
            if(ASA_MODE_OPEN == mode)
            {
                gFHAf.RamFree(pBuf);
                pBuf = AK_NULL;
                return ASA_FILE_EXIST;
            }
            else
            {
                //create mode
                //overflow page count
                if(page_cnt > (pFileInfo->end_page - pFileInfo->start_page))
                {
                    goto EXIT;
                }
                else
                {
                    break;
                }
            }
        }    
        else
        {
            //continue to search
            pFileInfo++;
        }
    }

    if(!bMatch)
    {
        if (pHeadInfo->item_num >= file_num_MAX)
        {
            gFHAf.Printf("SDBurn_write_file(): file num is max!\r\n");        
            goto EXIT;
        }
        
        //new asa file
        pFileInfo->start_page = pHeadInfo->info_end;
        pFileInfo->end_page = pFileInfo->start_page + page_cnt;
        pFileInfo->file_length = dataLen;
        gFHAf.MemCpy(pFileInfo->file_name, file_name, 8);

        //whether excess block size
        if(pFileInfo->end_page > SD_USER_INFO_SECTOR_END)
        {
            goto EXIT;
        }

        //chenge head info if new file
        pHeadInfo->item_num++;
        pHeadInfo->info_end = pFileInfo->end_page;
    }
    
    //write head info
    if(FHA_SUCCESS != Prod_SDWrite(pBuf, SD_USER_INFO_SECTOR_START, 1))
    {
        goto EXIT;
    }
    
    gFHAf.RamFree(pBuf);

    //write head info
    if(FHA_SUCCESS != Prod_SDWrite(dataBuf,
        pFileInfo->start_page, 
        pFileInfo->end_page - pFileInfo->start_page))
    {
        goto EXIT;
    }

    return ASA_FILE_SUCCESS;
EXIT:
    gFHAf.RamFree(pBuf);
    return ASA_FILE_FAIL;
}

T_U32 SDBurn_asa_read_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen)
{
    T_U8 *pBuf = AK_NULL;
    T_ASA_HEAD *pHead = AK_NULL;
    T_ASA_FILE_INFO *pFileInfo = AK_NULL;
    T_U32 i,j;
    T_U32 file_num;
    T_BOOL bMatch = AK_FALSE; //must false
    T_U32 ret= ASA_FILE_FAIL;
    T_U32 page_cnt = (dataLen + SD_SECTOR_SIZE - 1) / SD_SECTOR_SIZE;
    T_U32 start_page;

    if(AK_NULL == file_name || AK_NULL == dataBuf)
    {
        return ASA_FILE_FAIL;
    }

    pBuf = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if( AK_NULL == pBuf)
    {
        return ASA_FILE_FAIL;
    }

    //安全区文件最初始时只写两个块，也只尝试去读两个块
    //read SD_USER_INFO_SECTOR_START to get file info
    if(FHA_SUCCESS != Prod_SDRead(pBuf, SD_USER_INFO_SECTOR_START, 1))
    {
        goto EXIT;
    }

    //head info
    pHead = (T_ASA_HEAD *)pBuf;

    if (gFHAf.MemCmp(pHead->head_str, m_anyka_user_info, SD_USER_INFO_FLAG_CNT))
    {
        gFHAf.Printf("SDBurn_read_file(): no file!\r\n");        
        
        gFHAf.RamFree(pBuf);
        return ASA_FILE_FAIL;
    }

    //asa file info
    pFileInfo = (T_ASA_FILE_INFO *)(pBuf + sizeof(T_ASA_HEAD));

    file_num = pHead->item_num;

    //search asa file name
    for(i=0; i<file_num; i++)
    {
        bMatch = AK_TRUE;
        for(j=0; j < 8; j++)
        {
            if(file_name[j] != (pFileInfo->file_name)[j])
            {
                bMatch = AK_FALSE;
                break;
            } 

            if (0 == file_name[j])
                break;
        }

        //exist
        if(bMatch)
        {        
            break;
        }    
        else
        {
            //continue to search
            pFileInfo++;
        }
    }

    if(!bMatch)
    {
        goto EXIT;
    }
    else
    {
        if(page_cnt > (pFileInfo->end_page - pFileInfo->start_page))
        {
            goto EXIT;
        }

        //read head info
        start_page = pFileInfo->start_page;

        for (i=0; i<page_cnt; i++)
        {
            if(FHA_SUCCESS != Prod_SDRead(pBuf, start_page + i, 1))
            {
                goto EXIT;
            } 

            if ((i == page_cnt -1) && (dataLen % SD_SECTOR_SIZE) != 0)
            {
                gFHAf.MemCpy(dataBuf+i*SD_SECTOR_SIZE, pBuf, dataLen % SD_SECTOR_SIZE);
            }
            else
            {
                gFHAf.MemCpy(dataBuf+i*SD_SECTOR_SIZE, pBuf, SD_SECTOR_SIZE);
            }
        }
        
    }

    ret= ASA_FILE_SUCCESS;
            
EXIT:
    gFHAf.RamFree(pBuf);
    return ret;
}

/*
T_U32 SDBurn_GetSDLastPosPara(T_VOID)
{
    T_SD_STRUCT_INFO *struct_info;
    T_U32 file_count;
    T_U8  *pbuf = AK_NULL;
    
    pbuf = gFHAf.RamAlloc(SD_SECTOR_SIZE);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    }        

    if (FHA_SUCCESS != Prod_SDRead(pbuf, SD_STRUCT_INFO_SECTOR, 1))
    {
        gFHAf.Printf("FHA_SD G_L_P read fail\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    struct_info = (T_SD_STRUCT_INFO *)pbuf;
    m_burn_sd_info.bin_end_pos = struct_info->bin_end_pos;
    gFHAf.RamFree(pbuf);
    
    return FHA_SUCCESS;
}

T_U32 SDBurn_GetMaplist(T_U8 file_name[], T_U16 *map_data, T_U32 *file_len, T_BOOL bBackup)
{
    T_FHA_BIN_PARAM bin_param;
    T_U32 i;

    gFHAf.MemCpy(bin_param.file_name, file_name, sizeof(bin_param.file_name));
    bin_param.bBackup = bBackup;
    
    if (FHA_FAIL == SDBurn_GetBinStart(bin_param))
    {
        return FHA_FAIL;
    }
    
    
    return FHA_SUCCESS;
}
*/

