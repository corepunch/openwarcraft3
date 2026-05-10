#include "renderer/mdx/r_mdx.h"
#include "tools/viewer_common.h"

#include <stdarg.h>
#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATHSTR
#define PATHSTR char[512]
#endif

static const char *g_model_path = NULL;
static bool g_use_model_camera = false;
static HANDLE archives[64] = { 0 };
static viewer_orbit_t orbit;
static VECTOR3 g_model_center = { 0, 0, 0 };

#define MAX_TEXTURE_PREVIEWS 12

typedef struct {
    DWORD count;
    char paths[MAX_TEXTURE_PREVIEWS][512];
    LPCTEXTURE textures[MAX_TEXTURE_PREVIEWS];
    size2_t sizes[MAX_TEXTURE_PREVIEWS];
} texture_preview_cache_t;

static texture_preview_cache_t texture_previews = { 0 };

static void usage(void) {
    fprintf(stderr,
        "Usage:\n"
        "  mdxtool -mpq <archive.mpq> -model <file.mdx> [--use-model-camera]\n"
        "\n"
        "Examples:\n"
        "  mdxtool -mpq War3.mpq -model UI\\\\Glues\\\\MainMenu\\\\MainMenu3d\\\\MainMenu3d.mdx\n"
        "  mdxtool -mpq War3.mpq -model units\\\\orc\\\\Peon\\\\Peon.mdx --use-model-camera\n");
}

static void errorf(LPCSTR fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}

HANDLE FS_OpenFile(LPCSTR fileName) {
    return Viewer_OpenFile(archives, sizeof(archives) / sizeof(archives[0]), fileName);
}

void FS_CloseFile(HANDLE file) {
    Viewer_CloseFile(file);
}

bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted) {
    return Viewer_ExtractFile(archives, sizeof(archives) / sizeof(archives[0]), toExtract, extracted);
}

bool FS_FileExists(LPCSTR fileName) {
    return Viewer_FileExists(archives, sizeof(archives) / sizeof(archives[0]), fileName);
}

HANDLE FS_ReadFile(LPCSTR filename, LPDWORD size);

HANDLE MemAlloc(long size) { return Viewer_MemAlloc(size); }
void MemFree(HANDLE mem) { Viewer_MemFree(mem); }

bool Com_InMenuMode(void) {
    return true;
}

void Com_SetMenuMode(bool enabled) {
    (void)enabled;
}

void Sys_Quit(void) {
    exit(0);
}

static void Matrix4_getPreviewCameraMatrix(viewer_orbit_t const *orbit, float aspect, LPMATRIX4 output) {
    Viewer_OrbitBuildCamera(orbit, aspect, 35.0f, 10.0f, 4000.0f, output);
}

static void Matrix4_getSideLightMatrix(LPCVECTOR3 eye, LPCVECTOR3 target, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view;
    VECTOR3 forward = Vector3_sub(target, eye);
    VECTOR3 right = Vector3_cross(&forward, &(VECTOR3){ 0, 0, 1 });
    VECTOR3 lightEye;
    VECTOR3 lightOffset;
    VECTOR3 lightDir;
    VECTOR3 lightUp = { 0, 0, 1 };

    if (Vector3_len(&right) < 0.001f) {
        right = (VECTOR3){ 1, 0, 0 };
    } else {
        Vector3_normalize(&right);
    }

    lightOffset = Vector3_mad(&right, -1000.0f, &(VECTOR3){ 0, 0, 0 });
    lightOffset.z += 300.0f;
    lightEye = Vector3_add(eye, &lightOffset);
    lightDir = Vector3_sub(target, &lightEye);

    Matrix4_ortho(&proj, -scale, scale, -scale, scale, -1000.0, 3000.0);
    Matrix4_lookAt(&view, &lightEye, &lightDir, &lightUp);
    Matrix4_multiply(&proj, &view, output);
}

