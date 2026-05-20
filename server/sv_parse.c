#include "server.h"

void SV_ParseMove(LPSIZEBUF msg, LPCLIENT client) {
    edict_t *clent = client->edict;
    float x = MSG_ReadShort(msg);
    float y = MSG_ReadShort(msg);
    ge->ClientPanCamera(clent, &MAKE(VECTOR2, x, y));
}

void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
            case clc_move:
                SV_ParseMove(msg, client);
                break;;
            case clc_stringcmd:
                SV_ExecuteUserCommand(msg, client);
                break;
                
            // Phase 5: UI data requests
            case clc_request_map_list:
                SV_HandleMapListRequest(client);
                break;
            case clc_request_map_info:
                SV_HandleMapInfoRequest(client, msg);
                break;
            case clc_request_game_list:
                SV_HandleGameListRequest(client);
                break;
            case clc_request_player_list:
                SV_HandlePlayerListRequest(client);
                break;
                
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
