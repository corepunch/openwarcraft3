#include "common.h"

#include <ctype.h>
#include <stdlib.h>

extern void Key_WriteBindings(FILE *file);

static cvar_t *cvar_vars;

static LPSTR Cvar_CopyString(LPCSTR in) {
    size_t len = in ? strlen(in) : 0;
    LPSTR out = MemAlloc((long)len + 1);

    if (len) {
        memcpy(out, in, len);
    }
    return out;
}

static bool Cvar_NameIsValid(LPCSTR name) {
    if (!name || !*name) {
        return false;
    }
    for (LPCSTR p = name; *p; p++) {
        if (isspace((unsigned char)*p) || *p == '"' || *p == ';') {
            return false;
        }
    }
    return true;
}

static void Cvar_UpdateValue(cvar_t *var) {
    if (!var || !var->string) {
        return;
    }
    var->value = strtof(var->string, NULL);
    var->integer = (int)var->value;
}

static cvar_t *Cvar_FindVar(LPCSTR name) {
    if (!name) {
        return NULL;
    }
    FOR_EACH_LIST(cvar_t, var, cvar_vars) {
        if (!strcmp(var->name, name)) {
            return var;
        }
    }
    return NULL;
}

cvar_t *Cvar_Get(LPCSTR name, LPCSTR value, DWORD flags) {
    cvar_t *var;

    if (!Cvar_NameIsValid(name)) {
        fprintf(stderr, "Cvar_Get: invalid cvar name \"%s\"\n", name ? name : "");
        return NULL;
    }
    var = Cvar_FindVar(name);
    if (var) {
        var->flags |= flags;
        return var;
    }
    var = MemAlloc(sizeof(*var));
    var->name = Cvar_CopyString(name);
    var->string = Cvar_CopyString(value ? value : "");
    var->flags = flags;
    var->modified = true;
    Cvar_UpdateValue(var);
    ADD_TO_LIST(var, cvar_vars);
    return var;
}

cvar_t *Cvar_Set(LPCSTR name, LPCSTR value) {
    cvar_t *var = Cvar_Get(name, value ? value : "", 0);

    if (!var) {
        return NULL;
    }
    value = value ? value : "";
    if (!strcmp(var->string, value)) {
        return var;
    }
    MemFree(var->string);
    var->string = Cvar_CopyString(value);
    var->modified = true;
    Cvar_UpdateValue(var);
    return var;
}

cvar_t *Cvar_SetValue(LPCSTR name, FLOAT value) {
    char text[64];

    snprintf(text, sizeof(text), "%g", value);
    return Cvar_Set(name, text);
}

LPCSTR Cvar_String(LPCSTR name, LPCSTR fallback) {
    cvar_t *var = Cvar_FindVar(name);

    return var ? var->string : fallback;
}

int Cvar_Integer(LPCSTR name, int fallback) {
    cvar_t *var = Cvar_FindVar(name);

    return var ? var->integer : fallback;
}

FLOAT Cvar_Value(LPCSTR name, FLOAT fallback) {
    cvar_t *var = Cvar_FindVar(name);

    return var ? var->value : fallback;
}

static void Cvar_Set_f(void) {
    int argc = Cmd_Argc();
    LPCSTR name = Cmd_Argv(1);

    if (argc < 2) {
        fprintf(stderr, "usage: set <name> <value>\n");
        return;
    }
    if (argc == 2) {
        cvar_t *var = Cvar_FindVar(name);
        if (var) {
            fprintf(stderr, "\"%s\" is \"%s\"\n", var->name, var->string);
        } else {
            fprintf(stderr, "\"%s\" is undefined\n", name);
        }
        return;
    }
    Cvar_Set(name, Cmd_ArgsFrom(2));
}

static void Cvar_SetA_f(void) {
    cvar_t *var;
    int argc = Cmd_Argc();
    LPCSTR name = Cmd_Argv(1);

    if (argc < 3) {
        Cvar_Set_f();
        return;
    }
    var = Cvar_Set(name, Cmd_ArgsFrom(2));
    if (var) {
        var->flags |= CVAR_ARCHIVE;
    }
}

