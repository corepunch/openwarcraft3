#include "client.h"

int CL_ParseEntityBits(LPSIZEBUF buf, uint32_t *bits) {
    *bits = MSG_ReadShort(buf);
    return MSG_ReadShort(buf);
}

