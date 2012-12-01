#include <math.h>
#include "gles2N64.h"
#include "Debug.h"
#include "Types.h"
#include "RSP.h"
#include "GBI.h"
#include "gSP.h"
#include "gDP.h"
#include "3DMath.h"
#include "OpenGL.h"
#include "CRC.h"
#include <string.h>
#include "convert.h"
#include "S2DEX.h"
#include "VI.h"
#include "DepthBuffer.h"
#include <stdlib.h>

# ifndef min
#  define min(a,b) ((a) < (b) ? (a) : (b))
# endif
# ifndef max
#  define max(a,b) ((a) > (b) ? (a) : (b))
# endif

#ifdef DEBUG
extern u32 uc_crc, uc_dcrc;
extern char uc_str[256];
#endif

void gSPCombineMatrices();


#define gSPFlushTriangles() \
    if ((OGL.numElements > 0) && \
        (RSP.nextCmd != G_TRI1) && \
        (RSP.nextCmd != G_TRI2) && \
        (RSP.nextCmd != G_TRI4) && \
        (RSP.nextCmd != G_QUAD))\
        OGL_DrawTriangles()

gSPInfo gSP;

f32 identityMatrix[4][4] =
{
    { 1.0f, 0.0f, 0.0f, 0.0f },
    { 0.0f, 1.0f, 0.0f, 0.0f },
    { 0.0f, 0.0f, 1.0f, 0.0f },
    { 0.0f, 0.0f, 0.0f, 1.0f }
};

#ifdef __NEON_OPT2

const float __acosf_pi_2 = M_PI_2;

const float __acosf_lut[8] = {
	0.105312459675071, 	//p7
	0.105312459675071, 	//p7
	0.051599985887214, 	//p5
	0.051599985887214, 	//p5
	0.169303418571894,	//p3
	0.169303418571894,	//p3
	0.999954835104825,	//p1
	0.999954835104825	//p1
};

//computes 2 acosf in parallel
void acosf_neon(float x[2], float r[2])
{
	asm volatile (

	"vld1.f32	 	{d0}, %0				\n\t"	//d0 = {x0, x1};
	"vdup.f32	 	d17, %3					\n\t"	//d17 = {pi/2, pi/2};
	"vmov.f32	 	d6, d0					\n\t"	//d6 = d0;
	"vabs.f32	 	d0, d0					\n\t"	//d0 = fabs(d0) ;

	"vmov.f32	 	d5, #0.5				\n\t"	//d5 = 0.5;
	"vmls.f32	 	d5, d0, d5				\n\t"	//d5 = d5 - d0*d5;

	//fast invsqrt approx
	"vmov.f32 		d1, d5					\n\t"	//d1 = d5
	"vrsqrte.f32 	d5, d5					\n\t"	//d5 = ~ 1.0 / sqrt(d5)
	"vmul.f32 		d2, d5, d1				\n\t"	//d2 = d5 * d1
	"vrsqrts.f32 	d3, d2, d5				\n\t"	//d3 = (3 - d5 * d2) / 2
	"vmul.f32 		d5, d5, d3				\n\t"	//d5 = d5 * d3
	"vmul.f32 		d2, d5, d1				\n\t"	//d2 = d5 * d1
	"vrsqrts.f32 	d3, d2, d5				\n\t"	//d3 = (3 - d5 * d3) / 2
	"vmul.f32 		d5, d5, d3				\n\t"	//d5 = d5 * d3

	//fast reciporical approximation
	"vrecpe.f32		d1, d5					\n\t"	//d1 = ~ 1 / d5;
	"vrecps.f32		d2, d1, d5				\n\t"	//d2 = 2.0 - d1 * d5;
	"vmul.f32		d1, d1, d2				\n\t"	//d1 = d1 * d2;
	"vrecps.f32		d2, d1, d5				\n\t"	//d2 = 2.0 - d1 * d5;
	"vmul.f32		d5, d1, d2				\n\t"	//d5 = d1 * d2;

	//if |x| > 0.5 -> ax = sqrt((1-ax)/2), r = pi/2
	"vsub.f32		d5, d0, d5				\n\t"	//d5 = d0 - d5;
	"vmov.f32	 	d2, #0.5				\n\t"	//d2 = 0.5;
	"vcgt.f32	 	d3, d0, d2				\n\t"	//d3 = (d0 > d2);
	"vmov.f32		d1, #3.0 				\n\t"	//d1 = 3.0;
	"vshr.u32	 	d3, #31					\n\t"	//d3 = d3 >> 31;
	"vmov.f32		d16, #1.0 				\n\t"	//d16 = 1.0;
	"vcvt.f32.u32	d3, d3					\n\t"	//d3 = (float) d3;
	"vmls.f32		d0, d5, d3  			\n\t"	//d0 = d0 - d5 * d3;
	"vmul.f32		d7, d17, d3  			\n\t"	//d7 = d17 * d3;
	"vmls.f32		d16, d1, d3 			\n\t"	//d16 = d16 - d1 * d3;

	//polynomial:
	"vld1.32 		{d2, d3}, [%2]	 		\n\t"	//d2 = {p7, p7}, d3 = {p5, p5}
	"vmul.f32 		d1, d0, d0				\n\t"	//d1 = d0 * d0 = {x0^2, x1^2}
	"vld1.32 		{d4}, [%2, #16]			\n\t"	//d4 = {p3, p3}
	"vmla.f32 		d3, d2, d1				\n\t"	//d3 = d3 + d2 * d1;
	"vld1.32	 	{d5}, [%2, #24]			\n\t"	//d5 = {p1, p1}
	"vmla.f32 		d4, d3, d1				\n\t"	//d4 = d4 + d3 * d1;
	"vmla.f32 		d5, d4, d1				\n\t"	//d5 = d5 + d4 * d1;
	"vmul.f32 		d5, d5, d0				\n\t"	//d5 = d5 * d0;

	"vmla.f32 		d7, d5, d16				\n\t"	//d7 = d7 + d5*d16

	"vadd.f32 		d2, d7, d7				\n\t"	//d2 = d7 + d7
	"vclt.f32	 	d3, d6, #0				\n\t"	//d3 = (d6 < 0)
	"vshr.u32	 	d3, #31					\n\t"	//d3 = d3 >> 31;
	"vcvt.f32.u32	d3, d3					\n\t"	//d3 = (float) d3
	"vmls.f32 		d7, d2, d3		    	\n\t"	//d7 = d7 - d2 * d3;

	"vsub.f32 		d0, d17, d7		    	\n\t"	//d0 = d17 - d7

	"vst1.f32 		{d0}, [%1]				\n\t"	//s0 = s3

	::"r"(x), "r"(r), "r"(__acosf_lut),  "r"(__acosf_pi_2)
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7", "d16", "d17"
	);

}
#endif

#ifdef __VEC4_OPT
void gSPTransformVertex4(u32 v, float mtx[4][4])
{
#ifdef __NEON_OPT
    float *ptr = &gSP.vertices[v].x;
	asm volatile (
	"vld1.32 		{d0, d1}, [%1]		  	\n\t"	//q0 = {x,y,z,w}
	"add 		    %1, %1, %2   		  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d2, d3}, [%1]	    	\n\t"	//q1 = {x,y,z,w}
	"add 		    %1, %1, %2 	    	  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d4, d5}, [%1]	        \n\t"	//q2 = {x,y,z,w}
	"add 		    %1, %1, %2 		      	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d6, d7}, [%1]	        \n\t"	//q3 = {x,y,z,w}
    "sub 		    %1, %1, %3   		  	\n\t"	//q0 = {x,y,z,w}

	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//q9 = m
	"vld1.32 		{d20, d21}, [%0]!       \n\t"	//q10 = m
	"vld1.32 		{d22, d23}, [%0]!       \n\t"	//q11 = m
	"vld1.32 		{d24, d25}, [%0]        \n\t"	//q12 = m

	"vmov.f32 		q13, q12    			\n\t"	//q13 = q12
	"vmov.f32 		q14, q12    			\n\t"	//q14 = q12
	"vmov.f32 		q15, q12    			\n\t"	//q15 = q12

	"vmla.f32 		q12, q9, d0[0]			\n\t"	//q12 = q9*d0[0]
	"vmla.f32 		q13, q9, d2[0]			\n\t"	//q13 = q9*d0[0]
	"vmla.f32 		q14, q9, d4[0]			\n\t"	//q14 = q9*d0[0]
	"vmla.f32 		q15, q9, d6[0]			\n\t"	//q15 = q9*d0[0]
	"vmla.f32 		q12, q10, d0[1]			\n\t"	//q12 = q10*d0[1]
	"vmla.f32 		q13, q10, d2[1]			\n\t"	//q13 = q10*d0[1]
	"vmla.f32 		q14, q10, d4[1]			\n\t"	//q14 = q10*d0[1]
	"vmla.f32 		q15, q10, d6[1]			\n\t"	//q15 = q10*d0[1]
	"vmla.f32 		q12, q11, d1[0]			\n\t"	//q12 = q11*d1[0]
	"vmla.f32 		q13, q11, d3[0]			\n\t"	//q13 = q11*d1[0]
	"vmla.f32 		q14, q11, d5[0]			\n\t"	//q14 = q11*d1[0]
	"vmla.f32 		q15, q11, d7[0]			\n\t"	//q15 = q11*d1[0]

	"vst1.32 		{d24, d25}, [%1] 		\n\t"	//q12
	"add 		    %1, %1, %2 		      	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d26, d27}, [%1] 	    \n\t"	//q13
	"add 		    %1, %1, %2 	    	  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d28, d29}, [%1] 	    \n\t"	//q14
	"add 		    %1, %1, %2   		  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d30, d31}, [%1]     	\n\t"	//q15

	: "+&r"(mtx), "+&r"(ptr)
	: "I"(sizeof(SPVertex)), "I"(3 * sizeof(SPVertex))
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "d18","d19", "d20", "d21", "d22", "d23", "d24",
      "d25", "d26", "d27", "d28", "d29", "d30", "d31", "memory"
	);
#else
    float x, y, z, w;
    int i;
    for(i = 0; i < 4; i++)
    {
        x = gSP.vertices[v+i].x;
        y = gSP.vertices[v+i].y;
        z = gSP.vertices[v+i].z;
        w = gSP.vertices[v+i].w;
        gSP.vertices[v+i].x = x * mtx[0][0] + y * mtx[1][0] + z * mtx[2][0] + mtx[3][0];
        gSP.vertices[v+i].y = x * mtx[0][1] + y * mtx[1][1] + z * mtx[2][1] + mtx[3][1];
        gSP.vertices[v+i].z = x * mtx[0][2] + y * mtx[1][2] + z * mtx[2][2] + mtx[3][2];
        gSP.vertices[v+i].w = x * mtx[0][3] + y * mtx[1][3] + z * mtx[2][3] + mtx[3][3];
    }
#endif
}
#endif

#ifdef __VEC4_OPT
void gSPClipVertex4(u32 v)
{
    int i;
    for(i = 0; i < 4; i++){
        gSP.vertices[v+i].xClip = (gSP.vertices[v+i].x > gSP.vertices[v+i].w) - (gSP.vertices[v+i].x < -gSP.vertices[v+i].w);
        gSP.vertices[v+i].yClip = (gSP.vertices[v+i].y > gSP.vertices[v+i].w) - (gSP.vertices[v+i].y < -gSP.vertices[v+i].w);
        if (gSP.vertices[v+i].w <= 0.0f){
            gSP.vertices[v+i].zClip = -1;
        } else {
            gSP.vertices[v+i].zClip = (gSP.vertices[v+i].z > gSP.vertices[v+i].w) - (gSP.vertices[v+i].z < -gSP.vertices[v+i].w);
        }
    }
}
#endif

