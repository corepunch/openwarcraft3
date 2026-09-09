#ifndef cl_input_local_h
#define cl_input_local_h

#include "client.h"

#include <SDL2/SDL.h>

BOOL CL_MouseOverGameplayUI(void);
BOOL CL_GameplayInputReady(void);
void CL_SetCameraPosition(VECTOR2 position);

void CL_ResetInput(void);
DWORD CL_SelectionLimit(void);
void CL_ApplySelection(DWORD const *ids, DWORD n);

/* Minimap click-to-move-camera. Returns true if the click was on the minimap
 * (and the camera was recentered). No-op / false without a minimap. */
BOOL CL_TryMinimapClick(float x, float y);
void CL_UpdateMinimapDrag(float x, float y);
void CL_EndMinimapDrag(void);
BOOL CL_MinimapKeyEvent(int key, BOOL repeat);

#endif
