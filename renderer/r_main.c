#include "r_local.h"
#include "m3/r_m3.h"

#include <SDL2/SDL.h>
#ifndef __APPLE__
#include <SDL2/SDL_opengl.h>
#endif

refImport_t ri;
struct render_globals tr;

SDL_Window *window;
SDL_GLContext context;

#ifdef USE_SHADOWMAPS
bool is_rendering_lights = false;
#endif
static bool renderer_shutdown = false;

void M3_Init(void);
void MDLX_Init(void);

void M3_Shutdown(void);
void MDLX_Shutdown(void);


LPTEXTURE R_LoadTextureBLP1(HANDLE data, DWORD filesize);
LPTEXTURE R_LoadTextureBLP2(HANDLE data, DWORD filesize);
LPTEXTURE R_LoadTextureDDS(HANDLE data, DWORD filesize);

void R_Viewport(LPCRECT viewport) {
    glViewport(viewport->x * tr.drawableSize.width / 800,
               viewport->y * tr.drawableSize.height / 600,
               viewport->w * tr.drawableSize.width / 800,
               viewport->h * tr.drawableSize.height / 600);
}

LPTEXTURE R_AllocateSinglePixelTexture(int color) {
    LPTEXTURE texture = R_AllocateTexture(1, 1);
    R_LoadTextureMipLevel(texture, 0, (LPCCOLOR32)&color, 1, 1);
    return texture;
}

LPTEXTURE R_LoadTexture(LPCSTR textureFilename) {
    LPTEXTURE texture = NULL;
    HANDLE file = ri.FileOpen(textureFilename);
    if (!file) {
        return R_AllocateSinglePixelTexture(0xffffffff);
    }
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
        // Warcraft III FDF often refers to models as .mdl even when only the
        // .mdx file is present in the archive. Try that translation first so
        // classic menu assets like WarCraftIIILogo resolve correctly.
        if (strstr(modelFilename, ".mdl")) {
            PATHSTR tempFileName = { 0 };
            LPSTR ext = strstr(modelFilename, ".mdl");
            strncpy(tempFileName, modelFilename, ext - modelFilename);
            strcpy(tempFileName + strlen(tempFileName), ".mdx");
            file = ri.FileOpen(tempFileName);
        }
        if (!file) {
            // Try again without trailing sequence digits, e.g. *0.mdx.
            PATHSTR tempFileName = { 0 };
            LPCSTR end = modelFilename + strlen(modelFilename) - 4;
            if (end > modelFilename && isdigit(*(end - 1))) {
                end--;
            }
            strncpy(tempFileName, modelFilename, end - modelFilename);
            strcpy(tempFileName + strlen(tempFileName), ".mdx");
            file = ri.FileOpen(tempFileName);
        }
        if (!file) {
            fprintf(stderr, "Model not found: %s\n", modelFilename);
            return NULL;
        }
    }
    DWORD fileSize = SFileGetFileSize(file, NULL);
    void *buffer = ri.MemAlloc(fileSize);
    SFileReadFile(file, buffer, fileSize, NULL, NULL);
    ri.FileClose(file);
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
            if (strstr(modelFilename, ".mdl")) {
                PATHSTR tempFileName = { 0 };
                LPSTR ext = strstr(modelFilename, ".mdl");
                if (ext) {
                    strncpy(tempFileName, modelFilename, ext - modelFilename);
                    strcpy(tempFileName + strlen(tempFileName), ".mdx");
                    ri.MemFree(buffer);
                    return R_LoadModel(tempFileName);
                }
            }
            fprintf(stderr, "Unknown model format %.4s in file %s\n", (LPSTR)buffer, modelFilename);
            break;
    }
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
    if (!rt) {
        return;
    }
    if (rt->buffer) {
        glDeleteFramebuffers(1, &rt->buffer);
        rt->buffer = 0;
    }
    if (rt->texture) {
        glDeleteTextures(1, &rt->texture);
        rt->texture = 0;
    }
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
    Matrix4_ortho(&ui_matrix, 0.0f, window.width, 0.0f, window.height, 0.0f, 100.0f);

    Matrix3_normal(&normal_matrix, &model_matrix);

    R_Call(glEnable, GL_CULL_FACE);
    R_Call(glCullFace, GL_BACK);
    
    GLfloat const *viewProjectionMatrix =
#ifdef USE_SHADOWMAPS
        drawLight ? tr.viewDef.lightMatrix.v :
