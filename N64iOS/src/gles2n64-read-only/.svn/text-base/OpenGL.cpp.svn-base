
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <time.h>
#include <png.h>
#include <SDL.h>
#include <SDL/SDL_syswm.h>

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#define timeGetTime() time(NULL)

#include "winlnxdefs.h"
#include "gles2N64.h"
#include "OpenGL.h"
#include "Types.h"
#include "N64.h"
#include "gSP.h"
#include "gDP.h"
#include "Textures.h"
#include "ShaderCombiner.h"
#include "VI.h"

//#define BATCH_TEST

#ifdef BATCH_TEST
int     TotalTriangles = 0;
int     TotalDrawCalls = 0;
#define glDrawElements(A,B,C,D) TotalTriangles += B; TotalDrawCalls++; glDrawElements(A,B,C,D)
#define glDrawArrays(A,B,C)     TotalTriangles += C; TotalDrawCalls++; glDrawArrays(A,B,C)

#endif

GLInfo OGL;

EGLint		VersionMajor;
EGLint		VersionMinor;

EGLDisplay  Display;
EGLContext  Context;
EGLConfig   Config;
EGLSurface  Surface;

EGLNativeDisplayType    Device;
EGLNativeWindowType     Handle;

const EGLint ConfigAttribs[] =
{
	EGL_LEVEL,				0,
	EGL_DEPTH_SIZE,         16,
	EGL_STENCIL_SIZE,       0,
	EGL_SURFACE_TYPE,		EGL_WINDOW_BIT,
	EGL_RENDERABLE_TYPE,	EGL_OPENGL_ES2_BIT,
	EGL_NATIVE_RENDERABLE,	EGL_FALSE,
	EGL_NONE
};

const EGLint ContextAttribs[] =
{
	EGL_CONTEXT_CLIENT_VERSION, 	2,
	EGL_NONE
};

void OGL_EnableRunfast()
{
#ifdef __arm__
	static const unsigned int x = 0x04086060;
	static const unsigned int y = 0x03000000;
	int r;
	asm volatile (
		"fmrx	%0, fpscr			\n\t"	//r0 = FPSCR
		"and	%0, %0, %1			\n\t"	//r0 = r0 & 0x04086060
		"orr	%0, %0, %2			\n\t"	//r0 = r0 | 0x03000000
		"fmxr	fpscr, %0			\n\t"	//FPSCR = r0
		: "=r"(r)
		: "r"(x), "r"(y)
	);
#endif
}

const char* EGLErrorString()
{
	EGLint nErr = eglGetError();
	switch(nErr){
		case EGL_SUCCESS: 				return "EGL_SUCCESS";
		case EGL_BAD_DISPLAY:			return "EGL_BAD_DISPLAY";
		case EGL_NOT_INITIALIZED:		return "EGL_NOT_INITIALIZED";
		case EGL_BAD_ACCESS:			return "EGL_BAD_ACCESS";
		case EGL_BAD_ALLOC:				return "EGL_BAD_ALLOC";
		case EGL_BAD_ATTRIBUTE:			return "EGL_BAD_ATTRIBUTE";
		case EGL_BAD_CONFIG:			return "EGL_BAD_CONFIG";
		case EGL_BAD_CONTEXT:			return "EGL_BAD_CONTEXT";
		case EGL_BAD_CURRENT_SURFACE:	return "EGL_BAD_CURRENT_SURFACE";
		case EGL_BAD_MATCH:				return "EGL_BAD_MATCH";
		case EGL_BAD_NATIVE_PIXMAP:		return "EGL_BAD_NATIVE_PIXMAP";
		case EGL_BAD_NATIVE_WINDOW:		return "EGL_BAD_NATIVE_WINDOW";
		case EGL_BAD_PARAMETER:			return "EGL_BAD_PARAMETER";
		case EGL_BAD_SURFACE:			return "EGL_BAD_SURFACE";
		default:						return "unknown";
	}
};

int OGL_IsExtSupported( const char *extension )
{
	const GLubyte *extensions = NULL;
	const GLubyte *start;
	GLubyte *where, *terminator;

	where = (GLubyte *) strchr(extension, ' ');
	if (where || *extension == '\0')
		return 0;

	extensions = glGetString(GL_EXTENSIONS);

    if (!extensions) return 0;

	start = extensions;
	for (;;)
	{
		where = (GLubyte *) strstr((const char *) start, extension);
		if (!where)
			break;

		terminator = where + strlen(extension);
		if (where == start || *(where - 1) == ' ')
			if (*terminator == ' ' || *terminator == '\0')
				return 1;

		start = terminator;
	}

	return 0;
}

