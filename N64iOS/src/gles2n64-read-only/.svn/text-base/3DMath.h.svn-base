#ifndef _3DMATH_H
#define _3DMATH_H

#include <string.h>

inline void CopyMatrix( float m0[4][4], float m1[4][4] )
{
    memcpy( m0, m1, 16 * sizeof( float ) );
}

inline void MultMatrix( float m0[4][4], float m1[4][4], float dest[4][4])
{
#ifdef __NEON_OPT
    asm volatile (
	"vld1.32 		{d0, d1}, [%1]!			\n\t"	//q0 = m1
	"vld1.32 		{d2, d3}, [%1]!	    	\n\t"	//q1 = m1+4
	"vld1.32 		{d4, d5}, [%1]!	    	\n\t"	//q2 = m1+8
	"vld1.32 		{d6, d7}, [%1]	    	\n\t"	//q3 = m1+12
	"vld1.32 		{d16, d17}, [%0]!		\n\t"	//q8 = m0
	"vld1.32 		{d18, d19}, [%0]!   	\n\t"	//q9 = m0+4
	"vld1.32 		{d20, d21}, [%0]!   	\n\t"	//q10 = m0+8
	"vld1.32 		{d22, d23}, [%0]    	\n\t"	//q11 = m0+12

	"vmul.f32 		q12, q8, d0[0] 			\n\t"	//q12 = q8 * d0[0]
	"vmul.f32 		q13, q8, d2[0] 		    \n\t"	//q13 = q8 * d2[0]
	"vmul.f32 		q14, q8, d4[0] 		    \n\t"	//q14 = q8 * d4[0]
	"vmul.f32 		q15, q8, d6[0]	 		\n\t"	//q15 = q8 * d6[0]
	"vmla.f32 		q12, q9, d0[1] 			\n\t"	//q12 = q9 * d0[1]
	"vmla.f32 		q13, q9, d2[1] 		    \n\t"	//q13 = q9 * d2[1]
	"vmla.f32 		q14, q9, d4[1] 		    \n\t"	//q14 = q9 * d4[1]
	"vmla.f32 		q15, q9, d6[1] 		    \n\t"	//q15 = q9 * d6[1]
	"vmla.f32 		q12, q10, d1[0] 		\n\t"	//q12 = q10 * d0[0]
	"vmla.f32 		q13, q10, d3[0] 		\n\t"	//q13 = q10 * d2[0]
	"vmla.f32 		q14, q10, d5[0] 		\n\t"	//q14 = q10 * d4[0]
	"vmla.f32 		q15, q10, d7[0] 		\n\t"	//q15 = q10 * d6[0]
	"vmla.f32 		q12, q11, d1[1] 		\n\t"	//q12 = q11 * d0[1]
	"vmla.f32 		q13, q11, d3[1] 		\n\t"	//q13 = q11 * d2[1]
	"vmla.f32 		q14, q11, d5[1] 		\n\t"	//q14 = q11 * d4[1]
	"vmla.f32 		q15, q11, d7[1]	 	    \n\t"	//q15 = q11 * d6[1]

	"vst1.32 		{d24, d25}, [%2]! 		\n\t"	//d = q12
	"vst1.32 		{d26, d27}, [%2]! 	    \n\t"	//d+4 = q13
	"vst1.32 		{d28, d29}, [%2]! 	    \n\t"	//d+8 = q14
	"vst1.32 		{d30, d31}, [%2] 	    \n\t"	//d+12 = q15

	:"+r"(m0), "+r"(m1), "+r"(dest):
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
    "d16", "d17", "d18", "d19", "d20", "d21", "d22", "d23",
    "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
    "memory"
	);
#else
    int i;
    for (i = 0; i < 4; i++)
    {
        dest[0][i] = m0[0][i]*m1[0][0] + m0[1][i]*m1[0][1] + m0[2][i]*m1[0][2] + m0[3][i]*m1[0][3];
        dest[1][i] = m0[0][i]*m1[1][0] + m0[1][i]*m1[1][1] + m0[2][i]*m1[1][2] + m0[3][i]*m1[1][3];
        dest[2][i] = m0[0][i]*m1[2][0] + m0[1][i]*m1[2][1] + m0[2][i]*m1[2][2] + m0[3][i]*m1[2][3];
        dest[3][i] = m0[3][i]*m1[3][3] + m0[2][i]*m1[3][2] + m0[1][i]*m1[3][1] + m0[0][i]*m1[3][0];
    }
#endif
}

