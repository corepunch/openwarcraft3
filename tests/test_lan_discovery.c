/*
 * test_lan_discovery.c - Unit tests for the LAN server-browser layer.
 *
 * Covers the pure / unit-testable surface of Slice 1:
 *   - SV_BuildInfoResponseString: format, map basename reduction, defensive
 *     handling of NULL inputs.
 *   - CL_BrowserHandleInfoResponse: kv parsing, upsert behavior keyed by
 *     address tuple, multiple distinct entries.
 *
 * End-to-end behavior (real UDP sockets exchanging OOB packets, mDNS
 * advertise/browse) is integration-tested in the LXC build, not here.
 * Expiry of stale entries depends on clock_gettime and is also deferred
 * to integration.
 */

#include <string.h>
#include <arpa/inet.h>

#include "test_framework.h"

#include "../common/shared.h"
#include "../common/net.h"
#include "../common/net_oob.h"
#include "../server/sv_info.h"
#include "../client/cl_browser.h"

/* -----------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------- */

static netadr_t make_addr(BYTE a, BYTE b, BYTE c, BYTE d,
                          unsigned short port_host) {
    netadr_t adr;
    memset(&adr, 0, sizeof(adr));
    adr.type  = NA_IP;
    adr.ip[0] = a;
    adr.ip[1] = b;
    adr.ip[2] = c;
    adr.ip[3] = d;
    adr.port  = htons(port_host);
    return adr;
}

/* -----------------------------------------------------------------------
 * SV_BuildInfoResponseString
 * --------------------------------------------------------------------- */

static void test_buildinfo_includes_all_fields(void) {
    char buf[512];
    int n = SV_BuildInfoResponseString(buf, sizeof(buf),
                                       "myhost",
                                       "Maps\\Test\\Foo.w3m",
                                       2, 8, 1);
    ASSERT(n > 0);
    /* Must start with the infoResponse token. */
    ASSERT(strncmp(buf, OOB_INFORESPONSE, strlen(OOB_INFORESPONSE)) == 0);
    /* All keys present with their values. */
    ASSERT(strstr(buf, "\\hostname\\myhost") != NULL);
    ASSERT(strstr(buf, "\\map\\Foo.w3m") != NULL);  /* basename only */
    ASSERT(strstr(buf, "\\clients\\2") != NULL);
    ASSERT(strstr(buf, "\\maxclients\\8") != NULL);
    ASSERT(strstr(buf, "\\protocol\\1") != NULL);
}

static void test_buildinfo_strips_backslash_paths(void) {
    char buf[512];
    SV_BuildInfoResponseString(buf, sizeof(buf),
                               "h", "A\\B\\C\\Deep.w3m", 0, 0, 1);
    ASSERT(strstr(buf, "\\map\\Deep.w3m") != NULL);
    /* No part of the original path leaks through; if it did, the
     * embedded backslashes would corrupt the kv framing. */
    ASSERT(strstr(buf, "\\map\\A") == NULL);
}

static void test_buildinfo_strips_forward_slash_paths(void) {
    char buf[512];
    SV_BuildInfoResponseString(buf, sizeof(buf),
                               "h", "Maps/Foo.w3m", 0, 0, 1);
    ASSERT(strstr(buf, "\\map\\Foo.w3m") != NULL);
}

static void test_buildinfo_accepts_bare_basename(void) {
    char buf[512];
    SV_BuildInfoResponseString(buf, sizeof(buf),
                               "h", "BareName.w3m", 0, 0, 1);
    ASSERT(strstr(buf, "\\map\\BareName.w3m") != NULL);
}

static void test_buildinfo_handles_null_map(void) {
    char buf[512];
    int n = SV_BuildInfoResponseString(buf, sizeof(buf),
                                       "h", NULL, 0, 0, 1);
    ASSERT(n > 0);
    /* Empty map field: two backslashes back-to-back between map and clients. */
    ASSERT(strstr(buf, "\\map\\\\clients") != NULL);
}

static void test_buildinfo_handles_null_hostname(void) {
    char buf[512];
    int n = SV_BuildInfoResponseString(buf, sizeof(buf),
                                       NULL, "m.w3m", 1, 4, 1);
    ASSERT(n > 0);
    ASSERT(strstr(buf, "\\hostname\\\\map") != NULL);
}

/* -----------------------------------------------------------------------
 * CL_BrowserHandleInfoResponse
 * --------------------------------------------------------------------- */

static void test_browser_parses_minimal_infoResponse(void) {
    CL_BrowserInit();
    netadr_t from = make_addr(192, 168, 1, 10, PORT_SERVER);
    const char *payload =
        "\\" OOB_KEY_HOSTNAME "\\testhost"
        "\\" OOB_KEY_MAP "\\TestMap"
        "\\" OOB_KEY_CLIENTS "\\3"
        "\\" OOB_KEY_MAXCLIENTS "\\8"
        "\\" OOB_KEY_PROTOCOL "\\1";
    CL_BrowserHandleInfoResponse(&from, payload, (int)strlen(payload));

    int count = -1;
    const server_info_t *list = CL_BrowserList(&count);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(list[0].hostname, "testhost");
    ASSERT_STR_EQ(list[0].map, "TestMap");
    ASSERT_EQ_INT(list[0].clients, 3);
    ASSERT_EQ_INT(list[0].maxclients, 8);
    ASSERT_EQ_INT(list[0].protocol, 1);
}

