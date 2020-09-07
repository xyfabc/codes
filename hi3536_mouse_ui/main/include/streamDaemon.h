#ifndef STREAMDAEMON_H
#define STREAMDAEMON_H

#include <QThread>
#include "pinger.h"
#include "hisi_vdec.h"


class MainWindow;

class StreamDaemon : public QThread
{
    Q_OBJECT

public:
    StreamDaemon();
    void setMainwindow(MainWindow *w);
    void setChnId(int chnId);

private:
    void restartStream();
    bool ping_ip(QString ip);
    bool pingIP(const char *ips, int secs);
    bool CapturePicture(int cameraId);
    bool CapturePictureAndSave(const QString& filename);
    bool CaptureRGBPicture(const QString& filename);
    bool getYuv();
    bool getAllCameraYuv();

protected:
    void run();

private:
    Pinger pinger;
    unsigned char* pbuf;
    int flag;
    int chnId;
    int m_frame;
    qint64 m_firstFrameTick;
    qint64 m_lastPrintFps;

signals:
    void UpdateImgData(unsigned char*, int width, int height, int* flag,int id);
};

#endif // STREAMDAEMON_H
