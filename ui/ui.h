/*
 * ui.h — UI library public interface.
 *
 * Defines the import/export function tables for the UI library following
 * the same pattern as the renderer (client/renderer.h) and game DLL
 * (server/game.h).
 *
 * The client fills uiImport_t with callbacks for file I/O, memory allocation,
 * and command forwarding, then calls UI_GetAPI() to receive the uiExport_t
 * function table.
 *
 * The UI library loads FDF files, builds frame hierarchies, manages menu
 * navigation, and handles input events. It communicates with the server only
 * via data request commands (map lists, game lists, player info).
 */
#ifndef ui_h
#define ui_h

#include "../common/common.h"

/* Callbacks provided by the client to the UI library.
 * The UI imports file I/O, memory allocation, and command forwarding. */
typedef struct {
    /* File system operations (MPQ archive access) */
    HANDLE (*FileOpen)(LPCSTR fileName);
    BOOL (*FileExtract)(LPCSTR toExtract, LPCSTR extracted);
    void (*FileClose)(HANDLE file);
    HANDLE (*LoadFile)(LPCSTR fileName, LPDWORD size);
    
    /* Memory allocation */
    HANDLE (*MemAlloc)(long size);
    void (*MemFree)(HANDLE);
    
    /* Asset indexing (for textures, models, fonts) */
    int (*ModelIndex)(LPCSTR modelName);
    int (*ImageIndex)(LPCSTR imageName);
    int (*FontIndex)(LPCSTR fontName, DWORD fontSize);
    
    /* String table access (for localization) */
    LPCSTR (*GetString)(LPCSTR key);
    
    /* Command forwarding (send commands to server or execute locally) */
    void (*SendCommand)(LPCSTR cmd);
    void (*LocalCommand)(LPCSTR cmd);
    
    /* Data requests (map lists, game lists, player info) */
    void (*RequestMapList)(void);
    void (*RequestMapInfo)(DWORD mapIndex);
    void (*RequestGameList)(void);
    void (*RequestPlayerInfo)(void);
    
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
} uiExport_t;

/* Entry point called by the client to get the UI function table.
 * The client must fill the uiImport_t struct before calling this. */
uiExport_t UI_GetAPI(uiImport_t uiimport);

#endif
