#ifndef net_h
#define net_h

#define MAX_MSGLEN 256 * 1024

typedef void const *LPCVOID;
typedef struct sizeBuf_s *LPSIZEBUF;
typedef struct entityState_s entityState_t;

typedef enum {
    NA_LOOPBACK,
    NA_BROADCAST,
    NA_IP,
    NA_IPX,
    NA_BROADCAST_IPX
} netadrtype_t;

typedef enum {
    NS_CLIENT, NS_SERVER
} NETSOURCE;

typedef struct sizeBuf_s {
    LPBYTE data;
    DWORD maxsize;
    DWORD cursize;
    DWORD readcount;
} sizeBuf_t;

typedef struct {
    netadrtype_t type;
    BYTE ip[4];
    BYTE ipx[10];
    unsigned short port;
} netadr_t;

struct netchan {
    DWORD sock;
    sizeBuf_t message;
    BYTE message_buf[MAX_MSGLEN];
};

void NET_Write(NETSOURCE netsrc, DWORD sock, LPCVOID data, DWORD size);
int NET_Read(NETSOURCE netsrc, DWORD sock, HANDLE data, DWORD size);
int NET_GetPacket(NETSOURCE netsrc, DWORD sock, LPSIZEBUF msg);

void Netchan_Transmit(NETSOURCE netsrc, struct netchan *netchan);
void Netchan_OutOfBand(NETSOURCE netsrc, netadr_t adr, DWORD length, BYTE *data);
void Netchan_OutOfBandPrint(NETSOURCE netsrc, netadr_t adr, LPCSTR format, ...);

void MSG_Write(LPSIZEBUF buf, LPCVOID value, DWORD size);
void MSG_WriteByte(LPSIZEBUF buf, int value);
void MSG_WriteShort(LPSIZEBUF buf, int value);
void MSG_WriteLong(LPSIZEBUF buf, int value);
void MSG_WriteFloat(LPSIZEBUF buf, float value);
void MSG_WriteFloat2(LPSIZEBUF buf, float value);
void MSG_WriteString(LPSIZEBUF buf, LPCSTR value);
void MSG_WriteDeltaEntity(LPSIZEBUF buf, entityState_t const *from, entityState_t const *to, bool force);
void MSG_WriteDeltaUIFrame(LPSIZEBUF msg, LPCUIFRAME from, LPCUIFRAME to, bool force);
void MSG_WriteDeltaPlayerState(LPSIZEBUF msg, playerState_t const *from, playerState_t const *to);
void MSG_WriteEntityBits(LPSIZEBUF buf, DWORD bits, DWORD number);
void MSG_WritePos(LPSIZEBUF buf, LPCVECTOR3 pos);
void MSG_WriteDir(LPSIZEBUF buf, LPCVECTOR3 dir);
void MSG_WriteAngle(LPSIZEBUF buf, float f);

int MSG_Read(LPSIZEBUF buf, HANDLE value, DWORD size);
int MSG_ReadByte(LPSIZEBUF buf);
int MSG_ReadShort(LPSIZEBUF buf);
int MSG_ReadLong(LPSIZEBUF buf);
float MSG_ReadFloat(LPSIZEBUF buf);
void MSG_ReadString(LPSIZEBUF buf, LPSTR value);
void MSG_ReadPos(LPSIZEBUF buf, LPVECTOR3 pos);
void MSG_ReadDir(LPSIZEBUF buf, LPVECTOR3 dir);
float MSG_ReadAngle(LPSIZEBUF buf);
LPCSTR MSG_ReadString2(LPSIZEBUF buf);
void MSG_ReadDeltaEntity(LPSIZEBUF buf, entityState_t *edict, int number, int bits);
void MSG_ReadDeltaUIFrame(LPSIZEBUF msg, LPUIFRAME edict, int number, int bits);
void MSG_ReadDeltaPlayerState(LPSIZEBUF msg, playerState_t *edict, int number, int bits);
int MSG_ReadEntityBits(LPSIZEBUF msg, DWORD *bits);

HANDLE SZ_GetSpace(LPSIZEBUF buf, DWORD length);
void SZ_Write(LPSIZEBUF buf, void const *data, DWORD length);
void SZ_Printf(LPSIZEBUF msg, LPCSTR fmt, ...);
void SZ_Init(LPSIZEBUF buf, BYTE *data, DWORD length);
void SZ_Clear(LPSIZEBUF buf);

#endif
