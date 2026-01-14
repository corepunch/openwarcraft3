#include <stdarg.h>

#ifdef USE_LOOPBACK
// Loopback implementation for local client-server communication
#include "common.h"

#define BUFFER_SIZE (1024 * 256)

struct loopback {
    char buffer[BUFFER_SIZE];
    int read;
    int write;
};

static struct loopback bufs[2] = { 0 };

#else
// SDL_net TCP/IP socket implementation for networked multiplayer
#include <SDL2/SDL_net.h>

#include "common.h"

#define BUFFER_SIZE (1024 * 256)
#define MAX_SOCKETS 32

// Per-socket receive buffer for partial packet handling
struct socketBuffer {
    char buffer[BUFFER_SIZE];
    int size;  // Current amount of data in buffer
};

static struct socketBuffer sockBuffers[MAX_SOCKETS] = { 0 };

// Map DWORD socket handles to SDL_net TCPsocket pointers
static TCPsocket socketMap[MAX_SOCKETS] = { 0 };

// Get TCPsocket from handle
static TCPsocket getSocket(DWORD handle) {
    if (handle == 0 || handle >= MAX_SOCKETS) {
        return NULL;
    }
    return socketMap[handle];
}

// Store TCPsocket and return handle
static DWORD storeSocket(TCPsocket sock) {
    if (!sock) {
        return 0;
    }
    
    // Find an available slot
    for (int i = 1; i < MAX_SOCKETS; i++) {
        if (socketMap[i] == NULL) {
            socketMap[i] = sock;
            return (DWORD)i;
        }
    }
    
    // No slots available
    SDLNet_TCP_Close(sock);
    return 0;
}

// Remove socket from map
static void removeSocket(DWORD handle) {
    if (handle > 0 && handle < MAX_SOCKETS) {
        socketMap[handle] = NULL;
        memset(&sockBuffers[handle], 0, sizeof(sockBuffers[handle]));
    }
}
#endif

#ifdef USE_LOOPBACK
// Loopback implementation
void NET_Write(NETSOURCE netsrc, DWORD sock, LPCVOID data, DWORD size) {
    struct loopback *buf = &bufs[netsrc];
    FOR_LOOP(i, size) {
        buf->buffer[(buf->write++) % BUFFER_SIZE] = ((LPSTR)data)[i];
    }
}

int NET_Read(NETSOURCE netsrc, DWORD sock, HANDLE data, DWORD size) {
    struct loopback *buf = &bufs[!netsrc];
    FOR_LOOP(i, size) {
        if (buf->read == buf->write)
            return i;
        ((LPSTR)data)[i] = buf->buffer[(buf->read++) % BUFFER_SIZE];
    }
    return size;
}

int NET_GetPacket(NETSOURCE netsrc, DWORD sock, LPSIZEBUF msg) {
    DWORD size = 0;
    if (NET_Read(netsrc, sock, &size, 4)) {
        assert(size < MAX_MSGLEN);
        NET_Read(netsrc, sock, msg->data, size);
        msg->cursize = size;
        msg->maxsize = size;
        msg->readcount = 0;
        return size;
    }
    return 0;
}

#else
// SDL_net TCP/IP socket implementation
void NET_Write(NETSOURCE netsrc, DWORD sock, LPCVOID data, DWORD size) {
    TCPsocket tcpSock = getSocket(sock);
    if (!tcpSock) {
        return;
    }
    
    int sent = 0;
    while (sent < (int)size) {
        int result = SDLNet_TCP_Send(tcpSock, (const char *)data + sent, size - sent);
        if (result <= 0) {
            fprintf(stderr, "NET_Write: SDLNet_TCP_Send error: %s\n", SDLNet_GetError());
            return;
        }
        sent += result;
    }
}

