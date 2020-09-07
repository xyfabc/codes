#ifndef MAINWINDOW_MAIN_H
#define MAINWINDOW_MAIN_H

#include <QMainWindow>
#include <QPainter>
#include <QLine>
#include <QList>
#include <QRect>
#include <QTimer>
#include <QTime>
#include <QMap>
#include <QLabel>

#include "sample_comm.h"
#include "camera.h"
#include "streamDaemon.h"
#include "glwidget.h"
#include "../lcdform.h"
#include "QVideoWidget.h"
#include "CameraWidget.h"
#include "openglwindow.h"


namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();
    void setTitleName(QString name);
    void setTitleCharName(char *name);
    void setChnLine(unsigned char mode);
    void setChnLabelText();
    void updateImg( unsigned char* pbuf);
    bool CapturePicture(const QString& filename);



protected:
    void paintEvent(QPaintEvent *);
    void mouseMoveEvent(QMouseEvent *event);


private slots:
    void showTime();
    // void restartStream();
    void on_pushButton_4_clicked();
    void on_pushButton_5_clicked();

    void on_pushButton_clicked();

    void on_pushButton_6_clicked();

    void on_pushButton_7_clicked();

    void on_pushButton_8_clicked();

    void on_pushButton_9_clicked();

public slots:
    void upImages(unsigned char *buf);

private:
    // bool isTimerExist(int chn);

private:
    Ui::MainWindow *ui;
    QList<QLine> line_list;
    QMap<int,QLabel*> qLabel_map;
    QList<QRect> rect_list;
    bool isErease;
    QTimer *timer;
    //QTimer *pStreamProtectTimer;
    QMap<QTimer*, int> timer_map;
    StreamDaemon streamDaemon;

    // ChannelTextLabel *dialog;


public slots:
    void updateSub();
private:
    GLWidget* glWidget;
    LCDForm lcdForm;
    unsigned char* prgb_buf;
    QVideoWidget qvideoWidget;
    OpenGLWindow qOpenGLWindow;
    CameraWidget cameraWidget;
    CameraWidget bottomWidget;
    LCDForm* pLcdForm[12];
    OpenGLWindow *pOpenGLWindow[12];
    StreamDaemon *pStreamDaemon[12];

};

#endif // MAINWINDOW_H
