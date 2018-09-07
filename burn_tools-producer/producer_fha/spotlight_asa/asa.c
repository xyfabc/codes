/**
 * @FILENAME: remap.h
 * @BRIEF remap virtual memory to physical memory by mmu
 * Copyright (C) 2007 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR Justin.Zhao
 * @DATE 2008-1-9
 * @VERSION 1.0
 * @REF
 */

#include "anyka_types.h"
#include "nandflash.h"
#include "asa.h"
#include "asa_format.h"
#include "fha.h"
#include "nand_info.h"

#define ASA_BUFFER_SIZE_SPOT 4096

//static 
#pragma arm section zidata = "_bootbss_"
static T_U8 m_asa_blocks[ASA_BLOCK_COUNT] = {0};  //保存安全区块表的数组
static T_U32 m_write_time = 0;
static T_U32 m_PagePerBlock = 0;
static T_U32 m_BytesPerSector = 0;
static T_U32 m_Block_num = 0;

#pragma arm section zidata

static T_U8 m_asa_count = 0xFF; //初始化以后用于安全区的block个数，仅指安全区可用块
static T_U8 m_asa_head = 0xFF;  //安全区块数组中数据最新的块的索引

static T_U8 m_hinfo_data[HEAD_SIZE] = {0x41, 0x4E, 0x59, 0x4B, 0x41, 0x41, 0x53, 0x41};

static T_VOID asa_update_page_data(T_U32 page, T_U8 data[], T_U32 bad_blocks[], T_U32 bb_count);
static T_BOOL asa_read_page(T_U32 page, T_U8 data[], T_U32 count);

// extern 
//extern T_NANDFLASH nf_info;

/*********************************/
/**
 * @BREIF    remap_alloc
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_pVOID remap_alloc(T_U32 size);
/**
 * @BREIF    remap_free
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_pVOID remap_free(T_pVOID ptr);
/**
 * @BREIF    progmanage_set_protect
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_VOID progmanage_set_protect(T_BOOL protect);
/*************************************/

