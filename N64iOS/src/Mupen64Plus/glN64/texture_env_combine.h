#ifndef TEXTURE_ENV_COMBINE_H
#define TEXTURE_ENV_COMBINE_H

/*
**  ATIX EXTENSIONS
*/

/*
** GL_ATIX_texture_env_route
**
** Support
**   Rage 128         based : Not Supported
**   Radeon 7xxx      based : Supported
**   Radeon 8xxx/9000 based : Supported
**   Radeon 9500/9700 based : Supported
*/
#ifndef GL_ATIX_texture_env_route
#define GL_ATIX_texture_env_route 1

#define GL_SECONDARY_COLOR_ATIX                 0x8747
#define GL_TEXTURE_OUTPUT_RGB_ATIX              0x8748
#define GL_TEXTURE_OUTPUT_ALPHA_ATIX            0x8749

#endif /* GL_ATIX_texture_env_route */

/*
** GL_ATIX_vertex_shader_output_point_size
**
**  Support:
**   Rage 128         based : Not Supported
**   Radeon 7xxx      based : Not Supported
**   Radeon 8xxx/9000 based : Supported
**   Radeon 9500/9700 based : Supported
*/
#ifndef GL_ATIX_vertex_shader_output_point_size
#define GL_ATIX_vertex_shader_output_point_size 1

#define GL_OUTPUT_POINT_SIZE_ATIX       0x610E

#endif /* GL_ATIX_vertex_shader_output_point_size */

struct TexEnvCombinerArg
{
    GLenum source, operand;
};

struct TexEnvCombinerStage
{
    WORD constant;
    BOOL used;
    GLenum combine;
    TexEnvCombinerArg arg0, arg1, arg2;
    WORD outputTexture;
};

struct TexEnvCombiner
{
    BOOL usesT0, usesT1, usesNoise;

    WORD usedUnits;
    
    struct
    {
        WORD color, secondaryColor, alpha;
    } vertex;

    TexEnvCombinerStage color[8];
    TexEnvCombinerStage alpha[8];
};

void Init_texture_env_combine();
TexEnvCombiner *Compile_texture_env_combine( Combiner *color, Combiner *alpha );
void Set_texture_env_combine( TexEnvCombiner *envCombiner );
void Update_texture_env_combine_Colors( TexEnvCombiner* );
void Uninit_texture_env_combine();
void BeginTextureUpdate_texture_env_combine();
void EndTextureUpdate_texture_env_combine();
#endif

