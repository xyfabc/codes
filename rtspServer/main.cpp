/*#include "serverwidget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ServerWidget w;
    w.show();

    return a.exec();
}
*/
#include "Workthread.h"
int main()
{

    WorkThread *work=new WorkThread(9000);
    pthread_t tidRTSPServer;
    pthread_create(&tidRTSPServer,NULL,WorkThread::workthreadRTSPServer,work);




    pthread_t tid;

    pthread_create(&tid,NULL,WorkThread::workthread,work);


    pthread_join(tid,NULL);
    pthread_join(tidRTSPServer,NULL);

    return 0;
}


