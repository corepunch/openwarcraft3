#include "server.h"

void SV_WriteGameCommand(LPSIZEBUF msg, LPCSTR command, sizeBuf_t const *payload) {
    if (!msg || !command || !payload) {
        return;
    }
    if (payload->cursize > 0x7fff) {
        fprintf(stderr,
                "SV_WriteGameCommand: payload too large for %s: %u bytes\n",
                command,
                (unsigned)payload->cursize);
        return;
    }
    MSG_WriteByte(msg, svc_gamecmd);
    MSG_WriteString(msg, command);
    MSG_WriteShort(msg, (int)payload->cursize);
    SZ_Write(msg, payload->data, payload->cursize);
}

void SV_Multicast(LPCVECTOR3 origin, multicast_t to) {
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        SZ_Write(&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
    }
    SZ_Clear(&sv.multicast);
}
