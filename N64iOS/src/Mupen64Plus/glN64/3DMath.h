#ifndef _3DMATH_H
#define _3DMATH_H

#include <string.h>

inline void CopyMatrix( float m0[4][4], float m1[4][4] )
{
    memcpy( m0, m1, 16 * sizeof( float ) );
}

inline void MultMatrix( float m0[4][4], float m1[4][4] )
{
    int i;
    float dst[4][4];

    for (i = 0; i < 4; i++)
    {
        dst[0][i] = m0[0][i]*m1[0][0] + m0[1][i]*m1[0][1] + m0[2][i]*m1[0][2] + m0[3][i]*m1[0][3];
        dst[1][i] = m0[0][i]*m1[1][0] + m0[1][i]*m1[1][1] + m0[2][i]*m1[1][2] + m0[3][i]*m1[1][3];
        dst[2][i] = m0[0][i]*m1[2][0] + m0[1][i]*m1[2][1] + m0[2][i]*m1[2][2] + m0[3][i]*m1[2][3];
        dst[3][i] = m0[3][i]*m1[3][3] + m0[2][i]*m1[3][2] + m0[1][i]*m1[3][1] + m0[0][i]*m1[3][0];
    }
    memcpy( m0, dst, sizeof(float) * 16 );
}

inline void Transpose3x3Matrix( float mtx[4][4] )
{
    float tmp;

    tmp = mtx[0][1];
    mtx[0][1] = mtx[1][0];
    mtx[1][0] = tmp;

    tmp = mtx[0][2];
    mtx[0][2] = mtx[2][0];
    mtx[2][0] = tmp;

    tmp = mtx[1][2];
    mtx[1][2] = mtx[2][1];
    mtx[2][1] = tmp;
}

inline void TransformVertex(float vtx[4], float mtx[4][4])
{
    float x, y, z, w;
    x = vtx[0];
    y = vtx[1];
    z = vtx[2];
    w = vtx[3];

    vtx[0] = x * mtx[0][0] +
             y * mtx[1][0] +
             z * mtx[2][0] +
                 mtx[3][0];

    vtx[1] = x * mtx[0][1] +
             y * mtx[1][1] +
             z * mtx[2][1] +
                 mtx[3][1];

    vtx[2] = x * mtx[0][2] +
             y * mtx[1][2] +
             z * mtx[2][2] +
                 mtx[3][2];

    vtx[3] = x * mtx[0][3] +
             y * mtx[1][3] +
             z * mtx[2][3] +
                 mtx[3][3];
}

inline void TransformVector( float vec[3], float mtx[4][4] )
{
    vec[0] = mtx[0][0] * vec[0]
           + mtx[1][0] * vec[1]
           + mtx[2][0] * vec[2];
    vec[1] = mtx[0][1] * vec[0]
           + mtx[1][1] * vec[1]
           + mtx[2][1] * vec[2];
    vec[2] = mtx[0][2] * vec[0]
           + mtx[1][2] * vec[1]
           + mtx[2][2] * vec[2];
}

inline void Normalize(float v[3])
{
    float len;

    len = (float)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len != 0.0)
    {
        len = (float)sqrt( len );
        v[0] /= (float)len;
        v[1] /= (float)len;
        v[2] /= (float)len;
    }

}

inline void Normalize2D( float v[2] )
{
    float len;

    len = (float)sqrt( v[0]*v[0] + v[1]*v[1] );
    v[0] /= len;
    v[1] /= len;
}

inline float DotProduct( float v0[3], float v1[3] )
{
    float dot;
    dot = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
    return dot;
}

#endif

