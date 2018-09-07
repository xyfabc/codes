/**
 * @FILENAME: nanddataget.c
 * @BRIEF xx
 * Copyright (C) 2011 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR luqiliu
 * @DATE 2009-11-19
 * @VERSION 1.0
 * @modified lu 2011-11-03
 * 
 * @REF
 */

#include "fha.h"
#include "nand_info.h"
#include "nanddataget.h"

typedef struct
{
    T_U32 FileLen;
    T_U32 MapPage;          //bin map relation page
    T_U32 BlockIndex;       //bin block index in bin map relation buffer    
    T_U32 StartPage;        //bin startpage in a block
}T_NAND_BIN_GET;


T_BIN_NAND_INFO  m_bin_nand_info  = {0};
T_NAND_PHY_INFO  g_nand_phy_info;
T_U32             g_boot_startpage = 0;


static T_NAND_BIN_GET m_nand_bin_get;

/**
 * @BREIF    fha_init_nand_param
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_VOID
 * @retval   
 * @retval   
 */
extern T_BOOL ForceRead_Nandboot(T_U32 start_page, T_U8 *pData, T_U32 length);


T_VOID fha_init_nand_param(T_U16 phypagesize, T_U16 pagesperblock, T_U16 blockperplane)
{
    m_bin_nand_info.bin_page_size   = phypagesize;

    if (FHA_CHIP_10XX == g_burn_param.eAKChip)
    {
        m_bin_nand_info.boot_page_size  = (phypagesize > 7168) ?  7168 : phypagesize;
    }
    else
    {
        m_bin_nand_info.boot_page_size  = (phypagesize > 4096) ?  4096 : phypagesize;
    }
    
    m_bin_nand_info.page_per_block  = pagesperblock;
    m_bin_nand_info.block_per_plane = blockperplane;

/*
如果是spot平台
*/
    if (PLAT_SPOT == g_burn_param.ePlatform)
    {
        /*
        如果是小页，那么合成2048的大叶
        */
        if(512 == phypagesize)
        {
            m_bin_nand_info.bin_page_size = 2048;
            m_bin_nand_info.boot_page_size = 2048;
            m_bin_nand_info.page_per_block = 64;
            m_bin_nand_info.block_per_plane = blockperplane / 8;
        }
        else
        {
            m_bin_nand_info.bin_page_size = (phypagesize > 4096) ?  4096 : phypagesize;
        } 
    }    
}



