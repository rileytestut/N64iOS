
#include <stdlib.h>
#include "winlnxdefs.h"
#include "OpenGL.h"
#include "ShaderCombiner.h"

#ifndef max
#define max(a,b) ((a) > (b) ? (a) : (b))
#endif

#ifdef GLN64_MUX_DECODE
int saRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        ONE,                NOISE,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO
};

int sbRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        CENTER,             K4,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO
};

int mRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        SCALE,              COMBINED_ALPHA,
    TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,    SHADE_ALPHA,
    ENV_ALPHA,          LOD_FRACTION,       PRIM_LOD_FRAC,      K5,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO,
    ZERO,               ZERO,               ZERO,               ZERO
};

int aRGBExpanded[] =
{
    COMBINED,           TEXEL0,             TEXEL1,             PRIMITIVE,
    SHADE,              ENVIRONMENT,        ONE,                ZERO
};

int saAExpanded[] =
{
    COMBINED,           TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          ONE,                ZERO
};

int sbAExpanded[] =
{
    COMBINED,           TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          ONE,                ZERO
};

int mAExpanded[] =
{
    LOD_FRACTION,       TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          PRIM_LOD_FRAC,      ZERO,
};

int aAExpanded[] =
{
    COMBINED,           TEXEL0_ALPHA,       TEXEL1_ALPHA,       PRIMITIVE_ALPHA,
    SHADE_ALPHA,        ENV_ALPHA,          ONE,                ZERO
};
#else
static const int sc_Mux32[32] =
{
    COMBINED, TEXEL0, TEXEL1, PRIMITIVE,
    SHADE, ENVIRONMENT, ONE, COMBINED_ALPHA,
    TEXEL0_ALPHA, TEXEL1_ALPHA, PRIMITIVE_ALPHA, SHADE_ALPHA,
    ENV_ALPHA, LOD_FRACTION, PRIM_LOD_FRAC, K5,
    UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
    UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
    UNKNOWN, UNKNOWN, UNKNOWN, UNKNOWN,
    UNKNOWN, UNKNOWN, UNKNOWN, ZERO
};

static const int sc_Mux16[16] =
{
    COMBINED, TEXEL0, TEXEL1, PRIMITIVE,
    SHADE, ENVIRONMENT, ONE, COMBINED_ALPHA,
    TEXEL0_ALPHA, TEXEL1_ALPHA, PRIMITIVE_ALPHA, SHADE_ALPHA,
    ENV_ALPHA, LOD_FRACTION, PRIM_LOD_FRAC, ZERO
};
static const int sc_Mux8[8] =
{
    COMBINED, TEXEL0, TEXEL1, PRIMITIVE,
    SHADE, ENVIRONMENT, ONE, ZERO
};
#endif


int CCEncodeA[] = {0, 1, 2, 3, 4, 5, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 7, 15, 15, 6, 15 };
int CCEncodeB[] = {0, 1, 2, 3, 4, 5, 6, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 7, 15, 15, 15 };
int CCEncodeC[] = {0, 1, 2, 3, 4, 5, 31, 6, 7, 8, 9, 10, 11, 12, 13, 14, 31, 31, 15, 31, 31};
int CCEncodeD[] = {0, 1, 2, 3, 4, 5, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 15, 6, 15};
DWORD64 ACEncodeA[] = {7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7, 6, 7};
DWORD64 ACEncodeB[] = {7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7, 6, 7};
DWORD64 ACEncodeC[] = {7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 6, 7, 7, 7, 7, 7};
DWORD64 ACEncodeD[] = {7, 7, 7, 7, 7, 7, 7, 7, 0, 1, 2, 3, 4, 5, 7, 7, 7, 7, 7, 6, 7};

ShaderProgram *scProgramRoot = NULL;
ShaderProgram *scProgramCurrent = NULL;
int scProgramChanged = 0;
int scProgramCount = 0;

GLint _vertex_shader = 0;

const char *_frag_header = "                                \n"\
"uniform sampler2D uTex0;                                   \n"\
"uniform sampler2D uTex1;                                   \n"\
"uniform lowp vec4 uEnvColor;                               \n"\
"uniform lowp vec4 uPrimColor;                              \n"\
"uniform lowp vec4 uFogColor;                               \n"\
"uniform highp float uAlphaRef;                             \n"\
"uniform lowp float uPrimLODFrac;                           \n"\
"                                                           \n"\
"varying highp float vFactor;                               \n"\
"varying lowp vec4 vShadeColor;                             \n"\
"varying highp vec2 vTexCoord0;                             \n"\
"varying highp vec2 vTexCoord1;                             \n"\
"                                                           \n"\
"void main()                                                \n"\
"{                                                          \n"\
"lowp vec4 lFragColor;                                      \n";


