#include "r_local.h"

#include <SDL.h>
#include <SDL_opengl.h>

refImport_t ri;
struct render_globals tr;

SDL_Window *window;
SDL_GLContext context;
VECTOR2 GetWar3MapSize(LPCWAR3MAP war3Map);

bool is_rendering_lights = false;

void R_Viewport(LPCRECT viewport) {
    size2_t const windowSize = R_GetWindowSize();
    glViewport(viewport->x * windowSize.width / 800,
               viewport->y * windowSize.height / 600,
               viewport->w * windowSize.width / 800,
               viewport->h * windowSize.height / 600);
}

static void R_SetupGL(bool drawLight) {
    size2_t const window = R_GetWindowSize();
    
    MATRIX4 model_matrix;
    MATRIX3 normal_matrix;
    MATRIX4 ui_matrix;
    MATRIX4 texture_matrix;

    Matrix4_identity(&model_matrix);
    if (tr.world) {
        VECTOR2 s = GetWar3MapSize(tr.world);
        VECTOR2 c = tr.world->center;
        Matrix4_ortho(&texture_matrix, -s.x+c.x, s.x+c.x, -s.y+c.y, s.y+c.y, 0.0f, 100.0f);
    } else {
        Matrix4_identity(&texture_matrix);
    }
    Matrix4_ortho(&ui_matrix, 0.0f, window.width, window.height, 0.0f, 0.0f, 100.0f);

    Matrix3_normal(&normal_matrix, &model_matrix);

    R_Call(glEnable, GL_CULL_FACE);
    R_Call(glCullFace, GL_BACK);

    R_Call(glUseProgram, tr.shaderSkin->progid);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uProjectionMatrix, 1, GL_FALSE, drawLight ? tr.viewDef.lightMatrix.v : tr.viewDef.projectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uTextureMatrix, 1, GL_FALSE, texture_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderSkin->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniformMatrix3fv, tr.shaderSkin->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);
    
    R_Call(glUseProgram, tr.shaderStatic->progid);
    R_Call(glUniformMatrix4fv, tr.shaderStatic->uProjectionMatrix, 1, GL_FALSE, drawLight ? tr.viewDef.lightMatrix.v : tr.viewDef.projectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderStatic->uTextureMatrix, 1, GL_FALSE, texture_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderStatic->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderStatic->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniformMatrix3fv, tr.shaderStatic->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);

    R_Call(glUseProgram, tr.shaderUI->progid);

    R_Call(glUniformMatrix4fv, tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderUI->uModelMatrix, 1, GL_FALSE, model_matrix.v);

    R_Call(glDepthFunc, GL_LEQUAL);

    if (drawLight) {
        R_Call(glViewport, 0, 0, SHADOW_TEXSIZE, SHADOW_TEXSIZE);
        R_Call(glScissor, 0, 0, SHADOW_TEXSIZE, SHADOW_TEXSIZE);
        R_Call(glBindFramebuffer, GL_FRAMEBUFFER, tr.depthMapFBO);
        R_Call(glDepthMask, GL_TRUE);
        R_Call(glClear, GL_DEPTH_BUFFER_BIT);
    } else {
        R_Call(glBindFramebuffer, GL_FRAMEBUFFER, 0);
        R_Call(glActiveTexture, GL_TEXTURE1);
        R_Call(glBindTexture, GL_TEXTURE_2D, tr.depthMap);
    }
}

