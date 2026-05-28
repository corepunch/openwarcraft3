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
void UI_ClearTemplates(void);
void UI_Init(void);
void UI_ParseFDF_Buffer(LPCSTR fileName, LPSTR buffer);
void UI_ShowMainMenu(LPEDICT ent);
void UI_ShowSinglePlayerMenu(LPEDICT ent);
void UI_ShowMultiplayerMenu(LPEDICT ent);
void UI_ShowMultiplayerCreateMenu(LPEDICT ent);
void UI_ShowMultiplayerGameSetupMenu(LPEDICT ent, DWORD mapIndex);
extern struct game_import gi;

#define TEST_LAYER_CONSOLE 3

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

static LPCSTR test_find_sheet_cell(sheetRow_t *sheet, LPCSTR row, LPCSTR column) {
    (void)sheet;
    if (row && column && !strcmp(row, "Default")) {
        if (!strcmp(column, "GlueSpriteLayerTopRight")) {
            return "UI\\Glues\\SpriteLayers\\TopRightPanel.mdl";
        }
        if (!strcmp(column, "GlueSpriteLayerTopLeft")) {
            return "UI\\Glues\\SpriteLayers\\TopLeftPanel.mdl";
        }
        if (!strcmp(column, "GlueSpriteLayerBackground")) {
            return "UI\\Glues\\MainMenu\\MainMenu3d\\MainMenu3d.mdl";
        }
    }
    return NULL;
}

static LPSTR test_read_fdf_file(LPCSTR filename) {
    char path[512] = "data/fdf/";
    size_t prefix_len = strlen(path);
    size_t filename_len = strlen(filename);
    if (prefix_len + filename_len + 1 >= sizeof(path)) {
        return NULL;
    }
    for (size_t i = 0; i < filename_len; i++) {
        path[prefix_len + i] = filename[i] == '\\' ? '/' : filename[i];
    }
    path[prefix_len + filename_len] = '\0';

    FILE *fp = fopen(path, "rb");
    if (!fp) {
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    if (len < 0) {
        fclose(fp);
        return NULL;
    }
    LPSTR buffer = MemAlloc((DWORD)len + 1);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    size_t read = fread(buffer, 1, (size_t)len, fp);
    fclose(fp);
    buffer[read] = '\0';
    return buffer;
}

static bool test_has_repo_fdf(void) {
    char path[512] = "data/fdf/UI/FrameDef/Glue/MainMenu.fdf";
    return access(path, R_OK) == 0;
}

static void test_write_byte(LONG c) {
    MSG_WriteByte(&sv.multicast, (int)c);
}

static void test_write_short(LONG c) {
    MSG_WriteShort(&sv.multicast, (int)c);
}

static void test_write_long(LONG c) {
    MSG_WriteLong(&sv.multicast, (int)c);
}

static void test_write_ui_frame(LPCUIFRAME frame) {
    uiFrame_t empty = { 0 };
    empty.tex.coord[1] = 0xff;
    empty.tex.coord[3] = 0xff;
    MSG_WriteDeltaUIFrame(&sv.multicast, &empty, frame, true);
    MSG_WriteShort(&sv.multicast, frame->buffer.size);
    MSG_Write(&sv.multicast, frame->buffer.data, frame->buffer.size);
}

static void test_unicast(edict_t *ent) {
    FOR_LOOP(i, svs.num_clients) {
        if (svs.clients[i].edict == ent) {
            SZ_Write(&svs.clients[i].netchan.message, sv.multicast.data, sv.multicast.cursize);
            SZ_Clear(&sv.multicast);
            return;
        }
    }
    SZ_Clear(&sv.multicast);
}

static void reset_test_gi(void) {
    memset(&gi, 0, sizeof(gi));
    gi.MemAlloc = test_mem_alloc;
    gi.MemFree = test_mem_free;
    gi.ModelIndex = test_model_index;
    gi.ImageIndex = test_image_index;
    gi.FontIndex = test_font_index;
    gi.FindSheetCell = test_find_sheet_cell;
    gi.ReadFileIntoString = test_read_fdf_file;
    gi.WriteByte = test_write_byte;
    gi.WriteShort = test_write_short;
    gi.WriteLong = test_write_long;
    gi.WriteUIFrame = test_write_ui_frame;
    gi.unicast = test_unicast;
}

void SV_BuildClientFrame(LPCLIENT client) {
    (void)client;
}

void SV_WriteFrameToClient(LPCLIENT client) {
    Netchan_Transmit(NS_SERVER, &client->netchan);
}

static bool test_menu_command_seen = false;

static void setup_test_menu_frames(void) {
    static LPCSTR menu_fdf =
        "Frame \"FRAME\" \"MainMenuFrame\" {"
        " SetAllPoints,"
        " Frame \"FRAME\" \"ControlLayer\" {"
        "  SetAllPoints,"
        "  Frame \"FRAME\" \"MainMenuOnlyFrame\" {"
        "   Width 0.25,"
        "   Height 0.03125,"
        "  }"
        " }"
        "}"
        "Frame \"FRAME\" \"SinglePlayerMenu\" {"
        " SetAllPoints,"
        " Frame \"FRAME\" \"SinglePlayerOnlyFrame\" {"
        "  Width 0.125,"
        "  Height 0.0625,"
        " }"
        "}";
    LPSTR buffer = strdup(menu_fdf);
    ASSERT_NOT_NULL(buffer);
    UI_ClearTemplates();
    UI_ParseFDF_Buffer("test_menu.fdf", buffer);
    free(buffer);
}

static void test_client_command(LPEDICT ent, DWORD argc, LPCSTR argv[]) {
    ASSERT_NOT_NULL(ent);
    ASSERT_EQ_INT(argc, 2);
    ASSERT_STR_EQ(argv[0], "menu");
    ASSERT_STR_EQ(argv[1], "/single-player");
    test_menu_command_seen = true;
    UI_ShowSinglePlayerMenu(ent);
}

static bool decoded_layout_contains_onclick(LPCUIFRAME decoded, LPCSTR onclick) {
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        if (decoded[i].onclick && !strcmp(decoded[i].onclick, onclick)) {
            return true;
        }
    }
    return false;
}

