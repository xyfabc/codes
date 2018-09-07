/**
 * @FILENAME: nanddriver.c
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

//logic block to physical block 
T_U32 Prod_LB2PB(T_U32 LogBlock)
{
     //gFHAf.Printf("LogBlock B_%d, step:%d\r\n", LogBlock, g_burn_param.nBlockStep);
      return g_burn_param.nBlockStep * LogBlock;
}

//physical block to logic block

T_U32 Prod_PB2LB(T_U32 PhyBlock)
{
      return PhyBlock / g_burn_param.nBlockStep;
}

/**
 * @BREIF    Prod_NandEraseBlock
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL Prod_NandEraseBlock(T_U32 block_index)
{
    T_U32 phyBlock = Prod_LB2PB(block_index);
    T_U32 page_addr;

    //gFHAf.Printf("E B_%d\r\n", phyBlock);

    page_addr = phyBlock * m_bin_nand_info.page_per_block;

    return (FHA_SUCCESS == gFHAf.Erase(0, page_addr));       
}

/**
 * @BREIF    Prod_NandWriteBinPage
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL Prod_NandWriteBinPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[])
{
    T_U32 phyBlock = Prod_LB2PB(block_index);
    T_U32 page_addr;

    page_addr = phyBlock * m_bin_nand_info.page_per_block + page;
    return (FHA_SUCCESS == gFHAf.Write(0, page_addr, data, m_bin_nand_info.bin_page_size, spare, 4, FHA_DATA_BIN));
}

/**
 * @BREIF    Prod_NandReadBinPage
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL Prod_NandReadBinPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[])
{
    T_U32 phyBlock = Prod_LB2PB(block_index);
    T_U32 page_addr;

    page_addr = phyBlock * m_bin_nand_info.page_per_block + page;
    return (FHA_SUCCESS == gFHAf.Read(0, page_addr, data, m_bin_nand_info.bin_page_size, spare, 4, FHA_DATA_BIN));
}

/**
 * @BREIF    Prod_NandWriteASAPage
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL Prod_NandWriteASAPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[])
{
    T_U32 phyBlock = Prod_LB2PB(block_index);
    T_U32 page_addr;

   //gFHAf.Printf("W B_%d,P_%d\r\n", phyBlock, page);

    page_addr = phyBlock * m_bin_nand_info.page_per_block + page;
    return (FHA_SUCCESS == gFHAf.Write(0, page_addr, data, m_bin_nand_info.bin_page_size, spare, 4, FHA_DATA_ASA));
}

/**
 * @BREIF    Prod_NandReadASAPage
 * @AUTHOR   luqiliu
 * @DATE     2009-12-10
 * @RETURN   T_U32 
 * @retval   FHA_SUCCESS :  succeed
 * @retval   FHA_FAIL :     fail
 */

T_BOOL Prod_NandReadASAPage(T_U32 block_index, T_U32 page, T_U8 data[], T_U8 spare[])
{
    T_U32 phyBlock = Prod_LB2PB(block_index);
    T_U32 page_addr;

    //gFHAf.Printf("R B_%d,P_%d\r\n", phyBlock, page);

    page_addr = phyBlock * m_bin_nand_info.page_per_block + page;
    return (FHA_SUCCESS == gFHAf.Read(0, page_addr, data, m_bin_nand_info.bin_page_size, spare, 4, FHA_DATA_ASA));
}

T_BOOL Prod_NandWriteBootPage(T_U32 page, T_U8 data[])
{
    T_U32 page_addr;
    T_U8 spare[3] = {0};
    T_U32 data_len;
    T_U32 spare_len;

    switch(g_burn_param.eAKChip)
    {
        case FHA_CHIP_880X:
            spare_len = 3;
            break;
        case FHA_CHIP_980X:
            spare_len = 0;
            break;
        default:  
            spare_len = 0;
            break;   
    }   

    data_len = m_bin_nand_info.boot_page_size;

    if (FHA_CHIP_880X != g_burn_param.eAKChip && 0 == page)
    {
        if(FHA_CHIP_10XX == g_burn_param.eAKChip)
        {
            data_len = NAND_BOOT_PAGE_FIRST_10L;
        }
        else
        {
            data_len = NAND_BOOT_PAGE_FIRST;
        }
    }    
    
    page_addr = page;
    return (FHA_SUCCESS == gFHAf.Write(0, page_addr, data, data_len, spare, spare_len, FHA_DATA_BOOT));
}

T_BOOL Prod_NandReadBootPage(T_U32 page, T_U8 data[])
{
    T_U32 page_addr;
    T_U8 spare[3] = {0};
    T_U32 data_len;
    T_U32 spare_len;

    switch(g_burn_param.eAKChip)
    {
        case FHA_CHIP_880X:
            spare_len = 3;
            break;
        case FHA_CHIP_980X:
            spare_len = 0;
            break;
        default:  
            spare_len = 0;
            break;
    }   

    data_len = m_bin_nand_info.boot_page_size;

    if (FHA_CHIP_880X != g_burn_param.eAKChip && 0 == page)
    {
        if(FHA_CHIP_10XX == g_burn_param.eAKChip)
        {
            data_len = NAND_BOOT_PAGE_FIRST_10L;
        }
        else
        {
            data_len = NAND_BOOT_PAGE_FIRST;
        }
    }
    
    page_addr = page;

    if(!gFHAf.Read(0, page_addr, data, data_len, spare, spare_len, FHA_DATA_BOOT))
    {
        return AK_FALSE;
    }  

    if (FHA_CHIP_880X != g_burn_param.eAKChip && 0 == page)
    {
        gFHAf.MemSet(data + data_len, 0, m_bin_nand_info.boot_page_size - data_len);
    }  

    return AK_TRUE;
}


T_BOOL Prod_IsBadBlock(T_U32 block_index)
{
   T_U8  pData[4] = {0};
   T_U32 phyBlock = Prod_LB2PB(block_index);

#ifdef SPOT_SYS
   asa_get_bad_block(phyBlock, pData, 1);

    if ((pData[0] & (1 << 7)) != 0)
    {
        return AK_TRUE;
    }
    else
    {
        return AK_FALSE;
    }    
#else   
   return FHA_check_bad_block(phyBlock);
#endif
}

