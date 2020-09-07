#ifndef LCDFORM_H
#define LCDFORM_H

#include <QWidget>
#include <QThread>
#include "hisi_vdec.h"
#include <QObject>


namespace Ui {
class LCDForm;
}

class LCDForm : public QWidget
{
    Q_OBJECT

public:
    explicit LCDForm(QWidget *parent = 0);
    ~LCDForm();
    bool CapturePicture(const QString& filename);
    void setCameraId(int id);

protected:
    void run();

private slots:
    void on_pushButton_clicked();
public slots:
    void onRenderVideo(unsigned char *buf, int width, int height, int* flag);

private:
    Ui::LCDForm *ui;
    unsigned char* prgb_buf;
    int cameraId;
};

#endif // LCDFORM_H
