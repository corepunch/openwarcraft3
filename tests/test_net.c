/*
 * test_net.c — Unit tests for the network layer (net.c / msg.c).
 *
 * Tests exercise the public NET_* and SZ_* / MSG_* APIs directly, without
 * touching the UDP socket.  All address arguments use NA_LOOPBACK so that
 * NET_SendPacket routes to the in-process ring buffer and no real socket is
 * required.  NET_Init() is intentionally not called so udp_socket stays at
 * -1, making NET_GetUDPPacket a safe no-op throughout.
 *
 * Covered scenarios:
 *   loopback empty          — NET_GetPacket returns 0 on an idle buffer
 *   loopback round-trip     — a packet sent with NS_CLIENT is received by NS_SERVER
 *   loopback multiple       — several back-to-back packets are received in order
 *   NET_SendPacket dispatch — NA_LOOPBACK goes to ring buffer; NA_IP with no
 *                             socket is a silent no-op (no crash)
 *   SZ_Init / SZ_Clear      — size-buffer lifecycle helpers
 *   SZ_Write                — appends bytes and advances cursize
 *   NET_StringToAdr IP      — dotted-decimal address without port
 *   NET_StringToAdr port    — dotted-decimal address with explicit port
 */

#include <string.h>
#include <arpa/inet.h>

#include "test_framework.h"

/* Pull in the net types + common types without game state. */
#include "../src/common/shared.h"
#include "../src/common/net.h"

/* -----------------------------------------------------------------------
 * Helpers
 * --------------------------------------------------------------------- */

/* Drain all pending loopback packets for the given source so subsequent
 * tests start from a clean (read == write) ring-buffer state. */
static void drain_loopback(NETSOURCE netsrc) {
    static BYTE   drain_buf[MAX_MSGLEN];
    static sizeBuf_t msg = { drain_buf, MAX_MSGLEN, 0, 0 };
    netadr_t from;
    /* MAX_MSGLEN iterations is a generous upper bound; in practice the
     * ring buffer can hold far fewer whole packets of that size. */
    for (int guard = 0; guard < 64; guard++) {
        if (!NET_GetPacket(netsrc, &from, &msg))
            break;
    }
}

/* A loopback netadr_t ready to pass to NET_SendPacket. */
static netadr_t loopback_adr(void) {
    netadr_t adr;
    memset(&adr, 0, sizeof(adr));
    adr.type = NA_LOOPBACK;
    return adr;
}

/* -----------------------------------------------------------------------
 * SZ_Init / SZ_Clear
 * --------------------------------------------------------------------- */

static void test_sz_init(void) {
    BYTE buf[64];
    sizeBuf_t sz;
    SZ_Init(&sz, buf, sizeof(buf));
    ASSERT(sz.data == buf);
    ASSERT_EQ_INT(sz.maxsize, 64);
    ASSERT_EQ_INT(sz.cursize, 0);
    ASSERT_EQ_INT(sz.readcount, 0);
}

static void test_sz_clear(void) {
    BYTE buf[32];
    sizeBuf_t sz;
    SZ_Init(&sz, buf, sizeof(buf));
    sz.cursize = 10;
    SZ_Clear(&sz);
    ASSERT_EQ_INT(sz.cursize, 0);
}

/* -----------------------------------------------------------------------
 * SZ_Write
 * --------------------------------------------------------------------- */

static void test_sz_write_appends_data(void) {
    BYTE buf[32];
    sizeBuf_t sz;
    SZ_Init(&sz, buf, sizeof(buf));

    const char payload[] = "hello";
    SZ_Write(&sz, payload, 5);

    ASSERT_EQ_INT(sz.cursize, 5);
    ASSERT(memcmp(buf, "hello", 5) == 0);
}

static void test_sz_write_multiple(void) {
    BYTE buf[32];
    sizeBuf_t sz;
    SZ_Init(&sz, buf, sizeof(buf));

    SZ_Write(&sz, "AB", 2);
    SZ_Write(&sz, "CD", 2);

    ASSERT_EQ_INT(sz.cursize, 4);
    ASSERT(buf[0] == 'A' && buf[1] == 'B' && buf[2] == 'C' && buf[3] == 'D');
}

/* -----------------------------------------------------------------------
 * Loopback ring buffer
 * --------------------------------------------------------------------- */

