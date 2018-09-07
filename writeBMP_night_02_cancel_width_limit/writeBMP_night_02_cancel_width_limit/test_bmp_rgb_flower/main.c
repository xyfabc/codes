
#include<stdio.h>
#include<malloc.h>
#include<string.h>
#include<stdlib.h>

#include "write_bmp_func.h"
#include "TesoPics.h"

int main(int argc, char *argv[])
{
printf("now begin .... \n");	

        int nWd, nHi, nMo, nDpi;
        int nLoad;

        BYTE *pImg = NULL;
        nLoad = LoadImgFile("./blue_flower.bmp", &pImg, &nWd, &nHi, &nMo, &nDpi, 0);
        printf("nWd=%d, nHi=%d, nMo=%d, nDpi=%d\n", nWd, nHi, nMo, nDpi);
        printf("Load_Result=%d\n", nLoad);

 
	unsigned char *p = pImg;
	uint8_t *q = NULL;
        int nret = save_rgb_bmp(p, nWd, nHi, &q, "blue_flower_inverse.bmp");

        if(q!=NULL){ free(q); q = NULL;}

	FreeImgMemo(&pImg);

printf("now end ...\n");

	return 0;
} 
