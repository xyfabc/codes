#ifndef CAPTH_H
#define CAPTH_H

#include <QThread>
#include "v4l2/v4l2.h"




#if 1
#define WIDTH                                   640
#define HEIGHT                                  480
#define FPS                                     15
#else

#define WIDTH                                   1280
#define HEIGHT                                  720
#define EYE_DISTANCE_ENROLL_MAX  1000
#define EYE_DISTANCE_ENROLL_MIN  400

#endif // 0




#define LEFT_IRIS   0
#define RIGHT_IRIS  1
#define LEFT_RIGHT_IRIS  3


#define LEFT_MASK   2
#define RIGHT_MASK  3

#define MAX_USER_ID 64




#define GRAY_IMG_SIZE WIDTH*HEIGHT
#define RGB_IMG_SIZE  WIDTH*HEIGHT*3
#define YUV_IMG_SIZE  WIDTH*HEIGHT*3



using namespace std;



class CapTh: public QThread
{
    Q_OBJECT
public:
    CapTh();
    bool IsCapture;

    void run();
    unsigned char *buf;
    unsigned char *tmp;
    unsigned char *rgb;
    unsigned char *gray;
    string dev;
    int camera_id;

    //V4l2 cap;

signals:
      void UpdateRgbImgData(unsigned char*,int);

public slots:
      void StreamCtr(bool key);
};

#endif // CAPTH_H