//4x Transform normal and normalize
#ifdef __VEC4_OPT
void gSPTransformNormal4(u32 v, float mtx[4][4])
{
#ifdef __NEON_OPT
    void *ptr = (void*)&gSP.vertices[v].nx;
	asm volatile (
    "vld1.32 		{d0, d1}, [%1]		  	\n\t"	//q0 = {x,y,z,w}
	"add 		    %1, %1, %2  		  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d2, d3}, [%1]	    	\n\t"	//q1 = {x,y,z,w}
	"add 		    %1, %1, %2  		  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d4, d5}, [%1]	        \n\t"	//q2 = {x,y,z,w}
	"add 		    %1, %1, %2  		  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d6, d7}, [%1]	        \n\t"	//q3 = {x,y,z,w}
    "sub 		    %1, %1, %3  		  	\n\t"	//q0 = {x,y,z,w}

	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//q9 = m
	"vld1.32 		{d20, d21}, [%0]!	    \n\t"	//q10 = m+16
	"vld1.32 		{d22, d23}, [%0]    	\n\t"	//q11 = m+32

	"vmul.f32 		q12, q9, d0[0]			\n\t"	//q12 = q9*d0[0]
	"vmul.f32 		q13, q9, d2[0]			\n\t"	//q13 = q9*d2[0]
    "vmul.f32 		q14, q9, d4[0]			\n\t"	//q14 = q9*d4[0]
    "vmul.f32 		q15, q9, d6[0]			\n\t"	//q15 = q9*d6[0]

    "vmla.f32 		q12, q10, d0[1]			\n\t"	//q12 += q10*q0[1]
    "vmla.f32 		q13, q10, d2[1]			\n\t"	//q13 += q10*q2[1]
    "vmla.f32 		q14, q10, d4[1]			\n\t"	//q14 += q10*q4[1]
    "vmla.f32 		q15, q10, d6[1]			\n\t"	//q15 += q10*q6[1]

	"vmla.f32 		q12, q11, d1[0]			\n\t"	//q12 += q11*d1[0]
	"vmla.f32 		q13, q11, d3[0]			\n\t"	//q13 += q11*d3[0]
	"vmla.f32 		q14, q11, d5[0]			\n\t"	//q14 += q11*d5[0]
	"vmla.f32 		q15, q11, d7[0]			\n\t"	//q15 += q11*d7[0]

    "vmul.f32 		q0, q12, q12			\n\t"	//q0 = q12*q12
    "vmul.f32 		q1, q13, q13			\n\t"	//q1 = q13*q13
    "vmul.f32 		q2, q14, q14			\n\t"	//q2 = q14*q14
    "vmul.f32 		q3, q15, q15			\n\t"	//q3 = q15*q15

    "vpadd.f32 		d0, d0  				\n\t"	//d0[0] = d0[0] + d0[1]
    "vpadd.f32 		d2, d2  				\n\t"	//d2[0] = d2[0] + d2[1]
    "vpadd.f32 		d4, d4  				\n\t"	//d4[0] = d4[0] + d4[1]
    "vpadd.f32 		d6, d6  				\n\t"	//d6[0] = d6[0] + d6[1]

    "vmov.f32    	s1, s2  				\n\t"	//d0[1] = d1[0]
    "vmov.f32 	    s5, s6  				\n\t"	//d2[1] = d3[0]
    "vmov.f32 	    s9, s10  				\n\t"	//d4[1] = d5[0]
    "vmov.f32    	s13, s14  				\n\t"	//d6[1] = d7[0]

    "vpadd.f32 		d0, d0, d2  			\n\t"	//d0 = {d0[0] + d0[1], d2[0] + d2[1]}
    "vpadd.f32 		d1, d4, d6  			\n\t"	//d1 = {d4[0] + d4[1], d6[0] + d6[1]}

	"vmov.f32 		q1, q0					\n\t"	//q1 = q0
	"vrsqrte.f32 	q0, q0					\n\t"	//q0 = ~ 1.0 / sqrt(q0)
	"vmul.f32 		q2, q0, q1				\n\t"	//q2 = q0 * q1
	"vrsqrts.f32 	q3, q2, q0				\n\t"	//q3 = (3 - q0 * q2) / 2
	"vmul.f32 		q0, q0, q3				\n\t"	//q0 = q0 * q3
	"vmul.f32 		q2, q0, q1				\n\t"	//q2 = q0 * q1
	"vrsqrts.f32 	q3, q2, q0				\n\t"	//q3 = (3 - q0 * q2) / 2
	"vmul.f32 		q0, q0, q3				\n\t"	//q0 = q0 * q3

	"vmul.f32 		q3, q15, d1[1]			\n\t"	//q3 = q15*d1[1]
	"vmul.f32 		q2, q14, d1[0]			\n\t"	//q2 = q14*d1[0]
	"vmul.f32 		q1, q13, d0[1]			\n\t"	//q1 = q13*d0[1]
	"vmul.f32 		q0, q12, d0[0]			\n\t"	//q0 = q12*d0[0]

	"vst1.32 		{d0, d1}, [%1]  	    \n\t"	//d0={nx,ny,nz,pad}
	"add 		    %1, %1, %2   		  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d2, d3}, [%1]  	    \n\t"	//d2={nx,ny,nz,pad}
	"add 		    %1, %1, %2  		  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d4, d5}, [%1]  	    \n\t"	//d4={nx,ny,nz,pad}
	"add 		    %1, %1, %2  		  	\n\t"	//q0 = {x,y,z,w}
    "vst1.32 		{d6, d7}, [%1]        	\n\t"	//d6={nx,ny,nz,pad}

    : "+&r"(mtx), "+&r"(ptr)
    : "I"(sizeof(SPVertex)), "I"(3 * sizeof(SPVertex))
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "d16","d17", "d18","d19", "d20", "d21", "d22",
      "d23", "d24", "d25", "d26", "d27", "d28", "d29",
      "d30", "d31", "memory"
	);
#else
    float len, x, y, z;
    int i;
    for(i = 0; i < 4; i++){
        x = gSP.vertices[v+i].nx;
        y = gSP.vertices[v+i].ny;
        z = gSP.vertices[v+i].nz;

        gSP.vertices[v+i].nx = mtx[0][0]*x + mtx[1][0]*y + mtx[2][0]*z;
        gSP.vertices[v+i].ny = mtx[0][1]*x + mtx[1][1]*y + mtx[2][1]*z;
        gSP.vertices[v+i].nz = mtx[0][2]*x + mtx[1][2]*y + mtx[2][2]*z;
        len =   gSP.vertices[v+i].nx*gSP.vertices[v+i].nx +
                gSP.vertices[v+i].ny*gSP.vertices[v+i].ny +
                gSP.vertices[v+i].nz*gSP.vertices[v+i].nz;
        if (len != 0.0)
        {
            len = sqrtf(len);
            gSP.vertices[v+i].nx /= len;
            gSP.vertices[v+i].ny /= len;
            gSP.vertices[v+i].nz /= len;
        }
    }
#endif
}
#endif

//sizeof(light) = 24
#ifdef __VEC4_OPT
void __attribute__((noinline))
gSPLightVertex4(u32 v)
{
#ifdef __NEON_OPT
    volatile float result[16];

 	volatile int i = gSP.numLights;
    volatile int tmp = 0;
    volatile void *ptr0 = &(gSP.lights[0].r);
    volatile void *ptr1 = &(gSP.vertices[v].nx);
    volatile void *ptr2 = result;
	volatile void *ptr3 = gSP.matrix.modelView[gSP.matrix.modelViewi];
	asm volatile (
    "vld1.32 		{d0, d1}, [%1]		  	\n\t"	//q0 = {x,y,z,w}
	"add 		    %1, %1, %2 		      	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d2, d3}, [%1]	    	\n\t"	//q1 = {x,y,z,w}
	"add 		    %1, %1, %2 	    	  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d4, d5}, [%1]	        \n\t"	//q2 = {x,y,z,w}
	"add 		    %1, %1, %2   		  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d6, d7}, [%1]	        \n\t"	//q3 = {x,y,z,w}
    "sub 		    %1, %1, %3   		  	\n\t"	//q0 = {x,y,z,w}

	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//q9 = m
	"vld1.32 		{d20, d21}, [%0]!	    \n\t"	//q10 = m+16
	"vld1.32 		{d22, d23}, [%0]    	\n\t"	//q11 = m+32

	"vmul.f32 		q12, q9, d0[0]			\n\t"	//q12 = q9*d0[0]
	"vmul.f32 		q13, q9, d2[0]			\n\t"	//q13 = q9*d2[0]
    "vmul.f32 		q14, q9, d4[0]			\n\t"	//q14 = q9*d4[0]
    "vmul.f32 		q15, q9, d6[0]			\n\t"	//q15 = q9*d6[0]

    "vmla.f32 		q12, q10, d0[1]			\n\t"	//q12 += q10*q0[1]
    "vmla.f32 		q13, q10, d2[1]			\n\t"	//q13 += q10*q2[1]
    "vmla.f32 		q14, q10, d4[1]			\n\t"	//q14 += q10*q4[1]
    "vmla.f32 		q15, q10, d6[1]			\n\t"	//q15 += q10*q6[1]

	"vmla.f32 		q12, q11, d1[0]			\n\t"	//q12 += q11*d1[0]
	"vmla.f32 		q13, q11, d3[0]			\n\t"	//q13 += q11*d3[0]
	"vmla.f32 		q14, q11, d5[0]			\n\t"	//q14 += q11*d5[0]
	"vmla.f32 		q15, q11, d7[0]			\n\t"	//q15 += q11*d7[0]

    "vmul.f32 		q0, q12, q12			\n\t"	//q0 = q12*q12
    "vmul.f32 		q1, q13, q13			\n\t"	//q1 = q13*q13
    "vmul.f32 		q2, q14, q14			\n\t"	//q2 = q14*q14
    "vmul.f32 		q3, q15, q15			\n\t"	//q3 = q15*q15

    "vpadd.f32 		d0, d0  				\n\t"	//d0[0] = d0[0] + d0[1]
    "vpadd.f32 		d2, d2  				\n\t"	//d2[0] = d2[0] + d2[1]
    "vpadd.f32 		d4, d4  				\n\t"	//d4[0] = d4[0] + d4[1]
    "vpadd.f32 		d6, d6  				\n\t"	//d6[0] = d6[0] + d6[1]

    "vmov.f32    	s1, s2  				\n\t"	//d0[1] = d1[0]
    "vmov.f32 	    s5, s6  				\n\t"	//d2[1] = d3[0]
    "vmov.f32 	    s9, s10  				\n\t"	//d4[1] = d5[0]
    "vmov.f32    	s13, s14  				\n\t"	//d6[1] = d7[0]

    "vpadd.f32 		d0, d0, d2  			\n\t"	//d0 = {d0[0] + d0[1], d2[0] + d2[1]}
    "vpadd.f32 		d1, d4, d6  			\n\t"	//d1 = {d4[0] + d4[1], d6[0] + d6[1]}

	"vmov.f32 		q1, q0					\n\t"	//q1 = q0
	"vrsqrte.f32 	q0, q0					\n\t"	//q0 = ~ 1.0 / sqrt(q0)
	"vmul.f32 		q2, q0, q1				\n\t"	//q2 = q0 * q1
	"vrsqrts.f32 	q3, q2, q0				\n\t"	//q3 = (3 - q0 * q2) / 2
	"vmul.f32 		q0, q0, q3				\n\t"	//q0 = q0 * q3
	"vmul.f32 		q2, q0, q1				\n\t"	//q2 = q0 * q1
	"vrsqrts.f32 	q3, q2, q0				\n\t"	//q3 = (3 - q0 * q2) / 2
	"vmul.f32 		q0, q0, q3				\n\t"	//q0 = q0 * q3

	"vmul.f32 		q3, q15, d1[1]			\n\t"	//q3 = q15*d1[1]
	"vmul.f32 		q2, q14, d1[0]			\n\t"	//q2 = q14*d1[0]
	"vmul.f32 		q1, q13, d0[1]			\n\t"	//q1 = q13*d0[1]
	"vmul.f32 		q0, q12, d0[0]			\n\t"	//q0 = q12*d0[0]

	"vst1.32 		{d0, d1}, [%1]  	    \n\t"	//d0={nx,ny,nz,pad}
	"add 		    %1, %1, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d2, d3}, [%1]  	    \n\t"	//d2={nx,ny,nz,pad}
	"add 		    %1, %1, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d4, d5}, [%1]  	    \n\t"	//d4={nx,ny,nz,pad}
	"add 		    %1, %1, %2 		  	    \n\t"	//q0 = {x,y,z,w}
    "vst1.32 		{d6, d7}, [%1]        	\n\t"	//d6={nx,ny,nz,pad}

    : "+&r"(ptr3), "+&r"(ptr1)
    : "I"(sizeof(SPVertex)), "I"(3 * sizeof(SPVertex))
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "d16","d17", "d18","d19", "d20", "d21", "d22",
      "d23", "d24", "d25", "d26", "d27", "d28", "d29",
      "d30", "d31", "memory"
	);
    asm volatile (

    "mov    		%0, %5        			\n\t"	//r0=sizeof(light)
    "mla    		%0, %1, %0, %2 			\n\t"	//r0=r1*r0+r2

    "vmov.f32 		q8, q0  			    \n\t"	//q8=q0
    "vmov.f32 		q9, q1  			    \n\t"	//q9=q1
    "vmov.f32 		q10, q2  			    \n\t"	//q10=q2
    "vmov.f32 		q11, q3  			    \n\t"	//q11=q3

    "vld1.32 		{d0}, [%0]			    \n\t"	//d0={r,g}
    "flds   		s2, [%0, #8]			\n\t"	//d1[0]={b}
    "vmov.f32 		q1, q0  			    \n\t"	//q1=q0
    "vmov.f32 		q2, q0  			    \n\t"	//q2=q0
    "vmov.f32 		q3, q0  			    \n\t"	//q3=q0

    "vmov.f32 		q15, #0.0     			\n\t"	//q15=0
    "vdup.f32 		q15, d30[0]     		\n\t"	//q15=d30[0]

    "cmp     		%1, #0       			\n\t"	//
    "beq     		2f             			\n\t"	//(r1==0) goto 2

    "1:                          			\n\t"	//
    "vld1.32 		{d8}, [%2]!	        	\n\t"	//d8={r,g}
    "flds    		s18, [%2]   	    	\n\t"	//q9[0]={b}
    "add    		%2, %2, #4   	    	\n\t"	//q9[0]={b}
    "vld1.32 		{d10}, [%2]!    		\n\t"	//d10={x,y}
    "flds    		s22, [%2]   	    	\n\t"	//d11[0]={z}
    "add    		%2, %2, #4   	    	\n\t"	//q9[0]={b}

    "vmov.f32 		q13, q5  	       		\n\t"	//q13 = q5
    "vmov.f32 		q12, q4  	       		\n\t"	//q12 = q4

    "vmul.f32 		q4, q8, q13	       		\n\t"	//q4 = q8*q13
    "vmul.f32 		q5, q9, q13	       		\n\t"	//q5 = q9*q13
    "vmul.f32 		q6, q10, q13	        \n\t"	//q6 = q10*q13
    "vmul.f32 		q7, q11, q13	       	\n\t"	//q7 = q11*q13

    "vpadd.f32 		d8, d8  				\n\t"	//d8[0] = d8[0] + d8[1]
    "vpadd.f32 		d10, d10  				\n\t"	//d10[0] = d10[0] + d10[1]
    "vpadd.f32 		d12, d12  				\n\t"	//d12[0] = d12[0] + d12[1]
    "vpadd.f32 		d14, d14  				\n\t"	//d14[0] = d14[0] + d14[1]

    "vmov.f32    	s17, s18  				\n\t"	//d8[1] = d9[0]
    "vmov.f32    	s21, s22  				\n\t"	//d10[1] = d11[0]
    "vmov.f32    	s25, s26  				\n\t"	//d12[1] = d13[0]
    "vmov.f32    	s29, s30  				\n\t"	//d14[1] = d15[0]

    "vpadd.f32 		d8, d8, d10  			\n\t"	//d8 = {d8[0] + d8[1], d10[0] + d10[1]}
    "vpadd.f32 		d9, d12, d14  			\n\t"	//d9 = {d12[0] + d12[1], d14[0] + d14[1]}

    "vmax.f32 		q4, q4, q15  			\n\t"	//q4=max(q4, 0)

    "vmla.f32 		q0, q12, d8[0]  		\n\t"	//q0 +=
    "vmla.f32 		q1, q12, d8[1]  		\n\t"	//d1 = {d4[0] + d4[1], d6[0] + d6[1]}
    "vmla.f32 		q2, q12, d9[0]  		\n\t"	//d1 = {d4[0] + d4[1], d6[0] + d6[1]}
    "vmla.f32 		q3, q12, d9[1]  		\n\t"	//d1 = {d4[0] + d4[1], d6[0] + d6[1]}

    "subs     		%1, %1, #1       		\n\t"	//r1=r1 - 1
    "bne     		1b                 		\n\t"	//(r1!=0) goto 1

    "2:                          			\n\t"	//
#ifdef __PACKVERTEX_OPT
    "vmov.i32        q4, #255	        	\n\t"	//q4 = 255
    "vcvt.f32.u32    q4, q4	        	    \n\t"	//q4 = (float)q4
#else
    "vmov.f32        q4, #1.0	        	\n\t"	//
#endif
    "vmin.f32 		q0, q0, q4  	        \n\t"	//
    "vmin.f32 		q1, q1, q4  	        \n\t"	//
    "vmin.f32 		q2, q2, q4  	        \n\t"	//
    "vmin.f32 		q3, q3, q4  	        \n\t"	//
    "vst1.32 		{d0, d1}, [%4]!	        \n\t"	//
    "vst1.32 		{d2, d3}, [%4]! 	    \n\t"	//
    "vst1.32 		{d4, d5}, [%4]!	        \n\t"	//
    "vst1.32 		{d6, d7}, [%4]     	    \n\t"	//

    :: "r"(tmp), "r"(i), "r"(ptr0), "r"(ptr1), "r"(ptr2),
        "I"(sizeof(SPLight))
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "d8", "d9", "d10", "d11", "d12", "d13", "d14", "d15",
      "d16","d17", "d18","d19", "d20", "d21", "d22", "d23",
      "d24", "d25", "d26", "d27", "d28", "d29", "d30", "d31",
      "memory", "cc"
    );
    gSP.vertices[v].r = result[0];
    gSP.vertices[v].g = result[1];
    gSP.vertices[v].b = result[2];
    gSP.vertices[v+1].r = result[4];
    gSP.vertices[v+1].g = result[5];
    gSP.vertices[v+1].b = result[6];
    gSP.vertices[v+2].r = result[8];
    gSP.vertices[v+2].g = result[9];
    gSP.vertices[v+2].b = result[10];
    gSP.vertices[v+3].r = result[12];
    gSP.vertices[v+3].g = result[13];
    gSP.vertices[v+3].b = result[14];
#else
    gSPTransformNormal4(v, gSP.matrix.modelView[gSP.matrix.modelViewi]);
    for(int j = 0; j < 4; j++)
    {
        f32 r,g,b;
        r = gSP.lights[gSP.numLights].r;
        g = gSP.lights[gSP.numLights].g;
        b = gSP.lights[gSP.numLights].b;
        for (int i = 0; i < gSP.numLights; i++)
        {
            f32 intensity = DotProduct( &gSP.vertices[v+j].nx, &gSP.lights[i].x );
            if (intensity < 0.0f) intensity = 0.0f;
            gSP.vertices[v+j].r += gSP.lights[i].r * intensity;
            gSP.vertices[v+j].g += gSP.lights[i].g * intensity;
            gSP.vertices[v+j].b += gSP.lights[i].b * intensity;
        }
#ifdef __PACKVERTEX_OPT
        gSP.vertices[v+j].r = min(255.0f, r);
        gSP.vertices[v+j].g = min(255.0f, g);
        gSP.vertices[v+j].b = min(255.0f, b);
#else
        gSP.vertices[v].r = min(1.0, r);
        gSP.vertices[v].g = min(1.0, g);
        gSP.vertices[v].b = min(1.0, b);
#endif
    }
#endif
}
#endif

