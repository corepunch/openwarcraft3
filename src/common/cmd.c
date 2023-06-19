#include <string.h>

#include "common.h"

typedef struct cmd_function_s {
    struct cmd_function_s *next;
    LPCSTR name;
    xcommand_t function;
} cmd_function_t;

static sizeBuf_t cmd_text;
static BYTE cmd_text_buf[8192];
static bool cmd_wait;
static cmd_function_t *cmd_functions; // possible commands to execute


char *va(char *format, ...) {
    va_list argptr;
    static char string[1024];
    va_start (argptr, format);
    vsprintf (string, format,argptr);
    va_end (argptr);
    return string;
}

void Cmd_Wait_f (void) {
    cmd_wait = true;
}

void Cbuf_Init(void) {
    cmd_wait = false;
    SZ_Init (&cmd_text, cmd_text_buf, sizeof(cmd_text_buf));
}

void Cbuf_AddText(LPCSTR text) {
    DWORD l = (DWORD)strlen(text);
    if (cmd_text.cursize + l >= cmd_text.maxsize) {
        fprintf(stderr, "Cbuf_AddText: overflow\n");
        return;
    }
    SZ_Write(&cmd_text, text, l);
}

void Cmd_ExecuteString(LPCSTR text) {
    parser_t parser = { 0 };
    parser.tok = parser.token;
    parser.str = text;
    LPCSTR token = ParserGetToken(&parser);

    if (!token)
        return;

    // check functions
    FOR_EACH_LIST(cmd_function_t, cmd, cmd_functions) {
        if (!strcmp(token, cmd->name)) {
            if (!cmd->function) {    // forward to server command
                Cmd_ExecuteString(va("cmd %s", text));
            } else {
                cmd->function ();
            }
            return;
        }
    }
    
    // send it as a server command if we are connected
    Cmd_ForwardToServer(text);
}

void Cbuf_Execute(void) {
    DWORD i;
    LPSTR text;
    char line[1024];
    DWORD quotes;

    while (cmd_text.cursize) {
        // find a \n or ; line break
        text = (LPSTR)cmd_text.data;
        quotes = 0;
        for (i=0 ; i< cmd_text.cursize ; i++) {
            if (text[i] == '"')
                quotes++;
            if ( !(quotes&1) &&  text[i] == ';')
                break; // don't break if inside a quoted string
            if (text[i] == '\n')
                break;
        }
                
        memcpy(line, text, i);
        line[i] = 0;
        
// delete the text from the command buffer and move remaining commands down
// this is necessary because commands (exec, alias) can insert data at the
// beginning of the text buffer

        if (i == cmd_text.cursize) {
            cmd_text.cursize = 0;
        } else {
            i++;
            cmd_text.cursize -= i;
            memmove(text, text+i, cmd_text.cursize);
        }

// execute the command line
        Cmd_ExecuteString (line);
        
        if (cmd_wait) {
            // skip out while text still remains in buffer, leaving it
            // for next frame
            cmd_wait = false;
            break;
        }
    }
}

bool Cmd_Exists(LPCSTR cmd_name) {
    FOR_EACH_LIST(cmd_function_t, cmd, cmd_functions) {
        if (!strcmp(cmd_name, cmd->name)) {
            return true;
        }
    }
    return false;
}

void Cmd_AddCommand(LPCSTR cmd_name, xcommand_t function) {
    if (Cmd_Exists(cmd_name)) {
        fprintf(stderr, "Cmd_AddCommand: %s already defined\n", cmd_name);
    }
    cmd_function_t *cmd = MemAlloc(sizeof(cmd_function_t));
    cmd->name = cmd_name;
    cmd->function = function;
    ADD_TO_LIST(cmd, cmd_functions);
}

