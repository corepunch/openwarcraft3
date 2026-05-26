#include "server.h"

static DWORD SV_ConfigStringWireSize(DWORD index) {
    if (index == CS_STATUSBAR) {
        return 1 + 2 + sizeof(*sv.configstrings);
    }
    return 1 + 2 + (DWORD)strlen(ge->GetThemeValue(sv.configstrings[index])) + 1;
}

static void SV_FlushSpawnMessage(LPCLIENT cl, LPCSTR phase, DWORD count, DWORD *packets) {
    if (cl->netchan.message.cursize == 0) {
        return;
    }
    if (cl->netchan.message.overflowed) {
        fprintf(stderr,
                "SV_%s_f: transmitting overflowed batch count=%u bytes=%u max=%u\n",
                phase,
                (unsigned)count,
                (unsigned)cl->netchan.message.cursize,
                (unsigned)cl->netchan.message.maxsize);
    } else {
        fprintf(stderr,
                "SV_%s_f: transmitting batch count=%u bytes=%u max=%u\n",
                phase,
                (unsigned)count,
                (unsigned)cl->netchan.message.cursize,
                (unsigned)cl->netchan.message.maxsize);
    }
    Netchan_Transmit(NS_SERVER, &cl->netchan);
    if (packets) {
        (*packets)++;
    }
}

void SV_Configstrings_f(LPCLIENT cl) {
    DWORD count = 0;
    DWORD batch_count = 0;
    DWORD packets = 0;

    FOR_LOOP(i, MAX_CONFIGSTRINGS) {
        if (!*sv.configstrings[i])
            continue;
        if (cl->netchan.message.cursize + SV_ConfigStringWireSize(i) + 16 >= cl->netchan.message.maxsize) {
            SV_FlushSpawnMessage(cl, "Configstrings", batch_count, &packets);
            batch_count = 0;
        }
        SV_WriteConfigString(&cl->netchan.message, i);
        count++;
        batch_count++;
    }
    if (cl->netchan.message.cursize + 16 >= cl->netchan.message.maxsize) {
        SV_FlushSpawnMessage(cl, "Configstrings", batch_count, &packets);
        batch_count = 0;
    }
    MSG_WriteByte(&cl->netchan.message, svc_mirror);
    MSG_WriteString(&cl->netchan.message, "baselines");
    fprintf(stderr,
            "SV_Configstrings_f: queued %u configstrings in %u packet(s), final bytes=%u overflow=%d\n",
            (unsigned)count,
            (unsigned)(packets + 1),
            (unsigned)cl->netchan.message.cursize,
            cl->netchan.message.overflowed ? 1 : 0);
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

void SV_Baselines_f(LPCLIENT cl) {
    entityState_t nullstate;
    DWORD count = 0;
    DWORD batch_count = 0;
    DWORD packets = 0;

    memset(&nullstate, 0, sizeof(entityState_t));
    FOR_LOOP(index, ge->num_edicts) {
        edict_t *e = EDICT_NUM(index);
        if (e->svflags & SVF_NOCLIENT)
            continue;
        if (cl->netchan.message.cursize + 512 >= cl->netchan.message.maxsize) {
            SV_FlushSpawnMessage(cl, "Baselines", batch_count, &packets);
            batch_count = 0;
        }
        MSG_WriteByte(&cl->netchan.message, svc_spawnbaseline);
        MSG_WriteDeltaEntity(&cl->netchan.message, &nullstate, &e->s, true);
        count++;
        batch_count++;
    }
    if (cl->netchan.message.cursize + 16 >= cl->netchan.message.maxsize) {
        SV_FlushSpawnMessage(cl, "Baselines", batch_count, &packets);
        batch_count = 0;
    }
    MSG_WriteByte(&cl->netchan.message, svc_mirror);
    MSG_WriteString(&cl->netchan.message, "playerinfo");
    fprintf(stderr,
            "SV_Baselines_f: queued %u baselines in %u packet(s), final bytes=%u overflow=%d\n",
            (unsigned)count,
            (unsigned)(packets + 1),
            (unsigned)cl->netchan.message.cursize,
            cl->netchan.message.overflowed ? 1 : 0);
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

void SV_Begin_f(LPCLIENT cl) {
    cl->state = cs_spawned;
    ge->ClientBegin(cl->edict);
}

void SV_PlayerInfo_f(LPCLIENT cl) {
    /* Assign the client's game edict (Quake 2/3 pattern) */
    cl->edict = EDICT_NUM(CM_GetLocalPlayerNumber());
    MSG_WriteByte(&cl->netchan.message, svc_mirror);
    MSG_WriteString(&cl->netchan.message, "begin");
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

void SV_New_f(LPCLIENT cl) {
    MSG_WriteByte(&cl->netchan.message, svc_mirror);
    MSG_WriteString(&cl->netchan.message, "configstrings");
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

typedef struct {
    LPCSTR name;
    void (*func)(LPCLIENT client);
} ucmd_t;

ucmd_t ucmds[] = {
    { "new", SV_New_f },
    { "configstrings", SV_Configstrings_f },
    { "baselines", SV_Baselines_f },
    { "playerinfo", SV_PlayerInfo_f },
    { "begin", SV_Begin_f },
    { NULL }
};

void SV_ExecuteUserCommand(LPSIZEBUF msg, LPCLIENT client) {
    typedef char cmdarg_t[CMDARG_LEN];
    static cmdarg_t args[MAX_CMDARGS];
    static LPCSTR argv[MAX_CMDARGS];
    DWORD argc = 0;
    LPCSTR command = MSG_ReadString2(msg);
    parser_t p = { 0 };
    p.tok = p.token;
    p.str = command;
    for (LPCSTR tok = ParserGetToken(&p); tok; tok = ParserGetToken(&p)) {
        strcpy(args[argc], tok);
        argv[argc] = args[argc];
        argc++;
    }
    if (argc == 0) {
        return;
    }
    for (ucmd_t *u = ucmds; u->name; u++) {
        if (!strcmp(argv[0], u->name)) {
            u->func(client);
            return;
        }
    }
    ge->ClientCommand(client->edict, argc, argv);
}