#ifdef __VEC4_OPT
void gSPBillboardVertex4(u32 v)
{
#if __NEON_OPT
    void *ptr0 = (void*)&gSP.vertices[v].x;
    void *ptr1 = (void*)&gSP.vertices[0].x;
    asm volatile (

    "vld1.32 		{d0, d1}, [%0]		  	\n\t"	//q0 = {x,y,z,w}
	"add 		    %0, %0, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d2, d3}, [%0]	    	\n\t"	//q1 = {x,y,z,w}
	"add 		    %0, %0, %2 		      	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d4, d5}, [%0]	        \n\t"	//q2 = {x,y,z,w}
	"add 		    %0, %0, %2 	    	  	\n\t"	//q0 = {x,y,z,w}
	"vld1.32 		{d6, d7}, [%0]	        \n\t"	//q3 = {x,y,z,w}
    "sub 		    %0, %0, %3   		  	\n\t"	//q0 = {x,y,z,w}

    "vld1.32 		{d16, d17}, [%1]		\n\t"	//q2={x1,y1,z1,w1}
    "vadd.f32 		q0, q0, q8 			    \n\t"	//q1=q1+q1
    "vadd.f32 		q1, q1, q8 			    \n\t"	//q1=q1+q1
    "vadd.f32 		q2, q2, q8 			    \n\t"	//q1=q1+q1
    "vadd.f32 		q3, q3, q8 			    \n\t"	//q1=q1+q1
    "vst1.32 		{d0, d1}, [%0] 		    \n\t"	//
    "add 		    %0, %0, %2  		  	\n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d2, d3}, [%0]          \n\t"	//
    "add 		    %0, %0, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d4, d5}, [%0]          \n\t"	//
    "add 		    %0, %0, %2 		  	    \n\t"	//q0 = {x,y,z,w}
	"vst1.32 		{d6, d7}, [%0]          \n\t"	//
    : "+&r"(ptr0), "+&r"(ptr1)
    : "I"(sizeof(SPVertex)), "I"(3 * sizeof(SPVertex))
    : "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "d16", "d17", "memory"
    );
#else
    gSP.vertices[v].x += gSP.vertices[0].x;
    gSP.vertices[v].y += gSP.vertices[0].y;
    gSP.vertices[v].z += gSP.vertices[0].z;
    gSP.vertices[v].w += gSP.vertices[0].w;
    gSP.vertices[v+1].x += gSP.vertices[0].x;
    gSP.vertices[v+1].y += gSP.vertices[0].y;
    gSP.vertices[v+1].z += gSP.vertices[0].z;
    gSP.vertices[v+1].w += gSP.vertices[0].w;
    gSP.vertices[v+2].x += gSP.vertices[0].x;
    gSP.vertices[v+2].y += gSP.vertices[0].y;
    gSP.vertices[v+2].z += gSP.vertices[0].z;
    gSP.vertices[v+2].w += gSP.vertices[0].w;
    gSP.vertices[v+3].x += gSP.vertices[0].x;
    gSP.vertices[v+3].y += gSP.vertices[0].y;
    gSP.vertices[v+3].z += gSP.vertices[0].z;
    gSP.vertices[v+3].w += gSP.vertices[0].w;
#endif
}
#endif

#ifdef __VEC4_OPT
void gSPProcessVertex4( u32 v )
{
    if (gSP.changed & CHANGED_MATRIX)
        gSPCombineMatrices();

    gSPTransformVertex4(v, gSP.matrix.combined );

    if (gSP.matrix.billboard)
        gSPBillboardVertex4(v);

    if (!(gSP.geometryMode & G_ZBUFFER))
    {
        gSP.vertices[v].z = -gSP.vertices[v].w;
        gSP.vertices[v+1].z = -gSP.vertices[v+1].w;
        gSP.vertices[v+2].z = -gSP.vertices[v+2].w;
        gSP.vertices[v+3].z = -gSP.vertices[v+3].w;
    }

    if (gSP.geometryMode & G_LIGHTING)
    {
        if (OGL.enableLighting)
        {
            gSPLightVertex4(v);
        }
        else
        {
            gSP.vertices[v].r = gSP.lights[gSP.numLights].r;
            gSP.vertices[v].g = gSP.lights[gSP.numLights].g;
            gSP.vertices[v].b = gSP.lights[gSP.numLights].b;
            gSP.vertices[v+1].r = gSP.lights[gSP.numLights].r;
            gSP.vertices[v+1].g = gSP.lights[gSP.numLights].g;
            gSP.vertices[v+1].b = gSP.lights[gSP.numLights].b;
            gSP.vertices[v+2].r = gSP.lights[gSP.numLights].r;
            gSP.vertices[v+2].g = gSP.lights[gSP.numLights].g;
            gSP.vertices[v+2].b = gSP.lights[gSP.numLights].b;
            gSP.vertices[v+3].r = gSP.lights[gSP.numLights].r;
            gSP.vertices[v+3].g = gSP.lights[gSP.numLights].g;
            gSP.vertices[v+3].b = gSP.lights[gSP.numLights].b;
        }

        if (gSP.geometryMode & G_TEXTURE_GEN)
        {
            gSPTransformNormal4(v, gSP.matrix.projection);

            if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
            {
                gSP.vertices[v].s = acosf(gSP.vertices[v].nx) * 325.94931f;
                gSP.vertices[v].t = acosf(gSP.vertices[v].ny) * 325.94931f;
                gSP.vertices[v+1].s = acosf(gSP.vertices[v+1].nx) * 325.94931f;
                gSP.vertices[v+1].t = acosf(gSP.vertices[v+1].ny) * 325.94931f;
                gSP.vertices[v+2].s = acosf(gSP.vertices[v+2].nx) * 325.94931f;
                gSP.vertices[v+2].t = acosf(gSP.vertices[v+2].ny) * 325.94931f;
                gSP.vertices[v+3].s = acosf(gSP.vertices[v+3].nx) * 325.94931f;
                gSP.vertices[v+3].t = acosf(gSP.vertices[v+3].ny) * 325.94931f;
            }
            else // G_TEXTURE_GEN
            {
                gSP.vertices[v].s = (gSP.vertices[v].nx + 1.0f) * 512.0f;
                gSP.vertices[v].t = (gSP.vertices[v].ny + 1.0f) * 512.0f;
                gSP.vertices[v+1].s = (gSP.vertices[v+1].nx + 1.0f) * 512.0f;
                gSP.vertices[v+1].t = (gSP.vertices[v+1].ny + 1.0f) * 512.0f;
                gSP.vertices[v+2].s = (gSP.vertices[v+2].nx + 1.0f) * 512.0f;
                gSP.vertices[v+2].t = (gSP.vertices[v+2].ny + 1.0f) * 512.0f;
                gSP.vertices[v+3].s = (gSP.vertices[v+3].nx + 1.0f) * 512.0f;
                gSP.vertices[v+3].t = (gSP.vertices[v+3].ny + 1.0f) * 512.0f;
            }
        }
    }

    if (OGL.enableClipping)
        gSPClipVertex4(v);
}
#endif

void gSPClipVertex(u32 v)
{
#if defined(__NEON_OPT)
    int tmp;
    asm volatile (
    "vld1.32 		{d0, d1}, [%1]	    		\n\t"	//q0={x, y, z, w}
    "vdup.f32 		q8, d1[1]	        		\n\t"	//q8={w,w,w,w}
    "vneg.f32 		q9, q8	              	    \n\t"	//q9={-w,-w,-w,-w}
	"ldr 		    %0, [%1, #12]	        	\n\t"	//r1=w
    "vcgt.f32 		q1, q0, q8	    		    \n\t"	//q1={x>w, y>w, z>w, w>w}
    "vclt.f32 		q2, q0, q9	    		    \n\t"	//q2={x<-w, y<-w, z<-w, w<-w}
    "vshr.u32 		q1, q1, #31	    		    \n\t"	//q1=q1>>31
    "vshr.u32 		q2, q2, #31	    		    \n\t"	//q2=q2>>31
    "cmp 		    %0, #0	                	\n\t"	//w<0

	"mvn       		%0, #0	    			    \n\t"	//r0=0xFFFFFFFF
	"vmov.i32 		q0, #1	    			    \n\t"	//q0=1
	"vadd.s32 		q0, q0, q1	    			\n\t"	//q0=q0+q1
	"vmls.s32 		q1, q2, q0	    		    \n\t"	//q1=q1-q2*d0
    "vst1.32 		{d2}, [%2]	         		\n\t"	//q0={x, y, z, w}

    "fstspl 	    s6, [%2, #8]	  	        \n\t"	//
	"strmi 		    %0, [%2, #8]		        \n\t"	//
    :"=&r"(tmp) : "r"(&gSP.vertices[v].x), "r"(&gSP.vertices[v].xClip)
    : "d0","d1","d2","d3","d4","d5","d6", "d7", "d16","d17",
    "d18","d19","memory", "cc"
    );
#else
    gSP.vertices[v].xClip = (gSP.vertices[v].x > gSP.vertices[v].w) - (gSP.vertices[v].x < -gSP.vertices[v].w);
    gSP.vertices[v].yClip = (gSP.vertices[v].y > gSP.vertices[v].w) - (gSP.vertices[v].y < -gSP.vertices[v].w);
    if (gSP.vertices[v].w <= 0.0f){
        gSP.vertices[v].zClip = -1;
    } else {
        gSP.vertices[v].zClip = (gSP.vertices[v].z > gSP.vertices[v].w) - (gSP.vertices[v].z < -gSP.vertices[v].w);
    }
#endif
}