/**
 * @BREIF    read boot start 
 * @AUTHOR   lixingjian
 * @DATE     2012-10-29
 * @PARAM    [in] data buffer
 * @PARAM    [in] data buffer length
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32  fha_get_nandboot_begin(T_PFHA_BIN_PARAM bin_param)
{
    T_U32 page_size = m_bin_nand_info.bin_page_size;
    T_U8  *pbuf = AK_NULL;
    T_NAND_CONFIG *pNandCfg;
    T_U32 spare;

    if(0 == fha_str_cmp(bin_param->file_name, BOOT_NAME))
    {
        g_boot_startpage = 0;
        
        pbuf = gFHAf.RamAlloc(page_size);
        if (AK_NULL == pbuf)
        {
            return FHA_FAIL;
        }        

        if (!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - NAND_CONFIG_INFO_BACK_PAGE, pbuf, (T_U8 *)&spare))
        {
            gFHAf.RamFree(pbuf);
            return FHA_FAIL;
        }

        pNandCfg = (T_NAND_CONFIG *)pbuf;

        bin_param->data_length = pNandCfg->BootLength;
        
        return FHA_SUCCESS;
    }
    else
    {
        
        return FHA_FAIL;
    }
    
}

/**
 * @BREIF    get boot data
 * @AUTHOR   lixingjian
 * @DATE     2012-10-29
 * @PARAM    [in] data buffer
 * @PARAM    [in] data buffer length
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32  fha_get_nandboot(T_U8 *pData,  T_U32 data_len)
{
    T_U32 page_count = 0;
    T_U32 i = 0;
    
    gFHAf.Printf("data_len: %d\r\n", data_len);
    
    if (AK_NULL == pData || 0 == data_len)
    {
        return FHA_FAIL;
    }

    page_count = (data_len + m_bin_nand_info.boot_page_size - 1) / m_bin_nand_info.boot_page_size;  //total page count to write    

    if (!ForceRead_Nandboot(g_boot_startpage, pData, data_len))
    {
        gFHAf.Printf("backup_Boot_and_fs(): read boot fail\r\n");
        return AK_FALSE;
    } 
    g_boot_startpage = g_boot_startpage + page_count;
    
    return FHA_SUCCESS;
    
}



/**
 * @BREIF    get bin file length by name
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-12-15
 * @PARAM    [in] binName:bin name
 * @PARAM    [out] binLen:bin length 
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS:  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 fha_GetBinStart(T_PFHA_BIN_PARAM bin_param)
{    
    T_U32 i;
    T_U32 fileIndex = 0;
    T_U32 spare;
    T_FILE_CONFIG *pFileConfig;
    T_NAND_CONFIG *pNandCfg;
    T_U32 file_count;
    T_U32 page_size = m_bin_nand_info.bin_page_size;
    T_U8  *pbuf = AK_NULL;
    T_U16 *ptr;

    m_nand_bin_get.BlockIndex = 0;
    m_nand_bin_get.StartPage = 0;
    /*
   是否读取boot?
    */
    
    if(0 != fha_str_cmp(bin_param->file_name, BOOT_NAME))
    {
        pbuf = gFHAf.RamAlloc(page_size);
        if (AK_NULL == pbuf)
        {
            return FHA_FAIL;
        }        

      /*
                  读取nand config info, 获取bin文件的个数
         */

        if (!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - NAND_CONFIG_INFO_BACK_PAGE, pbuf, (T_U8 *)&spare))
        {
            gFHAf.RamFree(pbuf);
            return FHA_FAIL;
        }

        pNandCfg = (T_NAND_CONFIG *)pbuf;
        file_count = pNandCfg->BinFileCount;
        
        /*
                    读取file config info, 获取文件结构体信息
           */
        if (!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - FILE_CONFIG_INFO_BACK_PAGE, pbuf, (T_U8 *)&spare))
        {
            gFHAf.RamFree(pbuf);
            return FHA_FAIL;
        }

        pFileConfig = (T_FILE_CONFIG *)pbuf;
        
        // search bin file config
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

        /*
                   是否找到所需要读取的bin文件
           */

        if (i == file_count)
        {
            gFHAf.RamFree(pbuf);
            return FHA_FAIL;
        }

        m_nand_bin_get.MapPage = pFileConfig[fileIndex].map_index;
        m_nand_bin_get.FileLen = pFileConfig[fileIndex].file_length; 
        bin_param->data_length = pFileConfig[fileIndex].file_length;
        bin_param->ld_addr     = pFileConfig[fileIndex].ld_addr;
        if (pFileConfig[fileIndex].update_pos != 0)
        {
            bin_param->bUpdateSelf = AK_TRUE;
        }
        else
        {
            bin_param->bUpdateSelf = AK_FALSE;
        }

        /*
                  获取该bin文件的存储block的map
           */
        if (!Prod_NandReadASAPage(0, m_nand_bin_get.MapPage, pbuf, (T_U8 *)&spare))
        {
            gFHAf.Printf("FHA_NG Get bin st read fail\r\n");
            gFHAf.RamFree(pbuf);
            return FHA_FAIL;
        }             

        ptr = (T_U16 *)pbuf;
        if (ptr[1] != 0)
        {
            bin_param->bBackup = AK_TRUE;
        }
        else
        {
            bin_param->bBackup = AK_FALSE;
        }
        
        gFHAf.RamFree(pbuf);
    }
    
    return FHA_SUCCESS;
    
}

