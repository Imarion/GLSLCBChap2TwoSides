#include "teapot.h"
#include "teapotdata.h"

#include <cstdio>

#include <QVector4D>
#include <qmath.h>

Teapot::~Teapot()
{
    delete[] v;
    delete[] n;
    delete[] elems;
    delete[] tc;
}

Teapot::Teapot(int grid, const QMatrix4x4 & lidTransform)
{
    nVerts = 32 * (grid + 1) * (grid + 1);
    nFaces = grid * grid * 32;
    v = new float[ nVerts * 3 ];
    n = new float[ nVerts * 3 ];
    tc = new float[ nVerts * 2 ];
    elems = new unsigned int[nFaces * 6];

    generatePatches( v, n, tc, elems, grid );
    moveLid(grid, v, lidTransform);
}

void Teapot::generatePatches(float * v, float * n, float * tc, unsigned int* el, int grid) {
    float * B = new float[4*(grid+1)];  // Pre-computed Bernstein basis functions
    float * dB = new float[4*(grid+1)]; // Pre-computed derivitives of basis functions

    int idx = 0, elIndex = 0, tcIndex = 0;

    // Pre-compute the basis functions  (Bernstein polynomials)
    // and their derivatives
    computeBasisFunctions(B, dB, grid);

    // Build each patch
    // The rim
    buildPatchReflect(0, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true);
    // The body
    buildPatchReflect(1, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true);
    buildPatchReflect(2, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true);
    // The lid
    buildPatchReflect(3, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true);
    buildPatchReflect(4, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true);
    // The bottom
    buildPatchReflect(5, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, true, true);
    // The handle
    buildPatchReflect(6, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true);
    buildPatchReflect(7, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true);
    // The spout
    buildPatchReflect(8, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true);
    buildPatchReflect(9, B, dB, v, n, tc, el, idx, elIndex, tcIndex, grid, false, true);

    delete [] B;
    delete [] dB;
}

void Teapot::moveLid(int grid, float *v, const QMatrix4x4 & lidTransform) {

    int start = 3 * 12 * (grid+1) * (grid+1);
    int end = 3 * 20 * (grid+1) * (grid+1);

    for( int i = start; i < end; i+=3 )
    {
        QVector4D vert = QVector4D(v[i], v[i+1], v[i+2], 1.0f );
        vert = lidTransform * vert;
        v[i] = vert.x();
        v[i+1] = vert.y();
        v[i+2] = vert.z();
    }
}

void Teapot::buildPatchReflect(int patchNum,
                                    float *B, float *dB,
                                    float *v, float *n,
                                    float *tc, unsigned int *el,
                                    int &index, int &elIndex, int &tcIndex, int grid,
                                    bool reflectX, bool reflectY)
{
    QVector3D patch[4][4];
    QVector3D patchRevV[4][4];
    getPatch(patchNum, patch, false);
    getPatch(patchNum, patchRevV, true);

    // Patch without modification
    buildPatch(patch, B, dB, v, n, tc, el,
               index, elIndex, tcIndex, grid, QMatrix3x3(), true);

    // Patch reflected in x
    float matxdata[9] = {
        -1.0f, 0.0f, 0.0f,
         0.0f, 1.0f, 0.0f,
         0.0f, 0.0f, 1.0f};
    if( reflectX ) {
        buildPatch(patchRevV, B, dB, v, n, tc, el,
                   index, elIndex, tcIndex, grid, QMatrix3x3(matxdata), false );
    }

    // Patch reflected in y
    float matydata[9] = {
        1.0f,  0.0f, 0.0f,
        0.0f, -1.0f, 0.0f,
        0.0f,  0.0f, 1.0f
    };

    if( reflectY ) {
        buildPatch(patchRevV, B, dB, v, n, tc, el,
                   index, elIndex, tcIndex, grid, QMatrix3x3(matydata), false );
    }

    // Patch reflected in x and y
    if( reflectX && reflectY ) {
        buildPatch(patch, B, dB, v, n, tc, el,
                   index, elIndex, tcIndex, grid, QMatrix3x3(matxdata), true );
    }
}