const char *_vert = "                                       \n"\
"attribute highp vec4 	aPosition;                          \n"\
"attribute lowp vec4 	aColor;                             \n"\
"attribute highp vec2 aTexCoord0;                           \n"\
"attribute highp vec2 aTexCoord1;                           \n"\
"                                                           \n"\
"uniform bool		    uEnableFog;                         \n"\
"uniform float			uFogMultiplier, uFogOffset;         \n"\
"uniform bool 			uEnablePrimitiveZ;                  \n"\
"uniform bool 			uTriangleRS;                        \n"\
"uniform float 			uPrimitiveZ;                        \n"\
"                                                           \n"\
"uniform mediump vec2 	uTexScale;                          \n"\
"uniform mediump vec2 	uTexOffset[2];                      \n"\
"uniform mediump vec2 	uCacheShiftScale[2];                \n"\
"uniform mediump vec2 	uCacheScale[2];                     \n"\
"uniform mediump vec2 	uCacheOffset[2];                    \n"\
"                                                           \n"\
"varying highp float    vFactor;                            \n"\
"varying lowp vec4 		vShadeColor;                        \n"\
"varying highp vec2 	vTexCoord0;                         \n"\
"varying highp vec2 	vTexCoord1;                         \n"\
"                                                           \n"\
"void main()                                                \n"\
"{                                                          \n"\
"	gl_Position = aPosition;                                \n"\
"	vShadeColor = aColor;                                   \n"\
"	if (uEnablePrimitiveZ)                                  \n"\
"	{                                                       \n"\
"		gl_Position.z = uPrimitiveZ * aPosition.w;          \n"\
"	}                                                       \n"\
"                                                           \n"\
"	if (uEnableFog)                                         \n"\
"	{                                                       \n"\
"       highp float lFogCoord;                              \n"\
"		lFogCoord = max(-1.0, aPosition.z / aPosition.w)    \n"\
"           * uFogMultiplier + uFogOffset;                  \n"\
"		vFactor = 1.0 - lFogCoord * (1.0 / 255.0);          \n"\
"		vFactor = clamp(vFactor, 0.0, 1.0);                 \n"\
"	}                                                       \n"\
"                                                           \n"\
"	if (uTriangleRS)                                        \n"\
"	{                                                       \n"\
"		vTexCoord0 = (aTexCoord0 * (uTexScale[0] *          \n"\
"           uCacheShiftScale[0]) + (uCacheOffset[0] -       \n"\
"           uTexOffset[0])) * uCacheScale[0];               \n"\
"		vTexCoord1 = (aTexCoord0 * (uTexScale[1] *          \n"\
"           uCacheShiftScale[1]) + (uCacheOffset[1] -       \n"\
"           uTexOffset[1])) * uCacheScale[1];               \n"\
"	}                                                       \n"\
"	else                                                    \n"\
"	{                                                       \n"\
"		vTexCoord0 = aTexCoord0;                            \n"\
"		vTexCoord1 = aTexCoord1;                            \n"\
"	}                                                       \n"\
"                                                           \n"\
"}                                                          \n\n";

const char * _color_param_str(int param)
{
    switch(param)
    {
        case COMBINED:          return "lFragColor.rgb";
        case TEXEL0:            return "lTex0.rgb";
        case TEXEL1:            return "lTex1.rgb";
        case PRIMITIVE:         return "uPrimColor.rgb";
        case SHADE:             return "vShadeColor.rgb";
        case ENVIRONMENT:       return "uEnvColor.rgb";
        case CENTER:            return "uEnvColor.rgb";
        case SCALE:             return "uEnvColor.rgb";
        case COMBINED_ALPHA:    return "vec3(lFragColor.a)";
        case TEXEL0_ALPHA:      return "vec3(lTex0.a)";
        case TEXEL1_ALPHA:      return "vec3(lTex1.a)";
        case PRIMITIVE_ALPHA:   return "vec3(uPrimColor.a)";
        case SHADE_ALPHA:       return "vec3(vShadeColor.a)";
        case ENV_ALPHA:         return "vec3(uEnvColor.a)";
        case LOD_FRACTION:      return "vec3(0.0)";
        case PRIM_LOD_FRAC:     return "vec3(uPrimLODFrac)";
        case NOISE:             return "vec3(1.0)";
        case K4:                return "vec3(1.0)";
        case K5:                return "vec3(1.0)";
        case ONE:               return "vec3(1.0)";
        case ZERO:              return "vec3(0.0)";
        default:
            printf("UNKNOWN COMBINER MODE\n");
            return "vec3(0.0)";
    }
}