/**
 * @BREIF    get data
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-12-15
 * @PARAM    [in] data buffer
 * @PARAM    [in] data buffer length
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32 fha_GetBinData(T_U8* pBuf, T_U32 data_len)
{
    T_U32 spare;
    T_U32 block;
    T_U32 block_bak;
    T_U32 page_size  = m_bin_nand_info.bin_page_size;
    T_U32 block_size = m_bin_nand_info.page_per_block * page_size;
    T_U32 BlockPerPage = m_bin_nand_info.bin_page_size / 4;
    T_U32 block_Max;
    T_U32 PageCnt = 0;
    T_U32 i;
    T_U16* pMap = AK_NULL;     
    T_BOOL flag = AK_FALSE;
    T_U32  ret  = FHA_SUCCESS;
    
    if (AK_NULL == pBuf || 0 == data_len || 0 == m_nand_bin_get.FileLen)
        return FHA_FAIL;
    
    block_Max  = (m_nand_bin_get.FileLen + block_size - 1) / block_size;
    PageCnt = (block_Max + BlockPerPage - 1) / BlockPerPage;
    
    pMap = gFHAf.RamAlloc(PageCnt * page_size);
    if (AK_NULL == pMap)
    {
        gFHAf.Printf("FHA_NG Read bin malloc fail\r\n");
        return FHA_FAIL;
    }

    for (i=0; i<PageCnt; i++)
    {
        if (!Prod_NandReadASAPage(0, m_nand_bin_get.MapPage + i, (T_U8 *)pMap + i * page_size, (T_U8 *)&spare))
        {
            gFHAf.Printf("FHA_NG Read bin fail\r\n");
            gFHAf.RamFree(pMap);
            return FHA_FAIL;
        }        
    }

    /*
             获取文件的所在的对应的block
       */
    
    block     = Prod_PB2LB(pMap[m_nand_bin_get.BlockIndex * 2]);
    block_bak = Prod_PB2LB(pMap[m_nand_bin_get.BlockIndex * 2 + 1]);
    if (0 == block)
    {
        gFHAf.Printf("FHA_NG Read bin block error!\r\n");
        gFHAf.RamFree(pMap);
        return FHA_FAIL;
    }

    while (data_len)
    {
        if (!Prod_NandReadBinPage(block, m_nand_bin_get.StartPage, pBuf, (T_U8 *)&spare))
        {
            if (block_bak != 0)
            {
                flag = AK_TRUE;
                if (!Prod_NandReadBinPage(block_bak, m_nand_bin_get.StartPage, pBuf, (T_U8 *)&spare))
                {
                    gFHAf.Printf("FHA_NG Read bin block_bak fail!\r\n");
                    gFHAf.RamFree(pMap);
                    return FHA_FAIL;
                }
            }
            else
            {
                gFHAf.Printf("FHA_NG Read bin block fail!\r\n");
                gFHAf.RamFree(pMap);
                return FHA_FAIL;
            }
        }
        /*
                 由于读取主块出现问题，需要去读取备份块拷贝一遍数据到主块中
           */

        //2013-12-11 by lixingjian ,reviewer liaozhijun
        //由于获取bin文件目前只有烧录工具的bin回读功能使用
        //而bin回读功能只是把数据读出去，不需要破坏其他数据，所以
        //不需要再进行写数据的操作，估不需要跑如下的代码
        if (0)//flag)
        {
            //T_U32 i;
            T_U8 *ptBuf = AK_NULL;
            
            flag = AK_FALSE;
            ptBuf = gFHAf.RamAlloc(page_size);
            if (AK_NULL == ptBuf)
            {
                gFHAf.Printf("FHA_NG Read bin malloc fail++\r\n");
                gFHAf.RamFree(pMap);
                return FHA_FAIL;
            }

            Prod_NandEraseBlock(block);
            for (i=0; i<m_bin_nand_info.page_per_block; i++)
            {
                if (!Prod_NandReadBinPage(block_bak, i, ptBuf, (T_U8 *)&spare))
                {
                    gFHAf.Printf("FHA_NG Read bin block back fail++!\r\n");
                    ret = FHA_FAIL;
                }

                if (!Prod_NandWriteBinPage(block, i, ptBuf, (T_U8 *)&spare))
                {
                    gFHAf.Printf("FHA_NG write bin block fail++!\r\n");
                    ret = FHA_FAIL;
                }                
            }
            gFHAf.RamFree(ptBuf);
        }
        
        pBuf += page_size;
        m_nand_bin_get.StartPage++;
        if (data_len > page_size) 
            data_len -= page_size;
        else
            data_len = 0;

        //last page at the written block
        if(m_nand_bin_get.StartPage == m_bin_nand_info.page_per_block)
        {
            m_nand_bin_get.StartPage = 0;
            m_nand_bin_get.BlockIndex++;
            block     = Prod_PB2LB(pMap[m_nand_bin_get.BlockIndex * 2]);
            block_bak = Prod_PB2LB(pMap[m_nand_bin_get.BlockIndex * 2 + 1]);
            if (data_len != 0 && 0 == block)
            {
                gFHAf.Printf("FHA_NG Read bin block error!\r\n");
                gFHAf.RamFree(pMap);
                return FHA_FAIL;
            }
        }
    }

    gFHAf.RamFree(pMap);
    
    return ret;
}

