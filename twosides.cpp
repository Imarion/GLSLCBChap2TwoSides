#include "twosides.h"

#include <QtGlobal>

#include <QDebug>
#include <QFile>
#include <QImage>
#include <QTime>

#include <QVector2D>
#include <QVector3D>
#include <QMatrix4x4>

#include <cmath>
#include <cstring>

MyWindow::~MyWindow()
{
    if (mProgramSimpleADS != 0) delete mProgramSimpleADS;
    if (mProgram2Sides    != 0) delete mProgram2Sides;
}

MyWindow::MyWindow()
    : mProgramSimpleADS(0), mProgram2Sides(0), currentTimeMs(0), currentTimeS(0)
{
    setSurfaceType(QWindow::OpenGLSurface);
    setFlags(Qt::Window | Qt::WindowSystemMenuHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowCloseButtonHint);

    QSurfaceFormat format;
    format.setDepthBufferSize(24);
    format.setMajorVersion(4);
    format.setMinorVersion(3);
    format.setSamples(4);
    format.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(format);
    create();

    resize(800, 600);

    mContext = new QOpenGLContext(this);
    mContext->setFormat(format);
    mContext->create();

    mContext->makeCurrent( this );

    mFuncs = mContext->versionFunctions<QOpenGLFunctions_4_3_Core>();
    if ( !mFuncs )
    {
        qWarning( "Could not obtain OpenGL versions object" );
        exit( 1 );
    }
    if (mFuncs->initializeOpenGLFunctions() == GL_FALSE)
    {
        qWarning( "Could not initialize core open GL functions" );
        exit( 1 );
    }

    initializeOpenGLFunctions();

    QTimer *repaintTimer = new QTimer(this);
    connect(repaintTimer, &QTimer::timeout, this, &MyWindow::render);
    repaintTimer->start(1000/60);

    QTimer *elapsedTimer = new QTimer(this);
    connect(elapsedTimer, &QTimer::timeout, this, &MyWindow::modCurTime);
    elapsedTimer->start(1);       
}

void MyWindow::modCurTime()
{
    currentTimeMs++;
    currentTimeS=currentTimeMs/1000.0f;
}

void MyWindow::initialize()
{
    mFuncs->glGenVertexArrays(1, &mVAO);
    mFuncs->glBindVertexArray(mVAO);

    CreateVertexBuffer();
    initShaders();
    initMatrices();

    mRotationMatrixLocation = mProgramSimpleADS->uniformLocation("RotationMatrix");

    glFrontFace(GL_CCW);
    glEnable(GL_DEPTH_TEST);
}

void MyWindow::CreateVertexBuffer()
{
    QMatrix4x4 transform;
    transform.translate(QVector3D(0.0f, 1.5f, 0.25f));
    mTeapot = new Teapot(13, transform);

    // Create and populate the buffer objects
    unsigned int handle[3];
    glGenBuffers(3, handle);        

    glBindBuffer(GL_ARRAY_BUFFER, handle[0]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTeapot->getnVerts()) * sizeof(float), mTeapot->getv(), GL_STATIC_DRAW);

    glBindBuffer(GL_ARRAY_BUFFER, handle[1]);
    glBufferData(GL_ARRAY_BUFFER, (3 * mTeapot->getnVerts()) * sizeof(float), mTeapot->getn(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[2]);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * mTeapot->getnFaces() * sizeof(unsigned int), mTeapot->getelems(), GL_STATIC_DRAW);

    // Setup the VAO
    // Vertex positions
    mFuncs->glBindVertexBuffer(0, handle[0], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(0, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(0, 0);

    // Vertex normals
    mFuncs->glBindVertexBuffer(1, handle[1], 0, sizeof(GLfloat) * 3);
    mFuncs->glVertexAttribFormat(1, 3, GL_FLOAT, GL_FALSE, 0);
    mFuncs->glVertexAttribBinding(1, 1);

    // Indices
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle[2]);

    mFuncs->glBindVertexArray(0);
}

