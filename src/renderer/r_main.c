#include "r_local.h"

#include "TerrainArt/Terrain.h"
#include "TerrainArt/CliffTypes.h"

#include <SDL.h>
#include <SDL_opengl.h>

struct renderer_import ri;
struct render_globals tr;

SDL_Window *window;
SDL_GLContext context;

bool is_rendering_lights = false;

#define SHADOWMAP_SIZE 1000

void R_GetLigthMatrix(LPMATRIX4 lightSpaceMatrix) {
    VECTOR3 sunorg = Vector3_unm(&tr.viewDef.vieworg);
    VECTOR3 sunangles = { -35, 0, 45 };
    sunorg.y -= 700;
    sunorg.x += 500;
    Matrix4_ortho(lightSpaceMatrix, -SHADOWMAP_SIZE, SHADOWMAP_SIZE, -SHADOWMAP_SIZE, SHADOWMAP_SIZE, 100.0, 3500.0);
    Matrix4_rotate(lightSpaceMatrix, &(VECTOR3){0,0,45}, ROTATE_ZYX);
    Matrix4_rotate(lightSpaceMatrix, &sunangles, ROTATE_ZYX);
    Matrix4_translate(lightSpaceMatrix, &sunorg);
}

void R_GetProjectionMatrix(LPMATRIX4 projectionMatrix, viewDef_t const *viewDef) {
    SIZE2 const windowSize = R_GetWindowSize();
    float const aspect = (float)windowSize.width / (float)windowSize.height;
    VECTOR3 vieworg = Vector3_unm(&tr.viewDef.vieworg);
    Matrix4_perspective(projectionMatrix, tr.viewDef.fov, aspect, 100.0, 100000.0);
    Matrix4_rotate(projectionMatrix, &tr.viewDef.viewangles, ROTATE_XYZ);
    Matrix4_translate(projectionMatrix, &vieworg);
}