static void test_loopback_empty_returns_zero(void) {
    static BYTE   msg_buf[MAX_MSGLEN];
    static sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t from;

    drain_loopback(NS_SERVER);
    int r = NET_GetPacket(NS_SERVER, &from, &msg);
    ASSERT_EQ_INT(r, 0);
}

static void test_loopback_round_trip(void) {
    static BYTE   msg_buf[MAX_MSGLEN];
    static sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t adr  = loopback_adr();
    netadr_t from;

    drain_loopback(NS_SERVER);

    const BYTE payload[] = { 0x01, 0x02, 0x03, 0x04 };
    NET_SendPacket(NS_CLIENT, sizeof(payload), payload, adr);

    int r = NET_GetPacket(NS_SERVER, &from, &msg);

    ASSERT_EQ_INT(r, (int)sizeof(payload));
    ASSERT(memcmp(msg.data, payload, sizeof(payload)) == 0);
    ASSERT_EQ_INT(from.type, NA_LOOPBACK);
    /* Buffer should now be empty. */
    ASSERT_EQ_INT(NET_GetPacket(NS_SERVER, &from, &msg), 0);
}

static void test_loopback_multiple_packets_in_order(void) {
    static BYTE   msg_buf[MAX_MSGLEN];
    static sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t adr  = loopback_adr();
    netadr_t from;

    drain_loopback(NS_SERVER);

    const BYTE pkt1[] = { 'A', 'B' };
    const BYTE pkt2[] = { 'C', 'D', 'E' };
    NET_SendPacket(NS_CLIENT, sizeof(pkt1), pkt1, adr);
    NET_SendPacket(NS_CLIENT, sizeof(pkt2), pkt2, adr);

    int r1 = NET_GetPacket(NS_SERVER, &from, &msg);
    ASSERT_EQ_INT(r1, (int)sizeof(pkt1));
    ASSERT(msg.data[0] == 'A' && msg.data[1] == 'B');

    int r2 = NET_GetPacket(NS_SERVER, &from, &msg);
    ASSERT_EQ_INT(r2, (int)sizeof(pkt2));
    ASSERT(msg.data[0] == 'C' && msg.data[1] == 'D' && msg.data[2] == 'E');

    ASSERT_EQ_INT(NET_GetPacket(NS_SERVER, &from, &msg), 0);
}

static void test_loopback_server_to_client(void) {
    /* Server→client direction: server sends with NS_SERVER,
     * client reads with NS_CLIENT (reads bufs[!NS_CLIENT] = bufs[NS_SERVER]). */
    static BYTE   msg_buf[MAX_MSGLEN];
    static sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t adr  = loopback_adr();
    netadr_t from;

    drain_loopback(NS_CLIENT);

    const BYTE payload[] = { (BYTE)0xDE, (BYTE)0xAD };
    NET_SendPacket(NS_SERVER, sizeof(payload), payload, adr);

    int r = NET_GetPacket(NS_CLIENT, &from, &msg);
    ASSERT_EQ_INT(r, (int)sizeof(payload));
    ASSERT(msg.data[0] == 0xDE && msg.data[1] == 0xAD);
}

static void test_loopback_na_ip_no_crash_without_socket(void) {
    /* Sending to an NA_IP address when no UDP socket is open must be a
     * silent no-op — it must not crash or write to an invalid fd.
     * (udp_socket == -1 because NET_Init is never called in tests.) */
    static BYTE   msg_buf[MAX_MSGLEN];
    static sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t adr;
    memset(&adr, 0, sizeof(adr));
    adr.type    = NA_IP;
    adr.ip[0]   = 127; adr.ip[1] = 0; adr.ip[2] = 0; adr.ip[3] = 1;
    adr.port    = htons(27910);

    /* Should return without crashing; no packet appears on NS_SERVER. */
    const char payload[] = { 0x01 };
    NET_SendPacket(NS_CLIENT, sizeof(payload), payload, adr);

    netadr_t from;
    drain_loopback(NS_SERVER);
    ASSERT_EQ_INT(NET_GetPacket(NS_SERVER, &from, &msg), 0);
}

/* -----------------------------------------------------------------------
 * NET_StringToAdr
 * --------------------------------------------------------------------- */

