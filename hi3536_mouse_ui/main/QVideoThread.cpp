#include "QVideoThread.h"
#include <QDebug>
QVideoThread::QVideoThread(QObject *parent) : QThread (parent)
{

}

void QVideoThread::run()
{
    unsigned char* buf = new unsigned char[1920*1080 * 3 / 2];
    memset(buf, 1, 1920*1080 * 3 / 2);

    int count = 0;
    int flag = 1;
    while (1)
    {
        while (flag != 1)
        {
            QThread::msleep(1);
        }

        flag = 0;

        if (count % 4 == 0)
        {
            memset(buf, 1, 1920*1080 * 3 / 2);
            emit renderVideo(buf, 1920, 1080, &flag);
        }
        else if (count % 4 == 1)
        {
            memset(buf, 128, 1920*1080 * 3 / 2);
            emit renderVideo(buf, 1920, 1080, &flag);
        }
        else if (count % 4 == 2)
        {
            memset(buf, 200, 1920*1080 * 3 / 2);
            emit renderVideo(buf, 1920, 1080, &flag);
        }
        else if (count % 4 == 3)
        {
            memset(buf, 66, 1920*1080 * 3 / 2);
            emit renderVideo(buf, 1920, 1080, &flag);
        }

        QThread::msleep(10);
        count++;
    }
}