static void Cvar_List_f(void) {
    DWORD count = 0;

    FOR_EACH_LIST(cvar_t, var, cvar_vars) {
        fprintf(stderr, "%c %s \"%s\"\n",
                (var->flags & CVAR_ARCHIVE) ? '*' : ' ',
                var->name,
                var->string);
        count++;
    }
    fprintf(stderr, "%u cvars\n", count);
}

bool Cvar_Command(void) {
    cvar_t *var = Cvar_FindVar(Cmd_Argv(0));

    if (!var) {
        return false;
    }
    if (Cmd_Argc() == 1) {
        fprintf(stderr, "\"%s\" is \"%s\"\n", var->name, var->string);
    } else {
        Cvar_Set(var->name, Cmd_ArgsFrom(1));
    }
    return true;
}

static LPSTR Cvar_ReadLocalConfig(LPCSTR filename) {
    FILE *file;
    long fileSize;
    LPSTR buffer;

    file = fopen(filename, "rb");
    if (!file) {
        return NULL;
    }
    if (fseek(file, 0, SEEK_END) != 0) {
        fclose(file);
        return NULL;
    }
    fileSize = ftell(file);
    if (fileSize < 0 || fseek(file, 0, SEEK_SET) != 0) {
        fclose(file);
        return NULL;
    }
    buffer = MemAlloc(fileSize + 1);
    if (fileSize > 0 && fread(buffer, 1, fileSize, file) != (size_t)fileSize) {
        MemFree(buffer);
        fclose(file);
        return NULL;
    }
    fclose(file);
    buffer[fileSize] = '\0';
    return buffer;
}

void Cvar_LoadConfig(LPCSTR filename) {
    LPSTR text;

    if (!filename || !*filename) {
        return;
    }
    text = FS_ReadFileIntoString(filename);
    if (!text) {
        text = Cvar_ReadLocalConfig(filename);
    }
    if (!text) {
        return;
    }
    fprintf(stderr, "Executing %s\n", filename);
    Cbuf_AddText(text);
    Cbuf_AddText("\n");
    FS_FreeFileString(text);
}

static void Cvar_WriteEscaped(FILE *file, LPCSTR text) {
    for (LPCSTR p = text ? text : ""; *p; p++) {
        if (*p == '"') {
            fputc('\\', file);
        }
        fputc(*p, file);
    }
}

static bool Cvar_IsSessionOnly(LPCSTR name) {
    return name && (!strcmp(name, "map") || !strcmp(name, "connect"));
}

void Cvar_WriteConfig(LPCSTR filename) {
    FILE *file;
    DWORD count = 0;

    if (!filename || !*filename) {
        filename = Cvar_String("config", "");
    }
    file = fopen(filename, "wb");
    if (!file) {
        fprintf(stderr, "Couldn't write %s\n", filename);
        return;
    }
    fprintf(file, "// OpenWarcraft3 generated config\n");
    fprintf(file, "// Loaded after share/default.cfg and the build-specific defaults.\n\n");
    Key_WriteBindings(file);
    fprintf(file, "\n");
    FOR_EACH_LIST(cvar_t, var, cvar_vars) {
        if (!(var->flags & CVAR_ARCHIVE) || Cvar_IsSessionOnly(var->name)) {
            continue;
        }
        fprintf(file, "seta %s \"", var->name);
        Cvar_WriteEscaped(file, var->string);
        fprintf(file, "\"\n");
        count++;
    }
    fclose(file);
    fprintf(stderr, "Wrote %u archived cvars to %s\n", count, filename);
}

static void Cvar_WriteConfig_f(void) {
    Cvar_WriteConfig(Cmd_Argc() > 1 ? Cmd_Argv(1) : Cvar_String("config", ""));
}

static void Cvar_Exec_f(void) {
    if (Cmd_Argc() != 2) {
        fprintf(stderr, "usage: exec <filename>\n");
        return;
    }
    Cvar_LoadConfig(Cmd_Argv(1));
}

