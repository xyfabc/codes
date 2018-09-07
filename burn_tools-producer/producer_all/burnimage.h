#ifndef __BURN_IMAGE__
#define __BURN_IMAGE__

#define BLOCK_RESERVED_RATE (0.16f)
#define YAFFS_TAG_SIZE_512_1024 8
#ifdef CONFIG_YAFFS_SUPPORT_BIGFILE
#define YAFFS_TAG_SIZE 29
#else
#define YAFFS_TAG_SIZE 28
#endif

extern unsigned int yaffs_tag_size;

extern unsigned long partition_count; /* the number of the nandflash partition */
extern unsigned long image_size; /* the total size of image */

extern T_NAND_PHY_INFO nand_info; /*the nand info struct */

extern struct partitions *parts; /*the partition info */
extern unsigned long offset[];

extern unsigned long startblock; /* the first block which the image to be written */
extern unsigned long endblock; /* the end block which the image can be written */	
#endif