void OGL_InitStates()
{
    glEnableVertexAttribArray(SC_POSITION);
    glPolygonOffset(3.0f, 3.0f);
    glClearDepthf(0.0f);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_ALWAYS);
    glDepthMask(GL_FALSE);
    glEnable(GL_SCISSOR_TEST);
    glDepthRangef(1.0f, 0.0f);
    OGL_SwapBuffers();
}

void OGL_UpdateScale()
{
    OGL.scaleX = (float)OGL.width / (float)VI.width;
    OGL.scaleY = (float)OGL.height / (float)VI.height;
}

void OGL_ResizeWindow()
{
    //hmmm
}

bool OGL_Start()
{
    EGLint nConfigs;

    SDL_Init(SDL_INIT_TIMER);

#ifdef WIN32
    const SDL_VideoInfo *videoInfo;

    /* Initialize SDL */
    printf( "[gles2n64]: Initializing SDL video subsystem...\n" );
    if (SDL_InitSubSystem( SDL_INIT_VIDEO ) == -1)
    {
        printf( "[gles2n64]: Error initializing SDL video subsystem: %s\n", SDL_GetError() );
        return FALSE;
    }

    /* Video Info */
    printf( "[gles2n64]: Getting video info...\n" );
    if (!(videoInfo = SDL_GetVideoInfo()))
    {
        printf( "[gles2n64]: Video query failed: %s\n", SDL_GetError() );
        SDL_QuitSubSystem( SDL_INIT_VIDEO );
        return FALSE;
    }
    /* Set the video mode */
    printf( "[glN64]: (II) Setting video mode %dx%d...\n", (int)OGL.width, (int)OGL.height );
    if (!(OGL.hScreen = SDL_SetVideoMode( OGL.width, OGL.height - 32, 16, SDL_SWSURFACE )))
    {
        printf( "[glN64]: (EE) Error setting videomode %dx%d: %s\n", (int)OGL.width, (int)OGL.height, SDL_GetError() );
        SDL_QuitSubSystem( SDL_INIT_VIDEO );
        return FALSE;
    }
    SDL_WM_SetCaption( pluginName, pluginName );

	SDL_SysWMinfo info;
	SDL_VERSION(&info.version);
	SDL_GetWMInfo(&info);
	Handle = (EGLNativeWindowType) info.window;
	Device = GetDC(Handle);
#else
	Handle = NULL;
    Device = 0;
#endif

    printf( "[gles2n64]: EGL Context Creation\n");
    Display = eglGetDisplay((EGLNativeDisplayType) Device);
    if (Display == EGL_NO_DISPLAY){
        printf("[gles2n64]: EGL Display Get failed: %s \n", EGLErrorString());
        return FALSE;
    }

    if (!eglInitialize(Display, &VersionMajor, &VersionMinor)){
        printf("[gles2n64]: EGL Display Initialize failed: %s \n", EGLErrorString()); fflush(stdout);
        return FALSE;
    }

    if (!eglChooseConfig(Display, ConfigAttribs, &Config, 1, &nConfigs)){
        printf( "[gles2n64]: EGL Configuration failed: %s \n", EGLErrorString()); fflush(stdout);
        return FALSE;
    } else if (nConfigs != 1){
        printf( "[gles2n64]: EGL Configuration failed: nconfig %i, %s \n", nConfigs, EGLErrorString()); fflush(stdout);
        return FALSE;
    }

    Surface = eglCreateWindowSurface(Display, Config, Handle, NULL);
    if (Surface == EGL_NO_SURFACE){
		printf("[gles2n64]: EGL Surface Creation failed: %s will attempt without window... \n", EGLErrorString()); fflush(stdout);
        Surface = eglCreateWindowSurface(Display, Config, NULL, NULL);
        if (Surface == EGL_NO_SURFACE){
            printf("[gles2n64]: EGL Surface Creation failed: %s \n", EGLErrorString()); fflush(stdout);
            return FALSE;
        }
    }
    eglBindAPI(EGL_OPENGL_ES_API);

    Context = eglCreateContext(Display, Config, EGL_NO_CONTEXT, ContextAttribs);
    if (Context == EGL_NO_CONTEXT){
        printf("[gles2n64]: EGL Context Creation failed: %s \n", EGLErrorString()); fflush(stdout);
        return FALSE;
    }

    if (!eglMakeCurrent(Display, Surface, Surface, Context)){
        printf("[gles2n64]: EGL Make Current failed: %s \n", EGLErrorString()); fflush(stdout);
        return FALSE;
    };

    eglSwapInterval(Display, OGL.vSync);

    //Print some info
    EGLint attrib;
    printf( "[gles2n64]: Width: %i Height:%i \n", OGL.width, OGL.height);
    eglGetConfigAttrib(Display, Config, EGL_DEPTH_SIZE, &attrib);
    printf( "[gles2n64]: Depth Size: %i \n", attrib);
    eglGetConfigAttrib(Display, Config, EGL_BUFFER_SIZE, &attrib);
    printf( "[gles2n64]: Color Buffer Size: %i \n", attrib);

    printf( "[gles2n64]: Enable Runfast... \n");
    OGL_EnableRunfast();

    OGL_InitStates();
    OGL_UpdateScale();

    //check extensions
    if (OGL.enableAnisotropicFiltering && !OGL_IsExtSupported("GL_EXT_texture_filter_anistropic"))
    {
        printf("[gles2n64]: Anistropic Filtering is not supported.\n");
        OGL.enableAnisotropicFiltering = 0;
    }

    float f = 0;
    glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &f);
    if (OGL.maxAnisotropy > (int)f)
    {
        printf("[gles2n64]: Clamping max anistropy to %ix.\n", (int)f);
        OGL.maxAnisotropy = (int)f;
    }


    //We must have a shader bound before binding any textures:
    ShaderCombiner_Init();
    ShaderCombiner_Set(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
    ShaderCombiner_Set(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1));
    TextureCache_Init();

    memset(gSP.vertices, 0, 80 * sizeof(SPVertex));
    OGL.numElements = 0;
    OGL.renderState = RS_NONE;
    gSP.changed = gDP.changed = 0xFFFFFFFF;

    return TRUE;
}

