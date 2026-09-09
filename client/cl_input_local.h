#ifndef cl_input_local_h
#define cl_input_local_h

#include "client.h"

#include <SDL2/SDL.h>

BOOL CL_MouseOverGameplayUI(void);
BOOL CL_GameplayInputReady(void);
void CL_SetCameraPosition(VECTOR2 position);

void CL_InputModeInit(void);
void CL_InputModeResetMap(void);
void CL_InputModeMouseMotion(SDL_MouseMotionEvent const *motion);
void CL_InputModeFrame(void);
/* +select/-select. Return true to skip the generic RTS box/point path. */
BOOL CL_InputModeSelectDown(void);
BOOL CL_InputModeSelectUp(void);

/* Both interaction mechanisms are compiled for every game; config selects dispatch at startup. */
BOOL CL_InputOrbit(void);
void CL_ApplySelection(DWORD const *ids, DWORD n);
void CL_RtsInit(void);
void CL_RtsResetMap(void);
void CL_RtsFrame(void);
void CL_RtsMouseMotion(SDL_MouseMotionEvent const *motion);
void CL_OrbitInit(void);
void CL_OrbitResetMap(void);
void CL_OrbitFrame(void);
void CL_OrbitMouseMotion(SDL_MouseMotionEvent const *motion);
BOOL CL_OrbitSelectDown(void);
BOOL CL_OrbitSelectUp(void);

/* Minimap click-to-move-camera. Returns true if the click was on the minimap
 * (and the camera was recentered). No-op / false outside RTS input mode. */
BOOL CL_TryMinimapClick(float x, float y);
void CL_UpdateMinimapDrag(float x, float y);
void CL_EndMinimapDrag(void);
BOOL CL_MinimapKeyEvent(int key, BOOL repeat);

#endif
