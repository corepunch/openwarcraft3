#include "common.h"

void MSG_Write(struct sizebuf *buf, void const *value, int size) {
    if (buf->cursize + size > buf->maxsize) {
        fprintf(stderr, "Write buffer overflow\n");
        return;
    }
    memcpy(buf->data + buf->cursize, value, size);
    buf->cursize += size;
}

void MSG_WriteByte(struct sizebuf *buf, int value) {
    char val = value;
    MSG_Write(buf, &val, 1);
}

void MSG_WriteShort(struct sizebuf *buf, int value) {
    short val = value;
    MSG_Write(buf, &val, 2);
}

void MSG_WriteLong(struct sizebuf *buf, int value) {
    MSG_Write(buf, &value, 4);
}

void MSG_WriteString(struct sizebuf *buf, const char *value) {
    int const len = (int)strlen(value);
    MSG_WriteLong(buf, len + 1);
    MSG_Write(buf, value, len + 1);
}

void MSG_Read(struct sizebuf *buf, void *value, int size) {
    if (buf->readcount + size > buf->maxsize) {
        fprintf(stderr, "Write buffer overflow\n");
        return;
    }
    memcpy(value, buf->data + buf->readcount, size);
    buf->readcount += size;
}

int MSG_ReadByte(struct sizebuf *buf) {
    char value = 0;
    MSG_Read(buf, &value, 1);
    return value;
}

int MSG_ReadShort(struct sizebuf *buf) {
    short value =0 ;
    MSG_Read(buf, &value, 2);
    return value;
}
int MSG_ReadLong(struct sizebuf *buf) {
    int value = 0;
    MSG_Read(buf, &value, 4);
    return value;
}
void MSG_ReadString(struct sizebuf *buf, char *value) {
    MSG_Read(buf, value, MSG_ReadLong(buf));
}
