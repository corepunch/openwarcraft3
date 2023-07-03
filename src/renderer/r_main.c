#include "r_local.h"

#include <SDL.h>
#include <SDL_opengl.h>

refImport_t ri;
struct render_globals tr;

SDL_Window *window;
SDL_GLContext context;

bool is_rendering_lights = false;

void M3_Init(void);
void MDLX_Init(void);

void M3_Shutdown(void);
void MDLX_Shutdown(void);


LPTEXTURE R_LoadTextureBLP1(HANDLE data, DWORD filesize);
LPTEXTURE R_LoadTextureBLP2(HANDLE data, DWORD filesize);
LPTEXTURE R_LoadTextureDDS(HANDLE data, DWORD filesize);

void R_Viewport(LPCRECT viewport) {
    size2_t const windowSize = R_GetWindowSize();
    glViewport(viewport->x * windowSize.width / 800,
               viewport->y * windowSize.height / 600,
               viewport->w * windowSize.width / 800,
               viewport->h * windowSize.height / 600);
}

LPTEXTURE R_LoadTexture(LPCSTR textureFilename) {
    LPTEXTURE texture = NULL;
    HANDLE file = ri.FileOpen(textureFilename);
    if (!file)
        return NULL;
    DWORD fileSize = SFileGetFileSize(file, NULL);
    HANDLE buffer = ri.MemAlloc(fileSize);
    SFileReadFile(file, buffer, fileSize, NULL, NULL);
    switch (*(DWORD *)buffer) {
        case ID_BLP1:
            texture = R_LoadTextureBLP1(buffer, fileSize);
            break;
        case ID_BLP2:
            texture = R_LoadTextureBLP2(buffer, fileSize);
            break;
        case ID_DDS:
            texture = R_LoadTextureDDS(buffer, fileSize);
            break;
        default:
            fprintf(stderr, "Unknown texture format %.4s in file %s\n", (LPSTR)buffer, textureFilename);
            break;
    }
    ri.FileClose(file);
    SAFE_DELETE(buffer, ri.MemFree);
    return texture;
}

LPMODEL R_LoadModel(LPCSTR modelFilename) {
    HANDLE file = ri.FileOpen(modelFilename);
    LPMODEL model = NULL;
    if (file == NULL) {
        // try to load without *0.mdx
        PATHSTR tempFileName = { 0 };
        LPCSTR end = modelFilename + strlen(modelFilename) - 4;
        if (end > modelFilename && isdigit(*(end - 1))) {
            end--;
        }
        strncpy(tempFileName, modelFilename, end - modelFilename);
        strcpy(tempFileName + strlen(tempFileName), ".mdx");
        file = ri.FileOpen(tempFileName);
        if (!file) {
            fprintf(stderr, "Model not found: %s\n", modelFilename);
            return NULL;
        }
    }
    DWORD fileSize = SFileGetFileSize(file, NULL);
    void *buffer = ri.MemAlloc(fileSize);
    SFileReadFile(file, buffer, fileSize, NULL, NULL);
    switch (*(DWORD *)buffer) {
        case ID_MDLX:
            model = ri.MemAlloc(sizeof(model_t));
            model->mdx = R_LoadModelMDLX(buffer, fileSize);
            model->modeltype = ID_MDLX;
            break;
        case ID_43DM:
            model = ri.MemAlloc(sizeof(model_t));
            model->m3 = R_LoadModelM3(buffer, fileSize);
            model->modeltype = ID_43DM;
            break;
        default:
            fprintf(stderr, "Unknown model format %.4s in file %s\n", (LPSTR)buffer, modelFilename);
            break;
    }
    ri.FileClose(file);
    ri.MemFree(buffer);
    return model;
}

LPRENDERTARGET
R_AllocateRenderTexture(GLsizei width,
                        GLsizei height,
                        GLenum format,
                        GLenum type,
                        GLenum attachment)
{
    LPRENDERTARGET rt = ri.MemAlloc(sizeof(RENDERTARGET));
    R_Call(glGenFramebuffers, 1, &rt->buffer);
    R_Call(glGenTextures, 1, &rt->texture);
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, rt->buffer);
    R_Call(glBindTexture, GL_TEXTURE_2D, rt->texture);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    R_Call(glTexParameteri, GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    R_Call(glTexImage2D, GL_TEXTURE_2D, 0, format, width, height, 0, format, type, NULL);
    R_Call(glFramebufferTexture2D, GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, rt->texture, 0);
    if (attachment == GL_COLOR_ATTACHMENT0) {
        glClear(GL_COLOR_BUFFER_BIT);
    }
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, 0);
    return rt;
}

