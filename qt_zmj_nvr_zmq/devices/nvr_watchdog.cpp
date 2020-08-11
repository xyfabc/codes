#include "watchdog.h"
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "nvr_watchdog.h"



Watchdog::Watchdog()
{

}
#define DEV_FILE "/dev/watchdog"
void watchdog_task ()
{
    int timeout = 10;
    int fd  = open(DEV_FILE, O_RDWR);
    if (fd < 0){
        fprintf(stderr, "fail to open file:%s\n", DEV_FILE);
      //   return -1;
    }
    int ret = ioctl(fd, WDIOC_SETTIMEOUT, &timeout);
    if (0 != ret){
        fprintf(stderr, "[Error]%s(%d): ioctl ret =%d\n", __FUNCTION__, __LINE__, ret);
        //return -1;
    }else{
        printf("----------------\n");
    }
    while(1){
        sleep(2);
        printf("Feed WatchDog\n");
        ret = ioctl(fd, WDIOC_KEEPALIVE);
        if (0 != ret){
            fprintf(stderr, "[Error]%s(%d): ioctl ret =%d\n", __FUNCTION__, __LINE__, ret);
            break;
        }
    }

}

static void *watchDogRun(void *param)
{
    printf("-------------ipc_client_task----------------------------\n");

    watchdog_task();
    pthread_detach(pthread_self());
}
int watchDogInit()
{

    pthread_t		watchDogId;
    pthread_create(&watchDogId,NULL,watchDogRun,NULL);

}
