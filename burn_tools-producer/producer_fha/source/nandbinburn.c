/**
 * @FILENAME fha_binburn.c
 * @BRIEF write or read bin file and write bin information
 * Copyright (C) 2009 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR Jiang Dihui
 * @DATE 2009-09-10
 * @version 1.0
 */

#include "fha.h"
#include "nand_info.h"
#include "nandburn.h"

//origin and backup block when writing bin data
typedef struct
{
    T_U32 block_origin; //origin block
    T_U32 block_backup; //backup block
}T_PROD_BLOCK;

typedef struct
{
    T_U16*         pMapBlockCnt;   //eack bin block including spare block
    T_FILE_CONFIG* pFileConfig;    //each file config
    T_U16*         pMapTotalBuf;   //all map buffer
}T_BIN_INFO_BUF;

//burn bin file information
typedef struct
{
    T_U32 BinStartBlock;    //bin data start block
    T_U32 file_index;       //bin file count index
    T_U32 BlockIndex;       //bin block index in bin map relation buffer    
    T_U32 StartPage;        //bin startpage in a block
    T_U32 totalpagecnt;     //bin total size
    T_BOOL bBackup;         //bin backup
    T_BOOL bCheck;          //bin check self
    T_BOOL bUpdateSelf;     //spotlight update self, gave bin reserve same space
    T_BOOL bWriteBoot;      //write nand boot flag
    T_PROD_BLOCK EndBlock;  //origin and backup block when writing bin data
}T_DOWNLOAD_BIN;

static T_U16  CalMapTotalOffset(T_U32 fileIndex);
static T_BOOL RecoverBinFileInfo();
static T_BOOL SetUpdateParam(T_U8 strName[]);
static T_BOOL ForceWrite_Nandboot(T_U32 *start_page, const T_U8 *pData, T_U32 length);
static T_U32 ForceWrite_NandBin_Block(T_PROD_BLOCK *end_block, T_U32 start_page, T_U32 page_count, const T_U8 *pData);
static T_U32 burn_nand_bin_end(T_U32* endblock);
static T_U32 extend_nandbin_space(T_U32 start_block, T_U32 blk_cnt);
T_BOOL ForceRead_Nandboot(T_U32 start_page, T_U8 *pData, T_U32 length);

static T_BIN_INFO_BUF   m_pBin_info = {0};
static T_U32            m_UpdateBlockLimit[NAND_BIN_COUNT] = {0};
static T_U8             *m_fs_part_buf = AK_NULL;
static T_DOWNLOAD_BIN   m_DloadBin = {0};
static T_NAND_CONFIG    m_nand_config = {0};

T_U32 m_bin_resv_size = 0; //记录每个bin文件区域的扩展大小

#define     NAND_ONE_MSIZE            (1024*1024)
#define     NAND_ONE_KSIZE            (1024)





//设置这个大小后，每个bin文件后扩展这个大小的区域
T_U32 fha_set_bin_resv_size(T_U32 nSize)
{
    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        if(PLAT_LINUX == g_burn_param.ePlatform)
        {
            gFHAf.Printf("nSize(k)=%d\r\n", nSize);
             m_bin_resv_size = nSize*NAND_ONE_KSIZE;
             gFHAf.Printf("linux:FHA_NB set bin reserve size=%d\r\n", m_bin_resv_size);
        }
        else
        {
            gFHAf.Printf("nSize(M)=%d\r\n", nSize);
            m_bin_resv_size = nSize*NAND_ONE_MSIZE;
            gFHAf.Printf("RTOS:FHA_NB set bin reserve size=%d\r\n", m_bin_resv_size);
        }
        

    }
    return FHA_SUCCESS;
}


T_U32  fha_get_nand_last_pos(T_VOID)
{
    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        return m_DloadBin.BinStartBlock;
    }
    else
    {
        return m_nand_config.BinEndBlock;
    }   
}

/************************************************************************
 * NAME:     FHA_set_resv_zone_info
 * FUNCTION  set reserve area
 * PARAM:    [in] nSize--set reserve area size(unit Mbyte)
 *           [in] bErase-if == 1 ,erase reserve area, or else not erase 
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/
T_U32 fha_set_nand_resv_zone_info(T_U32  nSize, T_BOOL bErase)
{
    T_U32 block_size; //unit-kbyte
    T_U32 block_cnt;

    block_size = m_bin_nand_info.page_per_block * m_bin_nand_info.bin_page_size / 1024;
    block_cnt  = (nSize * 1024 +  block_size -1)/ block_size;
    gFHAf.Printf("FHA_NB set reserve blockcnt=%d\r\n", block_cnt);    

    if (bErase)
    {
        T_U32 i;

        for (i=0; i<block_cnt; i++)
        {
            Prod_NandEraseBlock(m_DloadBin.BinStartBlock + i);
        }
    }

    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        m_nand_config.ResvStartBlock = m_DloadBin.BinStartBlock;
        m_nand_config.ResvBlockCount = block_cnt;
        m_nand_config.BinStartBlock = m_nand_config.ResvStartBlock + block_cnt;
 
        m_DloadBin.BinStartBlock += block_cnt;
    }
    return FHA_SUCCESS;
}

/**
 * @BREIF    fha_set_asa_end_block
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_VOID
 * @retval   
 * @retval   
 */

T_VOID fha_set_asa_end_block(T_U32 blk_end)
{
    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        m_nand_config.BinStartBlock = blk_end + 1;
        m_nand_config.ASAStartBlock = 1;
        m_nand_config.ASAEndBlock   = blk_end;
    }
    m_DloadBin.BinStartBlock = blk_end + 1;
} 

