#ifndef net_h
#define net_h

#define MAX_MSGLEN 256 * 1024

typedef void const *LPCVOID;

typedef struct sizebuf *LPSIZEBUF;

struct sizebuf {
    LPBYTE data;
    DWORD maxsize;
    DWORD cursize;
    DWORD readcount;
};

struct netchan {
    DWORD sock;
    struct sizebuf message;
    BYTE message_buf[MAX_MSGLEN];
};

void NET_Write(DWORD sock, LPCVOID data, DWORD size);
int NET_Read(DWORD sock, HANDLE data, DWORD size);
int NET_GetPacket(DWORD sock, LPSIZEBUF msg);

void Netchan_Transmit(struct netchan *netchan);

void MSG_Write(LPSIZEBUF buf, LPCVOID value, DWORD size);
void MSG_WriteByte(LPSIZEBUF buf, int value);
void MSG_WriteShort(LPSIZEBUF buf, int value);
void MSG_WriteLong(LPSIZEBUF buf, int value);
void MSG_WriteString(LPSIZEBUF buf, const LPSTR value);

int MSG_Read(LPSIZEBUF buf, HANDLE value, DWORD size);
int MSG_ReadByte(LPSIZEBUF buf);
int MSG_ReadShort(LPSIZEBUF buf);
int MSG_ReadLong(LPSIZEBUF buf);
void MSG_ReadString(LPSIZEBUF buf, LPSTR value);

#endif