static DWORD decoded_layout_count_sprite_animation(LPCUIFRAME decoded, LPCSTR animation) {
    DWORD count = 0;
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        if (decoded[i].flags.type == FT_SPRITE &&
            decoded[i].text &&
            !strcmp(decoded[i].text, animation)) {
            count++;
        }
    }
    return count;
}

static DWORD decoded_layout_count_type(LPCUIFRAME decoded, FRAMETYPE type) {
    DWORD count = 0;
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        if (decoded[i].flags.type == type) {
            count++;
        }
    }
    return count;
}

static bool decoded_layout_contains_listbox_fetch(LPCUIFRAME decoded, LPCSTR fetchCommand) {
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        uiListBox_t const *listbox;
        if (decoded[i].flags.type != FT_LISTBOX ||
            decoded[i].buffer.size < sizeof(uiListBox_t) ||
            !decoded[i].buffer.data) {
            continue;
        }
        listbox = (uiListBox_t const *)decoded[i].buffer.data;
        if (!strcmp(listbox->fetchCommand, fetchCommand)) {
            return true;
        }
    }
    return false;
}

void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        ASSERT_EQ_INT(pack_id, clc_stringcmd);
        if (pack_id != clc_stringcmd) {
            return;
        }

        LPCSTR command = MSG_ReadString2(msg);
        LPCSTR argv[] = { "menu", "/single-player" };
        ASSERT_STR_EQ(command, "menu /single-player");
        ge->ClientCommand(client->edict, 2, argv);
    }
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
    test_ge.ClientCommand = test_client_command;
    ge = &test_ge;
    test_menu_command_seen = false;
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