/**
 * @BREIF    fha_burn_nand_init
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32  fha_burn_nand_init(T_PFHA_INIT_INFO pInit, T_PFHA_LIB_CALLBACK pCB, T_NAND_PHY_INFO *pNandPhyInfo)
{
    T_U8  *pbuf = AK_NULL;
    T_U32 bufLen;   
    
    if (AK_NULL == pNandPhyInfo)
        return FHA_FAIL;

    gFHAf.MemCpy(&g_nand_phy_info, pNandPhyInfo, sizeof(g_nand_phy_info));

    //initial bin burn param
    fha_init_nand_param(pNandPhyInfo->page_size, pNandPhyInfo->page_per_blk, pNandPhyInfo->plane_blk_num);
      
    m_DloadBin.BinStartBlock = 1;           //start block of the first bin, but user reserve area and secarea will increase this value
    m_DloadBin.file_index    = 0;           //index of the first bin file is 0
    m_DloadBin.BlockIndex    = 0;           //block index of map buffer is 0

    //size of malloc memory is used storage fileconfig and map relation of all bin
    //NAND_BIN_COUNT is count of bin file
    //BIN_MAX_BLOCK_COUNT is block count of all bin data
    bufLen = (sizeof(T_U16) + sizeof(T_FILE_CONFIG)) * NAND_BIN_COUNT + BIN_MAX_BLOCK_COUNT * 4;

    pbuf = gFHAf.RamAlloc(bufLen);

    if(AK_NULL == pbuf)
    {
        gFHAf.Printf("FHA_NB alloc fail\r\n");
        return FHA_FAIL;
    }

    gFHAf.MemSet(pbuf, 0, bufLen);

    m_pBin_info.pMapBlockCnt = (T_U16 *)pbuf;
    m_pBin_info.pFileConfig  = (T_FILE_CONFIG *)(pbuf + NAND_BIN_COUNT * sizeof(T_U16));
    m_pBin_info.pMapTotalBuf = (T_U16 *)(pbuf + (sizeof(T_U16) + sizeof(T_FILE_CONFIG)) * NAND_BIN_COUNT);

    //recover all bin info to buffer of m_pBin_info on update mode
    if(MODE_UPDATE == g_burn_param.eMode)
    {
        gFHAf.Printf("FHA_NB recover update bin info\r\n");
        if(!RecoverBinFileInfo())
            return FHA_FAIL;
    }
    
    return FHA_SUCCESS;
}


/**
 * @BREIF    write bin start,get and config some param, BinBurn_Init must be called yet 
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] bin information
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
//T_BOOL BinBurn_Start(T_BIN_INFO *pBinInfo)
T_U32  fha_write_nandbin_begin(T_PFHA_BIN_PARAM bin_param)
{
    T_U32 block_size, page_size, block_cnt;
    T_FILE_CONFIG* file_cfg;

    page_size  = m_bin_nand_info.bin_page_size;
    block_size = m_bin_nand_info.page_per_block * page_size;           
    block_cnt  = (bin_param->data_length + block_size - 1)/block_size;

    m_DloadBin.totalpagecnt = (bin_param->data_length + page_size - 1) / page_size;
    m_DloadBin.bBackup      = bin_param->bBackup;
    m_DloadBin.bCheck       = bin_param->bCheck;
    m_DloadBin.bUpdateSelf  = bin_param->bUpdateSelf;
    
    //search file index to get fileconfig on update mode 
    if(MODE_UPDATE == g_burn_param.eMode)
    {
        if(!SetUpdateParam(bin_param->file_name))
        {
            gFHAf.Printf("FHA_NB updata param err\r\n");
            return FHA_FAIL;
        }    
    }

    //m_DloadBin.file_cnt is gotten in SetUpdateParam on update mode,otherwise is increased base 0
    file_cfg = m_pBin_info.pFileConfig + m_DloadBin.file_index;
   
    //these value maybe changed listed below
    file_cfg->file_length = bin_param->data_length;
    file_cfg->ld_addr     = bin_param->ld_addr;

    //set value listed below when NEWBURN
    if(MODE_NEWBURN == g_burn_param.eMode)
    {
        gFHAf.MemSet(file_cfg->file_name, 0, 16);
        gFHAf.MemCpy(file_cfg->file_name, bin_param->file_name, 15); 

        file_cfg->run_pos = m_DloadBin.BinStartBlock;

        //set update 0 when NEWBURN, modify this value in BinBurn_UpdateSelf_Start if exiting updateself
        file_cfg->update_pos = 0;    

        //this value will be modified in BinBurn_WriteBinEnd if must assign some spare size,
        //this value is gotten in RecoverBinFileInfo inclue spare size when update mode
        m_pBin_info.pMapBlockCnt[m_DloadBin.file_index]   = (T_U16)(block_cnt);
    }

    //last call to control download
    m_DloadBin.BlockIndex = 0;
    m_DloadBin.StartPage  = 0;

    //get bin block to write
    m_DloadBin.EndBlock.block_origin = m_DloadBin.BinStartBlock;
    if (m_DloadBin.bBackup)
    {
        m_DloadBin.EndBlock.block_backup = m_DloadBin.BinStartBlock+1;
    }
    else
    {
        m_DloadBin.EndBlock.block_backup = 0;
    }    

    return FHA_SUCCESS;
}

/**
 * @BREIF    write bin data
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] data buffer
 * @PARAM    [in] data buffer length
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32  fha_write_nandbin(const T_U8 * pData,  T_U32 data_len)
{
//    T_U32 i;
    T_U32 page_cnt;
    T_U32 page_total;
    T_U32 page_to_write_cnt;
    T_U32 page_size         =  m_bin_nand_info.bin_page_size;
    T_U32 page_per_block    =  m_bin_nand_info.page_per_block;
    T_U32 page_cnt_sum      =  0;
    T_U32 nextblock = m_DloadBin.BinStartBlock;  
    T_U16* pMap = AK_NULL;

    //Get map offset, m_DloadBin.file_cnt be add 1 in DloadBinCtrl(STATUS_ORIG)
    T_U16 mapoffset = CalMapTotalOffset(m_DloadBin.file_index);

    if (AK_NULL == pData)
    {
        gFHAf.Printf("FHA_NB write bin para is NULL\r\n");
        return FHA_FAIL;
    }
    
    //map buffer
    pMap = &m_pBin_info.pMapTotalBuf[mapoffset/2];

    if (0 == data_len)
    {
        gFHAf.Printf("FHA_NB write bin data_len is 0\r\n");
        return FHA_FAIL;
    }    

    page_total = (data_len + page_size - 1) / page_size;              //total page count

    if (page_total > m_DloadBin.totalpagecnt)
    {
        gFHAf.Printf("FHA_NB write bin data_len is too large\r\n");
        return FHA_FAIL;
    }
    else if (page_total < m_DloadBin.totalpagecnt)
    {
        if (data_len != page_size * page_total)
        {
            gFHAf.Printf("FHA_NB write bin data_len is not page multiple\r\n");
            return FHA_FAIL;
        }
    }
    else
    {
        T_FILE_CONFIG* file_cfg;
        
        file_cfg = m_pBin_info.pFileConfig + m_DloadBin.file_index;
        if ((data_len % page_size) != (file_cfg->file_length % page_size))
        {
            gFHAf.Printf("FHA_NB write bin data_len is error!\r\n");
            return FHA_FAIL;
        }
    }
        
    while (page_total !=0)
    {
        //caculate page count to write in the writing block , each only write in a block
        if ((page_total + m_DloadBin.StartPage) > page_per_block)
        {
            page_to_write_cnt = page_per_block - m_DloadBin.StartPage;
        }
        else
        {
            page_to_write_cnt = page_total;
        }

        /*
        write orign block and backup block,  each time will only write in a block, 
        return page count written, this value is smaller than pageperblock,
        m_DloadBin.EndBlock maybe modified if the block is bad block
       */
        page_cnt = ForceWrite_NandBin_Block(&m_DloadBin.EndBlock, m_DloadBin.StartPage, page_to_write_cnt, pData + page_cnt_sum * page_size);

        if(0 == page_cnt) //write fail
        {
            gFHAf.Printf("FHA_NB write bin fail\r\n");
            return FHA_FAIL;
        }
        else
        {
            //total page written count
            page_cnt_sum += page_cnt;
            
            //total remain page count
            page_total -= page_cnt;
            
            //status of STATUS_ORIG to fill map buffer
            pMap[m_DloadBin.BlockIndex * 2]     = (T_U16)Prod_LB2PB(m_DloadBin.EndBlock.block_origin);
            pMap[m_DloadBin.BlockIndex * 2 + 1] = (T_U16)Prod_LB2PB(m_DloadBin.EndBlock.block_backup);

            m_DloadBin.StartPage += page_cnt;
            
            if (m_DloadBin.totalpagecnt > page_cnt)
            {
                m_DloadBin.totalpagecnt -= page_cnt;
            }
            else
            {
                m_DloadBin.totalpagecnt = 0;
            }

            //Fwl_ForceWrite_Block write a block to full
            if(m_DloadBin.StartPage >=  page_per_block || 0 == m_DloadBin.totalpagecnt)
            {  
                m_DloadBin.StartPage = 0;  //start page to set 0

                //nextblock to write
                nextblock = m_DloadBin.EndBlock.block_origin > m_DloadBin.EndBlock.block_backup ? (m_DloadBin.EndBlock.block_origin + 1) : (m_DloadBin.EndBlock.block_backup + 1); 
                //keep to use writing next block 
                m_DloadBin.EndBlock.block_origin = nextblock;

                if (m_DloadBin.bBackup)
                {
                    m_DloadBin.EndBlock.block_backup = nextblock + 1;
                }
                else
                {
                    m_DloadBin.EndBlock.block_backup = 0;
                }    

                //map index increase
                m_DloadBin.BlockIndex++;
            }

            //when write bin end, need to write reserve size for expand 
            if (0 == m_DloadBin.totalpagecnt)
            {
                if (MODE_NEWBURN == g_burn_param.eMode)
                {
                    if (FHA_SUCCESS != burn_nand_bin_end(&nextblock))
                    {
                        gFHAf.Printf("FHA_NB write bin end fail\r\n");
                        return FHA_FAIL;
                    }
                    
                    m_nand_config.BinEndBlock = nextblock;
                    gFHAf.Printf("FHA_NB last block:%d\r\n", nextblock);
                }
             }
        }
    }

    m_DloadBin.BinStartBlock = nextblock;

    return FHA_SUCCESS;
}

