#include "common.h"

#define BUFFER_SIZE (1024 * 256)

struct loopback {
    char buffer[BUFFER_SIZE];
    int read;
    int write;
};

struct loopback bufs[2] = { 0 };

void NET_Write(NETSOURCE netsrc, DWORD sock, LPCVOID data, DWORD size) {
    struct loopback *buf = &bufs[netsrc];
    FOR_LOOP(i, size) {
        buf->buffer[(buf->write++) % BUFFER_SIZE] = ((LPSTR)data)[i];
    }
}

int NET_Read(NETSOURCE netsrc, DWORD sock, HANDLE data, DWORD size) {
    struct loopback *buf = &bufs[!netsrc];
    FOR_LOOP(i, size) {
        if (buf->read == buf->write)
            return i;
        ((LPSTR)data)[i] = buf->buffer[(buf->read++) % BUFFER_SIZE];
    }
    return size;
}

int NET_GetPacket(NETSOURCE netsrc, DWORD sock, LPSIZEBUF msg) {
    DWORD size = 0;
    if (NET_Read(netsrc, sock, &size, 4)) {
        assert(size < MAX_MSGLEN);
        NET_Read(netsrc, sock, msg->data, size);
        msg->cursize = size;
        msg->maxsize = size;
        msg->readcount = 0;
        return size;
    }
    return 0;
}

//int NET_SendPacket(DWORD sock, LPSIZEBUF msg) {
//    NET_Write(sock, &msg->cursize, 4);
//    NET_Write(sock, msg->data, msg->cursize);
//    return 0;
//}

void Netchan_Transmit(NETSOURCE netsrc, struct netchan *netchan) {
    NET_Write(netsrc, netchan->sock, &netchan->message.cursize, 4);
    NET_Write(netsrc, netchan->sock, netchan->message_buf, netchan->message.cursize);
    netchan->message.cursize = 0;
}