static void test_menu_command_updates_client_layout_after_server_response(void) {
    struct netchan client_netchan;
    BYTE server_packet[MAX_MSGLEN];
    sizeBuf_t server_msg = { server_packet, MAX_MSGLEN, 0, 0 };
    netadr_t from;
    GAMECLIENT game_client = { 0 };
    edict_t client_edict = { 0 };
    LPCUIFRAME decoded;
    bool found_mainmenu_frame = false;
    bool found_singleplayer_frame = false;

    NET_Shutdown();
    reset_server_state(1);
    test_client_stubs_init();
    setup_test_menu_frames();

    sv.framenum = 1;
    sv.time = 100;
    svs.realtime = 100;
    svs.num_clients = 1;
    svs.clients[0].state = cs_spawned;
    svs.clients[0].netchan.remote_address.type = NA_LOOPBACK;
    SZ_Init(&svs.clients[0].netchan.message,
            svs.clients[0].netchan.message_buf,
            MAX_MSGLEN);

    game_client.ps.number = 0;
    client_edict.inuse = true;
    client_edict.client = &game_client;
    svs.clients[0].edict = &client_edict;

    UI_ShowMainMenu(&client_edict);
    SV_WriteFrameToClient(&svs.clients[0]);
    ASSERT(NET_GetPacket(NS_CLIENT, &from, &server_msg) > 0);
    CL_ParseServerMessage(&server_msg);
    ASSERT_NOT_NULL(cl.layout[TEST_LAYER_CONSOLE]);
    decoded = SCR_Clear(cl.layout[TEST_LAYER_CONSOLE]);
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        if (decoded[i].flags.type == FT_FRAME &&
            decoded[i].size.width == 0.25f &&
            decoded[i].size.height == 0.03125f) {
            found_mainmenu_frame = true;
            break;
        }
    }
    ASSERT(found_mainmenu_frame);

    memset(&client_netchan, 0, sizeof(client_netchan));
    client_netchan.remote_address.type = NA_LOOPBACK;
    SZ_Init(&client_netchan.message, client_netchan.message_buf, MAX_MSGLEN);
    MSG_WriteByte(&client_netchan.message, clc_stringcmd);
    MSG_WriteString(&client_netchan.message, "menu /single-player");
    Netchan_Transmit(NS_CLIENT, &client_netchan);

    ASSERT(!test_menu_command_seen);

    SV_Frame(100);

    ASSERT(test_menu_command_seen);
    ASSERT(NET_GetPacket(NS_CLIENT, &from, &server_msg) > 0);
    CL_ParseServerMessage(&server_msg);
    ASSERT_NOT_NULL(cl.layout[TEST_LAYER_CONSOLE]);
    decoded = SCR_Clear(cl.layout[TEST_LAYER_CONSOLE]);
    found_mainmenu_frame = false;
    FOR_LOOP(i, MAX_LAYOUT_OBJECTS) {
        if (decoded[i].flags.type == FT_FRAME &&
            decoded[i].size.width == 0.25f &&
            decoded[i].size.height == 0.03125f) {
            found_mainmenu_frame = true;
        }
        if (decoded[i].flags.type == FT_FRAME &&
            decoded[i].size.width == 0.125f &&
            decoded[i].size.height == 0.0625f) {
            found_singleplayer_frame = true;
        }
    }
    ASSERT(!found_mainmenu_frame);
    ASSERT(found_singleplayer_frame);

    SAFE_DELETE(cl.layout[TEST_LAYER_CONSOLE], MemFree);
    NET_Shutdown();
}

