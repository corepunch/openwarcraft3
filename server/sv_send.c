#include "server.h"

void SV_Multicast(LPCVECTOR3 origin, multicast_t to) {
    FOR_LOOP(i, svs.num_clients) {
        LPCLIENT client = &svs.clients[i];
        SZ_Write(&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
    }
    SZ_Clear(&sv.multicast);
}