void OGL_Stop()
{
#ifdef WIN32
    SDL_QuitSubSystem( SDL_INIT_VIDEO );
#endif

    ShaderCombiner_Destroy();
    TextureCache_Destroy();

	eglMakeCurrent(Display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
	eglDestroySurface(Display, Surface);
 	eglDestroyContext(Display, Context);
   	eglTerminate(Display);
}

void OGL_UpdateCullFace()
{
    if (gSP.geometryMode & G_CULL_BOTH)
    {
        glEnable( GL_CULL_FACE );
        if (gSP.geometryMode & G_CULL_BACK)
            glCullFace(GL_BACK);
        else
            glCullFace(GL_FRONT);
    }
    else
        glDisable(GL_CULL_FACE);
}

void OGL_UpdateViewport()
{
    int x, y, w, h;
    x = OGL.xpos + (int)(gSP.viewport.x * OGL.scaleX);
    y = OGL.ypos + (int)((VI.height - (gSP.viewport.y + gSP.viewport.height)) * OGL.scaleY);
    w = (int)(gSP.viewport.width * OGL.scaleX);
    h = (int)(gSP.viewport.height * OGL.scaleY);
    glViewport(x, y, w, h);
}

void OGL_UpdateDepthUpdate()
{
    if (gDP.otherMode.depthUpdate)
        glDepthMask(GL_TRUE);
    else
        glDepthMask(GL_FALSE);
}

void OGL_UpdateScissor()
{
    int x, y, w, h;
    x =  OGL.xpos + (int)(gDP.scissor.ulx * OGL.scaleX);
    y = OGL.ypos + (int)((VI.height - gDP.scissor.lry) * OGL.scaleY);
    w = (int)((gDP.scissor.lrx - gDP.scissor.ulx) * OGL.scaleX);
    h = (int)((gDP.scissor.lry - gDP.scissor.uly) * OGL.scaleY);
    glScissor(x, y, w, h);
}

void OGL_UpdateStates()
{
    if (gDP.otherMode.cycleType == G_CYC_COPY)
        ShaderCombiner_Set(EncodeCombineMode(0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0, 0, 0, 0, TEXEL0));
    else if (gDP.otherMode.cycleType == G_CYC_FILL)
        ShaderCombiner_Set(EncodeCombineMode(0, 0, 0, SHADE, 0, 0, 0, 1, 0, 0, 0, SHADE, 0, 0, 0, 1));
    else
        ShaderCombiner_Set(gDP.combine.mux);

    if (gSP.changed & CHANGED_GEOMETRYMODE)
    {
        OGL_UpdateCullFace();

        if (gSP.geometryMode & G_ZBUFFER)
            glEnable( GL_DEPTH_TEST );
        else
            glDisable( GL_DEPTH_TEST );

    }

    if (gDP.changed & CHANGED_RENDERMODE)
    {
        glDepthFunc((gDP.otherMode.depthCompare) ? GL_GEQUAL : GL_ALWAYS);
        glDepthMask((gDP.otherMode.depthUpdate) ? GL_TRUE : GL_FALSE);

        if (gDP.otherMode.depthMode == ZMODE_DEC)
            glEnable( GL_POLYGON_OFFSET_FILL );
        else
            glDisable( GL_POLYGON_OFFSET_FILL );
    }

    if (gDP.changed & CHANGED_DEPTHSOURCE)
        SC_SetUniform1i(uEnablePrimitiveZ, (gDP.otherMode.depthSource == G_ZS_PRIM));

    if (gDP.changed & CHANGED_PRIMITIVEZ)
        SC_SetUniform1f(uPrimitiveZ, gDP.primDepth.z);

    if ((gDP.changed & CHANGED_ALPHACOMPARE) || (gDP.changed & CHANGED_RENDERMODE))
        SC_SetUniform1f(uAlphaRef, (gDP.otherMode.cvgXAlpha) ? 0.5f : gDP.blendColor.a);

    if (gDP.changed & CHANGED_SCISSOR)
        OGL_UpdateScissor();

    if (gSP.changed & CHANGED_VIEWPORT)
        OGL_UpdateViewport();

    if (gSP.changed & CHANGED_FOGPOSITION)
    {
        SC_SetUniform1f(uFogMultiplier, (float) gSP.fog.multiplier);
        SC_SetUniform1f(uFogOffset, (float) gSP.fog.offset);
    }

    if (gSP.changed & CHANGED_TEXTURESCALE)
    {
        if (scProgramCurrent->usesT0 || scProgramCurrent->usesT1)
            SC_SetUniform2f(uTexScale, gSP.texture.scales, gSP.texture.scalet);
    }

    if ((gSP.changed & CHANGED_TEXTURE) || (gDP.changed & CHANGED_TILE) || (gDP.changed & CHANGED_TMEM))
    {
        if (scProgramCurrent->usesT0)
        {
            TextureCache_Update(0);
            SC_SetUniform2f(uTexOffset[0], gSP.textureTile[0]->fuls, gSP.textureTile[0]->fult);
            SC_SetUniform2f(uCacheShiftScale[0], cache.current[0]->shiftScaleS, cache.current[0]->shiftScaleT);
            SC_SetUniform2f(uCacheScale[0], cache.current[0]->scaleS, cache.current[0]->scaleT);
            SC_SetUniform2f(uCacheOffset[0], cache.current[0]->offsetS, cache.current[0]->offsetT);
        }
        //else TextureCache_ActivateDummy(0);

        if (scProgramCurrent->usesT1)
        {
            TextureCache_Update(1);
            SC_SetUniform2f(uTexOffset[1], gSP.textureTile[1]->fuls, gSP.textureTile[1]->fult);
            SC_SetUniform2f(uCacheShiftScale[1], cache.current[1]->shiftScaleS, cache.current[1]->shiftScaleT);
            SC_SetUniform2f(uCacheScale[1], cache.current[1]->scaleS, cache.current[1]->scaleT);
            SC_SetUniform2f(uCacheOffset[1], cache.current[1]->offsetS, cache.current[1]->offsetT);
        }
        //else TextureCache_ActivateDummy(1);
    }

    if ((gDP.changed & CHANGED_FOGCOLOR) && OGL.enableFog)
        SC_SetUniform4fv(uFogColor, &gDP.fogColor.r );

    if (gDP.changed & CHANGED_ENV_COLOR)
        SC_SetUniform4fv(uEnvColor, &gDP.envColor.r);

    if (gDP.changed & CHANGED_PRIM_COLOR)
    {
        SC_SetUniform4fv(uPrimColor, &gDP.primColor.r);
        SC_SetUniform1f(uPrimLODFrac, gDP.primColor.l);
    }

    if ((gDP.changed & CHANGED_RENDERMODE) || (gDP.changed & CHANGED_CYCLETYPE))
    {
        if ((gDP.otherMode.forceBlender) &&
            (gDP.otherMode.cycleType != G_CYC_COPY) &&
            (gDP.otherMode.cycleType != G_CYC_FILL) &&
            !(gDP.otherMode.alphaCvgSel))
        {
            glEnable( GL_BLEND );

            switch (gDP.otherMode.l >> 16)
            {
                case 0x0448: // Add
                case 0x055A:
                    glBlendFunc( GL_ONE, GL_ONE );
                    break;
                case 0x0C08: // 1080 Sky
                case 0x0F0A: // Used LOTS of places
                    glBlendFunc( GL_ONE, GL_ZERO );
                    break;
                case 0xC810: // Blends fog
                case 0xC811: // Blends fog
                case 0x0C18: // Standard interpolated blend
                case 0x0C19: // Used for antialiasing
                case 0x0050: // Standard interpolated blend
                case 0x0055: // Used for antialiasing
                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    break;
                case 0x0FA5: // Seems to be doing just blend color - maybe combiner can be used for this?
                case 0x5055: // Used in Paper Mario intro, I'm not sure if this is right...
                    glBlendFunc( GL_ZERO, GL_ONE );
                    break;
                default:
                    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
                    break;
            }
        }
        else
        {
            glDisable( GL_BLEND );
        }

        if (gDP.otherMode.cycleType == G_CYC_FILL)
        {
            glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
            glEnable( GL_BLEND );
        }
    }

    gDP.changed &= CHANGED_TILE | CHANGED_TMEM;
    gSP.changed &= CHANGED_TEXTURE | CHANGED_MATRIX;
}

void OGL_DrawTriangle(SPVertex *vertices, int v0, int v1, int v2)
{

}

void OGL_AddTriangle(int v0, int v1, int v2 )
{
    OGL.elements[OGL.numElements++] = v0;
    OGL.elements[OGL.numElements++] = v1;
    OGL.elements[OGL.numElements++] = v2;
}

void OGL_SetColorArray()
{
    if (scProgramCurrent->usesCol)
        glEnableVertexAttribArray(SC_COLOR);
    else
        glDisableVertexAttribArray(SC_COLOR);
}

void OGL_SetTexCoordArrays()
{
    if (scProgramCurrent->usesT0)
        glEnableVertexAttribArray(SC_TEXCOORD0);
    else
        glDisableVertexAttribArray(SC_TEXCOORD0);

    if (scProgramCurrent->usesT1)
        glEnableVertexAttribArray(SC_TEXCOORD1);
    else
        glDisableVertexAttribArray(SC_TEXCOORD1);
}

void OGL_DrawTriangles()
{
    if (gSP.changed || gDP.changed)
        OGL_UpdateStates();

    if (OGL.renderState != RS_TRIANGLE || scProgramChanged)
    {
        OGL_SetColorArray();
        OGL_SetTexCoordArrays();
        glDisableVertexAttribArray(SC_TEXCOORD1);
        SC_SetUniform1i(uTriangleRS, 1);
    }

    if (OGL.renderState != RS_TRIANGLE)
    {
        glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &gSP.vertices[0].x);
#ifdef __PACKVERTEX_OPT
        glVertexAttribPointer(SC_COLOR, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(SPVertex), &gSP.vertices[0].r);
#else
        glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &gSP.vertices[0].r);
#endif
        glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(SPVertex), &gSP.vertices[0].s);

        OGL_UpdateCullFace();
        OGL_UpdateViewport();
        glEnable(GL_SCISSOR_TEST);
        OGL.renderState = RS_TRIANGLE;
    }

    glDrawElements(GL_TRIANGLES, OGL.numElements, GL_UNSIGNED_BYTE, OGL.elements);
    OGL.numElements = 0;
}

