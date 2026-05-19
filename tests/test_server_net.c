#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "test_framework.h"

#include "../common/shared.h"
#include "../common/net.h"
#include "../server/server.h"

/* sv_init.c / sv_send.c globals (normally from sv_main.c). */
struct game_export *ge;
struct server sv;
struct server_static svs;

/* External symbols referenced by sv_init.c but unused in these tests. */
void SV_InitGameProgs(void) {}
void SV_ClearWorld(void) {}
void CM_LoadMap(LPCSTR mapFilename) { (void)mapFilename; }
LPDOODAD CM_GetDoodads(void) { return NULL; }
LPCMAPINFO CM_GetMapInfo(void) { return NULL; }

static struct game_export test_ge;

static void reset_server_state(int max_players) {
    memset(&sv, 0, sizeof(sv));
    memset(&svs, 0, sizeof(svs));
    memset(&test_ge, 0, sizeof(test_ge));
    test_ge.max_clients = max_players;
    ge = &test_ge;
}

static int open_client_socket(void) {
    int s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (s < 0)
        return -1;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    addr.sin_port = 0;
    if (bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(s);
        return -1;
    }
    return s;
}

static void send_connect_oob(int sock, unsigned short server_port) {
    enum {
        MAX_CONNECT_DATAGRAM_SIZE = 64,
        OOB_HEADER_SIZE = 4,
        CONNECT_TEXT_SIZE = 7
    };
    BYTE datagram[MAX_CONNECT_DATAGRAM_SIZE];
    DWORD msg_len = OOB_HEADER_SIZE + CONNECT_TEXT_SIZE; /* -1 + "connect" */
    DWORD packet_len = 4 + msg_len; /* net packet header + payload */
    int oob_marker = -1;
    datagram[0] = (BYTE)(msg_len & 0xFF);
    datagram[1] = (BYTE)((msg_len >> 8) & 0xFF);
    datagram[2] = (BYTE)((msg_len >> 16) & 0xFF);
    datagram[3] = (BYTE)((msg_len >> 24) & 0xFF);
    memcpy(datagram + 4, &oob_marker, sizeof(oob_marker));
    memcpy(datagram + 8, "connect", 7);

    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    to.sin_port = htons(server_port);

    (void)sendto(sock, datagram, packet_len, 0, (struct sockaddr *)&to, sizeof(to));
}

static BOOL recv_client_connect_oob(int sock) {
    enum {
        MAX_RECV_RETRIES = 40,
        RECV_POLL_DELAY_US = 5000
    };
    BYTE datagram[128];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    FOR_LOOP(i, MAX_RECV_RETRIES) {
        int r = recvfrom(sock, datagram, sizeof(datagram), 0, (struct sockaddr *)&from, &fromlen);
        if (r > 0) {
            if (r >= 4 + 4 + 14 && memcmp(datagram + 8, "client_connect", 14) == 0)
                return true;
            return false;
        }
        usleep(RECV_POLL_DELAY_US);
    }
    return false;
}

static void pump_server_connects(void) {
    enum {
        MAX_PACKETS_PER_PUMP = 64,
        MIN_CONNECT_MSG_SIZE = 11 /* -1 marker (4) + "connect" (7) */
    };
    BYTE msg_buf[MAX_MSGLEN];
    sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t from;
    int r;
    FOR_LOOP(i, MAX_PACKETS_PER_PUMP) {
        r = NET_GetPacket(NS_SERVER, &from, &msg);
        if (!r)
            break;
        if (r >= MIN_CONNECT_MSG_SIZE) {
            int hdr = 0;
            memcpy(&hdr, msg.data, sizeof(hdr));
            if (hdr == -1 && memcmp(msg.data + 4, "connect", 7) == 0)
                SV_DirectConnect(&from);
        }
    }
}

