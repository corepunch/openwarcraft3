#include "../client/client.h"
#include "../server/server.h"

#include <SDL2/SDL.h>

#define USAGE \
"Usage:\n" \
"  openwarcraft3 -mpq=<full path to MPQ file> -map=<path to map inside MPQ>\n"\
"\n" \
"Example:\n" \
"  openwarcraft3 -mpq=/Users/John/War3.mpq -map=Maps\\Campaign\\Human02.w3m\n" \
"\n" \
"Notes:\n" \
"  - The MPQ path must be an absolute path on your filesystem.\n" \
"  - The map path must use the internal path format from the MPQ.\n"

extern LPTEXTURE Texture;

HANDLE FS_AddArchive(LPCSTR);

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, LPSTR argv[]) {
    LPCSTR map = NULL;
    BOOL mpq = 0;
    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "-mpq=", 5)) {
            FS_AddArchive(argv[i]+5);
            mpq = 1;
        }
        if (!strncmp(argv[i], "-map=", 5)) {
            map = argv[i]+5;
        }
    }
    
    if (!mpq || !map) {
        printf(USAGE);
        return 1;
    }
    
    Com_Init();
    SV_Map(map);
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
