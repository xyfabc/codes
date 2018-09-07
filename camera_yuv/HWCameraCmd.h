
#ifndef _CAMERA_CMD_H_
#define _CAMERA_CMD_H_

#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>
/*
 * IOCTL_XXX commands
 */
#define IOCTL_SET_IMG_PARAM                       0    // arg type: img_param_t *
#define IOCTL_CIM_CONFIG                          1    // arg type: cim_config_t *
#define IOCTL_STOP_CIM                            2   // arg type: void
#define IOCTL_GET_IMG_PARAM                       3    // arg type: img_param_t *
#define IOCTL_GET_CIM_CONFIG                      4    // arg type: cim_config_t *
#define IOCTL_TEST_CIM_RAM                        5    // no arg type *
#define IOCTL_START_CIM                           6    // no arg type *
#define IOCTL_GET_IPU_ADDR                        7    // IPU Y U V address: ipu_buf_t
#define IOCTL_GET_IPU_OUTADDR                     8    // IPU Y U V address: ipu_buf_t
#define IOCTL_CIM_REFRESH                         9
#define IOCTL_SET_SENSOR                          10

#define IMAGE_BPP                                 16
#define IMAGE_WIDTH                               640
#define IMAGE_HEIGHT                              480



#define CIM_INITIAL                               1
#define CIM_PREVIEW                               2
#define CIM_STOP                                  3

#define VIDIOC_S_EXPOSURE                         300
#define VIDIOC_S_BRIGHT                           301
#define VIDIOC_S_CONTRAS                          302
#define VIDIOC_S_CONTRAS_CENTER                   303

#define VIDIOC_S_REG                              309
#define VIDIOC_G_REG                              308
#define MAX_YUV_BUF_NUM                           3
#define MAX_RGB_BUF_NUM                           3
typedef struct _videobuffer {
    void * start;
    ssize_t length;
}VideoBuffer;



#endif
