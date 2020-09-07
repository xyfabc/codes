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
//#include<QWSServer>

#include <QDir>
#include <QtPlugin>
#include <QPluginLoader>
#include "../PluginApp/Main/interfaceplugin.h"
#include "widget.h"
#include <QGridLayout>
#include "tools/TcYuvX.h"
#include "tools/write_bmp_func.h"
#include "hi_comm_ive.h"
#include "hi_ive.h"
#include "mpi_ive.h"



MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setWindowFlags(Qt::CustomizeWindowHint);
    //ui->statusBar->setVisible(false);
    this->resize(QApplication::desktop()->width(),QApplication::desktop()->height());
    
    //隱藏鼠標
    //QWSServer::setCursorVisible(false);
    
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
    // streamDaemon.start();
    
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
    
    
    
    //    QDir path = QDir(qApp->applicationDirPath());
    //    path.cd("./plugins");
    //    foreach (QFileInfo info, path.entryInfoList(QDir::Files | QDir::NoDotAndDotDot))
    //    {
    //            QPluginLoader pluginLoader(info.absoluteFilePath());
    //             qDebug()<<"11122222"<<info.absoluteFilePath();
    //            QObject *plugin = pluginLoader.instance();
    //            QString errorStr = pluginLoader.errorString();
    //             qDebug()<<errorStr;
    //             if(!pluginLoader.isLoaded())
    //                 qDebug() <<"*******************123"<< pluginLoader.errorString();
    //            if (plugin)
    //            {
    
    //                qDebug()<<"************111plugin22222"<<info.absoluteFilePath();
    //                InterfacePlugin *app = qobject_cast<InterfacePlugin*>(plugin);
    //                if (app)
    //                {
    //                    app->output("i am a plugin");
    //                }
    //            }
    //    }

#if 0
    int NumRows = 4;
    int NumColumns = 4;
    int count = 0;
    Widget *glWidgets[NumRows][NumColumns];
    QGridLayout *mainLayout = new QGridLayout;
    setLayout(mainLayout);
    for (int i = 0; i < NumRows; ++i) {
        for (int j = 0; j < NumColumns; ++j) {
            QColor clearColor;
            qDebug()<<"------count -----"<<count;
            clearColor.setHsv(((i * NumColumns) + j) * 255
                              / (NumRows * NumColumns - 1),
                              255, 63);

            glWidgets[i][j] = new Widget;
            glWidgets[i][j]->resize(1920/4, 1080/4);
            glWidgets[i][j]->show();
            mainLayout->addWidget(glWidgets[i][j], i, j);
            count++;
        }
    }
#endif
    // lcdForm.resize(800,600);
    // lcdForm.move(800,400);
    // lcdForm.show();
    //connect(&streamDaemon, SIGNAL(UpdateImgData(unsigned char*)), &lcdForm, SLOT(upImages(unsigned char *)), Qt::UniqueConnection);

    //connect(&streamDaemon, SIGNAL(UpdateImgData(unsigned char*, int , int , int* )), &qvideoWidget, SLOT(onRenderVideo(unsigned char* , int , int , int* )), Qt::UniqueConnection);
    //connect(&streamDaemon, SIGNAL(UpdateImgData(unsigned char*, int , int , int* )), &lcdForm, SLOT(onRenderVideo(unsigned char* , int , int , int* )), Qt::UniqueConnection);
    prgb_buf  =(unsigned char*)malloc(1920*1080*4);
    qvideoWidget.setVideoSize(1920, 1080);




    //    QGridLayout* gridLay = new QGridLayout;

    //    int index = 0;
    //    for (int i = 0; i < 1; i++)
    //    {
    //        for (int j = 0; j < 1; j++)
    //        {
    //            pLcdForm[index] = new LCDForm();
    //            pLcdForm[index]->setCameraId(index);
    //            pStreamDaemon[index] = new StreamDaemon();
    //            pStreamDaemon[index]->setChnId(index);
    //            connect(pStreamDaemon[index], SIGNAL(UpdateImgData(unsigned char*, int , int , int* )), \
    //                    pLcdForm[index], SLOT(onRenderVideo(unsigned char* , int , int , int* )), \
    //                    Qt::UniqueConnection);
    //            pStreamDaemon[index]->start();
    //            gridLay->addWidget(pLcdForm[index], i, j);
    //            index++;
    //        }
    //    }
    //    cameraWidget.setLayout(gridLay);
    cameraWidget.move(0,ZMJ_LOGO_BAR_HIGHT);
    cameraWidget.resize(1920, 500);
    streamDaemon.start();


    bottomWidget.move(0,1080-100);
    bottomWidget.resize(1920, 100);

  this->setCursor(QCursor(Qt::BlankCursor));

    // cameraWidget.show();
}