inline void MultMatrix( float m0[4][4], float m1[4][4] )
{
    float dst[4][4];
    MultMatrix(m0, m1, dst);
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
#ifdef __NEON_OPT
	asm volatile (
	"vld1.32 		{d0, d1}, [%1]		  	\n\t"	//d8 = {x,y}
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//Q1 = m
	"vld1.32 		{d20, d21}, [%0]!   	\n\t"	//Q2 = m+4
	"vld1.32 		{d22, d23}, [%0]!   	\n\t"	//Q3 = m+8
	"vld1.32 		{d24, d25}, [%0]    	\n\t"	//Q4 = m+12

	"vmul.f32 		q13, q9, d0[0]			\n\t"	//Q5 = Q1*Q0[0]
	"vmla.f32 		q13, q10, d0[1]			\n\t"	//Q5 += Q1*Q0[1]
	"vmla.f32 		q13, q11, d1[0]			\n\t"	//Q5 += Q2*Q0[2]
	"vadd.f32 		q13, q13, q12			\n\t"	//Q5 += Q3*Q0[3]
	"vst1.32 		{d26, d27}, [%1] 		\n\t"	//Q4 = m+12

	: "+r"(mtx) : "r"(vtx)
    : "d0", "d1", "d18","d19","d20","d21","d22","d23","d24","d25",
	"d26", "d27", "memory"
	);
#else
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
#endif
}



inline void TransformVector( float vec[3], float mtx[4][4] )
{
#ifdef __NEON_OPT
	asm volatile (
	"vld1.32 		{d0}, [%1]  			\n\t"	//Q0 = v
	"flds    		s2, [%1, #8]  			\n\t"	//Q0 = v
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//Q1 = m
	"vld1.32 		{d20, d21}, [%0]!	    \n\t"	//Q2 = m+4
	"vld1.32 		{d22, d23}, [%0]	    \n\t"	//Q3 = m+8

	"vmul.f32 		q1, q9, d0[0]			\n\t"	//Q5 = Q1*Q0[0]
	"vmla.f32 		q1, q10, d0[1]			\n\t"	//Q5 += Q1*Q0[1]
	"vmla.f32 		q1, q11, d1[0]			\n\t"	//Q5 += Q2*Q0[2]

	"vst1.32 		{d2}, [%1] 	    	    \n\t"	//Q4 = m+12
	"fsts   		s6, [%1, #8] 	    	\n\t"	//Q4 = m+12
	: "+r"(mtx): "r"(vec)
    : "d0","d1","d2","d3","d18","d19","d20","d21","d22", "d23", "memory"
	);
#else
    vec[0] = mtx[0][0] * vec[0]
           + mtx[1][0] * vec[1]
           + mtx[2][0] * vec[2];
    vec[1] = mtx[0][1] * vec[0]
           + mtx[1][1] * vec[1]
           + mtx[2][1] * vec[2];
    vec[2] = mtx[0][2] * vec[0]
           + mtx[1][2] * vec[1]
           + mtx[2][2] * vec[2];
#endif
}

inline void TransformVectorNormalize(float vec[3], float mtx[4][4])
{
#ifdef __NEON_OPT
	asm volatile (
	"vld1.32 		{d0}, [%1]  			\n\t"	//Q0 = v
	"flds    		s2, [%1, #8]  			\n\t"	//Q0 = v
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//Q1 = m
	"vld1.32 		{d20, d21}, [%0]!	    \n\t"	//Q2 = m+4
	"vld1.32 		{d22, d23}, [%0]	    \n\t"	//Q3 = m+8

	"vmul.f32 		q2, q9, d0[0]			\n\t"	//q2 = q9*Q0[0]
	"vmla.f32 		q2, q10, d0[1]			\n\t"	//Q5 += Q1*Q0[1]
	"vmla.f32 		q2, q11, d1[0]			\n\t"	//Q5 += Q2*Q0[2]

    "vmul.f32 		d0, d4, d4				\n\t"	//d0 = d0*d0
	"vpadd.f32 		d0, d0, d0				\n\t"	//d0 = d[0] + d[1]
    "vmla.f32 		d0, d5, d5				\n\t"	//d0 = d0 + d1*d1

	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d2) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d3) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4

	"vmul.f32 		q2, q2, d0[0]			\n\t"	//d0= d2*d4

	"vst1.32 		{d4}, [%1] 	    	    \n\t"	//Q4 = m+12
	"fsts   		s10, [%1, #8] 	    	\n\t"	//Q4 = m+12
	: "+r"(mtx): "r"(vec)
    : "d0","d1","d2","d3","d18","d19","d20","d21","d22", "d23", "memory"
	);