static float FitPreviewDistance(mdxModel_t const *mdx, float aspect, float fov_deg) {
    if (!mdx) {
        return 850.0f;
    }

    BOX3 const *box = &mdx->bounds.box;
    float const width = fabsf(box->max.x - box->min.x);
    float const depth = fabsf(box->max.y - box->min.y);
    float const height = fabsf(box->max.z - box->min.z);
    float const halfWidth = MAX(width, depth) * 0.5f;
    float const halfHeight = MAX(height * 0.5f, 1.0f);
    float const fill = 0.70f;
    float const vfov = fov_deg * (float)M_PI / 180.0f;
    float const hfov = 2.0f * atanf(tanf(vfov * 0.5f) * aspect);
    float const verticalDistance = halfHeight / (fill * tanf(vfov * 0.5f));
    float const horizontalDistance = halfWidth / (fill * tanf(hfov * 0.5f));
    float distance = MAX(verticalDistance, horizontalDistance);

    if (!isfinite(distance) || distance < 40.0f) {
        distance = 40.0f;
    }

    return distance * 1.05f;
}

static LPCSTR TextureBaseName(LPCSTR path) {
    LPCSTR base = path;
    if (!path) {
        return "";
    }
    for (LPCSTR s = path; *s; s++) {
        if (*s == '\\' || *s == '/') {
            base = s + 1;
        }
    }
    return base;
}

static void BuildTexturePreviewCache(refExport_t const *re, LPMODEL model) {
    texture_previews.count = 0;
    memset(texture_previews.paths, 0, sizeof(texture_previews.paths));
    memset(texture_previews.textures, 0, sizeof(texture_previews.textures));
    memset(texture_previews.sizes, 0, sizeof(texture_previews.sizes));

    DWORD textureCount = re->GetModelTextureCount ? re->GetModelTextureCount(model) : 0;
    for (DWORD i = 0; i < textureCount && texture_previews.count < MAX_TEXTURE_PREVIEWS; i++) {
        LPCSTR texturePath = re->GetModelTexturePath ? re->GetModelTexturePath(model, i) : NULL;
        if (!texturePath || !*texturePath) {
            continue;
        }

        strncpy(texture_previews.paths[texture_previews.count], texturePath, sizeof(texture_previews.paths[0]) - 1);
        texture_previews.paths[texture_previews.count][sizeof(texture_previews.paths[0]) - 1] = '\0';
        texture_previews.textures[texture_previews.count] = re->LoadTexture(texturePath);
        if (texture_previews.textures[texture_previews.count] && re->GetTextureSize) {
            texture_previews.sizes[texture_previews.count] = re->GetTextureSize(texture_previews.textures[texture_previews.count]);
        } else {
            texture_previews.sizes[texture_previews.count] = (size2_t){ 1, 1 };
        }
        texture_previews.count++;
    }
}

static void DrawTexturePreviews(refExport_t const *re) {
    if (!texture_previews.count) {
        return;
    }

    size2_t window = re->GetWindowSize();
    const float panelX = 0.60f;
    const float panelY = 0.06f;
    const float cellW = 0.085f;
    const float cellH = 0.11f;
    const float thumbW = 0.075f;
    const float thumbH = 0.070f;
    const DWORD cols = 2;

    for (DWORD i = 0; i < texture_previews.count; i++) {
        LPCTEXTURE texture = texture_previews.textures[i];
        if (!texture) {
            continue;
        }

        DWORD col = i % cols;
        DWORD row = i / cols;
        float cellX = panelX + (float)col * cellW;
        float cellY = panelY + (float)row * cellH;
        size2_t texSize = texture_previews.sizes[i];
        float aspect = texSize.height ? (float)texSize.width / (float)texSize.height : 1.0f;
        float drawW = thumbW;
        float drawH = thumbH;

        if (aspect > 1.0f) {
            drawH = drawW / aspect;
        } else if (aspect > 0.0f) {
            drawW = drawH * aspect;
        }

        if (drawW > thumbW) {
            drawW = thumbW;
        }
        if (drawH > thumbH) {
            drawH = thumbH;
        }

        float drawX = cellX + (thumbW - drawW) * 0.5f;
        float drawY = cellY + (thumbH - drawH) * 0.5f;
        RECT screen = { drawX, drawY, drawW, drawH };
        RECT uv = { 0, 0, 1, 1 };

        re->DrawImage(texture, &screen, &uv, COLOR32_WHITE);

        char line[512];
        snprintf(line, sizeof(line), "%s", TextureBaseName(texture_previews.paths[i]));
        DWORD textX = (DWORD)((drawX / 0.8f) * window.width);
        DWORD textY = (DWORD)(((drawY + drawH + 0.005f) / 0.6f) * window.height);
        re->PrintSysText(line, textX, textY, COLOR32_WHITE);
    }
}