MainWindow::~MainWindow()
{
    for(int i=0;i<ZMJ_VDEC_MAX_CHN;i++){
        delete qLabel_map[i];
    }
    delete ui;
}
void MainWindow::mouseMoveEvent(QMouseEvent *event)
{
    ui->label->setText(QString("坐标：（%1,%2）").arg(event->pos().x()).arg(event->pos().y()));
    qDebug()<<event->pos().x()<<event->pos().y();
    //statusBar->addPermanentWidget(lable);
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
#define IMG_W 1920
#define IMG_H 1080
void MainWindow::updateImg(unsigned char *pbuf)
{
    unsigned char* rgb=(unsigned char*)malloc(1920*1080*4);
    // RgbFromYuv420SP(rgb,pbuf,1920, 1080,0);
    //    IVE_HANDLE pIveHandle;
    //    IVE_SRC_IMAGE_S pstSrc;
    //    IVE_DST_IMAGE_S pstDst;
    //    IVE_CSC_CTRL_S pstCscCtrl;
    
    //    HI_S32 ret =  HI_MPI_IVE_CSC(&pIveHandle, &pstSrc,&pstDst, &pstCscCtrl, HI_TRUE);
    
    //    char file_name[64] ={0};
    
    //    struct tm *t;
    //    time_t tt;
    //    time(&tt);
    //    t = localtime(&tt);
    
    //    sprintf(file_name,"./img/test--%02d_%02d_%02d_%02d.bmp",t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
    //    printf("\n----------------file_name= %s\n",file_name);
    
    // save_rgb_bmp((uint8_t*)rgb,  IMG_W,IMG_H, NULL, file_name);
    
    
    QImage image(pbuf,IMG_W,1080,QImage::Format_RGB888);
    qDebug()<<"upImages-----1------";
    QPixmap pixmap = QPixmap::fromImage(image);
    qDebug()<<"upImages-----2------";
    QPixmap map = pixmap.scaled(640, 480);
    qDebug()<<"upImages-----3------";
    //ui->label->setPixmap(map);
    qDebug()<<"upImages----4-------";
    
    
    
    
    //QImage image(pbuf,640,480,QImage::Format_Indexed8);
    //    QImage image( file_name , 1920 , 1080 , QImage::Format_RGB888);
    //    QVector<QRgb> grayTable;
    
    //    QDateTime dateTime;
    //    QString strDateTime = dateTime.currentDateTime().toString("yyyy_MM_dd_hh_mm_ss_zzz");
    //    qDebug()<<strDateTime;
    //    QString filename = QString("%1.jpg").arg(strDateTime);
    //    qDebug()<<filename;
    
    //    unsigned int rgb=0;
    //    for(int i= 0; i< 256;i++)
    //    {
    //        grayTable.append(rgb);
    //        rgb+=0x010101;
    //    }
    //    image.setColorTable(grayTable);
    //        QPixmap pixmap2=QPixmap::fromImage(image);
    //            pixmap2=pixmap2.scaled(ui->label->size(), Qt::KeepAspectRatio);//自适应/等比例
    // ui->label->setPixmap(QPixmap::fromImage(image));
    free(rgb);
}


bool MainWindow::CapturePicture(const QString& filename)
{
    unsigned char* pbuf=(unsigned char*)malloc(1920*1080*4);
    if(pbuf)
    {
        if(GetDataForView(pbuf,0))
        {
            this->updateImg(pbuf);
            free(pbuf);
            return true;
        }
        free(pbuf);
    }
    return false;
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
    // CapturePicture("");
    // qDebug()<<text;
}



void MainWindow::updateSub()
{
    // glWidget->update();
}

void MainWindow::on_pushButton_4_clicked()
{
    //  CapturePicture("");

    //lcdForm.move(1000,500);
    //lcdForm.show();
    qvideoWidget.show();
    //bottomWidget.show();
    streamDaemon.start();

    qDebug()<<"qvideoWidget.show()-----------";
    //lcdForm.show();
    //    QImage *img=new QImage; //新建一个image对象
    //    img->load("./img/1test--01_00_01_00.jpg"); //将图像资源载入对象img，注意路径，可点进图片右键复制路径
    //    ui->label->setPixmap(QPixmap::fromImage(*img)); //将图片放入label，使用setPixmap,注意指针*img
}

void MainWindow::upImages(unsigned char *buf)
{
    qDebug()<<"upImages-----------";
    memcpy(prgb_buf,buf,1920*1080*3);
    QImage image(prgb_buf,IMG_W,1080,QImage::Format_RGB888);
    QPixmap pixmap = QPixmap::fromImage(image);
    QPixmap map = pixmap.scaled(860, 540);
    //ui->label->setPixmap(map);
}

void MainWindow::on_pushButton_5_clicked()
{
    lcdForm.move(1000,500);
    streamDaemon.start();
    lcdForm.show();
}

void MainWindow::on_pushButton_clicked()
{
    cameraWidget.show();
    bottomWidget.show();
    // streamDaemon.start();
}

void MainWindow::on_pushButton_6_clicked()
{
    qDebug()<<"on_pushButton_6_clicked-----------";
    QGridLayout* gridLay = new QGridLayout;
    int index = 0;
    for (int i = 0; i < 1; i++)
    {
        for (int j = 0; j < 1; j++)
        {
            //            pOpenGLWindow[index] = new OpenGLWindow();
            //            pStreamDaemon[index] = new StreamDaemon();
            //            pStreamDaemon[index]->setChnId(index);
            //            connect(pStreamDaemon[index], SIGNAL(UpdateImgData(unsigned char*, int , int , int* )), \
            //                    pOpenGLWindow[index], SLOT(onRenderVideo(unsigned char* , int , int , int* )), \
            //                    Qt::UniqueConnection);
            //            pStreamDaemon[index]->start();
            //            gridLay->addWidget(pOpenGLWindow[index], i, j);
            index++;
        }
    }
    // cameraWidget.setLayout(gridLay);
    // cameraWidget.show();
    //    pLcdForm[0]->show();
    //    pStreamDaemon[0]->start();
}

void MainWindow::on_pushButton_7_clicked()
{
    qDebug()<<"on_pushButton_7_clicked-----------";

#if 0
    QGridLayout* gridLay = new QGridLayout;
    int index = 0;
    for (int i = 0; i < 1; i++)
    {
        for (int j = 0; j < 1; j++)
        {
            pOpenGLWindow[index] = new OpenGLWindow();
            pStreamDaemon[index] = new StreamDaemon();
            pStreamDaemon[index]->setChnId(index);
            connect(pStreamDaemon[index], SIGNAL(UpdateImgData(unsigned char*, int , int , int* )), \
                    pOpenGLWindow[index], SLOT(onRenderVideo(unsigned char* , int , int , int* )), \
                    Qt::UniqueConnection);
            pStreamDaemon[index]->start();
            gridLay->addWidget(pOpenGLWindow[index], i, j);
            index++;
        }
    }
    cameraWidget.setLayout(gridLay);
    cameraWidget.show();
#else
    QGridLayout* gridLay = new QGridLayout;
    int index = 0;
    pStreamDaemon[0] = new StreamDaemon();
    pStreamDaemon[0]->setChnId(0);
    for (int i = 0; i < 1; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            pOpenGLWindow[index] = new OpenGLWindow();
            pOpenGLWindow[index]->setCameraId(index);
            connect(pStreamDaemon[0], SIGNAL(UpdateImgData(unsigned char*, int , int , int* ,int)), \
                    pOpenGLWindow[index], SLOT(onRenderVideo(unsigned char* , int , int , int* ,int)), \
                    Qt::UniqueConnection);
            gridLay->addWidget(pOpenGLWindow[index], i, j);
            index++;
        }
    }
    pStreamDaemon[0]->start();
    cameraWidget.setLayout(gridLay);
    cameraWidget.show();
#endif
}

void MainWindow::on_pushButton_8_clicked()
{
    pLcdForm[2]->show();
    pStreamDaemon[2]->start();
}

void MainWindow::on_pushButton_9_clicked()
{
    pLcdForm[3]->show();
    pStreamDaemon[3]->start();
}
