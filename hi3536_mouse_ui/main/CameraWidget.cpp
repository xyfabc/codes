#include "CameraWidget.h"
#include "ui_CameraWidget.h"
#include "widget.h"
#include <QGridLayout>

CameraWidget::CameraWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CameraWidget)
{
    ui->setupUi(this);


}

CameraWidget::~CameraWidget()
{
    delete ui;
}

void CameraWidget::on_pushButton_clicked()
{
    this->close();
}

void CameraWidget::on_pushButton_2_clicked()
{
//    Widget *glWidgets = new Widget();
//    glWidgets->resize(1920, 1080);
//    glWidgets->show();

    int w = width();
    int h = height();
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
            glWidgets[i][j]->resize(w/NumRows, h/NumColumns);
            glWidgets[i][j]->show();
            mainLayout->addWidget(glWidgets[i][j], i, j);
            count++;
        }
    }
}