void OGL_DrawLine( SPVertex *vertices, int v0, int v1, float width )
{
    int v[] = { v0, v1 };

    if (gSP.changed || gDP.changed)
        OGL_UpdateStates();

    if (OGL.renderState != RS_LINE || scProgramChanged)
    {
        OGL_SetColorArray();
        glDisableVertexAttribArray(SC_TEXCOORD0);
        glDisableVertexAttribArray(SC_TEXCOORD1);
        SC_SetUniform1i(uTriangleRS, 0);
        OGL_UpdateCullFace();
        OGL_UpdateViewport();
        OGL.renderState = RS_LINE;
    }

    glLineWidth( width * OGL.scaleX );
    float vert[8];
    GLcolor color[2];
    for (int i = 0; i < 2; i++)
    {
        vert[i*4+0] = vertices[v[i]].x;
        vert[i*4+1] = vertices[v[i]].y;
        vert[i*4+2] = vertices[v[i]].z;
        vert[i*4+3] = vertices[v[i]].w;
        color[i].r = vertices[v[i]].r;
        color[i].g = vertices[v[i]].g;
        color[i].b = vertices[v[i]].b;
        color[i].a = vertices[v[i]].a;
    }

    glVertexAttribPointer(SC_POSITION, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat), vert);
    glVertexAttribPointer(SC_COLOR, 4, GL_FLOAT, GL_FALSE, sizeof(GLfloat), &color[0].r);
    glDrawArrays(GL_LINES, 0, 2);
}

