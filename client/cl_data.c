/*
 * cl_data.c — Client-side data request/response handlers for UI library.
 *
 * This file implements network message handlers that request dynamic data
 * from the server (map lists, game lists, player info) and parse responses
 * to update the UI library via callbacks.
 *
 * Part of Phase 5: Data Request Protocol
 */

#include <stdlib.h>
#include "client.h"

/* Send a map list request to the server */
void CL_RequestMapList(void) {
    MSG_WriteByte(&cls.netchan.message, clc_request_map_list);
    CON_printf("CL_RequestMapList: sent request\n");
}

/* Send a map info request for a specific map index */
void CL_RequestMapInfo(int mapIndex) {
    MSG_WriteByte(&cls.netchan.message, clc_request_map_info);
    MSG_WriteShort(&cls.netchan.message, (short)mapIndex);
    CON_printf("CL_RequestMapInfo: index=%d\n", mapIndex);
}

/* Send a game list request to the server */
void CL_RequestGameList(void) {
    MSG_WriteByte(&cls.netchan.message, clc_request_game_list);
    CON_printf("CL_RequestGameList: sent request\n");
}

/* Send a player list request to the server */
void CL_RequestPlayerList(void) {
    MSG_WriteByte(&cls.netchan.message, clc_request_player_list);
    CON_printf("CL_RequestPlayerList: sent request\n");
}

/* Parse svc_map_list response from server */
void CL_ParseMapList(LPSIZEBUF msg) {
    short count = MSG_ReadShort(msg);
    
    if (count < 0 || count > MAX_LIST_FETCH_ROWS) {
        CON_printf("CL_ParseMapList: invalid count %d\n", count);
        return;
    }
    
    /* Allocate array of map names */
    LPCSTR *mapNames = MemAlloc(count * sizeof(LPCSTR));
    
    for (int i = 0; i < count; i++) {
        LPCSTR mapName = MSG_ReadString2(msg);
        mapNames[i] = strdup(mapName);
    }
    
    CON_printf("CL_ParseMapList: received %d maps\n", count);
    
    /* Forward to UI library */
    if (ui.UpdateMapList) {
        ui.UpdateMapList((DWORD)count, mapNames);
    }
    
    /* Free temporary storage */
    for (int i = 0; i < count; i++) {
        free((void *)mapNames[i]);
    }
    MemFree(mapNames);
}

/* Parse svc_map_info response from server */
void CL_ParseMapInfo(LPSIZEBUF msg) {
    short index = MSG_ReadShort(msg);
    
    LPCSTR title = MSG_ReadString2(msg);
    LPCSTR description = MSG_ReadString2(msg);
    LPCSTR preview = MSG_ReadString2(msg);
    
    CON_printf("CL_ParseMapInfo: index=%d title='%s'\n", index, title);
    
    /* Forward to UI library */
    if (ui.UpdateMapInfo) {
        ui.UpdateMapInfo((DWORD)index, title, description, preview);
    }
}

/* Parse svc_game_list response from server */
void CL_ParseGameList(LPSIZEBUF msg) {
    short count = MSG_ReadShort(msg);
    
    if (count < 0 || count > MAX_LIST_FETCH_ROWS) {
        CON_printf("CL_ParseGameList: invalid count %d\n", count);
        return;
    }
    
    /* Allocate array of game names */
    LPCSTR *gameNames = MemAlloc(count * sizeof(LPCSTR));
    
    for (int i = 0; i < count; i++) {
        LPCSTR gameName = MSG_ReadString2(msg);
        gameNames[i] = strdup(gameName);
    }
    
    CON_printf("CL_ParseGameList: received %d games\n", count);
    
    /* Forward to UI library */
    if (ui.UpdateGameList) {
        ui.UpdateGameList((DWORD)count, gameNames);
    }
    
    /* Free temporary storage */
    for (int i = 0; i < count; i++) {
        free((void *)gameNames[i]);
    }
    MemFree(gameNames);
}

/* Parse svc_player_list response from server */
void CL_ParsePlayerList(LPSIZEBUF msg) {
    short count = MSG_ReadShort(msg);
    
    if (count < 0 || count > MAX_LIST_FETCH_ROWS) {
        CON_printf("CL_ParsePlayerList: invalid count %d\n", count);
        return;
    }
    
    /* Allocate array of player names */
    LPCSTR *playerNames = MemAlloc(count * sizeof(LPCSTR));
    
    for (int i = 0; i < count; i++) {
        LPCSTR playerName = MSG_ReadString2(msg);
        playerNames[i] = strdup(playerName);
    }
    
    CON_printf("CL_ParsePlayerList: received %d players\n", count);
    
    /* Forward to UI library */
    if (ui.UpdatePlayerInfo) {
        ui.UpdatePlayerInfo((DWORD)count, playerNames);
    }
    
    /* Free temporary storage */
    for (int i = 0; i < count; i++) {
        free((void *)playerNames[i]);
    }
    MemFree(playerNames);
}
