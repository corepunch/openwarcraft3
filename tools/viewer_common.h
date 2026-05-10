#ifndef viewer_common_h
#define viewer_common_h

#include "../common/common.h"
#include <SDL2/SDL.h>

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

#endif