#endif
        tr.viewDef.viewProjectionMatrix.v;

    R_Call(glUseProgram, tr.shader[SHADER_DEFAULT]->progid);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uViewProjectionMatrix, 1, GL_FALSE, viewProjectionMatrix);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uTextureMatrix, 1, GL_FALSE, tr.viewDef.textureMatrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_DEFAULT]->uLightMatrix, 1, GL_FALSE, tr.viewDef.lightMatrix.v);
    R_Call(glUniformMatrix3fv, tr.shader[SHADER_DEFAULT]->uNormalMatrix, 1, GL_TRUE, normal_matrix.v);

    R_Call(glUseProgram, tr.shader[SHADER_UI]->progid);

    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uViewProjectionMatrix, 1, GL_FALSE, ui_matrix.v);
    R_Call(glUniformMatrix4fv, tr.shader[SHADER_UI]->uModelMatrix, 1, GL_FALSE, model_matrix.v);
    
    R_Call(glDepthFunc, GL_LEQUAL);

#ifdef USE_SHADOWMAPS
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
#else
    (void)drawLight;
    R_Call(glBindFramebuffer, GL_FRAMEBUFFER, 0);
#endif
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

static LPCSTR R_GLString(GLenum name) {
    GLubyte const *value = glGetString(name);
    return value ? (LPCSTR)value : "unknown";
}

static void R_PrintGLExtensions(void) {
    GLint num_extensions = 0;

    glGetIntegerv(GL_NUM_EXTENSIONS, &num_extensions);
    fprintf(stderr, "GL_EXTENSIONS:");
    FOR_LOOP(i, num_extensions) {
        GLubyte const *extension = glGetStringi(GL_EXTENSIONS, i);
        if (extension) {
            fprintf(stderr, " %s", extension);
        }
    }
    fprintf(stderr, "\n\n");
}

static void R_PrintDisplayModes(void) {
    int num_modes = SDL_GetNumDisplayModes(0);

    fprintf(stderr, "SDL display modes:\n");
    if (num_modes <= 0) {
        fprintf(stderr, " - none reported\n");
        return;
    }

    FOR_LOOP(i, num_modes) {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) != 0) {
            continue;
        }
        fprintf(stderr,
                " - Mode %2d: %dx%d@%d\n",
                i,
                mode.w,
                mode.h,
                mode.refresh_rate);
    }
}

void R_Init(DWORD width, DWORD height) {
    renderer_shutdown = false;
    BOOL gl_current = false;
    SDL_version sdl_version;

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

    fprintf(stderr, "Video initialization.\n");
    SDL_GetVersion(&sdl_version);
    fprintf(stderr,
            "SDL version is: %d.%d.%d\n",
            sdl_version.major,
            sdl_version.minor,
            sdl_version.patch);
    fprintf(stderr, "SDL video driver is \"%s\".\n", SDL_GetCurrentVideoDriver());
    R_PrintDisplayModes();
    fprintf(stderr, "Video initialized.\n\n");
    
    fprintf(stderr, "Refresher initialization.\n");
    window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_ALLOW_HIGHDPI);
    context = SDL_GL_CreateContext(window);
    if (context && SDL_GL_MakeCurrent(window, context) == 0) {
        gl_current = true;
    } else {
        fprintf(stderr, "ref_gl::R_Init() - could not make GL context current: %s\n", SDL_GetError());
    }
    
    SDL_GL_GetDrawableSize(window, (int *)&tr.drawableSize.width, (int *)&tr.drawableSize.height);
    fprintf(stderr, "Refresh: OpenWarcraft3 OpenGL Refresher\n");
    fprintf(stderr, "Client: OpenWarcraft3\n\n");
    fprintf(stderr, "Drawable size: %dx%d\n\n", tr.drawableSize.width, tr.drawableSize.height);
    if (gl_current) {
        fprintf(stderr, "OpenGL setting:\n");
        fprintf(stderr, "GL_VENDOR: %s\n", R_GLString(GL_VENDOR));
        fprintf(stderr, "GL_RENDERER: %s\n", R_GLString(GL_RENDERER));
        fprintf(stderr, "GL_VERSION: %s\n", R_GLString(GL_VERSION));
        fprintf(stderr, "GL_SHADING_LANGUAGE_VERSION: %s\n", R_GLString(GL_SHADING_LANGUAGE_VERSION));
        R_PrintGLExtensions();
    }
    
