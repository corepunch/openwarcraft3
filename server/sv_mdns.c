/*
 * sv_mdns.c - mDNS/DNS-SD service advertisement for the listen server.
 *
 * Avahi-based on Linux; stubs on other platforms.  Service type is
 * `_openwarcraft3._udp` per the network plan.
 */

#include "sv_mdns.h"

#ifdef __linux__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <avahi-client/client.h>
#include <avahi-client/publish.h>
#include <avahi-common/simple-watch.h>
#include <avahi-common/error.h>
#include <avahi-common/strlst.h>
#include <avahi-common/alternative.h>
#include <avahi-common/malloc.h>

#include "../common/net.h"
#include "../common/net_oob.h"

#define MDNS_HOSTNAME_MAX        64
#define MDNS_MAP_MAX             64
#define MDNS_SERVICE_MAX         64
#define MDNS_TXT_FIELD_MAX       128
/* Cap collision-rename retries so a hostile / saturated mDNS network
 * cannot make us recurse without bound through alternative-name
 * suggestions.  Avahi's alternative-name suffix grows numerically so
 * a real collision is resolved well within this limit. */
#define MDNS_MAX_NAME_RETRIES    16

static AvahiSimplePoll  *sv_mdns_poll        = NULL;
static AvahiClient      *sv_mdns_client      = NULL;
static AvahiEntryGroup  *sv_mdns_group       = NULL;
static unsigned short    sv_mdns_port        = 0;
static char              sv_mdns_map[MDNS_MAP_MAX]                = "";
static DWORD             sv_mdns_clients     = 0;
static DWORD             sv_mdns_maxclients  = 0;
static char              sv_mdns_service_name[MDNS_SERVICE_MAX]   = "OpenWarcraft3";
static int               sv_mdns_name_retries = 0;

static void sv_mdns_create_service(AvahiClient *c);
static void sv_mdns_entry_group_callback(AvahiEntryGroup *g,
                                      AvahiEntryGroupState state,
                                      void *userdata);
static void sv_mdns_client_callback(AvahiClient *c,
                                 AvahiClientState state,
                                 void *userdata);

/* Reduce a path to its basename, copying into a bounded buffer. */
static void sv_mdns_basename(const char *path, char *out, size_t outlen) {
    const char *base = path ? path : "";
    if (path) {
        for (const char *p = path; *p; p++) {
            if (*p == '\\' || *p == '/') base = p + 1;
        }
    }
    strncpy(out, base, outlen - 1);
    out[outlen - 1] = '\0';
}

/* Cached at first use; gethostname doesn't change once the system is up. */
static const char *sv_mdns_hostname(void) {
    static char cached[MDNS_HOSTNAME_MAX];
    static int  filled = 0;
    if (!filled) {
        if (gethostname(cached, sizeof(cached)) != 0) {
            strncpy(cached, "owc3-server", sizeof(cached) - 1);
        }
        cached[sizeof(cached) - 1] = '\0';
        filled = 1;
    }
    return cached;
}

/* Build the TXT-record string list reflecting current server info.
 * Caller owns the result and must free it with avahi_string_list_free. */
static AvahiStringList *sv_mdns_build_txt_records(void) {
    const char *hostname = sv_mdns_hostname();

    char map_base[MDNS_MAP_MAX];
    sv_mdns_basename(sv_mdns_map, map_base, sizeof(map_base));

    char f_hostname[MDNS_TXT_FIELD_MAX];
    char f_map[MDNS_TXT_FIELD_MAX];
    char f_clients[MDNS_TXT_FIELD_MAX];
    char f_maxclients[MDNS_TXT_FIELD_MAX];
    char f_protocol[MDNS_TXT_FIELD_MAX];
    snprintf(f_hostname,   sizeof(f_hostname),   "hostname=%s", hostname);
    snprintf(f_map,        sizeof(f_map),        "map=%s", map_base);
    snprintf(f_clients,    sizeof(f_clients),    "clients=%u", sv_mdns_clients);
    snprintf(f_maxclients, sizeof(f_maxclients), "maxclients=%u", sv_mdns_maxclients);
    snprintf(f_protocol,   sizeof(f_protocol),   "protocol=%d", PROTOCOL_VERSION);

    AvahiStringList *l = NULL;
    l = avahi_string_list_add(l, f_hostname);
    l = avahi_string_list_add(l, f_map);
    l = avahi_string_list_add(l, f_clients);
    l = avahi_string_list_add(l, f_maxclients);
    l = avahi_string_list_add(l, f_protocol);
    return l;
}

