#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QDebug>
#include <QFileDialog>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    logo_img=new QImage;
    m_pTimer = new QTimer(this);
    connect(m_pTimer, SIGNAL(timeout()), this, SLOT(handleTimeout()));
    edit_count = 0;
}

MainWindow::~MainWindow()
{
    delete logo_img;
    delete ui;
}

int MainWindow::revert_image(uchar *dst, uchar *src, int img_h, int img_w)
{
    int index = 0;
    for(int w=0;w<img_w;w++)
      for(int h=0;h<img_h;h++){
          dst[index]    = src[(h*img_w+w)*4];
          dst[index+1]  = src[(h*img_w+w)*4+1];
          dst[index+2]  = src[(h*img_w+w)*4+2];
          index += 3;
      }
    return img_h*img_w*3;
}

void MainWindow::build_movs(QString filename,uchar *dst,int img_h, int img_w)
{
    uchar img_buf[IMG_H*IMG_W*4] = {0};
    QImage* img=new QImage;
    qDebug()<<"11111:";
    if(! ( img->load(filename) ) ){ //加载图像
        delete img;
        return;
    }

    memcpy(img_buf,img->bits(),img->byteCount());
    qDebug()<<"image len:"<<img->byteCount();
    revert_image(dst,img_buf,img_h,img_w);
}

void MainWindow::save_file(QString outFileName, uchar *buf, int len)
{
    QFile file(outFileName);

    //判断文件是否存在
    if(file.exists()){
        qDebug()<<"file exists";
    }else{
        qDebug()<<"file no exist";
    }
    //已读写方式打开文件，
    //如果文件不存在会自动创建文件
    if(!file.open(QIODevice::ReadWrite)){
        qDebug()<<"open err";
    }else{
        qDebug()<<"open ok";
    }

    //获得文件大小
    //qint64 pos;
   // pos = file.size();
    //重新定位文件输入位置，这里是定位到文件尾端
   // file.seek(pos);

   // QByteArray array((char*)img_buf,307200);
    QByteArray array( (char *)buf,len);
    //写入文件
    qint64 length = -1;
    length = file.write(array,len);


    if(length == -1){
        qDebug()<<"write err";
    }else{
        qDebug()<<"write ok";
    }
    //关闭文件
    file.close();
}

void MainWindow::updateListview()
{

    ui->listWidget->clear();
    QString name;
    logo_obj.move_count = 0;
    image_list.clear();
    foreach (name, le_list) {
        ui->listWidget->addItem(name);
        logo_obj.move_count++;
        QImage img;
        if(! ( img.load(name) ) ) //加载图像
        {
        }
        image_list.append(img);
    }
}

void MainWindow::createBin()
{

}

void MainWindow::savePara()
{
   logo_obj.le_img_h =  ui->h_len_lineEdit_4->text().toInt();
   logo_obj.le_img_w =  ui->w_len_lineEdit_3->text().toInt();
   logo_obj.x_pos = ui->x_pos_lineEdit->text().toInt();
   logo_obj.y_pos = ui->y_pos_lineEdit_2->text().toInt();
   logo_obj.move_time = ui->time_lineEdit_5->text().toInt();
}

void MainWindow::updatePara()
{
    ui->h_len_lineEdit_4->setText(QString("%1").arg(logo_obj.le_img_h));
    ui->w_len_lineEdit_3->setText(QString("%1").arg(logo_obj.le_img_w));
    ui->x_pos_lineEdit->setText(QString("%1").arg(logo_obj.x_pos));
    ui->y_pos_lineEdit_2->setText(QString("%1").arg(logo_obj.y_pos));
    ui->time_lineEdit_5->setText(QString("%1").arg(logo_obj.move_time));
    qDebug()<<"-----------------------------";
}

