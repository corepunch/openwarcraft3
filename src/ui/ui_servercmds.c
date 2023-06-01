#include <stdlib.h>

#include "ui_local.h"

typedef struct {
    LPCSTR name;
    void (*func)(DWORD argc, LPCSTR argv[]);
} serverCommand_t;

serverCommand_t serverCommands[] = {
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

