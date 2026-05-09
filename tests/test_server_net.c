#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "test_framework.h"

#include "../src/common/shared.h"
#include "../src/common/net.h"
#include "../src/server/server.h"

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
        OOB_HEADER_SIZE = 4,
        CONNECT_TEXT_SIZE = 7
    };
    BYTE datagram[64];
    DWORD msg_len = OOB_HEADER_SIZE + CONNECT_TEXT_SIZE; /* -1 + "connect" */
    DWORD packet_len = 4 + msg_len; /* net packet header + payload */
    int oob = -1;
    datagram[0] = (BYTE)(msg_len & 0xFF);
    datagram[1] = (BYTE)((msg_len >> 8) & 0xFF);
    datagram[2] = (BYTE)((msg_len >> 16) & 0xFF);
    datagram[3] = (BYTE)((msg_len >> 24) & 0xFF);
    memcpy(datagram + 4, &oob, sizeof(oob));
    memcpy(datagram + 8, "connect", 7);

    struct sockaddr_in to;
    memset(&to, 0, sizeof(to));
    to.sin_family = AF_INET;
    to.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    to.sin_port = htons(server_port);

    (void)sendto(sock, datagram, packet_len, 0, (struct sockaddr *)&to, sizeof(to));
}

static BOOL recv_client_connect_oob(int sock) {
    BYTE datagram[128];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    FOR_LOOP(i, 40) {
        int r = recvfrom(sock, datagram, sizeof(datagram), 0, (struct sockaddr *)&from, &fromlen);
        if (r > 0) {
            if (r >= 4 + 4 + 14 && memcmp(datagram + 8, "client_connect", 14) == 0)
                return true;
            return false;
        }
        usleep(5000);
    }
    return false;
}

static void pump_server_connects(void) {
    enum { MIN_CONNECT_OOB_PAYLOAD = 11 };
    BYTE msg_buf[MAX_MSGLEN];
    sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t from;
    int r;
    FOR_LOOP(i, 64) {
        r = NET_GetPacket(NS_SERVER, &from, &msg);
        if (!r)
            break;
        if (r >= MIN_CONNECT_OOB_PAYLOAD) {
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
    reset_server_state(4);
    SZ_Init(&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));
    FOR_LOOP(i, 3) {
        SZ_Init(&svs.clients[i].netchan.message,
                svs.clients[i].netchan.message_buf, MAX_MSGLEN);
    }
    svs.num_clients = 3;

    SZ_Write(&sv.multicast, payload, sizeof(payload));
    SV_Multicast(&(VECTOR3){0,0,0}, MULTICAST_ALL_R);

    FOR_LOOP(i, 3) {
        ASSERT_EQ_INT(svs.clients[i].netchan.message.cursize, (int)sizeof(payload));
        ASSERT(memcmp(svs.clients[i].netchan.message.data, payload, sizeof(payload)) == 0);
    }
    ASSERT_EQ_INT(sv.multicast.cursize, 0);
}

void run_server_net_tests(void) {
    RUN_TEST(test_udp_multi_client_connects_register_distinct_slots);
    RUN_TEST(test_udp_connect_honors_ge_max_clients_limit);
    RUN_TEST(test_multicast_syncs_updates_to_all_connected_clients);
}