void __attribute__((noinline))
gSPLightVertex(u32 v)
{
#ifdef __NEON_OPT
    volatile float result[4];

    volatile int tmp = 0;
    volatile int i = gSP.numLights;
    volatile void *ptr0 = &gSP.lights[0].r;
    volatile void *ptr1 = &gSP.vertices[v].nx;
    volatile void *ptr2 = result;;
    volatile void *ptr3 = gSP.matrix.modelView[gSP.matrix.modelViewi];

	asm volatile (
	"vld1.32 		{d0, d1}, [%1]  		\n\t"	//Q0 = v
	"vld1.32 		{d18, d19}, [%0]!		\n\t"	//Q1 = m
	"vld1.32 		{d20, d21}, [%0]!	    \n\t"	//Q2 = m+4
	"vld1.32 		{d22, d23}, [%0]	    \n\t"	//Q3 = m+8

	"vmul.f32 		q2, q9, d0[0]			\n\t"	//q2 = q9*Q0[0]
	"vmla.f32 		q2, q10, d0[1]			\n\t"	//Q5 += Q1*Q0[1]
	"vmla.f32 		q2, q11, d1[0]			\n\t"	//Q5 += Q2*Q0[2]

    "vmul.f32 		d0, d4, d4				\n\t"	//d0 = d0*d0
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

	"vmul.f32 		q1, q2, d0[0]			\n\t"	//q1 = d2*d4

	"vst1.32 		{d2, d3}, [%1]  	    \n\t"	//d0={nx,ny,nz,pad}

	: "+&r"(ptr3): "r"(ptr1)
    : "d0","d1","d2","d3","d18","d19","d20","d21","d22", "d23", "memory"
	);

    asm volatile (
    "mov    		%0, #24        			\n\t"	//r0=24
    "mla    		%0, %1, %0, %2 			\n\t"	//r0=r1*r0+r2

    "vld1.32 		{d0}, [%0]!	    		\n\t"	//d0={r,g}
    "flds   		s2, [%0]	        	\n\t"	//d1[0]={b}
    "cmp            %0, #0     		        \n\t"	//
    "beq            2f       		        \n\t"	//(r1==0) goto 2

    "1:                        		        \n\t"	//
    "vld1.32 		{d4}, [%2]!	        	\n\t"	//d4={r,g}
    "flds    		s10, [%2]	        	\n\t"	//q5[0]={b}
    "add 		    %2, %2, #4 		        \n\t"	//r2+=4
    "vld1.32 		{d6}, [%2]!	    		\n\t"	//d6={x,y}
    "flds    		s14, [%2]	        	\n\t"	//d7[0]={z}
    "add 		    %2, %2, #4 		        \n\t"	//r2+=4
    "vmul.f32 		d6, d2, d6 			    \n\t"	//d6=d2*d6
    "vpadd.f32 		d6, d6   			    \n\t"	//d6=d6[0]+d6[1]
    "vmla.f32 		d6, d3, d7 			    \n\t"	//d6=d6+d3*d7
    "vmov.f32 		d7, #0.0     			\n\t"	//d7=0
    "vmax.f32 		d6, d6, d7   		    \n\t"	//d6=max(d6, d7)
    "vmla.f32 		q0, q2, d6[0] 		    \n\t"	//q0=q0+q2*d6[0]
    "sub 		    %1, %1, #1 		        \n\t"	//r0=r0-1
    "cmp 		    %1, #0   		        \n\t"	//r0=r0-1
    "bgt 		    1b 		                \n\t"	//(r1!=0) ? goto 1
    "b  		    2f 		                \n\t"	//(r1!=0) ? goto 1
    "2:                        	    		\n\t"	//
#ifdef __PACKVERTEX_OPT
    "vmov.i32        q1, #255	        	\n\t"	//d2 = 255
    "vcvt.f32.u32    q1, q1	        	    \n\t"	//d2 = (float)d2
#else
    "vmov.f32        q1, #1.0	        	\n\t"	//
#endif
    "vmin.f32        q0, q0, q1	        	\n\t"	//
    "vst1.32        {d0, d1}, [%3]	    	\n\t"	//

    : "+&r"(tmp), "+&r"(i), "+&r"(ptr0), "+&r"(ptr2)
    :: "d0", "d1", "d2", "d3", "d4", "d5", "d6", "d7",
      "d16", "memory", "cc"
    );
    gSP.vertices[v].r = result[0];
    gSP.vertices[v].g = result[1];
    gSP.vertices[v].b = result[2];

#else

    TransformVectorNormalize( &gSP.vertices[v].nx, gSP.matrix.modelView[gSP.matrix.modelViewi] );

    f32 r, g, b;
    r = gSP.lights[gSP.numLights].r;
    g = gSP.lights[gSP.numLights].g;
    b = gSP.lights[gSP.numLights].b;
    for (int i = 0; i < gSP.numLights; i++)
    {
        f32 intensity = DotProduct( &gSP.vertices[v].nx, &gSP.lights[i].x );
        if (intensity < 0.0f) intensity = 0.0f;
        r += gSP.lights[i].r * intensity;
        g += gSP.lights[i].g * intensity;
        b += gSP.lights[i].b * intensity;
    }
#ifdef __PACKVERTEX_OPT
    gSP.vertices[v].r = min(255.0f, r);
    gSP.vertices[v].g = min(255.0f, g);
    gSP.vertices[v].b = min(255.0f, b);
#else
    gSP.vertices[v].r = min(1.0, r);
    gSP.vertices[v].g = min(1.0, g);
    gSP.vertices[v].b = min(1.0, b);
#endif
#endif
}


void gSPLoadUcodeEx( u32 uc_start, u32 uc_dstart, u16 uc_dsize )
{
    RSP.PCi = 0;
    gSP.matrix.modelViewi = 0;
    gSP.changed |= CHANGED_MATRIX;
    gSP.status[0] = gSP.status[1] = gSP.status[2] = gSP.status[3] = 0;

    if ((((uc_start & 0x1FFFFFFF) + 4096) > RDRAMSize) || (((uc_dstart & 0x1FFFFFFF) + uc_dsize) > RDRAMSize))
    {
#ifdef DEBUG
            DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to loud ucode out of invalid address\n" );
            DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLoadUcodeEx( 0x%08X, 0x%08X, %i );\n", uc_start, uc_dstart, uc_dsize );
#endif
            return;
    }

    MicrocodeInfo *ucode = GBI_DetectMicrocode( uc_start, uc_dstart, uc_dsize );
    if (ucode->type != (u32)-1) last_good_ucode = ucode->type;
    if (ucode->type != NONE){
        GBI_MakeCurrent( ucode );
    } else {
#ifdef RSPTHREAD
        RSP.threadIdle = 0;
        RSP.threadEvents.push(RSPMSG_CLOSE);
#else
        puts( "Warning: Unknown UCODE!!!" );
#endif
    }
#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Unknown microcode: 0x%08X, 0x%08X, %s\n", uc_crc, uc_dcrc, uc_str );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLoadUcodeEx( 0x%08X, 0x%08X, %i );\n", uc_start, uc_dstart, uc_dsize );
#endif
}

void gSPCombineMatrices()
{
    MultMatrix(gSP.matrix.projection, gSP.matrix.modelView[gSP.matrix.modelViewi], gSP.matrix.combined);
    gSP.changed &= ~CHANGED_MATRIX;
}

void gSPProcessVertex( u32 v )
{
    f32 intensity;
    f32 r, g, b;

    if (gSP.changed & CHANGED_MATRIX)
        gSPCombineMatrices();

    TransformVertex( &gSP.vertices[v].x, gSP.matrix.combined );

    if (gSP.matrix.billboard)
    {
#if __NEON_OPT
        asm volatile (
        "vld1.32 		{d2, d3}, [%0]			\n\t"	//q1={x0,y0, z0, w0}
        "vld1.32 		{d4, d5}, [%1]			\n\t"	//q2={x1,y1, z1, w1}
        "vadd.f32 		q1, q1, q2 			    \n\t"	//q1=q1+q1
        "vst1.32 		{d2, d3}, [%0] 		    \n\t"	//
        :: "r"(&gSP.vertices[v].x), "r"(&gSP.vertices[0].x)
        : "d2", "d3", "d4", "d5", "memory"
        );
#else
        gSP.vertices[v].x += gSP.vertices[0].x;
        gSP.vertices[v].y += gSP.vertices[0].y;
        gSP.vertices[v].z += gSP.vertices[0].z;
        gSP.vertices[v].w += gSP.vertices[0].w;
#endif
    }

    if (!(gSP.geometryMode & G_ZBUFFER))
    {
        gSP.vertices[v].z = -gSP.vertices[v].w;
    }

    if (gSP.geometryMode & G_LIGHTING)
    {
        if (OGL.enableLighting)
        {
            gSPLightVertex(v);
        }
        else
        {
            gSP.vertices[v].r = gSP.lights[gSP.numLights].r;
            gSP.vertices[v].g = gSP.lights[gSP.numLights].g;
            gSP.vertices[v].b = gSP.lights[gSP.numLights].b;
        }

        if (gSP.geometryMode & G_TEXTURE_GEN)
        {
            TransformVectorNormalize(&gSP.vertices[v].nx, gSP.matrix.projection);

            if (gSP.geometryMode & G_TEXTURE_GEN_LINEAR)
            {
                gSP.vertices[v].s = acosf(gSP.vertices[v].nx) * 325.94931f;
                gSP.vertices[v].t = acosf(gSP.vertices[v].ny) * 325.94931f;
            }
            else // G_TEXTURE_GEN
            {
                gSP.vertices[v].s = (gSP.vertices[v].nx + 1.0f) * 512.0f;
                gSP.vertices[v].t = (gSP.vertices[v].ny + 1.0f) * 512.0f;
            }
        }
    }

    if (OGL.enableClipping)
        gSPClipVertex(v);
}

void gSPNoOp()
{
#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_IGNORED, "gSPNoOp();\n" );
#endif
}

void gSPMatrix( u32 matrix, u8 param )
{
    f32 mtx[4][4];
    u32 address = RSP_SegmentToPhysical( matrix );

    if (address + 64 > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load matrix from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPMatrix( 0x%08X, %s | %s | %s );\n",
            matrix,
            (param & G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_MODELVIEW",
            (param & G_MTX_LOAD) ? "G_MTX_LOAD" : "G_MTX_MUL",
            (param & G_MTX_PUSH) ? "G_MTX_PUSH" : "G_MTX_NOPUSH" );
#endif
        return;
    }

    RSP_LoadMatrix( mtx, address );

    if (param & G_MTX_PROJECTION)
    {
        if (param & G_MTX_LOAD)
            CopyMatrix( gSP.matrix.projection, mtx );
        else
            MultMatrix( gSP.matrix.projection, mtx );
    }
    else
    {
        if ((param & G_MTX_PUSH) && (gSP.matrix.modelViewi < (gSP.matrix.stackSize - 1)))
        {
            CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi + 1], gSP.matrix.modelView[gSP.matrix.modelViewi] );
            gSP.matrix.modelViewi++;
        }
#ifdef DEBUG
        else
            DebugMsg( DEBUG_ERROR | DEBUG_MATRIX, "// Modelview stack overflow\n" );
#endif

        if (param & G_MTX_LOAD)
            CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
        else
            MultMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
    }

    gSP.changed |= CHANGED_MATRIX;

#ifdef DEBUG
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3] );
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3] );
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[2][0], mtx[2][1], mtx[2][2], mtx[2][3] );
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[3][0], mtx[3][1], mtx[3][2], mtx[3][3] );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPMatrix( 0x%08X, %s | %s | %s );\n",
        matrix,
        (param & G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_MODELVIEW",
        (param & G_MTX_LOAD) ? "G_MTX_LOAD" : "G_MTX_MUL",
        (param & G_MTX_PUSH) ? "G_MTX_PUSH" : "G_MTX_NOPUSH" );
#endif
}

void gSPDMAMatrix( u32 matrix, u8 index, u8 multiply )
{
    f32 mtx[4][4];
    u32 address = gSP.DMAOffsets.mtx + RSP_SegmentToPhysical( matrix );

    if (address + 64 > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load matrix from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPDMAMatrix( 0x%08X, %i, %s );\n",
            matrix, index, multiply ? "TRUE" : "FALSE" );
#endif
        return;
    }

    RSP_LoadMatrix( mtx, address );

    gSP.matrix.modelViewi = index;

    if (multiply)
    {
        //CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], gSP.matrix.modelView[0] );
        //MultMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );
        MultMatrix(gSP.matrix.modelView[0], mtx, gSP.matrix.modelView[gSP.matrix.modelViewi]);
    }
    else
        CopyMatrix( gSP.matrix.modelView[gSP.matrix.modelViewi], mtx );

    CopyMatrix( gSP.matrix.projection, identityMatrix );


    gSP.changed |= CHANGED_MATRIX;
#ifdef DEBUG
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[0][0], mtx[0][1], mtx[0][2], mtx[0][3] );
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[1][0], mtx[1][1], mtx[1][2], mtx[1][3] );
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[2][0], mtx[2][1], mtx[2][2], mtx[2][3] );
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_MATRIX, "// %12.6f %12.6f %12.6f %12.6f\n",
        mtx[3][0], mtx[3][1], mtx[3][2], mtx[3][3] );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPDMAMatrix( 0x%08X, %i, %s );\n",
        matrix, index, multiply ? "TRUE" : "FALSE" );
#endif
}

void gSPViewport( u32 v )
{
    u32 address = RSP_SegmentToPhysical( v );

    if ((address + 16) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load viewport from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPViewport( 0x%08X );\n", v );
#endif
        return;
    }

    gSP.viewport.vscale[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  2], 2 );
    gSP.viewport.vscale[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address     ], 2 );
    gSP.viewport.vscale[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  6], 10 );// * 0.00097847357f;
    gSP.viewport.vscale[3] = *(s16*)&RDRAM[address +  4];
    gSP.viewport.vtrans[0] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 10], 2 );
    gSP.viewport.vtrans[1] = _FIXED2FLOAT( *(s16*)&RDRAM[address +  8], 2 );
    gSP.viewport.vtrans[2] = _FIXED2FLOAT( *(s16*)&RDRAM[address + 14], 10 );// * 0.00097847357f;
    gSP.viewport.vtrans[3] = *(s16*)&RDRAM[address + 12];

    gSP.viewport.x      = gSP.viewport.vtrans[0] - gSP.viewport.vscale[0];
    gSP.viewport.y      = gSP.viewport.vtrans[1] - gSP.viewport.vscale[1];
    gSP.viewport.width  = gSP.viewport.vscale[0] * 2;
    gSP.viewport.height = gSP.viewport.vscale[1] * 2;
    gSP.viewport.nearz  = gSP.viewport.vtrans[2] - gSP.viewport.vscale[2];
    gSP.viewport.farz   = (gSP.viewport.vtrans[2] + gSP.viewport.vscale[2]) ;

    gSP.changed |= CHANGED_VIEWPORT;

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPViewport( 0x%08X );\n", v );
#endif
}

