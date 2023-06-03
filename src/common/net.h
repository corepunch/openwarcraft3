#ifndef net_h
#define net_h

#define MAX_MSGLEN 256 * 1024

typedef void const *LPCVOID;
typedef struct sizebuf *LPSIZEBUF;
typedef struct entityState_s entityState_t;

typedef enum {
    NS_CLIENT, NS_SERVER
} NETSOURCE;

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

void NET_Write(NETSOURCE netsrc, DWORD sock, LPCVOID data, DWORD size);
int NET_Read(NETSOURCE netsrc, DWORD sock, HANDLE data, DWORD size);
int NET_GetPacket(NETSOURCE netsrc, DWORD sock, LPSIZEBUF msg);

void Netchan_Transmit(NETSOURCE netsrc, struct netchan *netchan);

void MSG_Write(LPSIZEBUF buf, LPCVOID value, DWORD size);
void MSG_WriteByte(LPSIZEBUF buf, int value);
void MSG_WriteShort(LPSIZEBUF buf, int value);
void MSG_WriteLong(LPSIZEBUF buf, int value);
void MSG_WriteFloat(LPSIZEBUF buf, float value);
void MSG_WriteString(LPSIZEBUF buf, LPCSTR value);
void MSG_WriteDeltaEntity(LPSIZEBUF buf, entityState_t const *from, entityState_t const *to, bool force);

int MSG_Read(LPSIZEBUF buf, HANDLE value, DWORD size);
int MSG_ReadByte(LPSIZEBUF buf);
int MSG_ReadShort(LPSIZEBUF buf);
int MSG_ReadLong(LPSIZEBUF buf);
float MSG_ReadFloat(LPSIZEBUF buf);
void MSG_ReadString(LPSIZEBUF buf, LPSTR value);
LPCSTR MSG_ReadString2(LPSIZEBUF buf);
void MSG_ReadDeltaEntity(LPSIZEBUF buf, entityState_t *edict, int number, int bits);

HANDLE SZ_GetSpace(LPSIZEBUF buf, DWORD length);
void SZ_Write(LPSIZEBUF buf, HANDLE data, DWORD length);
void SZ_Printf(LPSIZEBUF msg, LPCSTR fmt, ...);
void SZ_Init(LPSIZEBUF buf, BYTE *data, DWORD length);
void SZ_Clear(LPSIZEBUF buf);

#endif
