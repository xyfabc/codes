/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the documentation of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "openglwindow.h"

#include <QtCore/QCoreApplication>

#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLPaintDevice>
#include <QtGui/QPainter>



#define ATTRIB_VERTEX 3
#define ATTRIB_TEXTURE 4

//! [3]
static const char *vertexShaderSource ="attribute vec4 vertexIn; \n"
                                       "attribute vec2 textureIn;\n"
                                       "varying vec2 textureOut;\n"
                                       "void main(void)\n"
                                       "{\n"
                                       "    gl_Position = vertexIn; \n"
                                       "    textureOut = textureIn;\n"
                                       "}";

static const char *fragmentShaderSource ="varying highp vec2 textureOut;"
                                         "uniform highp sampler2D tex_y;"
                                         "uniform highp sampler2D tex_u;"
                                         "void main(void){"
                                         "highp vec3 yuv;"
                                         "highp vec3 rgb;"
                                         "yuv.x = texture2D(tex_y, textureOut).r;"
                                         "yuv.y = texture2D(tex_u, textureOut).g - 0.5;"
                                         "yuv.z = texture2D(tex_u, textureOut).r - 0.5;"

                                         "rgb = mat3( 1,       1,         1,"
                                         "0,       -0.39465,  2.03211,"
                                         "1.13983, -0.58060,  0) * yuv;"
                                         "gl_FragColor = vec4(rgb, 1);}";

//static const char *fragmentShaderSource ="varying highp vec2 textureOut;"
//                                         "uniform highp sampler2D tex_y;"
//                                         "uniform highp sampler2D tex_u;"
//                                         "void main(void){"
//                                         "highp vec3 yuv;"
//                                         "highp vec3 rgb;"
//                                         "rgb.r = texture2D(tex_y, textureOut).r;"
//                                         "rgb.g = 0.0;"
//                                         "rgb.b = 0.0;"
//                                         "gl_FragColor = vec4(rgb, 1);}";

OpenGLWindow::OpenGLWindow(QWidget *parent)
    : QOpenGLWidget(parent)
    , m_animating(false)
    , m_context(0)
    , m_device(0)
    , fbo_y(QOpenGLBuffer::PixelUnpackBuffer)
    , fbo_uv(QOpenGLBuffer::PixelUnpackBuffer)
{
    pixel_w = 1920;
    pixel_h = 1080;

    m_frame = 0;
    m_lastPrintFps = 0;
    m_firstFrameTick = 0;

    timecost = 0;
    cameraId = -1;
}

OpenGLWindow::~OpenGLWindow()
{
    delete m_device;
}

void OpenGLWindow::initializeGL()
{
    initializeOpenGLFunctions();

    m_program = new QOpenGLShaderProgram(this);
    m_program->addShaderFromSourceCode(QOpenGLShader::Vertex, vertexShaderSource);
    m_program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentShaderSource);
    m_program->bindAttributeLocation("vertexIn",ATTRIB_VERTEX);
    m_program->bindAttributeLocation("textureIn",ATTRIB_TEXTURE);
    m_program->link();
    glGetProgramiv(m_program->programId(),GL_LINK_STATUS,&linked);

    m_textureUniformY = m_program->uniformLocation("tex_y");
    m_textureUniformUV = m_program->uniformLocation("tex_u");

    m_program->bind();



     fbo_y.create();
     fbo_y.bind();
     fbo_y.allocate(nullptr, pixel_h * pixel_w);
     fbo_y.release();

     fbo_uv.create();
     fbo_uv.bind();
     fbo_uv.allocate(nullptr, pixel_h * pixel_w / 2);
     fbo_uv.release();

     glGenTextures(1, &id_y);
     glBindTexture(GL_TEXTURE_2D, id_y);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

     glGenTextures(1, &id_u);
     glBindTexture(GL_TEXTURE_2D, id_u);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
}

void OpenGLWindow::resizeGL(int w, int h)
{
    const qreal retinaScale = devicePixelRatio();
    glViewport(0, 0, width() * retinaScale, height() * retinaScale);
}

#include <QDebug>
#include <QDateTime>

