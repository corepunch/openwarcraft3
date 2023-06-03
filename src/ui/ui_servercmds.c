#include <stdlib.h>

#include "ui_local.h"

#define SERVERCOMMAND(NAME) void CMD_##NAME(DWORD argc, LPCSTR argv[])

typedef struct {
    LPCSTR name;
    void (*func)(DWORD argc, LPCSTR argv[]);
} serverCommand_t;

SERVERCOMMAND(Inventory) {
    if (argc > 1) {
        strcpy(ui.selected.abilities, argv[1]);
    }
    CommandBar_SetMode(CBAR_SHOW_ABILITIES);
}

serverCommand_t serverCommands[] = {
    { "inventory", CMD_Inventory },
    { NULL }
};

void UI_ServerCommand(DWORD argc, LPCSTR argv[]) {
    for (serverCommand_t const *cmd = serverCommands; cmd->name; cmd++) {
        if (!strcmp(cmd->name, argv[0])) {
            cmd->func(argc, argv);
            break;
        }
    }
}

