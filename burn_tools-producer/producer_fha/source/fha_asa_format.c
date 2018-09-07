 /**
  * @FILENAME: fha_asa_format.c
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
#include "fha_asa.h"
#include "asa_format.h"

//check wether a block is initial bad block or not
/**
 * @BREIF    check_block
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_BOOL check_block(T_U32 chip, T_U32 block);
extern T_VOID asa_global_init();


extern T_U8 m_hinfo_data[HEAD_SIZE];

/**
 * @BREIF    asa_scan_ewr
  * 功能描述:
 * 进行野蛮的擦读写，会破坏
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_S8 asa_scan_ewr(T_U8 pBuf[], T_U32 len, T_U8 mark)
{
    T_U32 block_count = 0;

    T_U32 chip;
    T_U32 block;
    T_U32 page_size;

    T_U32 byte_loc;
    T_U32 byte_offset;

    T_U32 i, j, k;

    T_U8 *pData = AK_NULL;

    T_U32 spare;

    if(AK_NULL == pBuf)
    {
        return SA_ERROR;
    }
    /*
        查找有多少个block    
      */
    block_count = m_fha_asa_param.BlockNum  * g_burn_param.nChipCnt;
    if(block_count > len * 8)
    {
        return SA_ERROR;
    }

    page_size = m_fha_asa_param.BytesPerPage;
    pData = gFHAf.RamAlloc(page_size);
    if(AK_NULL == pData)
    {
        gFHAf.Printf("alloc fail\n");
        while(1);
    }
    gFHAf.MemSet(pData, mark, page_size);
    gFHAf.MemSet(&spare, mark, 4);
    gFHAf.Printf("spare: %x\r\n", spare);

    gFHAf.Printf("before erase and write: %d\r\n", block_count);
    /*
       实现对nand的所以block擦读写，这里首先擦除和写入挑出坏快
      */
    for(i = 0; i < block_count; i++)
    {
        gFHAf.Printf("%d,", i);
        chip = i / (m_fha_asa_param.BlockNum);
        block = i % (m_fha_asa_param.BlockNum);
            
        byte_loc = i/8;
        byte_offset = 7 - i%8;

        if(!Prod_NandEraseBlock(block))
        {
            gFHAf.Printf("@@@Erase Bad Block: %d\r\n", i);
            pBuf[byte_loc] |= 1 << byte_offset;
            continue; 
        }

        for(j = 0; j < m_fha_asa_param.PagePerBlock; j++)
        {
            if(!Prod_NandWriteASAPage(block, j, pData, (T_U8*)&spare))
            {
                gFHAf.Printf("@@@Write Bad Block: %d\r\n", i);
                pBuf[byte_loc] |= 1 << byte_offset;
                break; 
            }
        }
    }

    gFHAf.MemSet(pData, 0, page_size);
    spare = 0;

    gFHAf.Printf("\r\nbefore read\r\n");
    /*
       实现对nand的所以block擦读写，这里是写入
      */
    
    for (i = 0; i <block_count; i++)
    {
        gFHAf.Printf("%d,", i);
        chip = i / m_fha_asa_param.BlockNum;
        block = i % m_fha_asa_param.BlockNum;
        
        byte_loc = i/8;
        byte_offset = 7 - i%8;
        
        for(j = 0; j < m_fha_asa_param.PagePerBlock; j++)
        {
            if(!Prod_NandReadASAPage(block, j, pData, (T_U8*)&spare))
            {
                gFHAf.Printf("@@@Read Bad Block: %d\r\n", i);
                pBuf[byte_loc] |= 1 << byte_offset;
                goto BAD_BLOCK; 
            }
            else
            {
                
                for(k = 0; k < 4; k++)
                {
                    if(((spare >> k*8) & 0xFF) != mark)
                    {
                        gFHAf.Printf("@@@Spare Changed Bad Block: %d, %x, %x\r\n", i, (spare >> k*8) & 0xFF, mark);
                        pBuf[byte_loc] |= 1 << byte_offset;
                        goto BAD_BLOCK;
                    }
                }
                for(k = 0; k < page_size; k++)
                {
                    if(pData[k] != mark)
                    {
                        gFHAf.Printf("@@@Data Changed Bad Block: %d\r\n", i);
                        pBuf[byte_loc] |= 1 << byte_offset;
                        goto BAD_BLOCK;
                    }
                }
            }
BAD_BLOCK:
            spare = 0;
            gFHAf.MemSet(pData, 0, page_size);
            break;
        }
    }       

    gFHAf.RamFree(pData);
    pData = AK_NULL;
    
    return SA_SUCCESS;
}

