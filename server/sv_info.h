#ifndef sv_info_h
#define sv_info_h

/*
 * sv_info.h - Pure formatters for OOB server-info responses.
 *
 * Kept separate from sv_main.c so the test binary can link these
 * functions without dragging in renderer / StormLib / game-library
 * dependencies.
 */

#include "../common/shared.h"

/* Format a Quake3-style infoResponse key/value blob into `out`.
 *
 * - hostname:    server hostname as reported by gethostname (or fallback).
 * - map_path:    in-MPQ map path; will be reduced to its basename so the
 *                response framing is not corrupted by embedded backslashes.
 * - clients:     current connected client count.
 * - maxclients:  configured maximum client slots.
 * - protocol:    PROTOCOL_VERSION at compile time.
 *
 * Output begins with the OOB_INFORESPONSE token (no leading -1 sequence;
 * caller wraps the result via Netchan_OutOfBand).  Returns the number of
 * bytes written, excluding the terminating null. */
int SV_BuildInfoResponseString(char *out, size_t outlen,
                               const char *hostname,
                               const char *map_path,
                               DWORD clients,
                               DWORD maxclients,
                               int protocol);

/* Parse the wire form "connect <version>" out of an OOB connect payload.
 * Returns the parsed version, or -1 if the payload is malformed
 * (missing space, missing number, oversize number field). */
int SV_ParseConnectVersion(const char *payload, int len);

#endif /* sv_info_h */
