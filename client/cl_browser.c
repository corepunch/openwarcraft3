/*
 * cl_browser.c - LAN server browser implementation.
 *
 * Storage: flat array of SERVER_BROWSE_MAX_ENTRIES entries plus an
 * open-addressing hash index keyed on netadr_t for O(1) lookup
 * (the array scan in find_or_alloc was O(N) per upsert).
 *
 * Time: a `now_ms` static is updated by CL_BrowserTick once per main
 * loop iteration so that upserts within the same frame share a
 * timestamp and we don't pay clock_gettime per call.
 *
 * Parsing: kv blobs from the OOB transport are walked once into a
 * server_info_t; the mDNS transport feeds the same struct directly
 * from cl_mdns.c.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <arpa/inet.h>

#include "cl_browser.h"
#include "../common/net.h"
#include "../common/net_oob.h"

#define HASH_BUCKETS (SERVER_BROWSE_MAX_ENTRIES * 2)
#define HASH_EMPTY    0xFFFF

static server_info_t browse_entries[SERVER_BROWSE_MAX_ENTRIES];
static int           browse_count = 0;

/* hash_index[bucket] holds 0..SERVER_BROWSE_MAX_ENTRIES-1, or HASH_EMPTY.
 * Open addressing with linear probing.  HASH_BUCKETS = 128 so the load
 * factor caps at 0.5 and probe chains stay short. */
static unsigned short hash_index[HASH_BUCKETS];

static DWORD cl_browser_now_ms = 0;
static DWORD cl_browser_last_purge_ms = 0;
static bool  cl_browser_full_warned = false;

/* ---------------------------------------------------------------------- */

static DWORD clock_now_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (DWORD)((unsigned long long)ts.tv_sec * 1000ULL +
                   (unsigned long long)ts.tv_nsec / 1000000ULL);
}

/* Cached "now" for the current frame, or a one-shot clock_gettime if
 * CL_BrowserTick hasn't been called yet (tests, early init). */
static DWORD browser_now(void) {
    if (cl_browser_now_ms != 0) return cl_browser_now_ms;
    return clock_now_ms();
}

static DWORD addr_hash(const netadr_t *a) {
    /* FNV-1a over the 4 ip bytes + 2 port bytes + 1 type byte.
     * Cheap, well-distributed for our key space. */
    DWORD h = 2166136261u;
    h = (h ^ (DWORD)a->type)   * 16777619u;
    h = (h ^ (DWORD)a->ip[0])  * 16777619u;
    h = (h ^ (DWORD)a->ip[1])  * 16777619u;
    h = (h ^ (DWORD)a->ip[2])  * 16777619u;
    h = (h ^ (DWORD)a->ip[3])  * 16777619u;
    h = (h ^ ((DWORD)a->port & 0xFF))   * 16777619u;
    h = (h ^ (((DWORD)a->port >> 8) & 0xFF)) * 16777619u;
    return h;
}

static bool addr_eq(const netadr_t *a, const netadr_t *b) {
    return a->type == b->type &&
           a->port == b->port &&
           memcmp(a->ip, b->ip, sizeof(a->ip)) == 0;
}

static int hash_lookup(const netadr_t *from) {
    DWORD h = addr_hash(from);
    for (int probe = 0; probe < HASH_BUCKETS; probe++) {
        DWORD bucket = (h + (DWORD)probe) % HASH_BUCKETS;
        unsigned short idx = hash_index[bucket];
        if (idx == HASH_EMPTY) return -1;
        if (idx < (unsigned short)browse_count &&
            addr_eq(&browse_entries[idx].address, from)) {
            return (int)idx;
        }
    }
    return -1;
}

static void hash_insert(const netadr_t *from, int slot) {
    DWORD h = addr_hash(from);
    for (int probe = 0; probe < HASH_BUCKETS; probe++) {
        DWORD bucket = (h + (DWORD)probe) % HASH_BUCKETS;
        if (hash_index[bucket] == HASH_EMPTY) {
            hash_index[bucket] = (unsigned short)slot;
            return;
        }
    }
}

static void hash_rebuild(void) {
    for (int i = 0; i < HASH_BUCKETS; i++) hash_index[i] = HASH_EMPTY;
    for (int i = 0; i < browse_count; i++) {
        hash_insert(&browse_entries[i].address, i);
    }
}

/* ---------------------------------------------------------------------- */

void CL_BrowserInit(void) {
    memset(browse_entries, 0, sizeof(browse_entries));
    for (int i = 0; i < HASH_BUCKETS; i++) hash_index[i] = HASH_EMPTY;
    browse_count = 0;
    cl_browser_now_ms = 0;
    cl_browser_last_purge_ms = 0;
    cl_browser_full_warned = false;
}

void CL_RefreshServerList(void) {
    netadr_t bcast;
    bcast.type  = NA_BROADCAST;
    bcast.ip[0] = 255;
    bcast.ip[1] = 255;
    bcast.ip[2] = 255;
    bcast.ip[3] = 255;
    bcast.port  = htons(PORT_SERVER);

    Netchan_OutOfBandPrint(NS_CLIENT, bcast,
                           OOB_GETINFO " %d", PROTOCOL_VERSION);
}

