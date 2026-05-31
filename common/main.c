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
"  openwarcraft3 -data <folder> +map <map>       (listen server + local client)\n" \
"  openwarcraft3 -data <folder>                 (client menu)\n" \
"  openwarcraft3 -data <folder> -connect <host>  (remote client, default port " \
                                                    __XSTR(PORT_SERVER) ")\n" \
"  openwarcraft3 -data <folder> -connect <host:port>\n" \
"\n" \
"Examples:\n" \
"  openwarcraft3 -data /home/user/Warcraft3 +map Maps\\\\Campaign\\\\Human02.w3m\n" \
"  openwarcraft3 -data /home/user/Warcraft3 -connect 192.168.1.10\n" \
"\n" \
"Notes:\n" \
"  - The data folder should contain Warcraft III MPQs and optionally Maps/.\n" \
"  - The data folder may also be saved as data in the generated per-build config.\n" \
"  - The map path uses the internal MPQ path format; use +map to launch one.\n" \
"  - Remote clients still need the game data for asset loading.\n"

#define __XSTR(x) __STR(x)
#define __STR(x) #x

extern LPTEXTURE Texture;

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, LPSTR argv[]) {
    BOOL data = 0;

    fprintf(stderr,
            "\nOpenWarcraft3\n"
            "Platform: %s\n"
            "Architecture: %s\n"
            "Byte ordering: %s\n\n",
            OW3_PLATFORM,
            OW3_ARCH,
            OW3_BYTE_ORDER);

    Com_Init(argc, (LPCSTR *)argv);

    LPCSTR data_dir = Cvar_String("data", "");
    if (data_dir && *data_dir) {
        if (!FS_AddDataDirectory(data_dir)) {
            fprintf(stderr, "Failed to add data directory: %s\n", data_dir);
            return 1;
        }
        data = 1;
    }

    if (!data) {
        printf(USAGE);
        return 1;
    }

    PATHSTR resolved_map;
    LPCSTR map = Cvar_String("map", "");
    LPCSTR connect_addr = Cvar_String("connect", "");
    bool has_map = map && *map;
    bool has_connect_addr = connect_addr && *connect_addr;
    bool menu_mode = !has_map && !has_connect_addr;
    bool listen_server_mode = has_map && !has_connect_addr;
    bool net_enabled = Cvar_Integer("net_enabled", 1) != 0;

    if (has_map) {
        if (!Com_ResolveMapArgument(map, resolved_map, sizeof(resolved_map))) {
            return 1;
        }
        map = resolved_map;
    }
    cls.key_dest = menu_mode ? key_menu : key_game;
    cls.state = ca_disconnected;

    unsigned short port = has_connect_addr ? 0 : PORT_SERVER;
    if (has_connect_addr && !net_enabled) {
        fprintf(stderr, "Cannot connect with net_enabled disabled\n");
        return 1;
    }
    bool net_active = net_enabled && !menu_mode;
    if (net_active && !NET_Init(port)) {
        fprintf(stderr, "NET_Init failed\n");
        return 1;
    }

    SV_Init();
    CL_Init();
    Cbuf_AddLateCommands();
    Cbuf_Execute();

    if (has_connect_addr) {
        // Remote-client mode: skip the local server, connect over UDP.
        CL_Connect(connect_addr, PORT_SERVER);
    } else if (listen_server_mode) {
        // Listen-server mode: show the client loading screen before the
        // synchronous server map load, mirroring Quake's loading plaque flow.
        CL_BeginLoadingMap(map);
        SCR_UpdateScreen(0);
        SV_Map(map);
    }
    // Menu mode: UI runs client-side, no server connection needed (Quake 3 pattern)

    fprintf(stderr, "OpenWarcraft3 initialized.\n\n");

    DWORD startTime = SDL_GetTicks();
    DWORD frameCount = 0;
    while (true) {
        DWORD currentTime = SDL_GetTicks();
        DWORD msec = currentTime - startTime;
        if (!has_connect_addr && sv.state == ss_game) {
            SV_Frame(msec);
        }
        CL_Frame(msec);
        startTime = currentTime;
        frameCount++;
        if (Cvar_Integer("com_frame_limit", 0) > 0 &&
            frameCount >= (DWORD)Cvar_Integer("com_frame_limit", 0)) {
            Com_Quit();
        }
    }

    return 0;
}