static bool BuildModelCameraMatrix(mdxModel_t const *model, float aspect, LPMATRIX4 output, LPVECTOR3 root) {
    if (!model || !model->cameras) {
        return false;
    }
    MATRIX4 proj, view;
    mdxCamera_t const *camera = model->cameras;
    VECTOR3 dir = Vector3_sub(&camera->targetPivot, &camera->pivot);

    Matrix4_perspective(&proj, 30.0f, aspect, 10.0f, 1000.0f);
    Matrix4_lookAt(&view, &camera->pivot, &dir, &(VECTOR3){ 0, 0, 1 });
    Matrix4_multiply(&proj, &view, output);

    if (root) {
        if (model->pivots && model->num_pivots > 0) {
            *root = model->pivots[0];
        } else {
            *root = (VECTOR3){ 0, 0, 0 };
        }
    }
    return true;
}

static mdxSequence_t const *PickSequence(mdxModel_t const *mdx) {
    if (!mdx || !mdx->sequences || mdx->num_sequences <= 0) {
        return NULL;
    }
    if (mdx->num_sequences > 2) {
        return mdx->sequences + 2;
    }
    return mdx->sequences;
}

static void RenderModelFrame(refExport_t const *re, LPMODEL model, DWORD now, bool useModelCamera) {
    viewDef_t viewdef = { 0 };
    renderEntity_t entity = { 0 };
    mdxModel_t const *mdx = model->mdx;
    VECTOR3 target = { 0, 0, 0 };
    VECTOR3 root = { 0, 0, 0 };
    size2_t windowSize = re->GetWindowSize();
    mdxSequence_t const *seq = PickSequence(mdx);
    float aspect = windowSize.height ? (float)windowSize.width / (float)windowSize.height : 1.0f;

    entity.model = model;
    entity.scale = 1.0f;
    entity.frame = 0;
    entity.oldframe = 0;
    entity.origin = (VECTOR3){ 0, 0, 0 };

    if (seq && seq->interval[1] > seq->interval[0]) {
        DWORD duration = seq->interval[1] - seq->interval[0];
        entity.frame = seq->interval[0] + now % duration;
        entity.oldframe = entity.frame;
    }

    if (useModelCamera && BuildModelCameraMatrix(mdx, aspect, &viewdef.viewProjectionMatrix, &root)) {
        entity.origin = root;
        Matrix4_getSideLightMatrix(&model->mdx->cameras->pivot, &model->mdx->cameras->targetPivot, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);
    } else {
        target = g_model_center;
        orbit.target = g_model_center;
        Matrix4_getPreviewCameraMatrix(&orbit, aspect, &viewdef.viewProjectionMatrix);
        Matrix4_getSideLightMatrix(&(VECTOR3){
            orbit.target.x + orbit.distance * cosf(orbit.pitch_deg * (float)M_PI / 180.0f) * cosf(orbit.yaw_deg * (float)M_PI / 180.0f),
            orbit.target.y + orbit.distance * cosf(orbit.pitch_deg * (float)M_PI / 180.0f) * sinf(orbit.yaw_deg * (float)M_PI / 180.0f),
            orbit.target.z + orbit.distance * sinf(orbit.pitch_deg * (float)M_PI / 180.0f),
        }, &target, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);
    }

    viewdef.viewport = (RECT){ 0, 0, 1, 1 };
    viewdef.scissor = (RECT){ 0, 0, 1, 1 };
    viewdef.time = now;
    viewdef.deltaTime = 16;
    viewdef.lerpfrac = 0.0f;
    viewdef.num_entities = 1;
    viewdef.entities = &entity;
    viewdef.rdflags = RDF_NOWORLDMODEL | RDF_NOFRUSTUMCULL;
    Matrix4_identity(&viewdef.textureMatrix);

    re->BeginFrame();
    re->RenderFrame(&viewdef);
    re->PrintSysText(useModelCamera ? "mdxtool: model camera" : "mdxtool: preview camera", 10, 10, COLOR32_WHITE);
    re->PrintSysText(g_model_path, 10, 28, COLOR32_WHITE);
    DrawTexturePreviews(re);
    re->EndFrame();
}

