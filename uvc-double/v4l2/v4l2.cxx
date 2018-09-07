#include "v4l2.h"
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdio.h>
#include <memory.h>
#include <linux/videodev2.h>

const std::string V4l2::DEFAULT_DEVICE("/dev/video0");

V4l2::V4l2(const std::string device) :
    _device(device),
    _fd(-1),
    _width(-1),
    _height(-1),
    _pixfmt(0),
    _mmap_bufs(NULL),
    _mmap_bufs_cnt(0),
    _streaming(false) {
        __open();
    }

V4l2::~V4l2() {
    stopStreaming();
    __munmap();
    __close();
}

void V4l2::setDevice(const std::string dev_name)
{
   _device =  dev_name;
}
bool V4l2::isValid(void) const {
    return (_fd > 0);
}

bool V4l2::__xioctl(unsigned long request, void *arg) const {
   int ret;
   do {
       ret = ioctl(_fd, request, arg);
   } while(-1 == ret && EINTR == errno);
   return 0 == ret;
}

bool V4l2::__open(void) {
    struct stat st;
    struct v4l2_capability cap;

    // The device must be a charactor device
    if(-1 == stat(_device.c_str(), &st) || 
            !S_ISCHR(st.st_mode)) {
        return false;
    }
    _fd = open(_device.c_str(), O_RDWR | O_NONBLOCK);
    if(-1 == _fd) {
        return false;
    }
    memset(&cap, 0, sizeof(cap));
    if(!__xioctl(VIDIOC_QUERYCAP, &cap) || 
            !(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE) ||
            !(cap.capabilities & V4L2_CAP_STREAMING)) {
        close(_fd);
        _fd = -1;
        return false;
    }
    return true;
}

void V4l2::__close(void) {
    if(_fd > 0) {
        close(_fd);
        _fd = -1;
    }
}

bool V4l2::__request_buffers(int cnt) {
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));
    req.count = cnt;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (!__xioctl(VIDIOC_REQBUFS, &req) || 2 > req.count) {
        return false;
    }
    _mmap_bufs_cnt = req.count;
    _mmap_bufs = new mmap_buf[req.count];
    if(!_mmap_bufs) {
        __release_buffers();
        return false;
    }
    return true;
}

void V4l2::__release_buffers(void) {
    if(!_mmap_bufs) {
        return;
    }
    struct v4l2_requestbuffers req;

    memset(&req, 0, sizeof(req));
    req.count = 0;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    __xioctl(VIDIOC_REQBUFS, &req);

    delete [] _mmap_bufs;
    _mmap_bufs = NULL;
    _mmap_bufs_cnt = 0;
}

bool V4l2::__mmap(void) {
    bool ret = true;

    if (!__request_buffers(2)) {
        return false;
    }
    for (int i = 0; i < _mmap_bufs_cnt; ++i) {
        struct v4l2_buffer buf;

        memset(&buf,0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = static_cast<unsigned int>(i);

        if (!__xioctl(VIDIOC_QUERYBUF, &buf)) {
            ret = false;
            break;
        }
        _mmap_bufs[i].length = buf.length;
        _mmap_bufs[i].base = static_cast<unsigned char *> (
                mmap(NULL, buf.length, PROT_READ | PROT_WRITE,
                    MAP_SHARED, _fd, buf.m.offset));
        if(MAP_FAILED == _mmap_bufs[i].base) {
            _mmap_bufs[i].base = NULL;
            ret = false;
            break;
        }
    }
    if (ret) {
        return true;
    }
    
    for(int i = _mmap_bufs_cnt; i--;) {
        if(_mmap_bufs[i].base) {
            munmap(_mmap_bufs[i].base, _mmap_bufs[i].length);
        }
    }
    __release_buffers();
    return false;
}

void V4l2::__munmap(void) {
    for(int i = _mmap_bufs_cnt; i--; ) {
        munmap(_mmap_bufs[i].base, _mmap_bufs[i].length);
    }
    __release_buffers();
}

bool V4l2::setPixelFormat(int width, int height, unsigned int pixfmt,int fps,bool force) {
    struct v4l2_format fmt;
    
    if (_fd < 0 || _streaming) {
        return false;
    }
    if (_width == width && 
            _height == height &&
            _pixfmt == pixfmt) {
        return true;
    }
    __munmap();

   memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.field = V4L2_FIELD_INTERLACED;
    fmt.fmt.pix.width = width;
    fmt.fmt.pix.height = height;
    fmt.fmt.pix.pixelformat = pixfmt;
    
    if(!__xioctl(VIDIOC_S_FMT, &fmt)) {
        return false;
    }
    if(force && (pixfmt != fmt.fmt.pix.pixelformat ||
                width != fmt.fmt.pix.width ||
                height != fmt.fmt.pix.height)) {
        return false;
    }
    if(!__mmap()) {
        return false;
    }
    _width = fmt.fmt.pix.width;
    _height = fmt.fmt.pix.height;
    _pixfmt = fmt.fmt.pix.pixelformat;



    /* set framerate */
    struct v4l2_streamparm setfps;
   // setfps = (struct v4l2_streamparm *) calloc (1,
                                               // sizeof(struct v4l2_streamparm));
    memset (&setfps, 0, sizeof(struct v4l2_streamparm));
    setfps.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    setfps.parm.capture.timeperframe.numerator = 1;
    setfps.parm.capture.timeperframe.denominator = fps;

    if(!__xioctl(VIDIOC_S_PARM, &setfps)) {
        return false;
    }
    return true;
}

bool V4l2::startStreaming(void) const {
    bool ret = true;
    enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

    if (!_mmap_bufs) {
        return false;
    }
    if (this->_streaming) {
        return true;
    }
    for (int i = 0; i < _mmap_bufs_cnt; ++i) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = type;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = static_cast<unsigned int>(i);
        
        if(!__xioctl(VIDIOC_QBUF, &buf)) {
            ret = false;
            break;
        }
    }
    if(!ret || !__xioctl(VIDIOC_STREAMON, &type)) {
        __xioctl(VIDIOC_STREAMOFF, &type);
        return false;
    }
    _streaming = true;
    return true;
}

void V4l2::stopStreaming(void) const {
    if(_streaming) {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        __xioctl(VIDIOC_STREAMOFF, &type);
        _streaming = false;
    }
}


int  V4l2::Grap(unsigned char *tmpb, int *size, int timeout) const {
    fd_set fds;
    struct timeval tv;
    struct v4l2_buffer buf;
   // printf("----------------\n");

    if(!_streaming) {
       // return Image();
        printf("----------------\n");
    }
    // printf("-------1---------\n");
    FD_ZERO(&fds);
    FD_SET(_fd, &fds);
    tv.tv_sec = timeout / 1000;
    tv.tv_usec = 1000 * (timeout % 1000);
 //printf("-------2---------\n");
    if(0 >= select(_fd + 1, &fds, NULL, NULL, &tv)) {
        //return Image();
        printf("--------4--------\n");
    }
    // printf("------3----------\n");
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    if(!__xioctl(VIDIOC_DQBUF, &buf)) {
        printf("--------6--------\n");
       // return Image();
    }
   //  printf("-------4---------\n");
    *size = buf.bytesused;
    memcpy(tmpb,_mmap_bufs[buf.index].base,*size);
  //   printf("--------5--------\n");
   // HorizontalMirror(tmpb, _mmap_bufs[buf.index].base, _width, _height);
   // Image img(_width, _height, _pixfmt, _mmap_bufs[buf.index].base, buf.bytesused);
    __xioctl(VIDIOC_QBUF, &buf);
  //   printf("--------6--------\n");
    //return img;
}