void MainWindow::InitWin()
{
    logo_obj.img_h = IMG_H;
    logo_obj.img_w = IMG_W;
    logo_obj.le_img_h = LE_IMG_H;
    logo_obj.le_img_w = LE_IMG_W;
    logo_obj.move_count = 0;
    logo_obj.move_time = 150;
    logo_obj.x_pos = 80;
    logo_obj.y_pos = 100;
}

/*******
* aimage big,bimage litte
*/
QImage MainWindow::mergeImages2Image(const QImage &aimage, const QImage &bimage, int x_pos, int y_pos, int x_len, int y_len)
{
    QImage image = aimage;
    int x=0;
    int y=0;
    for(y=0;y<y_len;y++)
        for(x=0;x<x_len;x++){
          image.setPixel(x+x_pos,y+y_pos,bimage.pixel(x,y));
        }
    return image;
}


void MainWindow::on_build_pushButton_clicked()
{
    if(logo_file_name == ""){
        QMessageBox::information(this,
                               tr("打开图像失败"),
                             tr("请选择背景图和动画!"));

        return ;

    }
    if(m_pTimer->isActive()){
        m_pTimer->stop();
        ui->build_pushButton->setText("开始演示");
    }else {
       m_pTimer->start(logo_obj.move_time);
       ui->build_pushButton->setText("停止演示");
    }

}

void MainWindow::on_pushButton_2_clicked()
{
    if(logo_file_name == ""){
        QMessageBox::information(this,
                               tr("生成失败"),
                             tr("请选择背景图和动画!"));

        return ;

    }
    int seek = 0;
    uchar img_buf[IMG_H*IMG_W*4] = {0};
    uchar img_revert[IMG_H*IMG_W*4] = {0};
    QImage* img=new QImage;

    uchar *all_data = new uchar[IMG_H*IMG_W*4*20];
//生成文件头
    memcpy(all_data+seek,(uchar *)&logo_obj,sizeof(struct LOGO_OBJ));
    qDebug()<<sizeof(struct LOGO_OBJ);
    seek += sizeof(struct LOGO_OBJ);

//生成背景图


    if(! ( img->load(logo_file_name) ) ) //加载图像
    {
        // QMessageBox::information(this,
        //                        tr("打开图像失败"),
        //                      tr("打开图像失败!"));
        delete img;
        return;
    }
    memcpy(img_buf,img->bits(),img->byteCount());
    qDebug()<<"logo image len:"<<img->byteCount();
    int size = revert_image(img_revert,img_buf,IMG_H,IMG_W);


    memcpy(all_data+seek,img_revert,size);
    seek += size;
//生成动画
    QString name;
    foreach (name, le_list) {
        if(! ( img->load(name) ) ) //加载图像
        {
            // QMessageBox::information(this,
            //                        tr("打开图像失败"),
            //                      tr("打开图像失败!"));
            delete img;
            return;
        }
        memcpy(img_buf,img->bits(),img->byteCount());
        qDebug()<<"move image len:"<<img->byteCount();
        int size = revert_image(img_revert,img_buf,LE_IMG_H,LE_IMG_W);


        memcpy(all_data+seek,img_revert,size);
        seek += size;

    }


//保存文件

    out_file_name = QFileDialog::getSaveFileName(this,
        tr("Save Bin"),
        "",
        tr("*.bin;;")); //选择路径
    if(out_file_name.isEmpty()){
        return;
    }else{

    }
    save_file(out_file_name,all_data,seek);
    qDebug()<<"all data len:"<<seek;

// recall res
     delete img;
     delete all_data;

     QMessageBox::information(this,
                              tr("生成成功"),
                             tr("生成Bin文件成功!"));

}


