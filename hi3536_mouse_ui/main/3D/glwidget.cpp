#include "glwidget.h"
#include <QVector>
#include <QMouseEvent>
#include <QTimer>
#include <QList>
#include <QPair>
#include <QDir>

GLWidget::GLWidget(QWidget* parent):
    QOpenGLWidget(parent),
    m_xRot(0),
    m_yRot(0)
{

}

GLWidget::~GLWidget()
{

}

QByteArray versionedShaderCode(const char *src)
{
    QByteArray versionedSrc;

    if (QOpenGLContext::currentContext()->isOpenGLES())
    {
        versionedSrc.append(QByteArrayLiteral("#version 300 es\n"));
    }
    else
        versionedSrc.append(QByteArrayLiteral("#version 330\n"));

    versionedSrc.append(src);
    return versionedSrc;
}

void GLWidget::initShaders()
{
    //QOpenGLShader *vshader = new QOpenGLShader(QOpenGLShader::Vertex, this);
//    const char *vsrc =
//        "layout(location = 0) in vec3 vertexPosition;\n"
//        "layout(location = 1) in vec3 vertexNormal;\n"
//        "out vec3 normal;\n"
//        "out vec3 position;\n"
//        "uniform mat4 MV;\n"
//        "uniform mat3 N;\n"
//        "uniform mat4 MVP;\n"
//        "void main()\n"
//        "{\n"
//        "    normal=normalize(N*vertexNormal);\n"
//        "    position=vec3(MV*vec4(vertexPosition,1.0));\n"
//        "    gl_Position=MVP*vec4(vertexPosition,1.0);\n"
//        "}\n";

//    const char *fsrc =
//            "in highp vec3 normal;\n"
//            "in highp vec3 position;\n"
//            "layout(location=0) out highp vec4 fragColor;\n"
//            "uniform highp vec4 lightPosition;\n"
//            "uniform highp vec3 lightIntensity;\n"
//            "uniform highp vec3 Ka;\n"
//            "uniform highp vec3 Kd;\n"
//            "uniform highp vec3 Ks;\n"
//            "uniform highp float shininess;\n"
//            "void main()\n"
//            "{\n"
//            "    highp vec3 n=normalize(normal);\n"
//            "    highp vec3 s=normalize(lightPosition.xyz-position);\n"
//            "    highp vec3 v=normalize(-position.xyz);\n"
//            "    highp vec3 h=normalize(v+s);\n"
//            "    highp float sdn=dot(s,n);\n"
//            "    highp vec3 ambient=Ka;\n"
//            "    highp vec3 diffuse=Kd*max(sdn,0.0);\n"
//            "    highp vec3 specular=Ks*mix(0.0,pow(dot(h,n),shininess),step(0.0,sdn));\n"
//            "    fragColor=vec4(lightIntensity*(0.1*ambient+diffuse+specular),1);\n"
//            "}\n";

    const char *vsrc =
        "layout(location = 0) in vec3 vertexPosition;\n"
        "layout(location = 1) in vec3 vertexNormal;\n"
        "out vec3 normal;\n"
        "out vec3 position;\n"
        "uniform mat4 MV;\n"
        "uniform mat3 N;\n"
        "uniform mat4 MVP;\n"
        "void main()\n"
        "{\n"
        "    normal=normalize(N*vertexNormal);\n"
        "    position=vec3(MV*vec4(vertexPosition,1.0));\n"
        "    gl_Position=MVP*vec4(vertexPosition,1.0);\n"
        "}\n";

    const char *fsrc =
            "in highp vec3 normal;\n"
            "in highp vec3 position;\n"
            "layout(location=0) out highp vec4 fragColor;\n"
            "uniform highp vec4 lightPosition;\n"
            "uniform highp vec3 lightIntensity;\n"
            "uniform highp vec3 Ka;\n"
            "uniform highp vec3 Kd;\n"
            "uniform highp vec3 Ks;\n"
            "uniform highp float shininess;\n"
            "void main()\n"
            "{\n"
            "    highp vec3 n=normalize(normal);\n"
            "    highp vec3 s=normalize(lightPosition.xyz-position);\n"
            "    highp vec3 v=normalize(-position.xyz);\n"
            "    highp vec3 h=normalize(v+s);\n"
            "    highp float sdn=dot(s,n);\n"
            "    highp vec3 ambient=Ka;\n"
            "    highp vec3 diffuse=Kd*max(sdn,0.0);\n"
            "    highp vec3 specular=Ks*mix(0.0,pow(dot(h,n),shininess),step(0.0,sdn));\n"
            "    fragColor=vec4(lightIntensity*(0.1*ambient+diffuse+specular),1);\n"
            "}\n";

    qDebug() << m_program.addShaderFromSourceCode(QOpenGLShader::Vertex, versionedShaderCode(vsrc));
     qDebug() << m_program.addShaderFromSourceCode(QOpenGLShader::Fragment, versionedShaderCode(fsrc));
    //m_program.addShader(vshader);
    //m_program.addShader(fshader);

//    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Vertex, ":/vshader.glsl"))
//        close();

//    // Compile fragment shader
//    if (!m_program.addShaderFromSourceFile(QOpenGLShader::Fragment, ":/fshader.glsl"))
//        close();

    // Link shader pipeline
    if (!m_program.link())
        close();
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    initShaders();
    m_program.bind();

    m_program.setUniformValue("lightPosition",QVector4D(6.0f,3.0f,5.0f,1.0f));//设置光源在空间中的位置
    m_program.setUniformValue("lightIntensity",QVector3D(0.9f,0.9f,0.9f));//设置光源强度

    loadOBJMTL();

    QVector3D eye(20, 13, 5);
    QVector3D center(0, 0, 0);
    QVector3D up(0, 1, 0);

    /*
        m_view为视点变换矩阵
        eye为眼睛在空间中的位置
        center为眼睛注视的中心
        up为向上的方向向量
    */
    m_view.setToIdentity();
    m_view.lookAt(eye,center,up);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f,0.0f,0.0f,1.0f);
}

