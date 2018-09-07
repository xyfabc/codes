#include <sys/types.h>          /* See NOTES */
#include <signal.h>
#include <sys/select.h>
#include <stdio.h>      /*标准输入输出定义*/
#include <stdlib.h>     /*标准函数库定义*/
#include <unistd.h>     /*Unix 标准函数定义*/
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>      /*文件控制定义*/
#include <termios.h>    /*PPSIX 终端控制定义*/
#include <errno.h>      /*错误号定义*/
#include <string.h>      /*字符串相关*/
#include <malloc.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <asm/types.h>         
#include <stdint.h>
#include <assert.h>
#include <stdbool.h>

#include "err.h"



#define COM_DEVICE0  "/dev/ttyS0"
static int iComFd0 = -1;

/*
 * @brief  设置串口通信速率
 * @param  fd     类型 int  打开串口的文件句柄
 * @param  speed  类型 int  串口速度
 * @return  void
 * @create at 2017.05.12 by Eric.Xi
 */
 
static int speed_arr[] = { B115200,B38400, B19200, B9600, B4800, B2400, B1200, B300};

static int name_arr[] = {  115200, 38400,  19200,  9600,  4800,  2400,  1200,  300};

static int set_speed(int fd, int speed)
{
  int   i,iRet; 
  int   status; 
  struct termios   Opt;
  iRet = tcgetattr(fd, &Opt); 
  if(iRet  !=  0) 
  { 
	DBG_PRINTF("tcgetattr Serial fail\n");     
	return  -1;  
  }
  
  for ( i= 0;  i < sizeof(speed_arr) / sizeof(int);  i++) 
  { 
    if  (speed == name_arr[i]) 
	{     
      tcflush(fd, TCIOFLUSH);                    
	  
      cfsetispeed(&Opt, speed_arr[i]);           
      cfsetospeed(&Opt, speed_arr[i]);           

	  
      status = tcsetattr(fd, TCSANOW, &Opt);     
      if(status != 0) 
	  {
	    DBG_PRINTF(" tcsettattr com baurdrate = %d error\n",speed_arr[i]);
        return -1;     
      }
	  
      tcflush(fd,TCIOFLUSH);                    

	  return 0;
    }  
  }
  DBG_PRINTF(" invaild baudrate \n");
  return -1;
}
 

/* 设置效验的函数：
 *
 * @brief   设置串口数据位，停止位和效验位
 * @param   fd     类型  int  打开的串口文件句柄
 * @param   databits 类型  int 数据位   取值 为 7 或者8
 * @param   stopbits 类型  int 停止位   取值为 1 或者2
 * @param   parity  类型  int  效验类型 取值为N,E,O,,S
 * @create at 2017.05.12 by Eric.Xi
 */
