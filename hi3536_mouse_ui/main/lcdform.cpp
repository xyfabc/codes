#include "lcdform.h"
#include "ui_lcdform.h"
#include <QDebug>

LCDForm::LCDForm(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::LCDForm)
{
    ui->setupUi(this);
      prgb_buf  =(unsigned char*)malloc(1920*1080*4);
}

LCDForm::~LCDForm()
{
    delete ui;
    delete prgb_buf;
}



#include <QImage>
#include <QRgb>
#include <QFileInfo>
#include <QDir>
#include "tools/TcYuvX.h"
#include "tools/write_bmp_func.h"
#include <QDateTime>

#define IMG_W 1920
#define IMG_H 1080

bool LCDForm::CapturePicture(const QString& filename)
{
    unsigned char* pbuf=(unsigned char*)malloc(1920*1080*4);
    if(pbuf)
    {
        if(GetDataForView(pbuf,0))
        {

            char file_name[64] ={0};

            struct tm *t;
            time_t tt;
            time(&tt);
            t = localtime(&tt);

            sprintf(file_name,"./img/test--%02d_%02d_%02d_%02d.bmp",t->tm_mday, t->tm_hour, t->tm_min, t->tm_sec);
            // save_rgb_bmp((uint8_t*)pbuf,  IMG_W,IMG_H, NULL, file_name);

            QImage image(pbuf,IMG_W,1080,QImage::Format_RGB888);
            QPixmap pixmap = QPixmap::fromImage(image);
            QPixmap map = pixmap.scaled(860, 540);
            ui->label->setPixmap(map);

            free(pbuf);
            return true;
        }
        free(pbuf);
    }
    return false;
}

void LCDForm::setCameraId(int id)
{
    cameraId = id;
}
void LCDForm::run()
{
    int i=0;
    // sleep(10);
    while(1){
        // sleep(1);
        //restartStream();
        if(i==1000){
            // break;
        }
        QDateTime dateTime;
        QString strDateTime = dateTime.currentDateTime().toString("yyyy_MM_dd_hh_mm_ss_zzz");
        qDebug()<<strDateTime;
        QString filename = QString("%1.jpg").arg(strDateTime);
        qDebug()<<filename;
        //            image.save(filename,"JPG");
        CapturePicture(filename);
        i++;
    }
}

void LCDForm::on_pushButton_clicked()
{
    this->close();
}

//void LCDForm::upImages(unsigned char *buf)
void LCDForm::onRenderVideo(unsigned char *buf, int width, int height, int* flag)
{
    qDebug()<<"upImages-----------"<<cameraId;
       memcpy(prgb_buf,buf,1920*1080*3);
    QImage image(prgb_buf,1920,1080,QImage::Format_RGB888);
   // qDebug() << __FILE__ << __FUNCTION__ << __LINE__<<"@@@@@@@@@@@@@";
    QPixmap pixmap = QPixmap::fromImage(image);
  //  qDebug() << __FILE__ << __FUNCTION__ << __LINE__<<"@@@@@@@@@@@@@";
    QPixmap map = pixmap.scaled(640, 480);
  //  qDebug() << __FILE__ << __FUNCTION__ << __LINE__<<"@@@@@@@@@@@@@";
    ui->label->setPixmap(map);
   // qDebug() << __FILE__ << __FUNCTION__ << __LINE__<<"@@@@@@@@@@@@@";
    *flag += 1;
}