static void sv_mdns_create_service(AvahiClient *c) {
    if (!sv_mdns_group) {
        sv_mdns_group = avahi_entry_group_new(c, sv_mdns_entry_group_callback, NULL);
        if (!sv_mdns_group) {
            fprintf(stderr, "SV_MDNS: avahi_entry_group_new failed: %s\n",
                    avahi_strerror(avahi_client_errno(c)));
            return;
        }
    }
    if (!avahi_entry_group_is_empty(sv_mdns_group)) {
        return;
    }

    AvahiStringList *txt = sv_mdns_build_txt_records();
    int r = avahi_entry_group_add_service_strlst(
        sv_mdns_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 0,
        sv_mdns_service_name, MDNS_SERVICE_TYPE,
        NULL, NULL, sv_mdns_port, txt);
    avahi_string_list_free(txt);

    if (r < 0) {
        if (r == AVAHI_ERR_COLLISION) {
            if (sv_mdns_name_retries >= MDNS_MAX_NAME_RETRIES) {
                fprintf(stderr, "SV_MDNS: gave up after %d collision retries\n",
                        sv_mdns_name_retries);
                return;
            }
            sv_mdns_name_retries++;
            char *new_name = avahi_alternative_service_name(sv_mdns_service_name);
            if (new_name) {
                strncpy(sv_mdns_service_name, new_name, sizeof(sv_mdns_service_name) - 1);
                sv_mdns_service_name[sizeof(sv_mdns_service_name) - 1] = '\0';
                avahi_free(new_name);
                avahi_entry_group_reset(sv_mdns_group);
                sv_mdns_create_service(c);
            }
            return;
        }
        fprintf(stderr, "SV_MDNS: add_service failed: %s\n", avahi_strerror(r));
        return;
    }

    r = avahi_entry_group_commit(sv_mdns_group);
    if (r < 0) {
        fprintf(stderr, "SV_MDNS: commit failed: %s\n", avahi_strerror(r));
    } else {
        /* Commit accepted: a future genuine collision can retry from
         * zero rather than carrying over from the previous boot's
         * attempts. */
        sv_mdns_name_retries = 0;
    }
}

