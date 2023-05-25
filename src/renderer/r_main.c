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

    R_Call(glEnable, GL_CULL_FACE);
    R_Call(glCullFace, GL_BACK);

    R_Call(glUseProgram, tr.shaderSkin->progid);

    R_Call(glUniformMatrix4fv, tr.shaderSkin->uProjectionMatrix, 1, GL_FALSE, tr.viewDef.projection_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uLightMatrix, 1, GL_FALSE, tr.viewDef.light_matrix.v);
    R_Call(glUniformMatrix3fv, tr.shaderSkin->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);

    R_Call(glUseProgram, tr.shaderStatic->progid);

    R_Call(glUniformMatrix4fv, tr.shaderStatic->uProjectionMatrix, 1, GL_FALSE, tr.viewDef.projection_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderStatic->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderStatic->uLightMatrix, 1, GL_FALSE, tr.viewDef.light_matrix.v);
    R_Call(glUniformMatrix3fv, tr.shaderStatic->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);

    R_Call(glUseProgram, tr.shaderUI->progid);

    R_Call(glUniformMatrix4fv, tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderUI->uModelMatrix, 1, GL_FALSE, model_matrix.v);
}

void R_InitShadowMap(void) {
    R_Call(glGenFramebuffers, 1, &tr.depthMapFBO);
    R_Call(glGenTextures, 1, &tr.depthMap);
    R_Call(glBindTexture, GL_TEXTURE_2D, tr.depthMap);
    R_Call(glTexImage2D, GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, tr.depthMapFBO);
    R_Call(glFramebufferTexture2D, GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, tr.depthMap, 0);
    R_Call(glDrawBuffer, GL_NONE);
    R_Call(glReadBuffer, GL_NONE);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, 0);
}

LPCTEXTURE tex1, tex2, tex3, tex4;

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

    int white = 0xffffffff;
    int black = 0x000000ff;
    
    tex1 = R_LoadTexture("UI\\Console\\Human\\HumanUITile01.blp");
    tex2 = R_LoadTexture("UI\\Console\\Human\\HumanUITile02.blp");
    tex3 = R_LoadTexture("UI\\Console\\Human\\HumanUITile03.blp");
    tex4 = R_LoadTexture("UI\\Console\\Human\\HumanUITile04.blp");
//    tr.selectionCircle = R_LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
    tr.selectionCircle = R_LoadModel("UI\\Feedback\\SelectionCircle\\SelectionCircle.mdx");
    tr.shaderStatic = R_InitShader(vertex_shader, fragment_shader);
    tr.shaderSkin = R_InitShader(vertex_shader_skin, fragment_shader);
    tr.shaderUI = R_InitShader(vertex_shader, fragment_shader_ui);
    tr.renbuf = R_MakeVertexArrayObject(NULL, 0);
    tr.whiteTexture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(tr.whiteTexture, 0, (LPCCOLOR32)&white, 1, 1);
    tr.blackTexture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(tr.blackTexture, 0, (LPCCOLOR32)&black, 1, 1);

    R_Call(glDisable, GL_DEPTH_TEST);
    R_Call(glClearColor, 0.0, 0.0, 0.0, 0.0);
    R_Call(glViewport, 0, 0, width, height);
    
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

VERTEX *R_AddStrip(VERTEX *buffer, struct rect const *screen, COLOR32 color) {
    VERTEX const data[] = {
        {
            .position = { screen->x, screen->y, 0 },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y, 0 },
            .color = color,
        },
        {
            .position = { screen->x+screen->width, screen->y+screen->height, 0 },
            .color = color,
        },
        {
            .position = { screen->x, screen->y+screen->height, 0 },
            .color = color,
        },
        {
            .position = { screen->x, screen->y, 0 },
            .color = color,
        },
    };
    memcpy(buffer, data, sizeof(data));
    return buffer + 5;
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

    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * num_vertices, simp, GL_STATIC_DRAW);

    R_BindTexture(tr.sysFont, 0);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, num_vertices);
}

void R_DrawPic(LPCTEXTURE texture, DWORD x, DWORD y) {
    static VERTEX simp[6];
    R_AddQuad(simp, &(struct rect) {
        x, y, texture->width, texture->height
    }, &(struct rect) { 0,0,1,1 }, (COLOR32){255,255,255,255});

    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);

    R_BindTexture(texture, 0);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, 6);
}

void R_DrawPicEx(LPCTEXTURE texture, DWORD x, DWORD y, struct rect const *uv) {
    static VERTEX simp[6];
    R_AddQuad(simp, &(struct rect) {
        x + uv->x * texture->width,
        y + uv->y * texture->height,
        texture->width * uv->width,
        texture->height * uv->height
    }, uv, (COLOR32){255,255,255,255});

    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);

    R_BindTexture(texture, 0);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, 6);
}

void R_DrawSelectionRect(struct rect const *rect, COLOR32 color) {
    static VERTEX simp[5];
    R_AddStrip(simp, rect, color);

    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glBindVertexArray, tr.renbuf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
    R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 5, simp, GL_STATIC_DRAW);

    R_BindTexture(tr.whiteTexture, 0);

    R_Call(glDisable, GL_CULL_FACE);
    R_Call(glEnable, GL_BLEND);
    R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    R_Call(glDrawArrays, GL_LINE_STRIP, 0, 5);
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

