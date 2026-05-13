#ifndef viewer_common_h
#define viewer_common_h

#include "tool_common.h"
#include <SDL2/SDL.h>
#include <math.h>

#define VIEWER_WINDOW_WIDTH 800
#define VIEWER_WINDOW_HEIGHT 600

typedef struct viewer_orbit_s {
    VECTOR3 target;
    float yaw_deg;
    float pitch_deg;
    float distance;
    bool dragging;
    bool reverse_drag;
} viewer_orbit_t;

HANDLE Viewer_AddArchive(HANDLE *archives, size_t count, LPCSTR filename);
HANDLE Viewer_OpenFile(HANDLE const *archives, size_t count, LPCSTR fileName);
void Viewer_CloseFile(HANDLE file);
bool Viewer_ExtractFile(HANDLE const *archives, size_t count, LPCSTR toExtract, LPCSTR extracted);
bool Viewer_FileExists(HANDLE const *archives, size_t count, LPCSTR fileName);
void Viewer_CloseArchives(HANDLE *archives, size_t count);

HANDLE Viewer_MemAlloc(long size);
void Viewer_MemFree(HANDLE mem);

bool Viewer_OrbitHandleEvent(viewer_orbit_t *orbit, SDL_Event const *event);
void Viewer_OrbitInit(viewer_orbit_t *orbit, VECTOR3 target, float distance, float yaw_deg, float pitch_deg);
void Viewer_OrbitBuildCamera(viewer_orbit_t const *orbit, float aspect, float fov, float znear, float zfar, LPMATRIX4 output);
void Viewer_OrbitBuildLight(viewer_orbit_t const *orbit, LPCVECTOR3 sunangles, float scale, LPMATRIX4 output);

#include "../common/parser.c"
#include "../common/sheet.c"

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

HANDLE Viewer_AddArchive(HANDLE *archives, size_t count, LPCSTR filename) {
    return Tool_AddArchive(archives, count, filename);
}

HANDLE Viewer_OpenFile(HANDLE const *archives, size_t count, LPCSTR fileName) {
    return Tool_OpenFile(archives, count, fileName);
}

void Viewer_CloseFile(HANDLE file) {
    Tool_CloseFile(file);
}

bool Viewer_ExtractFile(HANDLE const *archives, size_t count, LPCSTR toExtract, LPCSTR extracted) {
    return Tool_ExtractFile(archives, count, toExtract, extracted);
}

bool Viewer_FileExists(HANDLE const *archives, size_t count, LPCSTR fileName) {
    return Tool_FileExists(archives, count, fileName);
}

void Viewer_CloseArchives(HANDLE *archives, size_t count) {
    Tool_CloseArchives(archives, count);
}

HANDLE Viewer_MemAlloc(long size) {
    return Tool_MemAlloc(size);
}

void Viewer_MemFree(HANDLE mem) {
    Tool_MemFree(mem);
}

#endif