/**
 * @BREIF    count_bit0
 * 功能描述:
 * 
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */


static T_U8 count_bit0(T_U8 d)
{
    T_U8 i, j = 0;
    for(i = 0; i < 8; i++)
    {
        if(0 == (d & (0x1 << i)))
        {
            j++;
        } 
    }

    return j;         
}

//scan initial bad blocks
/**
 * @BREIF    asa_scan_normal
 * 功能描述:
 *  正常的扫描，提出出厂坏快
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_S8 asa_scan_normal(T_U8 pBuf[], T_U32 len)
{
    T_U32 block_count = 0;

    T_U32 chip;
    T_U32 block;

    T_U32 byte_loc;
    T_U32 byte_offset;

    T_U32 i, j;
    T_U32 spare;
    T_U32 k;
    T_U32 bad_block_cnt = 0;

    T_BOOL bUsed = AK_FALSE;

    if(AK_NULL == pBuf)
    {
        return SA_ERROR;
    }
    /*
       获取整个nand有多少个block
      */
    
    block_count = m_fha_asa_param.BlockNum * g_burn_param.nChipCnt;
    if(block_count > len*8)
    {
        return SA_ERROR;
    }
    /*
      逐个扫描
      */

    for(i = 0; i < m_fha_asa_param.PagePerBlock; i++)
    {
        k = 0;
        //check if block 0 is null or not
        gFHAf.MemSet(pBuf, 0x0, len);
        Prod_NandReadASAPage(0, i, pBuf, (T_U8 *)&spare);
       
        for(j = 0; j < m_fha_asa_param.BytesPerPage; j++)
        {
            if(pBuf[j] != 0xFF)
            {
               k += count_bit0(pBuf[j]);
                
            }

            if(k > 8)
            {
                bUsed = AK_TRUE;
                goto CHECK_BAD_BLOCK;
            }
        }
    }

CHECK_BAD_BLOCK:   
    gFHAf.MemSet(pBuf, 0, len);
    //check bad block
    if(!bUsed)
    {     
        for(i = 0; i < block_count; i++)
        {
            chip = i / m_fha_asa_param.BlockNum;
            block = i % m_fha_asa_param.BlockNum;

            byte_loc = i/8;
            byte_offset = 7 - i%8;     
            
            if ((PLAT_SPOT == g_burn_param.ePlatform) 
                && (1 == g_nand_phy_info.col_cycle) && (2048 == m_fha_asa_param.BytesPerPage))
            {
                T_U32 m;
                T_U32 block_small;
                for(m=0; m<8; m++)
                {
                    block_small = block * 8 + m;
                    if(AK_FALSE == check_block(chip, block_small))
                    {
                        pBuf[byte_loc] |= 1 << byte_offset;
                        bad_block_cnt++; 
                        break;
                    }
                }    
            }
            else 
            {
                if(AK_FALSE == check_block(chip, block))
                {
                    gFHAf.Printf("@@@Initial invalid block: %d\r\n", i);
                    pBuf[byte_loc] |= 1 << byte_offset;
                    bad_block_cnt++; 
                }
            }
        }

        //if bad block count <= 4%, think bad block is initial bad block
        if(bad_block_cnt > block_count * 4 / 100)
        {
            gFHAf.Printf("bad block is initial bad block!!!\r\n");
            gFHAf.MemSet(pBuf, 0, len);
        }    
        
    }
    return SA_SUCCESS;
}

