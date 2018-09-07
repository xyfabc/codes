/**
 * @FILENAME: spiburn.c
 * @BRIEF xx
 * Copyright (C) 2011 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR luqiliu
 * @DATE 2009-11-19
 * @VERSION 1.0
 * @modified lu 2011-11-03
 * 由于需要支持自升级，修改SPI的存储结构，以block为单位存储文件
 * @REF
 */

#include "spiburn.h"
#include "spi_sd_info.h"
#include "nand_info.h"
#include "fha_asa.h"


#define SPI_FS_PART_OFFSET 0
#define SPI_BIN_INFO_OFFSET 1
#define SPI_BIN_DATA_OFFSET 2

#define SPI_SPARE_RATIO   10
#define SPI_BLOCK_NUM     1024 //1024 * 8

#define TestBlockMap(BlockMap, BlockID)(((BlockMap)[(BlockID)>>3]&(1<<((BlockID)&7))))
#define SetBlockMap(BlockMap, BlockID) ((BlockMap)[(BlockID)>>3] |= (1<<((BlockID)&7)))
#define ClrBlockMap(BlockMap, BlockID) ((BlockMap)[(BlockID)>>3] &= ~(1<<((BlockID)&7)))

T_U32 g_spiboot_startpage = 0;
T_U32 g_firstbin_burn = AK_TRUE;



typedef struct
{
    T_BOOL bBootData;
    T_BOOL bCheck;
    T_BOOL bUpdateSelf;
    T_BOOL bBackUP;  
    T_U32  EraseBlocks;
    T_U32  BootDataPageIndex;
    T_U32  FileStartPage;
    T_U32  FileBackupPage;
    T_U32  FileTotalCnt;
    T_U32  FileCnt;
    T_U32  RemainSectorCnt;
    T_U32  extern_size;
    T_U32  TotSecCnt;
    T_U8   *BlockMap;
    T_SPI_FILE_CONFIG*  pFileInfo;
}T_DOWNLOAD_SPIFLASH; 

static T_DOWNLOAD_SPIFLASH         m_download_sflash = {0};

static T_U32 m_GetBin_SPI_Page_Start = 0;
static T_BOOL m_wirtebin_flag = AK_FALSE;


typedef struct
{
    T_U32 BinPageStart; /*bin data start addr*/
    T_U32 PageSize;     /*spi page size*/
    T_U32 PagesPerBlock;/*page per block*/
    T_U32 BinInfoStart;
    T_U32 FSPartStart;
}
T_SPI_BURN_INIT_INFO;

static T_SPI_BURN_INIT_INFO m_spi_burn;


T_U32 SPI_extend_page_cnt(T_U32 bin_len, T_BOOL flag, T_U32 infopagecnt);
T_BOOL SPI_Read_Update_Info(T_VOID);
T_BOOL SPI_get_pdate_param(T_U8 strName[], T_U32 file_len);
T_U32 Sflash_BurnUpdateConfig(T_VOID);

//only erase a block
static T_U32 Prod_SPIErase(T_U32 nChip,  T_U32 nBlock)
{
    return gFHAf.Erase(nChip, nBlock * m_spi_burn.PagesPerBlock);
}

//only write a page each time
static T_U32 Prod_SPIWritePage(T_U32 page, T_U32 pageCnt, const T_U8 *pData)
{
    return gFHAf.Write(0, page, pData, pageCnt, 0, 0, MEDIUM_SPIFLASH);
}

//only read a page each time
static T_U32 Prod_SPIReadPage(T_U32 page, T_U32 pageCnt, T_U8 *pData)
{
    return gFHAf.Read(0, page, pData, pageCnt, 0, 0, MEDIUM_SPIFLASH);
}



T_U32  Sflash_get_nand_last_pos(T_VOID)
{
    return m_download_sflash.FileStartPage;
  
}


T_U32 Sflash_MountInit(T_pVOID SpiInfo)
{
   T_SPI_INIT_INFO *info = (T_SPI_INIT_INFO *)SpiInfo;
   
    if (AK_NULL == info)
    {
        return FHA_FAIL;
    }

    m_spi_burn.PageSize      = info->PageSize;
    m_spi_burn.PagesPerBlock = info->PagesPerBlock;
    m_spi_burn.BinInfoStart  = info->BinPageStart + SPI_BIN_INFO_OFFSET;
    m_spi_burn.FSPartStart   = info->BinPageStart + SPI_FS_PART_OFFSET;
    m_spi_burn.BinPageStart  = info->BinPageStart + SPI_BIN_DATA_OFFSET;

    //linux平台那边，bin文件信息页和分区表页以一块存放
    if(PLAT_LINUX == g_burn_param.ePlatform)
    {
        m_spi_burn.BinPageStart = (m_spi_burn.BinPageStart*m_spi_burn.PageSize + m_spi_burn.PagesPerBlock*m_spi_burn.PageSize - 1)/(m_spi_burn.PagesPerBlock*m_spi_burn.PageSize);
        gFHAf.Printf("FHA_S SPIFlash_Init: BinPageStartblock:%d,\r\n", m_spi_burn.BinPageStart);  
        m_spi_burn.BinPageStart = m_spi_burn.BinPageStart*m_spi_burn.PagesPerBlock;
        gFHAf.Printf("FHA_S SPIFlash_Init: BinPageStart:%d,\r\n", m_spi_burn.BinPageStart);  
    }

    return FHA_SUCCESS;
}