/************************************************************************
 * NAME:     fha_write_nandboot
 * FUNCTION  set write boot start init para
 * PARAM:    [in] bin_len--boot length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_write_nandboot_begin(T_U32 bin_len)
{
    T_U32 page_size = m_bin_nand_info.boot_page_size;
    
    m_DloadBin.StartPage     = 0;
    m_DloadBin.totalpagecnt  = (bin_len + page_size - 1) / page_size;
    m_nand_config.BootLength = bin_len;
    m_DloadBin.bWriteBoot = AK_TRUE;
        
    return FHA_SUCCESS;
}

/**
 * @BREIF    write boot bin
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] data buffer
 * @PARAM    [in] data buffer length
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_U32  fha_write_nandboot(const T_U8 *pData,  T_U32 data_len)
{
    //boot data is written block0, m_DloadBin.StartPage will be increased when a page is written
    if(!ForceWrite_Nandboot(&m_DloadBin.StartPage, pData, data_len))
    {
        return FHA_FAIL;
    }

    return FHA_SUCCESS;
}

/************************************************************************
 * NAME:     fha_set_nand_fs_part
 * FUNCTION  set fs partition info to medium 
 * PARAM:    [in] pInfoBuf-----need to write fs info data pointer addr
 *           [in] data_len--need to write fs info data length
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32 fha_set_nand_fs_part(const T_U8 *pInfoBuf , T_U32 buf_len)
{
    T_U32 page_size       = m_bin_nand_info.bin_page_size;
    T_U32 spare = ZONE_INFO_FLAG;

    gFHAf.Printf("FHA_NB set fs part!\r\n");
    if (0 == buf_len || buf_len > page_size)
    {
        return FHA_FAIL;
    }
    
    if(!Prod_NandEraseBlock(0))
    {
        return FHA_FAIL;
    }

    if (AK_NULL == m_fs_part_buf)
        m_fs_part_buf = gFHAf.RamAlloc(page_size);
    
    if(AK_NULL == m_fs_part_buf)
    {
        return FHA_FAIL;
    }

    gFHAf.MemSet(m_fs_part_buf, 0, page_size);
    gFHAf.MemCpy(m_fs_part_buf, pInfoBuf, buf_len);
    if (!Prod_NandWriteASAPage(0, m_bin_nand_info.page_per_block-1, m_fs_part_buf, (T_U8 *)&spare))
    {
        return FHA_FAIL;
    }

    return FHA_SUCCESS;
}    
    
/**
 * @BREIF    write all bin map
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] a buffer that the size is pagesize 
 * @RETURN   T_BOOL 
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE :     fail
 */
T_BOOL fha_WriteBinMap(T_U8* pPageBuf)
{
    T_U32 i;
    T_U32 spare = MAP_INFO_FLAG;
    T_U32 binMapStartAddr; 
    T_U32 file_cnt;
    T_U32 bootEndAddr;
    T_U8  page_cnt[NAND_BIN_COUNT];

    if (MODE_NEWBURN == g_burn_param.eMode)
    {
        file_cnt = m_DloadBin.file_index;
    }
    else
    {
        file_cnt = m_nand_config.BinFileCount;
    }

    binMapStartAddr = m_bin_nand_info.page_per_block - FILE_CONFIG_INFO_BACK_PAGE;

    for(i=0; i<file_cnt; i++)
    {
        page_cnt[i] = (T_U8)((m_pBin_info.pMapBlockCnt[i] * 4 + m_bin_nand_info.bin_page_size - 1) / m_bin_nand_info.bin_page_size);
        binMapStartAddr -= page_cnt[i];
    } 

    bootEndAddr = (m_nand_config.BootLength + m_bin_nand_info.boot_page_size - 1) / m_bin_nand_info.boot_page_size;
    if (binMapStartAddr < bootEndAddr)
    {
        gFHAf.Printf("FHA_NB block0 space is not enough, binMapStartAddr:%d\r\n", binMapStartAddr);
        return AK_FALSE;
    }    

    gFHAf.MemSet(pPageBuf, 0, sizeof(T_U8) * m_bin_nand_info.bin_page_size);
    for(i=bootEndAddr; i<binMapStartAddr; i++)
    {
        if (!Prod_NandWriteASAPage(0, i, pPageBuf, (T_U8 *)&spare))
        {
            return AK_FALSE;
        }
    }
    
    for(i=0; i<file_cnt; i++)
    {
        T_U32 j, buf_len, buf_offset;

        buf_offset = 0;
        buf_len = m_pBin_info.pMapBlockCnt[i] * 4;
       
        if(MODE_NEWBURN == g_burn_param.eMode)
        {
            m_pBin_info.pFileConfig[i].map_index = binMapStartAddr;
            binMapStartAddr += page_cnt[i];
        }    

        gFHAf.Printf("FHA_NB map index:%d\r\n", m_pBin_info.pFileConfig[i].map_index);

        for(j=0; j<page_cnt[i]; j++)
        {
            T_U32 cpylen = ((buf_len - buf_offset) > m_bin_nand_info.bin_page_size) ? m_bin_nand_info.bin_page_size : (buf_len - buf_offset);

            gFHAf.MemSet(pPageBuf, 0, sizeof(T_U8) * m_bin_nand_info.bin_page_size);
            gFHAf.MemCpy(pPageBuf, (T_U8 *)((T_U32)m_pBin_info.pMapTotalBuf + CalMapTotalOffset(i) + buf_offset),
                            cpylen);
            if (!Prod_NandWriteASAPage(0, m_pBin_info.pFileConfig[i].map_index + j, pPageBuf, (T_U8 *)&spare))
            {
                return AK_FALSE;
            }

            buf_offset += m_bin_nand_info.bin_page_size;
        }
    }

    return AK_TRUE;
}

