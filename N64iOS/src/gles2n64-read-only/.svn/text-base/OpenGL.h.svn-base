#ifndef OPENGL_H
#define OPENGL_H

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>
#include <GLES2/gl2extimg.h>
#include "winlnxdefs.h"
#include "SDL.h"

#include "gSP.h"

#if 0
#define LOG(...)    fprintf(stdout, __VA_ARGS__); fflush(stdout)
#else
#define LOG(...)
#endif

#define RS_NONE         0
#define RS_TRIANGLE     1
#define RS_RECT         2
#define RS_TEXTUREDRECT 3
#define RS_LINE         4

struct GLVertex
{
    float x, y, z, w;
    struct
    {
        float r, g, b, a;
    } color, secondaryColor;
    float s0, t0, s1, t1;
    //float fog;
};

struct GLInfo
{
#ifdef WIN32
    SDL_Surface *hScreen;
#endif

    int     screenWidth, screenHeight;
    int     width, height;
    int     xpos, ypos;
    int     centre;
    int     frameSkip, vSync, frame;
    int     logFrameRate;
    int     enableFog;
    int     enableLighting;
    int     enableAlphaTest;
    int     enableClipping;
    int     accurateRDP;
    int     fullscreen, forceBilinear;
    int     enable2xSaI;
    int     enableAnisotropicFiltering;
    int     maxAnisotropy;
    int     textureBitDepth;
    int     textureMipmap;


    float   scaleX, scaleY;
    GLubyte elements[255];
    int     numElements;
    unsigned int    renderState;

    GLVertex rect[4];

#ifndef __LINUX__
    HWND    hFullscreenWnd;
#endif

    BYTE    combiner;
};

extern GLInfo OGL;

struct GLcolor
{
    float r, g, b, a;
};

bool OGL_Start();
void OGL_Stop();

void OGL_AddTriangle(int v0, int v1, int v2);
void OGL_DrawTriangles();
void OGL_DrawTriangle(SPVertex *vertices, int v0, int v1, int v2);
void OGL_DrawLine(SPVertex *vertices, int v0, int v1, float width);
void OGL_DrawRect(int ulx, int uly, int lrx, int lry, float *color);
void OGL_DrawTexturedRect(float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip );

void OGL_UpdateScale();
void OGL_UpdateStates();
void OGL_UpdateViewport();
void OGL_UpdateScissor();
void OGL_UpdateCullFace();

void OGL_ClearDepthBuffer();
void OGL_ClearColorBuffer(float *color);
void OGL_ResizeWindow();
void OGL_SaveScreenshot();
void OGL_SwapBuffers();
void OGL_ReadScreen( void *dest, int *width, int *height );

int  OGL_CheckError();
int  OGL_IsExtSupported( const char *extension );
#endif