/**
 * @BREIF    init spiflash burn
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-12-10
 * @PARAM    [out] chip ID
 * @RETURN   T_U32
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 Sflash_BurnInit(T_pVOID SpiInfo)
{
    if (FHA_FAIL == Sflash_MountInit(SpiInfo))
    {
        return FHA_FAIL;
    }
        
    m_download_sflash.bBootData = AK_FALSE;
    m_download_sflash.BootDataPageIndex = 0;
    m_download_sflash.FileStartPage = m_spi_burn.BinPageStart;
    m_download_sflash.FileBackupPage = T_U32_MAX;
    m_download_sflash.bCheck = AK_TRUE;
    m_download_sflash.pFileInfo = AK_NULL;
    m_download_sflash.FileTotalCnt = 0;
    m_download_sflash.FileCnt = 0;
    m_download_sflash.EraseBlocks = 0;
    m_download_sflash.bBackUP = AK_FALSE;
    m_download_sflash.extern_size = 0;
    m_download_sflash.TotSecCnt   = 0;
    m_download_sflash.BlockMap = AK_NULL;
        
    gFHAf.Printf("FHA_S SPIFlash_Init: FSPartStart:%d, PageSize:%d, PagesPerBlock:%d\r\n",
        m_spi_burn.FSPartStart, 
        m_spi_burn.PageSize,
        m_spi_burn.PagesPerBlock);  

    //malloc space for binfile info
    m_download_sflash.pFileInfo = (T_SPI_FILE_CONFIG*)gFHAf.RamAlloc(m_spi_burn.PageSize);
    if(AK_NULL== m_download_sflash.pFileInfo)
    {
        return FHA_FAIL;
    }

    m_download_sflash.BlockMap = (T_U8 *)gFHAf.RamAlloc(SPI_BLOCK_NUM);
    if(AK_NULL== m_download_sflash.BlockMap)
    {
        gFHAf.RamFree((T_U8 *)m_download_sflash.pFileInfo);
        m_download_sflash.pFileInfo = AK_NULL;
        return FHA_FAIL;
    }
    
    gFHAf.MemSet(m_download_sflash.pFileInfo, 0, m_spi_burn.PageSize);   
    gFHAf.MemSet(m_download_sflash.BlockMap, 0, SPI_BLOCK_NUM);   

    if (!SPI_Read_Update_Info())
    {
        gFHAf.RamFree((T_U8 *)m_download_sflash.pFileInfo);
        m_download_sflash.pFileInfo = AK_NULL;
        return FHA_FAIL;
    }

    return FHA_SUCCESS;
} 

/**
 * @BREIF    start to write boot area 
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-12-10
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 Sflash_BurnBootStart(T_U32 boot_len)
{
    //flag of boot area
    m_download_sflash.bBootData = AK_TRUE;
    m_download_sflash.bCheck    = AK_TRUE;
    m_download_sflash.RemainSectorCnt = (boot_len + m_spi_burn.PageSize - 1) / m_spi_burn.PageSize;
    m_download_sflash.TotSecCnt = 0;
    m_download_sflash.FileBackupPage = m_download_sflash.BootDataPageIndex + m_download_sflash.RemainSectorCnt;
        
    return FHA_SUCCESS;
}


/**
 * @BREIF    get bin information to start to write bin
 * @DATE     2009-09-10
 * @PARAM    [in] bin information
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
T_U32 Sflash_BurnBinStart(T_FHA_BIN_PARAM *pBinFile)
{ 
    T_SPI_FILE_CONFIG* pfile_cfg;
    T_U32 block_cnt = 0;
    T_U32 page_cnt  = 0;
    T_U32 TotSecCnt = 0;
    T_U32 PageStart = 0;

    m_download_sflash.bBootData = AK_FALSE;
    m_download_sflash.bCheck = pBinFile->bCheck;
    m_download_sflash.RemainSectorCnt = (pBinFile->data_length + m_spi_burn.PageSize - 1) / m_spi_burn.PageSize;
    m_download_sflash.bUpdateSelf = pBinFile->bUpdateSelf;
    
    m_wirtebin_flag = AK_TRUE; //此标志是作当是升级烧录时，如果存在没有写BIN，那么就不进行写bin区信息
    //is new burn mode
    if (MODE_NEWBURN != g_burn_param.eMode)
    {
         
        if (!SPI_get_pdate_param(pBinFile->file_name, pBinFile->data_length))
        {
            /*获取SPI升级参数失败*/
            return FHA_FAIL;
        }

        pfile_cfg = m_download_sflash.pFileInfo + m_download_sflash.FileCnt;
        m_download_sflash.FileCnt++;
    }
    else
    {
        pfile_cfg = m_download_sflash.pFileInfo + m_download_sflash.FileCnt;
        m_download_sflash.FileCnt++;
        //is need to backup
        if (pBinFile->bBackup)
        {
            if (!m_download_sflash.bBackUP)
            {
                if (m_download_sflash.FileStartPage != m_spi_burn.BinPageStart)
                {
                    gFHAf.Printf("FHA_S not support backup burn, is not first BIN backup burn\r\n");
                    return FHA_FAIL;
                }

                if (m_spi_burn.PagesPerBlock == T_U32_MAX || (m_spi_burn.FSPartStart % m_spi_burn.PagesPerBlock != 0))
                {
                    gFHAf.Printf("FHA_S backup burn init para ERR: FSPartStart:%d, PagePerBlock:%d\r\n", m_spi_burn.FSPartStart, m_spi_burn.PagesPerBlock);
                    return FHA_FAIL;
                }

                m_download_sflash.bBackUP = AK_TRUE;

                PageStart = (m_spi_burn.BinPageStart + m_spi_burn.PagesPerBlock - 1) / m_spi_burn.PagesPerBlock;
                /*偏移到下一个block的开始*/
                m_spi_burn.BinPageStart = (PageStart + 1) * m_spi_burn.PagesPerBlock;
                m_download_sflash.FileStartPage = m_spi_burn.BinPageStart;
            }

            page_cnt  = m_download_sflash.RemainSectorCnt + SPI_extend_page_cnt(pBinFile->data_length, AK_TRUE, 0);
            block_cnt = (page_cnt + m_spi_burn.PagesPerBlock - 1) / m_spi_burn.PagesPerBlock;
            TotSecCnt = block_cnt * m_spi_burn.PagesPerBlock;
        }
        else
        {
            if (m_download_sflash.bBackUP)
            {
                block_cnt = (m_download_sflash.RemainSectorCnt + m_spi_burn.PagesPerBlock - 1) / m_spi_burn.PagesPerBlock;
                TotSecCnt = block_cnt * m_spi_burn.PagesPerBlock;
                block_cnt = 0;
            }
            else
            {
                TotSecCnt = m_download_sflash.RemainSectorCnt;
            }
        }

        m_download_sflash.TotSecCnt = block_cnt * m_spi_burn.PagesPerBlock;
        m_download_sflash.FileBackupPage = m_download_sflash.FileStartPage + TotSecCnt;
        
        if(m_download_sflash.FileCnt > FILE_MAX_NUM)
        {
            gFHAf.Printf("FHA_S file num excess %d\r\n", FILE_MAX_NUM);
            return FHA_FAIL;
        }

        pfile_cfg->start_page  = m_download_sflash.FileStartPage;
        if (pBinFile->bBackup)
        {
            pfile_cfg->backup_page = m_download_sflash.FileBackupPage;
        }
        else
        {
            pfile_cfg->backup_page = T_U32_MAX;
        }
        
        gFHAf.MemCpy(pfile_cfg->file_name, pBinFile->file_name, 16);
    }
    
    pfile_cfg->file_length = pBinFile->data_length;
    pfile_cfg->ld_addr = pBinFile->ld_addr;

    gFHAf.Printf("FHA_S start page:%d", pfile_cfg->start_page);
    gFHAf.Printf(", Total SecCnt:%d, Write SecCnt:%d\r\n", 
        m_download_sflash.TotSecCnt,
        m_download_sflash.RemainSectorCnt);

    return FHA_SUCCESS;
}

