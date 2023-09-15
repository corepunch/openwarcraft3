#include <stdarg.h>
#include <pthread.h>

#include "server.h"

void PF_WriteByte(int c) { MSG_WriteByte(&sv.multicast, c); }
void PF_WriteShort(int c) { MSG_WriteShort(&sv.multicast, c); }
void PF_WriteLong(int c) { MSG_WriteLong(&sv.multicast, c); }
void PF_WriteFloat(float f) { MSG_WriteFloat(&sv.multicast, f); }
void PF_WriteString(LPCSTR s) { MSG_WriteString(&sv.multicast, s); }
void PF_WritePos(LPCVECTOR3 pos) { MSG_WritePos(&sv.multicast, pos); }
void PF_WriteDir(LPCVECTOR3 dir) { MSG_WriteDir(&sv.multicast, dir); }
void PF_WriteAngle(float f) { MSG_WriteAngle(&sv.multicast, f); }

void PF_TextRemoveComments(LPSTR buffer) {
    BOOL in_single_line_comment = false;
    BOOL in_block_comment = false;
    DWORD num_quotes = 0;
    char *src = buffer;
    char *dest = buffer;
    while (*src != '\0') {
        if (!in_single_line_comment && !in_block_comment) {
            if (*src == '"') {
                num_quotes++;
                *dest++ = *src++;
            } else if (num_quotes&1) {
                *dest++ = *src++;
            } else if (*src == '/' && *(src + 1) == '/') {
                in_single_line_comment = true;
                src += 2;
            } else if (*src == '/' && *(src + 1) == '*') {
                in_block_comment = true;
                src += 2;
            } else {
                *dest++ = *src++;
            }
        } else if (in_single_line_comment && *src == '\n') {
            in_single_line_comment = false;
            *dest++ = *src++;
        } else if (in_block_comment && *src == '*' && *(src + 1) == '/') {
            in_block_comment = false;
            src += 2;
        } else {
            src++;
        }
    }
    *dest = '\0';  // Null-terminate the modified buffer
}

BOMStatus PF_TextRemoveBom(LPSTR buffer) {
    unsigned char utf8BOM[] = { 0xEF, 0xBB, 0xBF };
    unsigned char utf16LEBOM[] = { 0xFF, 0xFE };
    unsigned char utf16BEBOM[] = { 0xFE, 0xFF };
    size_t length = strlen(buffer);

    if (length >= 3 && memcmp(buffer, utf8BOM, 3) == 0) {
        memmove(buffer, buffer + 3, length - 3);
        return UTF8_BOM_FOUND;
    } else if (length >= 2 && memcmp(buffer, utf16LEBOM, 2) == 0) {
        memmove(buffer, buffer + 2, length - 2);
        return UTF16LE_BOM_FOUND;
    } else if (length >= 2 && memcmp(buffer, utf16BEBOM, 2) == 0) {
        memmove(buffer, buffer + 2, length - 2);
        return UTF16BE_BOM_FOUND;
    } else {
        return NO_BOM;
    }
}

void PF_WriteEntity(LPCENTITYSTATE ent) {
    entityState_t empty;
    memset(&empty, 0, sizeof(entityState_t));
    MSG_WriteDeltaEntity(&sv.multicast, &empty, ent, true);
}

void PF_WriteUIFrame(LPCUIFRAME frame) {
    uiFrame_t empty;
    memset(&empty, 0, sizeof(uiFrame_t));
    empty.tex.coord[1] = 0xff;
    empty.tex.coord[3] = 0xff;
    MSG_WriteDeltaUIFrame(&sv.multicast, &empty, frame, true);
    MSG_WriteByte(&sv.multicast, frame->buffer.size);
    MSG_Write(&sv.multicast, frame->buffer.data, frame->buffer.size);
}

void PF_Confignstring(DWORD index, LPCSTR value, DWORD len) {
    if (index < 0 || index >= MAX_CONFIGSTRINGS) {
        fprintf(stderr, "configstring: bad index %i\n", index);
        return;
    }
    
    memset(sv.configstrings[index], 0, sizeof(sv.configstrings[index]));
    memcpy(sv.configstrings[index], value, len);
    
    //    if (sv.state != ss_loading)
    //    {    // send the update to everyone
    //        SZ_Clear (&sv.multicast);
    //        MSG_WriteChar (&sv.multicast, svc_configstring);
    //        MSG_WriteShort (&sv.multicast, index);
    //        MSG_WriteString (&sv.multicast, value);
    //
    //        SV_Multicast (vec3_origin, MULTICAST_ALL_R);
    //    }
}

