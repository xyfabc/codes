
#include "glmainwindow.h"
#include <QTimer>
#include <QGridLayout>

ZMJMainWindow::ZMJMainWindow(QWidget *parent) :
    QWidget(parent)
{
    QGridLayout *mainLayout = new QGridLayout;

    glWidget=new GLWidget;

    mainLayout->addWidget(glWidget, 0, 0);
    setLayout(mainLayout);
//    QTimer t;
//    QObject::connect(&t, SIGNAL(timeout()), this, SLOT(updateSub()));
//    t.start(30);
}

ZMJMainWindow::~ZMJMainWindow()
{

}

void ZMJMainWindow::updateSub()
{
    glWidget->update();
}