static int set_Parity(int fd,int databits,int stopbits,int parity)
{ 
    int iRet;
	struct termios options; 
	
	iRet = tcgetattr(fd , &options);
	
	if(iRet  !=  0) 
	{ 
		DBG_PRINTF("Please SetupSerial first\n");     
		return  -1;  
	}

	/* 1、设置数据位数  */
	options.c_cflag &= ~CSIZE; 
	switch (databits) 
	{   
		case 7:		
			options.c_cflag |= CS7; 
			break;
		case 8:     
			options.c_cflag |= CS8;
			break;   
		default:    
			DBG_PRINTF("Unsupported data size\n"); 
			return -1;  
			
	}
	
	switch (parity) 
	{   
		case 'n':
		case 'N':    
			options.c_cflag &= ~PARENB;    
			options.c_iflag &= ~INPCK;     
			break;  
			
		case 'o':   
		case 'O':     
			options.c_cflag |= (PARODD | PARENB); 
			options.c_iflag |= INPCK;             
			break;  
			
		case 'e':  
		case 'E':   
			options.c_cflag |= PARENB;      
			options.c_cflag &= ~PARODD;     
			options.c_iflag |= INPCK;       
			break;
			
		case 'S': 
		case 's':  /*as no parity*/   
		    options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
            options.c_iflag |= INPCK; /* disable pairty checking */
			break;  
			
		default:   
			DBG_PRINTF("Unsupported parity\n");    
			return -1;  
			
	} 

	
	/* 设置停止位*/  
	switch (stopbits)
	{   
		case 1:    
			options.c_cflag &= ~CSTOPB;  
			break;  
		case 2:    
			options.c_cflag |= CSTOPB;  
		   break;
		default:    
			 DBG_PRINTF("Unsupported stop bits\n");  
			 return -1; 
	} 

	if (parity != 'n' || parity != 'N')   
		options.c_iflag |= INPCK; 

    options.c_cflag |= CLOCAL | CREAD ;      
    options.c_cflag &= ~CRTSCTS ;            
    options.c_iflag &= ~(INLCR|ICRNL);
    options.c_iflag &= ~(IXON);
    options.c_iflag &= ~(IXON | IXOFF | IXANY); 
    options.c_lflag  &= ~(ICANON | ECHO | ECHOE | ISIG);  /* Input */
    options.c_oflag  &= ~OPOST;   /* Output */

	tcflush(fd,TCIFLUSH);      

	options.c_cc[VTIME] = 0; 
	options.c_cc[VMIN] = 1;

	iRet = tcsetattr(fd,TCSANOW,&options);
	
	if (iRet != 0)   
	{ 
		DBG_PRINTF("tcsetattr com fail\n");   
		return -1;  
	} 
	
	return 0;  
}

/*
 * @breif 打开串口
 * @create at 2017.05.12 by Eric.Xi
 */
static int opencomdev(char *pcComDev)
{

    int	fd = open(pcComDev, O_RDWR | O_NOCTTY | O_NDELAY);      
	if (-1 == fd)
	{
		DBG_PRINTF("Can't Open Serial Port");
		return -1;
	}
	else
	  return fd;
}



static int init_com(int fd,int speed,int databits,int stopbits,int parity)
{
   int iRet;
   
   if(fd == -1)
   {
      DBG_PRINTF("invild file describe fd = %d\n",fd);
	  return -1;
   }
   
   iRet = set_speed(fd,speed);
   if(iRet == -1)
   {
      DBG_PRINTF("set baudrate fail\n");
	  return -1;
   }

   iRet = set_Parity(fd,databits,stopbits,parity);
   if(iRet == -1)
   {
      DBG_PRINTF("set serial databits stopbits parity fail\n");
	  return -1;
   }

   return 0;
   	
}

static int select_comdev(int id)
{
   return iComFd0;
}





/*
 * 原型： int hw_com_init(void);
 * 功能： 串口初始化。
 * 参数： 无。
 * 返回： 0 成功，小于0失败。
 * @create at 2017.05.12 by Eric.Xi
 */


int hw_com_init()
{
   
   //init card com
   iComFd0 = opencomdev(COM_DEVICE0);
   if(iComFd0 == -1)
   {
      DBG_PRINTF("open serial 3 fail\n");
   }
   if(iComFd0 > 0){
   	 init_com(iComFd0,19200,8,1,'N');
   }
     
   if(iComFd0 == -1)
   	return ERR_FAIL;
   else
   	return ERR_OK;
   
}

/*
 * 原型： int hw_com_send(int id, void *data, int len);
 * 功能： 串口发送数据。
 * 参数： IN id: 串口标号，如果有n 个串口，那么id 取值为0~n-1
 * IN data: 要发送的数据。
 * IN len: 数据长度
 * 返回： 0 成功，小于0失败。
 * @create at 2017.05.12 by Eric.Xi
 */

int hw_com_send(int id, void *data, int len)
{
   int fd;
   int write_cnt;
   if((id > 3) || (id < 1) || (len <= 0))
   {
      DBG_PRINTF("invaild id or len\n");
	  return ERR_PARA;
   }

   fd = select_comdev(id);
   if(fd == -1)
   {
      DBG_PRINTF(" invaild serial id = %d\n",id);
	  return ERR_FAIL;
   }

   tcflush(fd,TCOFLUSH);       
  /* tcflush(fd,TCIOFLUSH); */ 
   
   write_cnt = write(fd,data,len);
   if(write_cnt < len)
   {
      DBG_PRINTF("write com fail\n");
      tcflush(fd,TCOFLUSH);    
	  return ERR_FAIL;
   }
  /*  tcflush(fd,TCOFLUSH);  */
   return ERR_OK;
}