//check wether a block is initial bad block or not
/**
 * @BREIF    check_block
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_BOOL check_block(T_U32 chip, T_U32 block)
{
    T_U32 page_count = 0;
    T_U32 pages[8] = {0};
    T_U32 column_count = 0;
    T_U32 columns[8] = {0};
    T_U16 real_pages_per_block;
    T_U32 i,j;
    T_U32 rowAddr, columnAddr;
    T_U32 page_size = 0;
    T_U8 IBB_type = g_nand_phy_info.custom_nd;
    
    page_size = g_nand_phy_info.page_size;
    real_pages_per_block = g_nand_phy_info.page_per_blk;
    //in enhanced SLC mode , FHA just use the half pages of a block. must be reverted, when checking bad block
    if (g_nand_phy_info.flag & (1 << 24))
        real_pages_per_block <<= 1;


    //samsung
    if(NAND_TYPE_SAMSUNG == IBB_type)
    {
        //slc
        if(real_pages_per_block < 128)
        {
            page_count = 2;
            pages[0] = 0;
            pages[1] = 1;

            //small page
            if(page_size < 2048)
            {
                column_count = 1;
                columns[0] = page_size + 5;
            }
            else
            {
                column_count = 1;
                columns[0] = page_size;
            }
        }
        else //mlc
        {
            page_count = 1;
            pages[0] = real_pages_per_block - 1;

            column_count = 1;
            columns[0] = page_size;
        }
    }

    //hynix
    if(NAND_TYPE_HYNIX == IBB_type)
    {
        //slc
        if(real_pages_per_block < 128)
        {
            page_count = 2;
            pages[0] = 0;
            pages[1] = 1;

            //small page
            if(page_size < 2048)
            {
                column_count = 1;
                columns[0] = page_size + 5;
            }
            else
            {
                column_count = 1;
                columns[0] = page_size;
            }
        }
        else
        {
            page_count = 2;
            pages[0] = real_pages_per_block - 1;
            pages[1] = real_pages_per_block - 3;

            column_count = 1;
            columns[0] = page_size;
        }
    }

    //toshiba normal
    if(NAND_TYPE_TOSHIBA == IBB_type)
    {
        column_count = 2;
        columns[0] = 0;
        columns[1] = page_size;
        
        if(real_pages_per_block < 128)
        {
            page_count = 2;
            pages[0] = 0;
            pages[1] = 1;
        }
        else
        {
            page_count = 1;
            pages[0] = real_pages_per_block - 1;
        }
    }

    //toshiba with extented blocks
    if(NAND_TYPE_TOSHIBA_EXT == IBB_type)
    {
        column_count = 2;
        columns[0] = 0;
        columns[1] = page_size;
        
        if(real_pages_per_block >= 128)
        {
            page_count = 2;
            pages[0] = 0;
            pages[1] = real_pages_per_block - 1;
        }
        else
        {
            gFHAf.Printf("asa check block param err\r\n");
            while(1);
        }    
    }

    //Micron
    if(NAND_TYPE_MICRON == IBB_type)
    {
        page_count = 2;
        pages[0] = 0;
        pages[1] = 1;

        column_count = 1;
        columns[0] = page_size;
    }
    //4k 的micron
    if(NAND_TYPE_MICRON_4K == IBB_type)
    {
        T_U32 count = 0;
        T_U8 *pData = AK_NULL;
        
        rowAddr = block * real_pages_per_block;
        columnAddr = page_size;

        pData = (T_U8 *)gFHAf.RamAlloc(256);
        
        gFHAf.ReadNandBytes(chip, rowAddr, columnAddr,  pData, 256);
        
        count = 0;
        for(i = 0; i < 218; i++)
        {
            if(0 == pData[i])
            {
                count++;
                //printf("%d_0x%x ", i, pData[i]);
            }
        }

        gFHAf.RamFree(pData);
        if(count > 0)
        {
            return AK_FALSE;
        }
        else
        {
            return AK_TRUE;
        }               
    }

    //ST
    if(NAND_TYPE_ST == IBB_type)
    {
        if(real_pages_per_block < 128)
        {
            if(page_size < 2048)
            {
                page_count = 2;
                pages[0] = 0;
                pages[1] = 1;

                column_count = 1;
                columns[0] = page_size+5;
            }
            else
            {
                page_count = 1;
                pages[0] = 0;

                column_count = 2;
                columns[0] = page_size;
                columns[1] = page_size+5;
            }
        }
        else
        {
            page_count = 1;
            pages[0] = real_pages_per_block - 1;

            column_count = 1;
            columns[0] = 0;
        }
    }
    
    //MIRA
    if(NAND_TYPE_MIRA == IBB_type)
    {
            page_count = 2;
            pages[0] = 0;
            pages[1] = real_pages_per_block - 1;

            column_count = 2;
            columns[0] = 0;
            columns[1] = page_size - 1;
    }

    //Other
    if(NAND_TYPE_UNKNOWN == IBB_type)
    {
    }

    if(0 == page_count || 0 == column_count)
    {
         gFHAf.Printf("asa IBB_type err\r\n");
         while(1);
    }

    //check for initial bad block
    for(i = 0; i < page_count; i++)
    {
        for(j = 0; j < column_count; j++)
        {
            T_U8 data[64];
            rowAddr = block * real_pages_per_block + pages[i];
                       
            columnAddr = columns[j];
            
            gFHAf.ReadNandBytes(chip, rowAddr, columnAddr,  data, 64);

            if(data[0] != 0xFF)
            {
                return AK_FALSE;
            }
        }
    }
    
    return AK_TRUE;
}

/**
 * @BREIF    asa_format
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_BOOL asa_format(T_U32 type)
{
    T_U32 i;
    T_U8 *buf_badblock, *buf_other, *dummy_buf;
    T_U32 block;
    T_U32 spare = 0;
    T_ASA_HEAD *pHead = AK_NULL;
    T_ASA_ITEM *pItem = AK_NULL;
    T_U32 block_count = 0; 
    T_U32 badblock_page_count = 0;
    T_U32 block_get = 0;
    T_U32 block_try = 0;
    
    
    block_count = m_fha_asa_param.BlockNum * g_burn_param.nChipCnt;
    badblock_page_count = (block_count - 1) / (m_fha_asa_param.BytesPerPage * 8) + 1;

    //alloc memory
    buf_badblock = gFHAf.RamAlloc(badblock_page_count * m_fha_asa_param.BytesPerPage);
    dummy_buf = gFHAf.RamAlloc(m_fha_asa_param.BytesPerPage);
    
    if((AK_NULL == buf_badblock) || (AK_NULL == dummy_buf))
    {
        return AK_FALSE;
    }

    buf_other = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if( AK_NULL == buf_other)
    {
        gFHAf.RamFree(buf_badblock);        
        gFHAf.RamFree(dummy_buf);
        return AK_FALSE;
    }
    
    
    gFHAf.MemSet(buf_badblock, 0, badblock_page_count * m_fha_asa_param.BytesPerPage);
    gFHAf.MemSet(dummy_buf, 0, m_fha_asa_param.BytesPerPage);//fill the dummy buf with 0x0

    switch(type)
    {
        case ASA_FORMAT_NORMAL:
            asa_scan_normal(buf_badblock, badblock_page_count * m_fha_asa_param.BytesPerPage);
            break;
        case ASA_FORMAT_EWR:
            asa_scan_ewr(buf_badblock, badblock_page_count * m_fha_asa_param.BytesPerPage, 0x55);
            break;
        default:
            break;
    }
    //for test factory 
#if 0
    m_asafun.Printf("+++++++++++++for test");
    while(1);
#endif
    //prepare for data in page 0
    //head
    pHead = (T_ASA_HEAD *)buf_other;

    gFHAf.MemCpy(pHead->head_str, m_hinfo_data, HEAD_SIZE);
    pHead->verify[0] = 0x0;
    pHead->verify[1] = 0x0;
    pHead->item_num = 2;
    pHead->info_end = 1 + badblock_page_count * 2;

    //initial bad block
    pItem = (T_ASA_ITEM *)(buf_other + sizeof(T_ASA_HEAD));

    pItem->page_start = 1;
    pItem->page_count = (T_U16)badblock_page_count;
    pItem->info_start = 0;
    pItem->info_len = 0;

    //total bad block
    pItem = (T_ASA_ITEM *)(buf_other + sizeof(T_ASA_HEAD) + sizeof(T_ASA_ITEM));

    pItem->page_start = (T_U16)(1 + badblock_page_count);
    pItem->page_count = (T_U16)badblock_page_count;
    pItem->info_start = 0;
    pItem->info_len = 0;

    //write data
    gFHAf.MemSet(m_fha_asa_block.asa_blocks, 0, ASA_BLOCK_COUNT);
    m_fha_asa_block.write_time = 1;
    block = 0;
    block_try = 0;
    block_get = 0;

    do
    {
        T_BOOL ret;
        #define CHECK(ret) if(!ret){Prod_NandEraseBlock(block);\
                buf_badblock[block/8] |= 1 << (7 - block%8);continue;}

        block_try++;
        m_fha_asa_block.asa_block_cnt = (T_U8)block_try;
        block ++;

        //ignore initial bad block
        if((buf_badblock[block/8] & (1 << (7 - block%8))) != 0)
        {
            continue;
        }
        
        ret = Prod_NandEraseBlock(block);
        CHECK(ret);

        //we use ASA_BLOCK_COUNT valid blocks as Secure Area and additional one after them as a tail block.
        //the spare in tail block is "0x0".
        spare = (m_fha_asa_block.write_time > ASA_BLOCK_COUNT) ? 0 : m_fha_asa_block.write_time;
        
        //write page 0
        //ret = m_asafun.WritePage(0, block, 0, buf_other,(T_U8*)&spare, 4);
        ret = Prod_NandWriteASAPage(block, 0, buf_other,(T_U8*)&spare);
        CHECK(ret);

        //write initial bad block
        for(i= 0; i < badblock_page_count; i++)
        {
            //ret = m_asafun.WritePage(0, block, 1+i, buf_badblock + i * m_asafun.BytesPerPage, (T_U8*)&spare, 4);
            ret = Prod_NandWriteASAPage(block, 1+i, buf_badblock + i * m_fha_asa_param.BytesPerPage, (T_U8*)&spare);
            if(!ret)
            {
                break;
            }
        }
        CHECK(ret);

        //write total bad block
        for(i= 0; i < badblock_page_count; i++)
        {
            //ret = m_asafun.WritePage(0, block, 1 + badblock_page_count + i, buf_badblock + i * m_asafun.BytesPerPage, (T_U8*)&spare, 4);
            ret = Prod_NandWriteASAPage( block, 1 + badblock_page_count + i, buf_badblock + i * m_fha_asa_param.BytesPerPage, (T_U8*)&spare);
            if(!ret)
            {
                break;
            }
        }
        CHECK(ret);

        //fill the dummy data in gap-pages to keep pages from bit-flips  
        for (i = 1 + badblock_page_count + i; i < (T_U32)m_fha_asa_param.PagePerBlock - 1; i++)
        {
            ret = Prod_NandWriteASAPage(block, i, dummy_buf, (T_U8*)&spare);
            if(!ret)
            {
                break;
            }
        }
        CHECK(ret);
        
        //write last page
        //ret = m_asafun.WritePage(0, block, m_asafun.PagePerBlock-1, buf_other, (T_U8*)&spare, 4);
        ret = Prod_NandWriteASAPage(block, m_fha_asa_param.PagePerBlock-1, buf_other, (T_U8*)&spare);
        CHECK(ret);

        if(m_fha_asa_block.write_time <= ASA_BLOCK_COUNT)
        {
            m_fha_asa_block.asa_head = (T_U8)block_get;
            m_fha_asa_block.asa_blocks[block_get++] = (T_U8)block;
            m_fha_asa_block.asa_count = (T_U8)block_get;
        }

        m_fha_asa_block.write_time++;
    }while(m_fha_asa_block.write_time <= (ASA_BLOCK_COUNT+1) && block_try < ASA_MAX_BLOCK_TRY);

    //free buffer
    gFHAf.RamFree(buf_badblock);
    //free buf_other
    gFHAf.RamFree(buf_other);
    //free dummy_buf
    gFHAf.RamFree(dummy_buf);

    /*
    设计asa 的end block
    */
    fha_set_asa_end_block(m_fha_asa_block.asa_block_cnt);

    return (block_try > ASA_MAX_BLOCK_TRY) ? AK_FALSE : AK_TRUE;    
}

