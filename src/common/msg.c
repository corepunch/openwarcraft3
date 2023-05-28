#include "common.h"

#define NETF(x) #x,((uint8_t *)&((entityState_t*)0)->x - (uint8_t *)NULL)

typedef struct {
    char *name;
    long offset;
    int bits; // 0 = float
} netField_t;

netField_t entityStateFields[] = {
    { NETF(origin.x), 0 },
    { NETF(origin.y), 0 },
    { NETF(origin.z), 0 },
    { NETF(angle), -1 },
    { NETF(scale), 0 },
    { NETF(frame), 32 },
    { NETF(model), 16 },
    { NETF(image), 16 },
    { NETF(player), 8 },
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

void MSG_WriteString(LPSIZEBUF buf, const LPSTR value) {
    int const len = (int)strlen(value);
    MSG_WriteLong(buf, len + 1);
    MSG_Write(buf, value, len + 1);
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
    short value =0 ;
    MSG_Read(buf, &value, 2);
    return value;
}
int MSG_ReadLong(LPSIZEBUF buf) {
    int value = 0;
    MSG_Read(buf, &value, 4);
    return value;
}
void MSG_ReadString(LPSIZEBUF buf, LPSTR value) {
    MSG_Read(buf, value, MSG_ReadLong(buf));
}

void MSG_WriteDeltaEntity(LPSIZEBUF msg, entityState_t const *from, entityState_t const *to) {
    int bits = 0;
    
    for (netField_t *field = entityStateFields; field->name; field++) {
        int *fromF = (int *)((uint8_t *)from + field->offset);
        int *toF = (int *)((uint8_t *)to + field->offset);
        if (*fromF != *toF) {
            bits |= 1 << (field - entityStateFields);
        }
    }

    if (bits == 0)
        return;
    
    MSG_WriteShort(msg, bits);
    MSG_WriteShort(msg, to->number);

    for (netField_t *field = entityStateFields; field->name; field++) {
        if ((bits & (1 << (field - entityStateFields))) == 0)
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

void MSG_ReadDeltaEntity(LPSIZEBUF msg, entityState_t *edict, int number, int bits) {
    edict->number = number;
    for (netField_t *field = entityStateFields; field->name; field++) {
        if ((bits & (1 << (field - entityStateFields))) == 0)
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