static void test_udp_multi_client_connects_register_distinct_slots(void) {
    int c1 = open_client_socket();
    int c2 = open_client_socket();
    ASSERT(c1 >= 0 && c2 >= 0);
    NET_Shutdown();
    ASSERT(NET_Init(PORT_SERVER + 9));
    reset_server_state(8);

    send_connect_oob(c1, PORT_SERVER + 9);
    send_connect_oob(c2, PORT_SERVER + 9);
    pump_server_connects();

    ASSERT_EQ_INT(svs.num_clients, 2);
    ASSERT_EQ_INT(svs.clients[0].state, cs_connected);
    ASSERT_EQ_INT(svs.clients[1].state, cs_connected);
    ASSERT(recv_client_connect_oob(c1));
    ASSERT(recv_client_connect_oob(c2));

    if (c1 >= 0) close(c1);
    if (c2 >= 0) close(c2);
    NET_Shutdown();
}

static void test_udp_connect_honors_ge_max_clients_limit(void) {
    int c1 = open_client_socket();
    int c2 = open_client_socket();
    int c3 = open_client_socket();
    ASSERT(c1 >= 0 && c2 >= 0 && c3 >= 0);
    NET_Shutdown();
    ASSERT(NET_Init(PORT_SERVER + 10));
    reset_server_state(2);

    send_connect_oob(c1, PORT_SERVER + 10);
    send_connect_oob(c2, PORT_SERVER + 10);
    send_connect_oob(c3, PORT_SERVER + 10);
    pump_server_connects();

    ASSERT_EQ_INT(svs.num_clients, 2);
    ASSERT(recv_client_connect_oob(c1));
    ASSERT(recv_client_connect_oob(c2));
    ASSERT(!recv_client_connect_oob(c3));

    if (c1 >= 0) close(c1);
    if (c2 >= 0) close(c2);
    if (c3 >= 0) close(c3);
    NET_Shutdown();
}

static void test_multicast_syncs_updates_to_all_connected_clients(void) {
    BYTE payload[] = { 0x11, 0x22, 0x33, 0x44 };
    VECTOR3 origin = { 0, 0, 0 };
    reset_server_state(4);
    SZ_Init(&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));
    FOR_LOOP(i, 3) {
        SZ_Init(&svs.clients[i].netchan.message,
                svs.clients[i].netchan.message_buf, MAX_MSGLEN);
    }
    svs.num_clients = 3;

    SZ_Write(&sv.multicast, payload, sizeof(payload));
    SV_Multicast(&origin, MULTICAST_ALL_R);

    FOR_LOOP(i, 3) {
        ASSERT_EQ_INT(svs.clients[i].netchan.message.cursize, (int)sizeof(payload));
        ASSERT(memcmp(svs.clients[i].netchan.message.data, payload, sizeof(payload)) == 0);
    }
    ASSERT_EQ_INT(sv.multicast.cursize, 0);
}

/* ----------------------------------------------------------------------
 * Slot lifecycle (Slice 2 commit 1)
 *
 * SV_DropClient transitions cs_connected -> cs_zombie, records drop_time,
 * bumps sv_advertised_state_version, sends an OOB disconnect to NA_IP
 * peers, and is a no-op on cs_free / cs_zombie slots.
 *
 * After ZOMBIE_TIME_MS, SV_RunZombieExpiry transitions cs_zombie -> cs_free.
 *
 * SV_DirectConnect prefers a cs_free slot before growing num_clients.
 * SV_FindClientByAddr skips cs_free slots, still matches cs_zombie
 * (so a reconnect during grace is rejected).
 * -------------------------------------------------------------------- */

static netadr_t make_ip_addr(BYTE a, BYTE b, BYTE c, BYTE d,
                             unsigned short host_port) {
    netadr_t adr;
    memset(&adr, 0, sizeof(adr));
    adr.type  = NA_IP;
    adr.ip[0] = a;
    adr.ip[1] = b;
    adr.ip[2] = c;
    adr.ip[3] = d;
    adr.port  = htons(host_port);
    return adr;
}

static void test_drop_client_transitions_to_zombie(void) {
    reset_server_state(4);
    netadr_t from = make_ip_addr(10, 0, 0, 1, 50000);
    SV_DirectConnect(&from);
    ASSERT_EQ_INT(svs.num_clients, 1);
    ASSERT_EQ_INT(svs.clients[0].state, cs_connected);

    DWORD ver_before = sv_advertised_state_version;
    svs.realtime = 12345;
    SV_DropClient(&svs.clients[0], "test");

    ASSERT_EQ_INT(svs.clients[0].state, cs_zombie);
    ASSERT_EQ_INT(svs.clients[0].drop_time, 12345);
    ASSERT(sv_advertised_state_version > ver_before);
    /* num_clients is NOT decremented; slot stays "owned" during grace. */
    ASSERT_EQ_INT(svs.num_clients, 1);
}

