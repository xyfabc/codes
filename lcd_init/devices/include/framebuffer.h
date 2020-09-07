#ifndef FRAMEBUFFER_H
#define FRAMEBUFFER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <pthread.h>

#include "hifb.h"
#include "hi_common.h"
#include "hi_comm_sys.h"
#include "hi_comm_vo.h"
#include "hi_comm_vb.h"

#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vpss.h"
#include "mpi_vdec.h"
#include "mpi_hdmi.h"
#include "mpi_vo.h"
#include "mpi_venc.h"
#include "mpi_region.h"
#include "sample_comm.h"



#define WIDTH                  1920
#define HEIGHT                 1080

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* __cplusplus */



typedef struct hiPTHREAD_HIFB_SAMPLE
{
    HI_S32 fd;
    HI_S32 layer;
    HI_S32 ctrlkey;
}PTHREAD_HIFB_SAMPLE_INFO;

extern HI_S32 hifb_init();
extern HI_S32 SetMousePos(int x,int y);
extern int HIFB_G3_init();



#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */

class framebuffer
{
public:
    framebuffer();
};



#endif // FRAMEBUFFER_H