void OpenGLWindow::paintGL()
{
    if (m_frame == 0)
    {
        m_firstFrameTick = QDateTime::currentMSecsSinceEpoch();
    }

    qint64 t1 = QDateTime::currentMSecsSinceEpoch();
    glClearColor(0,255,0,0);
    glClear(GL_COLOR_BUFFER_BIT);


    GLfloat vertexVertices[] = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f,  1.0f,
        1.0f,  1.0f,
    };

    GLfloat textureVertices[] = {
        0.0f,  1.0f,
        1.0f,  1.0f,
        0.0f,  0.0f,
        1.0f,  0.0f,
    };
    // 设置物理坐标
    glVertexAttribPointer(ATTRIB_VERTEX, 2, GL_FLOAT, 0, 0, vertexVertices);

    // 设置纹理坐标
    glVertexAttribPointer(ATTRIB_TEXTURE, 2, GL_FLOAT, 0, 0, textureVertices);

    glEnableVertexAttribArray(ATTRIB_VERTEX);
    glEnableVertexAttribArray(ATTRIB_TEXTURE);


    fbo_y.bind();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, id_y);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED_NV, pixel_w, pixel_h, 0, GL_RED_NV, GL_UNSIGNED_BYTE, nullptr);
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pixel_w, pixel_h, GL_RED_NV, GL_UNSIGNED_BYTE, nullptr);
    glUniform1i(m_textureUniformY, 0);
    fbo_y.release();


    fbo_uv.bind();
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, id_u);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RG, pixel_w / 2, pixel_h / 2, 0, GL_RG, GL_UNSIGNED_BYTE, nullptr);
    //glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, pixel_w / 2, pixel_h / 2, GL_RG, GL_UNSIGNED_BYTE, nullptr);
    glUniform1i(m_textureUniformUV, 1);
    fbo_uv.release();

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

    glDisableVertexAttribArray(ATTRIB_TEXTURE);
    glDisableVertexAttribArray(ATTRIB_VERTEX);


    ++m_frame;

    qint64 t2 = QDateTime::currentMSecsSinceEpoch();
    timecost += (t2 - t1);
//    if ((t2 - m_lastPrintFps) > 3000)
//    {
//        m_lastPrintFps = QDateTime::currentMSecsSinceEpoch();


//        qint64 timecostS = timecost / 1000;
//        if (timecostS > 0)
//        {
//            int fps = m_frame / timecostS;

//            qDebug() << "current fps: " << fps;
//            qDebug() << "current total time cost: " << timecostS;
//            qDebug() << "current total frmae: " << m_frame;
//        }

//    }

    if (m_frame % 100 == 0)
    {
        m_lastPrintFps = QDateTime::currentMSecsSinceEpoch();


        qint64 timecostS = timecost / 100;
        if (timecostS > 0)
        {
            int fps = (m_frame / timecostS) * 10;

            qDebug() << "current fps: " << fps;
            qDebug() << "current total time cost: " << timecostS;
            qDebug() << "current total frmae: " << m_frame;
        }
    }

    //qDebug() << "-------------" << timecost;
}

void OpenGLWindow::setCameraId(int id)
{
    this->cameraId = id;
}

void OpenGLWindow::onRenderVideo(unsigned char *buf, int width, int height, int *flag,int id)
{

    //qDebug() << "-------------"<<cameraId<<id;
    if(cameraId != id){
        return;
    }
    qint64 t1 = QDateTime::currentMSecsSinceEpoch();


    fbo_y.bind();
    fbo_y.write(0, buf, pixel_w * pixel_h);
    fbo_y.release();


    fbo_uv.bind();
    fbo_uv.write(0, buf + pixel_w * pixel_h, pixel_w * pixel_h / 2);
    fbo_uv.release();


    update();
    //repaint();

    *flag += 1;
   // ++m_frame;
    qint64 t2 = QDateTime::currentMSecsSinceEpoch();

    timecost += (t2 - t1);
    m_frame++;
    //qDebug()<<">>>>>>>>>>>>>>>>>>>>>"<< t2 - t1;
}