#pragma arm section code = "_asa_initphase_code_"
/**
 * @BREIF    asa_initial
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */
T_BOOL asa_initial(T_U32 PagePerBlock, T_U32 BytesPerSector, T_U32 Block_num)
{
    T_U32 wtime_primary;
    T_U8 *pBuf;
    T_U32 block, spare;
    T_U32 block_try, block_get;
    T_U32 block_end = 0;
    T_U32 i;
    T_BOOL bLastInfo = AK_FALSE;
    T_BOOL ret = AK_TRUE;

    m_PagePerBlock = PagePerBlock;
    m_BytesPerSector = BytesPerSector;
    m_Block_num = Block_num;
    
    //alloc memory
    pBuf = (T_U8 *)remap_alloc(ASA_BUFFER_SIZE_SPOT);
    if(AK_NULL == pBuf)
    {
        return AK_FALSE;
    }

    wtime_primary = 0;
    m_asa_head = 0;

    block_try = block_get = 0;
    block = 0;
  
    //scan
    while(block_get < ASA_BLOCK_COUNT && block_try < ASA_MAX_BLOCK_TRY)
    {
        T_ASA_HEAD *pHead = (T_ASA_HEAD *)pBuf;

        block_try++;
        block++;
        
        //read page 0, asa flag
        if(nand_remap_read(block * m_PagePerBlock, pBuf, &spare, AK_FALSE) != 0)
        {
            continue;
        }

        //check head
        for(i=0; i<HEAD_SIZE; i++)
        {
            if(pHead->head_str[i] != m_hinfo_data[i])
            {
                break;
            }
        }

        if(i < HEAD_SIZE)
        {
            continue;
        }

        //read time
        if(nand_remap_read((block+1) * m_PagePerBlock - 1, pBuf, &spare, AK_FALSE) != 0)
        {
            continue;
        }

        if(0xFFFFFFFF == spare)
        {
            continue;
        }

        block_end = block;
        if(0 == spare)
        {   
            bLastInfo = AK_TRUE;
            break;
        }

        //update spare
        if(spare > wtime_primary)
        {
            wtime_primary = spare;
            m_asa_head = block_get;

        }

        m_asa_blocks[block_get++] = block;
        m_asa_count = block_get;
    }
    
    if(0 == m_asa_blocks[m_asa_head])
    {
        if (bLastInfo)
        {
    		T_U8 badblock[ASA_MAX_BLOCK_TRY];
            
            m_write_time = 1;


            if (0 != nand_remap_read(block_end * m_PagePerBlock + 1, pBuf, &spare, AK_FALSE))
            {
                remap_free(pBuf);
                return AK_FALSE;
            }

            memcpy(badblock, pBuf, ASA_MAX_BLOCK_TRY);

            progmanage_set_protect(AK_FALSE);
    
            for(block = 1; (block < block_end) && (m_asa_count < ASA_BLOCK_COUNT); block++)
            {       
                if (badblock[block/8] & (1 << (7 - block%8)) != 0)
                {
                    continue;
                }
                if(0 != nand_eraseblock(0, block * m_PagePerBlock))
                {
                    continue;
                }

                for (i=0; i<m_PagePerBlock; i++)
                {
                    if (0 != nand_remap_read(block_end * m_PagePerBlock + i, pBuf, &spare, AK_FALSE))
                    {
                        progmanage_set_protect(AK_TRUE);
                        remap_free(pBuf);
                        return AK_FALSE;
                    }  
                    else
                    {
                        if (0 != nand_remap_write(block * m_PagePerBlock + i, pBuf, m_write_time, AK_FALSE))
                        {
                            break;
                        }    
                    }    
                }

                if (i < m_PagePerBlock)
                {
                    nand_eraseblock(0, block * m_PagePerBlock);
                    continue;
                }
                else
                { 
                    m_asa_head = m_asa_count;  
                    m_asa_blocks[m_asa_count++] = block;
                    m_write_time++;
                }    
            }

            progmanage_set_protect(AK_TRUE);

            ret = (m_asa_count > 0) ? AK_TRUE : AK_FALSE;
        }
        else
        {
            ret = AK_FALSE;
        }    
    }
    else
    {
        m_write_time = wtime_primary+1;

        //if asa blocks is not enough, fill with non-badblock
        if(block_get < ASA_BLOCK_COUNT)
        {
            T_U8 bad_blocks[ASA_MAX_BLOCK_TRY];
            T_U32 i, j;
            spare = 1;

            progmanage_set_protect(AK_FALSE);
            
            //get bad blocks
    		if(asa_get_bad_block(0, bad_blocks, ASA_MAX_BLOCK_TRY))
            {
                for(i = 1; (i < block_end) && (m_asa_count < ASA_BLOCK_COUNT); i++)
                {
                    for(j = 0; j < m_asa_count; j++)
                    {
                        if(m_asa_blocks[j] == i)
                        {
                            break;
                        }
                    }
                    if(j < m_asa_count)
                    {
                        continue;
                    }
                    
                    if((bad_blocks[i/8] & (1<<(7-(i%8)))) == 0)
                    {
                        if (0 != nand_eraseblock(0, i * m_PagePerBlock))
                        {
                            continue;
                        }

                        for (j=0; j<m_PagePerBlock; j++)
                        {
                            if (!asa_read_page(j, pBuf, m_asa_count))
                            {    
                                break;
                            }
                            else
                            {
                                if (0 != nand_remap_write(i * m_PagePerBlock + j, pBuf, spare, AK_FALSE))
                                {
                                    break;
                                }    
                            }    
                        }

                        if (j < m_PagePerBlock)
                        {
                            nand_eraseblock(0, i * m_PagePerBlock);
                            continue;
                        }
                        else
                        {
                            m_asa_blocks[m_asa_count++] = i;
                        }    
                    }
                }
            }

            progmanage_set_protect(AK_TRUE);
        }
    }
    remap_free(pBuf);

    return ret;
}
#pragma arm section code

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
    T_U32 spare;
    T_U32 i;
    T_U32 read_index = m_asa_head;

    //read data
    for (i=0; i<count; i++)
    {
        if (0 != m_asa_blocks[read_index])
        {
            if (0 == nand_remap_read(m_asa_blocks[read_index] * m_PagePerBlock + page, data, &spare, AK_FALSE)) 
            {
                break;
            }    
        }

        read_index = (read_index > 0) ? (read_index - 1) : (m_asa_count - 1);
    } 

    return ((i < count) ? AK_TRUE : AK_FALSE);
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
    T_U32 page_size = (m_BytesPerSector > 4096) ? 4096 : m_BytesPerSector;
    
    page_count = (m_Block_num- 1) / (page_size * 8) + 1;

    for(i = 0; i < bb_count; i++)
    {
        page_index = bad_blocks[i] / (page_size * 8);
        page_offset = bad_blocks[i] - page_index * (page_size * 8);

        if(page != 1 + page_count + page_index)
        {
            continue;
        }

        byte_loc = page_offset / 8;
        byte_offset = 7 - page_offset % 8;
        
        data[byte_loc] |= (1 << byte_offset);
    }
}

