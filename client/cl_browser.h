#ifndef cl_browser_h
#define cl_browser_h

/*
 * cl_browser.h - LAN server browser model.
 *
 * Maintains an in-memory list of servers discovered via OOB getinfo
 * broadcast.  Entries are upserted on each infoResponse and expire
 * after SERVER_INFO_EXPIRY_MS without a refresh.  This module owns
 * neither networking nor UI; cl_main.c pumps it from the packet
 * dispatch loop and (eventually) a menu module reads CL_BrowserList
 * for display.
 *
 * Discovery surfaces *untrusted* candidates.  Anyone on the network
 * can advertise a server with any name.  Trust enforcement happens at
 * the connect handshake, not here.
 */

#include "../common/shared.h"
#include "../common/net.h"

#define SERVER_BROWSE_MAX_ENTRIES   64
#define SERVER_BROWSE_HOSTNAME_LEN  64
#define SERVER_BROWSE_MAP_LEN       64
#define SERVER_BROWSE_EXPIRY_MS     5000

typedef struct {
    netadr_t address;
    char     hostname[SERVER_BROWSE_HOSTNAME_LEN];
    char     map[SERVER_BROWSE_MAP_LEN];
    DWORD    clients;
    DWORD    maxclients;
    int      protocol;
    DWORD    last_seen_ms;
} server_info_t;

void CL_BrowserInit(void);
void CL_RefreshServerList(void);

// Called by the OOB packet dispatcher in cl_main.c with the payload
// that follows the OOB_INFORESPONSE token.  Upserts the entry into
// the in-memory list (keyed by address) and stamps its last-seen time.
void CL_BrowserHandleInfoResponse(const netadr_t *from,
                                  const char *payload,
                                  int len);

// Shared upsert primitive used by both the OOB-broadcast path
// (CL_BrowserHandleInfoResponse) and the mDNS path (cl_mdns.c).
// Both discovery transports are parallel and equally untrusted; auth
// happens at the connect handshake, not the discovery layer.
void CL_BrowserUpsert(const netadr_t *from,
                      const char *hostname,
                      const char *map,
                      DWORD clients,
                      DWORD maxclients,
                      int protocol);

// Drop entries whose last_seen_ms is older than SERVER_BROWSE_EXPIRY_MS.
// Safe to call every frame from CL_Frame.
void CL_BrowserPurgeExpired(void);

// Return a pointer to the current list and its length.  Valid until
// the next call to any other CL_Browser* function.
const server_info_t *CL_BrowserList(int *count);

#endif /* cl_browser_h */
