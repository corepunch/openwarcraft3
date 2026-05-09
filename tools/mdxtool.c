#include "renderer/mdx/r_mdx.h"
#include "common/mpq.h"

#include <SDL2/SDL.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef PATHSTR
#define PATHSTR char[512]
#endif

static const char *g_model_path = NULL;
static bool g_use_model_camera = false;

static HANDLE archives[64] = { 0 };

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

static HANDLE FS_AddArchive(LPCSTR filename) {
    for (size_t i = 0; i < sizeof(archives) / sizeof(archives[0]); i++) {
        if (archives[i]) {
            continue;
        }
        if (!SFileOpenArchive(filename, 0, 0, &archives[i])) {
            fprintf(stderr, "Can't add archive %s\n", filename);
            return NULL;
        }
        return archives[i];
    }
    fprintf(stderr, "Too many archives\n");
    return NULL;
}

HANDLE FS_OpenFile(LPCSTR fileName) {
    if (!fileName || !*fileName) {
        return NULL;
    }
    for (size_t i = 0; i < sizeof(archives) / sizeof(archives[0]); i++) {
        HANDLE file = NULL;
        if (archives[i] && SFileOpenFileEx(archives[i], fileName, SFILE_OPEN_FROM_MPQ, &file)) {
            return file;
        }
    }
    return NULL;
}

void FS_CloseFile(HANDLE file) {
    if (file) {
        SFileCloseFile(file);
    }
}

bool FS_ExtractFile(LPCSTR toExtract, LPCSTR extracted) {
    for (size_t i = 0; i < sizeof(archives) / sizeof(archives[0]); i++) {
        if (archives[i] && SFileExtractFile(archives[i], toExtract, extracted, 0)) {
            return true;
        }
    }
    return false;
}

bool FS_FileExists(LPCSTR fileName) {
    HANDLE file = FS_OpenFile(fileName);
    if (!file) {
        return false;
    }
    FS_CloseFile(file);
    return true;
}

HANDLE FS_ReadFile(LPCSTR filename, LPDWORD size);

HANDLE MemAlloc(long size) {
    void *mem = calloc(1, (size_t)size);
    if (!mem) {
        fprintf(stderr, "Out of memory allocating %ld bytes\n", size);
        exit(1);
    }
    return mem;
}

void MemFree(HANDLE mem) {
    free(mem);
}

bool Com_InMenuMode(void) {
    return true;
}

void Com_SetMenuMode(bool enabled) {
    (void)enabled;
}

void Sys_Quit(void) {
    exit(0);
}

static VECTOR3 g_light_angles = { -40, 0, 60 };

static void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, FLOAT distance, LPMATRIX4 output) {
    VECTOR3 const vieworg = Vector3_unm(target);
    Matrix4_identity(output);
    Matrix4_translate(output, &(VECTOR3){0, 0, -distance});
    Matrix4_rotate(output, angles, ROTATE_ZYX);
    Matrix4_translate(output, &vieworg);
}

static void Matrix4_getPreviewCameraMatrix(LPCVECTOR3 target, float aspect, LPMATRIX4 output) {
    MATRIX4 proj, view;
    VECTOR3 eye = { 520.0f, -420.0f, 220.0f };
    VECTOR3 dir = Vector3_sub(target, &eye);

    Matrix4_perspective(&proj, 35.0f, aspect, 10.0f, 4000.0f);
    Matrix4_lookAt(&view, &eye, &dir, &(VECTOR3){ 0, 0, 1 });
    Matrix4_multiply(&proj, &view, output);
}

static void Matrix4_getPreviewLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view;
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, -1000.0, 3000.0);
    Matrix4_fromViewAngles(target, sunangles, 1000, &view);
    Matrix4_multiply(&proj, &view, output);
}

static void Matrix4_getLightMatrix(LPCVECTOR3 sunangles, LPCVECTOR3 target, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view;
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, 100.0, 3500.0);
    Matrix4_fromViewAngles(target, sunangles, 1000, &view);
    Matrix4_multiply(&proj, &view, output);
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
        Matrix4_getLightMatrix(&g_light_angles, &root, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);
    } else {
        target = mdx ? Box3_Center(&mdx->bounds.box) : (VECTOR3){ 0, 0, 0 };
        Matrix4_getPreviewCameraMatrix(&target, aspect, &viewdef.viewProjectionMatrix);
        Matrix4_getPreviewLightMatrix(&g_light_angles, &target, PORTRAIT_SHADOW_SIZE, &viewdef.lightMatrix);
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

    if (!FS_AddArchive(mpq)) {
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
    fprintf(stderr, "Sequences: %d, cameras: %p, pivots: %d\n",
            model->mdx->num_sequences,
            (void *)model->mdx->cameras,
            model->mdx->num_pivots);

    bool running = true;
    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                running = false;
            } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
                running = false;
            }
        }

        DWORD now = SDL_GetTicks();
        RenderModelFrame(&re, model, now, g_use_model_camera);
    }

    re.ReleaseModel(model);
    re.Shutdown();

    for (size_t i = 0; i < sizeof(archives) / sizeof(archives[0]); i++) {
        if (archives[i]) {
            SFileCloseArchive(archives[i]);
        }
    }

    return 0;
}
