#define GL_GLEXT_PROTOTYPES
#include <SDL_opengl.h>

/* NVidia extensions */
extern void APIENTRY glCombinerParameterfvNV (GLenum, const GLfloat *);
extern void APIENTRY glCombinerParameterfNV (GLenum, GLfloat);
extern void APIENTRY glCombinerParameterivNV (GLenum, const GLint *);
extern void APIENTRY glCombinerParameteriNV (GLenum, GLint);
extern void APIENTRY glCombinerInputNV (GLenum, GLenum, GLenum, GLenum, GLenum, GLenum);
extern void APIENTRY glCombinerOutputNV (GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLenum, GLboolean, GLboolean, GLboolean);
extern void APIENTRY glFinalCombinerInputNV (GLenum, GLenum, GLenum, GLenum);
extern void APIENTRY glGetCombinerInputParameterfvNV (GLenum, GLenum, GLenum, GLenum, GLfloat *);
extern void APIENTRY glGetCombinerInputParameterivNV (GLenum, GLenum, GLenum, GLenum, GLint *);
extern void APIENTRY glGetCombinerOutputParameterfvNV (GLenum, GLenum, GLenum, GLfloat *);
extern void APIENTRY glGetCombinerOutputParameterivNV (GLenum, GLenum, GLenum, GLint *);
extern void APIENTRY glGetFinalCombinerInputParameterfvNV (GLenum, GLenum, GLfloat *);
extern void APIENTRY glGetFinalCombinerInputParameterivNV (GLenum, GLenum, GLint *);

/* Forward declarations */
class Combiner;
class RegisterCombiners;

struct CombinerInput
{
    GLenum input;
    GLenum mapping;
    GLenum usage;
};

struct CombinerVariable
{
    GLenum input;
    GLenum mapping;
    GLenum usage;
    BOOL used;
};

struct GeneralCombiner
{
    CombinerVariable A, B, C, D;

    struct
    {
        GLenum ab;
        GLenum cd;
        GLenum sum;
    } output;
};

struct RegisterCombiners
{
    GeneralCombiner color[8];
    GeneralCombiner alpha[8];

    struct
    {
        CombinerVariable A, B, C, D, E, F, G;
    } final;

    struct 
    {
        WORD color, alpha;
    } constant[2];

    struct
    {
        WORD color, secondaryColor, alpha;
    } vertex;

    WORD numCombiners;
    BOOL usesT0, usesT1, usesNoise;
};

void Init_NV_register_combiners();
void Uninit_NV_register_combiners();
RegisterCombiners *Compile_NV_register_combiners( Combiner *color, Combiner *alpha );
void Update_NV_register_combiners_Colors( RegisterCombiners *regCombiners );
void Set_NV_register_combiners( RegisterCombiners *regCombiners );