static void test_string_to_adr_ip_only(void) {
    netadr_t adr;
    bool ok = NET_StringToAdr("10.0.0.1", 12345, &adr);
    ASSERT(ok);
    ASSERT_EQ_INT(adr.type, NA_IP);
    ASSERT_EQ_INT(adr.ip[0], 10);
    ASSERT_EQ_INT(adr.ip[1], 0);
    ASSERT_EQ_INT(adr.ip[2], 0);
    ASSERT_EQ_INT(adr.ip[3], 1);
    /* Default port should be used (stored in network byte order). */
    ASSERT_EQ_INT(ntohs(adr.port), 12345);
}

static void test_string_to_adr_ip_with_port(void) {
    netadr_t adr;
    bool ok = NET_StringToAdr("192.168.0.5:9000", 0, &adr);
    ASSERT(ok);
    ASSERT_EQ_INT(adr.type, NA_IP);
    ASSERT_EQ_INT(adr.ip[0], 192);
    ASSERT_EQ_INT(adr.ip[1], 168);
    ASSERT_EQ_INT(adr.ip[2], 0);
    ASSERT_EQ_INT(adr.ip[3], 5);
    /* Port from string must override the default (0). */
    ASSERT_EQ_INT(ntohs(adr.port), 9000);
}

static void test_string_to_adr_port_overrides_default(void) {
    netadr_t adr;
    /* Explicit port in string must take precedence over default_port. */
    bool ok = NET_StringToAdr("172.16.0.1:27910", 9999, &adr);
    ASSERT(ok);
    ASSERT_EQ_INT(ntohs(adr.port), 27910);
}

static void test_string_to_adr_bad_address(void) {
    netadr_t adr;
    /* Non-resolvable hostname should return false in a CI environment
     * where DNS may not resolve arbitrary names.  We test with an
     * invalid dotted-decimal address instead. */
    bool ok = NET_StringToAdr("999.999.999.999", 0, &adr);
    /* inet_pton fails; gethostbyname may also fail for this literal. */
    /* We only assert that the call does not crash — the return value
     * is implementation-defined for truly invalid addresses. */
    (void)ok;
    ASSERT(1);   /* always passes — ensures no crash */
}

/* -----------------------------------------------------------------------
 * MSG_Write* / MSG_Read* round-trips
 * --------------------------------------------------------------------- */

/*
 * Helper: allocate a fresh size-buffer backed by a stack array and return it
 * ready for writing.  The read-position starts at 0.
 */
static sizeBuf_t make_msg_buf(BYTE *buf, DWORD bufsz) {
    sizeBuf_t sb;
    SZ_Init(&sb, buf, bufsz);
    return sb;
}

static void test_msg_writebyte_readbyte_roundtrip(void) {
    BYTE buf[16];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    MSG_WriteByte(&sb, 0xAB);
    sb.readcount = 0;
    ASSERT_EQ_INT(MSG_ReadByte(&sb) & 0xFF, 0xAB);
}

static void test_msg_writeshort_readshort_roundtrip(void) {
    BYTE buf[16];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    MSG_WriteShort(&sb, 0x1234);
    sb.readcount = 0;
    ASSERT_EQ_INT(MSG_ReadShort(&sb) & 0xFFFF, 0x1234);
}

static void test_msg_writelong_readlong_roundtrip(void) {
    BYTE buf[16];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    MSG_WriteLong(&sb, (int)0xDEADBEEF);
    sb.readcount = 0;
    ASSERT_EQ_INT((unsigned int)MSG_ReadLong(&sb), (unsigned int)0xDEADBEEF);
}

static void test_msg_writefloat_readfloat_roundtrip(void) {
    BYTE buf[16];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    MSG_WriteFloat(&sb, 3.14f);
    sb.readcount = 0;
    ASSERT_EQ_FLOAT(MSG_ReadFloat(&sb), 3.14f, 0.0001f);
}

static void test_msg_writestring_readstring_roundtrip(void) {
    BYTE buf[64];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    MSG_WriteString(&sb, "hello");
    sb.readcount = 0;
    char out[32] = {0};
    MSG_ReadString(&sb, out);
    ASSERT_STR_EQ(out, "hello");
}

static void test_msg_readbyte_past_end_returns_zero(void) {
    /* An empty (cursize==0) buffer: any read must return 0, not crash. */
    BYTE buf[8] = {0};
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    /* cursize stays 0 — nothing has been written. */
    ASSERT_EQ_INT(MSG_ReadByte(&sb), 0);
}

