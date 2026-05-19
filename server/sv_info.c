/*
 * sv_info.c - Pure formatters for OOB server-info responses.
 *
 * No game-state globals, no StormLib, no renderer.  Linkable by the test
 * binary without pulling in transitive deps.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "sv_info.h"
#include "../common/net_oob.h"

int SV_BuildInfoResponseString(char *out, size_t outlen,
                               const char *hostname,
                               const char *map_path,
                               DWORD clients,
                               DWORD maxclients,
                               int protocol) {
    // Reduce map_path to its basename so embedded '\\' / '/' separators
    // don't collide with the kv blob framing.
    const char *map_base = map_path ? map_path : "";
    if (map_path) {
        for (const char *p = map_path; *p; p++) {
            if (*p == '\\' || *p == '/') {
                map_base = p + 1;
            }
        }
    }

    int n = snprintf(out, outlen,
        OOB_INFORESPONSE
        OOB_KV_SEPARATOR OOB_KEY_HOSTNAME   OOB_KV_SEPARATOR "%s"
        OOB_KV_SEPARATOR OOB_KEY_MAP        OOB_KV_SEPARATOR "%s"
        OOB_KV_SEPARATOR OOB_KEY_CLIENTS    OOB_KV_SEPARATOR "%u"
        OOB_KV_SEPARATOR OOB_KEY_MAXCLIENTS OOB_KV_SEPARATOR "%u"
        OOB_KV_SEPARATOR OOB_KEY_PROTOCOL   OOB_KV_SEPARATOR "%d",
        hostname ? hostname : "",
        map_base,
        clients,
        maxclients,
        protocol);

    if (n < 0) return 0;
    if ((size_t)n >= outlen) return outlen ? (int)(outlen - 1) : 0;
    return n;
}

int SV_ParseConnectVersion(const char *payload, int len) {
    if (!payload) return -1;
    int token_len = (int)(sizeof(OOB_CONNECT) - 1);
    if (len <= token_len)            return -1;
    if (payload[token_len] != ' ')   return -1;
    int vstart = token_len + 1;
    int vlen   = len - vstart;
    if (vlen <= 0)                   return -1;

    /* Bound the digits we copy.  A pathological caller could fabricate
     * a multi-KB version field; we don't care, only the first few
     * digits are meaningful. */
    char vbuf[16];
    if (vlen >= (int)sizeof(vbuf)) vlen = (int)sizeof(vbuf) - 1;
    memcpy(vbuf, payload + vstart, (size_t)vlen);
    vbuf[vlen] = '\0';
    if (vbuf[0] < '0' || vbuf[0] > '9') return -1;
    return atoi(vbuf);
}
