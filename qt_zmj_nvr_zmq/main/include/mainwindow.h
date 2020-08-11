#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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



protected:
    void paintEvent(QPaintEvent *);


private slots:
    void showTime();
   // void restartStream();
 

    void on_pushButton_4_clicked();

    void on_pushButton_5_clicked();

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
};

#endif // MAINWINDOW_H
