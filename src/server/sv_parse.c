#include "server.h"

void SV_ParseCommand(LPSIZEBUF msg, LPCLIENT client) {
    CLIENTMESSAGE cmsg;
    cmsg.cmd = MSG_ReadByte(msg);
    cmsg.num_entities = MSG_ReadShort(msg);
    FOR_LOOP(i, cmsg.num_entities) {
        cmsg.entities[i] = MSG_ReadShort(msg);
    }
    cmsg.targetentity = MSG_ReadShort(msg);
    cmsg.location.x = MSG_ReadShort(msg);
    cmsg.location.y = MSG_ReadShort(msg);
    ge->ClientCommand(&cmsg);
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