/**
 * @BREIF    SPI_WriteData
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @PARAM    
 * @PARAM    
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 SPI_WriteData(const T_U8 *pData, T_U32 data_len, T_U32 StartPage, T_U32 page_cnt)
{
    T_U32 i;
    T_U8 *pBuf = AK_NULL;
    
    if(FHA_SUCCESS != Sflash_BurnWritePage(pData, StartPage, page_cnt))
    {
        return FHA_FAIL;
    }
    else
    {    
        if (m_download_sflash.bCheck)
        {
            //malloc buffer to read data
            pBuf = gFHAf.RamAlloc(page_cnt * m_spi_burn.PageSize);

            if(AK_NULL == pBuf)
            {
                return FHA_FAIL;
            }

            if(FHA_SUCCESS != Prod_SPIReadPage(StartPage, page_cnt, pBuf))
            {
                gFHAf.Printf("FHA_S compare fail at page start:%d, cnt:%d\r\n", StartPage, page_cnt);
                gFHAf.RamFree(pBuf);
                return FHA_FAIL;
            }

            if(0 != gFHAf.MemCmp(pData, pBuf, data_len))
            {
                gFHAf.Printf("FHA_S compare fail\r\n");
                for (i=0; i<data_len; i++)
                {
                    //if (!(i%16))
                    //    gFHAf.Printf("\r\n"); 
                    if (pData[i] != pBuf[i])
                    {
                        gFHAf.Printf("St:%d, i:%d,S:%02x_D:%02x\r\n", StartPage, i, pData[i],pBuf[i]);
                    }
                }
                
                gFHAf.Printf("\r\n");    
                
                gFHAf.RamFree(pBuf);
                return FHA_FAIL;
            }

            gFHAf.RamFree(pBuf);
        }
    }

    return FHA_SUCCESS;
}

/**
 * @BREIF    write data buffer
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-12-10
 * @PARAM    [in] data buffer
 * @PARAM    [in] data buffer length
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 Sflash_BurnWriteData(const T_U8 *pData, T_U32 data_len)
{
    //T_U32 i;
    T_U32 page_cnt;
    T_U32 page_size = m_spi_burn.PageSize;
    T_U32* pStartPage;
    T_U8* pBuf = AK_NULL;
    T_SPI_FILE_CONFIG* pfile_cfg = AK_NULL;
    T_U32 firsttime_startpage = 0;

     //page of starting to write 
    pStartPage = m_download_sflash.bBootData ? &(m_download_sflash.BootDataPageIndex)
                    : &(m_download_sflash.FileStartPage);

    page_cnt = (data_len + page_size - 1) / page_size;

    if (!m_download_sflash.bBootData)
        pfile_cfg = m_download_sflash.pFileInfo + m_download_sflash.FileCnt - 1;

    gFHAf.Printf("FHA_S start page:%d_cnt:%d\r\n", *pStartPage, page_cnt);
    if (page_cnt > m_download_sflash.RemainSectorCnt)
    {
        gFHAf.Printf("FHA_S write SPI bin data_len is too large\r\n");
        return FHA_FAIL;
    }
    else if (page_cnt < m_download_sflash.RemainSectorCnt)
    {
        if (data_len != page_size * page_cnt)
        {
            gFHAf.Printf("FHA_S write SPI bin data_len is not page multiple\r\n");
            return FHA_FAIL;
        }
    }
    else if (m_download_sflash.bBootData) //spi boot 
    {
         if (page_cnt != m_download_sflash.RemainSectorCnt)
        {
            gFHAf.Printf("FHA_S write SPI boot data_len is error!\r\n");
            return FHA_FAIL;
        }
    }
    else
    {
        if (0 == m_download_sflash.FileCnt)
        {
            gFHAf.Printf("FHA_S write SPI bin FileCnt is 0!\r\n");
            return FHA_FAIL;
        }

        if ((data_len % page_size) != (pfile_cfg->file_length % page_size))
        {
            gFHAf.Printf("FHA_S write SPI bin data_len is error!\r\n");
            return FHA_FAIL;
        }        
    }

    if (FHA_SUCCESS != SPI_WriteData(pData, data_len, *pStartPage, page_cnt))
    {
        return FHA_FAIL;
    }

    if (m_download_sflash.bBackUP && m_download_sflash.TotSecCnt != 0)
    {
        /*只有全新烧录的时候，才对backup区写入*/
        if (FHA_SUCCESS != SPI_WriteData(pData, data_len, m_download_sflash.FileBackupPage, page_cnt))
        {
            return FHA_FAIL;
        }  
        m_download_sflash.FileBackupPage += page_cnt;
    }

    *pStartPage += page_cnt;
    m_download_sflash.RemainSectorCnt -= page_cnt;

    if (0 == m_download_sflash.RemainSectorCnt)
    {
        if (!m_download_sflash.bBootData 
            && m_download_sflash.bBackUP
            && MODE_NEWBURN == g_burn_param.eMode)
        {
            *pStartPage = pfile_cfg->backup_page + m_download_sflash.TotSecCnt;
            gFHAf.Printf("end:start_page:%d, backup_page:%d, tot_cnt:%d\r\n", *pStartPage, pfile_cfg->backup_page, m_download_sflash.TotSecCnt);
        }

        //如果有设置bin大小，那么增加SPI_extend_page_cnt
        if(!m_download_sflash.bBootData && m_download_sflash.extern_size != 0 
            && MODE_NEWBURN == g_burn_param.eMode && pfile_cfg->backup_page == T_U32_MAX)
        {
            //第一个bin文件烧录时，增加记录信息的二个页，这样算起第一个文件以块为单位
            if(g_firstbin_burn == AK_TRUE && PLAT_LINUX != g_burn_param.ePlatform)
            {
                firsttime_startpage = 2;
                g_firstbin_burn = AK_FALSE;
            }
            else
            {
                firsttime_startpage = 0;
            }
            *pStartPage = *pStartPage + SPI_extend_page_cnt(pfile_cfg->file_length, AK_FALSE, firsttime_startpage); //算起
            gFHAf.Printf("*pStartPage：%d\r\n", *pStartPage);
        }
        

        if(PLAT_LINUX == g_burn_param.ePlatform)
        {
            *pStartPage = ((*pStartPage*m_spi_burn.PageSize + m_spi_burn.PagesPerBlock*m_spi_burn.PageSize - 1)/(m_spi_burn.PagesPerBlock*m_spi_burn.PageSize));//这里以块对齐
            gFHAf.Printf("block num：%d\r\n", *pStartPage);
            *pStartPage = *pStartPage*m_spi_burn.PagesPerBlock;//这里算出占多少个页
            gFHAf.Printf("FHA_S write SPI next bin pStartPage：%d\r\n", *pStartPage);
        }
        else
        {
            //如果块以4K为大小的，那么以4K对齐
            if(m_spi_burn.PagesPerBlock*m_spi_burn.PageSize == 4096)
            {
                *pStartPage = ((*pStartPage*m_spi_burn.PageSize + 4096 - 1)/4096);//这里以4K对齐
                gFHAf.Printf("4K num：%d\r\n", *pStartPage);
                *pStartPage = *pStartPage*4096/m_spi_burn.PageSize;//这里算出占多少个页
                gFHAf.Printf("FHA_S write SPI bin pStartPage：%d\r\n", *pStartPage);
            }
        }

        if (MODE_UPDATE_SELF == g_burn_param.eMode)
        {
            gFHAf.Printf("FHA_S write SPI bin update config\r\n");
            if (Sflash_BurnUpdateConfig() != FHA_SUCCESS)
            {
                return FHA_FAIL;
            }
        }
    }
    
    return FHA_SUCCESS;
}

