#include "mainwindow.h"
#include <QApplication>
#include <QTextCodec> // 编码头文件


int main(int argc, char *argv[])
{
      QApplication a(argc, argv);
    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    if(codec && codec!= NULL)
    {
        QTextCodec::setCodecForLocale(codec);
        QTextCodec::setCodecForCStrings(codec);
        QTextCodec::setCodecForTr(codec);
    }


    MainWindow w;
    w.InitWin();
    w.updatePara();
    w.show();

    return a.exec();
}