static void test_drop_client_is_idempotent(void) {
    reset_server_state(4);
    netadr_t from = make_ip_addr(10, 0, 0, 1, 50001);
    SV_DirectConnect(&from);
    SV_DropClient(&svs.clients[0], "test");
    DWORD ver_after_first = sv_advertised_state_version;

    /* Second drop on the same zombie slot is a no-op. */
    SV_DropClient(&svs.clients[0], "test");
    ASSERT_EQ_INT(svs.clients[0].state, cs_zombie);
    ASSERT_EQ_INT(sv_advertised_state_version, ver_after_first);
}

static void test_num_live_clients_counts_only_active(void) {
    reset_server_state(4);
    netadr_t a = make_ip_addr(10, 0, 0, 1, 50010);
    netadr_t b = make_ip_addr(10, 0, 0, 2, 50011);
    netadr_t c = make_ip_addr(10, 0, 0, 3, 50012);
    SV_DirectConnect(&a);
    SV_DirectConnect(&b);
    SV_DirectConnect(&c);
    ASSERT_EQ_INT(SV_NumLiveClients(), 3);

    SV_DropClient(&svs.clients[1], "test");
    ASSERT_EQ_INT(SV_NumLiveClients(), 2);   /* zombie excluded */
    ASSERT_EQ_INT(svs.num_clients, 3);       /* allocated count unchanged */
}

static void test_zombie_expires_after_grace_period(void) {
    reset_server_state(4);
    netadr_t from = make_ip_addr(10, 0, 0, 1, 50020);
    SV_DirectConnect(&from);
    svs.realtime = 1000;
    SV_DropClient(&svs.clients[0], "test");

    /* Within grace: still zombie. */
    svs.realtime = 1000 + ZOMBIE_TIME_MS - 1;
    SV_RunZombieExpiry();
    ASSERT_EQ_INT(svs.clients[0].state, cs_zombie);

    /* At exactly grace boundary: transitions. */
    svs.realtime = 1000 + ZOMBIE_TIME_MS;
    SV_RunZombieExpiry();
    ASSERT_EQ_INT(svs.clients[0].state, cs_free);
}

static void test_find_client_by_addr_skips_free_slots(void) {
    reset_server_state(4);
    netadr_t from = make_ip_addr(10, 0, 0, 1, 50030);
    SV_DirectConnect(&from);
    SV_DropClient(&svs.clients[0], "test");

    /* Still zombie: FindClientByAddr matches so reconnect is rejected. */
    LPCLIENT zombie_match = SV_FindClientByAddr(&from);
    ASSERT(zombie_match == &svs.clients[0]);

    /* Force the slot to cs_free and re-query: must NOT match (stale addr). */
    svs.clients[0].state = cs_free;
    LPCLIENT free_match = SV_FindClientByAddr(&from);
    ASSERT_NULL(free_match);
}

static void test_reconnect_during_zombie_grace_is_rejected(void) {
    reset_server_state(4);
    netadr_t from = make_ip_addr(10, 0, 0, 1, 50040);
    SV_DirectConnect(&from);
    SV_DropClient(&svs.clients[0], "test");

    /* Same address connects again while slot is still cs_zombie.
     * SV_DirectConnect's early-out via SV_FindClientByAddr blocks it,
     * so num_clients should stay at 1 and no second slot allocated. */
    SV_DirectConnect(&from);
    ASSERT_EQ_INT(svs.num_clients, 1);
    ASSERT_EQ_INT(svs.clients[0].state, cs_zombie);
}

