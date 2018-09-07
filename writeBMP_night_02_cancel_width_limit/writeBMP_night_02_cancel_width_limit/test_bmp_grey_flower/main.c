
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
        nLoad = LoadImgFile("./blue_flower.bmp", &pImg, &nWd, &nHi, &nMo, &nDpi, 1);
        printf("nWd=%d, nHi=%d, nMo=%d, nDpi=%d\n", nWd, nHi, nMo, nDpi);
        printf("Load_Result=%d\n", nLoad);

 
	unsigned char *p = pImg;
	uint8_t *q = NULL;
        int nret = save_grey_bmp(p, nWd, nHi, &q, "cias_face_man_write.bmp");

        if(q!=NULL){ free(q); q = NULL;}

	FreeImgMemo(&pImg);

printf("now end ...\n");

	return 0;
} 