void OGL_DrawRect( int ulx, int uly, int lrx, int lry, float *color)
{
    if (gSP.changed || gDP.changed)
        OGL_UpdateStates();

    if (OGL.renderState != RS_RECT || scProgramChanged)
    {
        glDisableVertexAttribArray(SC_COLOR);
        glDisableVertexAttribArray(SC_TEXCOORD0);
        glDisableVertexAttribArray(SC_TEXCOORD1);
        SC_SetUniform1i(uTriangleRS, 0);
    }

    if (OGL.renderState != RS_RECT)
    {
        glVertexAttrib4f(SC_POSITION, 0, 0, gSP.viewport.nearz, 1.0);
        glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
        OGL.renderState = RS_RECT;
    }

    glViewport( OGL.xpos, OGL.ypos, OGL.width, OGL.height );
    //glDisable(GL_SCISSOR_TEST);
    glDisable(GL_CULL_FACE);

    OGL.rect[0].x = (float) ulx * (2.0f * VI.rwidth) - 1.0;
    OGL.rect[0].y = (float) uly * (-2.0f * VI.rheight) + 1.0;
    OGL.rect[1].x = (float) lrx * (2.0f * VI.rwidth) - 1.0;
    OGL.rect[1].y = OGL.rect[0].y;
    OGL.rect[2].x = OGL.rect[0].x;
    OGL.rect[2].y = (float) lry * (-2.0f * VI.rheight) + 1.0;
    OGL.rect[3].x = OGL.rect[1].x;
    OGL.rect[3].y = OGL.rect[2].y;

    glVertexAttrib4fv(SC_COLOR, color);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void OGL_DrawTexturedRect( float ulx, float uly, float lrx, float lry, float uls, float ult, float lrs, float lrt, bool flip )
{
    if (gSP.changed || gDP.changed)
        OGL_UpdateStates();

    if (OGL.renderState != RS_TEXTUREDRECT || scProgramChanged)
    {
        glDisableVertexAttribArray(SC_COLOR);
        OGL_SetTexCoordArrays();
        SC_SetUniform1i(uTriangleRS, 0);
    }

    if (OGL.renderState != RS_TEXTUREDRECT)
    {
        glVertexAttrib4f(SC_POSITION, 0, 0, gSP.viewport.nearz, 1.0);
        glVertexAttribPointer(SC_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].x);
        glVertexAttribPointer(SC_TEXCOORD0, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s0);
        glVertexAttribPointer(SC_TEXCOORD1, 2, GL_FLOAT, GL_FALSE, sizeof(GLVertex), &OGL.rect[0].s1);
        OGL_UpdateScissor();
        OGL.renderState = RS_TEXTUREDRECT;
    }

    glViewport( OGL.xpos, OGL.ypos, OGL.width, OGL.height );
    glDisable(GL_CULL_FACE);
    //glDisable(GL_SCISSOR_TEST);

    OGL.rect[0].x = (float) ulx * (2.0f * VI.rwidth) - 1.0f;
    OGL.rect[0].y = (float) uly * (-2.0f * VI.rheight) + 1.0f;
    OGL.rect[1].x = (float) lrx * (2.0f * VI.rwidth) - 1.0f;
    OGL.rect[1].y = OGL.rect[0].y;
    OGL.rect[2].x = OGL.rect[0].x;
    OGL.rect[2].y = (float) lry * (-2.0f * VI.rheight) + 1.0f;
    OGL.rect[3].x = OGL.rect[1].x;
    OGL.rect[3].y = OGL.rect[2].y;

    if (scProgramCurrent->usesT0 && cache.current[0] && gSP.textureTile[0])
    {
        OGL.rect[0].s0 = uls * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
        OGL.rect[0].t0 = ult * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;
        OGL.rect[3].s0 = (lrs + 1.0f) * cache.current[0]->shiftScaleS - gSP.textureTile[0]->fuls;
        OGL.rect[3].t0 = (lrt + 1.0f) * cache.current[0]->shiftScaleT - gSP.textureTile[0]->fult;

        if ((cache.current[0]->maskS) && (fmod( OGL.rect[0].s0, cache.current[0]->width ) == 0.0f) && !(cache.current[0]->mirrorS))
        {
            OGL.rect[3].s0 -= OGL.rect[0].s0;
            OGL.rect[0].s0 = 0.0f;
        }

        if ((cache.current[0]->maskT) && (fmod( OGL.rect[0].t0, cache.current[0]->height ) == 0.0f) && !(cache.current[0]->mirrorT))
        {
            OGL.rect[3].t0 -= OGL.rect[0].t0;
            OGL.rect[0].t0 = 0.0f;
        }

#if 0
        glActiveTexture( GL_TEXTURE0);

        if ((OGL.rect[0].s0 >= 0.0f) && (OGL.rect[3].s0 <= cache.current[0]->width))
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

        if ((OGL.rect[0].t0 >= 0.0f) && (OGL.rect[3].t0 <= cache.current[0]->height))
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#endif

        OGL.rect[0].s0 *= cache.current[0]->scaleS;
        OGL.rect[0].t0 *= cache.current[0]->scaleT;
        OGL.rect[3].s0 *= cache.current[0]->scaleS;
        OGL.rect[3].t0 *= cache.current[0]->scaleT;
    }

    if (scProgramCurrent->usesT1 && cache.current[1] && gSP.textureTile[1])
    {
        OGL.rect[0].s1 = uls * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
        OGL.rect[0].t1 = ult * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;
        OGL.rect[3].s1 = (lrs + 1.0f) * cache.current[1]->shiftScaleS - gSP.textureTile[1]->fuls;
        OGL.rect[3].t1 = (lrt + 1.0f) * cache.current[1]->shiftScaleT - gSP.textureTile[1]->fult;

        if ((cache.current[1]->maskS) && (fmod( OGL.rect[0].s1, cache.current[1]->width ) == 0.0f) && !(cache.current[1]->mirrorS))
        {
            OGL.rect[3].s1 -= OGL.rect[0].s1;
            OGL.rect[0].s1 = 0.0f;
        }

        if ((cache.current[1]->maskT) && (fmod( OGL.rect[0].t1, cache.current[1]->height ) == 0.0f) && !(cache.current[1]->mirrorT))
        {
            OGL.rect[3].t1 -= OGL.rect[0].t1;
            OGL.rect[0].t1 = 0.0f;
        }

#if 0
        glActiveTextureARB( GL_TEXTURE1_ARB );
        if ((OGL.rect[0].s1 == 0.0f) && (OGL.rect[3].s1 <= cache.current[1]->width))
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );

        if ((OGL.rect[0].t1 == 0.0f) && (OGL.rect[3].t1 <= cache.current[1]->height))
            glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
#endif
        OGL.rect[0].s1 *= cache.current[1]->scaleS;
        OGL.rect[0].t1 *= cache.current[1]->scaleT;
        OGL.rect[3].s1 *= cache.current[1]->scaleS;
        OGL.rect[3].t1 *= cache.current[1]->scaleT;
    }

