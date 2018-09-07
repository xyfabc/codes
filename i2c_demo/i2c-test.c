#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>

#define BUFF_SIZE   32

int main(int argc, char **argv)
{  
    struct i2c_rdwr_ioctl_data work_queue;
    
    unsigned int idx;
    unsigned int fd;
    unsigned int slave_address, reg_address, size;
    unsigned char val;
    
    int i;
    int ret;
    
    if(argc<5)
    {
        printf("Use:\n%s /dev/i2c-x slave reg_addr size\n",argv[0]);
        return 0;
    }
    sscanf(argv[2],"%x", &slave_address);
    sscanf(argv[3],"%x", &reg_address);
    sscanf(argv[4],"%x", &size);
        
    printf("i2c-test : slave = 0x%02x, reg_addr = 0x%02x, size = 0x%02x\n", slave_address, reg_address, size);
    
    sleep(1);
    
    fd = open(argv[1],O_RDWR);
    
    if(!fd)

    {
        printf("Error on opening the device file\n");
        return 0;
    }
    
    
    work_queue.nmsgs = 2;
    work_queue.msgs = (struct i2c_msg*)malloc(work_queue.nmsgs * sizeof(struct i2c_msg));
    
    if(!work_queue.msgs)
    {
        printf("Memory alloc error.\n");
        close(fd);
        return 0;
    }
    
    ioctl(fd, I2C_TIMEOUT, 1);
    ioctl(fd, I2C_RETRIES, 1);
    
    
    for(i=0; i<size; i++)
    {
        val = i + reg_address;
        
        (work_queue.msgs[0]).len = 1;
        (work_queue.msgs[0]).addr = slave_address;
        (work_queue.msgs[0]).buf = &val;
        
        (work_queue.msgs[1]).len = 1;
        (work_queue.msgs[1]).flags = I2C_M_RD;
        (work_queue.msgs[1]).addr = slave_address;
        (work_queue.msgs[1]).buf = &val;
        
        ret = ioctl(fd, I2C_RDWR, (unsigned long)&work_queue);
        if(ret < 0)
        {
            printf("Error during I2C_RDWR ioctl with error code: %d.\n",ret);
        }
        else
            printf("Reg: %02x val: %02x.\n",i+reg_address, val);
    }
    
    close(fd);
    return 0;
/*
    unsigned int fd;
    unsigned int mem_addr;
    unsigned int size;
    unsigned int idx;
    unsigned int slave;

    char buf[BUFF_SIZE];
    
    char cswap;
    
    union
    {
        unsigned short addr;
        char bytes[2];
    }tmp;
    
    memset(buf,0,sizeof(buf));
    
    if(argc<3)
    {
        printf("Use:\n%s /dev/i2c-x slave mem_addr size\n",argv[0]);
        return 0;
    }
    
    sscanf(argv[2],"%x", &slave);
    sscanf(argv[3],"%x", &mem_addr);
    sscanf(argv[4],"%x", &size);

    printf("i2c-test : slave = %x, mem_addr = %x, size = %x\n", slave, mem_addr, size);
    
    sleep(1);
    
    if(size>BUFF_SIZE)
        size = BUFF_SIZE;
    
    fd = open(argv[1],O_RDWR);
    
    if(!fd)
    {
        printf("Error on opening the device file\n");
        return 0;
    }
    
    ioctl(fd, I2C_SLAVE, slave);
    ioctl(fd, I2C_TIMEOUT, 1);
    ioctl(fd, I2C_RETRIES, 1);
    ioctl(fd, I2C_SET_CLOCK, 50000);
    
    
    for(idx=0; idx<size; ++idx, ++mem_addr)
    {
        tmp.addr = mem_addr;
        cswap = tmp.bytes[0];
        tmp.bytes[0] = tmp.bytes[1];
        tmp.bytes[1] = cswap;
        write(fd, &tmp.addr, 2);
        read(fd, &buf[idx], 1);
    }
    
    buf[size] = 0;
    close(fd);
    printf("Read %d char: \n", size);    
    for(idx=0;idx<size;idx++)
      printf("0x%2x ",buf[idx]);
    
    printf("\n");
    return 0;
*/    
}