void Teapot::buildPatch(QVector3D patch[][4],
                           float *B, float *dB,
                           float *v, float *n, float *tc,
                           unsigned int *el,
                           int &index, int &elIndex, int &tcIndex, int grid, QMatrix3x3 reflect,
                           bool invertNormal)
{
    int startIndex = index / 3;
    float tcFactor = 1.0f / grid;

    for( int i = 0; i <= grid; i++ )
    {
        for( int j = 0 ; j <= grid; j++)
        {
            QVector3D pt   = mattimesvec(reflect, evaluate(i,j,B,patch));
            QVector3D norm = mattimesvec(reflect, evaluateNormal(i,j,B,dB,patch));
            if( invertNormal )
                norm = -norm;

            v[index] = pt.x();
            v[index+1] = pt.y();
            v[index+2] = pt.z();

            n[index] = norm.x();
            n[index+1] = norm.y();
            n[index+2] = norm.z();

            tc[tcIndex] = i * tcFactor;
            tc[tcIndex+1] = j * tcFactor;

            index += 3;
            tcIndex += 2;
        }
    }

    for( int i = 0; i < grid; i++ )
    {
        int iStart = i * (grid+1) + startIndex;
        int nextiStart = (i+1) * (grid+1) + startIndex;
        for( int j = 0; j < grid; j++)
        {
            el[elIndex] = iStart + j;
            el[elIndex+1] = nextiStart + j + 1;
            el[elIndex+2] = nextiStart + j;

            el[elIndex+3] = iStart + j;
            el[elIndex+4] = iStart + j + 1;
            el[elIndex+5] = nextiStart + j + 1;

            elIndex += 6;
        }
    }
}

QVector3D Teapot::mattimesvec(QMatrix3x3 inmat, QVector3D invec)
{
    QGenericMatrix<3,3,float> m1;
    for (int r = 0; r < 3; ++r)
        for (int c = 0; c < 3; ++c)
            m1(r, c) = inmat(r, c);

    float data[3];
    data[0] = invec.x();
    data[1] = invec.y();
    data[2] = invec.z();
    QGenericMatrix<1,3,float> m2(data);

    QGenericMatrix<1,3,float> result = m1 * m2;
    /*
    for (int r = 0; r < 3; ++r)
        qDebug() << result(r, 0);
    */
    return QVector3D(result(0,0), result(1,0), result(2,0));
}

void Teapot::getPatch( int patchNum, QVector3D patch[][4], bool reverseV )
{
    for( int u = 0; u < 4; u++) {          // Loop in u direction
        for( int v = 0; v < 4; v++ ) {     // Loop in v direction
            if( reverseV ) {
                patch[u][v] = QVector3D(
                        TeapotData::cpdata[TeapotData::patchdata[patchNum][u*4+(3-v)]][0],
                        TeapotData::cpdata[TeapotData::patchdata[patchNum][u*4+(3-v)]][1],
                        TeapotData::cpdata[TeapotData::patchdata[patchNum][u*4+(3-v)]][2]
                        );
            } else {
                patch[u][v] = QVector3D(
                        TeapotData::cpdata[TeapotData::patchdata[patchNum][u*4+v]][0],
                        TeapotData::cpdata[TeapotData::patchdata[patchNum][u*4+v]][1],
                        TeapotData::cpdata[TeapotData::patchdata[patchNum][u*4+v]][2]
                        );
            }
        }
    }
}

void Teapot::computeBasisFunctions( float * B, float * dB, int grid ) {
    float inc = 1.0f / grid;
    for( int i = 0; i <= grid; i++ )
    {
        float t = i * inc;
        float tSqr = t * t;
        float oneMinusT = (1.0f - t);
        float oneMinusT2 = oneMinusT * oneMinusT;

        B[i*4 + 0] = oneMinusT * oneMinusT2;
        B[i*4 + 1] = 3.0f * oneMinusT2 * t;
        B[i*4 + 2] = 3.0f * oneMinusT * tSqr;
        B[i*4 + 3] = t * tSqr;

        dB[i*4 + 0] = -3.0f * oneMinusT2;
        dB[i*4 + 1] = -6.0f * t * oneMinusT + 3.0f * oneMinusT2;
        dB[i*4 + 2] = -3.0f * tSqr + 6.0f * t * oneMinusT;
        dB[i*4 + 3] = 3.0f * tSqr;
    }
}


QVector3D Teapot::evaluate( int gridU, int gridV, float *B, QVector3D patch[][4] )
{
    QVector3D p(0.0f,0.0f,0.0f);
    for( int i = 0; i < 4; i++) {
        for( int j = 0; j < 4; j++) {
            p += patch[i][j] * B[gridU*4+i] * B[gridV*4+j];
        }
    }
    return p;
}

QVector3D Teapot::evaluateNormal( int gridU, int gridV, float *B, float *dB, QVector3D patch[][4] )
{
    QVector3D du(0.0f,0.0f,0.0f);
    QVector3D dv(0.0f,0.0f,0.0f);

    for( int i = 0; i < 4; i++) {
        for( int j = 0; j < 4; j++) {
            du += patch[i][j] * dB[gridU*4+i] * B[gridV*4+j];
            dv += patch[i][j] * B[gridU*4+i] * dB[gridV*4+j];
        }
    }

    QVector3D norm = QVector3D::normal(du, dv);

    return norm;
}

float *Teapot::getv()
{
    return v;
}

int Teapot::getnVerts()
{
    return nVerts;
}

float *Teapot::getn()
{
    return n;
}

float *Teapot::gettc()
{
    return tc;
}

unsigned int *Teapot::getelems()
{    
    return elems;
}

int Teapot::getnFaces()
{
    return nFaces;
}

