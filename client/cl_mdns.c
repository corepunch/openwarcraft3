/*
 * cl_mdns.c - mDNS/DNS-SD service browser feeding the shared server list.
 *
 * Avahi-based on Linux; stubs elsewhere.  Resolves each discovered
 * service to its IPv4 address + port + TXT records, then calls
 * CL_BrowserUpsert (defined in cl_browser.c) to feed the same in-memory
 * list as the UDP-broadcast OOB path.
 */

#include "cl_mdns.h"

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/malloc.h>
#include <avahi-common/strlst.h>
#include <avahi-common/address.h>

#include "cl_browser.h"
#include "../common/shared.h"
#include "../common/net.h"
#include "../common/net_oob.h"

static AvahiSimplePoll     *cl_mdns_poll    = NULL;
static AvahiClient         *cl_mdns_client  = NULL;
static AvahiServiceBrowser *cl_mdns_browser = NULL;

static void cl_mdns_resolve_callback(AvahiServiceResolver *r,
                             AvahiIfIndex interface,
                             AvahiProtocol protocol,
                             AvahiResolverEvent event,
                             const char *name,
                             const char *type,
                             const char *domain,
                             const char *host_name,
                             const AvahiAddress *address,
                             uint16_t port,
                             AvahiStringList *txt,
                             AvahiLookupResultFlags flags,
                             void *userdata) {
    (void)interface;
    (void)protocol;
    (void)flags;
    (void)userdata;
    (void)name;
    (void)type;
    (void)domain;
    (void)host_name;

    if (event != AVAHI_RESOLVER_FOUND) {
        avahi_service_resolver_free(r);
        return;
    }
    /* netadr_t carries only 4 ip bytes; restrict to IPv4 for now.
     * IPv6 lands when netadr_t grows to support it (out of scope here). */
    if (address->proto != AVAHI_PROTO_INET) {
        avahi_service_resolver_free(r);
        return;
    }

    char hostname_v[64] = "";
    char map_v[64]      = "";
    DWORD clients_v     = 0;
    DWORD maxclients_v  = 0;
    int   protocol_v    = 0;

    for (AvahiStringList *cur = txt; cur; cur = avahi_string_list_get_next(cur)) {
        char  *key = NULL;
        char  *val = NULL;
        size_t vlen = 0;
        if (avahi_string_list_get_pair(cur, &key, &val, &vlen) == 0) {
            if (key && val) {
                if (strcmp(key, "hostname") == 0) {
                    strncpy(hostname_v, val, sizeof(hostname_v) - 1);
                    hostname_v[sizeof(hostname_v) - 1] = '\0';
                } else if (strcmp(key, "map") == 0) {
                    strncpy(map_v, val, sizeof(map_v) - 1);
                    map_v[sizeof(map_v) - 1] = '\0';
                } else if (strcmp(key, "clients") == 0) {
                    clients_v = (DWORD)strtoul(val, NULL, 10);
                } else if (strcmp(key, "maxclients") == 0) {
                    maxclients_v = (DWORD)strtoul(val, NULL, 10);
                } else if (strcmp(key, "protocol") == 0) {
                    protocol_v = (int)strtol(val, NULL, 10);
                }
            }
            avahi_free(key);
            avahi_free(val);
        }
    }

    netadr_t adr;
    memset(&adr, 0, sizeof(adr));
    adr.type = NA_IP;
    /* AvahiAddress.data.ipv4.address is in network byte order already. */
    memcpy(adr.ip, &address->data.ipv4.address, 4);
    adr.port = htons(port);

    CL_BrowserUpsert(&adr, hostname_v, map_v, clients_v, maxclients_v, protocol_v);

    avahi_service_resolver_free(r);
}

static void cl_mdns_browse_callback(AvahiServiceBrowser *b,
                            AvahiIfIndex interface,
                            AvahiProtocol protocol,
                            AvahiBrowserEvent event,
                            const char *name,
                            const char *type,
                            const char *domain,
                            AvahiLookupResultFlags flags,
                            void *userdata) {
    (void)b;
    (void)flags;
    (void)userdata;

    if (event == AVAHI_BROWSER_NEW) {
        /* Kick off a resolver; result lands in cl_mdns_resolve_callback. */
        if (!avahi_service_resolver_new(cl_mdns_client, interface, protocol,
                                        name, type, domain, AVAHI_PROTO_INET,
                                        0, cl_mdns_resolve_callback, NULL)) {
            fprintf(stderr, "CL_MDNS: resolver_new failed: %s\n",
                    avahi_strerror(avahi_client_errno(cl_mdns_client)));
        }
    }
    /* AVAHI_BROWSER_REMOVE / CACHE_EXHAUSTED / ALL_FOR_NOW / FAILURE: ignore.
     * Stale entries are pruned by CL_BrowserPurgeExpired, not by the browser. */
}

static void cl_mdns_client_callback(AvahiClient *c,
                            AvahiClientState state,
                            void *userdata) {
    (void)c;
    (void)userdata;
    if (state == AVAHI_CLIENT_FAILURE) {
        fprintf(stderr, "CL_MDNS: client failure: %s\n",
                avahi_strerror(avahi_client_errno(c)));
    }
}

void CL_MDNS_Init(void) {
    cl_mdns_poll = avahi_simple_poll_new();
    if (!cl_mdns_poll) {
        fprintf(stderr, "CL_MDNS: avahi_simple_poll_new failed\n");
        return;
    }
    int error;
    cl_mdns_client = avahi_client_new(avahi_simple_poll_get(cl_mdns_poll), 0,
                                    cl_mdns_client_callback, NULL, &error);
    if (!cl_mdns_client) {
        fprintf(stderr, "CL_MDNS: avahi_client_new failed: %s\n",
                avahi_strerror(error));
        avahi_simple_poll_free(cl_mdns_poll);
        cl_mdns_poll = NULL;
        return;
    }
    cl_mdns_browser = avahi_service_browser_new(cl_mdns_client,
                                              AVAHI_IF_UNSPEC,
                                              AVAHI_PROTO_INET,
                                              MDNS_SERVICE_TYPE,
                                              NULL, 0,
                                              cl_mdns_browse_callback, NULL);
    if (!cl_mdns_browser) {
        fprintf(stderr, "CL_MDNS: service_browser_new failed: %s\n",
                avahi_strerror(avahi_client_errno(cl_mdns_client)));
    }
}

void CL_MDNS_Tick(void) {
    if (cl_mdns_poll) {
        avahi_simple_poll_iterate(cl_mdns_poll, 0);
    }
}

void CL_MDNS_Shutdown(void) {
    if (cl_mdns_browser) {
        avahi_service_browser_free(cl_mdns_browser);
        cl_mdns_browser = NULL;
    }
    if (cl_mdns_client) {
        avahi_client_free(cl_mdns_client);
        cl_mdns_client = NULL;
    }
    if (cl_mdns_poll) {
        avahi_simple_poll_free(cl_mdns_poll);
        cl_mdns_poll = NULL;
    }
}

#else /* !__linux__ */

void CL_MDNS_Init(void)     {}
void CL_MDNS_Tick(void)     {}
void CL_MDNS_Shutdown(void) {}

#endif
