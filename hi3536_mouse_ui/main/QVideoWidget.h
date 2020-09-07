#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>


namespace Ui {
class MainWindow;
}

class QVideoWidget : public QOpenGLWidget
        , protected QOpenGLFunctions
{
    Q_OBJECT

public:
    explicit QVideoWidget(QWidget *parent = nullptr);
    ~QVideoWidget();

    void setVideoSize(int width, int height);

public slots:
    void onRenderVideo(unsigned char* buf, int width, int height, int* flag);

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:
    void createTexture(unsigned char* buf, int width, int height);
    void destroyTexture();

private:
    QOpenGLBuffer vbo;
    QOpenGLShaderProgram* program;

private:
    int vwidth;
    int vheight;
    unsigned char* videobuf;

    QOpenGLTexture* textureY;
    QOpenGLTexture* textureVU;
};

#endif // MAINWINDOW_H
