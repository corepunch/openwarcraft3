#include <stdarg.h>

#include "common.h"

#define NETF(type, x) #x,((uint8_t *)&((type*)0)->x - (uint8_t *)NULL)

typedef struct {
    char *name;
    long offset;
    int bits; // 0 = float
} netField_t;

netField_t entityStateFields[] = {
    { NETF(entityState_t, origin.x), 0 },
    { NETF(entityState_t, origin.y), 0 },
    { NETF(entityState_t, origin.z), 0 },
    { NETF(entityState_t, angle), -1 },
    { NETF(entityState_t, scale), 0 },
    { NETF(entityState_t, frame), 32 },
    { NETF(entityState_t, model), 16 },
    { NETF(entityState_t, image), 16 },
    { NETF(entityState_t, player), 8 },
    { NETF(entityState_t, flags), 8 },
    { NETF(entityState_t, renderfx), 8 },
    { NETF(entityState_t, radius), 0 },
    { NETF(entityState_t, splat), 32 },
    { NULL }
};

netField_t uiFrameFields[] = {
    { NETF(uiFrame_t, parent), 8 },
    { NETF(uiFrame_t, flagsvalue), 16 },
    { NETF(uiFrame_t, points.x[FPP_MIN]), 32 },
    { NETF(uiFrame_t, points.x[FPP_MID]), 32 },
    { NETF(uiFrame_t, points.x[FPP_MAX]), 32 },
    { NETF(uiFrame_t, points.y[FPP_MIN]), 32 },
    { NETF(uiFrame_t, points.y[FPP_MID]), 32 },
    { NETF(uiFrame_t, points.y[FPP_MAX]), 32 },
    { NETF(uiFrame_t, size), 32 },
    { NETF(uiFrame_t, tex.index), 16 },
    { NETF(uiFrame_t, tex.coord), 32 },
    { NETF(uiFrame_t, font.index), 16 },
    { NETF(uiFrame_t, code), 32 },
    { NETF(uiFrame_t, stat), 8 },
    { NULL }
};

netField_t playerStateFields[] = {
    { NETF(playerState_t, viewangles.x), 0 },
    { NETF(playerState_t, viewangles.y), 0 },
    { NETF(playerState_t, viewangles.z), 0 },
    { NETF(playerState_t, origin.x), 0 },
    { NETF(playerState_t, origin.y), 0 },
    { NETF(playerState_t, origin.z), 0 },
    { NETF(playerState_t, fov), 8 },
    { NETF(playerState_t, distance), 0 },
    { NETF(playerState_t, rdflags), 32 },
    { NETF(playerState_t, stats[0]), 32 },
    { NETF(playerState_t, stats[2]), 32 },
    { NETF(playerState_t, stats[4]), 32 },
    { NETF(playerState_t, stats[6]), 32 },
    { NETF(playerState_t, stats[8]), 32 },
    { NULL }
};


void MSG_Write(LPSIZEBUF buf, LPCVOID value, DWORD size) {
    if (buf->cursize + size > buf->maxsize) {
        fprintf(stderr, "Write buffer overflow\n");
        return;
    }
    memcpy(buf->data + buf->cursize, value, size);
    buf->cursize += size;
}

void MSG_WriteByte(LPSIZEBUF buf, int value) {
    char val = value;
    MSG_Write(buf, &val, 1);
}

void MSG_WriteShort(LPSIZEBUF buf, int value) {
    short val = value;
    MSG_Write(buf, &val, 2);
}

void MSG_WriteLong(LPSIZEBUF buf, int value) {
    MSG_Write(buf, &value, 4);
}

void MSG_WriteFloat(LPSIZEBUF buf, float value) {
    MSG_Write(buf, &value, 4);
}

void MSG_WriteFloat2(LPSIZEBUF buf, float value) {
    MSG_WriteShort(buf, value * 0xffff);
}

void MSG_WriteString(LPSIZEBUF buf, LPCSTR value) {
    MSG_Write(buf, value, (int)strlen(value) + 1);
}

void MSG_WritePos(LPSIZEBUF buf, LPCVECTOR3 pos) {
    MSG_WriteShort(buf, pos->x);
    MSG_WriteShort(buf, pos->y);
    MSG_WriteShort(buf, pos->z);
}

void MSG_ReadPos(LPSIZEBUF buf, LPVECTOR3 pos) {
    pos->x = MSG_ReadShort(buf);
    pos->y = MSG_ReadShort(buf);
    pos->z = MSG_ReadShort(buf);
}

void MSG_WriteDir(LPSIZEBUF buf, LPCVECTOR3 dir) {
    MSG_WriteFloat(buf, dir->x);
    MSG_WriteFloat(buf, dir->y);
    MSG_WriteFloat(buf, dir->z);
}

void MSG_ReadDir(LPSIZEBUF buf, LPVECTOR3 dir) {
    dir->x = MSG_ReadFloat(buf);
    dir->y = MSG_ReadFloat(buf);
    dir->z = MSG_ReadFloat(buf);
}

void MSG_WriteAngle(LPSIZEBUF buf, float f) {
    MSG_WriteByte(buf, (int)(f*256/(2*M_PI))&0xff);
}

float MSG_ReadAngle(LPSIZEBUF buf) {
    return MSG_ReadByte(buf)*(2*M_PI)/256;
}

int MSG_Read(LPSIZEBUF buf, HANDLE value, DWORD size) {
    if (buf->readcount + size > buf->cursize) {
        return 0;
    }
    memcpy(value, buf->data + buf->readcount, size);
    buf->readcount += size;
    return size;
}