/*
 * 原型： int hw_com_receive_timeout(int id, void *buf, int len,int timeout);
 * 功能： 串口接收数据。
 * 参数： IN id: 串口标号
 * OUT buf: 缓冲区。
 * OUT len: 数据长度
 * timeout: 串口数据接收超时时间，单位MS
 * 返回： 0 成功，小于0失败。
 * @create at 2017.05.12 by Eric.Xi
 */

//int hw_com_receive_timeout(int id, void *buf, int len,int timeout)
int hw_com_receive_timeout(int id, void *buf,int timeout)
{
	   int i;
	   int fd;
	   int iRet;  
	   int read_cnt;
	   int s_val, ms_val;
	   
	   fd_set fds;    
	   struct timeval tv;    


	   unsigned char *tmp_buf = (unsigned char *)buf;
	   if((id > 3) || (id < 1) )
	   {
		  DBG_PRINTF("invaild id or len\n");
		  return ERR_PARA;
	   }

	   fd = select_comdev(id);
	   if(fd == -1)
	   {
		DBG_PRINTF(" invaild serial id = %d\n",id);
		return ERR_FAIL;
	   }

	   s_val = timeout/1000;
	   ms_val = timeout%1000;
	   i = 0;
	   
	   while(1)
	   {	   
		   FD_ZERO (&fds);    
		   FD_SET (fd, &fds);

			/* Timeout. */    
	    	   tv.tv_sec = s_val;    
	              tv.tv_usec = ms_val*1000; 

				  //	DBG_PRINTF(" tv.tv_sec = %d::tv.tv_usec = %d\n",tv.tv_sec,tv.tv_usec);

		   iRet = select (fd + 1, &fds, NULL, NULL, &tv); 
		   if(iRet <= 0)  
		   {
			if((iRet == -1) && (EINTR == errno)) /* 表示select的出错,是由于中断造成的 */
			{
				 DBG_PRINTF("read com data timeout continue \n");
				 continue;
			}
			else
			{
				// DBG_PRINTF("read com data timeout\n");
				if(i != 0)
					return i;
				else
					return ERR_FAIL;
			}
		   }	   
		   else
		   {
			 read_cnt = read(fd,&tmp_buf[i],1); 
			// DBG_PRINTF("read com data = %d::%x\n",i,tmp_buf[i]);
			 if(read_cnt <= 0) 
			 {
				DBG_PRINTF("read com data error i+1 = %d\n",i+1);
				return ERR_FAIL;
			 }
				 i++;
		   }
	   }

	return i;
}

 struct ic_id_dev
 {
	 unsigned char rx_buffer[64];
	 unsigned int rx_cnt;
 };
 
 
 static struct ic_id_dev *ic_id_devp;

 static int drvICcmd(Uint32 nodeid, int funcode, Uint8 *data, Uint32 datalen)
 {
	 int	 i;
	 int err;
	 Uint8	 xor = (char)0;
	 Uint8	 tmpbuf[32];
 
	 tmpbuf[0] = (Uint8)0xAA;
	 tmpbuf[1] = (Uint8)0xBB;
	 tmpbuf[2] = (Uint8)((datalen + 5) & 0xFF);
	 tmpbuf[3] = (Uint8)((datalen + 5) >> 8);
	 tmpbuf[4] = (Uint8)(nodeid & 0xFF);
	 tmpbuf[5] = (Uint8)(nodeid >>8);
	 tmpbuf[6] = (Uint8)(funcode & 0xFF);
	 tmpbuf[7] = (Uint8)(funcode >> 8);
 
	 for(i=0;i<datalen;i++)
	 {
		 tmpbuf[8+i] = *data++;
	 }
 
	 for (i = 0; i < (datalen + 4); i ++)
	 {
		 xor ^= tmpbuf[i + 4];
	 }
 
	 tmpbuf[datalen + 8] = xor;
 
	// err = uart_puts(tmpbuf, datalen + 9);
	  err =  hw_com_send(1,tmpbuf, datalen + 9);
	 
	 return err;
 }



 int drvICReceiveCardNum_Cmd(void)
{
	int err;
	Uint8	buf[4];

	buf[0] = 0x52;
	err = drvICcmd(0, CMD_REQUEST_CARD, buf, 1);
	if(err < 0)
	{
		printf("RF Card CMD_REQUEST_CARD CMD ERROR\n!");
		return -2;
	}
	
	usleep(20*1000);
	//usleep(20000);
	
	err = drvICcmd(0, CMD_ANTI_COLLISION, 0, 0);
	if(err < 0)
	{
		printf("RF Card CMD_ANTI_COLLISION CMD ERROR\n!");
		return -3;
	}
	//usleep(200000);
	usleep(20*1000);
	

	return 0;
}  


