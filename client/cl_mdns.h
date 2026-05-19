#ifndef cl_mdns_h
#define cl_mdns_h

/*
 * cl_mdns.h - Client-side mDNS/DNS-SD service browser.
 *
 * Discovers `_openwarcraft3._udp.local.` services on the LAN and feeds
 * each resolution into the shared server-browser model via
 * CL_BrowserUpsert.  mDNS is a parallel transport to the UDP broadcast
 * getinfo flow in cl_browser.c; both produce equally untrusted
 * candidates and the auth gate is the connect handshake, not the
 * discovery layer.
 *
 * Linux implementation uses Avahi.  Non-Linux platforms link the
 * stub variants and the feature is silently disabled.
 *
 * Requires avahi-daemon to be running on the host for browse results
 * to actually arrive.
 */

void CL_MDNS_Init(void);
void CL_MDNS_Tick(void);
void CL_MDNS_Shutdown(void);

#endif /* cl_mdns_h */
