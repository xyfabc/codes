/**
 * @FILENAME: asa.c
 * @BRIEF xx
 * Copyright (C) 2007 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR xx
 * @DATE 2009-11-19
 * @VERSION 1.0
 * @REF
 */

#include "nand_info.h"
#include "fha_asa.h"
#include "asa_format.h"

static T_VOID asa_update_page_data(T_U32 page, T_U8 data[], T_U32 bad_blocks[], T_U32 bb_count);
static T_BOOL asa_read_page(T_U32 page, T_U8 data[], T_U32 count);
static T_VOID asa_init_repair(T_U32 block_end, T_U8* pBuf);
static T_BOOL asa_create_repair(T_U32 block_end, T_U8* pBuf);
static T_BOOL asa_get_bad_block(T_U32 start_block, T_U8 pData[], T_U32 length);

T_U8 m_hinfo_data[HEAD_SIZE] = {0x41, 0x4E, 0x59, 0x4B, 0x41, 0x41, 0x53, 0x41};
T_ASA_PARAM m_fha_asa_param = {0};
T_ASA_BLOCK m_fha_asa_block = {0}; 

T_U8   *m_pBuf_BadBlk = AK_NULL;
static T_U8 m_buf_stat = 0;

/**
 * @BREIF    asa_global_init
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_VOID asa_global_init()
{
    m_fha_asa_param.BlockNum     = g_nand_phy_info.blk_num;
    m_fha_asa_param.BytesPerPage = g_nand_phy_info.page_size;
    m_fha_asa_param.PagePerBlock = g_nand_phy_info.page_per_blk;

    if (PLAT_SPOT == g_burn_param.ePlatform)
    {
        if(512 == g_nand_phy_info.page_size)
        {
            m_fha_asa_param.BlockNum     = g_nand_phy_info.blk_num / 8;
            m_fha_asa_param.BytesPerPage = 2048;
            m_fha_asa_param.PagePerBlock = 64;
        }
        else
        {
            m_fha_asa_param.BytesPerPage = (g_nand_phy_info.page_size > 4096) ?  4096 : g_nand_phy_info.page_size;  
        } 
    }  
}    

/**
 * @BREIF    asa_BadBlBufInit
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL asa_BadBlBufInit(T_VOID)
{
    m_pBuf_BadBlk = (T_U8*)gFHAf.RamAlloc(((m_fha_asa_param.BlockNum * g_burn_param.nChipCnt) >> 3));
    if (AK_NULL != m_pBuf_BadBlk)
    {
        m_buf_stat = 1;
        return AK_TRUE;
    }
    gFHAf.Printf("asa alloc err: %d, %d, %d\n", m_fha_asa_param.BlockNum, g_burn_param.nChipCnt, (m_fha_asa_param.BlockNum * g_burn_param.nChipCnt) >> 3);
    return AK_FALSE;
}


/**
 * @BREIF   init security area
 * @AUTHOR  Liao Zhijun
 * @DATE     2009-11-23
 * @PARAM   T_ASA_INTERFACE: asa param struct
 * @RETURN   AK_TRUE, success, 
                    AK_FALSE, fail
 */
