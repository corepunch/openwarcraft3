#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <fcntl.h>

#include "test_framework.h"

#include "../common/shared.h"
#include "../common/net.h"
#include "../client/client.h"
#include "../server/server.h"

void test_client_stubs_init(void);
extern struct game_import gi;

/* External symbols referenced by sv_init.c but unused in these tests. */
void SV_InitGameProgs(void) {}
void SV_ClearWorld(void) {}
bool CM_LoadMap(LPCSTR mapFilename) { (void)mapFilename; return true; }
LPDOODAD CM_GetDoodads(void) { return NULL; }
LPCMAPINFO CM_GetMapInfo(void) { return NULL; }
struct cmodel *SV_LoadModel(LPCSTR filename) { (void)filename; return NULL; }
HANDLE FS_FindFirstFile(LPCSTR mask, SFILE_FIND_DATA *findData) {
    (void)mask;
    (void)findData;
    return NULL;
}
BOOL FS_FindNextFile(HANDLE find, SFILE_FIND_DATA *findData) {
    (void)find;
    (void)findData;
    return false;
}
BOOL FS_FindClose(HANDLE find) {
    (void)find;
    return true;
}

static void test_run_frame(void) {
}

static LPCSTR test_theme_value(LPCSTR filename) {
    return filename;
}

static HANDLE test_mem_alloc(long size) {
    return MemAlloc(size);
}

static void test_mem_free(HANDLE mem) {
    MemFree(mem);
}

static int test_model_index(LPCSTR name) {
    (void)name;
    return 0;
}

static int test_image_index(LPCSTR name) {
    (void)name;
    return 0;
}

static int test_font_index(LPCSTR name, DWORD fontSize) {
    (void)name;
    (void)fontSize;
    return 0;
}

static void reset_test_gi(void) {
    memset(&gi, 0, sizeof(gi));
    gi.MemAlloc = test_mem_alloc;
    gi.MemFree = test_mem_free;
    gi.ModelIndex = test_model_index;
    gi.ImageIndex = test_image_index;
    gi.FontIndex = test_font_index;
}

void SV_BuildClientFrame(LPCLIENT client) {
    (void)client;
}

void SV_WriteFrameToClient(LPCLIENT client) {
    (void)client;
}

void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client) {
    (void)msg;
    (void)client;
}

static struct game_export test_ge;

static void reset_server_state(int max_players) {
    memset(&sv, 0, sizeof(sv));
    memset(&svs, 0, sizeof(svs));
    memset(&test_ge, 0, sizeof(test_ge));
    SZ_Init(&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));
    test_ge.max_clients = max_players;
    test_ge.max_edicts = MAX_CLIENT_ENTITIES;
    test_ge.edict_size = sizeof(edict_t);
    test_ge.RunFrame = test_run_frame;
    test_ge.GetThemeValue = test_theme_value;
    ge = &test_ge;
    reset_test_gi();
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
        MAX_EMPTY_POLLS = 40,
        RECV_POLL_DELAY_US = 5000,
        MIN_CONNECT_MSG_SIZE = 11 /* -1 marker (4) + "connect" (7) */
    };
    BYTE msg_buf[MAX_MSGLEN];
    sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t from;
    DWORD empty_polls = 0;
    int r;

    for (DWORD packets = 0; packets < MAX_PACKETS_PER_PUMP && empty_polls < MAX_EMPTY_POLLS;) {
        r = NET_GetPacket(NS_SERVER, &from, &msg);
        if (!r) {
            empty_polls++;
            usleep(RECV_POLL_DELAY_US);
            continue;
        }
        empty_polls = 0;
        packets++;
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

void run_server_net_tests(void) {
    RUN_TEST(test_udp_multi_client_connects_register_distinct_slots);
    RUN_TEST(test_udp_connect_honors_ge_max_clients_limit);
    RUN_TEST(test_multicast_syncs_updates_to_all_connected_clients);
}
