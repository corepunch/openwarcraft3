#include "client.h"

int CL_ParseEntityBits(struct sizebuf *buf, uint32_t *bits) {
    *bits = MSG_ReadShort(buf);
    return MSG_ReadLong(buf);
}