/**
 * @BREIF    Sflash_BurnUpdateConfig
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32 Sflash_BurnUpdateConfig(T_VOID)
{
    T_U32 page_size = m_spi_burn.PageSize;
    T_U32 ret = FHA_SUCCESS;
    T_U8 *pBuf = AK_NULL;

    //gFHAf.Printf("FHA_S WriteConfig!\r\n");
    pBuf = gFHAf.RamAlloc(page_size);
    
    if (AK_NULL == pBuf)
    {
        return FHA_FAIL;
    }

    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        m_download_sflash.FileTotalCnt = m_download_sflash.FileCnt;
    }
    
    if(m_download_sflash.FileTotalCnt>0 && m_download_sflash.pFileInfo != AK_NULL)
    {
        gFHAf.Printf("FHA_S write fileinfo, count:%d, BinInfoStart: %d\r\n", m_download_sflash.FileTotalCnt, m_spi_burn.BinInfoStart);
        gFHAf.MemCpy(pBuf, &m_download_sflash.FileTotalCnt, 4);
        gFHAf.MemCpy(pBuf+4, m_download_sflash.pFileInfo, page_size - 4);
        if(FHA_SUCCESS != Sflash_BurnWritePage(pBuf, m_spi_burn.BinInfoStart, 1))
        {
            gFHAf.Printf("FHA_S write fileinfo fail\r\n");
            ret = FHA_FAIL;
        }
    }

    gFHAf.RamFree(pBuf);
    
    return ret;
}

/**
 * @BREIF    read bin info
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 Sflash_BurnWriteConfig(T_VOID)
{
    T_U32 ret = FHA_SUCCESS;

    if(m_wirtebin_flag == AK_TRUE)
    {
        ret = Sflash_BurnUpdateConfig();
    }
    
    if (m_download_sflash.pFileInfo != AK_NULL)
    {
        gFHAf.RamFree(m_download_sflash.pFileInfo);
        m_download_sflash.pFileInfo = AK_NULL;
    }

    if (m_download_sflash.BlockMap != AK_NULL)
    {
        gFHAf.RamFree(m_download_sflash.BlockMap);
        m_download_sflash.BlockMap = AK_NULL;
    }

    if (ret != FHA_FAIL)
    {
        gFHAf.Printf("FHA_S write config success!\r\n");
    }
    else
    {
        gFHAf.Printf("FHA_S write config fail!\r\n");
    }
    m_wirtebin_flag = AK_FALSE;
    
    return ret;
}

/**
 * @BREIF    Sflash_BurnWritePage
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32 Sflash_BurnWritePage(const T_U8 *pData, T_U32 startPage, T_U32 PageCnt)
{
    T_U32 i; 
//    T_U32 write_block_index;
    T_U32 BlockStart, BlockEnd;

#if 1
    BlockStart = startPage / m_spi_burn.PagesPerBlock;
    BlockEnd = (startPage + PageCnt) / m_spi_burn.PagesPerBlock;
    
    gFHAf.Printf("FHA_S block:%d, %d\r\n", BlockStart, BlockEnd);

    for (i=BlockStart; i<=BlockEnd; i++)
    {
        gFHAf.Printf("FHA_S block:%d\r\n", i);
        if((i == BlockEnd) && ((startPage + PageCnt) % m_spi_burn.PagesPerBlock) == 0)
        {
            break;
        }
        else
        {
            if (!TestBlockMap(m_download_sflash.BlockMap, i))
            {
                gFHAf.Printf("FHA_S erase:%d\r\n", i);
                if (FHA_SUCCESS != Prod_SPIErase(0, i))
                {
                    gFHAf.Printf("FHA_S erase fail at block:%d\r\n", i);
                    return FHA_FAIL;
                } 

                SetBlockMap(m_download_sflash.BlockMap, i);
            }
        }
    } 
#else
    write_block_index = (startPage + PageCnt) / m_spi_burn.PagesPerBlock;

    if (m_download_sflash.EraseBlocks <= write_block_index)
    {
        for (i=m_download_sflash.EraseBlocks; i<=write_block_index; i++)
        {
            gFHAf.Printf("FHA_S erase:%d\r\n", i);
            if (FHA_SUCCESS != Prod_SPIErase(0, i))
            {
                gFHAf.Printf("FHA_S erase fail at block:%d\r\n", i);
                return FHA_FAIL;
            } 
        } 

        m_download_sflash.EraseBlocks = write_block_index + 1;
    }
#endif

    if(FHA_SUCCESS != Prod_SPIWritePage(startPage, PageCnt, pData))
    {
        gFHAf.Printf("FHA_S write fail at page start:%d, cnt:%d\r\n", startPage, PageCnt);
        return FHA_FAIL;
    }

    return FHA_SUCCESS;
}

/**
 * @BREIF    Sflash_BurnReadPage
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 Sflash_BurnReadPage(T_U8* buf, T_U32 startPage, T_U32 PageCnt)
{
    if(FHA_SUCCESS != Prod_SPIReadPage(startPage, PageCnt, buf))
    {
        gFHAf.Printf("FHA_S read fail at page start:%d, cnt:%d\r\n", startPage, PageCnt);
        return FHA_FAIL;
    }

    return FHA_SUCCESS;
}

/************************************************************************
 * NAME:     FHA_read_bin_begin
 * FUNCTION  set read bin start init para
 * PARAM:    [in] bin_param--Bin file info struct
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

//read the boot len
T_U32 Sflash_GetBootStart(T_PFHA_BIN_PARAM bin_param)
{ 
    T_U32 boot_pagenum = bin_param->data_length; //外面传boot文件的占用pagenum
    
     if(0 == fha_str_cmp(bin_param->file_name, BOOT_NAME))
    {
        bin_param->data_length  = boot_pagenum*m_spi_burn.PageSize;  
    }
     
    return FHA_SUCCESS;  
}


//read the boot data
T_U32 Sflash_GetBootData(T_U8* pBuf, T_U32 data_len)
{

    T_U32 page_cnt;
    T_U32 page_size = m_spi_burn.PageSize;

    page_cnt = (data_len + page_size - 1) / page_size;

    if(FHA_SUCCESS != Prod_SPIReadPage(g_spiboot_startpage, page_cnt, pBuf))
    {
        gFHAf.Printf("FHA_S get fail at page start:%d, cnt:%d\r\n", g_spiboot_startpage, page_cnt);
        return FHA_FAIL;
    }
    g_spiboot_startpage = g_spiboot_startpage + page_cnt;
    
    return FHA_SUCCESS; 
}



T_U32 Sflash_GetBinStart(T_PFHA_BIN_PARAM bin_param)
{
    T_U32 i;
    T_U32 fileIndex = 0;
    T_SPI_FILE_CONFIG *pFileConfig;
    T_U32 file_count;
    T_U8  *pbuf = AK_NULL;
    
    pbuf = gFHAf.RamAlloc(m_spi_burn.PageSize);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    }        

    if(FHA_SUCCESS != Prod_SPIReadPage(m_spi_burn.BinInfoStart, 1, pbuf))
    {
        gFHAf.Printf("FHA_S G_B_S read fail\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    gFHAf.MemCpy(&file_count, pbuf, 4);

    if (file_count == 0 || file_count > FILE_MAX_NUM)
    {
        gFHAf.Printf("FHA_S G_B_S file num excess\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;        
    }
    
    pFileConfig = (T_SPI_FILE_CONFIG *)(pbuf + 4);
    
    for(i = 0; i < file_count; i++)
    {   
        if (bin_param->file_name[0] < file_count)
        {
            if (i == bin_param->file_name[0])
            {
                gFHAf.MemSet(bin_param, 0, sizeof(T_FHA_BIN_PARAM));
                gFHAf.MemCpy(bin_param->file_name, pFileConfig[i].file_name, sizeof(bin_param->file_name));
                fileIndex = i;
                break;
            }
        }
        else if(0 == fha_str_cmp(bin_param->file_name, pFileConfig[i].file_name))
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
    
    bin_param->data_length = pFileConfig[fileIndex].file_length;
    bin_param->ld_addr     = pFileConfig[fileIndex].ld_addr;
    m_GetBin_SPI_Page_Start = pFileConfig[fileIndex].start_page;
    
    gFHAf.RamFree(pbuf);

    return FHA_SUCCESS;    
}


/************************************************************************
 * NAME:     FHA_read_bin
 * FUNCTION  read bin to buf from medium
 * PARAM:    [out]pData-----need to read bin data buf pointer addr
 *           [in] data_len--need to read bin data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 Sflash_GetBinData(T_U8* pBuf, T_U32 data_len)
{
    T_U32 page_cnt;
    T_U32 page_size = m_spi_burn.PageSize;

    page_cnt = (data_len + page_size - 1) / page_size;

    if(FHA_SUCCESS != Prod_SPIReadPage(m_GetBin_SPI_Page_Start, page_cnt, pBuf))
    {
        gFHAf.Printf("FHA_S get fail at page start:%d, cnt:%d\r\n", m_GetBin_SPI_Page_Start, page_cnt);
        return FHA_FAIL;
    }

    m_GetBin_SPI_Page_Start += page_cnt;
       
    return FHA_SUCCESS; 
}


/************************************************************************
 * NAME:     Sflash_GetFSPart
 * FUNCTION  get fs partition info
 * PARAM:    [out]pInfoBuf---need to get fs info data pointer addr
 *           [in] data_len---need to get fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_GetFSPart(T_U8 *pInfoBuf, T_U32 buf_len)
{
    T_U8 *pBuf = AK_NULL;
    T_U32 ret;

    pBuf = gFHAf.RamAlloc(m_spi_burn.PageSize);
    if (AK_NULL == pBuf)
    {
        return FHA_FAIL;
    }
    
    m_GetBin_SPI_Page_Start = m_spi_burn.FSPartStart;
    gFHAf.Printf("FHA_S G_P_S:%d \n", m_GetBin_SPI_Page_Start);
    
    ret = Sflash_GetBinData(pBuf, m_spi_burn.PageSize);
    gFHAf.MemCpy(pInfoBuf, pBuf, buf_len);
    gFHAf.RamFree(pBuf);

    return ret;
}

/************************************************************************
 * NAME:     Sflash_SetFSPart
 * FUNCTION  set fs partition info to medium 
 * PARAM:    [in] pInfoBuf-----need to write fs info data pointer addr
 *           [in] data_len--need to write fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
/*
T_U32 Sflash_SetFSPart(const T_U8 *pInfoBuf, T_U32 buf_len)
{
    m_download_sflash.bBootData = AK_FALSE;
    m_download_sflash.bCheck    = AK_TRUE;
    m_download_sflash.FileStartPage = m_spi_burn.FSPartStart;
    gFHAf.Printf("FHA_S S_P_S:%d \n", m_download_sflash.FileStartPage);

    return Sflash_BurnWriteData(pInfoBuf, m_spi_burn.PageSize);
}
*/

