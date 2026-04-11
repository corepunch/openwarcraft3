# Network Architecture

OpenWarcraft3 uses the same runtime-dispatch networking model as Quake 2.  A
single send/receive API (`NET_SendPacket` / `NET_GetPacket`) handles all
communication; only the lowest layer changes path depending on whether the
destination is in the same process (loopback) or a remote machine (UDP).

## Address types (`netadr_t`)

Every packet is addressed with a `netadr_t`:

```c
typedef enum {
    NA_LOOPBACK,    // in-process ring buffer (zero copy, zero latency)
    NA_IP,          // unicast UDP
    NA_BROADCAST,   // broadcast UDP
} netadrtype_t;

typedef struct {
    netadrtype_t type;
    unsigned char ip[4];   // network byte order
    unsigned short port;   // network byte order
} netadr_t;
```

The `type` field is the only routing key.  `ip` and `port` are only
inspected when `type` is `NA_IP` or `NA_BROADCAST`.

## Loopback (listen-server / single-process)

When the executable starts with `-map=` it runs both the server and the local
client in the same process.  The local client slot is assigned address type
`NA_LOOPBACK`; no socket is involved.

Two 256 KiB ring buffers carry traffic in each direction:

```
  bufs[NS_CLIENT]  ←  client writes (clc_move, clc_connect, …)
  bufs[NS_SERVER]  ←  server writes (svc_packetentities, svc_serverdata, …)
```

Sending to `NA_LOOPBACK` appends a 4-byte little-endian length prefix followed
by the payload into the appropriate ring buffer.  Receiving polls the other
side's ring buffer.

```c
// src/common/net.c
void NET_SendPacket(NETSOURCE netsrc, int length, const void *data, netadr_t to) {
    switch (to.type) {
    case NA_LOOPBACK:
        NET_SendLoopPacket(netsrc, length, data);
        break;
    case NA_IP:
    case NA_BROADCAST:
        NET_SendUDPPacket(length, data, to);
        break;
    }
}

int NET_GetPacket(NETSOURCE netsrc, netadr_t *from, LPSIZEBUF msg) {
    int r = NET_GetLoopPacket(netsrc, from, msg);  // loopback first
    if (r) return r;
    return NET_GetUDPPacket(netsrc, from, msg);    // then real socket
}
```

## UDP (remote client)

When the executable starts with `-connect=` it binds to an ephemeral port
(port 0; the OS assigns one) and connects to the specified server over UDP.
The server always listens on `PORT_SERVER` (27910).

The UDP path shares the same framing as the loopback path: every datagram
starts with a 4-byte **little-endian** length prefix, followed by the payload.
The length is encoded/decoded with `memcpy` into a `uint32_t` to avoid
strict-aliasing and alignment issues:

```c
// Encode length prefix (little-endian, no aliasing UB)
uint32_t prefix = (uint32_t)length;
unsigned char sendbuf[4 + MAX_MSGLEN];
sendbuf[0] = (prefix      ) & 0xff;
sendbuf[1] = (prefix >>  8) & 0xff;
sendbuf[2] = (prefix >> 16) & 0xff;
sendbuf[3] = (prefix >> 24) & 0xff;
memcpy(sendbuf + 4, data, length);
sendto(udp_socket, sendbuf, 4 + length, 0, ...);
```

## Initialisation

`NET_Init(port)` is called once from `main.c` before any server or client
code runs:

| Mode | Port argument | Effect |
|------|--------------|--------|
| Listen server (`-map=`) | `PORT_SERVER` (27910) | Bind UDP socket to 27910 so remote clients can find it |
| Remote client (`-connect=`) | `0` | Bind UDP socket to OS-assigned ephemeral port |

If socket creation fails, `NET_Init` returns `0` and the process exits.

`NET_Shutdown()` is called on exit to close the socket.

## Connection handshake

### Remote client → server

1. Client calls `CL_Connect(host, port)` which resolves the hostname via
   `NET_StringToAdr` and sends an out-of-band `"connect"` datagram to the
   server.
2. The server's `SV_ReadPackets` reads the datagram, checks that the payload
   starts with `"connect"`, and calls `SV_DirectConnect(from)` to allocate a
   new client slot with `NA_IP` type.
3. From this point the normal `clc_*` / `svc_*` message exchange proceeds over
   UDP, identical to the loopback exchange.

### Local client (loopback)

`SV_Map` calls `SV_ClientConnect()` which allocates slot 0 with type
`NA_LOOPBACK`.  No network handshake is required; the client sends a
`clc_connect` message through the ring buffer and the server responds with
`svc_serverdata`.

## Command-line interface

```sh
# Listen server + local client (loopback; no real socket traffic)
openwarcraft3 -mpq=/path/to/War3.mpq -map=Maps\Campaign\Human02.w3m

# Remote client, default port (PORT_SERVER = 27910)
openwarcraft3 -mpq=/path/to/War3.mpq -connect=192.168.1.10

# Remote client, explicit port
openwarcraft3 -mpq=/path/to/War3.mpq -connect=192.168.1.10:27910
```

| Argument | Description |
|----------|-------------|
| `-mpq=<path>` | Absolute path to `War3.mpq` (required for asset loading in both modes) |
| `-map=<path>` | Internal MPQ path of the map to load; starts a listen server |
| `-connect=<host[:port]>` | Hostname or IP of the server to join; starts a remote-client session |

`-map` and `-connect` are mutually exclusive.  Omitting both (with a valid
`-mpq`) prints the usage message.

## Key files

| File | Purpose |
|------|---------|
| `src/common/net.c` | `NET_SendPacket`, `NET_GetPacket`, loopback buffers, UDP socket |
| `src/common/net.h` | `netadr_t`, `netchan_t`, `NETSOURCE`, public API |
| `src/common/main.c` | CLI parsing, `NET_Init`, mode selection |
| `src/server/sv_init.c` | `SV_ClientConnect` (loopback slot), `SV_DirectConnect` (UDP slot) |
| `src/server/sv_main.c` | `SV_ReadPackets` — OOB routing and per-slot dispatch |
| `src/client/cl_main.c` | `CL_Connect`, `CL_ReadPackets` |
