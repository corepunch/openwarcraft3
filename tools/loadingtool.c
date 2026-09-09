/* Batch the production loading writer without loading terrain, running frames, or opening a renderer. */
#include "server/server.h"
#include <stdlib.h>
#include <string.h>
#include <zlib.h>
#include <SDL2/SDL.h>

extern void Key_Init(void);

static struct {
    LPCSTR filter;
    DWORD maps, failed, slots[3], shortened, max_raw, max_zip;
} audit;

void Sys_Quit(void) { exit(0); }

/* JSON Lines keeps multiline WTS text on one record and remains usable with jq or Python. */
static void loading_json(LPCSTR text) {
    putchar('"');
    for (const unsigned char *p = (const unsigned char *)text; *p; p++) {
        if (*p == '"' || *p == '\\') printf("\\%c", *p);
        else if (*p < 32) printf("\\u%04x", *p);
        else putchar(*p);
    }
    putchar('"');
}

/* Inspect the actual serialized text frames; animation commands and non-text frames are not displayed strings. */
static void loading_texts(sizeBuf_t msg) {
    BOOL comma = false;
    msg.readcount = 1;
    putchar('[');
    while (msg.readcount + sizeof(DWORD) + sizeof(WORD) <= msg.cursize) {
        UIFRAME frame = { 0 };
        DWORD bits, number = MSG_ReadEntityBits(&msg, &bits);
        if (!number && !bits) break;
        MSG_ReadDeltaUIFrame(&msg, &frame, number, bits);
        DWORD size = (BYTE)MSG_ReadByte(&msg);
        msg.readcount += size;
        if (frame.flags.type != FT_STRING && frame.flags.type != FT_TEXT && frame.flags.type != FT_TEXTAREA) continue;
        if (!frame.text || !*frame.text) continue;
        if (comma) putchar(',');
        printf("{\"frame\":%u,\"bytes\":%zu,\"text\":", number, strlen(frame.text));
        loading_json(frame.text); putchar('}'); comma = true;
    }
    putchar(']');
}

/* Fresh media indices match SV_Map; use PrepareMap and the shared packer, never a parallel W3I/FDF parser. */
static void loading_map(LPCSTR path, void *unused) {
    (void)unused;
    if (audit.filter && !strstr(path, audit.filter)) return;
    audit.maps++;
    memset(&sv, 0, sizeof(sv));
    sv.state = ss_loading;
    SZ_Init(&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));
    SV_SetConfigString(CS_WORLD, path, (DWORD)strlen(path) + 1);
    printf("{\"map\":"); loading_json(path);
    if (!ge->PrepareMap(path) || sv.multicast.overflowed || sv.multicast.cursize < 8 ||
        sv.multicast.data[0] != svc_layout || sv.multicast.data[1] != LAYER_LOADING) {
        fprintf(stderr, "loadingtool: cannot prepare %s\n", path);
        puts(",\"error\":\"loading preparation failed\"}"); audit.failed++;
        return;
    }
    sizeBuf_t raw = { .data = sv.multicast.data + 1, .cursize = sv.multicast.cursize - 1 };
    uLongf size = compressBound(raw.cursize);
    BYTE *zip = MemAlloc(size);
    int err = compress2(zip, &size, raw.data, raw.cursize, Z_BEST_COMPRESSION);
    MemFree(zip);
    if (err != Z_OK) {
        fprintf(stderr, "loadingtool: compression failed for %s (%d)\n", path, err);
        puts(",\"error\":\"compression failed\"}"); audit.failed++;
        return;
    }
    DWORD slots = (size + MAX_PATHLEN - 1) / MAX_PATHLEN;
    audit.slots[MIN(slots, 3) - 1]++;
    audit.max_raw = MAX(audit.max_raw, raw.cursize); audit.max_zip = MAX(audit.max_zip, size);
    printf(",\"layout_bytes\":%u,\"compressed_bytes\":%lu,\"slots_without_shortening\":%u,\"original_texts\":", raw.cursize, size, slots);
    loading_texts(raw);
    if (!SV_BuildLoadingConfigstrings()) {
        fprintf(stderr, "loadingtool: layout cannot fit the current budget: %s\n", path);
        puts(",\"error\":\"512-byte budget exceeded\"}"); audit.failed++;
        return;
    }
    BYTE packed[BZ_LOADING_SCREEN_SIZE], shown[MAX_MSGLEN];
    memcpy(packed, sv.configstrings + CS_LOADINGSCREEN1, sizeof(packed));
    uLongf outsize = sizeof(shown), consumed = sizeof(packed);
    if (uncompress2(shown, &outsize, packed, &consumed) != Z_OK) {
        fprintf(stderr, "loadingtool: cannot decode packed layout for %s\n", path);
        puts(",\"error\":\"packed layout decode failed\"}"); audit.failed++;
        return;
    }
    BOOL shortened = size > BZ_LOADING_SCREEN_SIZE;
    audit.shortened += shortened;
    printf(",\"sent_compressed_bytes\":%lu,\"shortened\":%s,\"shown_texts\":", consumed, shortened ? "true" : "false");
    loading_texts((sizeBuf_t){ .data = shown, .cursize = outsize });
    puts("}");
}