//此接口根据spi镜像烧录进行修改的
T_U32 Sflash_SetFSPart(const T_U8 *pInfoBuf, T_U32 buf_len)
{
    m_download_sflash.bBootData = AK_FALSE;
    m_download_sflash.bCheck    = AK_TRUE;
    m_download_sflash.FileStartPage = m_spi_burn.FSPartStart;
    gFHAf.Printf("FHA_S S_P_S:%d \n", m_download_sflash.FileStartPage);

    return SPI_WriteData(pInfoBuf, buf_len, m_download_sflash.FileStartPage, 1);
}


/************************************************************************
 * NAME:     FHA_get_bin_num
 * FUNCTION  get bin file number
 *           [out] cnt----bin file count in medium
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 Sflash_GetBinNum(T_U32 *cnt)
{
    T_U32 TotalSize = 0;
    T_U32 file_count;
    T_U8  *pbuf = AK_NULL;

    if (AK_NULL == cnt)
    {
        return FHA_FAIL;
    }
    
    pbuf = gFHAf.RamAlloc(m_spi_burn.PageSize);
    if (AK_NULL == pbuf)
    {
        return 0;
    }        

    if(FHA_SUCCESS != Prod_SPIReadPage( m_spi_burn.BinInfoStart, 1, pbuf))
    {
        gFHAf.Printf("FHA_S G_B_N read fail\r\n");
        gFHAf.RamFree(pbuf);
        return 0;
    }

    gFHAf.MemCpy(&file_count, pbuf, 4);

    if (file_count == 0 || file_count > FILE_MAX_NUM)
    {
        gFHAf.Printf("FHA_S G_B_N SPI no binfile\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;  

    }

    *cnt = file_count;
    gFHAf.RamFree(pbuf);

    return FHA_SUCCESS;  

}

/************************************************************************
 * NAME:     Sflash_set_bin_resv_size
 * FUNCTION  set write bin reserve size(unit byte)
 * PARAM:    [in] bin_size--reserve bin size
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 Sflash_set_bin_resv_size(T_U32 bin_size)
{
    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        if(PLAT_LINUX == g_burn_param.ePlatform)
        {
            gFHAf.Printf("m_download_sflash.extern_size(k):%d\r\n", bin_size);
            m_download_sflash.extern_size = bin_size*1024;//以K为单位
            
        }
        else
        {
            gFHAf.Printf("m_download_sflash.extern_size(M):%d\r\n", bin_size);
            m_download_sflash.extern_size = bin_size*1024*1024;//以M为单位 
            
        }
        gFHAf.Printf("m_download_sflash.extern_size(B):%d\r\n", m_download_sflash.extern_size);
    }
    
    return FHA_SUCCESS;
}

/**
 * @BREIF    SPI_extend_page_cnt
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32 SPI_extend_page_cnt(T_U32 bin_len, T_BOOL flag, T_U32 infopagecnt)
{
    T_U32 ret = 0; 
    T_U32 page_size = m_spi_burn.PageSize;
    T_U32 extern_size_agenum = 0;
    T_U32 bin_len_pagenum = 0; 
    
    if (0 == m_download_sflash.extern_size)
    {
        if(AK_TRUE == flag)
        {
            return ((bin_len * SPI_SPARE_RATIO / 100 + page_size - 1) / page_size);
        }
        else
        {
            return 0;
        }
    }
    else
    {
        extern_size_agenum = (m_download_sflash.extern_size + page_size - 1) / page_size - infopagecnt;
        bin_len_pagenum = (bin_len + page_size - 1) / page_size;
        if(extern_size_agenum < bin_len_pagenum)
        {
            if(AK_TRUE == flag)
            {
                ret = (bin_len * SPI_SPARE_RATIO / 100 + page_size - 1) / page_size;
            }
            else
            {
                ret = 0;
            }
        }
        else
        {
            ret = extern_size_agenum - bin_len_pagenum;
        }
        
        //ret =(m_download_sflash.extern_size - bin_len + page_size - 1) / page_size;
        m_download_sflash.extern_size = 0;
        
        return ret;
    }
}  

/**
 * @BREIF    SPI_Read_Update_Info
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL SPI_Read_Update_Info(T_VOID)
{
    if (MODE_NEWBURN != g_burn_param.eMode)
    {
        T_U8 *pbuf;

        pbuf = gFHAf.RamAlloc(m_spi_burn.PageSize);
        if (AK_NULL == pbuf)
        {
            return FHA_FAIL;
        }

        gFHAf.MemSet(pbuf, 0x0, m_spi_burn.PageSize);
        
        gFHAf.Printf("FHA SPI_Read_Update_Info read m_spi_burn.BinInfoStart:%d \r\n", m_spi_burn.BinInfoStart);
        if(FHA_SUCCESS != Prod_SPIReadPage(m_spi_burn.BinInfoStart, 1, pbuf))
        {
            gFHAf.RamFree(pbuf);
            gFHAf.Printf("FHA SPI_Read_Update_Info read fail\r\n");
            return AK_FALSE;
        }

        gFHAf.MemCpy(&m_download_sflash.FileTotalCnt, pbuf, 4);
        gFHAf.MemCpy(m_download_sflash.pFileInfo, pbuf + 4, m_spi_burn.PageSize - 4);
        if (m_download_sflash.FileTotalCnt == 0 || m_download_sflash.FileTotalCnt > FILE_MAX_NUM)
        {
            gFHAf.RamFree(pbuf);
            gFHAf.Printf("FHA SPI_Read_Update_Info file num excess:%d\r\n", m_download_sflash.FileTotalCnt);
            return AK_FALSE;
        }
        
        gFHAf.RamFree(pbuf);
    }
    
    return AK_TRUE;    
}

/**
 * @BREIF    SPI_get_pdate_param
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL SPI_get_pdate_param(T_U8 strName[], T_U32 file_len)
{
    T_U32 i;
    T_U32 fileIndex = 0;
    T_U32 page_size = m_spi_burn.PageSize;

    for(i = 0; i < m_download_sflash.FileTotalCnt; i++)
    {            
        if(0 == fha_str_cmp(strName, m_download_sflash.pFileInfo[i].file_name))
        {
             fileIndex = i;
             break;
        }     
    }
    
    if (i == m_download_sflash.FileTotalCnt)
    {
        /*没有找到需要升级的文件*/
        gFHAf.Printf("FHA SPI_get_pdate_param: no find update file\r\n");
        return AK_FALSE;
    }
    else
    {
        T_U32 file_page_cnt;
        T_U32 space_page_cnt;

        if (T_U32_MAX == m_download_sflash.pFileInfo[fileIndex].backup_page)
        {
            /*没有备份区*/
            gFHAf.Printf("FHA SPI_get_pdate_param: BIN file no backup area\r\n");

            //如果没有备份区，那么升级时把数据写到原区域内
            //return AK_FALSE;
        }

        m_download_sflash.FileCnt = fileIndex;
            
        if (m_download_sflash.bUpdateSelf && m_download_sflash.pFileInfo[fileIndex].backup_page != T_U32_MAX)
        {
            T_U32 tmp = m_download_sflash.pFileInfo[fileIndex].start_page;

            m_download_sflash.pFileInfo[fileIndex].start_page  = m_download_sflash.pFileInfo[fileIndex].backup_page;
            m_download_sflash.pFileInfo[fileIndex].backup_page = tmp;   
        }  
        
        m_download_sflash.FileStartPage  = m_download_sflash.pFileInfo[fileIndex].start_page;
        m_download_sflash.FileBackupPage = m_download_sflash.pFileInfo[fileIndex].backup_page;
        gFHAf.Printf("FHA SPI_get_pdate_param:%d, bUpdateSelf: FileStartPage:%d, FileBackupPage:%d \r\n",
            m_download_sflash.bUpdateSelf,
            m_download_sflash.FileStartPage,
            m_download_sflash.FileBackupPage);

        if (m_download_sflash.FileBackupPage > m_download_sflash.FileStartPage)
        {
            space_page_cnt = m_download_sflash.FileBackupPage - m_download_sflash.FileStartPage;
        }
        else
        {
            space_page_cnt = m_download_sflash.FileStartPage - m_download_sflash.FileBackupPage;
        } 

        if (m_download_sflash.bUpdateSelf)
        {
            /*自升级不破坏backup的数据*/
            m_download_sflash.TotSecCnt = 0;
        }
        else
        {
            m_download_sflash.TotSecCnt = space_page_cnt;
        }
        
        file_page_cnt = (file_len + page_size - 1) / page_size;

        if (file_page_cnt > space_page_cnt)
        {
            gFHAf.Printf("FHA_S update data size is too large\r\n");
            return AK_FALSE;
        }    
    }

    return AK_TRUE; 
}



