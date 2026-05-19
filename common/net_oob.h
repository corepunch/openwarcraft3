#ifndef net_oob_h
#define net_oob_h

/*
 * net_oob.h - Out-of-band (OOB) opcode tokens for LAN server discovery.
 *
 * OOB packets are detected by a 4-byte -1 sequence marker (see
 * Netchan_OutOfBand in net.c).  The payload that follows is an ASCII
 * token, optionally followed by space-separated arguments.  Servers
 * dispatch on the leading token before any client slot is allocated.
 *
 * Format and tokens are borrowed from Quake 3 / dpmaster:
 *   client -> server:  "getinfo <protocol_version>"
 *   server -> client:  "infoResponse\hostname\<n>\map\<m>\clients\<c>\maxclients\<m>\protocol\<v>"
 *
 * The infoResponse payload is a backslash-delimited key/value blob
 * (alternating keys and values, no trailing backslash).  The leading
 * backslash before "hostname" is part of the wire format.
 *
 * Discovery via these tokens carries no authentication.  Anyone on the
 * LAN can emit either packet with arbitrary fields.  The auth gate is
 * the connect handshake (see sv_init.c:SV_DirectConnect and Slice 2
 * connect-token validation when it lands), not the discovery layer.
 */

#define OOB_CONNECT          "connect"
#define OOB_CLIENT_CONNECT   "client_connect"
#define OOB_GETSERVERS       "getservers"
#define OOB_GETINFO          "getinfo"
#define OOB_INFORESPONSE     "infoResponse"

/* Key names inside the infoResponse key/value blob. */
#define OOB_KEY_HOSTNAME   "hostname"
#define OOB_KEY_MAP        "map"
#define OOB_KEY_CLIENTS    "clients"
#define OOB_KEY_MAXCLIENTS "maxclients"
#define OOB_KEY_PROTOCOL   "protocol"

#define OOB_KV_SEPARATOR   "\\"

/* mDNS / DNS-SD service type used by sv_mdns.c (advertise) and
 * cl_mdns.c (browse).  One definition because the unity build
 * concatenates all .c files in a directory into a single TU. */
#define MDNS_SERVICE_TYPE  "_openwarcraft3._udp"

#include "shared.h"

/* Test whether `payload[0 .. len-1]` begins with the OOB ASCII `token`
 * AND that the byte immediately after the token is a word boundary
 * (space, NUL, or end-of-payload).  This is the strict variant that
 * prefix-match dispatch needs: it accepts "getinfo", "getinfo 1",
 * "getinfo\0...", but rejects "getinfoXYZ" or "getinfomore".
 *
 * Returns false on any of: NULL payload, NULL token, len < strlen(token),
 * bytes-mismatch, or non-boundary trailing byte. */
bool OOB_TokenMatches(const char *payload, int len, const char *token);

#endif /* net_oob_h */