//加载动画
void MainWindow::on_pushButton_clicked()
{
    if(m_pTimer->isActive()){
            QMessageBox::information(this,
                                   tr("打开图像失败"),
                                 tr("请先停止动画!"));

            return ;

    }
    if(logo_file_name == ""){
        QMessageBox::information(this,
                               tr("打开图像失败"),
                             tr("请选择背景图!"));

        return ;

    }
    if(ui->listWidget->count()>13){
        QMessageBox::information(this,
                               tr("打开图像失败"),
                             tr("动画超过14张"));
        return;
    }

    QStringList filelist = QFileDialog::getOpenFileNames(this,
        tr("Load Image"),
        "",
        tr("*.bmp;;")); //选择路径
    if(filelist.isEmpty()){
        return;
    }
    else{
        //qDebug()<<filename;
     }
   QString  filename;
   QImage* img=new QImage;
    bool isLoad = false;
   foreach (filename, filelist) {
       qDebug()<<filename;
       bool isExsit = 0;
       QString tmp;
       foreach (tmp, le_list) {
           if(tmp == filename){
               QMessageBox::information(this,
                                      tr("打开图像失败"),
                                    tr("动画已存在"));
               isExsit = true;
               break;
           }
       }
       if(isExsit){
           continue;
       }else{
           isLoad = true;
       }

       if(! ( img->load(filename) ) ) //加载图像
       {
          // delete img;
           continue;
       }
       if(img->byteCount() != logo_obj.le_img_h*logo_obj.le_img_w*4)
       {
            QMessageBox::information(this,
                                   tr("打开图像失败"),
                                 tr("打开图像失败!大小错误"));
            continue;
       }
       le_list.append(filename);
   }
    if(isLoad){
        updateListview();
        QImage image = mergeImages2Image(*logo_img,*img,logo_obj.x_pos,logo_obj.y_pos,logo_obj.le_img_w,logo_obj.le_img_h);
        ui->label->setPixmap(QPixmap::fromImage(image));
    }
    delete img;
   // ui->listWidget->addItem(filename);
}

void MainWindow::on_listWidget_itemPressed(QListWidgetItem *item)
{
    QImage* img=new QImage;
    QString filename = item->text();
    if(! ( img->load(filename) ) ) //加载图像
    {
        delete img;
        return;
    }
   // ui->label_2->setPixmap(QPixmap::fromImage(*img));
    QImage image = mergeImages2Image(*logo_img,*img,logo_obj.x_pos,logo_obj.y_pos,logo_obj.le_img_w,logo_obj.le_img_h);
    ui->label->setPixmap(QPixmap::fromImage(image));
    delete img;
}
//加载背景图
void MainWindow::on_pushButton_3_clicked()
{
    if(m_pTimer->isActive()){
            QMessageBox::information(this,
                                   tr("打开图像失败"),
                                 tr("请先停止动画!"));

            return ;

    }
     logo_file_name = QFileDialog::getOpenFileName(this,
        tr("Load Image"),
        "",
        tr("*.bmp;;")); //选择路径
    if(logo_file_name.isEmpty()){
        return;
    }
    else{

        qDebug()<<logo_file_name;
    }
    if(! ( logo_img->load(logo_file_name) ) ) //加载图像
    {

        return;
    }
    if(logo_img->byteCount() != IMG_H*IMG_W*4)
    {
         QMessageBox::information(this,
                                tr("打开图像失败"),
                              tr("打开图像失败!"));
         return;
    }
    ui->label->setPixmap(QPixmap::fromImage(*logo_img));
}



void MainWindow::handleTimeout()
{
    static int cur_count = 0;
    cur_count = cur_count%logo_obj.move_count;
    QImage img_logo = mergeImages2Image(*logo_img,image_list[cur_count],logo_obj.x_pos,logo_obj.y_pos,logo_obj.le_img_w,logo_obj.le_img_h);
    ui->label->setPixmap(QPixmap::fromImage(img_logo));
    cur_count++;
}
//上移动画
void MainWindow::on_UPtoolButton_clicked()
{
    if(m_pTimer->isActive()){
            QMessageBox::information(this,
                                   tr("移动图像失败"),
                                 tr("请先停止动画!"));

            return ;

    }
  QString filename;

  int index = ui->listWidget->currentRow();
  if(index<1){
      return;
  }
  qDebug()<<index<<le_list.at(index)<<le_list.at(index-1);
  filename = le_list.at(index);
  le_list.replace(index,le_list.at(index-1));
  le_list.replace(index-1,filename);
  qDebug()<<index<<le_list.at(index)<<le_list.at(index-1);
  updateListview();
  ui->listWidget->setCurrentRow(index-1);
}