T_U32 SPIBurn_asa_write_file(T_U8 file_name[], const T_U8 dataBuf[], T_U32 dataLen, T_U8 mode)
{
    T_U32* pStartPage;
    T_U32 page_size = m_spi_burn.PageSize;
    T_U32 page_cnt;
    T_U32 Block = 0;
    T_U8 *pbuf = AK_NULL;
    T_SPI_FILE_CONFIG*  pFileInfo = AK_NULL;
    T_U32 file_num = 0, i = 0, j = 0;
    T_BOOL bMatch = AK_FALSE;
    
    if(AK_NULL == file_name || AK_NULL == dataBuf)
    {
        return ASA_FILE_FAIL;
    }

    if (fha_str_len(file_name) >= 8)
    {
        gFHAf.Printf("SPIBurn_asa_write_file(): file name more than 8!\r\n");        
        return ASA_FILE_FAIL;
    }

    if(PLAT_LINUX == g_burn_param.ePlatform && MODE_NEWBURN == g_burn_param.eMode)
    {
        //全新烧录情况下，进行先把bin文件信息的一块进行先擦
       Block = m_spi_burn.BinInfoStart / m_spi_burn.PagesPerBlock;
        gFHAf.Printf("FHA_S  BinInfoStart block:%d, %d\r\n", Block);

        if (!TestBlockMap(m_download_sflash.BlockMap, Block))
        {
            gFHAf.Printf("FHA_S erase:%d\r\n", Block);
            if (FHA_SUCCESS != Prod_SPIErase(0, Block))
            {
                gFHAf.Printf("FHA_S erase fail at block:%d\r\n", Block);
                return FHA_FAIL;
            }
            SetBlockMap(m_download_sflash.BlockMap, Block);
        }
    }

    //进行读取bin信息
    pbuf = gFHAf.RamAlloc(m_spi_burn.PageSize);
    if (AK_NULL == pbuf)
    {
        return ASA_FILE_FAIL;
    }
    gFHAf.MemSet(pbuf, 0x0, m_spi_burn.PageSize);
    
    gFHAf.Printf("SPIBurn_asa_write_file.BinInfoStart:%d , dataLen:%d, mode:%d\r\n", m_spi_burn.BinInfoStart, dataLen, mode);
    if(FHA_SUCCESS != Prod_SPIReadPage(m_spi_burn.BinInfoStart, 1, pbuf))
    {
        gFHAf.RamFree(pbuf);
        gFHAf.Printf("FHA SPI_Read_Update_Info read fail\r\n");
        return ASA_FILE_FAIL;
    }
   
    gFHAf.MemCpy(&file_num, pbuf, 4);
    gFHAf.Printf("file_num:%x\r\n", file_num);
    if(T_U32_MAX == file_num)  
    {
        //烧录时,由于spi已被擦,那么读出来应是全ff
        pFileInfo = m_download_sflash.pFileInfo + m_download_sflash.FileCnt;
        m_download_sflash.FileCnt++;
        gFHAf.MemCpy(pFileInfo->file_name, file_name, 8);
        pFileInfo->file_length = dataLen;
        pFileInfo->ld_addr = 0;
        pFileInfo->backup_page = T_U32_MAX;
        pFileInfo->start_page = m_download_sflash.FileStartPage;
        pStartPage = &(m_download_sflash.FileStartPage);
    }
    else
    {
        //当平台重启时进行写 才会进到这里来.
        //所以此时是不需要进行写bin文件信息的.
        //m_wirtebin_flag = AK_FALSE;
       
        pFileInfo = (T_SPI_FILE_CONFIG *)(pbuf + 4);

        //查找是否已有文件存在
        for(i=0; i < file_num; i++)
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
                    //文件存在的话,以open方式就不进行写
                    gFHAf.RamFree(pbuf);
                    pbuf = AK_NULL;
                    return ASA_FILE_EXIST;
                }
                else
                {
                   break;
                }
            }    
            else
            {
                //continue to search
                pFileInfo++;
            }
        }
        
        if(!bMatch) //没有相应的文件
        {
            gFHAf.Printf("FHA SPIBurn_asa_write_file not the file\r\n");
            gFHAf.RamFree(pbuf);
            pbuf = AK_NULL;
            return ASA_FILE_FAIL;
        }
        else //有此文件 
        {
            gFHAf.Printf("find file num:%x\r\n", j);
            pFileInfo->file_length = dataLen;
            pStartPage = &(pFileInfo->start_page);
            
        }
        
    }

    //记录信息
    gFHAf.Printf("FHA_S file_name: %s\n",    pFileInfo->file_name);
    gFHAf.Printf("FHA_S file_length: %d\n",  pFileInfo->file_length);
    gFHAf.Printf("FHA_S start_page: %d\n",   pFileInfo->start_page);

    page_cnt = (dataLen + page_size - 1) / page_size;
    
    if (FHA_SUCCESS != SPI_WriteData(dataBuf, dataLen, *pStartPage, page_cnt))
    {
        gFHAf.RamFree(pbuf);
        pbuf = AK_NULL;
        gFHAf.Printf("*SPI_WriteData fail\r\n");
        return ASA_FILE_FAIL;
    }

    //记录下一个页
    *pStartPage += page_cnt;
    gFHAf.Printf("*pStartPage:%x\r\n", *pStartPage);

    
    if(PLAT_LINUX == g_burn_param.ePlatform)
    {
        *pStartPage = ((*pStartPage*m_spi_burn.PageSize + m_spi_burn.PagesPerBlock*m_spi_burn.PageSize- 1)/(m_spi_burn.PagesPerBlock*m_spi_burn.PageSize));//这里块对齐
        gFHAf.Printf("block num：%d\r\n", *pStartPage);
        *pStartPage = *pStartPage*m_spi_burn.PagesPerBlock;//这里算出占多少个页
        gFHAf.Printf("FHA_S write SPI asa pStartPage：%d\r\n", *pStartPage);
    }
    else
    {
        //以4K对齐
        if(m_spi_burn.PagesPerBlock*m_spi_burn.PageSize == 4096)
        {
            *pStartPage = ((*pStartPage*m_spi_burn.PageSize + 4096 - 1)/4096);//这里以4K对齐
            gFHAf.Printf("4K num：%d\r\n", *pStartPage);
            *pStartPage = *pStartPage*4096/m_spi_burn.PageSize;//这里算出占多少个页
            gFHAf.Printf("FHA_S write SPI asa pStartPage：%d\r\n", *pStartPage);
        }
    }
    gFHAf.RamFree(pbuf);
    pbuf = AK_NULL;
    
    return ASA_FILE_SUCCESS;
}