int NET_Read(NETSOURCE netsrc, DWORD sock, HANDLE data, DWORD size) {
    TCPsocket tcpSock = getSocket(sock);
    if (!tcpSock) {
        return 0;
    }
    
    struct socketBuffer *sockBuf = &sockBuffers[sock];
    int totalRead = 0;
    
    // First, use any buffered data
    if (sockBuf->size > 0) {
        int toCopy = (sockBuf->size < (int)size) ? sockBuf->size : (int)size;
        memcpy(data, sockBuf->buffer, toCopy);
        
        // Shift remaining data
        if (toCopy < sockBuf->size) {
            memmove(sockBuf->buffer, sockBuf->buffer + toCopy, sockBuf->size - toCopy);
        }
        sockBuf->size -= toCopy;
        totalRead = toCopy;
        
        if (totalRead == (int)size) {
            return totalRead;
        }
    }
    
    // Read more data from socket if needed
    while (totalRead < (int)size) {
        int result = SDLNet_TCP_Recv(tcpSock, (char *)data + totalRead, size - totalRead);
        if (result <= 0) {
            // No more data available or error
            return totalRead;
        }
        totalRead += result;
    }
    
    return totalRead;
}

int NET_GetPacket(NETSOURCE netsrc, DWORD sock, LPSIZEBUF msg) {
    DWORD size = 0;
    
    // Read the 4-byte size header
    int bytesRead = NET_Read(netsrc, sock, &size, 4);
    if (bytesRead < 4) {
        // Not enough data yet - NET_Read already buffered what was available
        return 0;
    }
    
    if (size == 0) {
        return 0;  // Empty packet
    }
    
    if (size >= MAX_MSGLEN) {
        fprintf(stderr, "NET_GetPacket: invalid packet size: %u\n", size);
        return 0;
    }
    
    // Read the packet data
    bytesRead = NET_Read(netsrc, sock, msg->data, size);
    if (bytesRead < (int)size) {
        // Not enough data yet - NET_Read already buffered what was available
        // We need to put the size header back in the buffer for next attempt
        TCPsocket tcpSock = getSocket(sock);
        if (tcpSock && sock < MAX_SOCKETS) {
            struct socketBuffer *sockBuf = &sockBuffers[sock];
            // Make room for the size header at the beginning
            if (sockBuf->size + 4 <= BUFFER_SIZE) {
                memmove(sockBuf->buffer + 4, sockBuf->buffer, sockBuf->size);
                memcpy(sockBuf->buffer, &size, 4);
                sockBuf->size += 4;
            }
        }
        return 0;
    }
    
    msg->cursize = size;
    msg->maxsize = size;
    msg->readcount = 0;
    return size;
}
#endif // USE_LOOPBACK

//int NET_SendPacket(DWORD sock, LPSIZEBUF msg) {
//    NET_Write(sock, &msg->cursize, 4);
//    NET_Write(sock, msg->data, msg->cursize);
//    return 0;
//}

void Netchan_Transmit(NETSOURCE netsrc, struct netchan *netchan) {
    if (netchan->message.cursize == 0)
        return;
    NET_Write(netsrc, netchan->sock, &netchan->message.cursize, 4);
    NET_Write(netsrc, netchan->sock, netchan->message_buf, netchan->message.cursize);
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
    sizeBuf_t send;
    BYTE send_buf[MAX_MSGLEN];

// write the packet header
    SZ_Init(&send, send_buf, sizeof(send_buf));
    
    MSG_WriteLong(&send, length + 4);
    MSG_WriteLong(&send, -1);    // -1 sequence means out of band
    SZ_Write(&send, data, length);

// send the datagram
//    NET_SendPacket(net_socket, send.cursize, send.data, adr);
    NET_Write(netsrc, 0, send.data, send.cursize);
}

void Netchan_OutOfBandPrint(NETSOURCE netsrc, netadr_t adr, LPCSTR format, ...) {
    va_list argptr;
    static char string[MAX_MSGLEN - 4];
    va_start(argptr, format);
    vsprintf(string, format,argptr);
    va_end(argptr);
    Netchan_OutOfBand(netsrc, adr, (DWORD)strlen(string), (BYTE *)string);
}

#ifndef USE_LOOPBACK
// SDL_net TCP/IP socket functions