void gSPForceMatrix( u32 mptr )
{
    u32 address = RSP_SegmentToPhysical( mptr );

    if (address + 64 > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to load from invalid address" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPForceMatrix( 0x%08X );\n", mptr );
#endif
        return;
    }

    RSP_LoadMatrix( gSP.matrix.combined, RSP_SegmentToPhysical( mptr ) );

    gSP.changed &= ~CHANGED_MATRIX;

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPForceMatrix( 0x%08X );\n", mptr );
#endif
}

void gSPLight( u32 l, s32 n )
{
    n--;
    u32 address = RSP_SegmentToPhysical( l );

    if ((address + sizeof( Light )) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load light from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
            l, n );
#endif
        return;
    }

    Light *light = (Light*)&RDRAM[address];

    if (n < 8)
    {
#ifdef __PACKVERTEX_OPT
        gSP.lights[n].r = light->r;
        gSP.lights[n].g = light->g;
        gSP.lights[n].b = light->b;
#else
        gSP.lights[n].r = light->r * 0.0039215689f;
        gSP.lights[n].g = light->g * 0.0039215689f;
        gSP.lights[n].b = light->b * 0.0039215689f;
#endif
        gSP.lights[n].x = light->x;
        gSP.lights[n].y = light->y;
        gSP.lights[n].z = light->z;

        Normalize( &gSP.lights[n].x );
    }

#ifdef DEBUG
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// x = %2.6f    y = %2.6f    z = %2.6f\n",
        _FIXED2FLOAT( light->x, 7 ), _FIXED2FLOAT( light->y, 7 ), _FIXED2FLOAT( light->z, 7 ) );
    DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// r = %3i    g = %3i   b = %3i\n",
        light->r, light->g, light->b );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLight( 0x%08X, LIGHT_%i );\n",
        l, n );
#endif
}

void gSPLookAt( u32 l )
{
}

void gSPVertex( u32 v, u32 n, u32 v0 )
{
    u32 address = RSP_SegmentToPhysical( v );

    if ((address + sizeof( Vertex ) * n) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPVertex( 0x%08X, %i, %i );\n",
            v, n, v0 );
#endif
        return;
    }

    Vertex *vertex = (Vertex*)&RDRAM[address];

    if ((n + v0) < (80))
    {
        unsigned int i = v0;
#ifdef __VEC4_OPT
        for (; i < n - (n%4) + v0; i += 4)
        {
            for(unsigned int j = 0; j < 4; j++)
            {
                gSP.vertices[i+j].x = vertex->x;
                gSP.vertices[i+j].y = vertex->y;
                gSP.vertices[i+j].z = vertex->z;
                gSP.vertices[i+j].flag = vertex->flag;
                gSP.vertices[i+j].s = _FIXED2FLOAT( vertex->s, 5 );
                gSP.vertices[i+j].t = _FIXED2FLOAT( vertex->t, 5 );
                if (gSP.geometryMode & G_LIGHTING)
                {
                    gSP.vertices[i+j].nx = vertex->normal.x;
                    gSP.vertices[i+j].ny = vertex->normal.y;
                    gSP.vertices[i+j].nz = vertex->normal.z;
#ifdef __PACKVERTEX_OPT
                    gSP.vertices[i+j].a = vertex->color.a;
#else
                    gSP.vertices[i+j].a = vertex->color.a * 0.0039215689f;
#endif
                }
                else
                {
#ifdef __PACKVERTEX_OPT
                    gSP.vertices[i+j].r = vertex->color.r;
                    gSP.vertices[i+j].g = vertex->color.g;
                    gSP.vertices[i+j].b = vertex->color.b;
                    gSP.vertices[i+j].a = vertex->color.a;
#else
                    gSP.vertices[i+j].r = vertex->color.r * 0.0039215689f;
                    gSP.vertices[i+j].g = vertex->color.g * 0.0039215689f;
                    gSP.vertices[i+j].b = vertex->color.b * 0.0039215689f;
                    gSP.vertices[i+j].a = vertex->color.a * 0.0039215689f;
#endif
                }
                vertex++;
            }
            gSPProcessVertex4(i);
        }
#endif
        for (; i < n + v0; i++)
        {
            gSP.vertices[i].x = vertex->x;
            gSP.vertices[i].y = vertex->y;
            gSP.vertices[i].z = vertex->z;
            gSP.vertices[i].flag = vertex->flag;
            gSP.vertices[i].s = _FIXED2FLOAT( vertex->s, 5 );
            gSP.vertices[i].t = _FIXED2FLOAT( vertex->t, 5 );
            if (gSP.geometryMode & G_LIGHTING)
            {
                gSP.vertices[i].nx = vertex->normal.x;
                gSP.vertices[i].ny = vertex->normal.y;
                gSP.vertices[i].nz = vertex->normal.z;
#ifdef __PACKVERTEX_OPT
                gSP.vertices[i].a = vertex->color.a;
#else
                gSP.vertices[i].a = vertex->color.a * 0.0039215689f;
#endif
            }
            else
            {
#ifdef __PACKVERTEX_OPT
                gSP.vertices[i].r = vertex->color.r;
                gSP.vertices[i].g = vertex->color.g;
                gSP.vertices[i].b = vertex->color.b;
                gSP.vertices[i].a = vertex->color.a;
#else
                gSP.vertices[i].r = vertex->color.r * 0.0039215689f;
                gSP.vertices[i].g = vertex->color.g * 0.0039215689f;
                gSP.vertices[i].b = vertex->color.b * 0.0039215689f;
                gSP.vertices[i].a = vertex->color.a * 0.0039215689f;
#endif
            }

#ifdef DEBUG
            DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// x = %6i    y = %6i    z = %6i \n",
                vertex->x, vertex->y, vertex->z );
            DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// s = %5.5f    t = %5.5f    flag = %i \n",
                vertex->s, vertex->t, vertex->flag );

            if (gSP.geometryMode & G_LIGHTING )
            {
                DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// nx = %2.6f    ny = %2.6f    nz = %2.6f\n",
                    _FIXED2FLOAT( vertex->normal.x, 7 ), _FIXED2FLOAT( vertex->normal.y, 7 ), _FIXED2FLOAT( vertex->normal.z, 7 ) );
            }
            else
            {
                DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED | DEBUG_VERTEX, "// r = %3u    g = %3u    b = %3u    a = %3u\n",
                    vertex->color.r, vertex->color.g, vertex->color.b, vertex->color.a );
            }
#endif

            gSPProcessVertex( i );
            vertex++;
        }
    }
#ifdef DEBUG
    else
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPVertex( 0x%08X, %i, %i );\n",
        v, n, v0 );
#endif

}

void gSPCIVertex( u32 v, u32 n, u32 v0 )
{
    u32 address = RSP_SegmentToPhysical( v );

    if ((address + sizeof( PDVertex ) * n) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPCIVertex( 0x%08X, %i, %i );\n",
            v, n, v0 );
#endif
        return;
    }

    PDVertex *vertex = (PDVertex*)&RDRAM[address];

    if ((n + v0) < (80))
    {
        unsigned int i = v0;
#ifdef __VEC4_OPT
        for (; i < n - (n%4) + v0; i += 4)
        {
            for(unsigned int j = 0; j < 4; j++)
            {
                gSP.vertices[i+j].x = vertex->x;
                gSP.vertices[i+j].y = vertex->y;
                gSP.vertices[i+j].z = vertex->z;
                gSP.vertices[i+j].flag = 0;
                gSP.vertices[i+j].s = _FIXED2FLOAT( vertex->s, 5 );
                gSP.vertices[i+j].t = _FIXED2FLOAT( vertex->t, 5 );
                u8 *color = &RDRAM[gSP.vertexColorBase + (vertex->ci & 0xff)];

                if (gSP.geometryMode & G_LIGHTING)
                {
                    gSP.vertices[i+j].nx = (s8)color[3];
                    gSP.vertices[i+j].ny = (s8)color[2];
                    gSP.vertices[i+j].nz = (s8)color[1];
#ifdef __PACKVERTEX_OPT
                    gSP.vertices[i+j].a = color[0];
#else
                    gSP.vertices[i+j].a = color[0] * 0.0039215689f;
#endif
                }
                else
                {
#ifdef __PACKVERTEX_OPT
                    gSP.vertices[i+j].r = color[3];
                    gSP.vertices[i+j].g = color[2];
                    gSP.vertices[i+j].b = color[1];
                    gSP.vertices[i+j].a = color[0];
#else
                    gSP.vertices[i+j].r = color[3] * 0.0039215689f;
                    gSP.vertices[i+j].g = color[2] * 0.0039215689f;
                    gSP.vertices[i+j].b = color[1] * 0.0039215689f;
                    gSP.vertices[i+j].a = color[0] * 0.0039215689f;
#endif
                }
                vertex++;
            }
            gSPProcessVertex4(i);
        }
#endif
        for(; i < n + v0; i++)
        {
            gSP.vertices[i].x = vertex->x;
            gSP.vertices[i].y = vertex->y;
            gSP.vertices[i].z = vertex->z;
            gSP.vertices[i].flag = 0;
            gSP.vertices[i].s = _FIXED2FLOAT( vertex->s, 5 );
            gSP.vertices[i].t = _FIXED2FLOAT( vertex->t, 5 );
            u8 *color = &RDRAM[gSP.vertexColorBase + (vertex->ci & 0xff)];

            if (gSP.geometryMode & G_LIGHTING)
            {
                gSP.vertices[i].nx = (s8)color[3];
                gSP.vertices[i].ny = (s8)color[2];
                gSP.vertices[i].nz = (s8)color[1];
#ifdef __PACKVERTEX_OPT
                gSP.vertices[i].a = color[0];
#else
                gSP.vertices[i].a = color[0] * 0.0039215689f;
#endif
            }
            else
            {
#ifdef __PACKVERTEX_OPT
                gSP.vertices[i].r = color[3];
                gSP.vertices[i].g = color[2];
                gSP.vertices[i].b = color[1];
                gSP.vertices[i].a = color[0];
#else
                gSP.vertices[i].r = color[3] * 0.0039215689f;
                gSP.vertices[i].g = color[2] * 0.0039215689f;
                gSP.vertices[i].b = color[1] * 0.0039215689f;
                gSP.vertices[i].a = color[0] * 0.0039215689f;
#endif
            }

            gSPProcessVertex( i );

            vertex++;
        }
    }
#ifdef DEBUG
    else
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPCIVertex( 0x%08X, %i, %i );\n",
        v, n, v0 );
#endif

}

void gSPDMAVertex( u32 v, u32 n, u32 v0 )
{
    u32 address = gSP.DMAOffsets.vtx + RSP_SegmentToPhysical( v );

    if ((address + 10 * n) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPDMAVertex( 0x%08X, %i, %i );\n",
            v, n, v0 );
#endif
        return;
    }

    if ((n + v0) < (80))
    {
        unsigned int i = v0;
#ifdef __VEC4_OPT
        for (; i < n - (n%4) + v0; i += 4){
            for(unsigned int j = 0; j < 4; j++){
                gSP.vertices[i+j].x = *(s16*)&RDRAM[address ^ 2];
                gSP.vertices[i+j].y = *(s16*)&RDRAM[(address + 2) ^ 2];
                gSP.vertices[i+j].z = *(s16*)&RDRAM[(address + 4) ^ 2];

                if (gSP.geometryMode & G_LIGHTING)
                {
                    gSP.vertices[i+j].nx = *(s8*)&RDRAM[(address + 6) ^ 3];
                    gSP.vertices[i+j].ny = *(s8*)&RDRAM[(address + 7) ^ 3];
                    gSP.vertices[i+j].nz = *(s8*)&RDRAM[(address + 8) ^ 3];
#ifdef __PACKVERTEX_OPT
                    gSP.vertices[i+j].a = *(u8*)&RDRAM[(address + 9) ^ 3];
#else
                    gSP.vertices[i+j].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
#endif
                }
                else
                {
#ifdef __PACKVERTEX_OPT
                    gSP.vertices[i+j].r = *(u8*)&RDRAM[(address + 6) ^ 3];
                    gSP.vertices[i+j].g = *(u8*)&RDRAM[(address + 7) ^ 3];
                    gSP.vertices[i+j].b = *(u8*)&RDRAM[(address + 8) ^ 3];
                    gSP.vertices[i+j].a = *(u8*)&RDRAM[(address + 9) ^ 3];
#else
                    gSP.vertices[i+j].r = *(u8*)&RDRAM[(address + 6) ^ 3] * 0.0039215689f;
                    gSP.vertices[i+j].g = *(u8*)&RDRAM[(address + 7) ^ 3] * 0.0039215689f;
                    gSP.vertices[i+j].b = *(u8*)&RDRAM[(address + 8) ^ 3] * 0.0039215689f;
                    gSP.vertices[i+j].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
#endif
                }
                address += 10;
            }
            gSPProcessVertex4(i);
        }
#endif
        for (; i < n + v0; i++)
        {
            gSP.vertices[i].x = *(s16*)&RDRAM[address ^ 2];
            gSP.vertices[i].y = *(s16*)&RDRAM[(address + 2) ^ 2];
            gSP.vertices[i].z = *(s16*)&RDRAM[(address + 4) ^ 2];

            if (gSP.geometryMode & G_LIGHTING)
            {
                gSP.vertices[i].nx = *(s8*)&RDRAM[(address + 6) ^ 3];
                gSP.vertices[i].ny = *(s8*)&RDRAM[(address + 7) ^ 3];
                gSP.vertices[i].nz = *(s8*)&RDRAM[(address + 8) ^ 3];
#ifdef __PACKVERTEX_OPT
                gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3];
#else
                gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
#endif
            }
            else
            {
#ifdef __PACKVERTEX_OPT
                gSP.vertices[i].r = *(u8*)&RDRAM[(address + 6) ^ 3];
                gSP.vertices[i].g = *(u8*)&RDRAM[(address + 7) ^ 3];
                gSP.vertices[i].b = *(u8*)&RDRAM[(address + 8) ^ 3];
                gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3];
#else
                gSP.vertices[i].r = *(u8*)&RDRAM[(address + 6) ^ 3] * 0.0039215689f;
                gSP.vertices[i].g = *(u8*)&RDRAM[(address + 7) ^ 3] * 0.0039215689f;
                gSP.vertices[i].b = *(u8*)&RDRAM[(address + 8) ^ 3] * 0.0039215689f;
                gSP.vertices[i].a = *(u8*)&RDRAM[(address + 9) ^ 3] * 0.0039215689f;
#endif
            }

            gSPProcessVertex( i );

            address += 10;
        }
    }