/**
 * @BREIF    write all bin file information
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] a buffer that the size is pagesize 
 * @RETURN   T_BOOL 
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE :     fail
 */
T_BOOL fha_WriteBinFileInfo(T_U8* pPageBuf)
{
    T_U32 spare; 

    if(MODE_NEWBURN == g_burn_param.eMode)
    {
        //m_nand_config.MainVer = 3;
        //m_nand_config.SubVer  = 0;
        //m_nand_config.Sub1Ver = 0;
        m_nand_config.BinFileCount    = m_DloadBin.file_index;
    }

    //write file config
    spare = BIN_FILE_INFO_FLAG;
    gFHAf.MemSet(pPageBuf, 0, sizeof(T_U8) * m_bin_nand_info.bin_page_size);
    gFHAf.MemCpy(pPageBuf, m_pBin_info.pFileConfig, m_nand_config.BinFileCount * sizeof(T_FILE_CONFIG));

    if (!Prod_NandWriteASAPage(0, m_bin_nand_info.page_per_block - FILE_CONFIG_INFO_BACK_PAGE, pPageBuf, (T_U8 *)&spare))
    {
        return AK_FALSE;
    }

    //write nand config
    spare = NAND_CONFIG_FLAG;
    gFHAf.MemSet(pPageBuf, 0, sizeof(T_U8) * m_bin_nand_info.bin_page_size);
    
    gFHAf.MemCpy(pPageBuf, &m_nand_config, sizeof(m_nand_config));
    if(!Prod_NandWriteASAPage(0, m_bin_nand_info.page_per_block - NAND_CONFIG_INFO_BACK_PAGE, pPageBuf, (T_U8 *)&spare))
    {
        return AK_FALSE;
    }

//for test
#if 0
    gFHAf.Printf("asa start:%d\n", m_nand_config.ASAStartBlock);
    gFHAf.Printf("asa end:%d\n", m_nand_config.ASAEndBlock);
    gFHAf.Printf("resv start:%d\n", m_nand_config.ResvStartBlock);
    gFHAf.Printf("resv cnt:%d\n", m_nand_config.ResvBlockCount);
    gFHAf.Printf("bin start:%d\n", m_nand_config.BinStartBlock);
    gFHAf.Printf("bin end:%d\n", m_nand_config.BinEndBlock);
    gFHAf.Printf("bin cnt:%d\n", m_nand_config.BinFileCount);
    gFHAf.Printf("boot len:%d\n", m_nand_config.BootLength);
#endif
 
    return AK_TRUE;
}

/**
 * @BREIF    backup_Boot_and_fs
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_BOOL backup_Boot_and_fs(T_VOID)
{
    T_U8* pBoootBuf = AK_NULL;

    if (AK_NULL == m_fs_part_buf)
    {
        m_fs_part_buf = gFHAf.RamAlloc(m_bin_nand_info.bin_page_size);
    
        if(AK_NULL == m_fs_part_buf)
        {
            gFHAf.Printf("backup_Boot_and_fs(): malloc m_fs_part_buf fail\r\n");
            return AK_FALSE;
        }
        
        if (FHA_FAIL == FHA_get_fs_part(m_fs_part_buf, m_bin_nand_info.bin_page_size))
        {
            gFHAf.Printf("backup_Boot_and_fs(): FHA_get_fs_part fail\r\n");
            return AK_FALSE;
        }
    }

    if (!m_DloadBin.bWriteBoot)
    {
        T_U32 data_len;
        T_U32 page_size;
        T_U32 page_cnt;
        
        if (0 == m_nand_config.BootLength)
        {
            gFHAf.Printf("backup_Boot_and_fs(): nandboot length is 0\r\n");
            return AK_FALSE;
        }

        data_len  = m_nand_config.BootLength;
        page_size = m_bin_nand_info.boot_page_size;
        page_cnt  = (data_len + page_size - 1) / page_size;
        
        pBoootBuf = gFHAf.RamAlloc(page_cnt * page_size);
        
        if(AK_NULL == pBoootBuf)
        {
            gFHAf.Printf("backup_Boot_and_fs(): malloc boot buf fail\r\n");
            return AK_FALSE;
        }  
        
        if (!ForceRead_Nandboot(0, pBoootBuf, data_len))
        {
            gFHAf.Printf("backup_Boot_and_fs(): read boot fail\r\n");
            gFHAf.RamFree(pBoootBuf);
            return AK_FALSE;
        }  

        
        if (FHA_FAIL == fha_write_nandboot_begin(data_len))
        {
            gFHAf.Printf("backup_Boot_and_fs(): fha_write_nandboot_begin fail\r\n");
            gFHAf.RamFree(pBoootBuf);
            return AK_FALSE;
        }

        if (FHA_FAIL == fha_write_nandboot(pBoootBuf, data_len))
        {
            gFHAf.Printf("backup_Boot_and_fs(): fha_write_nandboot fail\r\n");
            gFHAf.RamFree(pBoootBuf);
            return AK_FALSE;
        }
    }

    gFHAf.RamFree(pBoootBuf);
    
    return AK_TRUE;
}

/**
 * @BREIF    write config information(bin map relation, fileconfig, nandconfig, pInformation on serial)
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @RETURN   T_BOOL
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL : fail
 */
