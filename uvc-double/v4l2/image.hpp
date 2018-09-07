#ifndef IMAGE_H
#define IMAGE_H

#include <memory>
#include <cstring>
#include <linux/videodev2.h>

class Image
{
    private:
        int _width, _height;
        unsigned int _pixfmt;
        unsigned int _length;
        std::shared_ptr<unsigned char> _data;

    public:
        Image():
            _width(-1),
            _height(-1),
            _pixfmt(0),
            _length(0),
            _data(nullptr) {}

        Image(int width,  int height, unsigned int pixfmt, const unsigned char *data, unsigned int length) :
            _width(width),
            _height(height),
            _pixfmt(pixfmt),
            _length(length),
            _data(new unsigned char[length], std::default_delete<unsigned char[]>()) {
                if (data && _data) {
                    std::memcpy(_data.get(), data, length);
                }
            }


        bool isValid(void) const {return !!_data; }
        int width(void) const {return _width;}
        int height(void) const {return _height;}
        int pixfmt(void) const {return _pixfmt;}
        int length(void) const {return _length;}
        unsigned char *data(void) {return _data.get();}
        unsigned char const *data(void) const {return _data.get();}
};

#endif /* IMAGE_H */