DWORD NET_TCPSocket(void) {
    // SDL_net doesn't separate socket creation from connection/listening
    // This function is deprecated with SDL_net. Instead:
    // - For server sockets, use NET_TCPListen() directly
    // - For client sockets, use NET_TCPConnect() with a pre-allocated handle
    // 
    // To maintain some compatibility, we could allocate a slot here,
    // but that would leak resources if NET_TCPConnect is never called.
    // Therefore, this returns 0 (invalid handle).
    fprintf(stderr, "NET_TCPSocket: deprecated with SDL_net, use NET_TCPListen or NET_TCPConnect\n");
    return 0;
}

DWORD NET_TCPListen(unsigned short port, int backlog) {
    IPaddress ip;
    
    // Resolve to INADDR_ANY to create a server socket
    if (SDLNet_ResolveHost(&ip, NULL, port) < 0) {
        fprintf(stderr, "NET_TCPListen: SDLNet_ResolveHost failed: %s\n", SDLNet_GetError());
        return 0;
    }
    
    TCPsocket sock = SDLNet_TCP_Open(&ip);
    if (!sock) {
        fprintf(stderr, "NET_TCPListen: SDLNet_TCP_Open failed on port %u: %s\n", port, SDLNet_GetError());
        return 0;
    }
    
    fprintf(stderr, "NET_TCPListen: listening on port %u\n", port);
    return storeSocket(sock);
}

DWORD NET_TCPAccept(DWORD listensock) {
    TCPsocket serverSock = getSocket(listensock);
    if (!serverSock) {
        return 0;
    }
    
    TCPsocket clientSock = SDLNet_TCP_Accept(serverSock);
    if (!clientSock) {
        // No connection available (not an error)
        return 0;
    }
    
    IPaddress *remoteIP = SDLNet_TCP_GetPeerAddress(clientSock);
    if (remoteIP) {
        fprintf(stderr, "NET_TCPAccept: accepted connection from %d.%d.%d.%d:%d\n",
                (remoteIP->host >> 0) & 0xFF,
                (remoteIP->host >> 8) & 0xFF,
                (remoteIP->host >> 16) & 0xFF,
                (remoteIP->host >> 24) & 0xFF,
                remoteIP->port);
    }
    
    return storeSocket(clientSock);
}

int NET_TCPConnect(DWORD sock, LPCSTR host, unsigned short port) {
    IPaddress ip;
    
    // Resolve the hostname and port
    if (SDLNet_ResolveHost(&ip, host, port) < 0) {
        fprintf(stderr, "NET_TCPConnect: SDLNet_ResolveHost failed for %s:%u: %s\n", 
                host, port, SDLNet_GetError());
        return -1;
    }
    
    TCPsocket tcpSock = SDLNet_TCP_Open(&ip);
    if (!tcpSock) {
        fprintf(stderr, "NET_TCPConnect: SDLNet_TCP_Open failed to %s:%u: %s\n", 
                host, port, SDLNet_GetError());
        return -1;
    }
    
    fprintf(stderr, "NET_TCPConnect: connected to %s:%u\n", host, port);
    
    // SDL_net creates and connects in one operation, so we need to handle both cases:
    // 1. If sock is provided and valid, replace the existing socket
    // 2. If sock is 0, the caller should have called NET_TCPSocket first (which is now a no-op)
    //    In this case, we can't store the socket without knowing where to put it
    if (sock > 0 && sock < MAX_SOCKETS) {
        // Replace existing socket
        if (socketMap[sock]) {
            SDLNet_TCP_Close(socketMap[sock]);
        }
        socketMap[sock] = tcpSock;
        return 0;
    } else {
        // No valid handle provided - close the socket and return error
        // The caller should use NET_TCPSocket first, then NET_TCPConnect
        // But with SDL_net, they should just call NET_TCPConnect with a pre-allocated handle
        SDLNet_TCP_Close(tcpSock);
        fprintf(stderr, "NET_TCPConnect: invalid socket handle %u\n", sock);
        return -1;
    }
}

void NET_SetNonBlocking(DWORD sock, bool nonblocking) {
    // SDL_net doesn't have a direct non-blocking mode API
    // SDL_net sockets are non-blocking by default for accept and recv operations
    // This function is kept for API compatibility but does nothing
    (void)sock;
    (void)nonblocking;
}

