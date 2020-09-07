#ifndef WIDGET_H
#define WIDGET_H

#include <QTimer>
#include <QMatrix4x4>
#include <QOpenGLWidget>
#include <QOpenGLBuffer>
#include <QOpenGLFunctions>

class QOpenGLShaderProgram;
class QOpenGLTexture;

class Widget : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT

public:
    Widget(QOpenGLWidget *parent = nullptr);
    ~Widget();

protected:
    void initializeGL();
    void paintGL();
    void resizeGL(int width, int height);

private:
    QTimer timer;
    int mFrame = 0;
    QOpenGLBuffer vbo;
    QOpenGLTexture* texture;
    QOpenGLShaderProgram* program;
};

#endif // WIDGET_H
