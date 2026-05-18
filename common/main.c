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
"  openwarcraft3 -mpq=<path> -map=<map>          (listen server + local client)\n" \
"  openwarcraft3 -mpq=<path>                    (client menu)\n" \
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
void UI_Init(void);

HANDLE FS_AddArchive(LPCSTR);

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, LPSTR argv[]) {
    LPCSTR map = NULL;
    LPCSTR connect_addr = NULL;
    BOOL mpq = 0;

    fprintf(stderr,
            "\nOpenWarcraft3\n"
            "Platform: %s\n"
            "Architecture: %s\n"
            "Byte ordering: %s\n\n",
            OW3_PLATFORM,
            OW3_ARCH,
            OW3_BYTE_ORDER);

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

    if (!mpq) {
        printf(USAGE);
        return 1;
    }

    bool menu_mode = !map && !connect_addr;
    cls.key_dest = menu_mode ? key_menu : key_game;
    cls.state = ca_disconnected;

    // Local client/server traffic uses the in-process loopback channel. Only a
    // remote client needs a UDP socket here; local menu/map servers do not.
    if (connect_addr && !getenv("OW3_SKIP_NET")) {
        if (!NET_Init(0)) {
            fprintf(stderr, "NET_Init failed\n");
            return 1;
        }
    } else if (getenv("OW3_SKIP_NET")) {
        fprintf(stderr, "main: OW3_SKIP_NET set, skipping NET_Init\n");
    }

    Com_Init();

    if (connect_addr) {
        // Remote-client mode: skip the local server, connect over UDP.
        CL_Connect(connect_addr, PORT_SERVER);
    } else if (menu_mode) {
        // Menu mode still uses the local loopback server so the client can
        // consume server-authored UI layouts.
        UI_Init();
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