T_U32 fha_WriteConfig(T_VOID)
{  
    T_U8* pbuf = AK_NULL;
    T_U32 spare;

    pbuf = gFHAf.RamAlloc(sizeof(T_U8) * m_bin_nand_info.bin_page_size);

    if(AK_NULL == pbuf)
    {
        gFHAf.Printf("fha_WriteConfig(): malloc buf fail\r\n");
        return FHA_FAIL;
    }

    if (!backup_Boot_and_fs())
    {
        gFHAf.RamFree(pbuf);
        gFHAf.Printf("fha_WriteConfig(): backup_Boot_and_fs fail\r\n");
        return FHA_FAIL;
    }

    gFHAf.Printf("FHA_NB write map\r\n");

    if(fha_WriteBinMap(pbuf))
    {
        gFHAf.Printf("FHA_NB write bin info\r\n");
        if(fha_WriteBinFileInfo(pbuf))
        {
            if(m_fs_part_buf != AK_NULL && !Prod_NandWriteASAPage(0, m_bin_nand_info.page_per_block - FS_PART_INFO_BACK_PAGE, m_fs_part_buf, (T_U8 *)&spare))
            {
                gFHAf.RamFree(pbuf);
                return FHA_FAIL;
            }
        }
    }

    gFHAf.RamFree(pbuf);

    return FHA_SUCCESS;
}

T_U32 fha_nand_burn_close(T_VOID)
{
    T_U32 ret;
    
    ret = fha_WriteConfig();

    if (m_fs_part_buf != AK_NULL)
    {
        gFHAf.RamFree(m_fs_part_buf);
        m_fs_part_buf = AK_NULL;
    }

    if (m_pBin_info.pMapBlockCnt != AK_NULL)
    {
        gFHAf.RamFree(m_pBin_info.pMapBlockCnt);
        m_pBin_info.pMapBlockCnt = AK_NULL;
    }
        
    return ret;
} 


/**
 * @BREIF    caculate bin file map relation start offset by bin index
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] bin file index in buffer m_pBin_info
 * @RETURN   value of offset
 */
static T_U16  CalMapTotalOffset(T_U32 fileIndex)
{
    T_U32 i;
    T_U16 offset=0;

    for(i=0; i<fileIndex; i++)
    {
        offset += m_pBin_info.pMapBlockCnt[i] * 4;
    }

    return offset;
}

   

/**
 * @BREIF    recover all bin file information when updat mode
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
static T_BOOL RecoverBinFileInfo()
{
    T_U32 spare;
    T_U8* pbuf = AK_NULL;
    T_BOOL ret = AK_FALSE;
    T_U32 i;
    T_U32 page_size;
    T_U32 block_size;           

    page_size = m_bin_nand_info.bin_page_size;
    block_size = m_bin_nand_info.page_per_block * page_size; 
 
    pbuf = gFHAf.RamAlloc(page_size);
    if(AK_NULL == pbuf)
    {
        gFHAf.Printf("FHA_NB alloc fail\r\n");
        return AK_FALSE;
    }

    //get file count by read data of m_nand_config
    if(!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - 2, pbuf, (T_U8 *)&spare))
    {
        goto EXIT;
    }
    gFHAf.MemCpy(&m_nand_config, pbuf, sizeof(m_nand_config));

    //get fileconfig
    if(!Prod_NandReadASAPage(0, m_bin_nand_info.page_per_block - 3, pbuf, (T_U8 *)&spare))
    {
        goto EXIT;
    }
    
    gFHAf.MemCpy(m_pBin_info.pFileConfig, pbuf, m_nand_config.BinFileCount * sizeof(T_FILE_CONFIG));

    //fill m_pBin_info
    for(i=0; i<m_nand_config.BinFileCount; i++)
    {
        T_U32 file_len, blockcnt, blockAddcnt;
        T_U32 j, map_page_cnt, buf_len, buf_offset;

        file_len = m_pBin_info.pFileConfig[i].file_length;
        blockcnt = (file_len + block_size - 1)/block_size;
        blockAddcnt = (file_len/10 + block_size -1)/block_size;

        //each bin block count include spare size
        m_pBin_info.pMapBlockCnt[i] = (T_U16)(blockcnt + blockAddcnt);



        buf_offset = 0;
        buf_len = m_pBin_info.pMapBlockCnt[i] * 4;

        map_page_cnt = (buf_len + m_bin_nand_info.bin_page_size - 1) / m_bin_nand_info.bin_page_size;

        
        for(j=0; j<map_page_cnt; j++)
        {
            T_U32 cpylen = ((buf_len - buf_offset) > m_bin_nand_info.bin_page_size) ? m_bin_nand_info.bin_page_size : (buf_len - buf_offset);

            if(!Prod_NandReadASAPage(0, m_pBin_info.pFileConfig[i].map_index+j, pbuf, (T_U8 *)&spare))
            {
                goto EXIT;
            }
            //storage map relation in case of not all bin will be update
            gFHAf.MemCpy((T_U8 *)((T_U32)m_pBin_info.pMapTotalBuf + CalMapTotalOffset(i) + buf_offset), pbuf,
                    cpylen);

            buf_offset += m_bin_nand_info.bin_page_size;
        }

        //no updateself data or updateself data after origin data when exit updateself data
        if((0 == m_pBin_info.pFileConfig[i].update_pos) 
            || (m_pBin_info.pFileConfig[i].update_pos > m_pBin_info.pFileConfig[i].run_pos))
        {
            if(i < m_nand_config.BinFileCount-1)
            {
                m_UpdateBlockLimit[i] = m_pBin_info.pFileConfig[i+1].run_pos;
            }
            else  //last bin
            {
                //fake address for file system start block
                m_UpdateBlockLimit[i] = m_nand_config.BinEndBlock;
            }
        }
        else // updateself data before origin data when exit updateself data
        {
            m_UpdateBlockLimit[i] = m_pBin_info.pFileConfig[i].run_pos;
        }
       
    }

    ret = AK_TRUE;

EXIT:
    gFHAf.RamFree(pbuf);
    
    return ret;
}



/**
 * @BREIF    Get write bin param by bin name when updat mode
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] bin file name
 * @RETURN   T_BOOL
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE : fail
 */