#ifdef DEBUG
    else
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_VERTEX, "// Attempting to load vertices past vertex buffer size\n" );
#endif

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_VERTEX, "gSPDMAVertex( 0x%08X, %i, %i );\n",
        v, n, v0 );
#endif
}

void gSPDisplayList( u32 dl )
{
    u32 address = RSP_SegmentToPhysical( dl );

    if ((address + 8) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
            dl );
#endif
        return;
    }

    if (RSP.PCi < (GBI.PCStackSize - 1))
    {
#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "\n" );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
        dl );
#endif
        RSP.PCi++;
        RSP.PC[RSP.PCi] = address;
    }
#ifdef DEBUG
    else
    {
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// PC stack overflow\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDisplayList( 0x%08X );\n",
            dl );
    }
#endif
}

void gSPDMADisplayList( u32 dl, u32 n )
{
    if ((dl + (n << 3)) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load display list from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDMADisplayList( 0x%08X, %i );\n",
            dl, n );
#endif
        return;
    }

    u32 curDL = RSP.PC[RSP.PCi];

    RSP.PC[RSP.PCi] = RSP_SegmentToPhysical( dl );

    while ((RSP.PC[RSP.PCi] - dl) < (n << 3))
    {
        if ((RSP.PC[RSP.PCi] + 8) > RDRAMSize)
        {
#ifdef DEBUG
            DebugMsg( DEBUG_LOW | DEBUG_ERROR, "ATTEMPTING TO EXECUTE RSP COMMAND AT INVALID RDRAM LOCATION\n" );
#endif
            break;
        }

        u32 w0 = *(u32*)&RDRAM[RSP.PC[RSP.PCi]];
        u32 w1 = *(u32*)&RDRAM[RSP.PC[RSP.PCi] + 4];

#ifdef DEBUG
        DebugRSPState( RSP.PCi, RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
        DebugMsg( DEBUG_LOW | DEBUG_HANDLED, "0x%08lX: CMD=0x%02lX W0=0x%08lX W1=0x%08lX\n", RSP.PC[RSP.PCi], _SHIFTR( w0, 24, 8 ), w0, w1 );
#endif

        RSP.PC[RSP.PCi] += 8;
        RSP.nextCmd = _SHIFTR( *(u32*)&RDRAM[RSP.PC[RSP.PCi]], 24, 8 );

        GBI.cmd[_SHIFTR( w0, 24, 8 )]( w0, w1 );
    }

    RSP.PC[RSP.PCi] = curDL;

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPDMADisplayList( 0x%08X, %i );\n",
        dl, n );
#endif
}

void gSPBranchList( u32 dl )
{
    u32 address = RSP_SegmentToPhysical( dl );

    if ((address + 8) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to branch to display list at invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchList( 0x%08X );\n",
            dl );
#endif
        return;
    }

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchList( 0x%08X );\n",
        dl );
#endif

    RSP.PC[RSP.PCi] = address;
}

void gSPBranchLessZ( u32 branchdl, u32 vtx, f32 zval )
{
    u32 address = RSP_SegmentToPhysical( branchdl );

    if ((address + 8) > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Specified display list at invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
            branchdl, vtx, zval );
#endif
        return;
    }

    if (gSP.vertices[vtx].z <= zval)
        RSP.PC[RSP.PCi] = address;

#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPBranchLessZ( 0x%08X, %i, %i );\n",
            branchdl, vtx, zval );
#endif
}

void gSPSetDMAOffsets( u32 mtxoffset, u32 vtxoffset )
{
    gSP.DMAOffsets.mtx = mtxoffset;
    gSP.DMAOffsets.vtx = vtxoffset;

#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetDMAOffsets( 0x%08X, 0x%08X );\n",
            mtxoffset, vtxoffset );
#endif
}

void gSPSetVertexColorBase( u32 base )
{
    gSP.vertexColorBase = RSP_SegmentToPhysical( base );

#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetVertexColorBase( 0x%08X );\n",
            base );
#endif
}

void gSPSprite2DBase( u32 base )
{
}

void gSPCopyVertex( SPVertex *dest, SPVertex *src )
{
    dest->x = src->x;
    dest->y = src->y;
    dest->z = src->z;
    dest->w = src->w;
    dest->r = src->r;
    dest->g = src->g;
    dest->b = src->b;
    dest->a = src->a;
    dest->s = src->s;
    dest->t = src->t;
}

void gSPInterpolateVertex( SPVertex *dest, f32 percent, SPVertex *first, SPVertex *second )
{
    dest->x = first->x + percent * (second->x - first->x);
    dest->y = first->y + percent * (second->y - first->y);
    dest->z = first->z + percent * (second->z - first->z);
    dest->w = first->w + percent * (second->w - first->w);
    dest->r = first->r + percent * (second->r - first->r);
    dest->g = first->g + percent * (second->g - first->g);
    dest->b = first->b + percent * (second->b - first->b);
    dest->a = first->a + percent * (second->a - first->a);
    dest->s = first->s + percent * (second->s - first->s);
    dest->t = first->t + percent * (second->t - first->t);
}

void gSPTriangle( s32 v0, s32 v1, s32 v2, s32 flag )
{
    if ((v0 < 80) && (v1 < 80) && (v2 < 80))
    {
        // Don't bother with triangles completely outside clipping frustrum
        if ((OGL.enableClipping) && ((gSP.vertices[v0].xClip != 0) &&
            (gSP.vertices[v0].xClip == gSP.vertices[v1].xClip) &&
            (gSP.vertices[v1].xClip == gSP.vertices[v2].xClip)) ||
           ((gSP.vertices[v0].yClip != 0) &&
            (gSP.vertices[v0].yClip == gSP.vertices[v1].yClip) &&
            (gSP.vertices[v1].yClip == gSP.vertices[v2].yClip)) ||
           ((gSP.vertices[v0].zClip != 0) &&
            (gSP.vertices[v0].zClip == gSP.vertices[v1].zClip) &&
            (gSP.vertices[v1].zClip == gSP.vertices[v2].zClip)))
        {
            return;
        }

        // NoN work-around, clips triangles, and draws the clipped-off parts with clamped z
        if (GBI.current->NoN &&
            ((gSP.vertices[v0].zClip < 0) ||
            (gSP.vertices[v1].zClip < 0) ||
            (gSP.vertices[v2].zClip < 0)))
        {
            SPVertex nearVertices[4];
            SPVertex clippedVertices[4];
            //s32 numNearTris = 0;
            //s32 numClippedTris = 0;
            s32 nearIndex = 0;
            s32 clippedIndex = 0;

            s32 v[3] = { v0, v1, v2 };

            for (s32 i = 0; i < 3; i++)
            {
                s32 j = i + 1;
                if (j == 3) j = 0;

                if (((gSP.vertices[v[i]].zClip < 0) && (gSP.vertices[v[j]].zClip >= 0)) ||
                    ((gSP.vertices[v[i]].zClip >= 0) && (gSP.vertices[v[j]].zClip < 0)))
                {
                    f32 percent = (-gSP.vertices[v[i]].w - gSP.vertices[v[i]].z) / ((gSP.vertices[v[j]].z - gSP.vertices[v[i]].z) + (gSP.vertices[v[j]].w - gSP.vertices[v[i]].w));

                    gSPInterpolateVertex( &clippedVertices[clippedIndex], percent, &gSP.vertices[v[i]], &gSP.vertices[v[j]] );

                    gSPCopyVertex( &nearVertices[nearIndex], &clippedVertices[clippedIndex] );
                    nearVertices[nearIndex].z = -nearVertices[nearIndex].w;

                    clippedIndex++;
                    nearIndex++;
                }

                if (((gSP.vertices[v[i]].zClip < 0) && (gSP.vertices[v[j]].zClip >= 0)) ||
                    ((gSP.vertices[v[i]].zClip >= 0) && (gSP.vertices[v[j]].zClip >= 0)))
                {
                    gSPCopyVertex( &clippedVertices[clippedIndex], &gSP.vertices[v[j]] );
                    clippedIndex++;
                }
                else
                {
                    gSPCopyVertex( &nearVertices[nearIndex], &gSP.vertices[v[j]] );
                    nearVertices[nearIndex].z = -nearVertices[nearIndex].w;// + 0.00001f;
                    nearIndex++;
                }
            }

            OGL_DrawTriangle( clippedVertices, 0, 1, 2 );

            if (clippedIndex == 4) OGL_DrawTriangle( clippedVertices, 0, 2, 3 );

            //glDisable( GL_POLYGON_OFFSET_FILL );

            //glDepthFunc( GL_GEQUAL );

            OGL_DrawTriangle( nearVertices, 0, 1, 2 );
            if (nearIndex == 4) OGL_DrawTriangle( nearVertices, 0, 2, 3 );

            if (gDP.otherMode.depthMode == ZMODE_DEC)
                glEnable( GL_POLYGON_OFFSET_FILL );

            //if (gDP.otherMode.depthCompare)
            //    glDepthFunc( GL_GEQUAL );
        }
        else
            OGL_AddTriangle(v0, v1, v2);
    }
#ifdef DEBUG
    else
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TRIANGLE, "// Vertex index out of range\n" );
#endif

    if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
    gDP.colorImage.changed = TRUE;
    gDP.colorImage.height = (unsigned int)(max( gDP.colorImage.height, gDP.scissor.lry ));
}

void gSP1Triangle( s32 v0, s32 v1, s32 v2, s32 flag )
{
    gSPTriangle( v0, v1, v2, flag );
    gSPFlushTriangles();

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP1Triangle( %i, %i, %i, %i );\n",
        v0, v1, v2, flag );
#endif
}

void gSP2Triangles( s32 v00, s32 v01, s32 v02, s32 flag0,
                    s32 v10, s32 v11, s32 v12, s32 flag1 )
{
    gSPTriangle( v00, v01, v02, flag0 );
    gSPTriangle( v10, v11, v12, flag1 );
    gSPFlushTriangles();

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP2Triangles( %i, %i, %i, %i,\n",
        v00, v01, v02, flag0 );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i, %i );\n",
        v10, v11, v12, flag1 );
#endif
}

void gSP4Triangles( s32 v00, s32 v01, s32 v02,
                    s32 v10, s32 v11, s32 v12,
                    s32 v20, s32 v21, s32 v22,
                    s32 v30, s32 v31, s32 v32 )
{
    gSPTriangle( v00, v01, v02, 0 );
    gSPTriangle( v10, v11, v12, 0 );
    gSPTriangle( v20, v21, v22, 0 );
    gSPTriangle( v30, v31, v32, 0 );
    gSPFlushTriangles();

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP4Triangles( %i, %i, %i,\n",
        v00, v01, v02 );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i,\n",
        v10, v11, v12 );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i,\n",
        v20, v21, v22 );
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "               %i, %i, %i );\n",
        v30, v31, v32 );
#endif
}

void gSPDMATriangles( u32 tris, u32 n )
{
    u32 address = RSP_SegmentToPhysical( tris );

    if (address + sizeof( DKRTriangle ) * n > RDRAMSize)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_TRIANGLE, "// Attempting to load triangles from invalid address\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSPDMATriangles( 0x%08X, %i );\n" );
#endif
        return;
    }

    DKRTriangle *triangles = (DKRTriangle*)&RDRAM[address];

    for (u32 i = 0; i < n; i++)
    {
        gSP.geometryMode &= ~G_CULL_BOTH;

        if (!(triangles->flag & 0x40))
        {
            if (gSP.viewport.vscale[0] > 0)
                gSP.geometryMode |= G_CULL_BACK;
            else
                gSP.geometryMode |= G_CULL_FRONT;
        }
        gSP.changed |= CHANGED_GEOMETRYMODE;
        gSP.vertices[triangles->v0].s = _FIXED2FLOAT( triangles->s0, 5 );
        gSP.vertices[triangles->v0].t = _FIXED2FLOAT( triangles->t0, 5 );
        gSP.vertices[triangles->v1].s = _FIXED2FLOAT( triangles->s1, 5 );
        gSP.vertices[triangles->v1].t = _FIXED2FLOAT( triangles->t1, 5 );
        gSP.vertices[triangles->v2].s = _FIXED2FLOAT( triangles->s2, 5 );
        gSP.vertices[triangles->v2].t = _FIXED2FLOAT( triangles->t2, 5 );
        gSPTriangle( triangles->v0, triangles->v1, triangles->v2, 0 );

        triangles++;
    }

    gSPFlushTriangles();

#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSPDMATriangles( 0x%08X, %i );\n",
            tris, n );
#endif
}