void PF_Configstring(DWORD index, LPCSTR value) {
    if (!value) {
        value = "";
    }

    PF_Confignstring(index, value, (DWORD)(strlen(value) + 1));
    strcpy(sv.configstrings[index], value);
}

DWORD SV_GetTime(void) {
    return sv.time;
}

void PF_Unicast(edict_t *ent) {
    if (!ent)
        return;
    DWORD p = NUM_FOR_EDICT(ent);
    if (p < 1 || p > ge->max_clients)
        return;
    LPCLIENT client = svs.clients + (p-1);
    SZ_Write(&client->netchan.message, sv.multicast.data, sv.multicast.cursize);
    SZ_Clear(&sv.multicast);
}

void PF_error(LPCSTR fmt, ...) {
    char msg[1024];
    va_list argptr;
    va_start(argptr,fmt);
    vsprintf(msg, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "Game Error: %s\n", msg);
}

LPCANIMATION SV_GetAnimation(DWORD modelindex, LPCSTR animname) {
    struct cmodel *model = sv.models[modelindex];
    if (!model)
        return NULL;
    FOR_LOOP(i, model->num_animations) {
        if (!strcmp(animname, model->animations[i].name)) {
            return &model->animations[i];
        }
    }
    return NULL;
}

VECTOR2 get_flow_direction(DWORD heatmapindex, float fx, float fy);
struct thread {
    pthread_t thread;
    BOOL used;
};

struct thread threads[NUM_THREADS] = { 0 };

DWORD PF_CreateThread(HANDLE (func)(HANDLE), HANDLE args) {
    FOR_LOOP(i, NUM_THREADS) {
        if (!threads[i].used) {
            if (pthread_create(&threads[i].thread, NULL, func, args) != 0) {
                fprintf(stderr, "Error creating thread\n");
                exit(EXIT_FAILURE);
            } else {
                return i;
            }
        }
    }
    return -1;
}

void PF_JoinThread(DWORD thread) {
    if (pthread_join(threads[thread].thread, NULL) != 0) {
        fprintf(stderr, "Error joining thread %d\n", thread);
        exit(EXIT_FAILURE);
    }
}

void PF_Sleep(DWORD msec) {
    usleep(msec * 1000);
}

void SV_InitGameProgs(void) {
    struct game_import import;
    
    import.multicast = SV_Multicast;
    import.unicast = PF_Unicast;
        
    import.MemAlloc = MemAlloc;
    import.MemFree = MemFree;
    import.ModelIndex = SV_ModelIndex;
    import.ImageIndex = SV_ImageIndex;
    import.SoundIndex = SV_SoundIndex;
    import.FontIndex = SV_FontIndex;
    import.ReadSheet = FS_ParseSLK;
    import.ReadConfig = FS_ParseINI;
    import.FindSheetCell = FS_FindSheetCell;
    import.GetTime = SV_GetTime;
    import.GetAnimation = SV_GetAnimation;
    import.GetHeightAtPoint = CM_GetHeightAtPoint;
    import.BuildHeatmap = CM_BuildHeatmap;
    import.GetFlowDirection = get_flow_direction;
    import.ReadFileIntoString = FS_ReadFileIntoString;
    import.ReadFile = FS_ReadFile;
    import.ExtractFile = FS_ExtractFile;
    import.error = PF_error;
    import.configstring = PF_Configstring;
    import.confignstring = PF_Confignstring;
    import.WriteByte = PF_WriteByte;
    import.WriteShort = PF_WriteShort;
    import.WriteLong = PF_WriteLong;
    import.WriteFloat = PF_WriteFloat;
    import.WriteString = PF_WriteString;
    import.WritePosition = PF_WritePos;
    import.WriteDirection = PF_WriteDir;
    import.WriteAngle = PF_WriteAngle;
    import.WriteEntity = PF_WriteEntity;
    import.WriteUIFrame = PF_WriteUIFrame;
    import.CreateThread = PF_CreateThread;
    import.JoinThread = PF_JoinThread;
    import.Sleep = PF_Sleep;
    import.TextRemoveComments = PF_TextRemoveComments;
    import.TextRemoveBom = PF_TextRemoveBom;

    ge = GetGameAPI(&import);
    ge->Init();
}
