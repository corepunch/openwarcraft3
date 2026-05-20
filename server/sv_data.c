/*
 * sv_data.c — Server-side data request handlers for client UI.
 *
 * This file implements server handlers that respond to client data requests
 * with lists of maps, games, and player information.
 *
 * Part of Phase 5: Data Request Protocol
 */

#include "../common/common.h"
#include "server.h"

/* Handle clc_request_map_list from client */
void SV_HandleMapListRequest(LPCLIENT client) {
    CON_printf("SV_HandleMapListRequest: from client %d\n", (int)(client - svs.clients));
    
    /* For now, return a stub list */
    /* TODO: Scan Maps/ directory or MPQ for actual map list */
    LPCSTR stubMaps[] = {
        "Maps\\Campaign\\Human01.w3m",
        "Maps\\Campaign\\Human02.w3m",
        "Maps\\Campaign\\Orc01.w3m",
        "Maps\\(2)BridgeTooNear.w3m",
        "Maps\\(4)TwistedMeadows.w3m"
    };
    
    int count = sizeof(stubMaps) / sizeof(stubMaps[0]);
    
    /* Build response message */
    MSG_WriteByte(&client->netchan.message, svc_map_list);
    MSG_WriteShort(&client->netchan.message, (short)count);
    
    for (int i = 0; i < count; i++) {
        MSG_WriteString(&client->netchan.message, stubMaps[i]);
    }
    
    CON_printf("SV_HandleMapListRequest: sent %d maps\n", count);
}

/* Handle clc_request_map_info from client */
void SV_HandleMapInfoRequest(LPCLIENT client, LPSIZEBUF msg) {
    short mapIndex = MSG_ReadShort(msg);
    
    CON_printf("SV_HandleMapInfoRequest: client %d requested map %d\n", 
               (int)(client - svs.clients), mapIndex);
    
    /* For now, return stub data */
    /* TODO: Parse actual map file for info */
    MSG_WriteByte(&client->netchan.message, svc_map_info);
    MSG_WriteShort(&client->netchan.message, mapIndex);
    MSG_WriteString(&client->netchan.message, "Map Title");
    MSG_WriteString(&client->netchan.message, "Map description text");
    MSG_WriteString(&client->netchan.message, "UI\\Glues\\MapPreviews\\default.blp");
}

/* Handle clc_request_game_list from client */
void SV_HandleGameListRequest(LPCLIENT client) {
    CON_printf("SV_HandleGameListRequest: from client %d\n", (int)(client - svs.clients));
    
    /* For now, return a stub list */
    /* TODO: Query LAN/online game server for active games */
    LPCSTR stubGames[] = {
        "Game 1 - Bridge Too Near",
        "Game 2 - Twisted Meadows",
        "Game 3 - Custom Map"
    };
    
    int count = sizeof(stubGames) / sizeof(stubGames[0]);
    
    /* Build response message */
    MSG_WriteByte(&client->netchan.message, svc_game_list);
    MSG_WriteShort(&client->netchan.message, (short)count);
    
    for (int i = 0; i < count; i++) {
        MSG_WriteString(&client->netchan.message, stubGames[i]);
    }
    
    CON_printf("SV_HandleGameListRequest: sent %d games\n", count);
}

/* Handle clc_request_player_list from client */
void SV_HandlePlayerListRequest(LPCLIENT client) {
    CON_printf("SV_HandlePlayerListRequest: from client %d\n", (int)(client - svs.clients));
    
    /* Build list of connected players */
    int count = 0;
    LPCSTR playerNames[MAX_CLIENTS];
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (svs.clients[i].state >= cs_connected) {
            /* TODO: Get player name from edict or client structure */
            playerNames[count++] = "Player";
        }
    }
    
    /* Build response message */
    MSG_WriteByte(&client->netchan.message, svc_player_list);
    MSG_WriteShort(&client->netchan.message, (short)count);
    
    for (int i = 0; i < count; i++) {
        MSG_WriteString(&client->netchan.message, playerNames[i]);
    }
    
    CON_printf("SV_HandlePlayerListRequest: sent %d players\n", count);
}
