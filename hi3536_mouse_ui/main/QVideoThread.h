#ifndef QVIDEOTHREAD_H
#define QVIDEOTHREAD_H

#include <QObject>
#include <QThread>

class QVideoThread : public QThread
{
    Q_OBJECT
public:
    explicit QVideoThread(QObject *parent = nullptr);

public:
signals:
    void renderVideo(unsigned char* buf, int width, int height, int* flag);

protected:
    virtual void run();

signals:

public slots:
};

#endif // QVIDEOTHREAD_H