static void R_SetupGL(bool drawLight) {
    SIZE2 const window = R_GetWindowSize();
    MATRIX4 model_matrix;
    MATRIX3 normal_matrix;
    MATRIX4 ui_matrix;
    Matrix4_ortho(&ui_matrix, 0.0f, window.width, window.height, 0.0f, 0.0f, 100.0f);

    Matrix4_identity(&model_matrix);

    R_GetLigthMatrix(&tr.viewDef.light_matrix);
    R_GetProjectionMatrix(&tr.viewDef.projection_matrix, &tr.viewDef);

    if (drawLight) {
        tr.viewDef.projection_matrix = tr.viewDef.light_matrix;
    }
    Matrix3_normal(&normal_matrix, &model_matrix);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

    glUseProgram(tr.shaderSkin->progid);

    glUniformMatrix4fv(tr.shaderSkin->uProjectionMatrix, 1, GL_FALSE, tr.viewDef.projection_matrix.v);
    glUniformMatrix4fv(tr.shaderSkin->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    glUniformMatrix4fv(tr.shaderSkin->uLightMatrix, 1, GL_FALSE, tr.viewDef.light_matrix.v);
    glUniformMatrix3fv(tr.shaderSkin->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);

    glUseProgram(tr.shaderStatic->progid);

    glUniformMatrix4fv(tr.shaderStatic->uProjectionMatrix, 1, GL_FALSE, tr.viewDef.projection_matrix.v);
    glUniformMatrix4fv(tr.shaderStatic->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    glUniformMatrix4fv(tr.shaderStatic->uLightMatrix, 1, GL_FALSE, tr.viewDef.light_matrix.v);
    glUniformMatrix3fv(tr.shaderStatic->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);

    glUseProgram(tr.shaderUI->progid);

    glUniformMatrix4fv(tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    glUniformMatrix4fv(tr.shaderUI->uModelMatrix, 1, GL_FALSE, model_matrix.v);
}

void R_RenderFrame(viewDef_t const *viewDef) {
    SIZE2 const windowSize = R_GetWindowSize();

    tr.viewDef = *viewDef;

    // 1. first render to depth map
    glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    glBindFramebuffer(GL_FRAMEBUFFER, tr.depthMapFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
    is_rendering_lights = true;
    R_SetupGL(true);
    R_BindTexture(tr.shadowmap, 1);
    R_DrawWorld();
    R_DrawEntities();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    is_rendering_lights = false;
    // 2. then render scene as normal with shadow mapping (using depth map)
    glViewport(0, 0, windowSize.width, windowSize.height);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    R_SetupGL(false);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, tr.depthMap);
    R_DrawWorld();
    R_DrawEntities();
    R_DrawAlphaSurfaces();
}

void R_InitShadowMap(void) {
    glGenFramebuffers(1, &tr.depthMapFBO);
    glGenTextures(1, &tr.depthMap);
    glBindTexture(GL_TEXTURE_2D, tr.depthMap);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindFramebuffer(GL_FRAMEBUFFER, tr.depthMapFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tr.depthMap, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void R_Init(DWORD width, DWORD height) {
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

    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    context = SDL_GL_CreateContext(window);

    extern LPCSTR vertex_shader_skin;
    extern LPCSTR vertex_shader;
    extern LPCSTR fragment_shader;
    extern LPCSTR fragment_shader_ui;
    extern LPCSTR fragment_shader_alphatest;

    int white = -1;
    
    tr.shaderStatic = R_InitShader(vertex_shader, fragment_shader);
    tr.shaderSkin = R_InitShader(vertex_shader_skin, fragment_shader);
    tr.shaderUI = R_InitShader(vertex_shader, fragment_shader_ui);
    tr.renbuf = R_MakeVertexArrayObject(NULL, 0);
    tr.whiteTexture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(tr.whiteTexture, 0, (LPCCOLOR32)&white, 1, 1);

    glDisable(GL_DEPTH_TEST);
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glViewport(0, 0, width, height);
    
    FOR_LOOP(team, MAX_TEAMS) {
        PATHSTR glowFilename, colorFilename;
        sprintf(glowFilename, "ReplaceableTextures\\TeamGlow\\TeamGlow%02d.blp", team);
        sprintf(colorFilename, "ReplaceableTextures\\TeamColor\\TeamColor%02d.blp", team);
        tr.teamGlow[team] = R_LoadTexture(glowFilename);
        tr.teamColor[team] = R_LoadTexture(colorFilename);
    }

    tr.waterTexture = R_LoadTexture("ReplaceableTextures\\Water\\Water12.blp");
    tr.sysFont = R_MakeSysFontTexture();

    R_InitShadowMap();

    InitCliffTypes();
    InitTerrain();
}

VERTEX *R_AddQuad(VERTEX *buffer, struct rect const *screen, struct rect const *uv, COLOR32 color) {
    VERTEX const data[] = {
        {
            .position = { screen->x, screen->y, 0 },
            .texcoord = { uv->x, uv->y },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y, 0 },
            .texcoord = { uv->x+uv->width, uv->y },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y+screen->height, 0 },
            .texcoord = { uv->x+uv->width, uv->y+uv->height },
            .color = color,
        },
        {
            .position = { screen->x, screen->y, 0 },
            .texcoord = { uv->x, uv->y },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y+screen->height, 0 },
            .texcoord = { uv->x+uv->width, uv->y+uv->height },
            .color = color,
        },
        {
            .position = { screen->x, screen->y+screen->height, 0 },
            .texcoord = { uv->x, uv->y+uv->height },
            .color = color,
        },
    };
    memcpy(buffer, data, sizeof(data));
    return buffer + 6;
}

void R_PrintText(LPCSTR string, DWORD x, DWORD y, COLOR32 color) {
    static VERTEX simp[256 * 6];
    LPVERTEX it = simp;
    for (LPCSTR s = string; *s; s++) {
        DWORD ch = *s;
        float fx = ch % 16;
        float fy = ch / 16;
        it = R_AddQuad(it, &(struct rect) {
            x + 10 * (s - string), y, 8, 16
        }, &(struct rect) {
            fx/16,fy/8,1.f/16,1.f/8
        }, color);
    }

    DWORD num_vertices = (DWORD)(it - simp);

    glUseProgram(tr.shaderUI->progid);
    glBindVertexArray(tr.renbuf->vao);
    glBindBuffer(GL_ARRAY_BUFFER, tr.renbuf->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX) * num_vertices, simp, GL_STATIC_DRAW);

    R_BindTexture(tr.sysFont, 0);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
}

void R_DrawPic(LPCTEXTURE texture, DWORD x, DWORD y) {
    static VERTEX simp[6];
    R_AddQuad(simp, &(struct rect) {
        x, y, texture->width, texture->height
    }, &(struct rect) { 0,0,1,1 }, (COLOR32){255,255,255,255});

    glUseProgram(tr.shaderUI->progid);
    glBindVertexArray(tr.renbuf->vao);
    glBindBuffer(GL_ARRAY_BUFFER, tr.renbuf->vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);

    R_BindTexture(texture, 0);

    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}
bool R_IsPointVisible(LPCVECTOR3 point, float fThreshold) {
    VECTOR3 screen;
    Matrix4_multiply_vector3(&tr.viewDef.projection_matrix, point, &screen);
    if (screen.x < -fThreshold) return false;
    if (screen.y < -fThreshold) return false;
    if (screen.x > fThreshold) return false;
    if (screen.y > fThreshold) return false;
    return true;
}

void R_DrawBuffer(LPCBUFFER buffer, DWORD num_vertices) {
    glBindVertexArray(buffer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, buffer->vbo);
    glDrawArrays(GL_TRIANGLES, 0, num_vertices);
}

LPBUFFER R_MakeVertexArrayObject(LPCVERTEX vertices, DWORD size) {
    LPBUFFER buf = ri.MemAlloc(sizeof(BUFFER));

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
    glEnableVertexAttribArray(attrib_normal);

    glVertexAttribPointer(attrib_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, color));
    glVertexAttribPointer(attrib_position, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, position));
    glVertexAttribPointer(attrib_texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord));
    glVertexAttribPointer(attrib_texcoord2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord2));
    glVertexAttribPointer(attrib_skin, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vertex), FOFS(vertex, skin));
    glVertexAttribPointer(attrib_boneWeight, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, boneWeight));
    glVertexAttribPointer(attrib_normal, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, normal));

    if (vertices) {
        glBufferData(GL_ARRAY_BUFFER, size * sizeof(VERTEX), vertices, GL_STATIC_DRAW);
    }

    return buf;
}

void R_ReleaseVertexArrayObject(LPBUFFER buffer) {
    glDeleteBuffers(1, &buffer->vbo);
    glDeleteVertexArrays(1, &buffer->vao);
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
    ShutdownTerrain();
    ShutdownCliffTypes();

    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

struct size2 R_GetWindowSize(void) {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return (struct size2) {
        .width = width,
        .height = height,
    };
}

struct Renderer *Renderer_Init(struct renderer_import *import) {
    struct Renderer *renderer = import->MemAlloc(sizeof(struct Renderer));
    renderer->Init = R_Init;
    renderer->RegisterMap = R_RegisterMap;
    renderer->LoadTexture = R_LoadTexture;
    renderer->LoadModel = R_LoadModel;
    renderer->ReleaseModel = R_ReleaseModel;
    renderer->RenderFrame = R_RenderFrame;
    renderer->Init = R_Init;
    renderer->Shutdown = R_Shutdown;
    renderer->BeginFrame = R_BeginFrame;
    renderer->EndFrame = R_EndFrame;
    renderer->DrawPic = R_DrawPic;
    renderer->PrintText = R_PrintText;
    renderer->GetWindowSize = R_GetWindowSize;

    ri = *import;

    return renderer;
}
