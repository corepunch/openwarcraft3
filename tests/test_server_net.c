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
void test_client_stubs_clear_cvars(void);
void test_client_stubs_set_cvar(LPCSTR name, LPCSTR value);
extern struct game_import gi;

/* External symbols referenced by sv_init.c but unused in these tests. */
void SV_InitGameProgs(void) {}
void SV_ClearWorld(void) {}
bool CM_LoadMap(LPCSTR mapFilename) { (void)mapFilename; return true; }
LPDOODAD CM_GetDoodads(void) { return NULL; }
static LPMAPINFO test_mapinfo;
LPCMAPINFO CM_GetMapInfo(void) { return test_mapinfo; }
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
static edict_t test_edicts[MAX_CLIENT_ENTITIES];

static void test_spawn_entities(LPCMAPINFO mapinfo, LPCDOODAD doodads) {
    (void)mapinfo;
    (void)doodads;
}

static void reset_server_state(int max_players) {
    memset(&sv, 0, sizeof(sv));
    memset(&svs, 0, sizeof(svs));
    memset(&test_ge, 0, sizeof(test_ge));
    memset(test_edicts, 0, sizeof(test_edicts));
    test_mapinfo = NULL;
    SZ_Init(&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));
    test_ge.max_clients = max_players;
    test_ge.max_edicts = MAX_CLIENT_ENTITIES;
    test_ge.edict_size = sizeof(edict_t);
    test_ge.edicts = test_edicts;
    test_ge.num_edicts = max_players;
    test_ge.SpawnEntities = test_spawn_entities;
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

