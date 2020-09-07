#include "QVideoWidget.h"
#include "ui_mainwindow.h"

#include <QDebug>
#include <QDateTime>

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1

const char *vsrc =
    "attribute highp vec4 vertex;\n"
    "attribute mediump vec4 texCoord;\n"
    "varying mediump vec4 texc;\n"
    "uniform mediump mat4 matrix;\n"
    "void main(void)\n"
    "{\n"
    "    gl_Position = matrix * vertex;\n"
    "    texc = texCoord;\n"
    "}\n";

const char *fsrc =
    "uniform sampler2D texture_y;\n"
    "uniform sampler2D texture_vu;\n"
    "varying mediump vec4 texc;\n"
    "void main(void)\n"
    "{\n"
    "    mediump float y,u,v;\n"
    "    mediump vec3 rgb;\n"
    "    y = texture2D(texture_y, texc.st).r;\n"
    "    v = texture2D(texture_vu, texc.st).r - 0.5;\n"
    "    u = texture2D(texture_vu, texc.st).g - 0.5;\n"
//    "    rgb.r = y + 1.403 * (v );\n"
//    "    rgb.g = y - 0.344 * (u) - 0.714 * (v);\n"
//    "    rgb.b = y + 1.772 * (u);\n"
        "    rgb.r = y;\n"
        "    rgb.g = u;\n"
        "    rgb.b = v;\n"
    "    gl_FragColor=vec4(rgb,1);\n"
    "}\n";

QVideoWidget::QVideoWidget(QWidget *parent) :
    QOpenGLWidget(parent)
{
    videobuf = nullptr;
    this->vwidth = 0;
    this->vheight = 0;

    this->textureY = nullptr;
    this->textureVU = nullptr;
}

QVideoWidget::~QVideoWidget()
{
    if (videobuf)
    {
        delete[] videobuf;
        videobuf = nullptr;
    }

    if (textureY)
    {
        delete textureY;
        textureY = nullptr;
    }
    if (textureVU)
    {
        delete textureVU;
        textureVU = nullptr;
    }
}

void QVideoWidget::setVideoSize(int width, int height)
{
    if (this->vwidth != width || this->vheight != height)
    {
        this->vwidth = width;
        this->vheight = height;
    }
}

void QVideoWidget::initializeGL()
{
    initializeOpenGLFunctions();
    vbo.create();
    vbo.bind();
    vbo.create();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    fshader->compileSourceCode(fsrc);

    program = new QOpenGLShaderProgram;
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("texture_y", 0);
    program->setUniformValue("texture_vu", 1);

    QVector<GLfloat> vertData;
    static const int coords[4][3] = { { +1, -1, -1 }, { -1, -1, -1 }, { -1, +1, -1 }, { +1, +1, -1 } };
    for (int j = 0; j < 4; ++j) {
        // vertex position
        vertData.append(coords[j][0]);
        vertData.append(coords[j][1]);
        vertData.append(coords[j][2]);

        vertData.append(j == 0 || j == 3 ? 1:0);
        vertData.append(j == 0 || j == 1 ? 0:1);
    }
    vbo.create();
    vbo.bind();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));


//    unsigned char* buf = new unsigned char[vwidth*vheight*3/2];
//    FILE* tmp = fopen("E:/workspace/zzmj.automation.runtime.debug/test--01_01_31_11.yuv420sp", "rb");
//    fread(buf, 1, vwidth*vheight*3/2, tmp);
//    fclose(tmp);

    destroyTexture();
    createTexture(nullptr, vwidth, vheight);
}

void QVideoWidget::paintGL()
{
    qint64 t1 = QDateTime::currentMSecsSinceEpoch();
    QColor clearColor(255, 255, 255, 255);
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF(), clearColor.alphaF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 m;
    m.ortho(-1.0f, +1.0f, +1.0f, -1.0f, 4.0f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);
    qint64 t2 = QDateTime::currentMSecsSinceEpoch();
    program->setUniformValue("matrix", m);
    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    textureY->bind(0);
    textureVU->bind(1);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);

    qint64 t3 = QDateTime::currentMSecsSinceEpoch();

    qDebug() << "------1-------" << t2 - t1;
    qDebug() << "-----2--------" << t3 - t2;
}

void QVideoWidget::resizeGL(int width, int height)
{
    glViewport(0, 0, width, height);
}

void QVideoWidget::onRenderVideo(unsigned char *buf, int width, int height, int* flag)
{
    qint64 t1 = QDateTime::currentMSecsSinceEpoch();
    if (vwidth != width || vheight != height)
    {
        vwidth = width;
        vheight = height;

        destroyTexture();
        createTexture(buf, width, height);
        qDebug() << "11111";
    }
    else
    {
        //memcpy(videobuf, buf, vwidth * vheight * 3 / 2);

        textureY->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, buf);

        char* pix = reinterpret_cast<char*>(buf);
        pix = pix + this->vwidth * this->vheight;
        textureVU->setData(QOpenGLTexture::RG, QOpenGLTexture::UInt8, pix);
    }

    *flag += 1;
    repaint();
    qint64 t2 = QDateTime::currentMSecsSinceEpoch();
    qDebug()<<">>>>>>>>>>>>>>>>>>>>>"<<t2 -t1;
}

void QVideoWidget::createTexture(unsigned char* buf, int width, int height)
{
    videobuf = new unsigned char[width * height * 3 / 2];
    if (!buf)
    {
        memset(videobuf, 0, width * height * 3 / 2);
    }
    else
    {
        memcpy(videobuf, buf, width * height * 3 / 2);
    }

    textureY = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureY->setFormat(QOpenGLTexture::R8_UNorm);
    textureY->setSize(this->vwidth, this->vheight);
    textureY->allocateStorage();
    textureY->setData(QOpenGLTexture::Red, QOpenGLTexture::UInt8, videobuf);
    textureY->setMagnificationFilter(QOpenGLTexture::Linear);
    textureY->setMinificationFilter(QOpenGLTexture::Linear);
    textureY->setWrapMode(QOpenGLTexture::ClampToEdge);

    char* pix = reinterpret_cast<char*>(videobuf);
    pix = pix + this->vwidth * this->vheight;
    textureVU = new QOpenGLTexture(QOpenGLTexture::Target2D);
    textureVU->setFormat(QOpenGLTexture::RG8_UNorm);
    textureVU->setSize(this->vwidth / 2, this->vheight / 2);
    textureVU->allocateStorage();
    textureVU->setData(QOpenGLTexture::RG, QOpenGLTexture::UInt8, pix);
    textureVU->setMagnificationFilter(QOpenGLTexture::Linear);
    textureVU->setMinificationFilter(QOpenGLTexture::Linear);
    textureVU->setWrapMode(QOpenGLTexture::ClampToEdge);
}

void QVideoWidget::destroyTexture()
{
    if (videobuf)
    {
        delete[] videobuf;
        videobuf = nullptr;
    }

    if (textureY)
    {
        delete textureY;
        textureY = nullptr;
    }
    if (textureVU)
    {
        delete textureVU;
        textureVU = nullptr;
    }
}