/************************************************************************
 * NAME:     fha_GetFSPart
 * FUNCTION  get fs partition info
 * PARAM:    [out]pInfoBuf---need to get fs info data pointer addr
 *           [in] data_len---need to get fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_GetFSPart(T_U8 *pInfoBuf, T_U32 buf_len)
{
    T_U32 spare;
    T_U32 page_size = m_bin_nand_info.bin_page_size;
    T_U8 *pbuf;

    if (AK_NULL == pInfoBuf)
        return FHA_FAIL;

    pbuf = gFHAf.RamAlloc(page_size);
    if (AK_NULL == pbuf)
    {
        gFHAf.Printf("FHA_NG alloc fail\r\n");
        return FHA_FAIL;
    } 

    if (!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - FS_PART_INFO_BACK_PAGE, pbuf, (T_U8 *)&spare))
    {
        gFHAf.Printf("FHA_NG read fail\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    gFHAf.MemCpy(pInfoBuf, pbuf, buf_len);
    gFHAf.RamFree(pbuf);
    
    return FHA_SUCCESS;
}

/************************************************************************
 * NAME:     FHA_get_nand_para
 * FUNCTION  get nand para
 * PARAM:    [out] pNandPhyInfo--nand info struct pointer
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/


T_U32  fha_GetNandPara(T_NAND_PHY_INFO *pNandPhyInfo)
{
    T_U8 spare[3];
    T_U8 *pbuf;
    T_U32 j;
    T_U32 bootpagelen = 512;
    T_U32 spare_len;

    if (AK_NULL == pNandPhyInfo)
        return FHA_FAIL;
    
    pbuf = gFHAf.RamAlloc(bootpagelen);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    } 

    if (FHA_CHIP_880X == g_burn_param.eAKChip)
    {
        bootpagelen = 512;
        spare_len = 3;
    }
    else if (FHA_CHIP_10XX == g_burn_param.eAKChip)
    {
        bootpagelen = 402;
        spare_len = 0;
    }
    
    else if (FHA_CHIP_37XX == g_burn_param.eAKChip)
    {
        bootpagelen = 486;
        spare_len = 0;
    }
    
    else
    {
        bootpagelen = 472;  //目前98的是472
        spare_len = 0;
    }    

    if (FHA_SUCCESS != gFHAf.Read(0, 0, pbuf, bootpagelen, spare, spare_len, FHA_GET_NAND_PARAM))
    {
        gFHAf.Printf("FHA_NG read nand param fail\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    } 

    if ('A' != pbuf[4] || 'N' != pbuf[5] || 'Y' != pbuf[6] || 'K' != pbuf[7] || 'A' != pbuf[8])
    {
        gFHAf.Printf("FHA_NG read nand param data is err\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    } 

    for (j=9; j<bootpagelen-4; j++)
    {
        if ('N' == pbuf[j] && 'A' == pbuf[j+1] && 'N' == pbuf[j+2] && 'D' == pbuf[j+3])
        {
            gFHAf.MemCpy(pNandPhyInfo, pbuf + j + 4, sizeof(T_NAND_PHY_INFO));
            gFHAf.RamFree(pbuf);
            pbuf = AK_NULL;
            break;
        }    
    } 

    if ((bootpagelen-4) == j)
    {
        gFHAf.Printf("FHA_NG not param\r\n");
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }    

    if (pbuf != AK_NULL)
        gFHAf.RamFree(pbuf);
    
    return FHA_SUCCESS;
}


/************************************************************************
 * NAME:     FHA_get_resv_zone_info
 * FUNCTION  get reserve area
 * PARAM:    [out] start_block--get reserve area start block
 *           [out] block_cnt----get reserve area block count
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 fha_GetResvZoneInfo(T_U16 *start_block, T_U16 *block_cnt)
{
    T_U32 spare;
    T_U32 page_size = m_bin_nand_info.bin_page_size;
    T_U8 *pbuf;
    T_NAND_CONFIG *pNandConfig;

    if (AK_NULL == start_block || AK_NULL == block_cnt)
        return FHA_FAIL;
    
    pbuf = gFHAf.RamAlloc(page_size);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    } 

    if (!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - NAND_CONFIG_INFO_BACK_PAGE, pbuf, (T_U8 *)&spare))
    {
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    pNandConfig = (T_NAND_CONFIG *)pbuf;
    *start_block = (T_U16)pNandConfig->ResvStartBlock;
    *block_cnt   = (T_U16)pNandConfig->ResvBlockCount;
    
    gFHAf.RamFree(pbuf);
    
    return FHA_SUCCESS;
}

/**
 * @BREIF    fha_GetMaplist
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32 fha_GetMaplist(T_U8 file_name[], T_U16 *map_data, T_U32 *file_len, T_BOOL bBackup)
{
    T_U32 spare, i;    
    T_FHA_BIN_PARAM bin_param;
    T_U32 page_size  = m_bin_nand_info.bin_page_size;
    T_U32 block_size = m_bin_nand_info.page_per_block * page_size;
    T_U32 block_Max;
    T_U32 BlockPerPage = page_size / 4;
    T_U32 PageCnt = 0;
    T_U16* pMap = AK_NULL;     

    if (AK_NULL == map_data || AK_NULL == file_len)
    {
        gFHAf.Printf("FHA_NG Get map input para error!\r\n");
        return FHA_FAIL;
    }
    
    gFHAf.MemCpy(bin_param.file_name, file_name, 16);

    if (fha_GetBinStart(&bin_param))
    {
        *file_len = bin_param.data_length;
        block_Max  = (bin_param.data_length + block_size - 1) / block_size;
        PageCnt = (block_Max + BlockPerPage - 1) / BlockPerPage;
        pMap = gFHAf.RamAlloc(page_size * PageCnt);
        if (AK_NULL == pMap)
        {
            gFHAf.Printf("FHA_NG Get map malloc fail\r\n");
            
            return FHA_FAIL;
        }  
        
        for (i=0; i<PageCnt; i++)
        {
            if (!Prod_NandReadASAPage(0, m_nand_bin_get.MapPage + i, (T_U8 *)pMap + i * page_size, (T_U8 *)&spare))
            {
                gFHAf.Printf("FHA_NG Get map read fail\r\n");
                gFHAf.RamFree(pMap);
                return FHA_FAIL;
            }             
                
        }

        for (i=0; i<block_Max; i++)
        {
            if (bBackup)
            {
                map_data[i] = pMap[i * 2 + 1];
            }
            else
            {
                map_data[i] = pMap[i * 2];
            }
        }

        gFHAf.RamFree(pMap);
        return FHA_SUCCESS;
    }

    return FHA_FAIL;
}

/************************************************************************
 * NAME:     FHA_get_bin_num
 * FUNCTION  get bin file number
 *           [out] cnt----bin file count in medium
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 fha_GetBinNum(T_U32 *cnt)
{    
    T_U32 spare;
    T_NAND_CONFIG *pNandCfg;
    T_U32 file_count;
    T_U32 page_size = m_bin_nand_info.bin_page_size;
    T_U8  *pbuf = AK_NULL;
    
    pbuf = gFHAf.RamAlloc(page_size);
    if (AK_NULL == pbuf)
    {
        return FHA_FAIL;
    }        

    if (!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - NAND_CONFIG_INFO_BACK_PAGE, pbuf, (T_U8 *)&spare))
    {
        gFHAf.RamFree(pbuf);
        return FHA_FAIL;
    }

    pNandCfg = (T_NAND_CONFIG *)pbuf;
    file_count = pNandCfg->BinFileCount;
        
    *cnt = file_count;
    if (AK_NULL != pbuf)
    {
        gFHAf.RamFree(pbuf);
    }
    
    return FHA_SUCCESS;
}