static void test_browser_upserts_same_address(void) {
    CL_BrowserInit();
    netadr_t from = make_addr(10, 0, 0, 5, PORT_SERVER);
    const char *p1 =
        "\\hostname\\v1\\map\\m\\clients\\1\\maxclients\\4\\protocol\\1";
    const char *p2 =
        "\\hostname\\v2\\map\\m\\clients\\2\\maxclients\\4\\protocol\\1";
    CL_BrowserHandleInfoResponse(&from, p1, (int)strlen(p1));
    CL_BrowserHandleInfoResponse(&from, p2, (int)strlen(p2));

    int count = -1;
    const server_info_t *list = CL_BrowserList(&count);
    ASSERT_EQ_INT(count, 1);             /* upsert, not duplicate */
    ASSERT_STR_EQ(list[0].hostname, "v2");
    ASSERT_EQ_INT(list[0].clients, 2);
}

static void test_browser_distinct_addresses_get_distinct_entries(void) {
    CL_BrowserInit();
    netadr_t a = make_addr(192, 168, 1, 1, PORT_SERVER);
    netadr_t b = make_addr(192, 168, 1, 2, PORT_SERVER);
    const char *p =
        "\\hostname\\h\\map\\m\\clients\\0\\maxclients\\4\\protocol\\1";
    CL_BrowserHandleInfoResponse(&a, p, (int)strlen(p));
    CL_BrowserHandleInfoResponse(&b, p, (int)strlen(p));

    int count = -1;
    CL_BrowserList(&count);
    ASSERT_EQ_INT(count, 2);
}

static void test_browser_distinct_ports_get_distinct_entries(void) {
    CL_BrowserInit();
    netadr_t a = make_addr(192, 168, 1, 1, PORT_SERVER);
    netadr_t b = make_addr(192, 168, 1, 1, PORT_SERVER + 1);
    const char *p =
        "\\hostname\\h\\map\\m\\clients\\0\\maxclients\\4\\protocol\\1";
    CL_BrowserHandleInfoResponse(&a, p, (int)strlen(p));
    CL_BrowserHandleInfoResponse(&b, p, (int)strlen(p));

    int count = -1;
    CL_BrowserList(&count);
    ASSERT_EQ_INT(count, 2);
}

static void test_browser_ignores_unknown_keys(void) {
    CL_BrowserInit();
    netadr_t from = make_addr(127, 0, 0, 1, PORT_SERVER);
    const char *payload =
        "\\hostname\\h\\unknownkey\\junk\\map\\m"
        "\\clients\\0\\maxclients\\4\\protocol\\1";
    CL_BrowserHandleInfoResponse(&from, payload, (int)strlen(payload));

    int count = -1;
    const server_info_t *list = CL_BrowserList(&count);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(list[0].hostname, "h");
    ASSERT_STR_EQ(list[0].map, "m");
}

static void test_browser_partial_payload_still_parses_known_keys(void) {
    CL_BrowserInit();
    netadr_t from = make_addr(127, 0, 0, 1, PORT_SERVER);
    /* Only hostname and protocol; other fields stay at defaults. */
    const char *payload = "\\hostname\\partial\\protocol\\1";
    CL_BrowserHandleInfoResponse(&from, payload, (int)strlen(payload));

    int count = -1;
    const server_info_t *list = CL_BrowserList(&count);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(list[0].hostname, "partial");
    ASSERT_EQ_INT(list[0].protocol, 1);
    /* Untouched fields are zero / empty. */
    ASSERT_EQ_INT(list[0].clients, 0);
    ASSERT_STR_EQ(list[0].map, "");
}

/* -----------------------------------------------------------------------
 * Round-trip: server formatter + client parser
 * --------------------------------------------------------------------- */

static void test_server_formatter_feeds_client_parser(void) {
    char wire[512];
    int n = SV_BuildInfoResponseString(wire, sizeof(wire),
                                       "loopbox", "Map.w3m", 4, 8, 1);
    ASSERT(n > 0);

    /* Skip past the leading OOB_INFORESPONSE token; the client gets
     * the kv blob portion (this matches what cl_main.c does after
     * recognizing the token). */
    int token_len = (int)strlen(OOB_INFORESPONSE);
    ASSERT(n > token_len);
    const char *kv_blob = wire + token_len;
    int kv_len = n - token_len;

    CL_BrowserInit();
    netadr_t from = make_addr(127, 0, 0, 1, PORT_SERVER);
    CL_BrowserHandleInfoResponse(&from, kv_blob, kv_len);

    int count = -1;
    const server_info_t *list = CL_BrowserList(&count);
    ASSERT_EQ_INT(count, 1);
    ASSERT_STR_EQ(list[0].hostname, "loopbox");
    ASSERT_STR_EQ(list[0].map, "Map.w3m");
    ASSERT_EQ_INT(list[0].clients, 4);
    ASSERT_EQ_INT(list[0].maxclients, 8);
    ASSERT_EQ_INT(list[0].protocol, 1);
}

/* -----------------------------------------------------------------------
 * Suite entry
 * --------------------------------------------------------------------- */

void run_lan_discovery_tests(void) {
    RUN_TEST(test_buildinfo_includes_all_fields);
    RUN_TEST(test_buildinfo_strips_backslash_paths);
    RUN_TEST(test_buildinfo_strips_forward_slash_paths);
    RUN_TEST(test_buildinfo_accepts_bare_basename);
    RUN_TEST(test_buildinfo_handles_null_map);
    RUN_TEST(test_buildinfo_handles_null_hostname);
    RUN_TEST(test_browser_parses_minimal_infoResponse);
    RUN_TEST(test_browser_upserts_same_address);
    RUN_TEST(test_browser_distinct_addresses_get_distinct_entries);
    RUN_TEST(test_browser_distinct_ports_get_distinct_entries);
    RUN_TEST(test_browser_ignores_unknown_keys);
    RUN_TEST(test_browser_partial_payload_still_parses_known_keys);
    RUN_TEST(test_server_formatter_feeds_client_parser);
}
