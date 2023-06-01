#include "client.h"

#include <SDL.h>

void CL_Input(void) {
    static int button;
    static int moved = false;
    SDL_Event event;
    while(SDL_PollEvent(&event))
    {
        VECTOR2 mouse = {
            .x = event.button.x * 0.8 / WINDOW_WIDTH,
            .y = event.button.y * 0.6 / WINDOW_HEIGHT
        };
        switch(event.type)
        {
            case SDL_KEYUP:
                if(event.key.keysym.sym == SDLK_ESCAPE)
                    return Com_Quit();
                break;
            case SDL_MOUSEBUTTONDOWN:
                button = event.button.button;
                moved = false;
                cl.selection.rect.x = event.button.x;
                cl.selection.rect.y = event.button.y;
                switch (event.button.button) {
                    case 1: ui.MouseEvent(&mouse, UI_LEFT_MOUSE_DOWN); break;
                    case 2: ui.MouseEvent(&mouse, UI_RIGHT_MOUSE_DOWN); break;
                }
                ui.MouseEvent(&mouse, event.button.button == 1 ? UI_LEFT_MOUSE_DOWN : UI_RIGHT_MOUSE_DOWN );
                break;
            case SDL_MOUSEBUTTONUP:
                button = 0;
                if (moved == false) {
                    CL_SelectEntityAtScreenPoint(event.button.x, event.button.y);
                } else if (cl.selection.inProgress) {
                    CL_SelectEntitiesAtScreenRect(&cl.selection.rect);
                }
                cl.selection.inProgress = false;
                switch (event.button.button) {
                    case 1: ui.MouseEvent(&mouse, UI_LEFT_MOUSE_UP); break;
                    case 2: ui.MouseEvent(&mouse, UI_RIGHT_MOUSE_UP); break;
                }
                break;
            case SDL_MOUSEMOTION:
                switch (button) {
                    case 3:
                        cl.selection.inProgress = true;
                        cl.selection.rect.width = event.button.x - cl.selection.rect.x;
                        cl.selection.rect.height = event.button.y - cl.selection.rect.y;
                        moved = true;
                        break;
                    case 1:
                        moved = true;
                        cl.viewDef.camera.target.x -= event.motion.xrel * 5;
                        cl.viewDef.camera.target.y += event.motion.yrel * 5;
                        break;
                }
                break;
            case SDL_WINDOWEVENT:
                switch (event.window.event) {
                    case SDL_WINDOWEVENT_CLOSE:   // exit game
                        return Com_Quit();
                    default:
                        break;
                }
                break;
        }
    }
    cl.viewDef.camera.target.z = CM_GetHeightAtPoint(cl.viewDef.camera.target.x, cl.viewDef.camera.target.y);
}