void R_ReleaseRenderTexture(LPRENDERTARGET rt) {
    glDeleteFramebuffers(1, &rt->buffer);
    glDeleteTextures(1, &rt->texture);
    ri.MemFree(rt);
}

static void R_SetupGL(bool drawLight) {
    size2_t const window = R_GetWindowSize();
    
    MATRIX4 model_matrix;
    MATRIX3 normal_matrix;
    MATRIX4 ui_matrix;

    Matrix4_identity(&model_matrix);
    if (tr.world) {
        VECTOR2 s = GetWar3MapSize(tr.world);
        VECTOR2 c = tr.world->center;
        Matrix4_ortho(&tr.viewDef.textureMatrix, -s.x+c.x, s.x+c.x, -s.y+c.y, s.y+c.y, 0.0f, 100.0f);
    } else {
        Matrix4_identity(&tr.viewDef.textureMatrix);
    }
    Matrix4_ortho(&ui_matrix, 0.0f, window.width, window.height, 0.0f, 0.0f, 100.0f);

    Matrix3_normal(&normal_matrix, &model_matrix);

    R_Call(glEnable, GL_CULL_FACE);
    R_Call(glCullFace, GL_BACK);
    
    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uViewProjectionMatrix, 1, GL_FALSE, drawLight ? tr.viewDef.lightMatrix.v : tr.viewDef.viewProjectionMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uTextureMatrix, 1, GL_FALSE, tr.viewDef.textureMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniformMatrix3fv, tr.shader[SHADER_DEFAULT]->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);

    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);

    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    
    R_Call(glDepthFunc, GL_LEQUAL);

    if (drawLight) {
        R_Call(glViewport, 0, 0, SHADOW_TEXSIZE, SHADOW_TEXSIZE);
        R_Call(glScissor, 0, 0, SHADOW_TEXSIZE, SHADOW_TEXSIZE);
        R_Call(glBindFramebuffer, GL_FRAMEBUFFER, tr.rt[RT_DEPTHMAP]->buffer);
        R_Call(glDepthMask, GL_TRUE);
        R_Call(glClear, GL_DEPTH_BUFFER_BIT);
    } else {
        R_Call(glBindFramebuffer, GL_FRAMEBUFFER, 0);
        R_Call(glActiveTexture, GL_TEXTURE1);
        R_Call(glBindTexture, GL_TEXTURE_2D, tr.rt[RT_DEPTHMAP]->texture);
    }
}

LPCSTR selCirclesNames[NUM_SELECTION_CIRCLES] = {
    "ReplaceableTextures\\Selection\\SelectionCircleSmall.blp",
    "ReplaceableTextures\\Selection\\SelectionCircleMed.blp",
    "ReplaceableTextures\\Selection\\SelectionCircleLarge.blp",
};

LPCSTR sheetNames[SHEET_COUNT] = {
    "TerrainArt\\Terrain.slk",
    "TerrainArt\\CliffTypes.slk",
};

LPCSTR modelNames[MODEL_COUNT] = {
    "UI\\Feedback\\SelectionCircle\\SelectionCircle.mdx"
};

//#include "mdx/r_mdx.h"

