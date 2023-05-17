#include "r_local.h"

#include <SDL.h>
#include <SDL_opengl.h>

struct renderer_import ri;
struct render_globals tr;

SDL_Window *window;
SDL_GLContext context;

static void R_SetupGL(void) {
    struct matrix4 model_matrix;
    int width, height;
    
    SDL_GetWindowSize(window, &width, &height);
    
    Matrix4_identity(&model_matrix);
    
    Matrix4_perspective(&tr.viewDef.projection_matrix, tr.viewDef.fov, width / height, 100.0, 100000.0);
    Matrix4_rotate(&tr.viewDef.projection_matrix, &tr.viewDef.viewangles, ROTATE_XYZ);
    Matrix4_translate(&tr.viewDef.projection_matrix, &tr.viewDef.vieworg);
    
    glUseProgram(tr.shaderSkin->progid);
    
    glUniformMatrix4fv(tr.shaderSkin->u_projection_matrix, 1, GL_FALSE, tr.viewDef.projection_matrix.v);
    glUniformMatrix4fv(tr.shaderSkin->u_model_matrix, 1, GL_FALSE, model_matrix.v);
    
    glUseProgram(tr.shaderStatic->progid);
    
    glUniformMatrix4fv(tr.shaderStatic->u_projection_matrix, 1, GL_FALSE, tr.viewDef.projection_matrix.v);
    glUniformMatrix4fv(tr.shaderStatic->u_model_matrix, 1, GL_FALSE, model_matrix.v);
}

void R_RenderFrame(LPCVIEWDEF viewDef) {
    tr.viewDef = *viewDef;
    
    R_SetupGL();
    R_DrawWorld();
    R_DrawEntities();
    R_DrawAlphaSurfaces();
}

void R_Init(DWORD dwWidth, DWORD dwHeight) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);

    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, dwWidth, dwHeight, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    context = SDL_GL_CreateContext(window);

    extern LPCSTR vertex_shader_skin;
    extern LPCSTR vertex_shader;
    extern LPCSTR fragment_shader;
    
    tr.shaderStatic = R_InitShader(vertex_shader, fragment_shader);
    tr.shaderSkin = R_InitShader(vertex_shader_skin, fragment_shader);
    tr.renbuf = R_MakeVertexArrayObject(NULL, 0);

    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glViewport(0, 0, dwWidth, dwHeight);
    
//    Texture = R_LoadTexture("TerrainArt\\LordaeronSummer\\Lords_Dirt.blp");
    tr.waterTexture = R_LoadTexture("ReplaceableTextures\\Water\\Water12.blp");
        
//    Model = LoadModel(hArchive, "Doodads\\Terrain\\WoodBridgeLarge45\\WoodBridgeLarge45.mdx");
//    FOR_LOOP(i, Model->num_textures) {
//        printf("%s\n", Model->textures[i].path);
//    }
}

void R_DrawPic(LPCTEXTURE lpTexture) {
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
//    mat4x4_ortho(projection_matrix, 0.0f, (float)width, (float)height, 0.0f, 0.0f, 100.0f);
//    tr.renbuf = MakeVertexArrayObject(g_vertex_buffer_data, 6);
//        R_BindTexture(map.lpTerrain->shadowmap);
//        glEnable(GL_BLEND);
//        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
//        glBindVertexArray(tr.renbuf->vao);
//        glBindBuffer(GL_ARRAY_BUFFER, tr.renbuf->vbo);
//        glDrawArrays(GL_TRIANGLES, 0, 6);
}

LPBUFFER R_MakeVertexArrayObject(struct vertex const *data, DWORD size) {
    LPBUFFER buf = ri.MemAlloc(sizeof(struct render_buffer));
    
    glGenVertexArrays(1, &buf->vao);
    glGenBuffers(1, &buf->vbo);
    glBindVertexArray(buf->vao);
    glBindBuffer(GL_ARRAY_BUFFER, buf->vbo);

    glEnableVertexAttribArray(attrib_position);
    glEnableVertexAttribArray(attrib_color);
    glEnableVertexAttribArray(attrib_texcoord);
    glEnableVertexAttribArray(attrib_texcoord2);
    glEnableVertexAttribArray(attrib_skin);
    glEnableVertexAttribArray(attrib_boneWeight);
    
    glVertexAttribPointer(attrib_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, color));
    glVertexAttribPointer(attrib_position, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, position));
    glVertexAttribPointer(attrib_texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord));
    glVertexAttribPointer(attrib_texcoord2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord2));
    glVertexAttribPointer(attrib_skin, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vertex), FOFS(vertex, skin));
    glVertexAttribPointer(attrib_boneWeight, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, boneWeight));

    if (data) {
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(struct vertex), data, GL_STATIC_DRAW);
    }

    return buf;
}

void R_ReleaseVertexArrayObject(LPBUFFER lpBuffer) {
    glDeleteBuffers(1, &lpBuffer->vbo);
    glDeleteVertexArrays(1, &lpBuffer->vao);
}

void R_BeginFrame(void) {
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void R_EndFrame(void) {
    SDL_GL_SwapWindow(window);
    SDL_Delay(1);
}

void R_Shutdown(void) {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

struct Renderer *Renderer_Init(struct renderer_import *lpImport) {
    struct Renderer *lpRenderer = lpImport->MemAlloc(sizeof(struct Renderer));
    lpRenderer->Init = R_Init;
    lpRenderer->RegisterMap = R_RegisterMap;
    lpRenderer->LoadTexture = R_LoadTexture;
    lpRenderer->LoadModel = R_LoadModel;
    lpRenderer->ReleaseModel = R_ReleaseModel;
    lpRenderer->RenderFrame = R_RenderFrame;
    lpRenderer->Init = R_Init;
    lpRenderer->Shutdown = R_Shutdown;
    lpRenderer->BeginFrame = R_BeginFrame;
    lpRenderer->EndFrame = R_EndFrame;
    lpRenderer->DrawPic = R_DrawPic;
    ri = *lpImport;

    return lpRenderer;
}
