#ifndef ZMJWINDOWS_H
#define ZMJWINDOWS_H

#include <QWidget>
#include <QMainWindow>
#include "glwidget.h"




class ZMJWindows: public QWidget
{
public:
    ZMJWindows();

public slots:
    void updateSub();
private:
    GLWidget* glWidget;
};

#endif // ZMJWINDOWS_H
