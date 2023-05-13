#include "r_local.h"

#include <SDL2/SDL.h>
#include <SDL2/SDL_opengl.h>

struct RendererImport ri;

trGlobals_t tr;

SDL_Window *window;
SDL_GLContext context;

GLuint program;

static void R_SetupGL() {
    struct matrix4 projection_matrix, model_matrix;
    int width, height;

    SDL_GetWindowSize(window, &width, &height);

    matrix4_identity(&model_matrix);

    matrix4_perspective(&projection_matrix, tr.refdef.fov, width / height, 100.0, 100000.0);
    matrix4_rotate(&projection_matrix, &tr.refdef.viewangles, ROTATE_XYZ);
    matrix4_translate(&projection_matrix, &tr.refdef.vieworg);

    glUniformMatrix4fv( glGetUniformLocation( program, "u_projection_matrix" ), 1, GL_FALSE, projection_matrix.v );
    glUniformMatrix4fv( glGetUniformLocation( program, "u_model_matrix" ), 1, GL_FALSE, model_matrix.v );
}

void R_RenderFrame(struct refdef const *refdef) {
    tr.refdef = *refdef;
    
    R_SetupGL();
    R_DrawWorld();
    R_DrawEntities();
    R_DrawAlphaSurfaces();
}

void R_Init(DWORD dwWidth, DWORD dwHeight) {
    SDL_Init( SDL_INIT_VIDEO );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );
    SDL_GL_SetAttribute( SDL_GL_ACCELERATED_VISUAL, 1 );
    SDL_GL_SetAttribute( SDL_GL_RED_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_GREEN_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_BLUE_SIZE, 8 );
    SDL_GL_SetAttribute( SDL_GL_ALPHA_SIZE, 8 );

    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MAJOR_VERSION, 3 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_MINOR_VERSION, 2 );
    SDL_GL_SetAttribute( SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE );

    window = SDL_CreateWindow( "", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dwWidth, dwHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN );
    context = SDL_GL_CreateContext( window );
    program = R_InitShader();
    
    tr.renbuf = R_MakeVertexArrayObject(NULL, 0);

    glDisable( GL_DEPTH_TEST );
    glClearColor( 0.0, 0.0, 0.0, 0.0 );
    glViewport( 0, 0, dwWidth, dwHeight );
    
//    Texture = R_LoadTexture("TerrainArt\\LordaeronSummer\\Lords_Dirt.blp");
    tr.waterTexture = R_LoadTexture("ReplaceableTextures\\Water\\Water12.blp");
        
//    Model = LoadModel(hArchive, "Doodads\\Terrain\\WoodBridgeLarge45\\WoodBridgeLarge45.mdx");
//    FOR_LOOP(i, Model->num_textures) {
//        printf("%s\n", Model->textures[i].path);
//    }
}


typedef float t_mat4x4[16];

static inline void mat4x4_ortho( t_mat4x4 out, float left, float right, float bottom, float top, float znear, float zfar )
{
    #define T(a, b) (a * 4 + b)

    out[T(0,0)] = 2.0f / (right - left);
    out[T(0,1)] = 0.0f;
    out[T(0,2)] = 0.0f;
    out[T(0,3)] = 0.0f;

    out[T(1,1)] = 2.0f / (top - bottom);
    out[T(1,0)] = 0.0f;
    out[T(1,2)] = 0.0f;
    out[T(1,3)] = 0.0f;

    out[T(2,2)] = -2.0f / (zfar - znear);
    out[T(2,0)] = 0.0f;
    out[T(2,1)] = 0.0f;
    out[T(2,3)] = 0.0f;

    out[T(3,0)] = -(right + left) / (right - left);
    out[T(3,1)] = -(top + bottom) / (top - bottom);
    out[T(3,2)] = -(zfar + znear) / (zfar - znear);
    out[T(3,3)] = 1.0f;

    #undef T
}

void R_DrawPic(struct tTexture const *lpTexture) {
//    const struct tVertex g_vertex_buffer_data[] = {
//    /*  R, G, B, A, X, Y, Z U, V  */
//        { 1, 1, 1, 1, 0, 0, 0, 0, 0 },
//        { 1, 1, 1, 1, width, 0, 0, 1, 0 },
//        { 1, 1, 1, 1, width, height, 0, 1, 1 },
//        { 1, 1, 1, 1, 0, 0, 0, 0, 0 },
//        { 1, 1, 1, 1, width, height, 0, 1, 1 },
//        { 1, 1, 1, 1, 0, height, 0, 0, 1 },
//    };
//    t_mat4x4 projection_matrix;
//    mat4x4_ortho( projection_matrix, 0.0f, (float)width, (float)height, 0.0f, 0.0f, 100.0f );
//    tr.renbuf = MakeVertexArrayObject(g_vertex_buffer_data, 6);
//        R_BindTexture(map.heightmap->shadowmap);
//        glEnable(GL_BLEND);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//        glBindVertexArray( tr.renbuf.vao );
//        glBindBuffer(GL_ARRAY_BUFFER, tr.renbuf.vbo );
//        glDrawArrays( GL_TRIANGLES, 0, 6 );
}

void R_BeginFrame(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );
}

void R_EndFrame(void) {
    SDL_GL_SwapWindow( window );
    SDL_Delay( 1 );
}

void R_Shutdown(void) {
    SDL_GL_DeleteContext( context );
    SDL_DestroyWindow( window );
    SDL_Quit();
}

struct Renderer *Renderer_Init(struct RendererImport *lpImport) {
    struct Renderer *lpRenderer = lpImport->MemAlloc(sizeof(struct Renderer));
    lpRenderer->Init = R_Init;
    lpRenderer->RegisterMap = R_RegisterMap;
    lpRenderer->LoadTexture = R_LoadTexture;
    lpRenderer->LoadModel = R_LoadModel;
    lpRenderer->RenderFrame = R_RenderFrame;
    lpRenderer->Init = R_Init;
    lpRenderer->BeginFrame = R_BeginFrame;
    lpRenderer->EndFrame = R_EndFrame;
    lpRenderer->DrawPic = R_DrawPic;
    ri = *lpImport;

    return lpRenderer;
}