/**
 * @BREIF    FHA_asa_format
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_U32  FHA_asa_format(T_U32 type)
{
    if (asa_format(type))
        return FHA_SUCCESS;
    else
        return FHA_FAIL;
}


/************************************************************************
 * NAME:     FHA_check_Nand_isused
 * FUNCTION  正常的扫描，判断此nand是否使用过
 * PARAM:    [in] block -- need to check block item
 * RETURN:   is use return FHA_SUCCESS, or else retuen FHA_ FAIL
**************************************************************************/
T_BOOL  FHA_check_Nand_isused(T_U8 *pBuf, T_U32 len)
{
    
    T_U32 i, j;
    T_U32 k;
    T_U32 bad_block_cnt = 0;

    asa_global_init();
    
    //gFHAf.Printf("m_fha_asa_param.PagePerBlock: %d\r\n", m_fha_asa_param.PagePerBlock);

    //只读第0而,就可以判断是否是用过.
    for(i = 0; i < 1; i++)
    {
        k = 0;
        //check if block 0 is null or not
        gFHAf.MemSet(pBuf, 0x0, len);
        //Prod_NandReadASAPage(0, i, pBuf, (T_U8 *)&spare);
        //gFHAf.Printf("i: %d, %d\r\n",i, m_fha_asa_param.BytesPerPage);
        gFHAf.ReadNandBytes(0, 0 + i, 0,  pBuf, m_fha_asa_param.BytesPerPage);
       
        for(j = 0; j < m_fha_asa_param.BytesPerPage; j++)
        {
            //gFHAf.Printf("pBuf[%d]: %x\r\n", j, pBuf[j]);
            
            if(pBuf[j] != 0xFF)
            {
               k += count_bit0(pBuf[j]);
                
            }

            if(k > 8)
            {
                //gFHAf.Printf("k: %d\r\n", k);
                return FHA_SUCCESS;
            }
        }
    }
    //gFHAf.Printf("fail: %d\r\n", k);
    return FHA_FAIL;
    
}


