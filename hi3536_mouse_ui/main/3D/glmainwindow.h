#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "glwidget.h"


class ZMJMainWindow : public QWidget
{
    Q_OBJECT

public:
    explicit ZMJMainWindow(QWidget *parent = 0);
    ~ZMJMainWindow();

public slots:
    void updateSub();
private:
    GLWidget* glWidget;
};

#endif // MAINWINDOW_H
