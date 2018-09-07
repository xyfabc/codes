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

#define ASA_FILE_FAIL   	0
#define ASA_FILE_SUCCESS    1
#define ASA_FILE_EXIST   	2
#define ASA_MODE_OPEN		0
#define ASA_MODE_CREATE		1

T_BOOL asa_initial(T_U32 PagePerBlock, T_U32 BytesPerSector, T_U32 Block_num);

T_BOOL asa_get_bad_block(T_U32 start_block, T_U8 pData[], T_U32 length);

T_BOOL asa_set_bad_block(T_U32 block);

T_U32 asa_write_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen, T_U8 mode);

T_U32 asa_read_file(T_U8 file_name[], T_U8 dataBuf[], T_U32 dataLen);

T_BOOL asa_write_Serialno(T_U8* pData);

T_BOOL asa_get_Serialno(T_U8* pData);




