/*
 * net.c — Unified network layer: loopback ring buffers + UDP sockets.
 *
 * The routing decision is made at runtime based on the destination address
 * type (netadr_t.type), matching the Quake 2 pattern:
 *
 *   NET_SendPacket()  — routes to loopback buffer (NA_LOOPBACK) or UDP
 *                       socket (NA_IP / NA_BROADCAST) based on to.type.
 *   NET_GetPacket()   — checks the loopback buffer first, then the UDP
 *                       socket.  Returns the sender address in *from.
 *
 * Loopback buffers (always present):
 *   Two 256 KiB ring buffers provide a zero-latency in-process path for
 *   local (same-process) server+client communication.  bufs[NS_CLIENT]
 *   carries client→server traffic; bufs[NS_SERVER] carries server→client.
 *
 * UDP socket:
 *   A single global non-blocking UDP socket is created by NET_Init().
 *   The server binds it to PORT_SERVER; clients bind to port 0 (OS-assigned
 *   ephemeral port).  All datagrams carry a 4-byte little-endian length
 *   prefix so the framing is identical to the loopback format.
 */
#include <stdarg.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "common.h"

#define LOOPBACK_SIZE (1024 * 256)

/* ---------------------------------------------------------------------------
 * Loopback ring buffers
 * -------------------------------------------------------------------------*/

struct loopback {
    char buffer[LOOPBACK_SIZE];
    int read;
    int write;
};

// bufs[NS_CLIENT] holds client→server packets; bufs[NS_SERVER] holds
// server→client packets.  Each sender writes into bufs[netsrc]; each
// receiver reads from bufs[!netsrc].
static struct loopback loopbufs[2];

static void NET_SendLoopPacket(NETSOURCE netsrc, int length, const void *data) {
    struct loopback *buf = &loopbufs[netsrc];
    DWORD len = (DWORD)length;
    FOR_LOOP(i, 4) {
        buf->buffer[(buf->write++) % LOOPBACK_SIZE] = ((char *)&len)[i];
    }
    FOR_LOOP(i, length) {
        buf->buffer[(buf->write++) % LOOPBACK_SIZE] = ((const char *)data)[i];
    }
}

static int NET_GetLoopPacket(NETSOURCE netsrc, netadr_t *from, LPSIZEBUF msg) {
    struct loopback *buf = &loopbufs[!netsrc];
    if (buf->read == buf->write)
        return 0;

    DWORD size = 0;
    FOR_LOOP(i, 4) {
        ((char *)&size)[i] = buf->buffer[(buf->read++) % LOOPBACK_SIZE];
    }
    assert(size < MAX_MSGLEN);
    FOR_LOOP(i, size) {
        ((char *)msg->data)[i] = buf->buffer[(buf->read++) % LOOPBACK_SIZE];
    }
    msg->cursize = size;
    msg->maxsize = size;
    msg->readcount = 0;

    memset(from, 0, sizeof(*from));
    from->type = NA_LOOPBACK;
    return (int)size;
}

/* ---------------------------------------------------------------------------
 * UDP socket
 * -------------------------------------------------------------------------*/

static int udp_socket = -1;

static void NET_SendUDPPacket(int length, const void *data, netadr_t to) {
    if (udp_socket < 0)
        return;
    if (length <= 0 || length > MAX_MSGLEN) {
        fprintf(stderr, "NET_SendUDPPacket: bad packet length %d\n", length);
        return;
    }

    // Packet format: [4-byte little-endian length][data]
    static unsigned char sendbuf[MAX_MSGLEN + 4];
    DWORD len_le = (DWORD)length;
    // Write length as 4 bytes in explicit little-endian order to avoid
    // strict-aliasing UB and ensure wire-level portability.
    sendbuf[0] = (unsigned char)( len_le        & 0xff);
    sendbuf[1] = (unsigned char)((len_le >>  8) & 0xff);
    sendbuf[2] = (unsigned char)((len_le >> 16) & 0xff);
    sendbuf[3] = (unsigned char)((len_le >> 24) & 0xff);
    memcpy(sendbuf + 4, data, length);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    memcpy(&addr.sin_addr, to.ip, 4);
    addr.sin_port = to.port;    // already in network byte order

    if (sendto(udp_socket, sendbuf, length + 4, 0,
               (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "NET_SendUDPPacket: sendto failed: %d\n", errno);
    }
}

static int NET_GetUDPPacket(NETSOURCE netsrc, netadr_t *from, LPSIZEBUF msg) {
    (void)netsrc;
    if (udp_socket < 0)
        return 0;

    static unsigned char recvbuf[MAX_MSGLEN + 4];
    struct sockaddr_in srcaddr;
    socklen_t addrlen = sizeof(srcaddr);

    int bytes = recvfrom(udp_socket, recvbuf, sizeof(recvbuf), 0,
                         (struct sockaddr *)&srcaddr, &addrlen);
    if (bytes < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK)
            fprintf(stderr, "NET_GetUDPPacket: recvfrom failed: %d\n", errno);
        return 0;
    }
    if (bytes < 4)
        return 0;

    // Read the 4-byte little-endian length prefix via memcpy to avoid
    // strict-aliasing UB and cope with potentially unaligned buffers.
    DWORD size = 0;
    size  = (DWORD)recvbuf[0];
    size |= (DWORD)recvbuf[1] <<  8;
    size |= (DWORD)recvbuf[2] << 16;
    size |= (DWORD)recvbuf[3] << 24;
    if ((int)(size + 4) > bytes || size >= MAX_MSGLEN) {
        fprintf(stderr, "NET_GetUDPPacket: invalid packet size %u\n", size);
        return 0;
    }

    memcpy(msg->data, recvbuf + 4, size);
    msg->cursize = size;
    msg->maxsize = size;
    msg->readcount = 0;

    memset(from, 0, sizeof(*from));
    from->type = NA_IP;
    memcpy(from->ip, &srcaddr.sin_addr, 4);
    from->port = srcaddr.sin_port;  // keep in network byte order
    return (int)size;
}

