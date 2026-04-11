#include "../client/client.h"
#include "../server/server.h"

#include <SDL2/SDL.h>

#define USAGE \
"Usage:\n" \
"  openwarcraft3 -mpq=<path> -map=<map>          (listen server + local client)\n" \
"  openwarcraft3 -mpq=<path> -connect=<host>     (remote client, default port " \
                                                    __XSTR(PORT_SERVER) ")\n" \
"  openwarcraft3 -mpq=<path> -connect=<host:port>\n" \
"\n" \
"Examples:\n" \
"  openwarcraft3 -mpq=/home/user/War3.mpq -map=Maps\\\\Campaign\\\\Human02.w3m\n" \
"  openwarcraft3 -mpq=/home/user/War3.mpq -connect=192.168.1.10\n" \
"\n" \
"Notes:\n" \
"  - The MPQ path must be an absolute path on your filesystem.\n" \
"  - The map path uses the internal MPQ path format.\n" \
"  - Remote clients still need the MPQ for asset loading.\n"

#define __XSTR(x) __STR(x)
#define __STR(x) #x

extern LPTEXTURE Texture;

HANDLE FS_AddArchive(LPCSTR);

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, LPSTR argv[]) {
    LPCSTR map = NULL;
    LPCSTR connect_addr = NULL;
    BOOL mpq = 0;

    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "-mpq=", 5)) {
            FS_AddArchive(argv[i] + 5);
            mpq = 1;
        }
        if (!strncmp(argv[i], "-map=", 5)) {
            map = argv[i] + 5;
        }
        if (!strncmp(argv[i], "-connect=", 9)) {
            connect_addr = argv[i] + 9;
        }
    }

    if (!mpq || (!map && !connect_addr)) {
        printf(USAGE);
        return 1;
    }

    // Bind the UDP socket.  The listen server uses PORT_SERVER so remote
    // clients know where to send; a pure client binds to 0 (OS picks a
    // free ephemeral port).
    unsigned short udp_port = connect_addr ? 0 : PORT_SERVER;
    if (!NET_Init(udp_port)) {
        fprintf(stderr, "NET_Init failed\n");
        return 1;
    }

    Com_Init();

    if (connect_addr) {
        // Remote-client mode: skip the local server, connect over UDP.
        CL_Connect(connect_addr, PORT_SERVER);
    } else {
        // Listen-server mode: load the map and spawn entities.
        SV_Map(map);
    }

    DWORD startTime = SDL_GetTicks();
    while (true) {
        DWORD currentTime = SDL_GetTicks();
        DWORD msec = currentTime - startTime;
        if (!connect_addr) {
            SV_Frame(msec);
        }
        CL_Frame(msec);
        startTime = currentTime;
    }

    return 0;
}
