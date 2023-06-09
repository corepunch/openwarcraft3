#include "../client/client.h"
#include "../server/server.h"

#include "SDL.h"

extern LPTEXTURE Texture;

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, LPSTR argv[]) {
    Com_Init();
    
    SV_Map("Maps\\Campaign\\Human02.w3m");
    
    DWORD startTime = SDL_GetTicks();
    
    while (true) {
        DWORD currentTime = SDL_GetTicks();
        DWORD msec = currentTime - startTime;
        SV_Frame(msec);
        CL_Frame(msec);
        startTime = currentTime;
    }

    return 0;
}
