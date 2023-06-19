#include <stdarg.h>

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
    if (netchan->message.cursize == 0)
        return;
    NET_Write(netsrc, netchan->sock, &netchan->message.cursize, 4);
    NET_Write(netsrc, netchan->sock, netchan->message_buf, netchan->message.cursize);
    netchan->message.cursize = 0;
}

void SZ_Init(LPSIZEBUF buf, BYTE *data, DWORD length) {
    memset(buf, 0, sizeof(*buf));
    buf->data = data;
    buf->maxsize = length;
}

void SZ_Clear(LPSIZEBUF buf) {
    buf->cursize = 0;
}

HANDLE SZ_GetSpace(LPSIZEBUF buf, DWORD length) {
    if (buf->cursize + length > buf->maxsize) {
//        if (length > buf->maxsize)
//            Com_Error (ERR_FATAL, "SZ_GetSpace: %i is > full buffer size", length);
        fprintf(stderr, "SZ_GetSpace: overflow\n");
        SZ_Clear(buf);
    }
    HANDLE data = buf->data + buf->cursize;
    buf->cursize += length;
    return data;
}

void SZ_Write(LPSIZEBUF buf, void const *data, DWORD length) {
    memcpy(SZ_GetSpace(buf, length), data, length);
}

void Netchan_OutOfBand(NETSOURCE netsrc, netadr_t adr, DWORD length, BYTE *data) {
    sizeBuf_t send;
    BYTE send_buf[MAX_MSGLEN];

// write the packet header
    SZ_Init(&send, send_buf, sizeof(send_buf));
    
    MSG_WriteLong(&send, length + 4);
    MSG_WriteLong(&send, -1);    // -1 sequence means out of band
    SZ_Write(&send, data, length);

// send the datagram
//    NET_SendPacket(net_socket, send.cursize, send.data, adr);
    NET_Write(netsrc, 0, send.data, send.cursize);
}

void Netchan_OutOfBandPrint(NETSOURCE netsrc, netadr_t adr, LPCSTR format, ...) {
    va_list argptr;
    static char string[MAX_MSGLEN - 4];
    va_start(argptr, format);
    vsprintf(string, format,argptr);
    va_end(argptr);
    Netchan_OutOfBand(netsrc, adr, (DWORD)strlen(string), (BYTE *)string);
}