void gSP1Quadrangle( s32 v0, s32 v1, s32 v2, s32 v3 )
{
    gSPTriangle( v0, v1, v2, 0 );
    gSPTriangle( v0, v2, v3, 0 );
    gSPFlushTriangles();

#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TRIANGLE, "gSP1Quadrangle( %i, %i, %i, %i );\n",
            v0, v1, v2, v3 );
#endif
}

bool gSPCullVertices( u32 v0, u32 vn )
{
    unsigned int xClip, yClip, zClip;
    xClip = yClip = zClip = 0;
    for (unsigned int i = v0; i <= vn; i++)
    {
        if (gSP.vertices[i].xClip == 0)
            return FALSE;
        else if (gSP.vertices[i].xClip < 0)
        {
            if (xClip > 0)
                return FALSE;
            else
                xClip = gSP.vertices[i].xClip;
        }
        else if (gSP.vertices[i].xClip > 0)
        {
            if (xClip < 0)
                return FALSE;
            else
                xClip = gSP.vertices[i].xClip;
        }

        if (gSP.vertices[i].yClip == 0)
            return FALSE;
        else if (gSP.vertices[i].yClip < 0)
        {
            if (yClip > 0)
                return FALSE;
            else
                yClip = gSP.vertices[i].yClip;
        }
        else if (gSP.vertices[i].yClip > 0)
        {
            if (yClip < 0)
                return FALSE;
            else
                yClip = gSP.vertices[i].yClip;
        }

        if (gSP.vertices[i].zClip == 0)
            return FALSE;
        else if (gSP.vertices[i].zClip < 0)
        {
            if (zClip > 0)
                return FALSE;
            else
                zClip = gSP.vertices[i].zClip;
        }
        else if (gSP.vertices[i].zClip > 0)
        {
            if (zClip < 0)
                return FALSE;
            else
                zClip = gSP.vertices[i].zClip;
        }
    }
    return TRUE;
}

void gSPCullDisplayList( u32 v0, u32 vn )
{
    if (gSPCullVertices( v0, vn ))
    {
        if (RSP.PCi > 0)
            RSP.PCi--;
        else
        {
#ifdef DEBUG
            DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// End of display list, halting execution\n" );
#endif
            RSP.halt = TRUE;
        }
#ifdef DEBUG
        DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// Culling display list\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPCullDisplayList( %i, %i );\n\n",
            v0, vn );
#endif
    }
#ifdef DEBUG
    else
    {
        DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// Not culling display list\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPCullDisplayList( %i, %i );\n",
            v0, vn );
    }
#endif
}

void gSPPopMatrixN( u32 param, u32 num )
{
    if (gSP.matrix.modelViewi > num - 1)
    {
        gSP.matrix.modelViewi -= num;

        gSP.changed |= CHANGED_MATRIX;
    }
#ifdef DEBUG
    else
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );

    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrixN( %s, %i );\n",
        (param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" :
        (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID",
        num );
#endif
}

void gSPPopMatrix( u32 param )
{
    if (gSP.matrix.modelViewi > 0)
    {
        gSP.matrix.modelViewi--;

        gSP.changed |= CHANGED_MATRIX;
    }
#ifdef DEBUG
    else
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Attempting to pop matrix stack below 0\n" );

    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPPopMatrix( %s );\n",
        (param == G_MTX_MODELVIEW) ? "G_MTX_MODELVIEW" :
        (param == G_MTX_PROJECTION) ? "G_MTX_PROJECTION" : "G_MTX_INVALID" );
#endif
}

void gSPSegment( s32 seg, s32 base )
{
    if (seg > 0xF)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load address into invalid segment\n",
            SegmentText[seg], base );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
            SegmentText[seg], base );
#endif
        return;
    }

    if ((unsigned int)base > RDRAMSize - 1)
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Attempting to load invalid address into segment array\n",
            SegmentText[seg], base );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
            SegmentText[seg], base );
#endif
        return;
    }

    gSP.segment[seg] = base;

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSegment( %s, 0x%08X );\n",
        SegmentText[seg], base );
#endif
}

void gSPClipRatio( u32 r )
{
}

void gSPInsertMatrix( u32 where, u32 num )
{
    f32 fraction, integer;

    if (gSP.changed & CHANGED_MATRIX)
        gSPCombineMatrices();

    if ((where & 0x3) || (where > 0x3C))
    {
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR | DEBUG_MATRIX, "// Invalid matrix elements\n" );
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPInsertMatrix( 0x%02X, %i );\n",
            where, num );
#endif
        return;
    }

    if (where < 0x20)
    {
        fraction = modff( gSP.matrix.combined[0][where >> 1], &integer );
        gSP.matrix.combined[0][where >> 1] = (s16)_SHIFTR( num, 16, 16 ) + abs( (int)fraction );

        fraction = modff( gSP.matrix.combined[0][(where >> 1) + 1], &integer );
        gSP.matrix.combined[0][(where >> 1) + 1] = (s16)_SHIFTR( num, 0, 16 ) + abs( (int)fraction );
    }
    else
    {
        f32 newValue;

        fraction = modff( gSP.matrix.combined[0][(where - 0x20) >> 1], &integer );
        newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 16, 16 ), 16);

        // Make sure the sign isn't lost
        if ((integer == 0.0f) && (fraction != 0.0f))
            newValue = newValue * (fraction / abs( (int)fraction ));

        gSP.matrix.combined[0][(where - 0x20) >> 1] = newValue;

        fraction = modff( gSP.matrix.combined[0][((where - 0x20) >> 1) + 1], &integer );
        newValue = integer + _FIXED2FLOAT( _SHIFTR( num, 0, 16 ), 16 );

        // Make sure the sign isn't lost
        if ((integer == 0.0f) && (fraction != 0.0f))
            newValue = newValue * (fraction / abs( (int)fraction ));

        gSP.matrix.combined[0][((where - 0x20) >> 1) + 1] = newValue;
    }

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_MATRIX, "gSPInsertMatrix( %s, %i );\n",
        MWOMatrixText[where >> 2], num );
#endif
}

void gSPModifyVertex( u32 vtx, u32 where, u32 val )
{
    switch (where)
    {
        case G_MWO_POINT_RGBA:
#ifdef __PACKVERTEX_OPT
            gSP.vertices[vtx].r = _SHIFTR( val, 24, 8 );
            gSP.vertices[vtx].g = _SHIFTR( val, 16, 8 );
            gSP.vertices[vtx].b = _SHIFTR( val, 8, 8 );
            gSP.vertices[vtx].a = _SHIFTR( val, 0, 8 );
#else
            gSP.vertices[vtx].r = _SHIFTR( val, 24, 8 ) * 0.0039215689f;
            gSP.vertices[vtx].g = _SHIFTR( val, 16, 8 ) * 0.0039215689f;
            gSP.vertices[vtx].b = _SHIFTR( val, 8, 8 ) * 0.0039215689f;
            gSP.vertices[vtx].a = _SHIFTR( val, 0, 8 ) * 0.0039215689f;
#endif
#ifdef DEBUG
            DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
                vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
            break;
        case G_MWO_POINT_ST:
            gSP.vertices[vtx].s = _FIXED2FLOAT( (s16)_SHIFTR( val, 16, 16 ), 5 );
            gSP.vertices[vtx].t = _FIXED2FLOAT( (s16)_SHIFTR( val, 0, 16 ), 5 );
#ifdef DEBUG
            DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
                vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
            break;
        case G_MWO_POINT_XYSCREEN:
#ifdef DEBUG
            DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
                vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
            break;
        case G_MWO_POINT_ZSCREEN:
#ifdef DEBUG
            DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPModifyVertex( %i, %s, 0x%08X );\n",
                vtx, MWOPointText[(where - 0x10) >> 2], val );
#endif
            break;
    }
}

void gSPNumLights( s32 n )
{
    if (n <= 8)
        gSP.numLights = n;
#ifdef DEBUG
    else
        DebugMsg( DEBUG_HIGH | DEBUG_ERROR, "// Setting an invalid number of lights\n" );
#endif

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPNumLights( %i );\n",
        n );
#endif
}

void gSPLightColor( u32 lightNum, u32 packedColor )
{
    lightNum--;

    if (lightNum < 8)
    {
#ifdef __PACKVERTEX_OPT
        gSP.lights[lightNum].r = _SHIFTR( packedColor, 24, 8 );
        gSP.lights[lightNum].g = _SHIFTR( packedColor, 16, 8 );
        gSP.lights[lightNum].b = _SHIFTR( packedColor, 8, 8 );
#else
        gSP.lights[lightNum].r = _SHIFTR( packedColor, 24, 8 ) * 0.0039215689f;
        gSP.lights[lightNum].g = _SHIFTR( packedColor, 16, 8 ) * 0.0039215689f;
        gSP.lights[lightNum].b = _SHIFTR( packedColor, 8, 8 ) * 0.0039215689f;
#endif
    }
#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPLightColor( %i, 0x%08X );\n",
        lightNum, packedColor );
#endif
}

void gSPFogFactor( s16 fm, s16 fo )
{
    gSP.fog.multiplier = fm;
    gSP.fog.offset = fo;

    gSP.changed |= CHANGED_FOGPOSITION;
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPFogFactor( %i, %i );\n", fm, fo );
#endif
}

void gSPPerspNormalize( u16 scale )
{
#ifdef DEBUG
        DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPPerspNormalize( %i );\n", scale );
#endif
}

void gSPTexture( f32 sc, f32 tc, s32 level, s32 tile, s32 on )
{
    gSP.texture.scales = sc;
    gSP.texture.scalet = tc;

    if (gSP.texture.scales == 0.0f) gSP.texture.scales = 1.0f;
    if (gSP.texture.scalet == 0.0f) gSP.texture.scalet = 1.0f;

    gSP.texture.level = level;
    gSP.texture.on = on;

    if (gSP.texture.tile != tile)
    {
        gSP.texture.tile = tile;
        gSP.textureTile[0] = &gDP.tiles[tile];
        gSP.textureTile[1] = &gDP.tiles[(tile < 7) ? (tile + 1) : tile];
        gSP.changed |= CHANGED_TEXTURE;
    }

    gSP.changed |= CHANGED_TEXTURESCALE;

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED | DEBUG_TEXTURE, "gSPTexture( %f, %f, %i, %i, %i );\n",
        sc, tc, level, tile, on );
#endif
}

void gSPEndDisplayList()
{
    if (RSP.PCi > 0)
        RSP.PCi--;
    else
    {
#ifdef DEBUG
        DebugMsg( DEBUG_DETAIL | DEBUG_HANDLED, "// End of display list, halting execution\n" );
#endif
        RSP.halt = TRUE;
    }

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPEndDisplayList();\n\n" );
#endif
}

