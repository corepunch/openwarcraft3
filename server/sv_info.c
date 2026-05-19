/*
 * sv_info.c - Pure formatters for OOB server-info responses.
 *
 * No game-state globals, no StormLib, no renderer.  Linkable by the test
 * binary without pulling in transitive deps.
 */

#include <stdio.h>
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
    if ((size_t)n >= outlen) return (int)(outlen - 1);
    return n;
}