int MSG_ReadByte(LPSIZEBUF buf) {
    char value = 0;
    MSG_Read(buf, &value, 1);
    return value;
}

int MSG_ReadShort(LPSIZEBUF buf) {
    short value = 0;
    MSG_Read(buf, &value, 2);
    return value;
}

int MSG_ReadLong(LPSIZEBUF buf) {
    int value = 0;
    MSG_Read(buf, &value, 4);
    return value;
}

float MSG_ReadFloat(LPSIZEBUF buf) {
    float value = 0;
    MSG_Read(buf, &value, 4);
    return value;
}

void MSG_ReadString(LPSIZEBUF buf, LPSTR value) {
    for (int c = MSG_ReadByte(buf), i = 0;; c = MSG_ReadByte(buf), i++) {
        value[i] = c;
        if (c == 0)
            break;
    }
}

LPCSTR MSG_ReadString2(LPSIZEBUF buf) {
    static char buffer[2048];
    MSG_ReadString(buf, buffer);
    return buffer;
}

static DWORD MSG_GetBits(void const *from,
                         void const *to,
                         netField_t *fields)
{
    DWORD bits = 0;
    for (netField_t *field = fields; field->name; field++) {
        int *fromF = (int *)((uint8_t *)from + field->offset);
        int *toF = (int *)((uint8_t *)to + field->offset);
        if (*fromF != *toF) {
            bits |= 1 << (field - fields);
        }
    }
    return bits;
}

static void MSG_WriteFields(LPSIZEBUF msg,
                            void const *to,
                            netField_t *fields,
                            DWORD bits)
{
    for (netField_t *field = fields; field->name; field++) {
        if ((bits & (1 << (field - fields))) == 0)
            continue;
        int *toF = (int *)((uint8_t *)to + field->offset);
        switch (field->bits) {
            case 0: MSG_WriteShort(msg, *(float *)toF); break;
            case -1: MSG_WriteShort(msg, *(float *)toF * 500); break;
            case 32: MSG_WriteLong(msg, *toF); break;
            case 16: MSG_WriteShort(msg, *toF); break;
            case 8: MSG_WriteByte(msg, *toF); break;
        }
    }
}

static void MSG_ReadFields(LPSIZEBUF msg,
                           void const *edict,
                           netField_t *fields,
                           DWORD bits)
{
    for (netField_t *field = fields; field->name; field++) {
        if ((bits & (1 << (field - fields))) == 0)
            continue;
        int *toF = (int *)((uint8_t *)edict + field->offset);
        switch (field->bits) {
            case 0: *(float *)toF = MSG_ReadShort(msg); break;
            case -1: *(float *)toF = MSG_ReadShort(msg) / 500.f; break;
            case 32: *toF = MSG_ReadLong(msg); break;
            case 16: *toF = MSG_ReadShort(msg); break;
            case 8: *toF = MSG_ReadByte(msg); break;
        }
    }
}

void MSG_WriteDeltaEntity(LPSIZEBUF msg,
                          entityState_t const *from,
                          entityState_t const *to,
                          bool force)
{
    DWORD bits = MSG_GetBits(from, to, entityStateFields);
    if (bits == 0 && !force)
        return;
    MSG_WriteEntityBits(msg, bits, to->number);
    MSG_WriteFields(msg, to, entityStateFields, bits);
}

void MSG_ReadDeltaEntity(LPSIZEBUF msg,
                         entityState_t *edict,
                         int number,
                         int bits)
{
    edict->number = number;
    MSG_ReadFields(msg, edict, entityStateFields, bits);
}

void MSG_WriteDeltaUIFrame(LPSIZEBUF msg,
                           uiFrame_t const *from,
                           uiFrame_t const *to,
                           bool force)
{
    DWORD bits = MSG_GetBits(from, to, uiFrameFields);
    if (bits == 0 && !force)
        return;
    MSG_WriteEntityBits(msg, bits, to->number);
    MSG_WriteFields(msg, to, uiFrameFields, bits);
}

void MSG_ReadDeltaUIFrame(LPSIZEBUF msg,
                          uiFrame_t *edict,
                          int number,
                          int bits)
{
    edict->number = number;
    MSG_ReadFields(msg, edict, uiFrameFields, bits);
}

void MSG_WriteDeltaPlayerState(LPSIZEBUF msg,
                               playerState_t const *from,
                               playerState_t const *to)
{
    DWORD bits = MSG_GetBits(from, to, playerStateFields);
    MSG_WriteEntityBits(msg, bits, to->number);
    MSG_WriteFields(msg, to, playerStateFields, bits);
}

void MSG_ReadDeltaPlayerState(LPSIZEBUF msg,
                              playerState_t *edict,
                              int number,
                              int bits)
{
    edict->number = number;
    MSG_ReadFields(msg, edict, playerStateFields, bits);
}

void SZ_Printf(LPSIZEBUF msg, LPCSTR fmt, ...) {
    va_list argptr;
    va_start(argptr, fmt);
    msg->cursize += vsprintf((LPSTR)(msg->data + msg->cursize), fmt, argptr) + 1;
    va_end(argptr);
}

void MSG_WriteEntityBits(LPSIZEBUF buf, DWORD bits, DWORD number) {
    MSG_WriteShort(buf, bits);
    MSG_WriteShort(buf, number);
}

int MSG_ReadEntityBits(LPSIZEBUF buf, DWORD *bits) {
    *bits = MSG_ReadShort(buf);
    return MSG_ReadShort(buf);
}