T_U32  FHA_asa_scan(T_BOOL  bMount)
{
    T_U32 wtime_primary;
    T_U8 *pBuf;
    T_U32 block, spare;
    T_U32 block_try, block_get;
    T_U32 block_end = 0;
    T_BOOL bLastInfo = AK_FALSE;
    T_U32 ret = FHA_SUCCESS;

    asa_global_init();

    gFHAf.Printf("asa_scan start\r\n");
    if (bMount)
    {
        if (!asa_BadBlBufInit())
        {
            gFHAf.Printf("asa_scan malloc fail\r\n");
            return FHA_FAIL;
        }
    }

    //alloc memory
    pBuf = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if(AK_NULL == pBuf)
    {
        return FHA_FAIL;
    }

    wtime_primary = 0;
    m_fha_asa_block.asa_head = 0;

    block_try = block_get = 0;
    block = 0;
    gFHAf.MemSet(m_fha_asa_block.asa_blocks, 0, ASA_BLOCK_COUNT);
    
    //scan
    while(block_get < ASA_BLOCK_COUNT && block_try < ASA_MAX_BLOCK_TRY)
    {
        T_ASA_HEAD *pHead = (T_ASA_HEAD *)pBuf;

        block_try++;
        m_fha_asa_block.asa_block_cnt = (T_U8)block_try + 1;
        block++;
        
        //read page 0，asa flag
        if(!Prod_NandReadASAPage(block, 0, pBuf, (T_U8*)&spare))
        {
            gFHAf.Printf("asa_init read block:%d flag fail\r\n", block);
            continue;
        }

        //check head
        if(gFHAf.MemCmp(pHead->head_str, m_hinfo_data, HEAD_SIZE) != 0)
        {
            //gFHAf.Printf("asa_init block:%d is not asa block\r\n", block);
            continue;
        }

        //read time
        if(!Prod_NandReadASAPage(block, m_fha_asa_param.PagePerBlock - 1, pBuf, (T_U8*)&spare))
        {
            gFHAf.Printf("asa_init read times at block:%d fail\r\n", block);
            continue;
        }

        //spare为全0xFF,有可能是擦除以后没有写数据的块,这个块不能使用
        if(0xFFFFFFFF == spare)
        {
            gFHAf.Printf("asa_init times to max at block:%d\r\n", block);
            continue;
        }

        block_end = block;
        if(0 == spare)
        {
            //spare等于0是安全区结束块标识
            gFHAf.Printf("asa_init times is zero at block:%d\r\n", block);
            m_fha_asa_block.asa_block_cnt--;

            bLastInfo = AK_TRUE;
            break;
        }

        //update spare
        if(spare > wtime_primary)
        {
            //数据最新的块
            wtime_primary = spare;
            m_fha_asa_block.asa_head = (T_U8)block_get;
        }
        
        m_fha_asa_block.asa_blocks[block_get++] = (T_U8)block;
        m_fha_asa_block.asa_count = (T_U8)block_get;

        gFHAf.Printf("asa_init block:%d, times:%d\r\n", block, spare);
    }

    //ASA_MAX_BLOCK_TRY个块用完了
    if(ASA_MAX_BLOCK_TRY == block_try)
    {
        m_fha_asa_block.asa_block_cnt = ASA_MAX_BLOCK_TRY;
    }
    
    if(0 == m_fha_asa_block.asa_blocks[m_fha_asa_block.asa_head])
    {
        gFHAf.Printf("asa_init cannot find a fit block\r\n");
        if (bLastInfo)
        {
            //重新构建安全区
            if (!asa_create_repair(block_end, pBuf))
            {
                ret = FHA_FAIL;
            }
            else
            {
                ret = FHA_SUCCESS;
            }    
        }
        else
        {
            ret = FHA_FAIL;
        }    
    }
    else
    {

        m_fha_asa_block.write_time = wtime_primary+1;

        gFHAf.Printf("asa_init primary block:%d, times:%d\r\n", m_fha_asa_block.asa_blocks[m_fha_asa_block.asa_head], m_fha_asa_block.write_time);

        //if asa blocks is not enough, fill with non-badblock
        if(m_fha_asa_block.asa_count < ASA_BLOCK_COUNT)
        {
            asa_init_repair(block_end, pBuf);
        }
    }

    gFHAf.RamFree(pBuf);

    fha_set_asa_end_block((T_U32)m_fha_asa_block.asa_block_cnt);
    
    if (FHA_SUCCESS == ret)
    {
        T_U32 m;

        for (m=0; m<m_fha_asa_param.BlockNum * g_burn_param.nChipCnt; m++)
        {
            if (FHA_check_bad_block(m))
            {
                gFHAf.Printf("BB:%d ", m);
            }    
        } 

        gFHAf.Printf("\r\n");
    }    

    return ret;
}