T_BOOL SetUpdateParam(T_U8 strName[])
{
    T_U32 i;
    T_U32 fileIndex = 0;

    /*
    search bin file config, suppose bin file config  was recovered by call function  
    RecoverBinFileInfo in BurnBin_init
    */
    for(i = 0; i < m_nand_config.BinFileCount; i++)
    {            
        if(0 == fha_str_cmp(strName, m_pBin_info.pFileConfig[i].file_name))
        {
             fileIndex = i;
             break;
        }     
    }
    
    if (i == m_nand_config.BinFileCount)
    {
        return FHA_FAIL;
    }
    else
    {
        m_DloadBin.file_index = fileIndex;  //index of this bin in buffer of m_pBin_info

        if (m_DloadBin.bUpdateSelf)
        {
            if (m_pBin_info.pFileConfig[fileIndex].update_pos == 0)
            {
                return FHA_FAIL;
            }
        }
        
        if (m_DloadBin.bBackup)
        {
            T_U16  *pbuf = AK_NULL;
            T_U32 page_size = m_bin_nand_info.bin_page_size;
            T_U32 page_addr;
            T_U32 spare;
            
            pbuf = (T_U16 *)gFHAf.RamAlloc(page_size);
            if (AK_NULL == pbuf)
            {
                return FHA_FAIL;
            } 

            page_addr = m_pBin_info.pFileConfig[fileIndex].map_index;
            if (!Prod_NandReadASAPage(0, page_addr, (T_U8 *)pbuf, (T_U8 *)&spare))
            {
                gFHAf.Printf("FHA_NB SetUpdateParam(): read fail\r\n");
                gFHAf.RamFree(pbuf);
                return FHA_FAIL;
            }             
            
            if (0 == pbuf[1])
            {
                gFHAf.RamFree(pbuf);
                return FHA_FAIL;
            }

            gFHAf.RamFree(pbuf);

        }

        //no updateself data        
        if(0 == m_pBin_info.pFileConfig[fileIndex].update_pos)
        {
            //update bin data start position is old run position, keep run_pos and update_pos value
            m_DloadBin.BinStartBlock = m_pBin_info.pFileConfig[fileIndex].run_pos;

#ifdef UPDATE_SELF
            // not to support updateself for system if update pos is 0
            return FHA_FAIL;
#endif
        }
        else //exist updateself data
        {
            //update bin data start position is position-value of update_pos
            m_DloadBin.BinStartBlock = m_pBin_info.pFileConfig[fileIndex].update_pos;

            //exchange update_pos and run_pos
            m_pBin_info.pFileConfig[fileIndex].update_pos = m_pBin_info.pFileConfig[fileIndex].run_pos;
            m_pBin_info.pFileConfig[fileIndex].run_pos    = m_DloadBin.BinStartBlock;
        }
    }

    gFHAf.Printf("FHA_NB RB:%d,UB:%d after exchange\r\n", m_pBin_info.pFileConfig[fileIndex].run_pos, m_pBin_info.pFileConfig[fileIndex].update_pos);

    return FHA_SUCCESS;
}


/**
 * @BREIF    write boot bin data to block0
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in/out] start page in block0 to write
 * @PARAM    [in] data buffer
 * @PARAM    [in] length to write
 * @RETURN   T_BOOL 
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE :     fail
 */
static T_BOOL ForceWrite_Nandboot(T_U32 *start_page, const T_U8 *pData, T_U32 length)
{
    T_U32 i;
    T_U32 page_write_size, cmp_size;
    T_U32 page_count;
    T_U32 spare = 0;
    T_BOOL ret = AK_TRUE;
    T_U8 *pTempData;
    T_U8 *pBuf;

    if (AK_NULL == start_page || AK_NULL == pData)
    {
        gFHAf.Printf("FHA_NB write nandboot para is NULL\r\n");
        return FHA_FAIL;
    }
    
    if (0 == length)
    {
        gFHAf.Printf("FHA_NB write nandboot data_len is 0\r\n");
        return FHA_FAIL;
    } 

    page_write_size = m_bin_nand_info.boot_page_size;   //this value maybe smaller than pagesize if limit to 2K
    
    page_count = (length + page_write_size - 1) / page_write_size;  //total page count to write

    if (page_count > m_DloadBin.totalpagecnt)
    {
        gFHAf.Printf("FHA_NB write nandboot data_len is large\r\n");
        return FHA_FAIL;
    }
    else if (page_count < m_DloadBin.totalpagecnt)
    {
        if (length != page_write_size * page_count)
        {
            gFHAf.Printf("FHA_NB write nandboot data_len is not page multiple\r\n");
            return FHA_FAIL;
        }
    } 
    else
    {
        if ((length % page_write_size) != (m_nand_config.BootLength % page_write_size))
        {
            gFHAf.Printf("FHA_NB write nandboot data_len is error!\r\n");
            return FHA_FAIL;
        }
    }

    //erase block0 before starting write page0
    if(0 == *start_page)
    {
        if(!Prod_NandEraseBlock(0))
        {
            return AK_FALSE;
        }
    }
    
    pBuf = (T_U8 *)gFHAf.RamAlloc(page_write_size);
    if(AK_NULL == pBuf)
    {
        gFHAf.Printf("FHA_NB write nandboot malloc error\r\n");
        return AK_FALSE;
    }
    
    for(i = 0; i < page_count; i++)
    {
        pTempData = (T_U8 *)(pData + i * page_write_size);
        
        if(!Prod_NandWriteBootPage(*start_page, pTempData))
        {
            ret = AK_FALSE;
            break;
        }
         
         if (!Prod_NandReadBootPage(*start_page, pBuf))
         {
            ret = AK_FALSE;
            break;
         }
         
         if (FHA_CHIP_880X != g_burn_param.eAKChip && 0 == *start_page)
         {
            if(FHA_CHIP_10XX == g_burn_param.eAKChip)
            {
                cmp_size = NAND_BOOT_PAGE_FIRST_10L;
            }
            else
            {
                cmp_size = NAND_BOOT_PAGE_FIRST;
            }
         }
         else
         {
            cmp_size = page_write_size;
         }
         
         if(0 != gFHAf.MemCmp(pBuf, pTempData, cmp_size))
         {
            gFHAf.Printf("FHA_NB write nandboot cmpare data error\r\n");
            ret = AK_FALSE;
            break;
         }            

        (*start_page)++;
    }

    m_DloadBin.totalpagecnt -= page_count;
    gFHAf.RamFree(pBuf);
    
    return ret;
}

T_BOOL ForceRead_Nandboot(T_U32 start_page, T_U8 *pData, T_U32 length)
{
    T_U32 i;
    T_U32 page_read_size;
    T_U32 page_count;
    T_U8 *pBuf = AK_NULL;    
    T_BOOL ret = AK_TRUE;

    if (AK_NULL == pData)
    {
        gFHAf.Printf("FHA_NB read nandboot para is NULL\r\n");
        return AK_FALSE;
    }
    
    if (0 == length)
    {
        gFHAf.Printf("FHA_NB read nandboot data_len is 0\r\n");
        return AK_FALSE;
    } 

    page_read_size = m_bin_nand_info.boot_page_size;   //this value maybe smaller than pagesize if limit to 2K
    page_count = (length + page_read_size - 1) / page_read_size;  //total page count to write
    pBuf = (T_U8 *)gFHAf.RamAlloc(page_read_size);
    if(AK_NULL == pBuf)
    {
        gFHAf.Printf("FHA_NB read nandboot malloc fail\r\n");
        return AK_FALSE;
    }

    for(i = 0; i < page_count; i++)
    {
         if (!Prod_NandReadBootPage(start_page + i, pBuf))
         {
             gFHAf.Printf("FHA_NB read nandboot fail\r\n");
            ret = AK_FALSE;
            break;
         }

         if ((i == page_count-1) && (length % page_count) != 0)
         {
             gFHAf.MemCpy(pData+i*page_read_size, pBuf, length % page_read_size);
         }
         else
         {
             gFHAf.MemCpy(pData+i*page_read_size, pBuf, page_read_size);
         }
    }

    gFHAf.RamFree(pBuf);
    return ret;
}

