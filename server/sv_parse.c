#include "server.h"

void SV_ParseCameraPosition(LPSIZEBUF msg, LPCLIENT client) {
    edict_t *clent = client->edict;
    float x = MSG_ReadFloat(msg);
    float y = MSG_ReadFloat(msg);
    ge->ClientSetCameraPosition(clent, &MAKE(VECTOR2, x, y));
}

void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
            case clc_camera_position:
                SV_ParseCameraPosition(msg, client);
                break;;
            case clc_stringcmd:
                SV_ExecuteUserCommand(msg, client);
                break;
                
            // Phase 8: Unit UI data requests
            case clc_request_unit_ui:
                SV_HandleUnitUIRequest(client, msg);
                break;
                
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