T_U32 SPIBurn_asa_read_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen)
{
    T_U32 pStartPage;
    T_U32 page_size = m_spi_burn.PageSize;
    //T_U32 page_cnt;
    T_SPI_FILE_CONFIG* pfile_cfg = AK_NULL;
    T_U8 *pbuf = AK_NULL;
    T_SPI_FILE_CONFIG*  pFileInfo;
    T_U32 file_num = 0, i = 0, j = 0;
    T_BOOL bMatch = AK_FALSE;
    
    if(AK_NULL == file_name || AK_NULL == dataBuf)
    {
        return ASA_FILE_FAIL;
    }

    if (fha_str_len(file_name) >= 8)
    {
        gFHAf.Printf("SDBurn_write_file(): file name overlong!\r\n");        
        return ASA_FILE_FAIL;
    }

    //进行读取是否已存
    pbuf = gFHAf.RamAlloc(m_spi_burn.PageSize);
    if (AK_NULL == pbuf)
    {
        return ASA_FILE_FAIL;
    }
    gFHAf.MemSet(pbuf, 0x0, m_spi_burn.PageSize);
    
    gFHAf.Printf("SPIBurn_asa_write_file.BinInfoStart:%d, dataLen:%d \r\n", m_spi_burn.BinInfoStart, dataLen);
    if(FHA_SUCCESS != Prod_SPIReadPage(m_spi_burn.BinInfoStart, 1, pbuf))
    {
        gFHAf.RamFree(pbuf);
        gFHAf.Printf("FHA SPI_Read_Update_Info read fail\r\n");
        return ASA_FILE_FAIL;
    }
   
    gFHAf.MemCpy(&file_num, pbuf, 4);
    gFHAf.Printf("SPIBurn_asa_read_file file_num:%x\r\n", file_num);
    if (file_num == 0 || file_num > FILE_MAX_NUM)
    {
        gFHAf.Printf("FHA_S G_B_S file num %d\r\n", file_num);
        gFHAf.RamFree(pbuf);
        return ASA_FILE_FAIL;        
    }

    pFileInfo = (T_SPI_FILE_CONFIG *)(pbuf + 4);
    //gFHAf.MemCpy(pFileInfo, pbuf + 4, m_spi_burn.PageSize - 4);
    

    //search asa file name
    for(i=0; i<file_num; i++)
    {
        bMatch = AK_TRUE;
        for(j=0; file_name[j] && (pFileInfo->file_name)[j]; j++)
        {
            if(file_name[j] != (pFileInfo->file_name)[j])
            {
                bMatch = AK_FALSE;
                break;
            }    
        }

        if(file_name[j] != (pFileInfo->file_name)[j])
        {
            bMatch = AK_FALSE;
        }

        //exist
        if(bMatch)
        {        
            pStartPage = pFileInfo->start_page;
            break;
        }    
        else
        {
            //continue to search
            pFileInfo++;
        }
    }

    gFHAf.Printf("SPIBurn_asa_read_file j:%d\r\n", j);


    gFHAf.MemSet(pbuf, 0x0, m_spi_burn.PageSize);
    
    if(!bMatch)
    {
        gFHAf.Printf("FHA SPIBurn_asa_read_file read fail\r\n");
        gFHAf.RamFree(pbuf);
        pbuf = AK_NULL;
        return ASA_FILE_FAIL;
    }
    else
    {
        //读取mac或serial
        //page_cnt = (dataLen + page_size - 1) / page_size;
        
        gFHAf.Printf("SPIBurn_asa_read_file pStartPage:%d\r\n", pStartPage);
        if(FHA_SUCCESS != Prod_SPIReadPage(pStartPage, 1, pbuf))
        {
            gFHAf.Printf("FHA_S get fail at page start:%d, cnt:1\r\n", pFileInfo->start_page);
            
            gFHAf.RamFree(pbuf);
            pbuf = AK_NULL;
            return FHA_FAIL;
        }
        
        gFHAf.MemCpy(dataBuf, pbuf, dataLen);
    }

    
    gFHAf.RamFree(pbuf);
    pbuf = AK_NULL;
    
    return ASA_FILE_SUCCESS;
}

