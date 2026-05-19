/*
 * cl_browser.c - LAN server browser implementation.
 *
 * Sends OOB getinfo broadcasts on demand, parses infoResponse replies
 * into a fixed-size in-memory list, expires stale entries.  No I/O,
 * no UI: this module is the model layer only.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>

#include "cl_browser.h"
#include "../common/net.h"
#include "../common/net_oob.h"

static server_info_t browse_entries[SERVER_BROWSE_MAX_ENTRIES];
static int browse_count = 0;

static DWORD CL_BrowserNowMs(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (DWORD)((unsigned long long)ts.tv_sec * 1000ULL +
                   (unsigned long long)ts.tv_nsec / 1000000ULL);
}

void CL_BrowserInit(void) {
    memset(browse_entries, 0, sizeof(browse_entries));
    browse_count = 0;
}

void CL_RefreshServerList(void) {
    netadr_t bcast;
    memset(&bcast, 0, sizeof(bcast));
    bcast.type  = NA_BROADCAST;
    bcast.ip[0] = 255;
    bcast.ip[1] = 255;
    bcast.ip[2] = 255;
    bcast.ip[3] = 255;
    bcast.port  = htons(PORT_SERVER);

    Netchan_OutOfBandPrint(NS_CLIENT, bcast,
                           OOB_GETINFO " %d", PROTOCOL_VERSION);
}

/* Copy a value substring (between two backslashes or end-of-payload) into
 * dst with bounded length and null termination. */
static void copy_value(char *dst, int dstcap, const char *src, int srclen) {
    int n = srclen < (dstcap - 1) ? srclen : (dstcap - 1);
    if (n < 0) n = 0;
    memcpy(dst, src, n);
    dst[n] = '\0';
}

/* Look up the value for `key` in a Quake3-style backslash-delimited
 * key/value blob.  The blob looks like:
 *
 *     \k1\v1\k2\v2...
 *
 * (leading backslash is part of the format).  Returns true and fills
 * dst on hit; returns false on miss without touching dst. */
static bool kv_get(const char *blob, int bloblen,
                   const char *key, char *dst, int dstcap) {
    int keylen = (int)strlen(key);
    int i = 0;
    while (i < bloblen) {
        if (blob[i] != '\\') {
            i++;
            continue;
        }
        // At a separator: token is between i+1 and the next backslash.
        int kstart = i + 1;
        int kend = kstart;
        while (kend < bloblen && blob[kend] != '\\') kend++;
        int klen = kend - kstart;
        // Value runs from after the next backslash to the one after that.
        int vstart = kend + 1;
        int vend = vstart;
        while (vend < bloblen && blob[vend] != '\\') vend++;
        if (klen == keylen && memcmp(blob + kstart, key, keylen) == 0) {
            copy_value(dst, dstcap, blob + vstart, vend - vstart);
            return true;
        }
        // Advance to the next key separator (the one before vend if present).
        i = vend;
    }
    return false;
}

/* Find an existing entry matching from, or allocate a fresh one.
 * Returns NULL if the list is full and no expired slot can be reused. */
static server_info_t *find_or_alloc(const netadr_t *from) {
    for (int i = 0; i < browse_count; i++) {
        const netadr_t *a = &browse_entries[i].address;
        if (a->type == from->type &&
            memcmp(a->ip, from->ip, sizeof(a->ip)) == 0 &&
            a->port == from->port) {
            return &browse_entries[i];
        }
    }
    if (browse_count < SERVER_BROWSE_MAX_ENTRIES) {
        server_info_t *slot = &browse_entries[browse_count++];
        memset(slot, 0, sizeof(*slot));
        slot->address = *from;
        return slot;
    }
    return NULL;
}

void CL_BrowserUpsert(const netadr_t *from,
                      const char *hostname,
                      const char *map,
                      DWORD clients,
                      DWORD maxclients,
                      int protocol) {
    server_info_t *slot = find_or_alloc(from);
    if (!slot) {
        fprintf(stderr, "CL_BrowserUpsert: server list full\n");
        return;
    }
    if (hostname) {
        strncpy(slot->hostname, hostname, sizeof(slot->hostname) - 1);
        slot->hostname[sizeof(slot->hostname) - 1] = '\0';
    } else {
        slot->hostname[0] = '\0';
    }
    if (map) {
        strncpy(slot->map, map, sizeof(slot->map) - 1);
        slot->map[sizeof(slot->map) - 1] = '\0';
    } else {
        slot->map[0] = '\0';
    }
    slot->clients     = clients;
    slot->maxclients  = maxclients;
    slot->protocol    = protocol;
    slot->last_seen_ms = CL_BrowserNowMs();
}

void CL_BrowserHandleInfoResponse(const netadr_t *from,
                                  const char *payload,
                                  int len) {
    char hostname[SERVER_BROWSE_HOSTNAME_LEN] = "";
    char map[SERVER_BROWSE_MAP_LEN] = "";
    DWORD clients = 0, maxclients = 0;
    int protocol = 0;

    char buf[64];
    if (kv_get(payload, len, OOB_KEY_HOSTNAME, buf, sizeof(buf))) {
        strncpy(hostname, buf, sizeof(hostname) - 1);
        hostname[sizeof(hostname) - 1] = '\0';
    }
    if (kv_get(payload, len, OOB_KEY_MAP, buf, sizeof(buf))) {
        strncpy(map, buf, sizeof(map) - 1);
        map[sizeof(map) - 1] = '\0';
    }
    if (kv_get(payload, len, OOB_KEY_CLIENTS, buf, sizeof(buf))) {
        clients = (DWORD)strtoul(buf, NULL, 10);
    }
    if (kv_get(payload, len, OOB_KEY_MAXCLIENTS, buf, sizeof(buf))) {
        maxclients = (DWORD)strtoul(buf, NULL, 10);
    }
    if (kv_get(payload, len, OOB_KEY_PROTOCOL, buf, sizeof(buf))) {
        protocol = (int)strtol(buf, NULL, 10);
    }

    CL_BrowserUpsert(from, hostname, map, clients, maxclients, protocol);
}

void CL_BrowserPurgeExpired(void) {
    DWORD now = CL_BrowserNowMs();
    int write = 0;
    for (int read = 0; read < browse_count; read++) {
        DWORD age = now - browse_entries[read].last_seen_ms;
        if (age <= SERVER_BROWSE_EXPIRY_MS) {
            if (write != read) {
                browse_entries[write] = browse_entries[read];
            }
            write++;
        }
    }
    browse_count = write;
}

const server_info_t *CL_BrowserList(int *count) {
    if (count) *count = browse_count;
    return browse_entries;
}
