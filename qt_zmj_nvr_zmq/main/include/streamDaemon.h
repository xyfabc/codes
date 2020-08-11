#ifndef STREAMDAEMON_H
#define STREAMDAEMON_H

#include <QThread>
#include "pinger.h"



class StreamDaemon : public QThread
{
public:
    StreamDaemon();

private:
    void restartStream();
    bool ping_ip(QString ip);
    bool pingIP(const char *ips, int secs);

protected:
    void run();

private:
    Pinger pinger;
};

#endif // STREAMDAEMON_H