/**
 * @BREIF    asa_create_repair
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_BOOL asa_create_repair(T_U32 block_end, T_U8* pBuf)
{
    T_U32 i ; 
//	T_U32 j ;
    T_U32 spare;
    T_U8 badblock[ASA_MAX_BLOCK_TRY];

    m_fha_asa_block.write_time = 1;
    m_fha_asa_block.asa_count = 0;

    //读最后一个块(spare为0)第1页）获取出厂坏块信息
    if (!Prod_NandReadASAPage(block_end, 1, pBuf, (T_U8 *)&spare))
    {
        return AK_FALSE;
    }

    gFHAf.MemCpy(badblock, pBuf, ASA_MAX_BLOCK_TRY);

    //开始重建安全区
    for(i = 1; (i < block_end) && (m_fha_asa_block.asa_count < ASA_BLOCK_COUNT); i++)
    {
        T_U32 j;

        //如果安全区有出厂坏块跳过
        if ((badblock[i/8] & (1 << (7 - i%8))) != 0)
        {
            continue;
        }

        if (!Prod_NandEraseBlock(i))
        {
            continue;
        }

        //从最后块开始搬移数据到安全区的块
        for (j=0; j<m_fha_asa_param.PagePerBlock; j++)
        {
            if (!Prod_NandReadASAPage(block_end, j, pBuf, (T_U8 *)&spare))
            {
                return AK_FALSE;
            }  
            else
            {
                if (!Prod_NandWriteASAPage(i, j, pBuf, (T_U8 *)&m_fha_asa_block.write_time))
                {
                    break;
                }    
            }    
        }

        if (j < m_fha_asa_param.PagePerBlock)
        {
            Prod_NandEraseBlock(i);
            continue;
        }
        else
        {
            gFHAf.Printf("repair asa blk:%d, times:%d\r\n", i, m_fha_asa_block.write_time);
            m_fha_asa_block.asa_head = m_fha_asa_block.asa_count;            
            m_fha_asa_block.asa_blocks[m_fha_asa_block.asa_count++] = (T_U8)i;
            m_fha_asa_block.write_time++;
        }    
    }

    return (m_fha_asa_block.asa_count > 0) ? AK_TRUE : AK_FALSE;
} 

/**
 * @BREIF    asa_init_repair
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_VOID asa_init_repair(T_U32 block_end, T_U8* pBuf)
{
    T_U8 bad_blocks[ASA_MAX_BLOCK_TRY];
    T_U32 i, j;
    T_U32 spare = 1;

    //get bad blocks
    if(asa_get_bad_block(0, bad_blocks, ASA_MAX_BLOCK_TRY))
    {
        for(i = 1; (i < block_end) && (m_fha_asa_block.asa_count < ASA_BLOCK_COUNT); i++)
        {
            for(j = 0; j < m_fha_asa_block.asa_count; j++)
            {
                if(m_fha_asa_block.asa_blocks[j] == i)
                {
                    break;
                }
            }
            if(j < m_fha_asa_block.asa_count)
            {
                continue;
            }
            
            if((bad_blocks[i/8] & (1<<(7-(i%8)))) == 0)
            {
               // T_U32 j;
                if (!Prod_NandEraseBlock(i))
                {
                    continue;
                }

                for (j=0; j<m_fha_asa_param.PagePerBlock; j++)
                {
                    if (!asa_read_page(j, pBuf, m_fha_asa_block.asa_count))
                    {    
                        break;
                    }
                    else
                    {
                        if (!Prod_NandWriteASAPage(i, j, pBuf, (T_U8 *)&spare))
                        {
                            break;
                        }    
                    }    
                }

                if (j < m_fha_asa_param.PagePerBlock)
                {
                    Prod_NandEraseBlock(i);
                    continue;
                }
                else
                {
                    gFHAf.Printf("repair:%d\n", i);
                    m_fha_asa_block.asa_blocks[m_fha_asa_block.asa_count++] = (T_U8)i;
                }    
            }
        }
    }
}    

/*
从数据最新的块(在m_asa_blocks中索引为m_asa_head)开始，依次尝试从count个块中去读取数据
*/
 /**
     * @BREIF    asa_read_page
     * @AUTHOR   luqiliu
     * @DATE     2009-12-10
     * @RETURN   T_U32 
     * @retval   FHA_SUCCESS :  succeed
     * @retval   FHA_FAIL :     fail
   */

static T_BOOL asa_read_page(T_U32 page, T_U8 data[], T_U32 count)
{
    T_BOOL ret = AK_FALSE;
    T_U32 spare;
    T_U32 i;
    T_U32 read_index = m_fha_asa_block.asa_head;
    
    //check parameter
    if(page >= m_fha_asa_param.PagePerBlock || m_fha_asa_block.asa_head>= ASA_BLOCK_COUNT || AK_NULL == data)
    {
        return AK_FALSE;
    }

    //read data
    for (i=0; i<count; i++)
    {
        //若安全区块读写擦有失败时，m_asa_blocks中对应的数据将赋值为0
        if (0 != m_fha_asa_block.asa_blocks[read_index])
        {
            ret = Prod_NandReadASAPage(m_fha_asa_block.asa_blocks[read_index], page, data, (T_U8*)&spare);
        }
        
        if (ret)
        {
            break;
        }

        //索引为循环列表递减
        read_index = (read_index > 0) ? (read_index - 1) : (m_fha_asa_block.asa_count - 1);
    }    
    
    return ret;
}

//update 
/**
 * @BREIF    asa_update_page_data
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

static T_VOID asa_update_page_data(T_U32 page, T_U8 data[], T_U32 bad_blocks[], T_U32 bb_count)
{
    T_U32 page_count;
    T_U32 page_index, page_offset;
    T_U32 byte_loc, byte_offset;
    T_U8 i;
    
    page_count = (m_fha_asa_param.BlockNum * g_burn_param.nChipCnt - 1) / (m_fha_asa_param.BytesPerPage * 8) + 1;

    for(i = 0; i < bb_count; i++)
    {
        page_index = bad_blocks[i] / (m_fha_asa_param.BytesPerPage * 8);
        page_offset = bad_blocks[i] - page_index * (m_fha_asa_param.BytesPerPage * 8);

        if(page != 1 + page_count + page_index)//the previous pages(1 ~ page_count ) stores the bitmap of initial bad block
        {
            continue;
        }

        byte_loc = page_offset / 8;
        byte_offset = 7 - page_offset % 8;
        
        data[byte_loc] |= (1 << byte_offset);
    }
}

/**
 * @BREIF    get  bad block information of one or more blocks
 * @AUTHOR  Liao Zhijun
 * @DATE     2009-11-23
 * @PARAM   start_block: start block
                  pData[IN]: buffer used to store bad blocks information data
                  length: how many blocks u want to know
 * @RETURN   AK_TRUE, success, 
                    AK_FALSE, fail
 */