const char * _alpha_param_str(int param)
{
    switch(param)
    {
        case COMBINED:          return "lFragColor.a";
        case TEXEL0:            return "lTex0.a";
        case TEXEL1:            return "lTex1.a";
        case PRIMITIVE:         return "uPrimColor.a";
        case SHADE:             return "vShadeColor.a";
        case ENVIRONMENT:       return "uEnvColor.a";
        case CENTER:            return "uEnvColor.a";
        case SCALE:             return "uEnvColor.a";
        case COMBINED_ALPHA:    return "lFragColor.a";
        case TEXEL0_ALPHA:      return "lTex0.a";
        case TEXEL1_ALPHA:      return "lTex1.a";
        case PRIMITIVE_ALPHA:   return "uPrimColor.a";
        case SHADE_ALPHA:       return "vShadeColor.a";
        case ENV_ALPHA:         return "uEnvColor.a";
        case LOD_FRACTION:      return "0.0";
        case PRIM_LOD_FRAC:     return "uPrimLODFrac";
        case NOISE:             return "0.0";
        case K4:                return "1.0";
        case K5:                return "1.0";
        case ONE:               return "1.0";
        case ZERO:              return "0.0";
        default:
            printf("UNKNOWN COMBINER MODE\n");
            return "0.0";
    }
}

int _program_compare(ShaderProgram *prog, u64 mux, u32 flags)
{
    if (prog)
        return ((prog->combine.mux == mux) && (prog->flags == flags));
    else
        return 1;
}

void _glcompiler_error(GLint shader)
{
    int len, i;
    char* log;

    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &len);
    log = (char*) malloc(len + 1);
    glGetShaderInfoLog(shader, len, &i, log);
    log[len] = 0;
    printf("COMPILE ERROR: %s \n", log);
    free(log);
}

void _gllinker_error(GLint program)
{
    int len, i;
    char* log;

    glGetProgramiv(program, GL_INFO_LOG_LENGTH, &len);
    log = (char*) malloc(len + 1);
    glGetProgramInfoLog(program, len, &i, log);
    log[len] = 0;
    printf("LINK ERROR: %s \n", log);
    free(log);
};

void _locate_attributes(ShaderProgram *p)
{
    glBindAttribLocation(p->program, SC_POSITION,   "aPosition");
    glBindAttribLocation(p->program, SC_COLOR,      "aColor");
    glBindAttribLocation(p->program, SC_TEXCOORD0,  "aTexCoord0");
    glBindAttribLocation(p->program, SC_TEXCOORD1,  "aTexCoord1");
};