static void test_direct_connect_reuses_free_slot(void) {
    reset_server_state(4);
    netadr_t a = make_ip_addr(10, 0, 0, 1, 50050);
    netadr_t b = make_ip_addr(10, 0, 0, 2, 50051);
    SV_DirectConnect(&a);
    SV_DirectConnect(&b);
    ASSERT_EQ_INT(svs.num_clients, 2);

    /* Drop slot 0, expire its grace, then a fresh address connects.
     * It should land in slot 0 (the cs_free hole) rather than slot 2. */
    SV_DropClient(&svs.clients[0], "test");
    svs.realtime += ZOMBIE_TIME_MS;
    SV_RunZombieExpiry();
    ASSERT_EQ_INT(svs.clients[0].state, cs_free);

    netadr_t c = make_ip_addr(10, 0, 0, 3, 50052);
    SV_DirectConnect(&c);
    ASSERT_EQ_INT(svs.num_clients, 2);           /* no growth */
    ASSERT_EQ_INT(svs.clients[0].state, cs_connected);
    ASSERT(svs.clients[0].netchan.remote_address.ip[3] == 3);
}

/* ----------------------------------------------------------------------
 * Per-client read timeout (Slice 2)
 * SV_RunClientTimeouts drops cs_connected / cs_spawned non-loopback
 * clients that haven't sent a packet in CLIENT_TIMEOUT_MS.
 * -------------------------------------------------------------------- */

static void test_timeout_drops_silent_client(void) {
    reset_server_state(4);
    netadr_t from = make_ip_addr(10, 0, 0, 1, 50100);
    svs.realtime = 1000;
    SV_DirectConnect(&from);
    ASSERT_EQ_INT(svs.clients[0].state, cs_connected);

    /* Just shy of timeout: still alive. */
    svs.realtime = 1000 + CLIENT_TIMEOUT_MS - 1;
    SV_RunClientTimeouts();
    ASSERT_EQ_INT(svs.clients[0].state, cs_connected);

    /* At/past timeout: dropped to zombie. */
    svs.realtime = 1000 + CLIENT_TIMEOUT_MS;
    SV_RunClientTimeouts();
    ASSERT_EQ_INT(svs.clients[0].state, cs_zombie);
}

static void test_timeout_resets_on_packet(void) {
    reset_server_state(4);
    netadr_t from = make_ip_addr(10, 0, 0, 1, 50101);
    svs.realtime = 1000;
    SV_DirectConnect(&from);

    /* Simulate a packet arriving just before timeout would fire. */
    svs.realtime = 1000 + CLIENT_TIMEOUT_MS - 1;
    svs.clients[0].last_packet_ms = svs.realtime;

    /* Advance another full timeout window: no timeout because the
     * clock was reset by the packet. */
    svs.realtime += CLIENT_TIMEOUT_MS - 1;
    SV_RunClientTimeouts();
    ASSERT_EQ_INT(svs.clients[0].state, cs_connected);
}

static void test_timeout_skips_loopback_client(void) {
    reset_server_state(4);
    /* Manually install a loopback client.  Loopback exempt from timeout. */
    LPCLIENT cl = &svs.clients[0];
    cl->state = cs_connected;
    cl->netchan.remote_address.type = NA_LOOPBACK;
    cl->last_packet_ms = 0;          /* "never" */
    svs.num_clients = 1;
    svs.realtime = 1000 + CLIENT_TIMEOUT_MS + 5000;
    SV_RunClientTimeouts();
    ASSERT_EQ_INT(cl->state, cs_connected);
}

void run_server_net_tests(void) {
    RUN_TEST(test_udp_multi_client_connects_register_distinct_slots);
    RUN_TEST(test_udp_connect_honors_ge_max_clients_limit);
    RUN_TEST(test_multicast_syncs_updates_to_all_connected_clients);
    RUN_TEST(test_drop_client_transitions_to_zombie);
    RUN_TEST(test_drop_client_is_idempotent);
    RUN_TEST(test_num_live_clients_counts_only_active);
    RUN_TEST(test_zombie_expires_after_grace_period);
    RUN_TEST(test_find_client_by_addr_skips_free_slots);
    RUN_TEST(test_reconnect_during_zombie_grace_is_rejected);
    RUN_TEST(test_direct_connect_reuses_free_slot);
    RUN_TEST(test_timeout_drops_silent_client);
    RUN_TEST(test_timeout_resets_on_packet);
    RUN_TEST(test_timeout_skips_loopback_client);
}
