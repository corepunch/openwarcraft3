#include "viewer_common.h"
#include "../common/mpq.h"

#include <math.h>
#include <stdlib.h>

HANDLE Viewer_AddArchive(HANDLE *archives, size_t count, LPCSTR filename) {
    for (size_t i = 0; i < count; i++) {
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

HANDLE Viewer_OpenFile(HANDLE const *archives, size_t count, LPCSTR fileName) {
    if (!fileName || !*fileName) {
        return NULL;
    }
    for (size_t i = 0; i < count; i++) {
        HANDLE file = NULL;
        if (archives[i] && SFileOpenFileEx(archives[i], fileName, SFILE_OPEN_FROM_MPQ, &file)) {
            return file;
        }
    }
    return NULL;
}

void Viewer_CloseFile(HANDLE file) {
    if (file) {
        SFileCloseFile(file);
    }
}

bool Viewer_ExtractFile(HANDLE const *archives, size_t count, LPCSTR toExtract, LPCSTR extracted) {
    for (size_t i = 0; i < count; i++) {
        if (archives[i] && SFileExtractFile(archives[i], toExtract, extracted, 0)) {
            return true;
        }
    }
    return false;
}

bool Viewer_FileExists(HANDLE const *archives, size_t count, LPCSTR fileName) {
    HANDLE file = Viewer_OpenFile(archives, count, fileName);
    if (!file) {
        return false;
    }
    Viewer_CloseFile(file);
    return true;
}

void Viewer_CloseArchives(HANDLE *archives, size_t count) {
    for (size_t i = 0; i < count; i++) {
        if (archives[i]) {
            SFileCloseArchive(archives[i]);
            archives[i] = NULL;
        }
    }
}

HANDLE Viewer_MemAlloc(long size) {
    void *mem = calloc(1, (size_t)size);
    if (!mem) {
        fprintf(stderr, "Out of memory allocating %ld bytes\n", size);
        exit(1);
    }
    return mem;
}

void Viewer_MemFree(HANDLE mem) {
    free(mem);
}

static VECTOR3 Viewer_OrbitEye(viewer_orbit_t const *orbit) {
    float const yaw = orbit->yaw_deg * (float)M_PI / 180.0f;
    float const pitch = orbit->pitch_deg * (float)M_PI / 180.0f;
    float const cp = cosf(pitch);
    float const sp = sinf(pitch);
    float const cy = cosf(yaw);
    float const sy = sinf(yaw);
    return (VECTOR3) {
        orbit->target.x + orbit->distance * cp * cy,
        orbit->target.y + orbit->distance * cp * sy,
        orbit->target.z + orbit->distance * sp,
    };
}

static void Viewer_OrbitClamp(viewer_orbit_t *orbit) {
    orbit->pitch_deg = MAX(-89.0f, MIN(89.0f, orbit->pitch_deg));
    orbit->distance = MAX(40.0f, MIN(20000.0f, orbit->distance));
}

bool Viewer_OrbitHandleEvent(viewer_orbit_t *orbit, SDL_Event const *event) {
    switch (event->type) {
        case SDL_MOUSEBUTTONDOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                orbit->dragging = true;
            }
            break;
        case SDL_MOUSEBUTTONUP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                orbit->dragging = false;
            }
            break;
        case SDL_MOUSEMOTION:
            if (orbit->dragging) {
                float const direction = orbit->reverse_drag ? -1.0f : 1.0f;
                orbit->yaw_deg += (float)event->motion.xrel * 0.25f * direction;
                orbit->pitch_deg -= (float)event->motion.yrel * 0.25f * direction;
                Viewer_OrbitClamp(orbit);
                return true;
            }
            break;
        case SDL_MOUSEWHEEL:
            if (event->wheel.y != 0) {
                float const factor = powf(0.85f, (float)(-event->wheel.y));
                orbit->distance *= factor;
                Viewer_OrbitClamp(orbit);
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

void Viewer_OrbitInit(viewer_orbit_t *orbit, VECTOR3 target, float distance, float yaw_deg, float pitch_deg) {
    *orbit = (viewer_orbit_t) {
        .target = target,
        .yaw_deg = yaw_deg,
        .pitch_deg = pitch_deg,
        .distance = distance,
        .dragging = false,
        .reverse_drag = false,
    };
    Viewer_OrbitClamp(orbit);
}

void Viewer_OrbitBuildCamera(viewer_orbit_t const *orbit, float aspect, float fov, float znear, float zfar, LPMATRIX4 output) {
    MATRIX4 proj, view;
    VECTOR3 eye = Viewer_OrbitEye(orbit);
    VECTOR3 dir = Vector3_sub(&orbit->target, &eye);
    Matrix4_perspective(&proj, fov, aspect, znear, zfar);
    Matrix4_lookAt(&view, &eye, &dir, &(VECTOR3){ 0, 0, 1 });
    Matrix4_multiply(&proj, &view, output);
}

void Viewer_OrbitBuildLight(viewer_orbit_t const *orbit, LPCVECTOR3 sunangles, float scale, LPMATRIX4 output) {
    MATRIX4 proj, view;
    VECTOR3 eye = Viewer_OrbitEye(orbit);
    VECTOR3 dir = Vector3_sub(&orbit->target, &eye);
    Matrix4_ortho(&proj, -scale, scale, -scale, scale, -1000.0f, 3000.0f);
    Matrix4_lookAt(&view, &eye, &dir, &(VECTOR3){ 0, 0, 1 });
    Matrix4_multiply(&proj, &view, output);
    (void)sunangles;
}
