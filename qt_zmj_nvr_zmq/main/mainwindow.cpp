#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QDesktopWidget>
#include "hisi_vdec.h"
#include "rtspclient.h"
#include "zmj_nvr_cfg.h"
#include "nvrSysConfigMgr.h"

#include <QProcess>
#include <QMapIterator>


#include <QDir>
#include <QtPlugin>
#include <QPluginLoader>
#include "../PluginApp/Main/interfaceplugin.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint);
    ui->statusBar->setVisible(false);
    this->resize(QApplication::desktop()->width(),QApplication::desktop()->height());

    //隱藏鼠標
  //  QWSServer::setCursorVisible(false);

    setAutoFillBackground(false);  //这个不设置的话就背景变黑
    setWindowFlags(Qt::FramelessWindowHint);
    setAttribute(Qt::WA_TranslucentBackground,true);
    //QWSServer::setBackground(QColor(0, 0, 0, 0)) ;
    qDebug()<<QApplication::desktop()->width() <<QApplication::desktop()->height() ;

    //设置标签位置和大小
    int mainWidth = QApplication::desktop()->width();
    ui->label_logo->resize(ZMJ_LOGO_BAR_WITCH,ZMJ_LOGO_BAR_HIGHT);
    ui->label_title->resize((mainWidth-ZMJ_LOGO_BAR_WITCH)/2,ZMJ_LOGO_BAR_HIGHT);
    ui->label_time->resize((mainWidth-ZMJ_LOGO_BAR_WITCH)/2,ZMJ_LOGO_BAR_HIGHT);
    ui->label_logo->move(0,0);
    ui->label_title->move(ZMJ_LOGO_BAR_WITCH,0);
    ui->label_time->move(ZMJ_LOGO_BAR_WITCH+(mainWidth-ZMJ_LOGO_BAR_WITCH)/2,0);

    //时钟的初始化
    timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(showTime()));
    timer->start(1000);


    //重连机制定线程
    streamDaemon.start();

    //设置标签字符的颜色
    QFont font ( "Microsoft YaHei", 30, 75); //第一个属性是字体（微软雅黑），第二个是大小，第三个是加粗（权重是75）

    QPalette pe;
    pe.setColor(QPalette::WindowText,Qt::white);
    ui->label_time->setPalette(pe);
    ui->label_title->setPalette(pe);
    ui->label_time->setFont(font);
    ui->label_title->setFont(font);

    pe.setColor(QPalette::WindowText,Qt::red);
    //ui->label->setPalette(pe);

    //设置标签字符的位置
    ui->label_time->setAlignment(Qt::AlignRight|Qt::AlignVCenter);
    ui->label_logo->setAlignment(Qt::AlignHCenter|Qt::AlignVCenter);
    ui->label_title->setAlignment(Qt::AlignLeft|Qt::AlignVCenter);

    rect_list.clear();
    timer_map.clear();

    for(int i=0;i<ZMJ_VDEC_MAX_CHN;i++){
         QLabel *label =  new QLabel(this);
         QPalette pe;
         pe.setColor(QPalette::WindowText,Qt::red);
         label->setPalette(pe);

         pe.setColor(QPalette::Background, Qt::white);
        // label->setAutoFillBackground(true);
         label->setPalette(pe);

         qLabel_map.insert(i,label);
         label->close();
    }



    QDir path = QDir(qApp->applicationDirPath());
    path.cd("./plugins");
    foreach (QFileInfo info, path.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
    {
            QPluginLoader pluginLoader(info.absoluteFilePath());
             qDebug()<<"11122222"<<info.absoluteFilePath();
            QObject *plugin = pluginLoader.instance();
            QString errorStr = pluginLoader.errorString();
             qDebug()<<errorStr;
             if(!pluginLoader.isLoaded())
                 qDebug() <<"*******************123"<< pluginLoader.errorString();
            if (plugin)
            {

                qDebug()<<"************111plugin22222"<<info.absoluteFilePath();
                InterfacePlugin *app = qobject_cast<InterfacePlugin*>(plugin);
                if (app)
                {
                    app->output("i am a plugin");
                }
            }
    }

}


MainWindow::~MainWindow()
{
    for(int i=0;i<ZMJ_VDEC_MAX_CHN;i++){
         delete qLabel_map[i];
    }
    delete ui;
}

void MainWindow::setTitleName(QString name)
{
    qDebug()<<name;
    ui->label_title->setText(name);
}

void MainWindow::setTitleCharName(char *name)
{
    std::string str = name;
   // printf("--------%d-----%s\n",frame_info.len,frame_info.data);
    QString titleName = QString::fromStdString(str);
    setTitleName(titleName);
}

