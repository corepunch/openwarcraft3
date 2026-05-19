#ifndef sv_mdns_h
#define sv_mdns_h

/*
 * sv_mdns.h - Server-side mDNS/DNS-SD service advertisement.
 *
 * Publishes the listen-server as `_openwarcraft3._udp.local.` with a
 * TXT record carrying hostname, map, clients, maxclients, and the
 * protocol version.  Discovery via mDNS is a parallel transport to
 * the broadcast OOB getinfo flow in sv_main.c; both surface the same
 * untrusted candidate to a browsing client.
 *
 * Linux implementation uses Avahi.  Non-Linux platforms link the
 * stub variants and the feature is silently disabled.
 *
 * Requires avahi-daemon to be running on the host for advertisements
 * to actually be visible to other nodes on the LAN.
 */

#include "../common/shared.h"

void SV_MDNS_Init(unsigned short port);
void SV_MDNS_UpdateInfo(const char *map_path, DWORD clients, DWORD maxclients);
void SV_MDNS_Tick(void);
void SV_MDNS_Shutdown(void);

#endif /* sv_mdns_h */