static void test_menu_command_updates_client_layout_with_repo_fdf(void) {
    struct netchan client_netchan;
    BYTE server_packet[MAX_MSGLEN];
    sizeBuf_t server_msg = { server_packet, MAX_MSGLEN, 0, 0 };
    netadr_t from;
    GAMECLIENT game_client = { 0 };
    edict_t client_edict = { 0 };
    LPCUIFRAME decoded;

    NET_Shutdown();
    reset_server_state(1);
    test_client_stubs_init();
    if (!test_has_repo_fdf()) {
        return;
    }
    UI_Init();

    sv.framenum = 1;
    sv.time = 100;
    svs.realtime = 100;
    svs.num_clients = 1;
    svs.clients[0].state = cs_spawned;
    svs.clients[0].netchan.remote_address.type = NA_LOOPBACK;
    SZ_Init(&svs.clients[0].netchan.message,
            svs.clients[0].netchan.message_buf,
            MAX_MSGLEN);

    game_client.ps.number = 0;
    client_edict.inuse = true;
    client_edict.client = &game_client;
    svs.clients[0].edict = &client_edict;

    UI_ShowMainMenu(&client_edict);
    SV_WriteFrameToClient(&svs.clients[0]);
    ASSERT(NET_GetPacket(NS_CLIENT, &from, &server_msg) > 0);
    CL_ParseServerMessage(&server_msg);
    ASSERT_NOT_NULL(cl.layout[TEST_LAYER_CONSOLE]);
    decoded = SCR_Clear(cl.layout[TEST_LAYER_CONSOLE]);
    ASSERT(decoded_layout_contains_onclick(decoded, "menu /single-player"));
    ASSERT(decoded_layout_contains_onclick(decoded, "menu /realm-select"));
    ASSERT(!decoded_layout_contains_onclick(decoded, "menu /single-player/campaign"));
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "MainMenu Stand"), 2);
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "SinglePlayer Stand"), 0);
    ASSERT_EQ_INT(decoded_layout_count_type(decoded, FT_SPRITE), 3);

    memset(&client_netchan, 0, sizeof(client_netchan));
    client_netchan.remote_address.type = NA_LOOPBACK;
    SZ_Init(&client_netchan.message, client_netchan.message_buf, MAX_MSGLEN);
    MSG_WriteByte(&client_netchan.message, clc_stringcmd);
    MSG_WriteString(&client_netchan.message, "menu /single-player");
    Netchan_Transmit(NS_CLIENT, &client_netchan);

    test_menu_command_seen = false;
    SV_Frame(100);

    ASSERT(test_menu_command_seen);
    ASSERT(NET_GetPacket(NS_CLIENT, &from, &server_msg) > 0);
    CL_ParseServerMessage(&server_msg);
    decoded = SCR_Clear(cl.layout[TEST_LAYER_CONSOLE]);
    ASSERT(!decoded_layout_contains_onclick(decoded, "menu /single-player"));
    ASSERT(decoded_layout_contains_onclick(decoded, "menu /single-player/campaign"));
    ASSERT(decoded_layout_contains_onclick(decoded, "menu /main"));
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "MainMenu Stand"), 0);
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "SinglePlayer Stand"), 2);
    ASSERT_EQ_INT(decoded_layout_count_type(decoded, FT_SPRITE), 2);

    UI_ShowMultiplayerMenu(&client_edict);
    SV_WriteFrameToClient(&svs.clients[0]);
    ASSERT(NET_GetPacket(NS_CLIENT, &from, &server_msg) > 0);
    CL_ParseServerMessage(&server_msg);
    decoded = SCR_Clear(cl.layout[TEST_LAYER_CONSOLE]);
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "BattlenetCustom Stand"), 2);
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "BattlenetWelcome Stand"), 0);
    ASSERT(decoded_layout_contains_listbox_fetch(decoded, "lan-games"));

    UI_ShowMultiplayerCreateMenu(&client_edict);
    SV_WriteFrameToClient(&svs.clients[0]);
    ASSERT(NET_GetPacket(NS_CLIENT, &from, &server_msg) > 0);
    CL_ParseServerMessage(&server_msg);
    decoded = SCR_Clear(cl.layout[TEST_LAYER_CONSOLE]);
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "BattlenetCustomCreate Stand"), 2);
    ASSERT(decoded_layout_contains_listbox_fetch(decoded, "maps"));
    ASSERT(decoded_layout_contains_onclick(decoded, "menu /lan/setup?map={MapListBox}"));

    UI_ShowMultiplayerGameSetupMenu(&client_edict, 0);
    SV_WriteFrameToClient(&svs.clients[0]);
    ASSERT(NET_GetPacket(NS_CLIENT, &from, &server_msg) > 0);
    CL_ParseServerMessage(&server_msg);
    decoded = SCR_Clear(cl.layout[TEST_LAYER_CONSOLE]);
    ASSERT_EQ_INT(decoded_layout_count_sprite_animation(decoded, "BattlenetCustomCreate Stand"), 2);
    ASSERT(decoded_layout_contains_onclick(decoded, "menu /game"));
    ASSERT(decoded_layout_contains_onclick(decoded, "menu /lan/create"));
    ASSERT(!decoded_layout_contains_listbox_fetch(decoded, "maps"));

    SAFE_DELETE(cl.layout[TEST_LAYER_CONSOLE], MemFree);
    NET_Shutdown();
}

void run_server_net_tests(void) {
    RUN_TEST(test_udp_multi_client_connects_register_distinct_slots);
    RUN_TEST(test_udp_connect_honors_ge_max_clients_limit);
    RUN_TEST(test_multicast_syncs_updates_to_all_connected_clients);
    RUN_TEST(test_menu_command_updates_client_layout_after_server_response);
    RUN_TEST(test_menu_command_updates_client_layout_with_repo_fdf);
}

void run_menu_loop_tests(void) {
    RUN_TEST(test_menu_command_updates_client_layout_after_server_response);
    RUN_TEST(test_menu_command_updates_client_layout_with_repo_fdf);
}
