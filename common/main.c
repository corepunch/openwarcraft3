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
            if (!FS_AddArchive(argv[i] + 5)) {
                fprintf(stderr, "Failed to open MPQ archive: %s\n", argv[i] + 5);
                return 1;
            }
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

    fprintf(stderr, "main: mpq loaded, mode=%s\n", connect_addr ? "connect" : "map");

    // Bind the UDP socket unless explicitly disabled for local diagnostics.
    // The current sandbox cannot create/bind UDP sockets, so this escape hatch
    // lets us exercise the map/UI startup path anyway.
    if (!getenv("OW3_SKIP_NET")) {
        unsigned short udp_port = connect_addr ? 0 : PORT_SERVER;
        if (!NET_Init(udp_port)) {
            if (!connect_addr) {
                fprintf(stderr, "main: retrying NET_Init on ephemeral port for local smoke test\n");
                if (!NET_Init(0)) {
                    fprintf(stderr, "NET_Init failed\n");
                    return 1;
                }
            } else {
                fprintf(stderr, "NET_Init failed\n");
                return 1;
            }
        }
    } else {
        fprintf(stderr, "main: OW3_SKIP_NET set, skipping NET_Init\n");
    }

    Com_Init();

    if (connect_addr) {
        // Remote-client mode: skip the local server, connect over UDP.
        fprintf(stderr, "main: connecting to %s\n", connect_addr);
        CL_Connect(connect_addr, PORT_SERVER);
    } else {
        // Listen-server mode: load the map and spawn entities.
        TRACE(SV_Map, map);
        fprintf(stderr, "main: map load complete\n");
    }

    DWORD startTime = SDL_GetTicks();
    fprintf(stderr, "main: entering frame loop\n");
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