T_BOOL asa_get_bad_block(T_U32 start_block, T_U8 pData[], T_U32 length)
{
    T_U32 page_index, page_offset;
    T_U32 byte_loc, byte_offset;
    T_U32 index_old = 0xFFFFFFFF;
    T_U8 *pBuf = AK_NULL;
    T_U32 page_count;
    T_U32 i;
    
    //check parameter
    if(m_fha_asa_block.asa_head>= ASA_BLOCK_COUNT || AK_NULL == pData)
    {
        return AK_FALSE;
    }

    page_count = (m_fha_asa_param.BlockNum * g_burn_param.nChipCnt - 1) / (m_fha_asa_param.BytesPerPage * 8) + 1;
    //alloc memory
    pBuf = (T_U8 *)gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if(AK_NULL == pBuf)
    {
        return AK_FALSE;
    }

    //get all bad blocks
    for(i = 0; i < length; i++)
    {
        page_index = (start_block + i) / (m_fha_asa_param.BytesPerPage * 8);
        page_offset = (start_block + i) - page_index * (m_fha_asa_param.BytesPerPage * 8);
        
        byte_loc = page_offset / 8;
        byte_offset = 7 - page_offset % 8;

        //read page data if necessary
        if(index_old != page_index)
        {
            index_old = page_index;
            //尝试从安全区中所有块去读取信息
            if(!asa_read_page(1+page_count+page_index, pBuf, m_fha_asa_block.asa_count))
            {
                gFHAf.RamFree(pBuf);
                return AK_FALSE;
            }
        }

        //update bad block buffer
        if(pBuf[byte_loc] & (1 << byte_offset))
        {
            pData[i/8] |= 1<<(7-i%8);
        }
        else
        {
            pData[i/8] &= ~(1<<(7-i%8));
        }

    }

    gFHAf.RamFree(pBuf);
    return AK_TRUE;
}

/**
 * @BREIF    asa_restore_bad_blocks
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL asa_restore_bad_blocks()
{
    T_U32 block_next = 0;
    T_U32 block_written = 0;
    T_U32 i, j;
    T_U32 bad_blocks[ASA_BLOCK_COUNT] = {0};
    T_U8 bb_count = 0;
    T_U32 badblock_page_count = 0;
    T_U8 *buf_badblock = AK_NULL;
    T_U8 *pTmp = AK_NULL;

    badblock_page_count = (m_fha_asa_param.BlockNum  * g_burn_param.nChipCnt - 1) / (m_fha_asa_param.BytesPerPage * 8) + 1;

    //alloc memory
    buf_badblock = gFHAf.RamAlloc(badblock_page_count * m_fha_asa_param.BytesPerPage);
    if(AK_NULL == buf_badblock)
    {
        return AK_FALSE;
    }
    
    block_next = m_fha_asa_block.asa_head;
    
    for(j= 0; j < m_fha_asa_block.asa_count; j++)
    {
        T_U32 ret;
        T_U32 spare;
        
        spare = m_fha_asa_block.write_time;

        //get next block
        block_next = (block_next+1)%m_fha_asa_block.asa_count;
        if(0 == m_fha_asa_block.asa_blocks[block_next])
        {
            continue;
        }

        //避免擦除了安全区所有块
        if(block_next == m_fha_asa_block.asa_head)
        {
            return AK_FALSE;
        }

        //erase block
        if(!Prod_NandEraseBlock(m_fha_asa_block.asa_blocks[block_next]))
        {
            goto WRITE_FAIL;
        }
        
        //move page
        for(i = 0; i < m_fha_asa_param.PagePerBlock; i++)
        {
            if((0 == i) || (i > badblock_page_count * 2))
            {
                pTmp = buf_badblock;

                //除了刚被擦除的块，从所有块中去读取数据
                if(!asa_read_page(i, pTmp, m_fha_asa_block.asa_count-1))
                {
                    gFHAf.RamFree(buf_badblock);
                    return AK_FALSE;
                }
            }
            else if((i > 0) && (i < badblock_page_count + 1))
            {   
                pTmp = buf_badblock + (i - 1) * m_fha_asa_param.BytesPerPage;
                if(!asa_read_page(i, pTmp, m_fha_asa_block.asa_count-1))
                {
                    gFHAf.RamFree(buf_badblock);
                    return AK_FALSE;
                }
            }
            else
            {
                pTmp = buf_badblock + (i - badblock_page_count - 1) * m_fha_asa_param.BytesPerPage;
                asa_update_page_data(i, pTmp, bad_blocks, bb_count);
            }

            ret = Prod_NandWriteASAPage(m_fha_asa_block.asa_blocks[block_next], i, pTmp, (T_U8*)&spare);
            if(!ret)
            {
                goto WRITE_FAIL;
            }
        }

        //update global variable
        m_fha_asa_block.asa_head = (T_U8)block_next;
        
        m_fha_asa_block.write_time++;
        block_written++;

        continue;

WRITE_FAIL:
        Prod_NandEraseBlock(m_fha_asa_block.asa_blocks[block_next]);
        bad_blocks[bb_count++] = m_fha_asa_block.asa_blocks[block_next];
        //若操写失败，则把m_asa_blocks中对应块置为0，避免再去读
        m_fha_asa_block.asa_blocks[block_next] = 0;

        if (bb_count > 1)
        {
            gFHAf.Printf("asar@#@");
            gFHAf.RamFree(buf_badblock);
            return AK_FALSE;
        }
    }

    gFHAf.RamFree(buf_badblock);
    return (block_written > 0) ? AK_TRUE : AK_FALSE;
}
/**
 * @BREIF    set a block to bad block
 * @AUTHOR  Liao Zhijun
 * @DATE     2009-11-23
 * @PARAM   block: which block u want to set
 * @RETURN   AK_TRUE, success, 
                    AK_FALSE, fail
 */
