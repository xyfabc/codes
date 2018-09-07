/*
 * @(#)nandflash.h
 * @date 2009/06/18
 * @version 1.0
 * @author: aijun.
 * @Leader:xuchuang
 * Copyright 2015 Anyka corporation, Inc. All rights reserved.
 * ANYKA PROPRIETARY/CONFIDENTIAL. Use is subject to license terms.
 */


#ifndef __NANDFLASH_H__
#define __NANDFLASH_H__

#include "anyka_types.h"

/******************************************************************************
 The following is the interface with driver-layer.
 ******************************************************************************/

/* The return code when converting the logic addr into real-physic addr. */
#define NF_ADDR_CVT_PARAERR        0xFFFFFFFF

/* Corresponding to ChipCharacterBits */
#define NANDFLASH_COPYBACK_OK           0x80000000
#define NANDFLASH_SERIAL_RW_OK          0x00000001

#define NANDFLASH_MAPPING_MASK          0x70000000
#define NANDFLASH_MAPPING_DIRECTLY      0x40000000  // Directly-mapping mode
#define NANDFLASH_MAPPING_VERTICAL      0x20000000  // Vertical-mapping mode
#define NANDFLASH_MAPPING_HORIZONTAL    0x10000000  // Horizontal-mapping mode
#define NANDFLASH_MAPPING_SUBSECODDEVEN 0x30000000  // Subsection-Odd-Even-mapping mode

//for T_PNANDFLASH::NandType
#define    NANDFLASH_TYPE_OEM          0x0000ffff
#define    NANDFLASH_TYPE_EMULATE      0
#define    NANDFLASH_TYPE_PARTITION    1
#define    NANDFLASH_TYPE_SAMSUNG      2
#define    NANDFLASH_TYPE_HYNIX        3
#define    NANDFLASH_TYPE_TOSHIBA      4

//for function Nand_CreatePartition third paramter.
#define    NANDFLASH_PORTECT_LEVEL_NORMAL      0
#define    NANDFLASH_PORTECT_LEVEL_CHECK       1
#define    NANDFLASH_PORTECT_LEVEL_READONLY    2

//define the error code of nandflash
typedef enum tag_SNandErrorCode
{
    NF_SUCCESS        =    ((T_U16)1),
    NF_FAIL           =    ((T_U16)0),
    NF_WEAK_DANGER    =    ((T_U16)-1),    //successfully read, but a little dangerous.
    NF_ERR_PROGRAM    =    ((T_U16)-2),    //write fail.
    NF_ERR_ERASE      =    ((T_U16)-3),    //erase fail.
    NF_ERR_INIT       =    ((T_U16)-4),    //init error.
    NF_ERR_DEV        =    ((T_U16)-5),    //device error，have bad block
    NF_ERR_LAST       =    ((T_U16)-6),    //last state.

    //add a error code for mtd input paremeter.
    NF_ERR_PARAMETER  =    ((T_U16)-7),    //mtd parameter error
    NF_ERR_POWEROFF   =    ((T_U16)-8),     //FOR DEBUG/
    NF_STRONG_DANGER    =      ((T_U16)-9),      //successfully read, but very dangerous, forbidden to use again.
    NF_FAIL_IN_BLOCK0    =     ((T_U16)-10),      // block0 fails in mutiplane mode
    NF_FAIL_IN_BLOCK1    =     ((T_U16)-11),      // block1 fails in mutiplane mode
    NF_FAIL_IN_BLOCK2    =     ((T_U16)-12),      // block2 fails in mutiplane mode
    NF_FAIL_IN_BLOCK3    =     ((T_U16)-13)      // block3 fails in mutiplane mode
}E_NANDERRORCODE;

typedef struct SNandflash  T_NANDFLASH;
typedef struct SNandflash *T_PNANDFLASH;

typedef E_NANDERRORCODE (*fNand_WriteSector)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 block, T_U32 page, const T_U8 data[], T_U8* oob,T_U32 oob_len);
                              
typedef E_NANDERRORCODE (*fNand_ReadSector)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 block, T_U32 page, T_U8 data[], T_U8* oob,T_U32 oob_len);

                              
typedef E_NANDERRORCODE (*fNand_WriteFlag)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 block, T_U32 page, T_U8* oob,T_U32 oob_len);
                              