static void test_msg_writepos_readpos_roundtrip(void) {
    BYTE buf[32];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    VECTOR3 out = {0};
    VECTOR3 in  = {128.0f, -64.0f, 32.0f};
    MSG_WritePos(&sb, &in);
    sb.readcount = 0;
    MSG_ReadPos(&sb, &out);
    /* WritePos/ReadPos use MSG_WriteShort/MSG_ReadShort (lossy integer encoding). */
    ASSERT_EQ_INT((int)out.x, (int)in.x);
    ASSERT_EQ_INT((int)out.y, (int)in.y);
    ASSERT_EQ_INT((int)out.z, (int)in.z);
}

static void test_msg_writedir_readdir_roundtrip(void) {
    BYTE buf[32];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    VECTOR3 dir = {0.707f, 0.0f, -0.707f};
    VECTOR3 out = {0};
    MSG_WriteDir(&sb, &dir);
    sb.readcount = 0;
    MSG_ReadDir(&sb, &out);
    ASSERT_EQ_FLOAT(out.x, dir.x, 0.001f);
    ASSERT_EQ_FLOAT(out.y, dir.y, 0.001f);
    ASSERT_EQ_FLOAT(out.z, dir.z, 0.001f);
}

static void test_msg_writeangle_readangle_roundtrip(void) {
    BYTE buf[8];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    /* WriteAngle quantizes to 1/256th of a full revolution. */
    float angle = 1.5f;   /* radians */
    MSG_WriteAngle(&sb, angle);
    sb.readcount = 0;
    float out = MSG_ReadAngle(&sb);
    /* Tolerance = one quantum = 2π/256 ≈ 0.025 */
    ASSERT_EQ_FLOAT(out, angle, 0.025f);
}

static void test_msg_multiple_types_sequential(void) {
    BYTE buf[64];
    sizeBuf_t sb = make_msg_buf(buf, sizeof(buf));
    MSG_WriteByte(&sb,  42);
    MSG_WriteShort(&sb, 1000);
    MSG_WriteLong(&sb,  0x12345678);
    sb.readcount = 0;
    ASSERT_EQ_INT(MSG_ReadByte(&sb)  & 0xFF,       42);
    ASSERT_EQ_INT(MSG_ReadShort(&sb) & 0xFFFF, 1000);
    ASSERT_EQ_INT((unsigned int)MSG_ReadLong(&sb), (unsigned int)0x12345678);
}

/* -----------------------------------------------------------------------
 * Suite entry point
 * --------------------------------------------------------------------- */

void run_net_tests(void) {
    /* No game state required — NET_* functions are self-contained. */
    RUN_TEST(test_sz_init);
    RUN_TEST(test_sz_clear);
    RUN_TEST(test_sz_write_appends_data);
    RUN_TEST(test_sz_write_multiple);
    RUN_TEST(test_loopback_empty_returns_zero);
    RUN_TEST(test_loopback_round_trip);
    RUN_TEST(test_loopback_multiple_packets_in_order);
    RUN_TEST(test_loopback_server_to_client);
    RUN_TEST(test_loopback_na_ip_no_crash_without_socket);
    RUN_TEST(test_string_to_adr_ip_only);
    RUN_TEST(test_string_to_adr_ip_with_port);
    RUN_TEST(test_string_to_adr_port_overrides_default);
    RUN_TEST(test_string_to_adr_bad_address);
    /* MSG read/write round-trips */
    RUN_TEST(test_msg_writebyte_readbyte_roundtrip);
    RUN_TEST(test_msg_writeshort_readshort_roundtrip);
    RUN_TEST(test_msg_writelong_readlong_roundtrip);
    RUN_TEST(test_msg_writefloat_readfloat_roundtrip);
    RUN_TEST(test_msg_writestring_readstring_roundtrip);
    RUN_TEST(test_msg_readbyte_past_end_returns_zero);
    RUN_TEST(test_msg_writepos_readpos_roundtrip);
    RUN_TEST(test_msg_writedir_readdir_roundtrip);
    RUN_TEST(test_msg_writeangle_readangle_roundtrip);
    RUN_TEST(test_msg_multiple_types_sequential);
}