T_BOOL asa_set_bad_block(T_U32 block)
{
    T_U8 *pBuf = AK_NULL;
    T_U32 block_next = 0;
    T_U32 block_written = 0;
    T_U32 i, j;
    T_U32 bad_blocks[ASA_BLOCK_COUNT] = {0};
    T_U8 bb_count = 0;

    //check parameter
    if(m_fha_asa_block.asa_head >= ASA_BLOCK_COUNT)
    {
        return AK_FALSE;
    }

    //alloc memory
    pBuf = (T_U8 *)gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if(AK_NULL == pBuf)
    {
        return AK_FALSE;
    }
 
    bad_blocks[0] = block;
    bb_count = 1;
    
    block_next = m_fha_asa_block.asa_head;
    for(j = 0; (j < m_fha_asa_block.asa_count) && (block_written < 2); j++)
    {
        T_U32 ret;
        T_U32 spare;

        spare = m_fha_asa_block.write_time;

        //get next block
        block_next = (block_next+1) % m_fha_asa_block.asa_count;
        if(0 == m_fha_asa_block.asa_blocks[block_next])
        {
            continue;
        }

        //避免擦除了安全区所有块
        if(block_next == m_fha_asa_block.asa_head)
        {
            gFHAf.RamFree(pBuf);
            gFHAf.Printf("FHA: set bad blk err, asa get only one blk.\r\n");
            
            return AK_FALSE;
        }

         //erase block
        if(!Prod_NandEraseBlock(m_fha_asa_block.asa_blocks[block_next]))
        {
            goto WRITE_FAIL;
        }
        
        //move pages before the page
        for(i = 0; i < m_fha_asa_param.PagePerBlock; i++)
        {
            //除了刚被擦除的块，从所有块中去读取数据
            if(!asa_read_page(i, pBuf, m_fha_asa_block.asa_count-1))
            {
                gFHAf.RamFree(pBuf);
                return AK_FALSE;
            }

            asa_update_page_data(i, pBuf, bad_blocks, bb_count);
            ret = Prod_NandWriteASAPage(m_fha_asa_block.asa_blocks[block_next], i, pBuf, (T_U8*)&spare);
            if(!ret)
            {
                goto WRITE_FAIL;
            }
        }

        //update global variable
        m_fha_asa_block.asa_head = (T_U8)block_next;
        
        m_fha_asa_block.write_time++;
        block_written++;

        continue;

WRITE_FAIL:
        Prod_NandEraseBlock(m_fha_asa_block.asa_blocks[block_next]);
        bad_blocks[bb_count++] = m_fha_asa_block.asa_blocks[block_next];
        m_fha_asa_block.asa_blocks[block_next] = 0;

        if (bb_count > 2)
        {
            gFHAf.Printf("asar@#@");
            gFHAf.RamFree(pBuf);
            while(1);
        }
    }
    
    gFHAf.RamFree(pBuf);
    
    return (block_written > 0) ? AK_TRUE : AK_FALSE;
}