void NET_CloseSocket(DWORD sock) {
    TCPsocket tcpSock = getSocket(sock);
    if (tcpSock) {
        SDLNet_TCP_Close(tcpSock);
        removeSocket(sock);
    }
}

// Network discovery - broadcast to find active games on LAN
void NET_DiscoverGames(unsigned short port, int timeout_ms) {
    // UDP discovery using SDL_net
    UDPsocket sock = SDLNet_UDP_Open(0);  // 0 = bind to any port
    if (!sock) {
        fprintf(stderr, "NET_DiscoverGames: SDLNet_UDP_Open failed: %s\n", SDLNet_GetError());
        return;
    }
    
    // Allocate a packet for sending
    UDPpacket *packet = SDLNet_AllocPacket(1024);
    if (!packet) {
        fprintf(stderr, "NET_DiscoverGames: SDLNet_AllocPacket failed: %s\n", SDLNet_GetError());
        SDLNet_UDP_Close(sock);
        return;
    }
    
    // Prepare broadcast address
    IPaddress broadcastAddr;
    // Use SDLNet_ResolveHost to properly handle byte ordering
    if (SDLNet_ResolveHost(&broadcastAddr, "255.255.255.255", port) < 0) {
        fprintf(stderr, "NET_DiscoverGames: SDLNet_ResolveHost for broadcast failed: %s\n", SDLNet_GetError());
        SDLNet_FreePacket(packet);
        SDLNet_UDP_Close(sock);
        return;
    }
    
    // Prepare discovery message
    const char *query = "OPENWARCRAFT3_DISCOVER";
    int queryLen = strlen(query);
    memcpy(packet->data, query, queryLen);
    packet->len = queryLen;
    packet->address = broadcastAddr;
    
    // Send broadcast
    if (SDLNet_UDP_Send(sock, -1, packet) == 0) {
        fprintf(stderr, "NET_DiscoverGames: SDLNet_UDP_Send failed: %s\n", SDLNet_GetError());
        SDLNet_FreePacket(packet);
        SDLNet_UDP_Close(sock);
        return;
    }
    
    fprintf(stderr, "NET_DiscoverGames: sent discovery broadcast on port %u\n", port);
    fprintf(stderr, "Scanning for active games (timeout: %dms)...\n", timeout_ms);
    
    // Listen for responses
    int gameCount = 0;
    Uint32 startTime = SDL_GetTicks();
    
    while (1) {
        Uint32 currentTime = SDL_GetTicks();
        if (currentTime - startTime > (Uint32)timeout_ms) {
            break;
        }
        
        int numrecv = SDLNet_UDP_Recv(sock, packet);
        if (numrecv == 1) {
            gameCount++;
            // Ensure null termination without buffer overflow
            if (packet->maxlen > 0) {
                if (packet->len < packet->maxlen) {
                    packet->data[packet->len] = '\0';  // Null terminate
                } else {
                    packet->data[packet->maxlen - 1] = '\0';  // Truncate and terminate
                }
            }
            fprintf(stderr, "[Game %d] %d.%d.%d.%d:%d - %s\n", 
                    gameCount,
                    (packet->address.host >> 0) & 0xFF,
                    (packet->address.host >> 8) & 0xFF,
                    (packet->address.host >> 16) & 0xFF,
                    (packet->address.host >> 24) & 0xFF,
                    SDL_SwapBE16(packet->address.port),
                    (char*)packet->data);
        } else if (numrecv == -1) {
            fprintf(stderr, "NET_DiscoverGames: SDLNet_UDP_Recv error: %s\n", SDLNet_GetError());
            break;
        }
        
        // Small delay to avoid busy waiting
        SDL_Delay(10);
    }
    
    if (gameCount == 0) {
        fprintf(stderr, "No active games found on the network.\n");
    } else {
        fprintf(stderr, "Found %d active game(s).\n", gameCount);
    }
    
    SDLNet_FreePacket(packet);
    SDLNet_UDP_Close(sock);
}

#endif // USE_LOOPBACK
