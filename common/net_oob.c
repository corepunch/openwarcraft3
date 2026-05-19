/*
 * net_oob.c - Helpers for parsing OOB packet tokens.
 *
 * Kept pure (no globals, no I/O, no transport coupling) so the test
 * binary can link this without dragging in the server's StormLib or
 * renderer dependencies.
 */

#include <string.h>

#include "net_oob.h"

bool OOB_TokenMatches(const char *payload, int len, const char *token) {
    if (!payload || !token || len <= 0) {
        return false;
    }
    int toklen = (int)strlen(token);
    if (toklen <= 0 || len < toklen) {
        return false;
    }
    if (memcmp(payload, token, toklen) != 0) {
        return false;
    }
    /* Boundary check: the byte AFTER the token must terminate it.
     * Otherwise "getinfo" would match "getinfoXYZ" or "connect" would
     * match "connectABC", and Slice 2's getchallenge/challenge tokens
     * would collide with each other via prefix match. */
    if (len == toklen) {
        return true;          /* token is the entire payload */
    }
    char next = payload[toklen];
    return next == ' ' || next == '\0';
}