/**
 * @BREIF    write_block
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL write_block(T_ASA_HEAD* pHead, T_ASA_FILE_INFO *pFileInfo, T_U32 fileInfoOffset, const T_U8* dataBuf)
{
    T_U32 block_next = 0;
    T_U32 block_written = 0;
    T_U32 i, j;
    T_U32 bad_blocks[ASA_BLOCK_COUNT] = {0};
    T_U8 bb_count = 0;
    T_U8* pTmp = AK_NULL;
    T_U8* pTmpBuf = AK_NULL;
            
    pTmpBuf = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if( AK_NULL == pTmpBuf)
    {
        return AK_FALSE;
    }
    
    block_next = m_fha_asa_block.asa_head;
    
    for(j= 0; j < m_fha_asa_block.asa_count; j++)
    {
        T_U32 ret;
        T_U32 spare;
        
        spare = m_fha_asa_block.write_time;

        //get next block
        block_next = (block_next+1) % m_fha_asa_block.asa_count;
        if(0 == m_fha_asa_block.asa_blocks[block_next])
        {
            continue;
        }

        //避免擦除了安全区所有块
        if(block_next == m_fha_asa_block.asa_head)
        {
            return AK_FALSE;
        }

        //erase block
        if(!Prod_NandEraseBlock(m_fha_asa_block.asa_blocks[block_next]))
        {
            goto WRITE_FAIL;
        }
        
        //move page
        for(i = 0; i < m_fha_asa_param.PagePerBlock; i++)
        {
            //没有写过的页，不需要进行读写
            if(i > pHead->info_end && i < m_fha_asa_param.PagePerBlock - 1)
            {
                continue;
            }

            if(i<pFileInfo->start_page|| i>=pFileInfo->end_page)
            {
                //写安全区文件只尝试两个块的操作，防止破坏安全区
                if(!asa_read_page(i, pTmpBuf, 2))
                {
                    gFHAf.RamFree(pTmpBuf);
                    return AK_FALSE;
                }

                
                if(0 == i)
                {
                    gFHAf.MemCpy(pTmpBuf, pHead, sizeof(T_ASA_HEAD));
                    gFHAf.MemCpy(pTmpBuf + fileInfoOffset, pFileInfo, sizeof(T_ASA_FILE_INFO));
                }
                
                asa_update_page_data(i, pTmpBuf, bad_blocks, bb_count);

                pTmp = pTmpBuf;
            }
            else
            {
                pTmp = (T_U8*)(dataBuf + (i - pFileInfo->start_page) * m_fha_asa_param.BytesPerPage);
            }

            ret = Prod_NandWriteASAPage(m_fha_asa_block.asa_blocks[block_next], i, pTmp, (T_U8*)&spare);
            if(!ret)
            {
                goto WRITE_FAIL;
            }
        }

        //update global variable
        m_fha_asa_block.asa_head = (T_U8)block_next;
        
        m_fha_asa_block.write_time++;
        block_written++;

        continue;

WRITE_FAIL:
        Prod_NandEraseBlock(m_fha_asa_block.asa_blocks[block_next]);
        bad_blocks[bb_count++] = m_fha_asa_block.asa_blocks[block_next];
        //若操写失败，则把m_asa_blocks中对应块置为0，避免再去读
        m_fha_asa_block.asa_blocks[block_next] = 0;

        if (bb_count > 1)
        {
            gFHAf.Printf("asar@#@");
            gFHAf.RamFree(pTmpBuf);
            return AK_FALSE;
        }
    }

    gFHAf.RamFree(pTmpBuf);
    return (block_written > 0) ? AK_TRUE : AK_FALSE;

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

T_U32 asa_write_file(T_U8 file_name[], const T_U8 dataBuf[], T_U32 dataLen, T_U8 mode)
{
    T_U8 *pBuf = AK_NULL;
    T_ASA_HEAD HeadInfo;
    T_ASA_FILE_INFO *pFileInfo = AK_NULL;
    T_ASA_FILE_INFO newFileInfo;
    T_U32 file_info_offset;
    T_U32 i,j;
    T_U32 file_num;
    T_BOOL bMatch = AK_FALSE; //must false
    T_U16 page_cnt = (T_U16)(dataLen-1)/m_fha_asa_param.BytesPerPage + 1;
    
    if(AK_NULL == file_name || AK_NULL == dataBuf || m_fha_asa_block.asa_head >= ASA_BLOCK_COUNT)
    {
        return ASA_FILE_FAIL;
    }

    if (fha_str_len(file_name) > sizeof(pFileInfo->file_name) -1)
    {
        gFHAf.Printf("asa file name overlong!\r\n");        
        return ASA_FILE_FAIL;
    }
    
    pBuf = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if( AK_NULL == pBuf)
    {
        return ASA_FILE_FAIL;
    }

    //read page0 to get file info
    //安全区文件最初始时只写两个块，也只尝试去读两个块
    if(!asa_read_page(0, pBuf, 2))
    {
        goto EXIT;
    }

    //head info
    gFHAf.MemCpy(&HeadInfo, pBuf, sizeof(T_ASA_HEAD));

    file_info_offset = sizeof(T_ASA_HEAD) + 2 * sizeof(T_ASA_ITEM);

    //asa file info
    pFileInfo = (T_ASA_FILE_INFO *)(pBuf + file_info_offset);

    file_num = HeadInfo.item_num - 2;

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
            file_info_offset += sizeof(T_ASA_FILE_INFO);
        }
    }

    if(!bMatch)
    {
        //new asa file
        pFileInfo->start_page = HeadInfo.info_end;
        pFileInfo->end_page = pFileInfo->start_page + page_cnt;
        pFileInfo->file_length = dataLen;
        gFHAf.MemCpy(pFileInfo->file_name, file_name, 8);

        //whether excess block size
        if(pFileInfo->end_page > m_fha_asa_param.PagePerBlock)
        {
            goto EXIT;
        }

        //chenge head info if new file
        HeadInfo.item_num++;
        HeadInfo.info_end = pFileInfo->end_page;
    }

    gFHAf.MemCpy(&newFileInfo, pFileInfo, sizeof(T_ASA_FILE_INFO));

    gFHAf.RamFree(pBuf);

    if(!write_block(&HeadInfo, &newFileInfo, file_info_offset, dataBuf))
    {
        return ASA_FILE_FAIL;
    }
    else
    {
        return ASA_FILE_SUCCESS;
    }
        
EXIT:
    gFHAf.RamFree(pBuf);
    return ASA_FILE_FAIL;
}

T_U32 asa_delete_all_file()
{
    T_U8 *pBuf = AK_NULL;
    T_ASA_HEAD HeadInfo;
    T_ASA_FILE_INFO FileInfo;
    T_U32 file_info_offset;
    T_U32 block_count;
    T_U32 badblock_page_count;
    T_U32 ret= ASA_FILE_FAIL;

    block_count = g_burn_param.nChipCnt * m_fha_asa_param.BlockNum;
    badblock_page_count = (block_count - 1) / (m_fha_asa_param.BytesPerPage * 8) + 1;
    
    if(m_fha_asa_block.asa_head >= ASA_BLOCK_COUNT)
    {
        return ASA_FILE_FAIL;
    }

    pBuf = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if( AK_NULL == pBuf)
    {
        return ASA_FILE_FAIL;
    }

    //read page0 to get head info
    //安全区文件最初始时只写两个块，也只尝试去读两个块
    if(!asa_read_page(0, pBuf, 2))
    {
        ret = ASA_FILE_FAIL;
        goto EXIT;
    }

    //head info
    gFHAf.MemCpy(&HeadInfo, pBuf, sizeof(T_ASA_HEAD));
    HeadInfo.item_num = 2;
    HeadInfo.info_end = 1 + badblock_page_count*2;

    FileInfo.start_page = HeadInfo.info_end;
    FileInfo.end_page = HeadInfo.info_end;

    file_info_offset = sizeof(T_ASA_HEAD) + 2 * sizeof(T_ASA_ITEM);

    gFHAf.RamFree(pBuf);
    if(!write_block(&HeadInfo, &FileInfo, file_info_offset, AK_NULL))
    {
        goto EXIT;
    }

    return ASA_FILE_SUCCESS;
        
EXIT:
    gFHAf.RamFree(pBuf);
    return ret;

}


/************************************************************************
 * NAME:     FHA_asa_read_file
 * FUNCTION  get infomation from security area by input file name
 * PARAM:    [in] file_name -- asa file name
 *           [out]pData ------ buffer used to store information data
 *           [in] data_len --- need to get data length
 * RETURN:   success return ASA_FILE_SUCCESS, fail retuen ASA_FILE_FAIL
**************************************************************************/