/**
 * @BREIF    write bin data only to a block 
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in/out] origin and backup block to write
 * @PARAM    [in] start page in block to write
 * @PARAM    [in] page count to write
 * @PARAM    [in] data buffer
 * @RETURN   T_U32 
 * @retval   not 0 :  page count to write
 * @retval   0 :     fail
 */
static T_U32 ForceWrite_NandBin_Block(T_PROD_BLOCK *end_block, T_U32 start_page, T_U32 page_count, const T_U8 *pData)
{
    T_U32 i;
    T_U32 blk_to_write, blk_to_read;
    T_U8 *pBuf = AK_NULL;
    T_BOOL bMoveData = AK_FALSE;
    T_U32 spare = 0;
    T_U32 try_times = 0;
    T_U32 success_times = 0;
    T_U32 page_size= m_bin_nand_info.bin_page_size;
    T_U32 page_data_size = m_bin_nand_info.bin_page_size;
    T_U32 data_write_times;

    if(start_page + page_count > m_bin_nand_info.page_per_block)
    {
        return 0;
    }

    if (m_DloadBin.bBackup)
    {
        data_write_times = 2;
    }
    else
    {
        data_write_times = 1;
        end_block->block_backup = 0;
    }
   
    blk_to_write = end_block->block_origin;
    blk_to_read = end_block->block_backup;

    do
    {
        //erase block if start from page 0
        if(0 == start_page || bMoveData)
        {
            gFHAf.Printf("wb:%d\r\n",blk_to_write);

            if(blk_to_write >= m_bin_nand_info.block_per_plane)
            {
                gFHAf.Printf("FHA_NB block:%d beyond a plane:%d\r\n",blk_to_write, m_bin_nand_info.block_per_plane);
                return 0;
            }

            //check whether beyond limit when update mode
            if(MODE_UPDATE == g_burn_param.eMode)
            {
                if(blk_to_write >= m_UpdateBlockLimit[m_DloadBin.file_index])
                {
                    gFHAf.Printf("FHA_NB block:%d beyond limit block:%d\r\n",blk_to_write, m_UpdateBlockLimit[m_DloadBin.file_index]);
                    return 0;
                }
            }
            
            if(Prod_IsBadBlock(blk_to_write))
            {
                gFHAf.Printf("FHA_NB BB:%d\r\n",blk_to_write);
                goto WRITE_FAIL;
            }
           
            if(!Prod_NandEraseBlock(blk_to_write))
            {
                gFHAf.Printf("FHA_NB EF:%d\r\n",blk_to_write);
                goto WRITE_FAIL;
            }
        }

        //if fail in last write, we need to move the data in pages before start block
        if(bMoveData)
        {
            pBuf = (T_U8 *)gFHAf.RamAlloc(page_size);
            if(AK_NULL == pBuf)
            {
                return 0;
            }
            
            for(i = 0; i < start_page; i++)
            {
                if(!Prod_NandReadBinPage(blk_to_read, i, pBuf, (T_U8 *)&spare))
                {
                    gFHAf.RamFree(pBuf);
                    return 0;
                }

                if(!Prod_NandWriteBinPage(blk_to_write, i, pBuf, (T_U8 *)&spare))
                {
                    gFHAf.RamFree(pBuf);
                    goto WRITE_FAIL; 
                }
            }

            gFHAf.RamFree(pBuf);
        }
        if (m_DloadBin.bCheck)
        {
            pBuf = (T_U8 *)gFHAf.RamAlloc(page_size);
            if(AK_NULL == pBuf)
            {
                return 0;
            }
        }
        //write data
        for(i = 0; i < page_count; i++)
        {
            T_U32 j;
            spare = 0;   

            if (PLAT_SPOT == g_burn_param.ePlatform)
            {
                for (j=0; j<(page_data_size>>2); j++)
                {
                      if (((T_U32*)(pData+i*page_data_size))[j] != 0xffffffff)
                        break;
                }

                if(j==(page_data_size>>2))
                {
                    spare = 0x11235813;
                }
            }
            
            if(!Prod_NandWriteBinPage(blk_to_write, start_page+i, (T_U8 *)pData+i*page_data_size, (T_U8*)&spare))
            {
                break;
            }
            
            if (m_DloadBin.bCheck)
            {
                const T_U8 *pTempData = AK_NULL;

                pTempData = pData + i * page_data_size;
                if(!Prod_NandReadBinPage(blk_to_write, start_page+i, pBuf, (T_U8*)&spare))
                {
                    break;
                }

                if(0 != gFHAf.MemCmp(pBuf, pTempData, page_size))
                {
                    /*
                    T_U32 t;
                    for (t=0; t<page_size; t++)
                    {
                        if(pTempData[t] != pBuf[t])
                            gFHAf.Printf("%d_%02x->%02x ", t, pTempData[t], pBuf[t]); 
                    }
                    */
                    gFHAf.Printf("\r\nFHA_NB Compare fail \r\n");
                    break;
                }                
            }
            
        }

        if (m_DloadBin.bCheck)
            gFHAf.RamFree(pBuf);

        if(i >= page_count)
        {
            bMoveData = AK_FALSE;

            blk_to_read = end_block->block_origin;
            blk_to_write = end_block->block_backup;

            success_times++;  
        }
        else
        {
WRITE_FAIL:
            gFHAf.Printf("FHA_NB MB:%d\r\n",blk_to_write);

            bMoveData = AK_TRUE;

            blk_to_write = (blk_to_write > blk_to_read) ? (blk_to_write+1) : (blk_to_read+1);

            //faile to modify endblock
            if(success_times > 0)
            {
                end_block->block_backup = blk_to_write;
            }
            else
            {
                end_block->block_origin = blk_to_write;
            }
        }

        try_times++;
        
    }
    while(success_times < data_write_times && try_times < 50);

    if(try_times < 50)
    {
        return page_count;
    }
    else
    {
        return 0;
    }
}