#if 0
    if ((gDP.otherMode.cycleType == G_CYC_COPY) && !OGL.forceBilinear)
    {
        glActiveTexture(GL_TEXTURE0);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
    }
#endif

    OGL.rect[1].s0 = OGL.rect[3].s0;
    OGL.rect[1].t0 = OGL.rect[0].t0;
    OGL.rect[1].s1 = OGL.rect[3].s1;
    OGL.rect[1].t1 = OGL.rect[0].t1;
    OGL.rect[2].s0 = OGL.rect[0].s0;
    OGL.rect[2].t0 = OGL.rect[3].t0;
    OGL.rect[2].s1 = OGL.rect[0].s1;
    OGL.rect[2].t1 = OGL.rect[3].t1;

    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}

void OGL_ClearDepthBuffer()
{
    glDisable( GL_SCISSOR_TEST );
    glDepthMask(GL_TRUE );
    glClear( GL_DEPTH_BUFFER_BIT );
    OGL_UpdateDepthUpdate();
    glEnable( GL_SCISSOR_TEST );
}

void OGL_ClearColorBuffer( float *color )
{
    glDisable( GL_SCISSOR_TEST );
    glClearColor( color[0], color[1], color[2], color[3] );
    glClear( GL_COLOR_BUFFER_BIT );
    glEnable( GL_SCISSOR_TEST );
}