void GLWidget::resizeGL(int w,int h)
{
    glViewport(0,0,w,h);
    m_proj.setToIdentity();
    m_proj.perspective(60.0f,(float)w/h,0.3f,1000);

    update();
}

void GLWidget::paintGL()
{
    m_program.bind();

    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 model;
    model.setToIdentity();
    model.translate(-10.0f,0.0f,4.5f);
    model.scale(0.8f);
    model.rotate(-90,0,1,0);
    model.rotate(m_xRot / 16.0f, 1, 0, 0);
    model.rotate(m_yRot / 16.0f, 0, 1, 0);

    for (int x = 0; x < 20; x++)
    {
        for (int y = 0; y < 20; y++)
        {
            for (int z = 0; z < 20; z++)
            {/*
                for(ObjLoader* loader: objLoaderList)
                {
                    loader->drawObjEntity(&m_program,model,m_view,m_proj); //绘制实体
                    model.translate(3.0f,i - 5, j-5);
                }*/

                QMatrix4x4 tmp = model;

                /*
                    model为模型变换矩阵，设定模型在空间中的位置，大小，旋转
                */

                tmp.translate(x-10,y-10,z-10);
                objLoaderList[0]->drawObjEntity(&m_program,tmp,m_view,m_proj); //绘制实体
            }
        }
    }

    //loaderBoard.drawObjEntity(&m_program,model,m_view,m_proj); //绘制实体

    //model.rotate(90,0,1,0);
    //model.translate(19.0f,0.0f,-9.3f);

    //loaderCoordinate.drawObjEntity(&m_program,model,m_view,m_proj);
}

void GLWidget::loadOBJMTL()
{
//    QDir dir(OBJ_DIR_PATH);
//    QStringList filters;
//    filters << "*.obj";
//    QStringList filePathList;
//    filePathList = dir.entryList(filters, QDir::Files|QDir::Readable, QDir::Name);

//    QString objPath = "";
//    QString mtlPath = "";
//    for (QString& tmp: filePathList)
//    {
//        objPath = OBJ_DIR_PATH + "/" + tmp;
//        mtlPath = OBJ_DIR_PATH + "/" + tmp.replace(".obj", ".mtl");
//        if (!QFile::exists(mtlPath))
//            continue;

//        ObjLoader* loader = new ObjLoader;
//        if (!loader->Load(objPath, mtlPath))
//        {
//            delete  loader;
//            continue;
//        }

//        loader->allocateVertexBuffer(&m_program);//分配图元空间*/
//        objLoaderList.push_back(loader);

//        //objList.push_back(qMakePair<QString, QString>(objPath, mtlPath));
//    }

    QList<QPair<QString, QString>> objList;
    objList.push_back(qMakePair<QString, QString>(":/obj/arcticcondor.obj",":/obj/arcticcondor.mtl"));
    //objList.push_back(qMakePair<QString, QString>(":/obj/saber_c3_evk_v2_0_for_qt_edit2.obj",":/obj/saber_c3_evk_v2_0_for_qt_edit2.mtl"));
    //objList.push_back(qMakePair<QString, QString>(":/obj/coordinate.obj",":/obj/coordinate.mtl"));
    //objList.push_back(qMakePair<QString, QString>("E:/moshoushijie/3d121007ms/2/Creature/Arakkoa/arakkoa.obj", "E:/moshoushijie/3d121007ms/2/Creature/Arakkoa/arakkoa.mtl"));

//    int i = 0;
    for (QPair<QString, QString>& objPath: objList)
    {
        ObjLoader* loader = new ObjLoader;
        if (!loader->Load(objPath.first, objPath.second))
        {
            delete  loader;
            continue;
        }

        loader->allocateVertexBuffer(&m_program);//分配图元空间*/
        objLoaderList.push_back(loader);
    }

//    if(!loaderBoard.Load())
//    {
//        qDebug()<<"Fail to load model";
//        exit(1);
//    }

//    if(!loaderBoard.Load())
//    {
//        qDebug()<<"Fail to load model";
//        exit(1);
//    }

//    loaderBoard.allocateVertexBuffer(&m_program);//分配图元空间*/

//    if(!loaderCoordinate.Load())
//    {
//        qDebug()<<"Fail to load model";
//        exit(1);
//    }

//    loaderCoordinate.allocateVertexBuffer(&m_program);
}

QSize GLWidget::sizeHint() const
{
    return QSize(600,400);
}

QSize GLWidget::minimumSizeHint() const
{
    return QSize(60,60);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_lastPos=event->pos();
}
\
void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int dx=event->x()-m_lastPos.x();
    int dy=event->y()-m_lastPos.y();

    m_xRot+=dx;
    m_yRot+=dy;
    update();
}
