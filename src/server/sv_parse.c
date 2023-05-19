#include "server.h"

void SV_ParseMove(LPSIZEBUF msg, LPCLIENT lpClient) {
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

void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT lpClient) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
            case clc_move:
                SV_ParseMove(msg, lpClient);
                break;
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}

