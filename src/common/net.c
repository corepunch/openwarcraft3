#include "common.h"

#define BUFFER_SIZE (1024 * 256)

struct loopback {
    char buffer[BUFFER_SIZE];
    int read;
    int write;
};

struct loopback bufs[2] = { 0 };

void NET_Write(int sock, void const *data, int size) {
    struct loopback *buf = &bufs[sock];
    FOR_LOOP(i, size) {
        buf->buffer[(buf->write++) % BUFFER_SIZE] = ((char *)data)[i];
    }
}

int NET_Read(int sock, void *data, int size) {
    struct loopback *buf = &bufs[sock];
    FOR_LOOP(i, size) {
        if (buf->read == buf->write)
            return i;
        ((char *)data)[i] = buf->buffer[(buf->read++) % BUFFER_SIZE];
    }
    return size;
}

void Netchan_Transmit(struct netchan *netchan) {
    NET_Write(netchan->sock, netchan->message_buf, netchan->message.cursize);
    netchan->message.cursize = 0;
}
