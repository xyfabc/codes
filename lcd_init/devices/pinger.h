#ifndef PINGER_H
#define PINGER_H

#include <QString>

class Pinger
{
public:
    Pinger();
    bool ping(QString ip);
    int ping_ip( const char *ips, int secs);
};


#endif // PINGER_H
