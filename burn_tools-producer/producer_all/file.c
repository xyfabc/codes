/*
 * @FILENAME file.c
 * @BRIEF handle common file burn transc
 * Copyright (C) 2010 Anyka (GuangZhou) Software Technology Co., Ltd.
 * @AUTHOR Zhang Jingyuan
 * @DATE 2010-10-10
 * @version 1.0
 */
#define _LARGEFILE64_SOURCE

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include "fha.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "usbburn.h"

#define COMMAND_MAX_SIZE (ANYKA_MAX_PATH + 8)
static int fd;
static char tar_file_name[ANYKA_MAX_PATH];
static int tar_file_flag;

/*
  * open the file that is being burned
  * if the file is directory, judge whether the file is being.
  */
int start_burn_file(char name[], int isdir)
{
	int i;
	struct stat buf;
	char mkdrcm[ANYKA_MAX_PATH + 9] = "mkdir -p ";

	if (isdir) {
		if (lstat(name, &buf) == 0) {
			if (!S_ISDIR(buf.st_mode)) {
				printf("the file is being, not diretory\n");
				return AK_FALSE;
			}
			else
				return AK_TRUE;
		}
		strcat(mkdrcm, name);
		if (system(mkdrcm) == 0)
			return AK_TRUE;
		else {
			printf("mkdir diretory fail\n");
			return AK_FALSE;
		}
	}

	for (i = strlen(name) - 1; i >= 0; i--)
		if (name[i] == '/')
			break;
	if (i == 0 || i == strlen(name) - 1) {
		printf("the path is error:%s\n", name);
		return AK_FALSE;
	}
	
	strcat(mkdrcm, name);
	mkdrcm[i + 9] = 0;

	//mkdir the directory
	system(mkdrcm);
	
	fd = open(name, O_WRONLY | O_CREAT | O_TRUNC | O_LARGEFILE);
	if (fd <= 0)
		return AK_FALSE;

	/* if the file is unzip file, reserve the name including the absolute path and set the flag */
	if (strcmp(&name[strlen(name) - 6], ".unzip") == 0) {
		strcpy(tar_file_name, name);
		tar_file_flag = 1;
	}

	return AK_TRUE;
}

/* 
  * write the data received to the file
  */
int burn_file(char buf[], int len)
{
	if (fd > 0)
		return write(fd, buf, len);
}

/*
  * close the file when the file data is all burned.
  * if the file is compressed file, decompression it
  * and delete the compressed file.
  */
int close_file()
{
	int i ,ret;
	unsigned char command1[COMMAND_MAX_SIZE] = "tar xvf ",
		command2[COMMAND_MAX_SIZE] = "rm ";

	ret = close(fd);
	if (ret < 0)
		return AK_FALSE;
	fd = -1;

	/* decompressing the file to the download directory and delete it finally */
	if (tar_file_flag == 1) {
		strcat(command1, tar_file_name);
		strcat(command1, " -C ");
		strcat(command1, tar_file_name);
		for (i = strlen(command1); i >= 0; i--) {
			if (command1[i] == '/') {
				command1[i] = '\0';
				break;
			}
		}
		printf("%s\n", command1);
		/* execute the command1 to decompress the file */
		system(command1);

		strcat(command2, tar_file_name);
		printf("%s\n", command2);
		/* execute the command2 to delete the unzip file */
		system(command2);
		tar_file_flag = 0;
	}

	return AK_TRUE;
}