T_U32 asa_read_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen)
{
    T_U8 *pBuf = AK_NULL;
    T_ASA_HEAD *pHead = AK_NULL;
    T_ASA_FILE_INFO *pFileInfo = AK_NULL;
    T_U32 i,j;
    T_U32 file_num;
    T_BOOL bMatch = AK_FALSE; //must false
    T_U32 ret= ASA_FILE_FAIL;
    T_U32 start_page;
    T_U32 end_page;
    T_U16 page_cnt = (T_U16)(dataLen-1)/m_fha_asa_param.BytesPerPage + 1;

    if(AK_NULL == file_name || AK_NULL == dataBuf || m_fha_asa_block.asa_head >= ASA_BLOCK_COUNT)
    {
        return ASA_FILE_FAIL;
    }

    pBuf = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if( AK_NULL == pBuf)
    {
        return ASA_FILE_FAIL;
    }

    //安全区文件最初始时只写两个块，也只尝试去读两个块
    //read page0 to get file info
    if(!asa_read_page(0, pBuf, 2))
    {
        goto EXIT;
    }

    //head info
    pHead = (T_ASA_HEAD *)pBuf;

    //asa file info
    pFileInfo = (T_ASA_FILE_INFO *)(pBuf + sizeof(T_ASA_HEAD) + 2 * sizeof(T_ASA_ITEM));

    file_num = pHead->item_num - 2;

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
        start_page = pFileInfo->start_page;
        end_page = pFileInfo->end_page;

        if(page_cnt > (end_page - start_page))
        {
            goto EXIT;
        }
        
        i=0;
        for(; i<dataLen/m_fha_asa_param.BytesPerPage; i++)
        {
            //安全区文件最初始时只写两个块，也只尝试去读两个块
            if(!asa_read_page(start_page+i, dataBuf+i*m_fha_asa_param.BytesPerPage, 2))
            {
                goto EXIT;
            }    
        }

        if(0 != dataLen%m_fha_asa_param.BytesPerPage)
        {
            if(!asa_read_page(start_page+i, pBuf, 2))
            {
                goto EXIT;
            }
            else
            {
                gFHAf.MemCpy(dataBuf+i*m_fha_asa_param.BytesPerPage, pBuf, dataLen%m_fha_asa_param.BytesPerPage);
            }
        }    
    }

    ret= ASA_FILE_SUCCESS;
            
EXIT:
    gFHAf.RamFree(pBuf);
    return ret;
}

