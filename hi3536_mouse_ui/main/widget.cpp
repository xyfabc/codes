#include "widget.h"
#include <QDebug>
#include <math.h>
#include <QOpenGLTexture>
#include <QOpenGLShaderProgram>

Widget::Widget(QOpenGLWidget *parent)
    : QOpenGLWidget(parent)
{
    timer.setInterval(10);
    connect(&timer,SIGNAL(timeout()),this,SLOT(update()));
    timer.start();
}

Widget::~Widget()
{

}

void Widget::initializeGL()
{
    //为当前环境初始化openGL函数
    initializeOpenGLFunctions();
     glClearColor(1.0f,1.0f,1.0f,1.0f);
    texture = new QOpenGLTexture(QImage("/mnt/nfs/phytec/imx/phy-imx6ull/rollerL.png"));
    //创建顶点着色器ki
    QOpenGLShader* vshader = new QOpenGLShader(QOpenGLShader::Vertex,this);
    const QString vsrc =
                         "uniform highp mat4 matrix;                    \n"

                         "                                              \n"
                         "attribute highp vec4 vPosition;               \n"
                         "attribute lowp vec2 vTexcoord;                \n"
                         "                                              \n"
                         "varying lowp vec2 Texcoord;                   \n"
                         "                                              \n"
                         "void main()                                   \n"
                         "{                                             \n"
                         "    Texcoord = vTexcoord;                     \n"
                         "    gl_Position = matrix * vPosition;         \n"
                         "}                                             \n";
    vshader->compileSourceCode(vsrc);
    //创建片段着色器
    QOpenGLShader* fshader = new QOpenGLShader(QOpenGLShader::Fragment,this);
    const QString fsrc =

                         "uniform highp sampler2D texture;                  \n"
                         "                                                  \n"
                         "varying lowp vec2 Texcoord;                       \n"
                         "void main()                                       \n"
                         "{                                                 \n"
                         "    gl_FragColor = texture2D(texture, Texcoord);  \n"
                         "}                                                 \n";
    fshader->compileSourceCode(fsrc);
    //创建着色器程序
    program = new QOpenGLShaderProgram;
    program->addShader(vshader);
    program->addShader(fshader);
    program->link();
    program->bind();
    vbo.create();
    vbo.bind();
    //cbo.create();
    //cbo.bind();
}

void Widget::resizeGL(int, int)
{

}

void Widget::paintGL()
{
    int w = width();
    int h = height();
    //int side = qMin(w,h);
    int side = qMin(w,h);
    //qDebug()<<"---------"<<width()<<height();
    glViewport((w - side) / 2, (h - side) / 2, side, side);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    GLfloat vertices[] =
    {
        -0.10f,-0.10f,
         0.10f,-0.1f,
         0.10f, 0.1f,
        -0.10f, 0.1f
    };
    GLfloat coord[]
    {
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f
    };
    /*GLfloat color[]=
    {
        1.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 1.0f,
        1.0f, 1.0f, 1.0f
    };*/
    GLint vPosition = static_cast<GLint>(program->attributeLocation("vPosition"));
    //GLint vColor = static_cast<GLint>(program->attributeLocation("vColor"));
    GLint vTexcoord = static_cast<GLint>(program->attributeLocation("vTexcoord"));

    vbo.allocate(vertices, 16 * sizeof(GLfloat));
    //vbo.write(8 * sizeof(GLfloat), color, 12 * sizeof(GLfloat));
    vbo.write(8 * sizeof(GLfloat), coord, 8 * sizeof(GLfloat));

    program->setAttributeBuffer(vPosition, GL_FLOAT, 0, 2, 0);
    //program->setAttributeBuffer(vColor, GL_FLOAT, 8 * sizeof(GLfloat), 3, 0);
    program->setAttributeBuffer(vTexcoord, GL_FLOAT, 8 * sizeof(GLfloat), 2, 0);

    glEnableVertexAttribArray(static_cast<GLuint>(vPosition));
    glEnableVertexAttribArray(static_cast<GLuint>(vTexcoord));

    texture->bind();

    for(float i = 0; i < 10; i++)
    {
        for(float j = 0; j < 10; j++)
        {
            QMatrix4x4 matrix;
            matrix.perspective(60.0f, 1, 1.0f, 1.0f);
            matrix.translate(-0.9f + i/5, -0.9f + j/5, 0);
            matrix.rotate(mFrame, 0, 0, 1);
            program->setUniformValue("matrix",matrix);
            program->setUniformValue("texture",0);

            glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
        }
    }

    mFrame += 3;
}
