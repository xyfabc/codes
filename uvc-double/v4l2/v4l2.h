#ifndef V4L2_H
#define V4L2_H

#include <string>
#include <linux/videodev2.h>

#include "uncopyable.hpp"
//#include "image.hpp"


class V4l2: private Uncopyable
{
public:
    explicit V4l2(const std::string dev_name = DEFAULT_DEVICE);
    ~V4l2();
    bool isValid(void) const;
    bool setPixelFormat(int width, int height, unsigned int pixfmt, int fps, bool force = true);
    bool isStreaming(void) const;
    bool startStreaming(void) const;
    void stopStreaming(void) const;
    
    int Grap(unsigned char *buf,int *size,int timeout) const;
    void setDevice(const std::string dev_name);

private:
    std::string _device;
    int _fd;
    int _width, _height;
    unsigned int _pixfmt;
    struct mmap_buf {
        unsigned char *base;
        size_t length;
    } *_mmap_bufs;
    int _mmap_bufs_cnt;
    mutable bool _streaming;

    static const std::string DEFAULT_DEVICE;
    bool __xioctl(unsigned long request, void *args) const;
    bool __open(void);
    void __close(void);

    bool __request_buffers(int cnt = 4);
    void __release_buffers(void);

    bool __mmap(void);
    void __munmap(void);
};

#endif /* V4L2_H */