static bool Cvar_IsSwitch(LPCSTR arg, char marker) {
    return arg && arg[0] == marker && arg[1] != '\0';
}

void Cvar_ApplyConfigCommandLine(int argc, LPCSTR *argv) {
    for (int i = 1; i < argc; i++) {
        LPCSTR arg = argv[i];

        if (!arg || !*arg) {
            continue;
        }
        if (!strncmp(arg, "-config=", 8)) {
            Cvar_Set("config", arg + 8);
            continue;
        }
        if (!strcmp(arg, "+set") && i + 2 < argc && !strcmp(argv[i + 1], "config")) {
            Cvar_Set("config", argv[i + 2]);
            i += 2;
        }
    }
}

void Cvar_ApplyCommandLine(int argc, LPCSTR *argv) {
    for (int i = 1; i < argc; i++) {
        LPCSTR arg = argv[i];

        if (!arg || !*arg) {
            continue;
        }
        if (!strncmp(arg, "-map=", 5)) {
            Cvar_Set("map", arg + 5);
            continue;
        }
        if (!strncmp(arg, "-connect=", 9)) {
            Cvar_Set("connect", arg + 9);
            continue;
        }
        if (!strncmp(arg, "-data=", 6)) {
            Cvar_Set("fs_data", arg + 6);
            continue;
        }
        if (arg[0] == '-' && strchr(arg, '=')) {
            LPCSTR equals = strchr(arg, '=');
            char name[128];
            size_t len = (size_t)(equals - (arg + 1));

            if (len >= sizeof(name)) {
                len = sizeof(name) - 1;
            }
            memcpy(name, arg + 1, len);
            name[len] = '\0';
            Cvar_Set(name, equals + 1);
            continue;
        }
        if (!strcmp(arg, "+set") && i + 2 < argc) {
            Cvar_Set(argv[i + 1], argv[i + 2]);
            i += 2;
            continue;
        }
        if (!strcmp(arg, "+map") && i + 1 < argc) {
            Cvar_Set("map", argv[i + 1]);
            i += 1;
            continue;
        }
        if (Cvar_IsSwitch(arg, '+')) {
            char line[1024];
            DWORD used;

            snprintf(line, sizeof(line), "%s", arg + 1);
            used = (DWORD)strlen(line);
            while (i + 1 < argc
                   && !Cvar_IsSwitch(argv[i + 1], '+')
                   && !Cvar_IsSwitch(argv[i + 1], '-')) {
                i++;
                if (used + strlen(argv[i]) + 2 >= sizeof(line)) {
                    break;
                }
                line[used++] = ' ';
                snprintf(line + used, sizeof(line) - used, "%s", argv[i]);
                used = (DWORD)strlen(line);
            }
            Cbuf_AddText(line);
            Cbuf_AddText("\n");
        }
    }
}

void Cvar_Init(void) {
    Cmd_AddCommand("set", Cvar_Set_f);
    Cmd_AddCommand("seta", Cvar_SetA_f);
    Cmd_AddCommand("cvarlist", Cvar_List_f);
    Cmd_AddCommand("exec", Cvar_Exec_f);
    Cmd_AddCommand("writeconfig", Cvar_WriteConfig_f);

#ifdef WOW
    Cvar_Get("config", "share/openwow-config.cfg", CVAR_ARCHIVE);
#else
    Cvar_Get("config", "share/openwarcraft3-config.cfg", CVAR_ARCHIVE);
#endif
    Cvar_Get("fs_data", "", CVAR_ARCHIVE);
    Cvar_Get("map", "", 0);
    Cvar_Get("connect", "", 0);
    Cvar_Get("r_module", "renderer", CVAR_ARCHIVE);
    Cvar_Get("ui_module", "ui", CVAR_ARCHIVE);
    Cvar_Get("g_module", "game", CVAR_ARCHIVE);
    Cvar_Get("ui_start_route", "/main", CVAR_ARCHIVE);
    Cvar_Get("ui_game_setup_map", "", 0);
    Cvar_Get("net_enabled", "1", CVAR_ARCHIVE);
    Cvar_Get("com_frame_limit", "0", 0);
}