//    m3 = R_LoadModel("Assets\\Units\\Terran\\SpecialOpsDropship\\SpecialOpsDropship.m3");
//    R_LoadModel("Assets\\Units\\Terran\\MarineTychus\\MarineTychus.m3");
//    R_LoadModel("Assets\\Units\\Zerg\\Queen\\Queen.m3");
    
    extern LPCSTR vs_skin;
    extern LPCSTR vs_default;
    extern LPCSTR fs_default;
    extern LPCSTR fs_ui;
    extern LPCSTR fs_splat;
    extern LPCSTR fs_alphatest;
    extern LPCSTR fs_commandbutton;
    
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
    tr.shader[SHADER_SPLAT] = R_InitShader(vs_default, fs_splat);
    tr.shader[SHADER_COMMANDBUTTON] = R_InitShader(vs_default, fs_commandbutton);
    fprintf(stderr, "Loading shaders succeeded.\n");

    tr.buffer[RBUF_TEMP1] = R_MakeVertexArrayObject(NULL, 0);
    tr.texture[TEX_WHITE] = R_AllocateSinglePixelTexture(0xffffffff);
    tr.texture[TEX_BLACK] = R_AllocateSinglePixelTexture(0xff000000);
    tr.texture[TEX_WATER] = R_LoadTexture("ReplaceableTextures\\Water\\Water12.blp");
    tr.texture[TEX_FONT] = R_MakeSysFontTexture();
#ifdef USE_SHADOWMAPS
    tr.rt[RT_DEPTHMAP] = R_AllocateRenderTexture(SHADOW_TEXSIZE, SHADOW_TEXSIZE, GL_DEPTH_COMPONENT, GL_FLOAT, GL_DEPTH_ATTACHMENT);
#endif
    R_Call(glDisable, GL_DEPTH_TEST);
    R_Call(glClearColor, 0.25, 0.25, 0.25, 1.0);
    R_Call(glViewport, 0, 0, tr.drawableSize.width, tr.drawableSize.height);
    R_InitParticles();
    M3_Init();
    MDLX_Init();
    fprintf(stderr, "Refresher initialized.\n\n");
}

