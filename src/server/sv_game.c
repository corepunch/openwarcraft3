#include <stdarg.h>

#include "server.h"

void PF_WriteByte (int c) { MSG_WriteByte(&sv.multicast, c); }
void PF_WriteShort (int c) { MSG_WriteShort(&sv.multicast, c); }
void PF_WriteLong (int c) { MSG_WriteLong(&sv.multicast, c); }
void PF_WriteFloat (float f) { MSG_WriteFloat(&sv.multicast, f); }
void PF_WriteString (char *s) { MSG_WriteString(&sv.multicast, s); }
//void PF_WritePos (vec3_t pos) {MSG_WritePos (&sv.multicast, pos);}
//void PF_WriteDir (vec3_t dir) {MSG_WriteDir (&sv.multicast, dir);}
//void PF_WriteAngle (float f) {MSG_WriteAngle (&sv.multicast, f);}

void PF_Configstring(DWORD index, LPCSTR value)
{
    if (index < 0 || index >= MAX_CONFIGSTRINGS) {
        fprintf(stderr, "configstring: bad index %i\n", index);
        return;
    }

    if (!value) {
        value = "";
    }

    strcpy(sv.configstrings[index], value);

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

void PF_error(char *fmt, ...) {
    char msg[1024];
    va_list argptr;
    va_start(argptr,fmt);
    vsprintf(msg, fmt, argptr);
    va_end(argptr);
    fprintf(stderr, "Game Error: %s\n", msg);
}

animationInfo_t const *SV_GetAnimation(int modelindex, animationType_t animname) {
    struct cmodel *model = sv.models[modelindex];
    if (!model)
        return NULL;
    animationTypeVariants_t const *av = &model->animtypes[animname];
    if (av->num_animations == 0)
        return NULL;
    if (animname == ANIM_STAND && (rand() % 8) > 0) {
        return &av->animations[0];
    } else {
        return &av->animations[0];//rand() % av->num_animations];
    }
}

VECTOR2 get_flow_direction(handle_t heatmapindex, float fx, float fy);

void SV_InitGameProgs(void) {
    struct game_import import;
    
    import.multicast = SV_Multicast;
    import.unicast = PF_Unicast;
        
    import.MemAlloc = MemAlloc;
    import.MemFree = MemFree;
    import.ModelIndex = SV_ModelIndex;
    import.ImageIndex = SV_ImageIndex;
    import.SoundIndex = SV_SoundIndex;
    import.ParseSheet = FS_ParseSheet;
    import.FindConfigValue = INI_FindValue;
    import.ParseConfig = FS_ParseConfig;
    import.GetAnimation = SV_GetAnimation;
    import.GetHeightAtPoint = CM_GetHeightAtPoint;
    import.BuildHeatmap = CM_BuildHeatmap;
    import.GetFlowDirection = get_flow_direction;
    import.error = PF_error;
    import.configstring = PF_Configstring;
    import.ParserGetToken = ParserGetToken;
    import.WriteByte = PF_WriteByte;
    import.WriteShort = PF_WriteShort;
    import.WriteLong = PF_WriteLong;
    import.WriteFloat = PF_WriteFloat;
    import.WriteString = PF_WriteString;

    ge = GetGameAPI(&import);
    ge->Init();
}