#else
    float len;

    vec[0] = mtx[0][0] * vec[0]
           + mtx[1][0] * vec[1]
           + mtx[2][0] * vec[2];
    vec[1] = mtx[0][1] * vec[0]
           + mtx[1][1] * vec[1]
           + mtx[2][1] * vec[2];
    vec[2] = mtx[0][2] * vec[0]
           + mtx[1][2] * vec[1]
           + mtx[2][2] * vec[2];
    len = vec[0]*vec[0] + vec[1]*vec[1] + vec[2]*vec[2];
    if (len != 0.0)
    {
        len = sqrtf(len);
        vec[0] /= len;
        vec[1] /= len;
        vec[2] /= len;
    }
#endif
}


inline void Normalize(float v[3])
{
#ifdef __NEON_OPT
	asm volatile (
	"vld1.32 		{d4}, [%0]!	    		\n\t"	//d4={x,y}
	"flds    		s10, [%0]   	    	\n\t"	//d5[0] = z
	"sub    		%0, %0, #8   	    	\n\t"	//d5[0] = z
	"vmul.f32 		d0, d4, d4				\n\t"	//d0= d4*d4
	"vpadd.f32 		d0, d0, d0				\n\t"	//d0 = d[0] + d[1]
    "vmla.f32 		d0, d5, d5				\n\t"	//d0 = d0 + d5*d5

	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d2) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d3) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d4

	"vmul.f32 		q2, q2, d0[0]			\n\t"	//d0= d2*d4
	"vst1.32 		{d4}, [%0]!  			\n\t"	//d2={x0,y0}, d3={z0, w0}
	"fsts    		s10, [%0]     			\n\t"	//d2={x0,y0}, d3={z0, w0}

	:"+r"(v) :
    : "d0", "d1", "d2", "d3", "d4", "d5", "memory"
	);
#else
    float len;

    len = (float)(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (len != 0.0)
    {
        len = (float)sqrt( len );
        v[0] /= (float)len;
        v[1] /= (float)len;
        v[2] /= (float)len;
    }
#endif
}

inline void Normalize2D( float v[2] )
{
#ifdef __NEON_OPT
	asm volatile (
	"vld1.32 		{d4}, [%0]	    		\n\t"	//d4={x0,y0}
	"vmul.f32 		d0, d4, d4				\n\t"	//d0= d4*d4
	"vpadd.f32 		d0, d0					\n\t"	//d0 = d[0] + d[1]

	"vmov.f32 		d1, d0					\n\t"	//d1 = d0
	"vrsqrte.f32 	d0, d0					\n\t"	//d0 = ~ 1.0 / sqrt(d0)
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d3 = (3 - d0 * d2) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3
	"vmul.f32 		d2, d0, d1				\n\t"	//d2 = d0 * d1
	"vrsqrts.f32 	d3, d2, d0				\n\t"	//d4 = (3 - d0 * d3) / 2
	"vmul.f32 		d0, d0, d3				\n\t"	//d0 = d0 * d3

	"vmul.f32 		d4, d4, d0[0]			\n\t"	//d0= d2*d4
	"vst1.32 		{d4}, [%0]	    		\n\t"	//d2={x0,y0}

	:: "r"(v)
    : "d0", "d1", "d2", "d3", "d4", "d5", "memory"
	);
#else
    float len;
    len = (float)sqrt( v[0]*v[0] + v[1]*v[1] );
    v[0] /= len;
    v[1] /= len;
#endif
}

inline float DotProduct( float v0[3], float v1[3] )
{
    float dot;
#ifdef __NEON_OPT
	asm volatile (
	"vld1.32 		{d8}, [%1]!			\n\t"	//d8={x0,y0}
	"vld1.32 		{d10}, [%2]!		\n\t"	//d10={x1,y1}
	"flds 			s18, [%1, #0]	    \n\t"	//d9[0]={z0}
	"flds 			s22, [%2, #0]	    \n\t"	//d11[0]={z1}
	"vmul.f32 		d12, d8, d10		\n\t"	//d0= d2*d4
	"vpadd.f32 		d12, d12, d12		\n\t"	//d0 = d[0] + d[1]
	"vmla.f32 		d12, d9, d11		\n\t"	//d0 = d0 + d3*d5
    "fmrs	        %0, s24	    		\n\t"	//r0 = s0
	: "=r"(dot), "+r"(v0), "+r"(v1):
    : "d8", "d9", "d10", "d11", "d12"

	);
#else
    dot = v0[0]*v1[0] + v0[1]*v1[1] + v0[2]*v1[2];
#endif
    return dot;
}

#endif