static int drvICInit(void)
{
	int err;
	Uint8	buf[4];

	buf[0] = 1;
	err = drvICcmd(0, CMD_SET_ANNTENA, buf, 1);
	if(err < 0)
	{
		printf("rfcard cmd error:CMD_SET_ANNTENA\n");
		return -1;
	}
	
	sleep(1);
	
	buf[0] = 0x52;
	err = drvICcmd(0, CMD_REQUEST_CARD, buf, 1);
	if(err < 0)
	{
		printf("rfcard cmd error:CMD_REQUEST_CARD\n");
		return -2;
	}

	return 0;
}

#define   GET_ID_TIMEOUT   100   /* 50ms */
int get_card_id(unsigned int * data)
{
	unsigned char *pt = ic_id_devp->rx_buffer;
	int i = 0;
	Uint8	buf[4];
	int index;
	int ret = 0;

  	memset(pt,0xff,64);
  	ic_id_devp->rx_cnt = 0;

	if(drvICReceiveCardNum_Cmd() < 0)
	{
				printf("Get RFCardNum error!!\n");
				return -1;
	}
	ret = hw_com_receive_timeout(1, pt,GET_ID_TIMEOUT);
	if (ret < 0){
		DBG_PRINTF("hw_com_receive_timeout!\n"); 
		return -1;
	 } 

	if((pt[18] == 0x02) && (pt[19] == 0x02) && (pt[20] == 0x00))	// it is a legal card
	{
		//num = (pt[9] << 24) | (pt[10] << 16) | (pt[11] << 8) | pt[12];			
		index = 21;
			
		for(i = 0;i < 4;i ++)
		{
			buf[i] = pt[index];
			if((buf[i] == 0xaa) && (pt[index + 1] == 0x00))
				index += 2;
			else
				index += 1;
		}
	  *data = (buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];	
	  //if(0x16A4AE00 != *data)
	  printf("\nget a new card: 0x%08X\n", *data);
	  
	  ret = 0;

	}
	else
		ret = -1;	
		 
  return ret;
}




/*
 * @func get card id demo
 * @create at 2017.05.12 by Eric.Xi
 */

int main()
{
	char buf[32] = {0};
	int ret;	 
	unsigned int card_num = 0;

	ic_id_devp = (struct ic_id_dev *)malloc(sizeof(struct ic_id_dev));
	if(NULL == ic_id_devp)
	{
		return -1;
	}

	
	hw_com_init();
	drvICInit();
 	struct  timeval t1, t2;
	while(1){
	gettimeofday(&t1,NULL);



		ret = get_card_id(&card_num);
		if(ERR_OK == ret){
       			gettimeofday(&t2,NULL);
        		printf("\nget id ok time: %ld\n",1000000*(t2.tv_sec-t1.tv_sec)+ t2.tv_usec-t1.tv_usec);
			//DBG_PRINTF("get card id ok!\n"); 
		}

		//sleep(1);
	}

	return 0;
}