int main(int argc, char **argv) {
    const char *mpq = NULL;
    const char *modelPath = NULL;

    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "--use-model-camera") || !strcmp(argv[i], "-use-model-camera")) {
            g_use_model_camera = true;
        } else if (!strncmp(argv[i], "-mpq=", 5)) {
            mpq = argv[i] + 5;
        } else if (!strcmp(argv[i], "-mpq")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            mpq = argv[i];
        } else if (!strncmp(argv[i], "-model=", 7)) {
            modelPath = argv[i] + 7;
        } else if (!strcmp(argv[i], "-model")) {
            if (++i >= argc) {
                usage();
                return 1;
            }
            modelPath = argv[i];
        } else if (argv[i][0] != '-' && !modelPath) {
            modelPath = argv[i];
        } else {
            usage();
            return 1;
        }
    }

    if (!mpq || !modelPath) {
        usage();
        return 1;
    }

    if (!Viewer_AddArchive(archives, sizeof(archives) / sizeof(archives[0]), mpq)) {
        return 1;
    }

    refExport_t re = R_GetAPI((refImport_t){
        .FileOpen = FS_OpenFile,
        .FileExtract = FS_ExtractFile,
        .FileClose = FS_CloseFile,
        .InMenuMode = Com_InMenuMode,
        .MemAlloc = MemAlloc,
        .MemFree = MemFree,
        .ReadSheet = FS_ParseSLK,
        .FindSheetCell = FS_FindSheetCell,
        .error = errorf,
    });

    Com_SetMenuMode(true);
    fprintf(stderr, "mdxtool: renderer init\n");
    re.Init(1280, 720);
    fprintf(stderr, "mdxtool: renderer ready\n");

    g_model_path = modelPath;
    fprintf(stderr, "mdxtool: loading model %s\n", modelPath);
    LPMODEL model = re.LoadModel(modelPath);
    if (!model || !model->mdx) {
        fprintf(stderr, "Failed to load MDX model: %s\n", modelPath);
        re.Shutdown();
        return 1;
    }

    fprintf(stderr, "mdxtool: loaded model %s\n", modelPath);
    g_model_center = Box3_Center(&model->mdx->bounds.box);
    fprintf(stderr, "Sequences: %d, cameras: %p, pivots: %d\n",
            model->mdx->num_sequences,
            (void *)model->mdx->cameras,
            model->mdx->num_pivots);

    BuildTexturePreviewCache(&re, model);

    size2_t window = re.GetWindowSize();
    float const aspect = window.height ? (float)window.width / (float)window.height : 1.0f;
    VECTOR3 orbit_target = g_model_center;
    float const orbit_distance = FitPreviewDistance(model->mdx, aspect, 35.0f);
    Viewer_OrbitInit(&orbit, orbit_target, orbit_distance, -45.0f, 20.0f);
    orbit.reverse_drag = true;

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            } else {
                Viewer_OrbitHandleEvent(&orbit, &event);
            }
        }

        DWORD now = SDL_GetTicks();
        RenderModelFrame(&re, model, now, g_use_model_camera);
    }

    re.ReleaseModel(model);
    re.Shutdown();

    Viewer_CloseArchives(archives, sizeof(archives) / sizeof(archives[0]));

    return 0;
}
