#include "server.h"

void SV_ParseCommand(LPSIZEBUF msg, LPCLIENT client) {
    CLIENTMESSAGE cmsg = {
        .cmd = MSG_ReadByte(msg),
        .entity = MSG_ReadShort(msg),
        .targetentity = MSG_ReadShort(msg),
        .location = {
            .x = MSG_ReadShort(msg),
            .y = MSG_ReadShort(msg),
        }
    };
    if (cmsg.entity < ge->num_edicts) {
        ge->ClientCommand(&cmsg);
    }
}

void SV_ParseMove(LPSIZEBUF msg, LPCLIENT client) {
    client->camera_position.x = MSG_ReadShort(msg);
    client->camera_position.y = MSG_ReadShort(msg);
}

void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
            case clc_move:
                SV_ParseMove(msg, client);
                break;;
            case clc_command:
                SV_ParseCommand(msg, client);
                break;
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