static void send_info_oob(int sock, unsigned short server_port) {
    enum {
        MAX_INFO_DATAGRAM_SIZE = 64,
        OOB_HEADER_SIZE = 4,
        INFO_TEXT_SIZE = 4
    };
    BYTE datagram[MAX_INFO_DATAGRAM_SIZE];
    DWORD msg_len = OOB_HEADER_SIZE + INFO_TEXT_SIZE; /* -1 + "info" */
    DWORD packet_len = 4 + msg_len; /* net packet header + payload */
    int oob_marker = -1;
    datagram[0] = (BYTE)(msg_len & 0xFF);
    datagram[1] = (BYTE)((msg_len >> 8) & 0xFF);
    datagram[2] = (BYTE)((msg_len >> 16) & 0xFF);
    datagram[3] = (BYTE)((msg_len >> 24) & 0xFF);
    memcpy(datagram + 4, &oob_marker, sizeof(oob_marker));
    memcpy(datagram + 8, "info", 4);

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

static void pump_server_connectionless(void) {
    enum {
        MAX_PACKETS_PER_PUMP = 64,
        MAX_EMPTY_POLLS = 40,
        RECV_POLL_DELAY_US = 5000,
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
        SV_ConnectionlessPacket(&from, &msg);
    }
}

static BOOL recv_info_oob(int sock, LPSTR out, DWORD out_size) {
    enum {
        MAX_RECV_RETRIES = 40,
        RECV_POLL_DELAY_US = 5000
    };
    BYTE datagram[512];
    struct sockaddr_in from;
    socklen_t fromlen = sizeof(from);
    int flags = fcntl(sock, F_GETFL, 0);
    fcntl(sock, F_SETFL, flags | O_NONBLOCK);

    if (out && out_size > 0) {
        out[0] = '\0';
    }
    FOR_LOOP(i, MAX_RECV_RETRIES) {
        int r = recvfrom(sock, datagram, sizeof(datagram), 0, (struct sockaddr *)&from, &fromlen);
        if (r > 8) {
            DWORD len = MIN((DWORD)(r - 8), out_size ? out_size - 1 : 0);
            if (out && out_size > 0) {
                memcpy(out, datagram + 8, len);
                out[len] = '\0';
            }
            return memcmp(datagram + 8, "info", 4) == 0;
        }
        usleep(RECV_POLL_DELAY_US);
    }
    return false;
}

static void drain_client_packets(void) {
    BYTE msg_buf[MAX_MSGLEN];
    sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t from;

    while (NET_GetPacket(NS_CLIENT, &from, &msg)) {
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

static void test_lan_info_query_returns_discoverable_server_metadata(void) {
    int c1 = open_client_socket();
    char info[512];
    ASSERT(c1 >= 0);
    NET_Shutdown();
    ASSERT(NET_Init(PORT_SERVER + 11));
    reset_server_state(8);
    sv.state = ss_game;
    snprintf(sv.configstrings[CS_WORLD], sizeof(sv.configstrings[CS_WORLD]),
             "Maps\\Melee\\TwinRivers.w3m");
    svs.num_clients = 1;
    svs.clients[0].state = cs_spawned;

    send_info_oob(c1, PORT_SERVER + 11);
    pump_server_connectionless();

    ASSERT(recv_info_oob(c1, info, sizeof(info)));
    ASSERT(strstr(info, "\\hostname\\OpenWarcraft3") != NULL);
    ASSERT(strstr(info, "\\mapname\\Maps/Melee/TwinRivers.w3m") != NULL);
    ASSERT(strstr(info, "\\players\\1") != NULL);
    ASSERT(strstr(info, "\\maxplayers\\8") != NULL);
    ASSERT(strstr(info, "\\speed\\2") != NULL);

    if (c1 >= 0) close(c1);
    NET_Shutdown();
}

static void test_lan_info_query_returns_lobby_metadata(void) {
    int c1 = open_client_socket();
    char info[512];
    ASSERT(c1 >= 0);
    NET_Shutdown();
    ASSERT(NET_Init(PORT_SERVER + 12));
    reset_server_state(8);
    sv.state = ss_lobby;
    snprintf(sv.configstrings[CS_WORLD], sizeof(sv.configstrings[CS_WORLD]),
             "Maps\\Melee\\TwinRivers.w3m");
    svs.num_clients = 1;
    svs.clients[0].state = cs_connected;

    send_info_oob(c1, PORT_SERVER + 12);
    pump_server_connectionless();

    ASSERT(recv_info_oob(c1, info, sizeof(info)));
    ASSERT(strstr(info, "\\mapname\\Maps/Melee/TwinRivers.w3m") != NULL);
    ASSERT(strstr(info, "\\players\\1") != NULL);

    if (c1 >= 0) close(c1);
    NET_Shutdown();
}

static void test_lobby_team_selection_expands_map_forces(void) {
    MAPINFO info;

    reset_server_state(4);
    memset(&info, 0, sizeof(info));
    info.num_teams = 1;
    info.teams = MemAlloc(sizeof(*info.teams));
    memset(info.teams, 0, sizeof(*info.teams));
    info.teams[0].playerMasks = 0x0f;
    FOR_LOOP(i, 4) {
        info.players[i].used = true;
        info.players[i].playerType = kPlayerTypeHuman;
        info.players[i].playerRace = kPlayerRaceHuman;
    }
    test_mapinfo = &info;

    test_client_stubs_clear_cvars();
    test_client_stubs_set_cvar("sv_lobby_slots", "2");
    test_client_stubs_set_cvar("sv_slot0_map_player", "0");
    test_client_stubs_set_cvar("sv_slot0_type", "human");
    test_client_stubs_set_cvar("sv_slot0_team", "0");
    test_client_stubs_set_cvar("sv_slot1_map_player", "1");
    test_client_stubs_set_cvar("sv_slot1_type", "human");
    test_client_stubs_set_cvar("sv_slot1_team", "3");

    SV_Map("Maps\\Melee\\Test.w3m");

    ASSERT_EQ_INT(info.num_teams, 4);
    ASSERT((info.teams[0].playerMasks & (1u << 0)) != 0);
    ASSERT((info.teams[0].playerMasks & (1u << 1)) == 0);
    ASSERT((info.teams[3].playerMasks & (1u << 1)) != 0);

    SV_Shutdown();
    SAFE_DELETE(info.teams, MemFree);
    test_mapinfo = NULL;
    test_client_stubs_clear_cvars();
}

static void test_lobby_start_preserves_connected_clients(void) {
    MAPINFO info;
    netadr_t remote = { NA_IP, { 127, 0, 0, 1 }, { 0 }, htons(PORT_SERVER + 13) };

    reset_server_state(4);
    memset(&info, 0, sizeof(info));
    test_mapinfo = &info;

    SV_StartLobby("Maps\\Melee\\Test.w3m");
    ASSERT_EQ_INT(sv.state, ss_lobby);
    ASSERT_EQ_INT(svs.num_clients, 1);
    SV_DirectConnect(&remote);
    ASSERT_EQ_INT(svs.num_clients, 2);

    SV_Map("Maps\\Melee\\Test.w3m");

    ASSERT_EQ_INT(sv.state, ss_game);
    ASSERT_EQ_INT(svs.num_clients, 2);
    ASSERT_EQ_INT(svs.clients[0].state, cs_connected);
    ASSERT_EQ_INT(svs.clients[0].netchan.remote_address.type, NA_LOOPBACK);
    ASSERT_EQ_INT(svs.clients[1].state, cs_connected);
    ASSERT_EQ_INT(svs.clients[1].netchan.remote_address.type, NA_IP);
    ASSERT_EQ_INT(svs.clients[1].netchan.remote_address.port, remote.port);

    SV_Shutdown();
    test_mapinfo = NULL;
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

static void test_lobby_chat_broadcasts_to_connected_clients(void) {
    BYTE msg_buf[MAX_MSGLEN];
    sizeBuf_t msg = { msg_buf, MAX_MSGLEN, 0, 0 };
    netadr_t from;
    char text[512];

    NET_Shutdown();
    reset_server_state(4);
    drain_client_packets();
    sv.state = ss_lobby;
    svs.num_clients = 2;
    FOR_LOOP(i, svs.num_clients) {
        svs.clients[i].state = cs_connected;
        svs.clients[i].netchan.remote_address.type = NA_LOOPBACK;
        SZ_Init(&svs.clients[i].netchan.message,
                svs.clients[i].netchan.message_buf,
                MAX_MSGLEN);
    }

    SV_LobbyBroadcastChat("Host", "hello team");

    FOR_LOOP(i, svs.num_clients) {
        ASSERT(NET_GetPacket(NS_CLIENT, &from, &msg));
        ASSERT_EQ_INT(MSG_ReadByte(&msg), svc_lobby_chat);
        MSG_ReadString(&msg, text);
        ASSERT_STR_EQ(text, "Host: hello team");
    }
}

void run_server_net_tests(void) {
    RUN_TEST(test_udp_multi_client_connects_register_distinct_slots);
    RUN_TEST(test_udp_connect_honors_ge_max_clients_limit);
    RUN_TEST(test_lan_info_query_returns_discoverable_server_metadata);
    RUN_TEST(test_lan_info_query_returns_lobby_metadata);
    RUN_TEST(test_lobby_team_selection_expands_map_forces);
    RUN_TEST(test_lobby_start_preserves_connected_clients);
    RUN_TEST(test_multicast_syncs_updates_to_all_connected_clients);
    RUN_TEST(test_lobby_chat_broadcasts_to_connected_clients);
}
