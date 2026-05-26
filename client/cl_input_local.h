#ifndef cl_input_local_h
#define cl_input_local_h

#include "client.h"

#include <SDL2/SDL.h>

BOOL CL_MouseOverGameplayUI(void);

void CL_InputModeInit(void);
void CL_InputModeSetGameplay(void);
void CL_InputModeMouseMotion(SDL_MouseMotionEvent const *motion);
BOOL CL_InputModeMouseWheel(SDL_MouseWheelEvent const *wheel);
void CL_InputModeFrame(void);

#endif