static void OGL_png_error(png_structp png_write, const char *message)
{
    printf("PNG Error: %s\n", message);
}

static void OGL_png_warn(png_structp png_write, const char *message)
{
    printf("PNG Warning: %s\n", message);
}

void OGL_SaveScreenshot()
{
    // start by getting the base file path
    char filepath[2048], filename[2048];
    filepath[0] = 0;
    filename[0] = 0;
    strcpy(filepath, screenDirectory);
    if (strlen(filepath) > 0 && filepath[strlen(filepath)-1] != '/')
        strcat(filepath, "/");
    strcat(filepath, "mupen64");
    // look for a file
    int i;
    for (i = 0; i < 100; i++)
    {
        sprintf(filename, "%s_%03i.png", filepath, i);
        FILE *pFile = fopen(filename, "r");
        if (pFile == NULL)
            break;
        fclose(pFile);
    }
    if (i == 100) return;
    // allocate PNG structures
    png_structp png_write = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, OGL_png_error, OGL_png_warn);
    if (!png_write)
    {
        printf("Error creating PNG write struct.\n");
        return;
    }
    png_infop png_info = png_create_info_struct(png_write);
    if (!png_info)
    {
        png_destroy_write_struct(&png_write, (png_infopp)NULL);
        printf("Error creating PNG info struct.\n");
        return;
    }
    // Set the jumpback
    if (setjmp(png_jmpbuf(png_write)))
    {
        png_destroy_write_struct(&png_write, &png_info);
        printf("Error calling setjmp()\n");
        return;
    }
    // open the file to write
    FILE *savefile = fopen(filename, "wb");
    if (savefile == NULL)
    {
        printf("Error opening '%s' to save screenshot.\n", filename);
        return;
    }
    // give the file handle to the PNG compressor
    png_init_io(png_write, savefile);

    // read pixel data from OpenGL
    char *pixels;
    int width, height;
    OGL_ReadScreen(pixels, &width, &height );

    // set the info
    png_set_IHDR(png_write, png_info, width, height, 8, PNG_COLOR_TYPE_RGB,
                 PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    // lock the screen, get a pointer and row pitch
    long pitch = width * 3;
    // allocate row pointers and scale each row to 24-bit color
    png_byte **row_pointers;
    row_pointers = (png_byte **) malloc(height * sizeof(png_bytep));
    for (int i = 0; i < height; i++)
    {
        row_pointers[i] = (png_byte *) (pixels + (height - 1 - i) * pitch);
    }
    // set the row pointers
    png_set_rows(png_write, png_info, row_pointers);
    // write the picture to disk
    png_write_png(png_write, png_info, 0, NULL);
    // free memory
    free(row_pointers);
    png_destroy_write_struct(&png_write, &png_info);
    free(pixels);
    // all done
}

