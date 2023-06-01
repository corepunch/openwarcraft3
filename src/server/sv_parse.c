#include "server.h"

void SV_ParseMove(LPSIZEBUF msg, LPCLIENT client) {
    client->camera_position.x = MSG_ReadShort(msg);
    client->camera_position.y = MSG_ReadShort(msg);
}

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
    ge->ClientCommand(client->edict, argc, argv);
}

void SV_ParseClientMessage(LPSIZEBUF msg, LPCLIENT client) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
            case clc_move:
                SV_ParseMove(msg, client);
                break;;
            case clc_stringcmd:
                SV_ExecuteUserCommand(msg, client);
                break;
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