void CL_BrowserTick(DWORD now_ms) {
    cl_browser_now_ms = now_ms;
    if (now_ms - cl_browser_last_purge_ms >= SERVER_BROWSE_PURGE_PERIOD_MS) {
        CL_BrowserPurgeExpired();
        cl_browser_last_purge_ms = now_ms;
    }
}

/* Copy into a fixed-size field with bounded length + null term.  Avoids
 * the strncpy zero-pad. */
static void copy_field(char *dst, int dstcap, const char *src) {
    if (!src) {
        dst[0] = '\0';
        return;
    }
    int len = 0;
    while (len < dstcap - 1 && src[len] != '\0') len++;
    memcpy(dst, src, (size_t)len);
    dst[len] = '\0';
}

/* Single-pass parse of an OOB infoResponse kv blob into `out`.
 *
 * Blob shape: "\k1\v1\k2\v2..." (leading backslash is part of the
 * format).  Unknown keys are silently skipped (forward-compat).
 *
 * Replaces the previous per-key kv_get loop which walked the blob 5x. */
static void parse_info_blob(const char *blob, int bloblen, server_info_t *out) {
    int i = 0;
    while (i < bloblen) {
        if (blob[i] != '\\') { i++; continue; }
        int kstart = i + 1;
        int kend = kstart;
        while (kend < bloblen && blob[kend] != '\\') kend++;
        int klen = kend - kstart;
        int vstart = kend + 1;
        int vend = vstart;
        while (vend < bloblen && blob[vend] != '\\') vend++;
        int vlen = vend - vstart;

        /* Dispatch on first char + key length to avoid 5 strcmps. */
        const char *k = blob + kstart;
        const char *v = blob + vstart;
        char vbuf[64];
        int vcap = vlen < (int)sizeof(vbuf) - 1 ? vlen : (int)sizeof(vbuf) - 1;
        memcpy(vbuf, v, (size_t)vcap);
        vbuf[vcap] = '\0';

        if (klen == 8 && memcmp(k, "hostname", 8) == 0) {
            copy_field(out->hostname, sizeof(out->hostname), vbuf);
        } else if (klen == 3 && memcmp(k, "map", 3) == 0) {
            copy_field(out->map, sizeof(out->map), vbuf);
        } else if (klen == 7 && memcmp(k, "clients", 7) == 0) {
            out->clients = (DWORD)strtoul(vbuf, NULL, 10);
        } else if (klen == 10 && memcmp(k, "maxclients", 10) == 0) {
            out->maxclients = (DWORD)strtoul(vbuf, NULL, 10);
        } else if (klen == 8 && memcmp(k, "protocol", 8) == 0) {
            out->protocol = (int)strtol(vbuf, NULL, 10);
        }
        i = vend;
    }
}

void CL_BrowserHandleInfoResponse(const netadr_t *from,
                                  const char *payload,
                                  int len) {
    server_info_t info;
    info.address = *from;
    info.hostname[0] = '\0';
    info.map[0] = '\0';
    info.clients = 0;
    info.maxclients = 0;
    info.protocol = 0;
    info.last_seen_ms = 0;  /* CL_BrowserUpsert overwrites */
    parse_info_blob(payload, len, &info);
    CL_BrowserUpsert(&info);
}

void CL_BrowserUpsert(const server_info_t *info) {
    int idx = hash_lookup(&info->address);
    server_info_t *slot;

    if (idx >= 0) {
        slot = &browse_entries[idx];
    } else if (browse_count < SERVER_BROWSE_MAX_ENTRIES) {
        int new_slot = browse_count++;
        slot = &browse_entries[new_slot];
        slot->address = info->address;
        hash_insert(&info->address, new_slot);
    } else {
        if (!cl_browser_full_warned) {
            fprintf(stderr,
                    "CL_BrowserUpsert: server list full at %d entries; "
                    "further upserts dropped silently\n",
                    SERVER_BROWSE_MAX_ENTRIES);
            cl_browser_full_warned = true;
        }
        return;
    }

    /* Direct field assignment, no memset+overwrite. */
    copy_field(slot->hostname, sizeof(slot->hostname), info->hostname);
    copy_field(slot->map,      sizeof(slot->map),      info->map);
    slot->clients      = info->clients;
    slot->maxclients   = info->maxclients;
    slot->protocol     = info->protocol;
    slot->last_seen_ms = browser_now();
}

void CL_BrowserPurgeExpired(void) {
    DWORD now = browser_now();
    int write = 0;
    bool dropped_any = false;
    for (int read = 0; read < browse_count; read++) {
        DWORD age = now - browse_entries[read].last_seen_ms;
        if (age <= SERVER_BROWSE_EXPIRY_MS) {
            if (write != read) {
                browse_entries[write] = browse_entries[read];
            }
            write++;
        } else {
            dropped_any = true;
        }
    }
    browse_count = write;
    if (dropped_any) {
        /* Re-warn next time we hit full. */
        cl_browser_full_warned = false;
        hash_rebuild();
    }
}

const server_info_t *CL_BrowserList(int *count) {
    if (count) *count = browse_count;
    return browse_entries;
}