LPTEXTURE R_AllocateSinglePixelTexture(int color) {
    LPTEXTURE texture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(texture, 0, (LPCCOLOR32)&color, 1, 1);
    return texture;
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
        
//    m3 = R_LoadModel("Assets\\Units\\Terran\\SpecialOpsDropship\\SpecialOpsDropship.m3");
//    R_LoadModel("Assets\\Units\\Terran\\MarineTychus\\MarineTychus.m3");
//    R_LoadModel("Assets\\Units\\Zerg\\Queen\\Queen.m3");
    
//
//    LPCSTR models[] = {
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHAAL0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHLAA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransXCAH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAXHA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAAXH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAALH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransALHB0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAHXC0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAAHL0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransBALH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHXCA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransCXHA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransXHAA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransALHA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransBHLA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHBAL0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransABHL0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransCAHX0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHAAX0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransXAAH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransLAAH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAHXA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHXAA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransXHAC0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransLHAA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransACXH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHACX0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransLHBA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAHLA0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransLABH0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransHLAB0.mdx",
//        "Doodads\\Terrain\\CliffTrans\\CliffTransAAHX0.mdx",
//        NULL
//    };
//
//    for (LPCSTR *model = models; *model; model++){
//        model_t *m = R_LoadModel(*model);
//        printf("%f %f %f %f\n", m->mdx->bounds.box.min.x, m->mdx->bounds.box.min.y, m->mdx->bounds.box.max.x, m->mdx->bounds.box.max.y);
//    }
//
    extern LPCSTR vs_skin;
    extern LPCSTR vs_default;
    extern LPCSTR fs_default;
    extern LPCSTR fs_ui;
    extern LPCSTR fs_alphatest;
    
    FOR_LOOP(i, MODEL_COUNT) {
        tr.model[i] = R_LoadModel(modelNames[i]);
    }
    
    FOR_LOOP(i, SHEET_COUNT) {
        tr.sheet[i] = ri.ReadSheet(sheetNames[i]);
    }
    
    FOR_LOOP(i, NUM_SELECTION_CIRCLES) {
        tr.texture[TEX_SELECTION_CIRCLE+i] = R_LoadTexture(selCirclesNames[i]);
    }

    FOR_LOOP(team, MAX_TEAMS) {
        PATHSTR glowFilename, colorFilename;
        sprintf(glowFilename, "ReplaceableTextures\\TeamGlow\\TeamGlow%02d.blp", team);
        sprintf(colorFilename, "ReplaceableTextures\\TeamColor\\TeamColor%02d.blp", team);
        tr.texture[TEX_TEAM_GLOW + team] = R_LoadTexture(glowFilename);
        tr.texture[TEX_TEAM_COLOR + team] = R_LoadTexture(colorFilename);
    }

    tr.shader[SHADER_DEFAULT] = R_InitShader(vs_default, fs_default);
    tr.shader[SHADER_UI] = R_InitShader(vs_default, fs_ui);
    
    tr.buffer[RBUF_TEMP1] = R_MakeVertexArrayObject(NULL, 0);
    tr.texture[TEX_WHITE] = R_AllocateSinglePixelTexture(0xffffffff);
    tr.texture[TEX_BLACK] = R_AllocateSinglePixelTexture(0xff000000);
    tr.texture[TEX_WATER] = R_LoadTexture("ReplaceableTextures\\Water\\Water12.blp");
    tr.texture[TEX_FONT] = R_MakeSysFontTexture();
    tr.rt[RT_DEPTHMAP] = R_AllocateRenderTexture(SHADOW_TEXSIZE, SHADOW_TEXSIZE, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);
    
    R_Call(glDisable, GL_DEPTH_TEST);
    R_Call(glClearColor, 0.0, 0.0, 0.0, 0.0);
    R_Call(glViewport, 0, 0, width, height);
    
    R_InitParticles();
    
    M3_Init();
    MDLX_Init();
}

void R_Shutdown(void) {
    M3_Shutdown();
    MDLX_Shutdown();
    
    R_ShutdownFogOfWar();
    R_ShutdownParticles();
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
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
    R_BindTexture(tr.texture[TEX_SHADOWMAP], 1);
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
    R_DrawParticles();
    R_RevertSettings();
    
//    extern LPCTEXTURE dds;
//    R_DrawPic(dds, 0, 0);
}

void R_RenderFrame(viewDef_t const *viewDef) {
    R_Call(glActiveTexture, GL_TEXTURE2);
    R_Call(glBindTexture, GL_TEXTURE_2D, R_GetFogOfWarTexture());
    R_Call(glActiveTexture, GL_TEXTURE0);

    tr.viewDef = *viewDef;
    Frustum_Calculate(&viewDef->viewProjectionMatrix, &tr.viewDef.frustum);

    R_RenderFogOfWar();
    R_RenderShadowMap();
    R_RenderView();
//    R_RenderOverlays();
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
