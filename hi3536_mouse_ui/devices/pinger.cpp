#include "pinger.h"

#include <QString>
#include <QProcess>
#include <QDebug>

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>

#define PACKET_SIZE     4096
#define MAX_WAIT_TIME   5
#define MAX_NO_PACKETS  3


Pinger::Pinger()
{

}
/*校验和算法*/
unsigned short cal_chksum(unsigned short *addr,int len)
{       int nleft=len;
        int sum=0;
        unsigned short *w=addr;
        unsigned short answer=0;

/*把ICMP报头二进制数据以2字节为单位累加起来*/
        while(nleft>1)
        {       sum+=*w++;
                nleft-=2;
        }
        /*若ICMP报头为奇数个字节，会剩下最后一字节。把最后一个字节视为一个2字节数据的高字节，这个2字节数据的低字节为0，继续累加*/
        if( nleft==1)
        {       *(unsigned char *)(&answer)=*(unsigned char *)w;
                sum+=answer;
        }
        sum=(sum>>16)+(sum&0xffff);
        sum+=(sum>>16);
        answer=~sum;
        return answer;
}
int Pinger::ping_ip( const char *ips, int secs)
{
    struct timeval *tval;
    int maxfds = 0;
    fd_set readfds;

    struct sockaddr_in addr;
    struct sockaddr_in from;
    // 设定Ip信息
    bzero(&addr,sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(ips);

    struct timeval timeo;
    // 设定TimeOut时间
    timeo.tv_sec = secs;
    timeo.tv_usec = 0;


    char sendpacket[PACKET_SIZE];
    char recvpacket[PACKET_SIZE];
    // 设定Ping包
    memset(sendpacket, 0, sizeof(sendpacket));
    pid_t pid;
    // 取得PID，作为Ping的Sequence ID
    pid=getpid();

    struct ip *iph;
    struct icmp *icmp;


    icmp=(struct icmp*)sendpacket;
    icmp->icmp_type=ICMP_ECHO;  //回显请求
    icmp->icmp_code=0;
    icmp->icmp_cksum=0;
    icmp->icmp_seq=0;
    icmp->icmp_id=pid;
    tval= (struct timeval *)icmp->icmp_data;
    gettimeofday(tval,NULL);
    icmp->icmp_cksum=cal_chksum((unsigned short *)icmp,sizeof(struct icmp));  //校验

    int n;
    int retry=5;

    // 取得socket
    int mSocket = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
    if (setsockopt(mSocket, SOL_SOCKET, SO_SNDTIMEO, &timeo, sizeof(timeo)) == -1)
    {
        printf("#########setsocket error\n");
        //JYZ_ASSERT(0);
        close(mSocket);
        return 0;
    }
    printf("#####ping\n");
    // 发包
RETRY:
    n = sendto(mSocket, (char *)&sendpacket, sizeof(struct icmp), 0, (struct sockaddr *)&addr, sizeof(addr));
    if (n < 1)
    {
        printf("sendto %s ret:%d, error:%d\n",ips,n,errno);
        if(errno==EINTR)
        {
            retry--;
            if(retry)
                goto RETRY;
        }
        close(mSocket);
        return 0;
    }

    // 接受
    // 由于可能接受到其他Ping的应答消息，所以这里要用循环
    while(1)
    {
        // 设定TimeOut时间，这次才是真正起作用的
        FD_ZERO(&readfds);
        FD_SET(mSocket, &readfds);
        maxfds = mSocket + 1;
        n = select(maxfds, &readfds, NULL, NULL, &timeo);
        if (n <= 0)
        {
            printf("ip:%s,Time out error\n",ips);
            close(mSocket);
            return 0;
        }

        // 接受
        memset(recvpacket, 0, sizeof(recvpacket));
        int fromlen = sizeof(from);
        n = recvfrom(mSocket, recvpacket, sizeof(recvpacket), 0, (struct sockaddr *)&from, (socklen_t *)&fromlen);
        if (n < 1) {
            break;
        }


        char *from_ip = (char *)inet_ntoa(from.sin_addr);
            // 判断是否是自己Ping的回复
        if (strcmp(from_ip,ips) != 0)
        {
            printf("Ping %s RESP %s not the same\n",ips,from_ip);
            close(mSocket);
            return 0;
        }

        iph = (struct ip *)recvpacket;

        icmp=(struct icmp *)(recvpacket + (iph->ip_hl<<2));

        //printf("ip:%s\n,icmp->icmp_type:%d\n,icmp->icmp_id:%d\n",ips,icmp->icmp_type,icmp->icmp_id);
        // 判断Ping回复包的状态
        if (icmp->icmp_type == ICMP_ECHOREPLY && icmp->icmp_id == pid)   //ICMP_ECHOREPLY回显应答
        {
            // 正常就退出循环
            printf("Ping Success\n");
            close(mSocket);
            return 1;
        }
        else
        {
            // 否则继续等
            continue;
        }
    }
    close(mSocket);
    return 0;
}
bool Pinger::ping(QString ip)
{
    int exitCode;

    QString strArg = "ping -s 1 -c 1 " + ip;        //linux平台下的格式

    exitCode = QProcess::execute(strArg);
    if(0 == exitCode){
            //it's alive
           qDebug() << "shell ping " + ip + " sucessed!";
           return true;
     }else{
            qDebug() << "shell ping " + ip + " failed!";
            return false;
    }

}

