#include <stdio.h>
#include <stdlib.h>
#include "fha.h"
#include "nand_char_interface.h"
#include "burnimage.h"

extern T_SPI_INIT_INFO spi_info;
extern T_FHA_INIT_INFO init_info;
extern unsigned char *userpart;
struct partitions *parts;
T_NAND_PHY_INFO nand_info;
unsigned int yaffs_tag_size;

unsigned long partition_count;	/* the number of the nandflash partition */
unsigned long image_size;		/* the total size of image */
unsigned long startblock;		/* the first block which the image to be written */
unsigned long endblock;			/* the end block which the image can be written */
unsigned long startpage;		/* the first page which the image to be written */

#if !(defined(CONFIG_AK37XX_SPI) || defined(CONFIG_AK39XX))
/*
 * judge whether the partition of NO. index can be burned
 */
int burn_yaffs2_image_start(int index)
{	
	int i;
	unsigned long bytes_per_block, filelen_occupy_blocks;

	startblock = parts[index].offset / (nand_info.page_per_blk * nand_info.page_size);
	endblock = startblock + parts[index].size / (nand_info.page_per_blk * nand_info.page_size);
	startpage = 0;
	if (image_size == 0) {
		printf("error:burn_yaffs2_image()!!!\n");
		return AK_FALSE;
	}

	printf("image_size = %d\n", image_size);
	if (image_size % (nand_info.page_size + yaffs_tag_size) != 0) {
		printf("error:burn_yaffs2_image, image_size:%d %%(%d+%d) != 0\n", image_size, nand_info.page_size, yaffs_tag_size);
		return AK_FALSE;
	}

	bytes_per_block = nand_info.page_per_blk * nand_info.page_size;

	//add 28 bytes yaffs tag space
	//filelen_occupy_blocks = (image_size + (bytes_per_block + nand_info.page_per_blk * yaffs_tag_size -1)) \
			/ (bytes_per_block + nand_info.page_per_blk * yaffs_tag_size);
	if (image_size / (nand_info.page_size + yaffs_tag_size) * nand_info.page_size > parts[index].size) {
		printf("error:burn_yaffs2_image(), no such space!!!\n");
		return AK_FALSE;
	}

	if ((userpart[index] == 1) && (init_info.eMode == MODE_UPDATE)) {
		printf("%s is user part, must not be burned", parts[index].name);
		return AK_FALSE;
	}

	return AK_TRUE;
}

/*
 * the buf points to data to be written,
 * buflen must be the times of (nand_info->page_size + yaffs_tag_size),
 * or will return -1;
 */
int burn_yaffs2_image(int index, unsigned char *buf, unsigned long buflen)
{	
	unsigned char *buf_tmp;
	int i;
	unsigned char flag = 0;
	unsigned long page_num, buflen_tmp;
	
	if (buflen % (nand_info.page_size + yaffs_tag_size) != 0) {
		printf("error:burn_yaffs2_image, buflen:%d %%(%d+%d) != 0\n", buflen, nand_info.page_size, yaffs_tag_size);
		return AK_FALSE;
	}
	if (startblock >= endblock) {
		printf("there is no enough space to write!\n");
		return AK_FALSE;
	}

	i = startblock;
	while (buflen > 0 && i < endblock)
	{
		/* 
		 * to configure whether the block is bad or not 
		 * if bad, go to next block
		 */
		if (FHA_check_bad_block(i) == AK_TRUE) {
			i++;
			continue;
		}
		buf_tmp = buf;
		buflen_tmp = buflen;
		while (startpage < nand_info.page_per_blk && buflen_tmp > 0)
		{
			/*
			 * to configure whether the page is written well or not
			 * if not, set the block bad and go to the next block.
			 * rewrite the data in the block again to the next block
			 */
			page_num = i * nand_info.page_per_blk + startpage;
			if (nand_write(0, page_num, buf_tmp, nand_info.page_size,
							buf_tmp + nand_info.page_size, yaffs_tag_size, 3) == AK_FALSE) {
				FHA_set_bad_block(i);
				flag = 1;	/* set the flag when the page is written bad */
				break;
			}
			buf_tmp += (nand_info.page_size + yaffs_tag_size);
			buflen_tmp -= (nand_info.page_size + yaffs_tag_size);
			startpage++;
		}
		if (startpage == nand_info.page_per_blk) {
			i++;
			startpage = 0;
		}
		if (flag) {
			flag = 0;
			i++;
			startpage = 0;
			continue;
		}
		buf = buf_tmp;
		buflen = buflen_tmp;
	}
	startblock = i;
	if (i == endblock)
		return AK_FALSE;

	return AK_TRUE;
}
#else
/*
 * judge whether the partition to burn and the size of image
 * or if in update mode
 * the partition must not be usrt partition
 */
int burn_jffs2_image_start(int index)
{
	int j;
	unsigned int start_block;
	unsigned int end_block;
	if (parts[index].size < image_size) {
		printf("image size large than partition size\n");
		return -1;
	}

	if (init_info.eMode == MODE_UPDATE)
		{	
					printf("erase the system partiton part[%d]\n", index);
					start_block = parts[index].offset / (spi_info.PageSize * spi_info.PagesPerBlock);
					end_block = start_block + parts[index].size / (spi_info.PageSize * spi_info.PagesPerBlock);
					for (j = start_block; j < end_block; j++) {			
						if (SPIFlash_erase(0, j * spi_info.PagesPerBlock) == FHA_FAIL) {
							printf("block %u erase failed\n", j);
							return AK_FALSE;
						} 
						printf("\n");
					}
			printf("erase the system partiton part[%d] successfull\n", index);
		}
/*
	if ((userpart[index] == 1) && (init_info.eMode == MODE_UPDATE)) {
		printf("%s is user part, must not be burned", parts[index].name);
		return -1;
	}
*/
	startpage = parts[index].offset / spi_info.PageSize;
	printf("startpage:%d\n", startpage);

	return 0;
}

/*
 * the buf points to data to be written,
 * if buf length not the times of page size
 * fill it with all 0xff
 */
int burn_jffs2_image(int index, unsigned char *buf, unsigned long len)
{
	int page_count;

	page_count = len / spi_info.PageSize;
	if (len % spi_info.PageSize) {
		memset((buf + len), 0xFF, (spi_info.PageSize - (len % spi_info.PageSize)));
		page_count += 1;
	}

	printf("startpage:%lu, page_count:%lu\n", startpage, page_count);
	if (SPIFlash_write(0, startpage, buf, page_count, NULL, 0, 0) == FHA_FAIL) {
		printf("write page failed, startpage: %lu, data len: %lu\n", startpage, len);
		return -1;
	}
	startpage += page_count;

	return 0;
}

#endif