/* ---------------------------------------------------------------------------
 * Public API
 * -------------------------------------------------------------------------*/

bool NET_Init(unsigned short port) {
    memset(loopbufs, 0, sizeof(loopbufs));

    udp_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0) {
        fprintf(stderr, "NET_Init: socket creation failed: %d\n", errno);
        return false;
    }

    int flag = 1;
    setsockopt(udp_socket, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(udp_socket, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "NET_Init: bind failed on port %u: %d\n", port, errno);
        close(udp_socket);
        udp_socket = -1;
        return false;
    }

    // Set non-blocking so NET_GetUDPPacket never stalls the main loop
    int flags = fcntl(udp_socket, F_GETFL, 0);
    if (flags >= 0)
        fcntl(udp_socket, F_SETFL, flags | O_NONBLOCK);

    fprintf(stderr, "NET_Init: UDP socket bound to port %u\n", port);
    return true;
}

void NET_Shutdown(void) {
    if (udp_socket >= 0) {
        close(udp_socket);
        udp_socket = -1;
    }
}

bool NET_StringToAdr(LPCSTR s, unsigned short default_port, netadr_t *adr) {
    char host[256];
    unsigned short port = default_port;

    strncpy(host, s, sizeof(host) - 1);
    host[sizeof(host) - 1] = '\0';

    char *colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        port = (unsigned short)atoi(colon + 1);
    }

    memset(adr, 0, sizeof(*adr));
    adr->type = NA_IP;
    adr->port = htons(port);

    if (inet_pton(AF_INET, host, adr->ip) > 0)
        return true;

    // Fall back to hostname resolution
    struct hostent *he = gethostbyname(host);
    if (!he || he->h_addrtype != AF_INET) {
        fprintf(stderr, "NET_StringToAdr: failed to resolve \"%s\"\n", host);
        return false;
    }
    memcpy(adr->ip, he->h_addr_list[0], 4);
    return true;
}

// Route a packet to the loopback buffer or the UDP socket depending on
// the destination address type — the core of the Quake 2 network model.
void NET_SendPacket(NETSOURCE netsrc, int length, const void *data, netadr_t to) {
    switch (to.type) {
    case NA_LOOPBACK:
        NET_SendLoopPacket(netsrc, length, data);
        break;
    case NA_IP:
    case NA_BROADCAST:
        NET_SendUDPPacket(length, data, to);
        break;
    default:
        break;
    }
}

// Check the loopback buffer first (zero latency for local clients), then
// fall through to the UDP socket for remote clients.
int NET_GetPacket(NETSOURCE netsrc, netadr_t *from, LPSIZEBUF msg) {
    int r = NET_GetLoopPacket(netsrc, from, msg);
    if (r)
        return r;
    return NET_GetUDPPacket(netsrc, from, msg);
}

void Netchan_Transmit(NETSOURCE netsrc, struct netchan *netchan) {
    if (netchan->message.cursize == 0)
        return;
    NET_SendPacket(netsrc, (int)netchan->message.cursize,
                   netchan->message_buf, netchan->remote_address);
    netchan->message.cursize = 0;
}

void SZ_Init(LPSIZEBUF buf, BYTE *data, DWORD length) {
    memset(buf, 0, sizeof(*buf));
    buf->data = data;
    buf->maxsize = length;
}

void SZ_Clear(LPSIZEBUF buf) {
    buf->cursize = 0;
}

HANDLE SZ_GetSpace(LPSIZEBUF buf, DWORD length) {
    if (buf->cursize + length > buf->maxsize) {
//        if (length > buf->maxsize)
//            Com_Error (ERR_FATAL, "SZ_GetSpace: %i is > full buffer size", length);
        fprintf(stderr, "SZ_GetSpace: overflow\n");
        SZ_Clear(buf);
    }
    HANDLE data = buf->data + buf->cursize;
    buf->cursize += length;
    return data;
}

void SZ_Write(LPSIZEBUF buf, void const *data, DWORD length) {
    memcpy(SZ_GetSpace(buf, length), data, length);
}

void Netchan_OutOfBand(NETSOURCE netsrc, netadr_t adr, DWORD length, BYTE *data) {
    BYTE send_buf[MAX_MSGLEN];
    sizeBuf_t send;

    SZ_Init(&send, send_buf, sizeof(send_buf));
    MSG_WriteLong(&send, -1);       // -1 sequence marks an out-of-band packet
    SZ_Write(&send, data, length);

    NET_SendPacket(netsrc, (int)send.cursize, send.data, adr);
}

void Netchan_OutOfBandPrint(NETSOURCE netsrc, netadr_t adr, LPCSTR format, ...) {
    va_list argptr;
    static char string[MAX_MSGLEN - 4];
    va_start(argptr, format);
    vsnprintf(string, sizeof(string), format, argptr);
    va_end(argptr);
    string[sizeof(string) - 1] = '\0';
    Netchan_OutOfBand(netsrc, adr, (DWORD)strlen(string), (BYTE *)string);
}