/************************************************************************
 * NAME:     FHA_Get_factory_badblock_Buf
 * FUNCTION  Get_factory_badblock_Buf
 * PARAM:    [in] block -- need to check block item
 * RETURN:   is bad block return FHA_SUCCESS, or else retuen FHA_ FAIL
**************************************************************************/

T_BOOL  FHA_Get_factory_badblock_Buf(T_U8 *buf, T_U32 babblock_pagecnt)
{
    T_U32 block_get = 0;
    T_U32 block_try = 0;
    T_U32 block = 0;
    T_U32 asa_badblock_num = 0;  //块坏buf所占用的块数
    T_U32 i = 0, j = 0;
    T_U32 spare ;
    

    //查出安全区的第一个块    
    while(block_get < ASA_BLOCK_COUNT && block_try < ASA_MAX_BLOCK_TRY)
    {
        T_ASA_HEAD *pHead = (T_ASA_HEAD *)buf;

        block_try++;
        block++;
        //gFHAf.Printf("block:%d, block_try:%d \r\n", block, block_try);
        //read page 0，asa flag
        if(!Prod_NandReadASAPage(block, 0, buf, (T_U8*)&spare))
        {
            gFHAf.Printf("asa_init read block:%d flag fail\r\n", block);
            continue;
        }

        //check head
        if(gFHAf.MemCmp(pHead->head_str, m_hinfo_data, HEAD_SIZE) != 0)
        {
            gFHAf.Printf("asa_init block:%d is not asa block\r\n", block);
            continue;
        }

        gFHAf.MemSet(buf, 0, babblock_pagecnt*m_fha_asa_param.BytesPerPage);
        
        gFHAf.Printf("block:%d, %d\r\n", block, babblock_pagecnt);
        for(j = 1; j <= babblock_pagecnt; j++)
        {
            //gFHAf.Printf("page: %d\r\n", j);
            if(!Prod_NandReadASAPage(block, j, buf, (T_U8*)&spare))
            {
                gFHAf.Printf("asa_init read times at block:%d fail\r\n", block);
                return FHA_FAIL;
            }
        }
        
        //读完出厂坏块表后直接退出
        return FHA_SUCCESS;

    }

    return FHA_FAIL;
}


/************************************************************************
 * NAME:     FHA_Nand_check_block
 * FUNCTION  FHA_Nand_check_block
 * PARAM:    [in] chip -- need to check block item
 *                [in] block -- block num
 * RETURN:   is bad block return FHA_SUCCESS, or else retuen FHA_ FAIL
**************************************************************************/
T_BOOL FHA_Nand_check_block(T_U32 chip, T_U32 block)
{
    if(check_block(chip, block))
    {
        return FHA_SUCCESS;
    }
    return FHA_FAIL;
}





