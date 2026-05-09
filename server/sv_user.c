#include "server.h"

void SV_Configstrings_f(LPCLIENT cl) {
    FOR_LOOP(i, MAX_CONFIGSTRINGS) {
        if (!*sv.configstrings[i])
            continue;
        SV_WriteConfigString(&cl->netchan.message, i);
    }
    MSG_WriteByte(&cl->netchan.message, svc_mirror);
    MSG_WriteString(&cl->netchan.message, "baselines");
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

void SV_Baselines_f(LPCLIENT cl) {
    entityState_t nullstate;
    memset(&nullstate, 0, sizeof(entityState_t));
    FOR_LOOP(index, ge->num_edicts) {
        edict_t *e = EDICT_NUM(index);
        if (e->svflags & SVF_NOCLIENT)
            continue;
        MSG_WriteByte(&cl->netchan.message, svc_spawnbaseline);
        MSG_WriteDeltaEntity(&cl->netchan.message, &nullstate, &e->s, true);
    }
    MSG_WriteByte(&cl->netchan.message, svc_mirror);
    MSG_WriteString(&cl->netchan.message, "playerinfo");
    Netchan_Transmit(NS_SERVER, &cl->netchan);
}

void SV_Begin_f(LPCLIENT cl) {
    cl->state = cs_spawned;
    ge->ClientBegin(cl->edict);
}

void SV_PlayerInfo_f(LPCLIENT cl) {
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
    for (ucmd_t *u = ucmds; u->name; u++) {
        if (!strcmp(argv[0], u->name)) {
            u->func(client);
            return;
        }
    }
    ge->ClientCommand(client->edict, argc, argv);
}