/************************************************************************
 * NAME:     asa_get_bad_block
 * FUNCTION  get bad block information of one or more blocks
 * PARAM:    [in] start_block -- start block
 *           [out]pData -------- buffer used to store bad blocks information data
 *           [in] blk_cnt ------ how many blocks you want to know
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_BOOL asa_get_bad_block(T_U32 start_block, T_U8 pData[], T_U32 length)
{
    T_U32 page_index, page_offset;
    T_U32 byte_loc, byte_offset;
    T_U32 index_old = 0xFFFFFFFF;
    T_U8 *pBuf = AK_NULL;
    T_U32 page_count;
    T_U32 i;
    T_U32 page_size = (m_BytesPerSector > 4096) ? 4096 : m_BytesPerSector;
    
    //check parameter
    if(AK_NULL == pData)
    {
        return AK_FALSE;
    }

    page_count = (m_Block_num- 1) / (page_size * 8) + 1;
    
    //alloc memory
    pBuf = (T_U8 *)remap_alloc(ASA_BUFFER_SIZE_SPOT);
    if(AK_NULL == pBuf){
        return AK_FALSE;
    }

    //get all bad blocks
    for(i = 0; i < length; i++)
    {
        page_index = (start_block + i) / (page_size * 8);
        page_offset = (start_block + i) - page_index * (page_size * 8);
        
        byte_loc = page_offset / 8;
        byte_offset = 7 - page_offset % 8;

        //read page data if necessary
        if(index_old != page_index)
        {
            index_old = page_index;
            if(!asa_read_page(1+page_count+page_index, pBuf, m_asa_count))
            {
                remap_free(pBuf);
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

    remap_free(pBuf);
    return AK_TRUE;
}

/************************************************************************
 * NAME:     asa_set_bad_block
 * FUNCTION  set nand flash bad block
 * PARAM:    [in] block -- the bad block item
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_BOOL asa_set_bad_block(T_U32 block)
{
    T_U8 *pBuf = AK_NULL;
    T_U32 block_next = 0;
    T_U32 block_written = 0;
    T_U32 i, j;
    T_U32 bad_blocks[ASA_BLOCK_COUNT] = {0};
    T_U8 bb_count = 0;

    //alloc memory
    pBuf = (T_U8 *)remap_alloc(ASA_BUFFER_SIZE_SPOT);
    if(AK_NULL == pBuf)
    {
        return AK_FALSE;
    }
 
    bad_blocks[0] = block;
    bb_count = 1;

    progmanage_set_protect(AK_FALSE);
    
    block_next = m_asa_head;
    for(j = 0; (j < m_asa_count) && (block_written < 2); j++)
    {
        T_U32 spare;

        spare = m_write_time;

        //get next block
        block_next = (block_next+1) % m_asa_count;
        if(0 == m_asa_blocks[block_next])
        {
            continue;
        }

        if(block_next == m_asa_head)
        {
            remap_free(pBuf);
            return AK_FALSE;
        }

        //erase block
        if(0 != nand_eraseblock(0, m_asa_blocks[block_next] * m_PagePerBlock))
        {
            goto WRITE_FAIL;
        }
        
        //move pages before the page
        for(i = 0; i < m_PagePerBlock; i++)
        {
            if(!asa_read_page(i, pBuf, m_asa_count-1))
            {
                progmanage_set_protect(AK_TRUE);
                remap_free(pBuf);
                return AK_FALSE;
            }

            asa_update_page_data(i, pBuf, bad_blocks, bb_count);

            if(0 != nand_remap_write(m_asa_blocks[block_next] * m_PagePerBlock+ i, pBuf, spare, AK_FALSE))
            {
                goto WRITE_FAIL;
            }
        }

        //update global variable
        m_asa_head = block_next;       
        m_write_time++;
        block_written++;

        continue;

WRITE_FAIL:
        nand_eraseblock(0, m_asa_blocks[block_next]*m_PagePerBlock);
        bad_blocks[bb_count++] = m_asa_blocks[block_next];
        m_asa_blocks[block_next] = 0;

        if (bb_count > 2)
        {
            while(1);
        } 
    }
    
    progmanage_set_protect(AK_TRUE);
    remap_free(pBuf);
    return (block_written > 0) ? AK_TRUE : AK_FALSE;
}

#pragma arm section code = "_update_"
/**
 * @BREIF    write_block
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL write_block(T_ASA_HEAD* pHead, T_ASA_FILE_INFO *pFileInfo, T_U32 fileInfoOffset, T_U8* dataBuf)
{
	T_U32 block_next = 0;
    T_U32 block_written = 0;
	T_U32 i, j;
	T_U32 bad_blocks[ASA_BLOCK_COUNT] = {0};
    T_U8 bb_count = 0;
    T_U8* pTmp = AK_NULL;
	T_U8* pTmpBuf = AK_NULL;
    T_U32 page_size = (m_BytesPerSector> 4096) ? 4096 : m_BytesPerSector;
            
    pTmpBuf = remap_alloc(ASA_BUFFER_SIZE_SPOT);
	if( AK_NULL == pTmpBuf)
	{
		return AK_FALSE;
	}

	block_next = m_asa_head;
    
	progmanage_set_protect(AK_FALSE);

    for(j = 0; j < m_asa_count; j++)
	{
		T_U32 spare;
		
		spare = m_write_time;

		//get next block
		block_next = (block_next+1)%m_asa_count;
		if(0 == m_asa_blocks[block_next])
        {
			continue;
        }

        if(block_next == m_asa_head)
        {
            return AK_FALSE;
        }

		//erase block
		if(0 != nand_eraseblock(0, m_asa_blocks[block_next] * m_PagePerBlock))
		{
			goto WRITE_FAIL;
		}

		//move page
		for(i = 0; i < m_PagePerBlock; i++)
		{
            //没有写过的页，不需要进行读写
            if(i > pHead->info_end && i < m_PagePerBlock - 1)
            {
                continue;
            }
            
			if(i<pFileInfo->start_page || i>=pFileInfo->end_page)
			{ 
				if(!asa_read_page(i, pTmpBuf, 2))
				{
				    remap_free(pTmpBuf);
				    progmanage_set_protect(AK_TRUE);
					return AK_FALSE;
				}
                
                if(0 == i)
                {
                    memcpy(pTmpBuf, pHead, sizeof(T_ASA_HEAD));
                    memcpy(pTmpBuf + fileInfoOffset, pFileInfo, sizeof(T_ASA_FILE_INFO));
                }
                
                asa_update_page_data(i, pTmpBuf, bad_blocks, bb_count);

                pTmp = pTmpBuf;
			}
			else
			{
				pTmp = (T_U8*)(dataBuf + (i - pFileInfo->start_page) * page_size);
			}

			if(0!= nand_remap_write(m_asa_blocks[block_next] * m_PagePerBlock+ i, pTmp, spare, AK_FALSE))
			{
				goto WRITE_FAIL;
			}
		}

		//update global variable
		m_asa_head = block_next;
		m_write_time++;
		block_written++;

		continue;

WRITE_FAIL:
        nand_eraseblock(0, m_asa_blocks[block_next] * m_PagePerBlock);
        bad_blocks[bb_count++] = m_asa_blocks[block_next];
        m_asa_blocks[block_next] = 0;

        if (bb_count > 1)
        {
            remap_free(pTmpBuf);
            progmanage_set_protect(AK_TRUE);
            return AK_FALSE;
        } 
	}

    remap_free(pTmpBuf);
    progmanage_set_protect(AK_TRUE);
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

T_U32 asa_write_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen, T_U8 mode)
{
	T_U8 *pBuf = AK_NULL;
	T_ASA_HEAD HeadInfo;
	T_ASA_FILE_INFO *pFileInfo = AK_NULL;
    T_ASA_FILE_INFO newFileInfo;
    T_U32 file_info_offset;
	T_U32 i,j;
	T_U32 file_num;
	T_BOOL bMatch = AK_FALSE; //must false
	T_U32 page_size = (m_BytesPerSector > 4096) ? 4096 : m_BytesPerSector;
	T_U16 page_cnt = (dataLen-1)/page_size + 1;
	
	if(AK_NULL == file_name || AK_NULL == dataBuf || m_asa_head>= ASA_BLOCK_COUNT)
	{
        return ASA_FILE_FAIL;
    }

	pBuf = remap_alloc(ASA_BUFFER_SIZE_SPOT);
	if( AK_NULL == pBuf)
	{
		return ASA_FILE_FAIL;
	}

	//read page0 to get file info
	if(!asa_read_page(0, pBuf, 2))
	{
		goto EXIT;
	}

	//head info

    memcpy(&HeadInfo, pBuf, sizeof(T_ASA_HEAD));

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
				remap_free(pBuf);
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
		memcpy(pFileInfo->file_name, file_name, 8);

		//whether excess block size
		if(pFileInfo->end_page > m_PagePerBlock)
		{
			goto EXIT;
		}

		//chenge head info if new file
		HeadInfo.item_num++;
		HeadInfo.info_end = pFileInfo->end_page;
	}


    memcpy(&newFileInfo, pFileInfo, sizeof(T_ASA_FILE_INFO));

    remap_free(pBuf);

	if(!write_block(&HeadInfo, &newFileInfo, file_info_offset, dataBuf))
	{
		return ASA_FILE_FAIL;
	}
    else
    {
        return ASA_FILE_SUCCESS;
    }
		
EXIT:
	remap_free(pBuf);
	return ASA_FILE_FAIL;
}

/************************************************************************
 * NAME:     asa_read_file
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
    T_U32 page_size = (m_BytesPerSector > 4096) ? 4096 : m_BytesPerSector;
	T_U16 page_cnt = (dataLen-1)/page_size + 1;
	
	if(AK_NULL == file_name || AK_NULL == dataBuf || m_asa_head >= ASA_BLOCK_COUNT)
	{
        return ASA_FILE_FAIL;
    }

	pBuf = remap_alloc(ASA_BUFFER_SIZE_SPOT);
	if( AK_NULL == pBuf)
	{
		return ASA_FILE_FAIL;
	}

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
		for(; i<dataLen/page_size; i++)
		{
			if(!asa_read_page(start_page+i, dataBuf+i*page_size, 2))
			{
				goto EXIT;
			}	
		}

		if(0 != dataLen%page_size)
		{
			if(!asa_read_page(start_page+i, pBuf, 2))
			{
				goto EXIT;
			}
			else
			{
				memcpy(dataBuf+i*page_size, pBuf, dataLen%page_size);
			}
		}	
	}

	ret= ASA_FILE_SUCCESS;
			
EXIT:
	remap_free(pBuf);
	return ret;
}	

#define SERIAL_NO_NAME      "SERIAL"
#define SERIALNO_LEN         8
/**
 * @BREIF    asa_write_Serialno
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL asa_write_Serialno(T_U8* pData)
{
    if(ASA_FILE_FAIL == asa_write_file(SERIAL_NO_NAME, pData, SERIALNO_LEN, ASA_MODE_CREATE))
    {
        return AK_FALSE;
    }
    return AK_TRUE;
}

/**
 * @BREIF    asa_get_Serialno
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL asa_get_Serialno(T_U8* pData)
{
    if(ASA_FILE_FAIL == asa_read_file(SERIAL_NO_NAME, pData, SERIALNO_LEN))
    {
        return AK_FALSE;
    }

    return AK_TRUE;
}

/************************************************************************
 * NAME:     fha_WriteNandLibVerion
 * FUNCTION  set burn all lib version
 * PARAM:    [in] lib_info--all lib version struct pointer addr
 *           [in] lib_cnt---lib count
 * RETURN:   success return FHA_SUCCESS, fail retuen FHA_ FAIL
**************************************************************************/

T_U32  fha_WriteNandLibVerion(T_LIB_VER_INFO *lib_info,  T_U32 lib_cnt)
{
    T_U32 page_size;
    
    if (AK_NULL == lib_info)
    {
        return FHA_FAIL;
    }

    page_size = (m_BytesPerSector > 4096) ? 4096 : m_BytesPerSector;

    if (sizeof(T_LIB_VER_INFO) * lib_cnt > page_size)
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
 * NAME:     fha_GetNandLibVerion
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

    size = (m_BytesPerSector > 4096) ? 4096 : m_BytesPerSector;

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

#pragma arm section code