int main(int argc, char **argv) {
    LPCSTR data = NULL;
    BOOL tft = false;
    for (int i = 1; i < argc; i++) {
        if (!strcmp(argv[i], "-data") && i + 1 < argc) data = argv[++i];
        else if (!strcmp(argv[i], "-tft")) tft = true;
        else if (!strcmp(argv[i], "-roc")) tft = false;
        else if (argv[i][0] != '-' && !audit.filter) audit.filter = argv[i];
        else {
            fprintf(stderr, "Usage: loadingtool -data <Warcraft III directory> [-roc|-tft] [map-path substring]\n");
            return strcmp(argv[i], "--help") && strcmp(argv[i], "-h") ? 1 : 0;
        }
    }
    if (!data) { fprintf(stderr, "loadingtool: -data is required\n"); return 1; }
    Cbuf_Init(); Cvar_Init(); Key_Init();
    LPSTR basepath = SDL_GetBasePath();
    FS_ResolveShareDirectory(basepath); SDL_free(basepath);
    FS_SetSheetHost(&MAKE(SHEETHOST, .ReadFile = FS_ReadFile, .FreeFile = FS_FreeFile, .MemAlloc = MemAlloc, .MemFree = MemFree));
    FS_Init();
    PATHSTR cfg;
    snprintf(cfg, sizeof(cfg), "%s/%s/config.cfg", FS_BasePath(), BZ_GAME);
    if (!Cvar_LoadConfig(cfg)) { fprintf(stderr, "loadingtool: missing game defaults: %s\n", cfg); return 1; }
    Cbuf_Execute();
    Cvar_Set("dedicated", "1"); Cvar_Set("fs_expansion", tft ? "1" : "0");
    if (!FS_AddDataDirectory(data)) return 1;
    SZ_Init(&sv.multicast, sv.multicast_buf, sizeof(sv.multicast_buf));
    SV_InitGameProgs();
    FS_ListMaps(loading_map, NULL);
    ge->Shutdown(); FS_Shutdown();
    printf("{\"summary\":{\"maps\":%u,\"failed\":%u,\"one_slot\":%u,\"two_slots\":%u,\"over_two_slots\":%u,\"shortened\":%u,\"max_layout_bytes\":%u,\"max_compressed_bytes\":%u}}\n", audit.maps, audit.failed, audit.slots[0], audit.slots[1], audit.slots[2], audit.shortened, audit.max_raw, audit.max_zip);
    if (!audit.maps) fprintf(stderr, "loadingtool: no maps matched\n");
    return audit.failed || !audit.maps ? 1 : 0;
}
