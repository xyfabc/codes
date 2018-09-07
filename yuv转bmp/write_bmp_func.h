
//first version: 2014_12_01
//second version: 2015_02_13

 
#ifndef WRITE_BMP_2014_12_01_KY_H
#define WRITE_BMP_2014_12_01_KY_H


typedef unsigned char uint8_t;

typedef signed short int16_t;
typedef unsigned short uint16_t;

typedef signed int int32_t;
typedef unsigned int uint32_t;


/*ERROR*/
#define BMPERR_INTERNAL_ERROR -501 /*include malloc failure, open file failure*/
#define BMPERR_WIDTH_NOT_FOUR_TIMES -502
#define BMPERR_INVALID_INPUT -503 




/* function:
 * 	save grey raw data (8 bits) to bmp image
 * Paras:
  	pDst, output, the data of the saved image, here pDst can input as NULL.
 * Return:
 * 	+: success, -: failure.
 */
int save_grey_bmp(uint8_t *pSrc, int32_t width, int32_t height, uint8_t *pDst);




#endif

