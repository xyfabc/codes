#include <iostream>
#include <string>
#include <stdlib.h>

#include "v4l2.h"


using namespace std;

//int main(int argc, char *argv[])
//{
//    auto dev = string("/dev/video0");
//    if(argc >= 2) {
//        dev = string(argv[1]);
//    }

//    V4l2 cap(dev);
//    if(!cap.isValid()) {
//        cerr << "Device: " << dev << " is not valid!!!" << endl;
//        exit(1);
//    }
//    cap.setPixelFormat(1920, 1080, V4L2_PIX_FMT_MJPEG, true);

//    for (int i = 0; i < 10; ++i) {
//        cout << "Testing " << i << endl;
//        cap.startStreaming();
//        for(int j = 0; j < 20; ++j) {
//            Image img = cap.Grap(2000);
//            if(img.isValid()) {
//                cout << "IMG: " << j << ": " << img.length() << endl;
//            }
//        }
//        cap.stopStreaming();
//    }
//    return 0;

//}