/*
//判断安全区每个页可以读成功的块的个数
T_U32 asa_get_good_block_count()
{
    T_U32 i,j;
    T_U32 block_good_count = m_asa_count;
    T_U8* pBuf = AK_NULL;
    T_U32 spare;

    //alloc memory
    pBuf = gFHAf.RamAlloc(ASA_BUFFER_SIZE);
    if(AK_NULL == pBuf)
    {
       return 0xFF;
    }

    for (i=0; i<m_asa_count; i++)
    {
        for (j=0; j<m_asafun.PagePerBlock; j++)
        {
            if (!Prod_NandWriteASAPage(0, m_asa_blocks[i], j, pBuf, &spare, 4))
            {
                block_good_count--;
                break;
            }    
        }    
    }

    gFHAf.RamFree(pBuf);

    return block_good_count;
}
*/

//**********************************************************************************
T_U32  FHA_set_bad_block(T_U32 block)
{
    T_U32 byte_loc, byte_offset;
    T_BOOL ret;
    
    ret = asa_set_bad_block(block);

    if(AK_NULL != m_pBuf_BadBlk)
    {
        byte_loc = block / 8;
        byte_offset = 7 - block % 8;
        m_pBuf_BadBlk[byte_loc] |= 1 << byte_offset;
    }

    return ret;
}

/************************************************************************
 * NAME:     FHA_check_bad_block
 * FUNCTION  check nand flash bad block
 * PARAM:    [in] block -- need to check block item
 * RETURN:   is bad block return FHA_SUCCESS, or else retuen FHA_ FAIL
**************************************************************************/

T_BOOL  FHA_check_bad_block(T_U32 block)
{
    T_U8  pData[4] = {0};

     if(1 == m_buf_stat && m_pBuf_BadBlk != AK_NULL)
    {
        asa_get_bad_block(0, m_pBuf_BadBlk, m_fha_asa_param.BlockNum * g_burn_param.nChipCnt);
        m_buf_stat = 2;
    }

    if(m_buf_stat > 1  && m_pBuf_BadBlk != AK_NULL)
    {
        T_U32 byte_loc, byte_offset;

        byte_loc = block / 8;
        byte_offset = 7 - block % 8;

        if(m_pBuf_BadBlk[byte_loc] & (1 << byte_offset))
        {
            return AK_TRUE;
        }
    }
    else
    {
        asa_get_bad_block(block, pData, 1);

        if ((pData[0] & (1 << 7)) != 0)
        {
            return AK_TRUE;
        }
    }

    return AK_FALSE;
}

/************************************************************************
 * NAME:     FHA_get_bad_block
 * FUNCTION  get bad block information of one or more blocks
 * PARAM:    [in] start_block -- start block
 *           [out]pData -------- buffer used to store bad blocks information data
 *           [in] blk_cnt ------ how many blocks you want to know
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  FHA_get_bad_block(T_U32 start_block, T_U8 *pData, T_U32 blk_cnt)
{
    if (asa_get_bad_block(start_block, pData, blk_cnt))
        return FHA_SUCCESS; 
    else
        return FHA_FAIL;
}

/************************************************************************
 * NAME:     FHA_set_lib_version
 * FUNCTION  set burn all lib version
 * PARAM:    [in] lib_info--all lib version struct pointer addr
 *           [in] lib_cnt---lib count
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_WriteNandLibVerion(T_LIB_VER_INFO *lib_info,  T_U32 lib_cnt)
{
    if (AK_NULL == lib_info)
    {
        return FHA_FAIL;
    }

    if (sizeof(T_LIB_VER_INFO) * lib_cnt > m_fha_asa_param.BytesPerPage)
    {
        gFHAf.Printf("FHA SET VER: cnt too large\r\n");
        return FHA_FAIL;
    }
    
    if (FHA_FAIL == asa_write_file("LIBVER", 
        (T_U8 *)lib_info,
        sizeof(T_LIB_VER_INFO) * lib_cnt,
        ASA_MODE_CREATE))
    {
        return FHA_FAIL;
    }
    
    return FHA_SUCCESS;
}

/************************************************************************
 * NAME:     FHA_get_lib_verison
 * FUNCTION  get burn lib version by input lib_info->lib_name
 * PARAM:    [in-out] lib_info--input lib_info->lib_name, output lib_info->lib_version
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_GetNandLibVerion(T_LIB_VER_INFO *lib_info)
{
    T_U32 pos, size;
    T_U8 *buf = AK_NULL;
    T_LIB_VER_INFO *info;

    if (AK_NULL == lib_info)
    {
        return FHA_FAIL;
    }

    size = (T_U32)m_fha_asa_param.BytesPerPage;
    buf = gFHAf.RamAlloc(size);
    if(AK_NULL == buf)
    {
        gFHAf.Printf("FHA GET VER: malloc fail\r\n");
        return FHA_FAIL;
    }

    if (FHA_FAIL == asa_read_file("LIBVER", buf, size))
    {
        gFHAf.Printf("FHA GET VER: read file fail\r\n");
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