static void sv_mdns_entry_group_callback(AvahiEntryGroup *g,
                                      AvahiEntryGroupState state,
                                      void *userdata) {
    (void)g;
    (void)userdata;
    if (state == AVAHI_ENTRY_GROUP_COLLISION) {
        if (sv_mdns_name_retries >= MDNS_MAX_NAME_RETRIES) {
            fprintf(stderr, "SV_MDNS: gave up after %d collision retries\n",
                    sv_mdns_name_retries);
            return;
        }
        sv_mdns_name_retries++;
        char *new_name = avahi_alternative_service_name(sv_mdns_service_name);
        if (new_name) {
            strncpy(sv_mdns_service_name, new_name, sizeof(sv_mdns_service_name) - 1);
            sv_mdns_service_name[sizeof(sv_mdns_service_name) - 1] = '\0';
            avahi_free(new_name);
            if (sv_mdns_client) {
                avahi_entry_group_reset(sv_mdns_group);
                sv_mdns_create_service(sv_mdns_client);
            }
        }
    } else if (state == AVAHI_ENTRY_GROUP_FAILURE) {
        fprintf(stderr, "SV_MDNS: entry group failure: %s\n",
                avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
    }
}

static void sv_mdns_client_callback(AvahiClient *c,
                                 AvahiClientState state,
                                 void *userdata) {
    (void)userdata;
    if (state == AVAHI_CLIENT_S_RUNNING) {
        sv_mdns_create_service(c);
    } else if (state == AVAHI_CLIENT_FAILURE) {
        fprintf(stderr, "SV_MDNS: client failure: %s\n",
                avahi_strerror(avahi_client_errno(c)));
    } else if (state == AVAHI_CLIENT_S_COLLISION ||
               state == AVAHI_CLIENT_S_REGISTERING) {
        if (sv_mdns_group) {
            avahi_entry_group_reset(sv_mdns_group);
        }
    }
}

void SV_MDNS_Init(unsigned short port) {
    if (port == 0) {
        /* Advertising port 0 is meaningless; the caller almost certainly
         * meant PORT_SERVER. Refuse rather than silently publishing a
         * useless record. */
        fprintf(stderr, "SV_MDNS: refusing to advertise port 0\n");
        return;
    }
    sv_mdns_port = port;
    sv_mdns_name_retries = 0;
    sv_mdns_poll = avahi_simple_poll_new();
    if (!sv_mdns_poll) {
        fprintf(stderr, "SV_MDNS: avahi_simple_poll_new failed\n");
        return;
    }
    int error;
    sv_mdns_client = avahi_client_new(avahi_simple_poll_get(sv_mdns_poll), 0,
                                    sv_mdns_client_callback, NULL, &error);
    if (!sv_mdns_client) {
        fprintf(stderr, "SV_MDNS: avahi_client_new failed: %s\n",
                avahi_strerror(error));
        avahi_simple_poll_free(sv_mdns_poll);
        sv_mdns_poll = NULL;
    }
}

void SV_MDNS_UpdateInfo(const char *map_path, DWORD clients, DWORD maxclients) {
    /* Compare to the cached state.  If nothing visible has changed,
     * skip the Avahi serialize/push entirely.  Callers should be able
     * to invoke this every frame without paying for it. */
    bool changed = false;
    if (map_path && strcmp(map_path, sv_mdns_map) != 0) {
        strncpy(sv_mdns_map, map_path, sizeof(sv_mdns_map) - 1);
        sv_mdns_map[sizeof(sv_mdns_map) - 1] = '\0';
        changed = true;
    }
    if (clients != sv_mdns_clients) {
        sv_mdns_clients = clients;
        changed = true;
    }
    if (maxclients != sv_mdns_maxclients) {
        sv_mdns_maxclients = maxclients;
        changed = true;
    }
    if (!changed) {
        return;
    }

    if (sv_mdns_group && !avahi_entry_group_is_empty(sv_mdns_group)) {
        AvahiStringList *txt = sv_mdns_build_txt_records();
        int r = avahi_entry_group_update_service_txt_strlst(
            sv_mdns_group, AVAHI_IF_UNSPEC, AVAHI_PROTO_INET, 0,
            sv_mdns_service_name, MDNS_SERVICE_TYPE, NULL, txt);
        avahi_string_list_free(txt);
        if (r < 0) {
            fprintf(stderr, "SV_MDNS: update_txt failed: %s\n", avahi_strerror(r));
        }
    }
}

void SV_MDNS_Tick(void) {
    if (sv_mdns_poll) {
        avahi_simple_poll_iterate(sv_mdns_poll, 0);
    }
}

void SV_MDNS_Shutdown(void) {
    if (sv_mdns_client) {
        avahi_client_free(sv_mdns_client);
        sv_mdns_client = NULL;
    }
    if (sv_mdns_poll) {
        avahi_simple_poll_free(sv_mdns_poll);
        sv_mdns_poll = NULL;
    }
    sv_mdns_group = NULL;
}

#else /* !__linux__ */

void SV_MDNS_Init(unsigned short port) { (void)port; }
void SV_MDNS_UpdateInfo(const char *m, DWORD c, DWORD mc) {
    (void)m; (void)c; (void)mc;
}
void SV_MDNS_Tick(void) {}
void SV_MDNS_Shutdown(void) {}

#endif
