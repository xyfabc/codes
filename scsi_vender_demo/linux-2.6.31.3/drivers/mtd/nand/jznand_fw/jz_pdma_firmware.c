/*
 * jz_pdma_firmware.c
 *
 * Jz NAND PDMA firmware
 */

unsigned int pdma_fw[] = {
#ifndef CONFIG_MTD_NAND_TOGGLE
#include "jz_pdma_common.fw"
#else
#include "jz_pdma_toggle.fw"
#endif
};