void R_RenderFrame(viewDef_t const *viewDef) {
    SIZE2 const windowSize = R_GetWindowSize();

    tr.viewDef = *viewDef;

    // 1. first render to depth map
    R_Call(glViewport, 0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, tr.depthMapFBO);
    R_Call(glClear, GL_DEPTH_BUFFER_BIT);
    is_rendering_lights = true;
    R_SetupGL(true);
    R_BindTexture(tr.shadowmap, 1);
    R_DrawWorld();
    R_DrawEntities();
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, 0);

    is_rendering_lights = false;
    // 2. then render scene as normal with shadow mapping (using depth map)
    R_Call(glViewport, 0, 0, windowSize.width, windowSize.height);
    R_Call(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    R_SetupGL(false);
    R_Call(glActiveTexture, GL_TEXTURE1);
    R_Call(glBindTexture, GL_TEXTURE_2D, tr.depthMap);
    R_DrawWorld();
    R_DrawEntities();
    R_DrawAlphaSurfaces();
}

void R_DrawBuffer(LPCBUFFER buffer, DWORD num_vertices) {
    R_Call(glBindVertexArray, buffer->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, buffer->vbo);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, num_vertices);
}

LPBUFFER R_MakeVertexArrayObject(LPCVERTEX vertices, DWORD size) {
    LPBUFFER buf = ri.MemAlloc(sizeof(BUFFER));

    R_Call(glGenVertexArrays, 1, &buf->vao);
    R_Call(glGenBuffers, 1, &buf->vbo);
    R_Call(glBindVertexArray, buf->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, buf->vbo);

    R_Call(glEnableVertexAttribArray, attrib_position);
    R_Call(glEnableVertexAttribArray, attrib_color);
    R_Call(glEnableVertexAttribArray, attrib_texcoord);
    R_Call(glEnableVertexAttribArray, attrib_texcoord2);
    R_Call(glEnableVertexAttribArray, attrib_skin);
    R_Call(glEnableVertexAttribArray, attrib_boneWeight);
    R_Call(glEnableVertexAttribArray, attrib_normal);

    R_Call(glVertexAttribPointer, attrib_color, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, color));
    R_Call(glVertexAttribPointer, attrib_position, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, position));
    R_Call(glVertexAttribPointer, attrib_texcoord, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord));
    R_Call(glVertexAttribPointer, attrib_texcoord2, 2, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, texcoord2));
    R_Call(glVertexAttribPointer, attrib_skin, 4, GL_UNSIGNED_BYTE, GL_FALSE, sizeof(struct vertex), FOFS(vertex, skin));
    R_Call(glVertexAttribPointer, attrib_boneWeight, 4, GL_UNSIGNED_BYTE, GL_TRUE, sizeof(struct vertex), FOFS(vertex, boneWeight));
    R_Call(glVertexAttribPointer, attrib_normal, 3, GL_FLOAT, GL_FALSE, sizeof(struct vertex), FOFS(vertex, normal));

    if (vertices) {
        R_Call(glBufferData, GL_ARRAY_BUFFER, size * sizeof(VERTEX), vertices, GL_STATIC_DRAW);
    }

    return buf;
}

void R_ReleaseVertexArrayObject(LPBUFFER buffer) {
    R_Call(glDeleteBuffers, 1, &buffer->vbo);
    R_Call(glDeleteVertexArrays, 1, &buffer->vao);
}

void R_BeginFrame(void) {
    R_Call(glEnable, GL_DEPTH_TEST);
    R_Call(glDepthMask, GL_TRUE);
    R_Call(glDepthFunc, GL_LEQUAL);
    R_Call(glEnable, GL_CULL_FACE);
    R_Call(glCullFace, GL_BACK);
    R_Call(glClear, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void R_EndFrame(void) {
//    MATRIX4 ui_matrix;
//    Matrix4_ortho(&ui_matrix, 0.0f, 1600, 1200/* window.height*/, 0.0f, 0.0f, 100.0f);
//    R_Call(glUseProgram, tr.shaderUI->progid);
//    R_Call(glUniformMatrix4fv, tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
//
//    R_DrawPicEx(tr.blackTexture, 0, 950, &(struct rect){0,0,1600,250});
//
//    R_DrawPicEx(tex1, 0, 0, &(struct rect){0,0,1,0.25});
//    R_DrawPicEx(tex2, 512, 0, &(struct rect){0,0,1,0.25});
//    R_DrawPicEx(tex3, 1024, 0, &(struct rect){0,0,1,0.25});
//    R_DrawPicEx(tex4, 1024+512, 0, &(struct rect){0,0,1,0.25});
//
//    R_DrawPicEx(tex1, 0, 1200-512, &(struct rect){0,0.25,1,0.75});
//    R_DrawPicEx(tex2, 512, 1200-512, &(struct rect){0,0.25,2,0.75});
//    R_DrawPicEx(tex3, 1024, 1200-512, &(struct rect){0,0.25,1,0.75});
//    R_DrawPicEx(tex4, 1024+512, 1200-512, &(struct rect){0,0.25,1,0.75});

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
    renderer->DrawSelectionRect = R_DrawSelectionRect;
    renderer->PrintText = R_PrintText;
    renderer->GetWindowSize = R_GetWindowSize;
#ifdef DEBUG_PATHFINDING
    void R_SetPathTexture(LPCCOLOR32 debugTexture);
    renderer->SetPathTexture = R_SetPathTexture;
#endif

    ri = *import;

    return renderer;
}