void R_InitShadowMap(void) {
    R_Call(glGenFramebuffers, 1, &tr.depthMapFBO);
    R_Call(glGenTextures, 1, &tr.depthMap);
    R_Call(glBindTexture, GL_TEXTURE_2D, tr.depthMap);
    R_Call(glTexImage2D, GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_TEXSIZE, SHADOW_TEXSIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
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
    int black = 0xff000000;

    tr.terrainSheet = ri.ReadSheet("TerrainArt\\Terrain.slk");
    tr.cliffSheet = ri.ReadSheet("TerrainArt\\CliffTypes.slk");

//    tr.selectionCircle = R_LoadModel("UI\\Feedback\\Confirmation\\Confirmation.mdx");
    tr.selectionCircle = R_LoadModel("UI\\Feedback\\SelectionCircle\\SelectionCircle.mdx");
    tr.selectionCircleSmall = R_LoadTexture("ReplaceableTextures\\Selection\\SelectionCircleSmall.blp");
    tr.selectionCircleMed = R_LoadTexture("ReplaceableTextures\\Selection\\SelectionCircleMed.blp");
    tr.selectionCircleLarge = R_LoadTexture("ReplaceableTextures\\Selection\\SelectionCircleLarge.blp");

    tr.shaderStatic = R_InitShader(vertex_shader, fragment_shader);
    tr.shaderSkin = R_InitShader(vertex_shader_skin, fragment_shader);
    tr.shaderUI = R_InitShader(vertex_shader, fragment_shader_ui);
    tr.renbuf = R_MakeVertexArrayObject(NULL, 0);
    tr.whiteTexture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(tr.whiteTexture, 0, (LPCCOLOR32)&white, 1, 1);
    tr.blackTexture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(tr.blackTexture, 0, (LPCCOLOR32)&black, 1, 1);
    
#define SIGHT_SIZE 64
    
    tr.sightTexture = R_AllocateTexture(SIGHT_SIZE, SIGHT_SIZE);
    COLOR32 col[SIGHT_SIZE * SIGHT_SIZE];
    DWORD mid = SIGHT_SIZE/2;
    VECTOR2 center = {mid,mid};
    FOR_LOOP(x, SIGHT_SIZE) {
        FOR_LOOP(y, SIGHT_SIZE) {
            float const d = Vector2_distance(&center, &(VECTOR2){x,y});
            float const f = MAX(0, 1.0 - d / mid);
            DWORD c = MIN(1, f * 2.0) * 0xff;
            col[x+y*SIGHT_SIZE].r = 0xff;
            col[x+y*SIGHT_SIZE].g = 0xff;
            col[x+y*SIGHT_SIZE].b = 0xff;
            col[x+y*SIGHT_SIZE].a = c;
        }
    }
    R_LoadTextureMipLevel(tr.sightTexture, 0, col, SIGHT_SIZE, SIGHT_SIZE);

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
}

bool R_IsPointVisible(LPCVECTOR3 point, float fThreshold) {
    VECTOR3 screen = Matrix4_multiply_vector3(&tr.viewDef.projectionMatrix, point);
    if (screen.x < -fThreshold) return false;
    if (screen.y < -fThreshold) return false;
    if (screen.x > fThreshold) return false;
    if (screen.y > fThreshold) return false;
    return true;
}

DWORD R_GetViewWidth(void) {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return width;
}

DWORD R_GetViewHeight(void) {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return height;
}

void R_SetupViewport(LPCRECT r) {
    DWORD w = R_GetViewWidth(), h  = R_GetViewHeight();
    R_Call(glViewport, r->x * w, r->y * h, r->w * w, r->h * h);
}

void R_SetupScissor(LPCRECT r) {
    DWORD w = R_GetViewWidth(), h  = R_GetViewHeight();
    R_Call(glEnable, GL_SCISSOR_TEST);
    R_Call(glScissor, r->x * w, r->y * h, r->w * w, r->h * h);
}

void R_RevertSettings(void) {
    R_SetupViewport(&(RECT){0,0,1,1});
    R_SetupScissor(&(RECT){0,0,1,1});
}

void R_RenderShadowMap(void) {
    is_rendering_lights = true;
    R_SetupGL(true);
    R_BindTexture(tr.shadowmap, 1);
    R_DrawWorld();
    R_DrawEntities();
}

void R_RenderView(void) {
    is_rendering_lights = false;
    R_SetupViewport(&tr.viewDef.viewport);
    R_SetupScissor(&tr.viewDef.scissor);
    R_SetupGL(false);
    R_DrawWorld();
    R_DrawEntities();
    R_DrawAlphaSurfaces();
    R_RevertSettings();
}

void R_RenderFogOfWar(void) {
    if (!tr.world)
        return;
    MATRIX4 model_matrix, proj_matrix;
    VECTOR2 mapsize = GetWar3MapSize(tr.world);
    DWORD const texwidth = (tr.world->width - 1) * 4;
    DWORD const texheight = (tr.world->height - 1) * 4;
    Matrix4_identity(&model_matrix);
    Matrix4_ortho(&proj_matrix, 0.0f, mapsize.x, 0.0f, mapsize.y, 0.0f, 100.0f);

    R_Call(glUseProgram, tr.shaderUI->progid);
    R_Call(glUniformMatrix4fv, tr.shaderUI->uProjectionMatrix, 1, GL_FALSE, proj_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shaderUI->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glViewport, 0, 0, texwidth, texheight);
    R_Call(glScissor, 0, 0, texwidth, texheight);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, tr.world->fogOfWarFBO);
    R_Call(glDepthMask, GL_FALSE);
    R_Call(glDepthFunc, GL_ALWAYS);
    R_Call(glClearColor, 0, 0, 0, 0);
    R_Call(glClear, GL_COLOR_BUFFER_BIT);
    R_Call(glActiveTexture, GL_TEXTURE0);
    R_Call(glBindTexture, GL_TEXTURE_2D, tr.sightTexture->texid);

    float viewsize = 2000;
    
    FOR_LOOP(p, tr.viewDef.num_entities) {
        renderEntity_t *ent = tr.viewDef.entities+p;
        if (ent->team != 1)continue;
        VERTEX simp[6];
        RECT screen = {
            ent->origin.x - tr.world->center.x - viewsize/2,
            ent->origin.y - tr.world->center.y - viewsize/2,
            viewsize,
            viewsize,
        };
        R_AddQuad(simp, &screen, &(RECT){0,0,1,1}, (COLOR32){255,255,255,255});
                
        R_Call(glDisable, GL_CULL_FACE);
        R_Call(glBindVertexArray, tr.renbuf->vao);
        R_Call(glBindBuffer, GL_ARRAY_BUFFER, tr.renbuf->vbo);
        R_Call(glBufferData, GL_ARRAY_BUFFER, sizeof(VERTEX) * 6, simp, GL_STATIC_DRAW);
        R_Call(glDisable, GL_CULL_FACE);
        R_Call(glEnable, GL_BLEND);
        R_Call(glBlendFunc, GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        R_Call(glDrawArrays, GL_TRIANGLES, 0, 6);
    }
}

void R_RenderFrame(viewDef_t const *viewDef) {
    R_Call(glActiveTexture, GL_TEXTURE2);
    if (tr.world) {
        R_Call(glBindTexture, GL_TEXTURE_2D, tr.world->fogOfWarTexture);
    } else {
        R_Call(glBindTexture, GL_TEXTURE_2D, tr.whiteTexture->texid);
    }
    R_Call(glActiveTexture, GL_TEXTURE0);
    tr.viewDef = *viewDef;
    R_RenderFogOfWar();
    R_RenderShadowMap();
    R_RenderView();
}

void R_DrawBuffer(LPCBUFFER buffer, DWORD num_vertices) {
    R_Call(glBindVertexArray, buffer->vao);
    R_Call(glBindBuffer, GL_ARRAY_BUFFER, buffer->vbo);
    R_Call(glDrawArrays, GL_TRIANGLES, 0, num_vertices);
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
    SDL_GL_SwapWindow(window);
    SDL_Delay(1);
}

void R_Shutdown(void) {
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

size2_t R_GetWindowSize(void) {
    int width, height;
    SDL_GetWindowSize(window, &width, &height);
    return (size2_t) {
        .width = width,
        .height = height,
    };
}

size2_t R_GetTextureSize(LPCTEXTURE texture) {
    if (!texture) {
        return (size2_t) { 0, 0 };
    } else {
        return (size2_t) {
            .width = texture->width,
            .height = texture->height,
        };
    }
}

refExport_t R_GetAPI(refImport_t imp) {
#ifdef DEBUG_PATHFINDING
    void R_SetPathTexture(LPCCOLOR32 debugTexture);
#endif
    ri = imp;
    return (refExport_t) {
        .Init = R_Init,
        .RegisterMap = R_RegisterMap,
        .LoadTexture = R_LoadTexture,
        .LoadModel = R_LoadModel,
        .LoadFont = R_LoadFont,
        .ReleaseModel = R_ReleaseModel,
        .RenderFrame = R_RenderFrame,
        .Shutdown = R_Shutdown,
        .BeginFrame = R_BeginFrame,
        .EndFrame = R_EndFrame,
        .DrawPic = R_DrawPic,
        .DrawImage = R_DrawImage,
        .DrawSelectionRect = R_DrawSelectionRect,
        .PrintSysText = R_PrintSysText,
        .GetWindowSize = R_GetWindowSize,
        .GetTextureSize = R_GetTextureSize,
        .DrawPortrait = R_DrawPortrait,
        .DrawText = R_DrawText,
        .TraceEntity = R_TraceEntity,
        .TraceLocation = R_TraceLocation,
        .EntitiesInRect = R_EntitiesInRect,
#ifdef DEBUG_PATHFINDING
        .SetPathTexture = R_SetPathTexture,
#endif
    };
}
