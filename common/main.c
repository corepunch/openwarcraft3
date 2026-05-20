#include "../client/client.h"
#include "../server/server.h"

#include <SDL2/SDL.h>
#include <stdlib.h>

#if defined(__APPLE__)
#define OW3_PLATFORM "Darwin"
#elif defined(_WIN32)
#define OW3_PLATFORM "Windows"
#elif defined(__linux__)
#define OW3_PLATFORM "Linux"
#else
#define OW3_PLATFORM "Unknown"
#endif

#if defined(__aarch64__) || defined(_M_ARM64)
#define OW3_ARCH "arm64"
#elif defined(__x86_64__) || defined(_M_X64)
#define OW3_ARCH "x86_64"
#elif defined(__i386__) || defined(_M_IX86)
#define OW3_ARCH "x86"
#else
#define OW3_ARCH "unknown"
#endif

#if __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#define OW3_BYTE_ORDER "big endian"
#else
#define OW3_BYTE_ORDER "little endian"
#endif

#define USAGE \
"Usage:\n" \
"  openwarcraft3 -data=<folder> -map=<map>       (listen server + local client)\n" \
"  openwarcraft3 -data=<folder>                 (client menu)\n" \
"  openwarcraft3 -data=<folder> -connect=<host>  (remote client, default port " \
                                                    __XSTR(PORT_SERVER) ")\n" \
"  openwarcraft3 -data=<folder> -connect=<host:port>\n" \
"\n" \
"Examples:\n" \
"  openwarcraft3 -data=/home/user/Warcraft3 -map=Maps\\\\Campaign\\\\Human02.w3m\n" \
"  openwarcraft3 -data=/home/user/Warcraft3 -connect=192.168.1.10\n" \
"\n" \
"Notes:\n" \
"  - The data folder should contain Warcraft III MPQs and optionally Maps/.\n" \
"  - The map path uses the internal MPQ path format.\n" \
"  - Remote clients still need the game data for asset loading.\n"

#define __XSTR(x) __STR(x)
#define __STR(x) #x

extern LPTEXTURE Texture;

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, LPSTR argv[]) {
    LPCSTR map = NULL;
    LPCSTR connect_addr = NULL;
    BOOL data = 0;

    fprintf(stderr,
            "\nOpenWarcraft3\n"
            "Platform: %s\n"
            "Architecture: %s\n"
            "Byte ordering: %s\n\n",
            OW3_PLATFORM,
            OW3_ARCH,
            OW3_BYTE_ORDER);

    for (int i = 0; i < argc; i++) {
        if (!strncmp(argv[i], "-data=", 6)) {
            if (!FS_AddDataDirectory(argv[i] + 6)) {
                fprintf(stderr, "Failed to add data directory: %s\n", argv[i] + 6);
                return 1;
            }
            data = 1;
        }
        if (!strncmp(argv[i], "-map=", 5)) {
            map = argv[i] + 5;
        }
        if (!strncmp(argv[i], "-connect=", 9)) {
            connect_addr = argv[i] + 9;
        }
    }

    if (!data) {
        printf(USAGE);
        return 1;
    }

    bool menu_mode = !map && !connect_addr;
    cls.key_dest = menu_mode ? key_menu : key_game;
    cls.state = ca_disconnected;

    unsigned short port = connect_addr ? 0 : PORT_SERVER;
    if (!NET_Init(port)) {
        fprintf(stderr, "NET_Init failed\n");
        return 1;
    }

    Com_Init();

    if (connect_addr) {
        // Remote-client mode: skip the local server, connect over UDP.
        CL_Connect(connect_addr, PORT_SERVER);
    } else if (menu_mode) {
        // Menu mode: client-side UI library handles menu rendering (Phase 4)
        SV_ClientConnect();
    } else if (!menu_mode) {
        // Listen-server mode: load the map and spawn entities.
        SV_Map(map);
    }

    fprintf(stderr, "OpenWarcraft3 initialized.\n\n");

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