T_U32 burn_nand_bin_end(T_U32* endblock)
{
    T_FILE_CONFIG* pfile_cfg = m_pBin_info.pFileConfig + m_DloadBin.file_index;
    T_U32 block_size = m_bin_nand_info.page_per_block * m_bin_nand_info.bin_page_size;           
    T_U32 bin_blk_cnt = (pfile_cfg->file_length + block_size - 1)/block_size;
    T_U32 enlarge_blk_cnt = 0;
    T_U32 real_blk_cnt_used;
    
    if(m_bin_resv_size != 0)
    {
        //bin区大小”有赋值，且该值 < 文件大小*110%时，根据“文件大小*110%”划分空间
        if(m_bin_resv_size <= (pfile_cfg->file_length+pfile_cfg->file_length/10))
        {
            enlarge_blk_cnt = (pfile_cfg->file_length / 10 + block_size - 1) / block_size;
        }
        else
        {
             //bin区大小”有赋值，且该值 > 文件大小*110%时，根据“bin区大小”划分空间
            enlarge_blk_cnt = ((m_bin_resv_size - pfile_cfg->file_length) + block_size - 1) / block_size;
        }
    }
    else
    {
        //bin区大小”无赋值，根据“文件大小*110%”划分空间
        enlarge_blk_cnt = (pfile_cfg->file_length / 10 + block_size - 1) / block_size;
    }
    gFHAf.Printf("enlarge_blk_cnt:%d\r\n", enlarge_blk_cnt);

    if (m_DloadBin.bBackup)
    {
        enlarge_blk_cnt *= 2;
    } 
    
    real_blk_cnt_used = extend_nandbin_space(*endblock, enlarge_blk_cnt);

    if (real_blk_cnt_used < enlarge_blk_cnt)
    {
        return FHA_FAIL;
    }
    //gFHAf.Printf("real_blk_cnt_used:%d\r\n", real_blk_cnt_used);
    //gFHAf.Printf("*endblock:%d\r\n", *endblock);
    *endblock += real_blk_cnt_used;
    //gFHAf.Printf("*endblock:%d\r\n", *endblock);
    if (m_DloadBin.bUpdateSelf)
    {
        pfile_cfg->update_pos = *endblock;

        gFHAf.Printf("FHA_NB updata_pos:%d\r\n", pfile_cfg->update_pos);

        if (m_DloadBin.bBackup)
        {
            bin_blk_cnt *= 2;
        } 

        bin_blk_cnt += enlarge_blk_cnt;

        real_blk_cnt_used = extend_nandbin_space(*endblock, bin_blk_cnt);

        if (real_blk_cnt_used < bin_blk_cnt)
        {
            return FHA_FAIL;
        } 

        *endblock += real_blk_cnt_used;
    }

    m_DloadBin.file_index++;

    return FHA_SUCCESS;
}    
    

T_U32 extend_nandbin_space(T_U32 start_block, T_U32 blk_cnt)
{
    T_U32 i, j;
    T_U32 resv_blk_cnt = blk_cnt;
    T_U32 spare = BIN_RESV_FLAG;
    T_U8 *data = AK_NULL;

    data = gFHAf.RamAlloc(m_bin_nand_info.bin_page_size);

    if (AK_NULL == data)
        return 0;
    
    gFHAf.MemSet(data, 0, m_bin_nand_info.bin_page_size);
    
    for (i=0; i<resv_blk_cnt; i++)
    {
        T_U32 blk_index = start_block + i;
        if (Prod_IsBadBlock(blk_index))
        {
            gFHAf.Printf("FHA_NB EBB:%d\r\n", blk_index);
            resv_blk_cnt++;
            continue;
        } 

        if (!Prod_NandEraseBlock(blk_index))
        {
            
            resv_blk_cnt++;
            continue;
        } 

        for (j=0; j<m_bin_nand_info.page_per_block; j++)
        {
            if(!Prod_NandWriteBinPage(blk_index, j, data,  (T_U8 *)&spare))
            {
                gFHAf.Printf("FHA_NB WBf:%d\r\n", blk_index);
                resv_blk_cnt++;
                break;
            }    
        }

        if (resv_blk_cnt > BIN_MAX_BLOCK_COUNT)
        {
            resv_blk_cnt =  0;
            break;
        }    
    } 
    
    gFHAf.RamFree(data);

    return resv_blk_cnt;
}    

#if 0
/**
 * @BREIF    read bin data to buffer
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in] map table of bin data
 * @PARAM    [in] block index
 * @PARAM    [in] start page to read
 * @PARAM    [in] buffer
 * @PARAM    [in] length to read
 * @RETURN   T_U32
 * @retval   NOT 0 :  end page in last block
 * @retval   0 :     fail
 */
T_U32 Fwl_ForceRead(T_U16 pMap[], T_U32 *index, T_U32 start_page, T_U8 pData[], T_U32 length)
{  
    T_U32 i;
    T_U32 page_cnt;
    T_U32 start_page_block;
    T_U32 page_size         =  m_bin_nand_info.bin_page_size;
    T_U32 page_per_block    =  m_bin_nand_info.page_per_block;
    T_U32 page_cnt_total    =  0;
    T_U16 block_to_read;
    T_U32 spare;
    T_U32 tmp_index = *index;

    page_cnt = (length-1) / page_size + 1;
  
    for(i=0; i<page_cnt; i++)
    {
        start_page_block = (page_cnt_total + start_page) % page_per_block;  //start page of block to read
        block_to_read = Prod_PB2LB(pMap[tmp_index * 2]);  //logic block of origin data

        if(!Prod_NandReadBinPage(block_to_read, start_page_block, pData+page_size*page_cnt_total, &spare))
        {
            block_to_read = Prod_PB2LB(pMap[tmp_index * 2+1]); //logic block of backup data

            if(!Prod_NandReadBinPage(block_to_read, start_page_block, pData+page_size*page_cnt_total, &spare))
            {
                return 0;
            }
        }

        page_cnt_total++;  //increase page cout to read

        tmp_index = *index + (page_cnt_total + start_page) / page_per_block; //caculate map index
    }

    *index = tmp_index;

    return start_page_block+1;
}

/**
 * @BREIF    read boot bin data to buffer
 * @AUTHOR   Jiang Dihui
 * @DATE     2009-09-10
 * @PARAM    [in/out] start page in block0 to read
 * @PARAM    [in] buffer
 * @PARAM    [in] length to read
 * @RETURN   T_BOOL 
 * @retval   AK_TRUE :  succeed
 * @retval   AK_FALSE :     fail
 */
T_BOOL Fwl_ForceRead_Nandboot(T_U32 *start_page, T_U8 pData[], T_U32 length)
{
    T_U32 i;
    T_U32 page_read_size;
    T_U32 page_count;
    T_BOOL ret = AK_TRUE;
    
    page_read_size = m_bin_nand_info.boot_page_size;  //this value maybe smaller than pagesize if limit to 2K

    page_count = (length-1) / page_read_size + 1;  //total page count to read

    for(i = 0; i < page_count; i++)
    {
        //if(!gFHAf.ReadBootPage(*start_page, pData+ i*page_read_size))
        {
            ret = AK_FALSE;
            break;
        }  

        (*start_page)++; //increase start page
    }

    return ret;    
}

#endif