void R_Shutdown(void) {
    if (renderer_shutdown) {
        return;
    }
    renderer_shutdown = true;
    M3_Shutdown();
    MDLX_Shutdown();
    
    R_ShutdownFogOfWar();
    R_ShutdownParticles();
    
    SDL_GL_DeleteContext(context);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

void R_SetupViewport(LPCRECT r) {
    R_Call(glViewport,
           r->x * tr.drawableSize.width,
           r->y * tr.drawableSize.height,
           r->w * tr.drawableSize.width,
           r->h * tr.drawableSize.height);
}

void R_SetupScissor(LPCRECT r) {
    R_Call(glEnable, GL_SCISSOR_TEST);
    R_Call(glScissor,
           r->x * tr.drawableSize.width,
           r->y * tr.drawableSize.height,
           r->w * tr.drawableSize.width,
           r->h * tr.drawableSize.height);
}

void R_RevertSettings(void) {
    R_SetupViewport(&(RECT){0,0,1,1});
    R_SetupScissor(&(RECT){0,0,1,1});
}

#ifdef USE_SHADOWMAPS
void R_RenderShadowMap(void) {
    is_rendering_lights = true;
    R_SetupGL(true);
    R_BindTexture(tr.texture[TEX_SHADOWMAP], 1);
    R_DrawWorld();
    R_DrawEntities();
}
#endif

void R_RenderView(void) {
#ifdef USE_SHADOWMAPS
    is_rendering_lights = false;
#endif
    R_SetupViewport(&tr.viewDef.viewport);
    R_SetupScissor(&tr.viewDef.scissor);
    R_SetupGL(false);
    if (tr.viewDef.rdflags & RDF_NOWORLDMODEL) {
        R_Call(glClear, GL_DEPTH_BUFFER_BIT);
    }
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
#ifdef USE_SHADOWMAPS
    R_RenderShadowMap();
#endif
    R_RenderView();
    R_RenderOverlays();
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

float GetAccurateHeightAtPoint(float sx, float sy);

typedef struct {
    LPMODEL model;
    DWORD count;
    char paths[256][512];
} model_texture_cache_t;

static model_texture_cache_t model_texture_cache = { 0 };

static void R_TextureCacheAdd(LPCSTR path) {
    if (!path || !*path || model_texture_cache.count >= 256) {
        return;
    }
    FOR_LOOP(i, model_texture_cache.count) {
        if (!strcmp(model_texture_cache.paths[i], path)) {
            return;
        }
    }
    strncpy(model_texture_cache.paths[model_texture_cache.count], path, sizeof(model_texture_cache.paths[0]) - 1);
    model_texture_cache.paths[model_texture_cache.count][sizeof(model_texture_cache.paths[0]) - 1] = 0;
    model_texture_cache.count++;
}

static void R_BuildModelTextureCache(LPMODEL model) {
    if (model_texture_cache.model == model) {
        return;
    }
    model_texture_cache.model = model;
    model_texture_cache.count = 0;

    if (!model) {
        return;
    }

    switch (model->modeltype) {
        case ID_MDLX:
            if (model->mdx && model->mdx->textures) {
                FOR_LOOP(i, model->mdx->num_textures) {
                    R_TextureCacheAdd(model->mdx->textures[i].path);
                }
            }
            break;
        case ID_43DM:
            if (model->m3 && model->m3->materialStandard) {
                FOR_LOOP(i, model->m3->materialStandardNum) {
                    m3Material_t const *material = &model->m3->materialStandard[i];
                    m3Layer_t const *layers[] = {
                        material->diffuseLayer,
                        material->decalLayer,
                        material->specularLayer,
                        material->glossLayer,
                        material->emissiveLayer,
                        material->emissive2Layer,
                        material->evioLayer,
                        material->evioMaskLayer,
                        material->alphaMaskLayer,
                        material->alphaMask2Layer,
                        material->normalLayer,
                        material->heightLayer,
                        material->lightMapLayer,
                        material->ambientOcclusionLayer,
                    };
                    FOR_LOOP(layerIndex, sizeof(layers) / sizeof(layers[0])) {
                        m3Layer_t const *layer = layers[layerIndex];
                        if (layer && layer->imagePath && *layer->imagePath) {
                            R_TextureCacheAdd(layer->imagePath);
                        }
                    }
                }
            }
            break;
        default:
            break;
    }
}

bool R_GetModelInfo(LPMODEL model, LPMODELINFO info) {
    if (!model || !info) {
        return false;
    }

    memset(info, 0, sizeof(*info));

    R_BuildModelTextureCache(model);
    if (model_texture_cache.model == model) {
        info->textureCount = MIN(model_texture_cache.count, MODELINFO_MAX_TEXTURES);
        FOR_LOOP(i, info->textureCount) {
            info->texturePaths[i] = model_texture_cache.paths[i];
        }
    }

    if (model->modeltype == ID_MDLX && model->mdx) {
        bool found = false;
        float min_u = 1.0f, min_v = 1.0f, max_u = 0.0f, max_v = 0.0f;

        FOR_EACH_LIST(mdxGeoset_t, geoset, model->mdx->geosets) {
            if (!geoset->texcoord || geoset->num_texcoord <= 0) {
                continue;
            }
            FOR_LOOP(i, geoset->num_texcoord) {
                float u = geoset->texcoord[i].x;
                float v = geoset->texcoord[i].y;
                if (!found) {
                    min_u = max_u = u;
                    min_v = max_v = v;
                    found = true;
                } else {
                    if (u < min_u) min_u = u;
                    if (u > max_u) max_u = u;
                    if (v < min_v) min_v = v;
                    if (v > max_v) max_v = v;
                }
            }
        }

        if (found) {
            info->textureUVRect.x = min_u;
            info->textureUVRect.y = min_v;
            info->textureUVRect.w = max_u - min_u;
            info->textureUVRect.h = max_v - min_v;
            info->hasTextureUVRect = true;
        }
    }

    return true;
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
        .DrawImageEx = R_DrawImageEx,
        .DrawSelectionRect = R_DrawSelectionRect,
        .PrintSysText = R_PrintSysText,
        .GetWindowSize = R_GetWindowSize,
        .GetTextureSize = R_GetTextureSize,
        .DrawPortrait = R_DrawPortrait,
        .DrawSprite = R_DrawSprite,
        .DrawText = R_DrawText,
        .GetTextSize = R_GetTextSize,
        .GetModelInfo = R_GetModelInfo,
        .DrawBoundingBox = R_DrawBoundingBox,
        .GetHeightAtPoint = GetAccurateHeightAtPoint,
        .TraceEntity = R_TraceEntity,
        .TraceLocation = R_TraceLocation,
        .EntitiesInRect = R_EntitiesInRect,
#ifdef DEBUG_PATHFINDING
        .SetPathTexture = R_SetPathTexture,
#endif
    };
}
