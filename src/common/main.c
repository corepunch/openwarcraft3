#include "../client/client.h"
#include "../server/server.h"

#include "SDL.h"

extern struct texture *Texture;

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, char * argv[]) {
    FS_Init();
    SV_Init();
    CL_Init();
    SV_Map("Maps\\Campaign\\Human01.w3m");
    
    
    uint32_t startTime = SDL_GetTicks();
    
    while (true) {
        uint32_t currentTime = SDL_GetTicks();
        uint32_t msec = currentTime - startTime;
        SV_Frame(msec);
        CL_Frame(msec);
        startTime = currentTime;
    }

    return 0;
}
