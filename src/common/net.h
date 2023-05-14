#ifndef net_h
#define net_h

#define MAX_MSGLEN 256 * 1024

struct sizebuf {
    unsigned char *data;
    int maxsize;
    int cursize;
    int readcount;
};

struct netchan {
    int sock;
    struct sizebuf message;
    unsigned char message_buf[MAX_MSGLEN];
};

void NET_Write(int sock, void const *data, int size);
int NET_Read(int sock, void *data, int size);

void Netchan_Transmit(struct netchan *netchan);

void MSG_Write(struct sizebuf *buf, void const *value, int size);
void MSG_WriteByte(struct sizebuf *buf, int value);
void MSG_WriteShort(struct sizebuf *buf, int value);
void MSG_WriteLong(struct sizebuf *buf, int value);
void MSG_WriteString(struct sizebuf *buf, const char *value);

void MSG_Read(struct sizebuf *buf, void *value, int size);
int MSG_ReadByte(struct sizebuf *buf);
int MSG_ReadShort(struct sizebuf *buf);
int MSG_ReadLong(struct sizebuf *buf);
void MSG_ReadString(struct sizebuf *buf, char *value);

#endif