int OGL_CheckError()
{
    GLenum e = glGetError();
    if (e != GL_NO_ERROR)
    {
        printf("GL Error: ");
        switch(e)
        {
            case GL_INVALID_ENUM:   printf("INVALID ENUM"); break;
            case GL_INVALID_VALUE:  printf("INVALID VALUE"); break;
            case GL_INVALID_OPERATION:  printf("INVALID OPERATION"); break;
            case GL_OUT_OF_MEMORY:  printf("OUT OF MEMORY"); break;
        }
        printf("\n");
        return 1;
    }
    return 0;
}

void
OGL_SwapBuffers()
{
   if (OGL.logFrameRate){
        static int frames[5] = { 0, 0, 0, 0, 0 };
        static int framesIndex = 0;
        static Uint32 lastTicks = 0;
        Uint32 ticks = SDL_GetTicks();
        frames[framesIndex]++;
        if (ticks >= (lastTicks + 1000))
        {
            float fps = 0.0f;
            for (int i = 0; i < 5; i++) fps += frames[i];
            fps /= 5.0f;
            printf("fps = %f \n", fps);
#ifdef BATCH_TEST
            printf("average draw calls per frame = %f\n", (float)TotalDrawCalls / frames[framesIndex]);
            printf("average vertices per draw call = %f\n", (float)TotalTriangles / TotalDrawCalls);
            TotalDrawCalls = 0;
            TotalTriangles = 0;
#endif
            framesIndex = (framesIndex + 1) % 5;
            frames[framesIndex] = 0;
            lastTicks = ticks;
        }
    }

#ifdef PROFILE_GBI
    Uint32 profileTicks = SDL_GetTicks();
    static u32 profileLastTicks = 0;
    if (profileTicks >= (profileLastTicks + 10000))
    {
        printf("GBI PROFILE DATA: %i ms \n", profileTicks - profileLastTicks);
        printf("=========================================================\n");
        GBI_ProfilePrint(stdout);
        printf("=========================================================\n");
        GBI_ProfileReset();
        profileLastTicks = profileTicks;
    }
#endif

    // if emulator defined a render callback function, call it before buffer swap
    if (renderCallback) (*renderCallback)();

    eglSwapBuffers(Display, Surface);

}


void OGL_ReadScreen( void *dest, int *width, int *height )
{

    int w = OGL.width - OGL.xpos;
    int h = OGL.height - OGL.ypos;
    void *rgba = malloc( w * h * 4 );
    dest = malloc(w * h * 3);
    if (rgba == 0 || dest == 0)
        return;

    *width = w;
    *height = h;

    glReadPixels( OGL.xpos, OGL.ypos, OGL.width, OGL.height, GL_RGBA, GL_UNSIGNED_BYTE, rgba);

    //convert to RGB format:
    char *c = (char*) rgba;
    char *d = (char*) dest;
    for(int i = 0; i < (w * h); i++)
    {
        d[0] = c[0];
        d[1] = c[1];
        d[2] = c[2];
        d += 3;
        c += 4;
    }

    free(rgba);
}