void MyWindow::initMatrices()
{
    ModelMatrix2Sided.translate(1.6f, 0.5f, 0.0f);
    ModelMatrix2Sided.rotate( -90.0f, QVector3D(1.0f,0.0f,0.0f));
    ModelMatrix2Sided.scale(0.50f);

    ModelMatrixSimpleADS.translate(-1.6f, 0.5f, 0.0f);
    ModelMatrixSimpleADS.rotate( -90.0f, QVector3D(1.0f,0.0f,0.0f));
    ModelMatrixSimpleADS.scale(0.50f);

    ViewMatrix.lookAt(QVector3D(2.0f,4.0f,2.0f), QVector3D(0.0f,0.0f,0.0f), QVector3D(0.0f,1.0f,0.0f));
}

void MyWindow::resizeEvent(QResizeEvent *)
{
    mUpdateSize = true;

    ProjectionMatrix.setToIdentity();
    ProjectionMatrix.perspective(70.0f, (float)this->width()/(float)this->height(), 0.3f, 100.0f);
}

void MyWindow::render()
{
    if(!isVisible() || !isExposed())
        return;

    if (!mContext->makeCurrent(this))
        return;

    static bool initialized = false;
    if (!initialized) {
        initialize();
        initialized = true;
    }

    if (mUpdateSize) {
        glViewport(0, 0, size().width(), size().height());
        mUpdateSize = false;
    }

    static float EvolvingVal = 0;
    EvolvingVal += 0.1f;

    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //QMatrix4x4 RotationMatrix;
    //RotationMatrix.rotate(EvolvingVal, QVector3D(0.1f, 0.0f, 0.1f));
    ModelMatrixSimpleADS.rotate(0.3f, QVector3D(0.1f, 0.0f, 0.1f));
    ModelMatrix2Sided.rotate(0.3f, QVector3D(0.1f, 0.0f, 0.1f));

    mFuncs->glBindVertexArray(mVAO);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgramSimpleADS->bind();
    {
        QVector4D worldLight = QVector4D(2.0f, 4.0f, 2.0f, 1.0f);
        mProgramSimpleADS->setUniformValue("Light.Position", ViewMatrix * worldLight );

        mProgramSimpleADS->setUniformValue("Material.Kd", 0.9f, 0.5f, 0.3f);
        mProgramSimpleADS->setUniformValue("Light.Ld", 1.0f, 1.0f, 1.0f);
        mProgramSimpleADS->setUniformValue("Material.Ka", 0.9f, 0.5f, 0.3f);
        mProgramSimpleADS->setUniformValue("Light.La", 0.4f, 0.4f, 0.4f);
        mProgramSimpleADS->setUniformValue("Material.Ks", 0.8f, 0.8f, 0.8f);
        mProgramSimpleADS->setUniformValue("Light.Ls", 1.0f, 1.0f, 1.0f);
        mProgramSimpleADS->setUniformValue("Material.Shininess", 100.0f);

        QMatrix4x4 mv1 = ViewMatrix * ModelMatrixSimpleADS;
        mProgramSimpleADS->setUniformValue("ModelViewMatrix", mv1);
        mProgramSimpleADS->setUniformValue("NormalMatrix", mv1.normalMatrix());
        mProgramSimpleADS->setUniformValue("MVP", ProjectionMatrix * mv1);

        glDrawElements(GL_TRIANGLES, 6 * mTeapot->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgramSimpleADS->release();

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    mProgram2Sides->bind();
    {
        QVector4D worldLight = QVector4D(2.0f, 4.0f, 2.0f, 1.0f);
        mProgram2Sides->setUniformValue("Light.Position", ViewMatrix * worldLight );

        mProgram2Sides->setUniformValue("Material.Kd", 0.9f, 0.5f, 0.3f);
        mProgram2Sides->setUniformValue("Light.Ld", 1.0f, 1.0f, 1.0f);
        mProgram2Sides->setUniformValue("Material.Ka", 0.9f, 0.5f, 0.3f);
        mProgram2Sides->setUniformValue("Light.La", 0.4f, 0.4f, 0.4f);
        mProgram2Sides->setUniformValue("Material.Ks", 0.8f, 0.8f, 0.8f);
        mProgram2Sides->setUniformValue("Light.Ls", 1.0f, 1.0f, 1.0f);
        mProgram2Sides->setUniformValue("Material.Shininess", 100.0f);

        QMatrix4x4 mv2 = ViewMatrix * ModelMatrix2Sided;
        mProgram2Sides->setUniformValue("ModelViewMatrix", mv2);
        mProgram2Sides->setUniformValue("NormalMatrix", mv2.normalMatrix());
        mProgram2Sides->setUniformValue("MVP", ProjectionMatrix * mv2);

        glDrawElements(GL_TRIANGLES, 6 * mTeapot->getnFaces(), GL_UNSIGNED_INT, ((GLubyte *)NULL + (0)));

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);
    }
    mProgram2Sides->release();

    mContext->swapBuffers(this);
}

void MyWindow::initShaders()
{
    QOpenGLShader vShader(QOpenGLShader::Vertex);
    QOpenGLShader fShader(QOpenGLShader::Fragment);    
    QFile         shaderFile;
    QByteArray    shaderSource;

    //Simple ADS
    shaderFile.setFileName(":/vshader_ads.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vertex simple ADS compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader_ads.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag   simple ADS compile: " << fShader.compileSourceCode(shaderSource);

    mProgramSimpleADS = new (QOpenGLShaderProgram);
    mProgramSimpleADS->addShader(&vShader);
    mProgramSimpleADS->addShader(&fShader);
    qDebug() << "shader link tree: " << mProgramSimpleADS->link();

    //2 sided ADS
    shaderFile.setFileName(":/vshader_2sides.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "vertex 2-sided compile: " << vShader.compileSourceCode(shaderSource);

    shaderFile.setFileName(":/fshader_2sides.txt");
    shaderFile.open(QIODevice::ReadOnly);
    shaderSource = shaderFile.readAll();
    shaderFile.close();
    qDebug() << "frag   2-sided compile: " << fShader.compileSourceCode(shaderSource);

    mProgram2Sides = new (QOpenGLShaderProgram);
    mProgram2Sides->addShader(&vShader);
    mProgram2Sides->addShader(&fShader);
    qDebug() << "shader link 2-sided: " << mProgram2Sides->link();

}

void MyWindow::PrepareTexture(GLenum TextureTarget, const QString& FileName, GLuint& TexObject, bool flip)
{
    QImage TexImg;

    if (!TexImg.load(FileName)) qDebug() << "Erreur chargement texture";
    if (flip==true) TexImg=TexImg.mirrored();

    glGenTextures(1, &TexObject);
    glBindTexture(TextureTarget, TexObject);
    glTexImage2D(TextureTarget, 0, GL_RGB, TexImg.width(), TexImg.height(), 0, GL_BGRA, GL_UNSIGNED_BYTE, TexImg.bits());
    glTexParameterf(TextureTarget, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameterf(TextureTarget, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void MyWindow::keyPressEvent(QKeyEvent *keyEvent)
{
    switch(keyEvent->key())
    {
        case Qt::Key_P:
            break;
        case Qt::Key_Up:
            break;
        case Qt::Key_Down:
            break;
        case Qt::Key_Left:
            break;
        case Qt::Key_Right:
            break;
        case Qt::Key_Delete:
            break;
        case Qt::Key_PageDown:
            break;
        case Qt::Key_Home:
            break;
        case Qt::Key_Z:
            break;
        case Qt::Key_Q:
            break;
        case Qt::Key_S:
            break;
        case Qt::Key_D:
            break;
        case Qt::Key_A:
            break;
        case Qt::Key_E:
            break;
        default:
            break;
    }
}

void MyWindow::printMatrix(const QMatrix4x4& mat)
{
    const float *locMat = mat.transposed().constData();

    for (int i=0; i<4; i++)
    {
        qDebug() << locMat[i*4] << " " << locMat[i*4+1] << " " << locMat[i*4+2] << " " << locMat[i*4+3];
    }
}