void MainWindow::setChnLine(unsigned char  mode)
{
    int x0 = 0,y0 = 0,x1 = 0,y1 = 0,w = 0,h = 0;
    int u32Height = ZMJ_HDMI_HIGHT-ZMJ_LOGO_BAR_HIGHT;
    int u32Width = ZMJ_HDMI_WIDTCH;
    int i = 0;
    int u32Square = 0;
    QLine line;
    QRect rect;
    qDebug()<<"------------"<<mode;
    rect_list.clear();
    switch (mode)
    {
        case VO_MODE_1MUX:

            break;
        case VO_MODE_4MUX:
            u32Square = 2;

            for(i=0;i<mode;i++){
                x0       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
                w   = ALIGN_BACK(u32Width/u32Square, 2);
                h  = ALIGN_BACK(u32Height/u32Square, 2);
                rect.setRect(x0,y0,w,h);
                rect_list.append(rect);
            }
            break;
        case VO_MODE_6MUX:
        u32Square = 3;
        for (i=0; i<mode; i++)
        {
            if(i == 0){
                x0       = ALIGN_BACK((u32Width/u32Square) * (0%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (0/u32Square), 2);
                w   = ALIGN_BACK(2*u32Width/u32Square, 2);
                h  = ALIGN_BACK(2*u32Height/u32Square, 2);
            }else if(i == 1){
                x0       = ALIGN_BACK((u32Width/u32Square) * ((i+1)%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * ((i+1)/u32Square), 2);
                w   = ALIGN_BACK(u32Width/u32Square, 2);
                h  = ALIGN_BACK(u32Height/u32Square, 2);
            }else{
                x0       = ALIGN_BACK((u32Width/u32Square) * ((i+3)%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * ((i+3)/u32Square), 2);
                w   = ALIGN_BACK(u32Width/u32Square, 2);
                h  = ALIGN_BACK(u32Height/u32Square, 2);
            }
            rect.setRect(x0,y0,w,h);
            rect_list.append(rect);
        }

            break;
        case VO_MODE_9MUX:
        u32Square = 3;

        for(i=0;i<mode;i++){
            x0       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
            y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
            w   = ALIGN_BACK(u32Width/u32Square, 2);
            h  = ALIGN_BACK(u32Height/u32Square, 2);
            rect.setRect(x0,y0,w,h);
            rect_list.append(rect);
        }

            break;
        case VO_MODE_16MUX:
        u32Square = 4;

        for(i=0;i<mode;i++){
            x0       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
            y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
            w   = ALIGN_BACK(u32Width/u32Square, 2);
            h  = ALIGN_BACK(u32Height/u32Square, 2);
            rect.setRect(x0,y0,w,h);
            rect_list.append(rect);
        }

            break;
        default:
            ;
    }
    update();
}


void MainWindow::setChnLabelText()
{
    int x0 = 0,y0 = 0,w = 0,h = 0;
    int u32Height = ZMJ_HDMI_HIGHT-ZMJ_LOGO_BAR_HIGHT;
    int u32Width = ZMJ_HDMI_WIDTCH;
    int i = 0;
    int u32Square = 0;

   // return;

    qDebug()<<"-----setChnLabelText------------------------------------------";
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getCacheNvrInfo();
    int mode = pNvrInfo->cmaraChnNum;
    setChnLine(mode);

    int count = 0;
    foreach (QLabel* label, qLabel_map){
        // label->setText("无配置1111111111");
        label->hide();
        QString str = QString("无配置 Channel:%1").arg(count);
        label->setText(str);
        count++;
        qDebug()<<"--------------------------------------------------";
    }
    if(0 == pNvrInfo->isLabelShow){
        return;
    }
    for(int i = 0;i<pNvrInfo->cameraNum;i++){
        NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+i;
        int chn = pCamera->cmaraChnID;
        if(chn < ZMJ_VDEC_MAX_CHN){
            QString str = QString("%1 (IP:%2 Channel:%3)").arg((char*)pCamera->sCameraName).arg(pCamera->struIP.sIpV4).arg(pCamera->cmaraChnID);
            qLabel_map[chn]->setText(str);

        }   
    }


         // cout << value << endl;

    switch (mode)
    {
        case VO_MODE_1MUX:

            break;
        case VO_MODE_4MUX:
            u32Square = 2;

            for(i=0;i<mode;i++){
                x0       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
                w   = ALIGN_BACK(u32Width/u32Square, 2);
                h  = ALIGN_BACK(u32Height/u32Square, 2);
                QLabel *label = qLabel_map[i];
                label->setVisible(true);
                label->resize(w-10,ZMJ_LOGO_BAR_HIGHT);
//                qDebug()<<"label---------------"<<x0<<y0<<x0+5<<y0+h-ZMJ_LOGO_BAR_HIGHT-10;
                label->move(x0+5,y0+h-ZMJ_LOGO_BAR_HIGHT-10);
//
            }
            break;
        case VO_MODE_6MUX:
        u32Square = 3;
        for (i=0; i<mode; i++)
        {
            if(i == 0){
                x0       = ALIGN_BACK((u32Width/u32Square) * (0%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (0/u32Square), 2);
                w   = ALIGN_BACK(2*u32Width/u32Square, 2);
                h  = ALIGN_BACK(2*u32Height/u32Square, 2);
            }else if(i == 1){
                x0       = ALIGN_BACK((u32Width/u32Square) * ((i+1)%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * ((i+1)/u32Square), 2);
                w   = ALIGN_BACK(u32Width/u32Square, 2);
                h  = ALIGN_BACK(u32Height/u32Square, 2);
            }else{
                x0       = ALIGN_BACK((u32Width/u32Square) * ((i+3)%u32Square), 2);
                y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * ((i+3)/u32Square), 2);
                w   = ALIGN_BACK(u32Width/u32Square, 2);
                h  = ALIGN_BACK(u32Height/u32Square, 2);
            }
             qDebug()<<"label---------------"<<x0<<y0;
             QLabel *label = qLabel_map[i];
             label->resize(w-10,ZMJ_LOGO_BAR_HIGHT);
             qDebug()<<"label---------------"<<x0<<y0<<x0+5<<y0+h-ZMJ_LOGO_BAR_HIGHT-10;
             label->move(x0+5,y0+h-ZMJ_LOGO_BAR_HIGHT-10);
             label->setVisible(true);
        }

            break;
        case VO_MODE_9MUX:
        u32Square = 3;

        for(i=0;i<mode;i++){
            x0       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
            y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
            w   = ALIGN_BACK(u32Width/u32Square, 2);
            h  = ALIGN_BACK(u32Height/u32Square, 2);

            QLabel *label = qLabel_map[i];
            label->resize(w-10,ZMJ_LOGO_BAR_HIGHT);
            qDebug()<<"label---------------"<<x0<<y0<<x0+5<<y0+h-ZMJ_LOGO_BAR_HIGHT-10;
            label->move(x0+5,y0+h-ZMJ_LOGO_BAR_HIGHT-10);
            label->setVisible(true);
        }

            break;
        case VO_MODE_16MUX:
        u32Square = 4;

        for(i=0;i<mode;i++){
            x0       = ALIGN_BACK((u32Width/u32Square) * (i%u32Square), 2);
            y0       = ZMJ_LOGO_BAR_HIGHT+ALIGN_BACK((u32Height/u32Square) * (i/u32Square), 2);
            w   = ALIGN_BACK(u32Width/u32Square, 2);
            h  = ALIGN_BACK(u32Height/u32Square, 2);
            qDebug()<<"label----VO_MODE_16MUX-----------"<<x0<<y0;
            QLabel *label = qLabel_map[i];
            label->resize(w-10,ZMJ_LOGO_BAR_HIGHT);
            qDebug()<<"label---------------"<<x0<<y0<<x0+5<<y0+h-ZMJ_LOGO_BAR_HIGHT-10;
            label->move(x0+5,y0+h-ZMJ_LOGO_BAR_HIGHT-10);
            label->setVisible(true);
        }

            break;
        default:
            ;
    }
}




void MainWindow::paintEvent(QPaintEvent *)
{
    QPainter painter(this);
    painter.setPen(QPen(Qt::gray,2));//设置画笔形式
    painter.drawRect(0,ZMJ_LOGO_BAR_HIGHT,ZMJ_HDMI_WIDTCH,(ZMJ_HDMI_HIGHT-ZMJ_LOGO_BAR_HIGHT));//画矩形
     painter.drawRect(0,0,ZMJ_HDMI_WIDTCH,ZMJ_HDMI_HIGHT);//画矩形
    int i = 0;
    while(rect_list.size()!=0 && i != rect_list.size()){
          painter.drawRect(rect_list.at(i));
          i++;
      }
}


void MainWindow::showTime()
{
    //QTime time = QTime::currentTime();
    QDateTime time =QDateTime::currentDateTime();
    QString text = time.toString("yyyy-MM-dd hh:mm:ss ");

    ui->label_time->setText(text);
    // qDebug()<<text;
}


//open
void MainWindow::on_pushButton_4_clicked()
{
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getCacheNvrInfo();
     NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+0;
     RTSPClientStopOneCamera(pCamera);
     qDebug()<<"close";
}

//close
void MainWindow::on_pushButton_5_clicked()
{
    NvrSysConfigMgr *pNvrSysConfigMgr = NvrSysConfigMgr::Inst();
    NET_DVR_IPDEVINFO *pNvrInfo = pNvrSysConfigMgr->getCacheNvrInfo();
     NET_DVR_CAMERAINFO *pCamera = pNvrInfo->cameraList+0;
     RTSPClientRestartOneCamera(pCamera);
     qDebug()<<"open";
}
