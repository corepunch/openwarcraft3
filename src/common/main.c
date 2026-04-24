#include "../client/client.h"
#include "../server/server.h"

#define USAGE \
"Usage:\n" \
"  openwarcraft3 -mpq=<path> [-map=<map>]         (listen server + local client)\n" \
"  openwarcraft3 -mpq=<path> -connect=<host>      (remote client, default port " \
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
"  - Omitting -map opens the map-selector window in the UI.\n" \
"  - Remote clients still need the MPQ for asset loading.\n"

#define __XSTR(x) __STR(x)
#define __STR(x) #x

/* Forward declarations for the orion-ui layer (src/ui/ui_main.c).
 * We use plain C types to avoid the rect_t conflict between the game
 * headers and orion-ui headers. */
void UI_Init(void);
void UI_ProcessEvents(void);
void UI_Shutdown(void);
int  UI_IsRunning(void);

/* axGetMilliseconds() from the platform library — returns ms since epoch. */
extern unsigned long axGetMilliseconds(void);

HANDLE FS_AddArchive(LPCSTR);

void Sys_Quit(void) {
    exit(0);
}

int main(int argc, LPSTR argv[]) {
    LPCSTR map          = NULL;
    LPCSTR connect_addr = NULL;
    BOOL   mpq          = 0;

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

    if (!mpq) {
        printf(USAGE);
        return 1;
    }

    // Initialise the orion-ui platform and create the OpenGL context BEFORE
    // the renderer is started (R_Init relies on a live GL context).
    UI_Init();

    // Bind the UDP socket.
    unsigned short udp_port = connect_addr ? 0 : PORT_SERVER;
    if (!NET_Init(udp_port)) {
        fprintf(stderr, "NET_Init failed\n");
        UI_Shutdown();
        return 1;
    }

    // Initialise common subsystems (calls SV_Init + CL_Init + R_Init).
    Com_Init();

    if (connect_addr) {
        // Remote-client mode: connect over UDP; no local server.
        CL_Connect(connect_addr, PORT_SERVER);
    } else if (map) {
        // Listen-server mode with an explicit map from the command line.
        SV_Map(map);
    }
    // If neither -map nor -connect was given the map-selector window (opened
    // by UI_Init) will let the player pick a map interactively.

    unsigned long startTime = axGetMilliseconds();
    while (UI_IsRunning()) {
        unsigned long currentTime = axGetMilliseconds();
        DWORD msec = (DWORD)(currentTime - startTime);
        startTime  = currentTime;

        if (!connect_addr) {
            SV_Frame(msec);
        }
        CL_Frame(msec);

        // Drain the platform event queue, dispatch window messages, and
        // repaint all windows (including the game FBO blit).
        UI_ProcessEvents();
    }

    UI_Shutdown();
    return 0;
}