//下移动画
void MainWindow::on_DowntoolButton_clicked()
{
    if(m_pTimer->isActive()){
            QMessageBox::information(this,
                                   tr("移动图像失败"),
                                 tr("请先停止动画!"));

            return ;

    }
    QString filename;
    int index = ui->listWidget->currentRow();
    if((index+1)>(ui->listWidget->count()- 1)){
        return;
    }
    qDebug()<<index<<le_list.at(index)<<le_list.at(index+1);
    filename = le_list.at(index);
    le_list.replace(index,le_list.at(index+1));
    le_list.replace(index+1,filename);
    qDebug()<<index<<le_list.at(index)<<le_list.at(index+1);
    updateListview();
    ui->listWidget->setCurrentRow(index+1);
}

//删除动画
void MainWindow::on_pushButton_4_clicked()
{
    int index = ui->listWidget->currentRow();
    if(ui->listWidget->count()<1){
        return;
    }
    le_list.removeAt(index);
    updateListview();
}

void MainWindow::on_toolButton_clicked()
{
    if(m_pTimer->isActive()){
            QMessageBox::information(this,
                                   tr("打开图像失败"),
                                 tr("请先停止动画!"));

            return ;

    }
    savePara();
    updatePara();
    QMessageBox::information(this,
                           tr("成功"),
                         tr("更新参数成功!"));
}

void MainWindow::on_time_lineEdit_5_textChanged(const QString &arg1)
{
    if(edit_count<5){
        edit_count++;
         return;
    }
    if(ui->time_lineEdit_5->text().toInt()<0){
            QMessageBox::information(this,
                                   tr("错误"),
                                 tr("请输入正确数字"));

            return ;

    }
    savePara();
    updatePara();
}

void MainWindow::on_h_len_lineEdit_4_textChanged(const QString &arg1)
{
    if(edit_count<5){
        edit_count++;
         return;
    }
    if((ui->h_len_lineEdit_4->text().toInt()<0)||(ui->h_len_lineEdit_4->text().toInt()>IMG_H)){
            QMessageBox::information(this,
                                   tr("错误"),
                                 tr("请输入正确数字"));

            return ;

    }
    savePara();
    updatePara();
}

void MainWindow::on_w_len_lineEdit_3_textChanged(const QString &arg1)
{
    if(edit_count<5){
        edit_count++;
         return;
    }
    if((ui->w_len_lineEdit_3->text().toInt()<0)||(ui->h_len_lineEdit_4->text().toInt()>IMG_W)){
            QMessageBox::information(this,
                                   tr("错误"),
                                 tr("请输入正确数字"));

            return ;

    }
    savePara();
    updatePara();
}

void MainWindow::on_x_pos_lineEdit_textChanged(const QString &arg1)
{
    if(edit_count<5){
        edit_count++;
         return;
    }
    if((ui->x_pos_lineEdit->text().toInt()<0)\
            ||(ui->h_len_lineEdit_4->text().toInt()>(IMG_W-logo_obj.le_img_w))){
            QMessageBox::information(this,
                                   tr("错误"),
                                 tr("请输入正确数字"));

            return ;

    }
    savePara();
    updatePara();
}

void MainWindow::on_y_pos_lineEdit_2_textChanged(const QString &arg1)
{
    if(edit_count<5){
        edit_count++;
        return;
    }
    if((ui->y_pos_lineEdit_2->text().toInt()<0)\
            ||(ui->h_len_lineEdit_4->text().toInt()>(IMG_H-logo_obj.le_img_h))){
            QMessageBox::information(this,
                                   tr("错误"),
                                 tr("请输入正确数字"));

            return ;

    }
    savePara();
    updatePara();
}