#define LocateUniform(A) \
    p->uniforms.A.loc = glGetUniformLocation(p->program, #A);

void _locate_uniforms(ShaderProgram *p)
{
    LocateUniform(uTex0);
    LocateUniform(uTex1);
    LocateUniform(uEnvColor);
    LocateUniform(uPrimColor);
    LocateUniform(uPrimLODFrac);
    LocateUniform(uFogColor);
    LocateUniform(uEnableFog);
    LocateUniform(uEnablePrimitiveZ);
    LocateUniform(uTriangleRS);
    LocateUniform(uFogMultiplier);
    LocateUniform(uFogOffset);
    LocateUniform(uPrimitiveZ);
    LocateUniform(uAlphaRef);
    LocateUniform(uTexScale);
    LocateUniform(uTexOffset[0]);
    LocateUniform(uTexOffset[1]);
    LocateUniform(uCacheShiftScale[0]);
    LocateUniform(uCacheShiftScale[1]);
    LocateUniform(uCacheScale[0]);
    LocateUniform(uCacheScale[1]);
    LocateUniform(uCacheOffset[0]);
    LocateUniform(uCacheOffset[1]);
}

void _force_uniforms()
{
    SC_ForceUniform1i(uTex0, 0);
    SC_ForceUniform1i(uTex1, 1);
    SC_ForceUniform4fv(uEnvColor, &gDP.envColor.r);
    SC_ForceUniform4fv(uPrimColor, &gDP.primColor.r);
    SC_ForceUniform1f(uPrimLODFrac, gDP.primColor.l);
    SC_ForceUniform4fv(uFogColor, &gDP.fogColor.r);
    SC_ForceUniform1i(uEnableFog, (OGL.enableFog && (gSP.geometryMode & G_FOG)));
    SC_ForceUniform1i(uEnablePrimitiveZ, (gDP.otherMode.depthSource == G_ZS_PRIM));
    SC_ForceUniform1i(uTriangleRS, (OGL.renderState == RS_TRIANGLE));
    SC_ForceUniform1f(uFogMultiplier, gSP.fog.multiplier);
    SC_ForceUniform1f(uFogOffset, gSP.fog.offset);
    SC_ForceUniform1f(uAlphaRef, (gDP.otherMode.cvgXAlpha) ? 0.5 : gDP.blendColor.a);
    SC_ForceUniform1f(uPrimitiveZ, gDP.primDepth.z);
    SC_ForceUniform2f(uTexScale, gSP.texture.scales, gSP.texture.scalet);

    if (gSP.textureTile[0]){
        SC_ForceUniform2f(uTexOffset[0], gSP.textureTile[0]->fuls, gSP.textureTile[0]->fult);
    } else {
        SC_ForceUniform2f(uTexOffset[0], 0.0f, 0.0f);
    }

    if (gSP.textureTile[1])
    {
        SC_ForceUniform2f(uTexOffset[1], gSP.textureTile[1]->fuls, gSP.textureTile[1]->fult);
    }
    else
    {
        SC_ForceUniform2f(uTexOffset[1], 0.0f, 0.0f);
    }

    if (cache.current[0])
    {
        SC_ForceUniform2f(uCacheShiftScale[0], cache.current[0]->shiftScaleS, cache.current[0]->shiftScaleT);
        SC_ForceUniform2f(uCacheScale[0], cache.current[0]->scaleS, cache.current[0]->scaleT);
        SC_ForceUniform2f(uCacheOffset[0], cache.current[0]->offsetS, cache.current[0]->offsetT);
    }
    else
    {
        SC_ForceUniform2f(uCacheShiftScale[0], 1.0f, 1.0f);
        SC_ForceUniform2f(uCacheScale[0], 1.0f, 1.0f);
        SC_ForceUniform2f(uCacheOffset[0], 0.0f, 0.0f);
    }

    if (cache.current[1])
    {
        SC_ForceUniform2f(uCacheShiftScale[1], cache.current[1]->shiftScaleS, cache.current[1]->shiftScaleT);
        SC_ForceUniform2f(uCacheScale[1], cache.current[1]->scaleS, cache.current[1]->scaleT);
        SC_ForceUniform2f(uCacheOffset[1], cache.current[1]->offsetS, cache.current[1]->offsetT);
    }
    else
    {
        SC_ForceUniform2f(uCacheShiftScale[1], 1.0f, 1.0f);
        SC_ForceUniform2f(uCacheScale[1], 1.0f, 1.0f);
        SC_ForceUniform2f(uCacheOffset[1], 0.0f, 0.0f);
    }

}

void _update_uniforms()
{
    SC_SetUniform4fv(uEnvColor, &gDP.envColor.r);
    SC_SetUniform4fv(uPrimColor, &gDP.primColor.r);
    SC_SetUniform1f(uPrimLODFrac, gDP.primColor.l);
    SC_SetUniform4fv(uFogColor, &gDP.fogColor.r);
    SC_SetUniform1i(uEnableFog, (OGL.enableFog && (gSP.geometryMode & G_FOG)));
    SC_SetUniform1i(uEnablePrimitiveZ, (gDP.otherMode.depthSource == G_ZS_PRIM));
    SC_SetUniform1i(uTriangleRS, (OGL.renderState == RS_TRIANGLE));
    SC_SetUniform1f(uFogMultiplier, gSP.fog.multiplier);
    SC_SetUniform1f(uFogOffset, gSP.fog.offset);
    SC_SetUniform1f(uAlphaRef, (gDP.otherMode.cvgXAlpha) ? 0.5 : gDP.blendColor.a);
    SC_SetUniform1f(uPrimitiveZ, gDP.primDepth.z);

    //for some reason i must force these...
    SC_ForceUniform2f(uTexScale, gSP.texture.scales, gSP.texture.scalet);
    if (scProgramCurrent->usesT0)
    {
        if (gSP.textureTile[0])
        {
            SC_ForceUniform2f(uTexOffset[0], gSP.textureTile[0]->fuls, gSP.textureTile[0]->fult);
        }
        if (cache.current[0])
        {
            SC_ForceUniform2f(uCacheShiftScale[0], cache.current[0]->shiftScaleS, cache.current[0]->shiftScaleT);
            SC_ForceUniform2f(uCacheScale[0], cache.current[0]->scaleS, cache.current[0]->scaleT);
            SC_ForceUniform2f(uCacheOffset[0], cache.current[0]->offsetS, cache.current[0]->offsetT);
        }
    }

    if (scProgramCurrent->usesT1)
    {
        if (gSP.textureTile[1])
        {
            SC_ForceUniform2f(uTexOffset[1], gSP.textureTile[1]->fuls, gSP.textureTile[1]->fult);
        }
        if (cache.current[1])
        {
            SC_ForceUniform2f(uCacheShiftScale[1], cache.current[1]->shiftScaleS, cache.current[1]->shiftScaleT);
            SC_ForceUniform2f(uCacheScale[1], cache.current[1]->scaleS, cache.current[1]->scaleT);
            SC_ForceUniform2f(uCacheOffset[1], cache.current[1]->offsetS, cache.current[1]->offsetT);
        }
    }
};

void ShaderCombiner_Init()
{
    //compile vertex shader:
    GLint success;
    const char *src[1];
    src[0] = _vert;
    _vertex_shader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(_vertex_shader, 1, (const char**) src, NULL);
    glCompileShader(_vertex_shader);
    glGetShaderiv(_vertex_shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        _glcompiler_error(_vertex_shader);
    }
};

void ShaderCombiner_DeleteProgram(ShaderProgram *prog)
{
    if (prog)
    {
        ShaderCombiner_DeleteProgram(prog->left);
        ShaderCombiner_DeleteProgram(prog->right);
        glDeleteProgram(prog->program);
        glDeleteShader(prog->fragment);
        free(prog);
        scProgramCount--;
    }
}

void ShaderCombiner_Destroy()
{
    ShaderCombiner_DeleteProgram(scProgramRoot);
    glDeleteShader(_vertex_shader);
    scProgramCount = scProgramChanged = 0;
    scProgramRoot = scProgramCurrent = NULL;
}

void ShaderCombiner_Set(u64 mux)
{
    //banjo tooie hack
    if ((gDP.otherMode.cycleType == G_CYC_1CYCLE) && (mux == 0x00ffe7ffffcf9fcfLL))
    {
        mux = EncodeCombineMode( 0, 0, 0, 0, TEXEL0, 0, PRIMITIVE, 0,
                                 0, 0, 0, 0, TEXEL0, 0, PRIMITIVE, 0 );
    }

    //determine flags
    int flags = 0;
    if ((OGL.enableFog) &&(gSP.geometryMode & G_FOG))
        flags |= SC_FOGENABLED;

    if ((OGL.enableAlphaTest) &&
        (((gDP.otherMode.alphaCompare == G_AC_THRESHOLD) && !(gDP.otherMode.alphaCvgSel)) ||
         gDP.otherMode.cvgXAlpha))
    {
        flags |= SC_ALPHAENABLED;
        if (gDP.otherMode.cvgXAlpha || gDP.blendColor.a > 0.0f)
            flags |= SC_ALPHAGREATER;
    }
    if (gDP.otherMode.cycleType == G_CYC_2CYCLE)
        flags |= SC_2CYCLE;

    //if already bound:
    if (scProgramCurrent && _program_compare(scProgramCurrent, mux, flags))
    {
        scProgramChanged = 0;
        return;
    }

    //traverse binary tree for cached programs
    scProgramChanged = 1;
    ShaderProgram *root = scProgramRoot;
    ShaderProgram *prog = root;
    while(!_program_compare(prog, mux, flags))
    {
        root = prog;
        if (prog->combine.mux < mux)
            prog = prog->right;
        else
            prog = prog->left;
    }

    //build new program
    if (!prog)
    {
        scProgramCount++;
        prog = ShaderCombiner_Compile(mux, flags);
        if (!root)
            scProgramRoot = prog;
        else if (root->combine.mux < mux)
            root->right = prog;
        else
            root->left = prog;

    }
    else
    {
        scProgramCurrent = prog;
        glUseProgram(prog->program);
        _update_uniforms();
    }
}

ShaderProgram *ShaderCombiner_Compile(u64 mux, int flags)
{
    GLint success;
    char frag[4096];
    char *buffer = frag;
    ShaderProgram *prog = (ShaderProgram*) malloc(sizeof(ShaderProgram));

    prog->left = prog->right = NULL;
    prog->usesT0 = prog->usesT1 = prog->usesCol = 0;
    prog->combine.mux = mux;
    prog->flags = flags;
    prog->vertex = _vertex_shader;

    // Decode and expand the combine mode into a more general form
    int comb[4][4];

#ifdef GLN64_MUX_DECODE
    comb[0][0] = saRGBExpanded[prog->combine.saRGB0];
    comb[0][1] = sbRGBExpanded[prog->combine.sbRGB0];
    comb[0][2] = mRGBExpanded[prog->combine.mRGB0];
    comb[0][3] = aRGBExpanded[prog->combine.aRGB0];
    comb[1][0] = saAExpanded[prog->combine.saA0];
    comb[1][1] = sbAExpanded[prog->combine.sbA0];
    comb[1][2]  = mAExpanded[prog->combine.mA0];
    comb[1][3]  = aAExpanded[prog->combine.aA0];
    comb[2][0] = saRGBExpanded[prog->combine.saRGB1];
    comb[2][1] = sbRGBExpanded[prog->combine.sbRGB1];
    comb[2][2]  = mRGBExpanded[prog->combine.mRGB1];
    comb[2][3]  = aRGBExpanded[prog->combine.aRGB1];
    comb[3][0] = saAExpanded[prog->combine.saA1];
    comb[3][1] = sbAExpanded[prog->combine.sbA1];
    comb[3][2]  = mAExpanded[prog->combine.mA1];
    comb[3][3]  = aAExpanded[prog->combine.aA1];
#else
    comb[0][0] = sc_Mux16[(prog->combine.muxs0>>20)&0x0F];
    comb[0][1] = sc_Mux16[(prog->combine.muxs1>>28)&0x0F];
    comb[0][2] = sc_Mux32[(prog->combine.muxs0>>15)&0x1F];
    comb[0][3] = sc_Mux8[(prog->combine.muxs1>>15)&0x07];
    comb[1][0] = sc_Mux8[(prog->combine.muxs0>>12)&0x07];
    comb[1][1] = sc_Mux8[(prog->combine.muxs1>>12)&0x07];
    comb[1][2] = sc_Mux8[(prog->combine.muxs0>>9)&0x07];
    comb[1][3] = sc_Mux8[(prog->combine.muxs1>>9)&0x07];
    comb[2][0] = sc_Mux16[(prog->combine.muxs0>>5)&0x0F];
    comb[2][1] = sc_Mux16[(prog->combine.muxs1>>24)&0x0F];
    comb[2][2] = sc_Mux32[(prog->combine.muxs0)&0x1F];
    comb[2][3] = sc_Mux8[(prog->combine.muxs1>>6)&0x07];
    comb[3][0] = sc_Mux8[(prog->combine.muxs1>>21)&0x07];
    comb[3][1] = sc_Mux8[(prog->combine.muxs1>>3)&0x07];
    comb[3][2] = sc_Mux8[(prog->combine.muxs1>>18)&0x07];
    comb[3][3] = sc_Mux8[(prog->combine.muxs1)&0x07];
#endif
    for(int i=0; i < ((flags & SC_2CYCLE) ? 4 : 2); i++)
    {
        for(int j=0;j<4;j++)
        {
            prog->usesT0 |= (comb[i][j] == TEXEL0 || comb[i][j] == TEXEL0_ALPHA);
            prog->usesT1 |= (comb[i][j] == TEXEL1 || comb[i][j] == TEXEL1_ALPHA);
            prog->usesCol |= (comb[i][j] == SHADE || comb[i][j] == SHADE_ALPHA);
        }
    }

    buffer += sprintf(buffer, "%s", _frag_header);
    if (prog->usesT0)
        buffer += sprintf(buffer, "lowp vec4 lTex0 = texture2D(uTex0, vTexCoord0); \n");
    if (prog->usesT1)
        buffer += sprintf(buffer, "lowp vec4 lTex1 = texture2D(uTex1, vTexCoord1); \n");

    for(int i = 0; i < ((flags & SC_2CYCLE) ? 2 : 1); i++)
    {
        if (OGL.accurateRDP)
        {

            buffer += sprintf(buffer, "lFragColor.rgb = (%s - %s) * %s; \n",
                _color_param_str(comb[i*2][0]),
                _color_param_str(comb[i*2][1]),
                _color_param_str(comb[i*2][2])
                );
            buffer += sprintf(buffer, "lFragColor.a = (%s - %s) * %s; \n",
                _alpha_param_str(comb[i*2+1][0]),
                _alpha_param_str(comb[i*2+1][1]),
                _alpha_param_str(comb[i*2+1][2])
                );

            buffer += sprintf(buffer, "if (lFragColor.r < 0.0) {lFragColor.r = (lFragColor.r < -0.5) ? 0.0 : 1.0;}\n");
            buffer += sprintf(buffer, "if (lFragColor.g < 0.0) {lFragColor.g = (lFragColor.g < -0.5) ? 0.0 : 1.0;}\n");
            buffer += sprintf(buffer, "if (lFragColor.b < 0.0) {lFragColor.b = (lFragColor.b < -0.5) ? 0.0 : 1.0;}\n");
            buffer += sprintf(buffer, "if (lFragColor.a < 0.0) {lFragColor.a = (lFragColor.a < -0.5) ? 0.0 : 1.0;}\n");

            buffer += sprintf(buffer, "lFragColor.rgb += %s; \n", _color_param_str(comb[i*2][3]));
            buffer += sprintf(buffer, "lFragColor.a += %s; \n", _alpha_param_str(comb[i*2+1][3]));
        }
        else
        {
            buffer += sprintf(buffer, "lFragColor.rgb = (%s - %s) * %s + %s; \n",
                _color_param_str(comb[i*2][0]),
                _color_param_str(comb[i*2][1]),
                _color_param_str(comb[i*2][2]),
                _color_param_str(comb[i*2][3])
            );
            buffer += sprintf(buffer, "lFragColor.a = (%s - %s) * %s + %s; \n",
                _alpha_param_str(comb[i*2+1][0]),
                _alpha_param_str(comb[i*2+1][1]),
                _alpha_param_str(comb[i*2+1][2]),
                _alpha_param_str(comb[i*2+1][3])
                );
        }
    };

    //fog
    if (flags&SC_FOGENABLED)
        buffer += sprintf(buffer, "lFragColor = mix(uFogColor, lFragColor, vFactor); \n");

    //alpha function
    if (flags&SC_ALPHAENABLED)
    {
        if (flags&SC_ALPHAGREATER)
            buffer += sprintf(buffer, "if (lFragColor.a < uAlphaRef) discard; \n");
        else
            buffer += sprintf(buffer, "if (lFragColor.a <= uAlphaRef) discard; \n");
    }
    buffer += sprintf(buffer, "gl_FragColor = lFragColor; \n");
    buffer += sprintf(buffer, "} \n\n");
    *buffer = 0;

#ifdef PRINT_SHADER
    printf("=============================================================\n");
    printf("Num=%i \t usesT0=%i usesT1=%i usesCol=%i \n", scProgramCount, prog->usesT0, prog->usesT1, prog->usesCol);
    printf("=============================================================\n");
    printf("%s", frag);
    printf("=============================================================\n");
#endif

    prog->program = glCreateProgram();

    //Compile:
    char *src[1];
    src[0] = frag;
    GLint len[1];
    len[0] = min(4096, strlen(frag));
    prog->fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(prog->fragment, 1, (const char**) src, len);
    glCompileShader(prog->fragment);
    glGetShaderiv(prog->fragment, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        _glcompiler_error(prog->fragment);
    }

    //link
    _locate_attributes(prog);
    glAttachShader(prog->program, prog->fragment);
    glAttachShader(prog->program, prog->vertex);
    glLinkProgram(prog->program);
    glGetProgramiv(prog->program, GL_LINK_STATUS, &success);
    if (!success)
    {
        _gllinker_error(prog->program);
    }
    _locate_uniforms(prog);

    scProgramCurrent = prog;
    glUseProgram(prog->program);
    _force_uniforms();

    return prog;
}

