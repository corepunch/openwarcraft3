#include "common.h"

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