//从第0而开始写一个spiflash空间大小的数据到spiflash上
T_U32 Sflash_BurnWrite_AllData_start(T_FHA_BIN_PARAM *pBinFile)
{
    //获取数据的长度
    m_download_sflash.bCheck = pBinFile->bCheck;
    m_download_sflash.RemainSectorCnt = (pBinFile->data_length + m_spi_burn.PageSize - 1) / m_spi_burn.PageSize;
    
    m_wirtebin_flag = AK_FALSE; //写整一个spi不需要写bin信息
    m_download_sflash.FileStartPage = 0;//从第0而开始烧录
    gFHAf.Printf("start page:%d_:%d_:%d\r\n", m_download_sflash.FileStartPage, pBinFile->data_length);

    //比较长度是否超过spiflash的大小
    //由于这里没有传总长度进来，所以只有在烧录工具上进行判断定
    
    return FHA_SUCCESS;
}

T_U32 Sflash_BurnWrite_AllData(const T_U8 *pData, T_U32 data_len)
{
    T_U32 page_cnt;
    T_U32 page_size = m_spi_burn.PageSize;
    T_U32* pStartPage;

     //page of starting to write 
    pStartPage = &(m_download_sflash.FileStartPage);

    page_cnt = (data_len + page_size - 1) / page_size;

    gFHAf.Printf("FHA_S start page:%d, cnt:%d, data_len:%d\r\n", *pStartPage, page_cnt, data_len);
    if (page_cnt > m_download_sflash.RemainSectorCnt)
    {
        gFHAf.Printf("FHA_S write SPI bin data_len is too large\r\n");
        return FHA_FAIL;
    }
    else if (page_cnt < m_download_sflash.RemainSectorCnt)
    {
        if (data_len != page_size * page_cnt)
        {
            gFHAf.Printf("FHA_S write SPI bin data_len is not page multiple\r\n");
            return FHA_FAIL;
        }
    } 
    
    //写数据和比较数据
    if (FHA_SUCCESS != SPI_WriteData(pData, data_len, *pStartPage, page_cnt))
    {
        return FHA_FAIL;
    }

    *pStartPage += page_cnt;
    m_download_sflash.RemainSectorCnt -= page_cnt;
    gFHAf.Printf("m_download_sflash.RemainSectorCnt:%d\r\n", m_download_sflash.RemainSectorCnt);
    
    return FHA_SUCCESS;
}


//从第0而开始写一个spiflash空间大小的数据到spiflash上
T_U32 Sflash_BurnWrite_MTD_start(T_FHA_BIN_PARAM *pBinFile)
{
    //获取数据的长度
    m_download_sflash.bCheck = pBinFile->bCheck;
    m_download_sflash.RemainSectorCnt = (pBinFile->data_length + m_spi_burn.PageSize - 1) / m_spi_burn.PageSize;

    m_wirtebin_flag = AK_TRUE; 
    gFHAf.Printf("start page:%d_:%d_:%d\r\n", m_download_sflash.RemainSectorCnt, m_download_sflash.FileStartPage, pBinFile->data_length);

    //比较长度是否超过spiflash的大小
    //由于这里没有传总长度进来，所以只有在烧录工具上进行判断定
    
    return FHA_SUCCESS;
}

T_U32 Sflash_BurnWrite_MTD(const T_U8 *pData, T_U32 data_len)
{
    if(FHA_SUCCESS != Sflash_BurnWrite_AllData(pData, data_len))
    {
        return FHA_FAIL;
    }

    
    return FHA_SUCCESS;
}


//从第0而开始写一个spiflash空间大小的数据到spiflash上
T_U32 Sflash_BurnRead_AllData_start(T_FHA_BIN_PARAM *pBinFile)
{
    //获取数据的长度
    m_download_sflash.RemainSectorCnt = (pBinFile->data_length + m_spi_burn.PageSize - 1) / m_spi_burn.PageSize;
    
    m_wirtebin_flag = AK_FALSE; //写整一个spi不需要写bin信息
    m_download_sflash.FileStartPage = 0;//从第0而开始烧录
    gFHAf.Printf("start page:%d_:%d\r\n", m_download_sflash.FileStartPage, pBinFile->data_length);

    //比较长度是否超过spiflash的大小
    //由于这里没有传总长度进来，所以只有在烧录工具上进行判断定
    
    return FHA_SUCCESS;
}

T_U32 Sflash_BurnRead_AllData(const T_U8 *pData, T_U32 data_len)
{
    T_U32 page_cnt, i = 0;
    T_U32 page_size = m_spi_burn.PageSize;
    T_U32* pStartPage;

     //page of starting to write 
    pStartPage = &(m_download_sflash.FileStartPage);

    page_cnt = (data_len + page_size - 1) / page_size;

    gFHAf.Printf("FHA_S start page:%d, cnt:%d, data_len:%d\r\n", *pStartPage, page_cnt, data_len);
    if (page_cnt > m_download_sflash.RemainSectorCnt)
    {
        gFHAf.Printf("FHA_S write SPI bin data_len is too large\r\n");
        return FHA_FAIL;
    }
    else if (page_cnt < m_download_sflash.RemainSectorCnt)
    {
        if (data_len != page_size * page_cnt)
        {
            gFHAf.Printf("FHA_S write SPI bin data_len is not page multiple\r\n");
            return FHA_FAIL;
        }
    } 
    
    if(FHA_SUCCESS != Prod_SPIReadPage(*pStartPage, page_cnt, pData))
    {
        gFHAf.Printf("FHA_S get fail at page start:%d, cnt:%d\r\n", *pStartPage, page_cnt);
        return FHA_FAIL;
    }


    *pStartPage += page_cnt;
    m_download_sflash.RemainSectorCnt -= page_cnt;
    gFHAf.Printf("m_download_sflash.RemainSectorCnt:%d\r\n", m_download_sflash.RemainSectorCnt);
    
    return FHA_SUCCESS;
}