void gSPGeometryMode( u32 clear, u32 set )
{
    gSP.geometryMode = (gSP.geometryMode & ~clear) | set;

    gSP.changed |= CHANGED_GEOMETRYMODE;

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPGeometryMode( %s%s%s%s%s%s%s%s%s%s, %s%s%s%s%s%s%s%s%s%s );\n",
        clear & G_SHADE ? "G_SHADE | " : "",
        clear & G_LIGHTING ? "G_LIGHTING | " : "",
        clear & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
        clear & G_ZBUFFER ? "G_ZBUFFER | " : "",
        clear & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
        clear & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
        clear & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
        clear & G_CULL_BACK ? "G_CULL_BACK | " : "",
        clear & G_FOG ? "G_FOG | " : "",
        clear & G_CLIPPING ? "G_CLIPPING" : "",
        set & G_SHADE ? "G_SHADE | " : "",
        set & G_LIGHTING ? "G_LIGHTING | " : "",
        set & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
        set & G_ZBUFFER ? "G_ZBUFFER | " : "",
        set & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
        set & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
        set & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
        set & G_CULL_BACK ? "G_CULL_BACK | " : "",
        set & G_FOG ? "G_FOG | " : "",
        set & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPSetGeometryMode( u32 mode )
{
    gSP.geometryMode |= mode;

    gSP.changed |= CHANGED_GEOMETRYMODE;
#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPSetGeometryMode( %s%s%s%s%s%s%s%s%s%s );\n",
        mode & G_SHADE ? "G_SHADE | " : "",
        mode & G_LIGHTING ? "G_LIGHTING | " : "",
        mode & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
        mode & G_ZBUFFER ? "G_ZBUFFER | " : "",
        mode & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
        mode & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
        mode & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
        mode & G_CULL_BACK ? "G_CULL_BACK | " : "",
        mode & G_FOG ? "G_FOG | " : "",
        mode & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPClearGeometryMode( u32 mode )
{
    gSP.geometryMode &= ~mode;

    gSP.changed |= CHANGED_GEOMETRYMODE;

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_HANDLED, "gSPClearGeometryMode( %s%s%s%s%s%s%s%s%s%s );\n",
        mode & G_SHADE ? "G_SHADE | " : "",
        mode & G_LIGHTING ? "G_LIGHTING | " : "",
        mode & G_SHADING_SMOOTH ? "G_SHADING_SMOOTH | " : "",
        mode & G_ZBUFFER ? "G_ZBUFFER | " : "",
        mode & G_TEXTURE_GEN ? "G_TEXTURE_GEN | " : "",
        mode & G_TEXTURE_GEN_LINEAR ? "G_TEXTURE_GEN_LINEAR | " : "",
        mode & G_CULL_FRONT ? "G_CULL_FRONT | " : "",
        mode & G_CULL_BACK ? "G_CULL_BACK | " : "",
        mode & G_FOG ? "G_FOG | " : "",
        mode & G_CLIPPING ? "G_CLIPPING" : "" );
#endif
}

void gSPLine3D( s32 v0, s32 v1, s32 flag )
{
    OGL_DrawLine( gSP.vertices, v0, v1, 1.5f );

#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLine3D( %i, %i, %i );\n", v0, v1, flag );
#endif
}

void gSPLineW3D( s32 v0, s32 v1, s32 wd, s32 flag )
{
    OGL_DrawLine( gSP.vertices, v0, v1, 1.5f + wd * 0.5f );
#ifdef DEBUG
    DebugMsg( DEBUG_HIGH | DEBUG_UNHANDLED, "gSPLineW3D( %i, %i, %i, %i );\n", v0, v1, wd, flag );
#endif
}

void gSPBgRect1Cyc( u32 bg )
{
    u32 address = RSP_SegmentToPhysical( bg );
    uObjScaleBg *objScaleBg = (uObjScaleBg*)&RDRAM[address];

    gSP.bgImage.address = RSP_SegmentToPhysical( objScaleBg->imagePtr );
    gSP.bgImage.width = objScaleBg->imageW >> 2;
    gSP.bgImage.height = objScaleBg->imageH >> 2;
    gSP.bgImage.format = objScaleBg->imageFmt;
    gSP.bgImage.size = objScaleBg->imageSiz;
    gSP.bgImage.palette = objScaleBg->imagePal;
    gDP.textureMode = TEXTUREMODE_BGIMAGE;

    f32 imageX = _FIXED2FLOAT( objScaleBg->imageX, 5 );
    f32 imageY = _FIXED2FLOAT( objScaleBg->imageY, 5 );
    f32 imageW = objScaleBg->imageW >> 2;
    f32 imageH = objScaleBg->imageH >> 2;

    f32 frameX = _FIXED2FLOAT( objScaleBg->frameX, 2 );
    f32 frameY = _FIXED2FLOAT( objScaleBg->frameY, 2 );
    f32 frameW = _FIXED2FLOAT( objScaleBg->frameW, 2 );
    f32 frameH = _FIXED2FLOAT( objScaleBg->frameH, 2 );
    f32 scaleW = _FIXED2FLOAT( objScaleBg->scaleW, 10 );
    f32 scaleH = _FIXED2FLOAT( objScaleBg->scaleH, 10 );

    f32 frameX0 = frameX;
    f32 frameY0 = frameY;
    f32 frameS0 = imageX;
    f32 frameT0 = imageY;

    f32 frameX1 = frameX + min( (imageW - imageX) / scaleW, frameW );
    f32 frameY1 = frameY + min( (imageH - imageY) / scaleH, frameH );
    //f32 frameS1 = imageX + min( (imageW - imageX) * scaleW, frameW * scaleW );
    //f32 frameT1 = imageY + min( (imageH - imageY) * scaleH, frameH * scaleH );

    gDP.otherMode.cycleType = G_CYC_1CYCLE;
    gDP.changed |= CHANGED_CYCLETYPE;
    gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );
    gDPTextureRectangle( frameX0, frameY0, frameX1 - 1, frameY1 - 1, 0, frameS0 - 1, frameT0 - 1, scaleW, scaleH );

    if ((frameX1 - frameX0) < frameW)
    {
        f32 frameX2 = frameW - (frameX1 - frameX0) + frameX1;
        gDPTextureRectangle( frameX1, frameY0, frameX2 - 1, frameY1 - 1, 0, 0, frameT0, scaleW, scaleH );
    }

    if ((frameY1 - frameY0) < frameH)
    {
        f32 frameY2 = frameH - (frameY1 - frameY0) + frameY1;
        gDPTextureRectangle( frameX0, frameY1, frameX1 - 1, frameY2 - 1, 0, frameS0, 0, scaleW, scaleH );
    }

    gDPTextureRectangle( 0, 0, 319, 239, 0, 0, 0, scaleW, scaleH );
/*  u32 line = (u32)(frameS1 - frameS0 + 1) << objScaleBg->imageSiz >> 4;
    u16 loadHeight;
    if (objScaleBg->imageFmt == G_IM_FMT_CI)
        loadHeight = 256 / line;
    else
        loadHeight = 512 / line;

    gDPSetTile( objScaleBg->imageFmt, objScaleBg->imageSiz, line, 0, 7, objScaleBg->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
    gDPSetTile( objScaleBg->imageFmt, objScaleBg->imageSiz, line, 0, 0, objScaleBg->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
    gDPSetTileSize( 0, 0, 0, frameS1 * 4, frameT1 * 4 );
    gDPSetTextureImage( objScaleBg->imageFmt, objScaleBg->imageSiz, imageW, objScaleBg->imagePtr );

    gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

    for (u32 i = 0; i < frameT1 / loadHeight; i++)
    {
        //if (objScaleBg->imageLoad == G_BGLT_LOADTILE)
            gDPLoadTile( 7, frameS0 * 4, (frameT0 + loadHeight * i) * 4, frameS1 * 4, (frameT1 + loadHeight * (i + 1) * 4 );
        //else
        //{
//          gDPSetTextureImage( objScaleBg->imageFmt, objScaleBg->imageSiz, imageW, objScaleBg->imagePtr + (i + imageY) * (imageW << objScaleBg->imageSiz >> 1) + (imageX << objScaleBg->imageSiz >> 1) );
//          gDPLoadBlock( 7, 0, 0, (loadHeight * frameW << objScaleBg->imageSiz >> 1) - 1, 0 );
//      }

        gDPTextureRectangle( frameX0, frameY0 + loadHeight * i,
            frameX1, frameY0 + loadHeight * (i + 1) - 1, 0, 0, 0, 4, 1 );
    }*/
}

void gSPBgRectCopy( u32 bg )
{
    u32 address = RSP_SegmentToPhysical( bg );
    uObjBg *objBg = (uObjBg*)&RDRAM[address];

    gSP.bgImage.address = RSP_SegmentToPhysical( objBg->imagePtr );
    gSP.bgImage.width = objBg->imageW >> 2;
    gSP.bgImage.height = objBg->imageH >> 2;
    gSP.bgImage.format = objBg->imageFmt;
    gSP.bgImage.size = objBg->imageSiz;
    gSP.bgImage.palette = objBg->imagePal;
    gDP.textureMode = TEXTUREMODE_BGIMAGE;

    u16 imageX = objBg->imageX >> 5;
    u16 imageY = objBg->imageY >> 5;

    s16 frameX = objBg->frameX / 4;
    s16 frameY = objBg->frameY / 4;
    u16 frameW = objBg->frameW >> 2;
    u16 frameH = objBg->frameH >> 2;

    gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

    gDPTextureRectangle( frameX, frameY, frameX + frameW - 1, frameY + frameH - 1, 0, imageX, imageY, 4, 1 );
}

void gSPObjRectangle( u32 sp )
{
    u32 address = RSP_SegmentToPhysical( sp );
    uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];

    f32 scaleW = _FIXED2FLOAT( objSprite->scaleW, 10 );
    f32 scaleH = _FIXED2FLOAT( objSprite->scaleH, 10 );
    f32 objX = _FIXED2FLOAT( objSprite->objX, 2 );
    f32 objY = _FIXED2FLOAT( objSprite->objY, 2 );
    u32 imageW = objSprite->imageW >> 2;
    u32 imageH = objSprite->imageH >> 2;

    gDPTextureRectangle( objX, objY, objX + imageW / scaleW - 1, objY + imageH / scaleH - 1, 0, 0.0f, 0.0f, scaleW * (gDP.otherMode.cycleType == G_CYC_COPY ? 4.0f : 1.0f), scaleH );
}

void gSPObjLoadTxtr( u32 tx )
{
    u32 address = RSP_SegmentToPhysical( tx );
    uObjTxtr *objTxtr = (uObjTxtr*)&RDRAM[address];

    if ((gSP.status[objTxtr->block.sid >> 2] & objTxtr->block.mask) != objTxtr->block.flag)
    {
        switch (objTxtr->block.type)
        {
            case G_OBJLT_TXTRBLOCK:
                gDPSetTextureImage( 0, 1, 0, objTxtr->block.image );
                gDPSetTile( 0, 1, 0, objTxtr->block.tmem, 7, 0, 0, 0, 0, 0, 0, 0 );
                gDPLoadBlock( 7, 0, 0, ((objTxtr->block.tsize + 1) << 3) - 1, objTxtr->block.tline );
                break;
            case G_OBJLT_TXTRTILE:
                gDPSetTextureImage( 0, 1, (objTxtr->tile.twidth + 1) << 1, objTxtr->tile.image );
                gDPSetTile( 0, 1, (objTxtr->tile.twidth + 1) >> 2, objTxtr->tile.tmem, 7, 0, 0, 0, 0, 0, 0, 0 );
                gDPLoadTile( 7, 0, 0, (((objTxtr->tile.twidth + 1) << 1) - 1) << 2, (((objTxtr->tile.theight + 1) >> 2) - 1) << 2 );
                break;
            case G_OBJLT_TLUT:
                gDPSetTextureImage( 0, 2, 1, objTxtr->tlut.image );
                gDPSetTile( 0, 2, 0, objTxtr->tlut.phead, 7, 0, 0, 0, 0, 0, 0, 0 );
                gDPLoadTLUT( 7, 0, 0, objTxtr->tlut.pnum << 2, 0 );
                break;
        }
        gSP.status[objTxtr->block.sid >> 2] = (gSP.status[objTxtr->block.sid >> 2] & ~objTxtr->block.mask) | (objTxtr->block.flag & objTxtr->block.mask);
    }
}

void gSPObjSprite( u32 sp )
{
    u32 address = RSP_SegmentToPhysical( sp );
    uObjSprite *objSprite = (uObjSprite*)&RDRAM[address];

    f32 scaleW = _FIXED2FLOAT( objSprite->scaleW, 10 );
    f32 scaleH = _FIXED2FLOAT( objSprite->scaleH, 10 );
    f32 objX = _FIXED2FLOAT( objSprite->objX, 2 );
    f32 objY = _FIXED2FLOAT( objSprite->objY, 2 );
    u32 imageW = objSprite->imageW >> 5;
    u32 imageH = objSprite->imageH >> 5;

    f32 x0 = objX;
    f32 y0 = objY;
    f32 x1 = objX + imageW / scaleW - 1;
    f32 y1 = objY + imageH / scaleH - 1;

    gSP.vertices[0].x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
    gSP.vertices[0].y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
    gSP.vertices[0].z = 0.0f;
    gSP.vertices[0].w = 1.0f;
    gSP.vertices[0].s = 0.0f;
    gSP.vertices[0].t = 0.0f;
    gSP.vertices[1].x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y0 + gSP.objMatrix.X;
    gSP.vertices[1].y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y0 + gSP.objMatrix.Y;
    gSP.vertices[1].z = 0.0f;
    gSP.vertices[1].w = 1.0f;
    gSP.vertices[1].s = imageW - 1;
    gSP.vertices[1].t = 0.0f;
    gSP.vertices[2].x = gSP.objMatrix.A * x1 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
    gSP.vertices[2].y = gSP.objMatrix.C * x1 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
    gSP.vertices[2].z = 0.0f;
    gSP.vertices[2].w = 1.0f;
    gSP.vertices[2].s = imageW - 1;
    gSP.vertices[2].t = imageH - 1;
    gSP.vertices[3].x = gSP.objMatrix.A * x0 + gSP.objMatrix.B * y1 + gSP.objMatrix.X;
    gSP.vertices[3].y = gSP.objMatrix.C * x0 + gSP.objMatrix.D * y1 + gSP.objMatrix.Y;
    gSP.vertices[3].z = 0.0f;
    gSP.vertices[3].w = 1.0f;
    gSP.vertices[3].s = 0;
    gSP.vertices[3].t = imageH - 1;

    gDPSetTile( objSprite->imageFmt, objSprite->imageSiz, objSprite->imageStride, objSprite->imageAdrs, 0, objSprite->imagePal, G_TX_CLAMP, G_TX_CLAMP, 0, 0, 0, 0 );
    gDPSetTileSize( 0, 0, 0, (imageW - 1) << 2, (imageH - 1) << 2 );
    gSPTexture( 1.0f, 1.0f, 0, 0, TRUE );

    VI_UpdateSize();

    //glOrtho( 0, VI.width, VI.height, 0, 0.0f, 32767.0f );
    for(int i = 0; i < 4;i++)
    {
        gSP.vertices[i].x = 2.0f * VI.rwidth * gSP.vertices[i].x - 1.0f;
        gSP.vertices[i].y = -2.0f * VI.rheight * gSP.vertices[i].y + 1.0f;
        gSP.vertices[i].z = -1.0f;
        gSP.vertices[i].w = 1.0f;
    }

    OGL_AddTriangle(0, 1, 2);
    OGL_AddTriangle(0, 2, 3);
    OGL_DrawTriangles();

    if (depthBuffer.current) depthBuffer.current->cleared = FALSE;
    gDP.colorImage.changed = TRUE;
    gDP.colorImage.height = (unsigned int)(max( gDP.colorImage.height, gDP.scissor.lry ));
}

void gSPObjLoadTxSprite( u32 txsp )
{
    gSPObjLoadTxtr( txsp );
    gSPObjSprite( txsp + sizeof( uObjTxtr ) );
}

void gSPObjLoadTxRectR( u32 txsp )
{
    gSPObjLoadTxtr( txsp );
//  gSPObjRectangleR( txsp + sizeof( uObjTxtr ) );
}

void gSPObjMatrix( u32 mtx )
{
    u32 address = RSP_SegmentToPhysical( mtx );
    uObjMtx *objMtx = (uObjMtx*)&RDRAM[address];

    gSP.objMatrix.A = _FIXED2FLOAT( objMtx->A, 16 );
    gSP.objMatrix.B = _FIXED2FLOAT( objMtx->B, 16 );
    gSP.objMatrix.C = _FIXED2FLOAT( objMtx->C, 16 );
    gSP.objMatrix.D = _FIXED2FLOAT( objMtx->D, 16 );
    gSP.objMatrix.X = _FIXED2FLOAT( objMtx->X, 2 );
    gSP.objMatrix.Y = _FIXED2FLOAT( objMtx->Y, 2 );
    gSP.objMatrix.baseScaleX = _FIXED2FLOAT( objMtx->BaseScaleX, 10 );
    gSP.objMatrix.baseScaleY = _FIXED2FLOAT( objMtx->BaseScaleY, 10 );
}

void gSPObjSubMatrix( u32 mtx )
{
}

