#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFile>
#include <QByteArray>
#include <QListWidget>
#include <QTimer>

#define IMG_H 320
#define IMG_W 240

#define LE_IMG_W 80
#define LE_IMG_H 96

namespace Ui {
class MainWindow;
}
struct LOGO_OBJ {

    //背景图属性
    int le_img_h;
    int le_img_w;

    //动画属性
    int move_count;   /*动画张数*/
    int img_h;
    int img_w;

    int move_time;      /*动画间隔时间ms*/

    int x_pos;          /*动画起始X坐标*/
    int y_pos;          /*动画起始Y坐标*/
};


class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    int revert_image(uchar *dst, uchar *src, int img_h, int img_w);
    void build_movs(QString filename, uchar *dst, int img_h, int img_w);
    void save_file(QString outFileName,uchar *buf,int len);
    void updateListview();
    void createBin();
    void savePara();
    void updatePara();
    void InitWin();
    QImage mergeImages2Image(const QImage &aimage, const QImage &bimage,int x_pos,int y_pos,int x_len,int y_len);

private slots:

    void on_build_pushButton_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void on_listWidget_itemPressed(QListWidgetItem *item);

    void on_pushButton_3_clicked();
    void on_UPtoolButton_clicked();

    void on_DowntoolButton_clicked();

    void on_pushButton_4_clicked();

    void on_toolButton_clicked();

    void on_time_lineEdit_5_textChanged(const QString &arg1);

    void on_h_len_lineEdit_4_textChanged(const QString &arg1);

    void on_w_len_lineEdit_3_textChanged(const QString &arg1);

    void on_x_pos_lineEdit_textChanged(const QString &arg1);

    void on_y_pos_lineEdit_2_textChanged(const QString &arg1);

public slots:
    void handleTimeout();  //超时处理函数
private:
    QTimer *m_pTimer;

private:
    Ui::MainWindow *ui;
    QImage *logo_img;
     QList <QImage> image_list;
    QString out_file_name;
    QString logo_file_name;
    QList <QString>  le_list;
    struct LOGO_OBJ logo_obj;
    int edit_count;
};

#endif // MAINWINDOW_H
