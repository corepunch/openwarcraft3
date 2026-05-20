/*
 * ui.h — UI library public interface.
 *
 * Defines the import/export function tables for the UI library following
 * the same pattern as the renderer (client/renderer.h) and game DLL
 * (server/game.h).
 *
 * The client fills uiImport_t with callbacks for file I/O, memory allocation,
 * and command execution, then calls UI_GetAPI() to receive the uiExport_t
 * function table.
 *
 * The UI library loads FDF files, builds frame hierarchies, manages menu
 * navigation, and handles input events. It owns its string table (loaded from
 * war3skins.txt) for localization. Commands are executed via Cmd_ExecuteText;
 * the engine's command dispatcher handles routing (local vs server).
 */
#ifndef ui_h
#define ui_h

#include "../client/renderer.h"

/* Unit UI data structures (Phase 8) */
#define MAX_COMMAND_BUTTONS 12
#define MAX_INVENTORY_SLOTS 6
#define MAX_BUILD_QUEUE_ITEMS 7

typedef struct {
    char art[256];        /* Button icon path */
    char tooltip[256];    /* Tooltip text */
    char ubertip[512];    /* Extended tooltip */
    char command[256];    /* Command to execute on click */
    char hotkey;          /* Keyboard hotkey */
} uiCommandButton_t;

typedef struct {
    char art[256];        /* Item icon path */
    char tooltip[256];    /* Tooltip text */
    char ubertip[512];    /* Extended tooltip */
    BYTE slot;            /* Inventory slot index (0-5) */
} uiInventoryItem_t;

typedef struct {
    char art[256];        /* Queue item icon path */
    WORD entity;          /* Entity number of building unit */
} uiQueueItem_t;

typedef struct {
    WORD entity_num;                              /* Entity number */
    BYTE num_buttons;                             /* Number of command buttons */
    uiCommandButton_t buttons[MAX_COMMAND_BUTTONS];
    BYTE num_inventory;                           /* Number of inventory items */
    uiInventoryItem_t inventory[MAX_INVENTORY_SLOTS];
    BYTE num_queue;                               /* Number of build queue items */
    uiQueueItem_t queue[MAX_BUILD_QUEUE_ITEMS];
} uiUnitData_t;

/* Callbacks provided by the client to the UI library.
 * The UI imports file I/O, memory allocation, and command forwarding. */
typedef struct {
    /* File system operations (archive-agnostic, Quake 3 pattern) */
    int (*FS_ReadFile)(LPCSTR fileName, void **buf);  /* Returns file size, allocates buf */
    void (*FS_FreeFile)(void *buf);
    
    /* Memory allocation */
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    
    /* Asset indexing (for textures, models, fonts) */
    int (*ModelIndex)(LPCSTR modelName);
    int (*ImageIndex)(LPCSTR imageName);
    int (*FontIndex)(LPCSTR fontName, DWORD fontSize);
    
    /* Command execution (following Quake 3 pattern)
     * UI executes console commands; engine dispatcher handles routing */
    void (*Cmd_ExecuteText)(LPCSTR text);
    
    /* Data requests (map lists, game lists, player info) */
    void (*RequestMapList)(void);
    void (*RequestMapInfo)(int mapIndex);
    void (*RequestGameList)(void);
    void (*RequestPlayerInfo)(void);
    
    /* Game state access (for in-game HUD) */
    LPCPLAYER (*GetPlayerState)(void);          /* Access to cl.playerstate */
    DWORD (*GetNumEntities)(void);              /* cl.num_entities */
    LPCENTITYSTATE (*GetEntity)(DWORD idx);     /* &cl.ents[idx].current */
    
    /* Unit UI data requests (for command card, inventory, build queue) */
    void (*RequestUnitUI)(DWORD num_selected, DWORD *entity_nums);
    
    /* Renderer access for frame drawing */
    LPRENDERER (*GetRenderer)(void);
    
    /* Error reporting */
    void (*Error)(LPCSTR fmt, ...);
    void (*Printf)(LPCSTR fmt, ...);
} uiImport_t;

/* Function table exported by the UI library to the client. */
typedef struct {
    /* Initialization and shutdown */
    void (*Init)(void);
    void (*Shutdown)(void);
    
    /* Main loop integration */
    void (*Refresh)(DWORD msec);
    void (*DrawFrame)(void);
    
    /* Input event handling */
    void (*KeyEvent)(int key, BOOL down, DWORD time);
    void (*MouseEvent)(int x, int y, int button, BOOL down);
    
    /* Menu navigation (called by client for button clicks, console commands) */
    void (*MenuCommand)(LPCSTR route);
    
    /* Data updates from server (called when server sends response messages) */
    void (*UpdateMapList)(DWORD count, LPCSTR *names);
    void (*UpdateMapInfo)(DWORD index, LPCSTR title, LPCSTR description, LPCSTR preview);
    void (*UpdateGameList)(DWORD count, LPCSTR *names);
    void (*UpdatePlayerInfo)(DWORD playerCount, LPCSTR *playerNames);
    
    /* Unit UI data updates (Phase 8: HUD migration) */
    void (*UpdateUnitUI)(DWORD num_units, uiUnitData_t *units);
} uiExport_t;

/* Entry point called by the client to get the UI function table.
 * The client must fill the uiImport_t struct before calling this. */
uiExport_t UI_GetAPI(uiImport_t uiimport);

#endif