typedef E_NANDERRORCODE (*fNand_ReadFlag)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 block, T_U32 page, T_U8* oob,T_U32 oob_len);
                              
typedef E_NANDERRORCODE (*fNand_CopyBack)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 SourceBlock, T_U32 DestBlock, T_U32 page);
                              
typedef E_NANDERRORCODE (*fNand_MultiCopyBack)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 PlaneNum, T_U32 SourceBlock, T_U32 DestBlock, T_U32 page);
                              
typedef E_NANDERRORCODE (*fNand_MultiReadSector)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 PlaneNum, T_U32 block, T_U32 page,T_U8 data[], T_U8* SpareTbl,T_U32 oob_len);//SpareTbl将是包含MutiPlaneNum个T_MTDOOB缓冲的指针
                              
typedef E_NANDERRORCODE (*fNand_MultiWriteSector)(T_PNANDFLASH nand, T_U32 chip,
                              T_U32 PlaneNum, T_U32 block, T_U32 page,const T_U8 data[], T_U8* SpareTbl,T_U32 oob_len);    //SpareTbl将是包含MutiPlaneNum个T_MTDOOB缓冲的指针
                              
typedef E_NANDERRORCODE (*fNand_EraseBlock)(T_PNANDFLASH nand, T_U32 chip, T_U32 block);

typedef E_NANDERRORCODE (*fNand_MultiEraseBlock)(T_PNANDFLASH nand, T_U32 chip, T_U32 planeNum, T_U32 block);

typedef T_BOOL (*fNand_IsBadBlock)(T_PNANDFLASH nand, T_U32 chip, T_U32 block);

typedef T_BOOL (*fNand_SetBadBlock)(T_PNANDFLASH nand, T_U32 chip, T_U32 block);

typedef T_U32  (*fNand_Fake2Real)(T_PNANDFLASH nand, T_U32 plane,
                                  T_U32 FakePhyAddr, T_U32 *chip);

typedef struct tag_SPartitionNandFlash
{
    T_U32 StartBlk;
    T_U32 BlkCnt;
    T_PNANDFLASH large;
}T_PARTITIONNANDFLASH, *T_PPARTITIONNANDFLASH;

#define NANDFLASH_RANDOMISER        17
#define NANDFLASH_COPYBACK          16
#define NANDFLASH_MULTI_COPYBACK    15
#define NANDFLASH_MULTI_READ        14
#define NANDFLASH_MULTI_ERASE       13
#define NANDFLASH_MULTI_WRITE       12

struct SNandflash        //介质结构体说明
{
    T_U32   NandType;
    /* character bits, 最高4位表示plane属性，最低位表示是否需要block内顺序写page
       bit31表示是否有copyback，1表示有copyback
       bit30表示是否只有一个plane，1表示只有一个plane
       bit29表示是否前后plane，1表示有前后plane
       bit28表示是否奇偶plane，1表示有奇偶plane
       bit0表示在同一个block内是否需要顺序写page，1表示需要按顺序写，即该nand为MLC
    注意: 如果(bit29和bit28)为'11'，则表示该chip包括4个plane，既有奇偶也有前后plane */
    T_U32   ChipCharacterBits;
    T_U32   PlaneCnt;
    T_U32   PlanePerChip;
    T_U32   BlockPerPlane;
    T_U32   PagePerBlock;
    T_U32   SectorPerPage;
    T_U32   BytesPerSector;
    fNand_WriteSector   WriteSector;
    fNand_ReadSector    ReadSector;
    fNand_WriteFlag     WriteFlag;
    fNand_ReadFlag      ReadFlag;
    fNand_EraseBlock    EraseBlock;
    fNand_CopyBack      CopyBack;
    fNand_IsBadBlock    IsBadBlock;  
    fNand_SetBadBlock    SetBadBlock;
    fNand_Fake2Real     Fake2Real; // the func of phy addr converting
    fNand_MultiReadSector    MultiRead;
    fNand_MultiWriteSector    MultiWrite;
    fNand_MultiCopyBack        MultiCopyBack;
    fNand_MultiEraseBlock     MultiEraseBlock;
    T_U32     BufStart[1];
};
#endif


