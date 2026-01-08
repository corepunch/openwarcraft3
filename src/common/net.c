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
// TCP/IP socket implementation for networked multiplayer
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "common.h"

#define BUFFER_SIZE (1024 * 256)
#define MAX_SOCKETS 32

// Per-socket receive buffer for partial packet handling
struct socketBuffer {
    char buffer[BUFFER_SIZE];
    int size;  // Current amount of data in buffer
};

static struct socketBuffer sockBuffers[MAX_SOCKETS] = { 0 };
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
// TCP/IP socket implementation
void NET_Write(NETSOURCE netsrc, DWORD sock, LPCVOID data, DWORD size) {
    if (sock == 0) {
        // Socket 0 is invalid, skip
        return;
    }
    
    int sent = 0;
    while (sent < (int)size) {
        int result = send(sock, (const char *)data + sent, size - sent, 0);
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Socket buffer full, wait and retry
                usleep(1000);  // Sleep 1ms
                continue;
            }
            fprintf(stderr, "NET_Write: send error: %d\n", errno);
            return;
        }
        sent += result;
    }
}

int NET_Read(NETSOURCE netsrc, DWORD sock, HANDLE data, DWORD size) {
    if (sock == 0 || sock >= MAX_SOCKETS) {
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
        int result = recv(sock, (char *)data + totalRead, size - totalRead, 0);
        if (result < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // No more data available right now
                return totalRead;
            }
            fprintf(stderr, "NET_Read: recv error: %d\n", errno);
            return totalRead;
        }
        if (result == 0) {
            // Connection closed
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
        if (sock < MAX_SOCKETS) {
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
// TCP/IP socket functions

DWORD NET_TCPSocket(void) {
    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock < 0) {
        fprintf(stderr, "NET_TCPSocket: socket creation failed: %d\n", errno);
        return 0;
    }
    
    // Enable TCP_NODELAY to disable Nagle's algorithm for lower latency
    int flag = 1;
    setsockopt(sock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    // Enable SO_REUSEADDR to allow quick restart
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &flag, sizeof(flag));
    
    return (DWORD)sock;
}

DWORD NET_TCPListen(unsigned short port, int backlog) {
    DWORD sock = NET_TCPSocket();
    if (sock == 0) {
        return 0;
    }
    
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);
    
    if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        fprintf(stderr, "NET_TCPListen: bind failed on port %u: %d\n", port, errno);
        close(sock);
        return 0;
    }
    
    if (listen(sock, backlog) < 0) {
        fprintf(stderr, "NET_TCPListen: listen failed: %d\n", errno);
        close(sock);
        return 0;
    }
    
    fprintf(stderr, "NET_TCPListen: listening on port %u\n", port);
    return sock;
}

DWORD NET_TCPAccept(DWORD listensock) {
    struct sockaddr_in clientAddr;
    socklen_t clientLen = sizeof(clientAddr);
    
    int clientSock = accept(listensock, (struct sockaddr *)&clientAddr, &clientLen);
    if (clientSock < 0) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            fprintf(stderr, "NET_TCPAccept: accept failed: %d\n", errno);
        }
        return 0;
    }
    
    // Enable TCP_NODELAY on client socket
    int flag = 1;
    setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    fprintf(stderr, "NET_TCPAccept: accepted connection from %s:%d\n",
            inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
    
    return (DWORD)clientSock;
}

int NET_TCPConnect(DWORD sock, LPCSTR host, unsigned short port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    
    // Try to convert as IP address first
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        // Not a valid IP address, try hostname resolution
        struct hostent *he = gethostbyname(host);
        if (he == NULL || he->h_addrtype != AF_INET) {
            fprintf(stderr, "NET_TCPConnect: failed to resolve hostname: %s\n", host);
            return -1;
        }
        memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);
    }
    
    if (connect(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        if (errno != EINPROGRESS) {
            fprintf(stderr, "NET_TCPConnect: connect failed to %s:%u: %d\n", host, port, errno);
            return -1;
        }
    }
    
    fprintf(stderr, "NET_TCPConnect: connected to %s:%u\n", host, port);
    return 0;
}

void NET_SetNonBlocking(DWORD sock, bool nonblocking) {
    int flags = fcntl(sock, F_GETFL, 0);
    if (flags < 0) {
        fprintf(stderr, "NET_SetNonBlocking: fcntl F_GETFL failed: %d\n", errno);
        return;
    }
    
    if (nonblocking) {
        flags |= O_NONBLOCK;
    } else {
        flags &= ~O_NONBLOCK;
    }
    
    if (fcntl(sock, F_SETFL, flags) < 0) {
        fprintf(stderr, "NET_SetNonBlocking: fcntl F_SETFL failed: %d\n", errno);
    }
}

void NET_CloseSocket(DWORD sock) {
    if (sock != 0) {
        close(sock);
        
        // Clear socket buffer
        if (sock < MAX_SOCKETS) {
            memset(&sockBuffers[sock], 0, sizeof(sockBuffers[sock]));
        }
    }
}

// Network discovery - broadcast to find active games on LAN
void NET_DiscoverGames(unsigned short port, int timeout_ms) {
    int sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        fprintf(stderr, "NET_DiscoverGames: socket creation failed: %d\n", errno);
        return;
    }
    
    // Enable broadcast
    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        fprintf(stderr, "NET_DiscoverGames: failed to enable broadcast: %d\n", errno);
        close(sock);
        return;
    }
    
    // Set receive timeout
    struct timeval tv;
    tv.tv_sec = timeout_ms / 1000;
    tv.tv_usec = (timeout_ms % 1000) * 1000;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    
    // Broadcast discovery packet
    struct sockaddr_in broadcastAddr;
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));
    broadcastAddr.sin_family = AF_INET;
    broadcastAddr.sin_addr.s_addr = INADDR_BROADCAST;
    broadcastAddr.sin_port = htons(port);
    
    const char *query = "OPENWARCRAFT3_DISCOVER";
    if (sendto(sock, query, strlen(query), 0, 
               (struct sockaddr *)&broadcastAddr, sizeof(broadcastAddr)) < 0) {
        fprintf(stderr, "NET_DiscoverGames: broadcast failed: %d\n", errno);
        close(sock);
        return;
    }
    
    fprintf(stderr, "NET_DiscoverGames: sent discovery broadcast on port %u\n", port);
    
    // Listen for responses
    char buffer[1024];
    struct sockaddr_in serverAddr;
    socklen_t addrLen = sizeof(serverAddr);
    int gameCount = 0;
    
    fprintf(stderr, "Scanning for active games (timeout: %dms)...\n", timeout_ms);
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recvfrom(sock, buffer, sizeof(buffer) - 1, 0,
                            (struct sockaddr *)&serverAddr, &addrLen);
        
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // Timeout - no more responses
                break;
            }
            fprintf(stderr, "NET_DiscoverGames: recvfrom error: %d\n", errno);
            break;
        }
        
        if (bytes > 0) {
            buffer[bytes] = '\0';
            gameCount++;
            fprintf(stderr, "[Game %d] %s:%d - %s\n", 
                    gameCount,
                    inet_ntoa(serverAddr.sin_addr),
                    ntohs(serverAddr.sin_port),
                    buffer);
        }
    }
    
    if (gameCount == 0) {
        fprintf(stderr, "No active games found on the network.\n");
    } else {
        fprintf(stderr, "Found %d active game(s).\n", gameCount);
    }
    
    close(sock);
}

#endif // USE_LOOPBACK
