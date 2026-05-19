#ifndef cl_browser_h
#define cl_browser_h

/*
 * cl_browser.h - LAN server browser model.
 *
 * Maintains an in-memory list of servers discovered via OOB getinfo
 * broadcast and via mDNS.  Both transports feed CL_BrowserUpsert with
 * a fully-populated server_info_t; the OOB transport additionally
 * exposes CL_BrowserHandleInfoResponse as a wrapper that parses the
 * kv blob first.
 *
 * Discovery surfaces *untrusted* candidates.  Anyone on the network
 * can advertise a server with any name.  Trust enforcement happens at
 * the connect handshake, not here.
 *
 * Time: CL_BrowserTick(now_ms) must be called once per main-loop
 * iteration to advance the internal clock and run the rate-limited
 * purge of expired entries.  Tests that don't care about expiry can
 * skip Tick; CL_BrowserUpsert lazily backstops with clock_gettime.
 */

#include "../common/shared.h"
#include "../common/net.h"

#define SERVER_BROWSE_MAX_ENTRIES   64
#define SERVER_BROWSE_HOSTNAME_LEN  64
#define SERVER_BROWSE_MAP_LEN       64
#define SERVER_BROWSE_EXPIRY_MS     5000
#define SERVER_BROWSE_PURGE_PERIOD_MS 1000

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

/* Send the OOB getinfo broadcast.  Replies arrive asynchronously via
 * the OOB packet pump (CL_BrowserHandleInfoResponse). */
void CL_RefreshServerList(void);

/* Per-frame housekeeping: caches `now_ms` so subsequent Upsert calls
 * stamp entries with the same time, and runs the expired-entry sweep
 * on a 1 Hz cadence (not every frame). */
void CL_BrowserTick(DWORD now_ms);

/* Parse an OOB infoResponse kv blob into a server_info_t (single pass)
 * and Upsert.  The address is taken from the `from` argument, not from
 * the blob. */
void CL_BrowserHandleInfoResponse(const netadr_t *from,
                                  const char *payload,
                                  int len);

/* Shared upsert primitive used by both the OOB-broadcast path and the
 * mDNS path (cl_mdns.c).  Caller passes a fully-populated server_info_t
 * including the address field.  The list key is info->address (type,
 * ip, port tuple).
 *
 * Snapshot semantics: every call replaces all fields.  Callers that
 * mean "merge non-empty fields" should read the current entry first
 * via CL_BrowserList and copy through unchanged values. */
void CL_BrowserUpsert(const server_info_t *info);

/* Drop entries whose last_seen_ms is older than SERVER_BROWSE_EXPIRY_MS.
 * Normally driven from CL_BrowserTick on a rate-limited schedule; safe
 * to call directly from tests. */
void CL_BrowserPurgeExpired(void);

/* Return a pointer to the current list and its length.  Valid until
 * the next call to any other CL_Browser* function. */
const server_info_t *CL_BrowserList(int *count);

#endif /* cl_browser_h */
