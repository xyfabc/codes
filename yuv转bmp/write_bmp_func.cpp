
#include<stdio.h>
#include<malloc.h>
#include<string.h>
#include<stdlib.h>

#include "write_bmp_func.h"


int save_grey_bmp(uint8_t *pSrc, int32_t width, int32_t height, uint8_t *pDst)
{
	if( (width == 0) || (height==0) ) return (BMPERR_INVALID_INPUT);
	if(pDst == NULL) return (BMPERR_INVALID_INPUT);

	int i = 0;

	int dataSizePerLine = (width + 3)/4*4;//for future use
	int nsizeImage = dataSizePerLine * height;
	int nsrcSize = nsizeImage; //width * height;
	
	uint8_t *p = pDst;

	int nfilesize = 14 + 40 + 4*256 + nsizeImage;
	int noffbits = 14 + 40 + 4*256;

	/*file header, 14 bytes */
	p[0] = 'B'; p[1] = 'M';//bfType
	p[2] = (nfilesize)&0xff; p[3] = (nfilesize>>8)&0xff; p[4] = (nfilesize>>16)&0xff; p[5] = (nfilesize>>24)&0xff;//bdSize
	p[6] = 0x00; p[7] = 0x00;//bfReserved1
	p[8] = 0x00; p[9] = 0x00;//bfReserved2
	p[10] = (noffbits)&0xff; p[11] = (noffbits>>8)&0xff; p[12] = (noffbits>>16)&0xff; p[13] = (noffbits>>24)&0xff;//bfOffBits

	/*bitmap information header, 40 bytes*/
	p[14] = 0x28; p[15] = 0x00; p[16] = 0x00; p[17] = 0x00;//biSize, fixed 40
	p[18] = (width)&0xff; p[19] = (width>>8)&0xff; p[20] = (width>>16)&0xff; p[21] = (width>>24)&0xff;//Width
	p[22] = (height)&0xff; p[23] = (height>>8)&0xff; p[24] = (height>>16)&0xff; p[25] = (height>>24)&0xff;//Height
	p[26] = 0x01; p[27] = 0x00;//biPlanes, 1
	p[28] = 0x08; p[29] = 0x00;//biBitCount,here grey 8 bits
	p[30] = 0x00; p[31] = 0x00; p[32] = 0x00; p[33] = 0x00;//biCompression, 0 no compression
	p[34] = (nsizeImage)&0xff; p[35] = (nsizeImage>>8)&0xff; p[36] = (nsizeImage>>16)&0xff; p[37] = (nsizeImage>>24)&0xff;//biSizeImage
	p[38] = 0x00; p[39] = 0x00; p[40] = 0x00; p[41] = 0x00;//biXPelsPerMeter
	p[42] = 0x00; p[43] = 0x00; p[44] = 0x00; p[45] = 0x00;//biYPelsPerMeter
	p[46] = 0x00; p[47] = 0x00; p[48] = 0x00; p[49] = 0x00;//biClrUsed, 0
	p[50] = 0x00; p[51] = 0x00; p[52] = 0x00; p[53] = 0x00;//biClrImportant, 0

	/*color table*/
	for(i=0; i<256; i++){
		p[54+4*i+0] = i;
		p[54+4*i+1] = i;
		p[54+4*i+2] = i;
		p[54+4*i+3] = 0;
	}
	uint8_t *q = p+14+40+4*256;
	
	unsigned char zeropad[] = "\0\0\0\0";

        pSrc += width * (height - 1);
        for( i = 0; i< height ; i++ ){
                memcpy(q, pSrc, width);
                if(dataSizePerLine > width){
                        memcpy(q+width, zeropad, dataSizePerLine - width);
                }
                q += dataSizePerLine;
                pSrc -= width;//here here, note bmp save order, bottom->up, left->right
        }


	 return (14+40+4*256+nsrcSize);
}

