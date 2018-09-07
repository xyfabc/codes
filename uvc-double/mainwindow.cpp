#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    rgb = new unsigned char[RGB_IMG_SIZE];

    pCapTh_0 = new CapTh();
    //pCapTh_1 = new CapTh(1);

    connect(pCapTh_0, SIGNAL(UpdateRgbImgData(unsigned char*,int)), this, SLOT(upImage(unsigned char *,int)), Qt::QueuedConnection);
   // connect(pCapTh_1, SIGNAL(UpdateRgbImgData(unsigned char*,int)), this, SLOT(upImage(unsigned char *,int)), Qt::QueuedConnection);

    pCapTh_0->start();
   // pCapTh_1->start();

    ui->pushButton->setVisible(false);
    ui->pushButton_2->setVisible(false);


}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::upImage(unsigned char *buf, int id)
{

    memcpy(rgb,buf,RGB_IMG_SIZE);
    if(0 == id){
        QImage image(rgb,WIDTH,HEIGHT,QImage::Format_RGB888);
        QPixmap pixmap = QPixmap::fromImage(image);
        QPixmap map = pixmap.scaled(640, 480);
        ui->label_0->setPixmap(map);
    }else if(1 == id){
        QImage image(rgb,WIDTH,HEIGHT,QImage::Format_RGB888);
        QPixmap pixmap = QPixmap::fromImage(image);
        QPixmap map = pixmap.scaled(640, 480);
        ui->label_1->setPixmap(map);
    }
}

void MainWindow::on_pushButton_clicked()
{
    static int click_count0=0;
    if(0==click_count0%2){
        pCapTh_0->StreamCtr(true);
        ui->pushButton->setText("stop_camera");
    }else{
        pCapTh_0->StreamCtr(false);
        ui->pushButton->setText("start_camera");
    }
    click_count0++;
}

void MainWindow::on_pushButton_2_clicked()
{
    static int click_count1=0;
    if(0==click_count1%2){
        pCapTh_1->StreamCtr(true);
        ui->pushButton_2->setText("stop_camera");
    }else{
        pCapTh_1->StreamCtr(false);
        ui->pushButton_2->setText("start_camera");
    }
    click_count1++;
}
