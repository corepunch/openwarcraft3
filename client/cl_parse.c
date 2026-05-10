/*
 * cl_parse.c — Parse server-to-client messages.
 *
 * CL_ParseServerMessage() is the main dispatch loop: it reads each message
 * type byte and calls the appropriate handler.  The supported message types
 * are defined in src/common/common.h (svc_* constants).
 *
 * Entity state arrives as delta-compressed packets (svc_packetentities) that
 * are applied on top of the previous frame's state.  Player state, UI layout,
 * config strings and temporary effects each have their own message types.
 */
#include "client.h"

/* Apply a stream of delta-encoded entity updates.  For each entity the server
 * sends only the fields that changed since the previous frame.  A U_REMOVE
 * flag signals that an entity should be removed from the local table. */
static void CL_ReadPacketEntities(LPSIZEBUF msg) {
#ifdef DIAG_OUTPUT
    DWORD changed_entities = 0;
    static DWORD next_menu_entity_log_time = 0;
#endif
    while (true) {
        DWORD bits = 0;
        int nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        centity_t *ent = &cl.ents[nument];
        if (bits & (1 << U_REMOVE)) {
            memset(ent, 0, sizeof(centity_t));
            continue;
        }
        ent->prev = ent->current;
        MSG_ReadDeltaEntity(msg, &ent->current, nument, bits);
#ifdef DIAG_OUTPUT
        changed_entities++;
#endif
        if (ent->serverframe != cl.frame.serverframe - 1) {
            ent->prev = ent->current;
        }
        ent->serverframe = cl.frame.serverframe;
    }
    cl.num_entities = MAX_CLIENT_ENTITIES;

#ifdef DIAG_OUTPUT
    if (Com_InMenuMode() && cl.time >= next_menu_entity_log_time) {
        DWORD printed = 0;
        DWORD total = 0;
        next_menu_entity_log_time = cl.time + 1000;
        DIAGF("CL_ReadPacketEntities(menu): changed=%u\n", (unsigned)changed_entities);
        FOR_LOOP(i, MAX_CLIENT_ENTITIES) {
            centity_t const *ce = &cl.ents[i];
            DWORD model_index = ce->current.model;
            if (!model_index)
                continue;
            total++;
            if (printed < 8) {
                LPCSTR model_name = (model_index < MAX_MODELS) ? cl.configstrings[CS_MODELS + model_index] : "<model-index-oob>";
                DIAGF(
                        "  snap-ent[%u]: num=%u model=%u (%s) origin=(%.1f %.1f %.1f) frame=%u player=%u\n",
                        (unsigned)i,
                        (unsigned)ce->current.number,
                        (unsigned)model_index,
                        model_name ? model_name : "<null>",
                        ce->current.origin.x,
                        ce->current.origin.y,
                        ce->current.origin.z,
                        (unsigned)ce->current.frame,
                        (unsigned)ce->current.player);
                printed++;
            }
        }
        DIAGF("  snap-ent-total=%u\n", (unsigned)total);
    }
#endif
}

static void CL_ParseConfigString(LPSIZEBUF msg) {
    int const index = MSG_ReadShort(msg);
    if (index == CS_STATUSBAR) {
        MSG_Read(msg, cl.configstrings[index], sizeof(*cl.configstrings));
    } else {
        MSG_ReadString(msg, cl.configstrings[index]);
    }
//    printf("%d %s\n", index, cl.configstrings[index]);
}

static void CL_ParseBaseline(LPSIZEBUF msg) {
    DWORD bits = 0;
    DWORD index = MSG_ReadEntityBits(msg, &bits);
    centity_t *cent = &cl.ents[index];
    memset(&cent->baseline, 0, sizeof(entityState_t));
    MSG_ReadDeltaEntity(msg, &cent->baseline, index, bits);
    memcpy(&cent->current, &cent->baseline, sizeof(entityState_t));
    memcpy(&cent->prev, &cent->baseline, sizeof(entityState_t));
}

/* Handle the svc_frame header: record the server frame number and time, then
 * snapshot the current entity states into their "prev" fields so the renderer
 * can interpolate between the previous and current positions. */
void CL_ParseFrame(LPSIZEBUF msg) {
    cl.frame.serverframe = MSG_ReadLong(msg);
    cl.frame.servertime = MSG_ReadLong(msg);
    cl.frame.oldclientframe = MSG_ReadLong(msg);
    cl.time = cl.frame.servertime;
    
    FOR_LOOP(index, MAX_CLIENT_ENTITIES) {
        centity_t *ce = &cl.ents[index];
        if (!ce->current.model)
            continue;
        ce->prev = ce->current;
    }
}

void CL_ParsePlayerInfo(LPSIZEBUF msg) {
    DWORD bits;
    DWORD plnum = MSG_ReadEntityBits(msg, &bits);
    MSG_ReadDeltaPlayerState(msg, &cl.playerstate, plnum, bits);
    cl.viewDef.camerastate[1] = cl.viewDef.camerastate[0];
    cl.viewDef.camerastate[0].origin.x = cl.playerstate.origin.x;
    cl.viewDef.camerastate[0].origin.y = cl.playerstate.origin.y;
    cl.viewDef.camerastate[0].origin.z = 0;
    cl.viewDef.camerastate[0].viewquat = cl.playerstate.viewquat;
    cl.viewDef.camerastate[0].distance = cl.playerstate.distance;
    cl.viewDef.camerastate[0].fov = cl.playerstate.fov;
}

/* Receive an svc_layout message from the server.  The server serializes the
 * entire UI frame tree as a binary blob; the client stores the raw blob and
 * passes it to the renderer each frame without interpreting the contents. */
void CL_ParseLayout(LPSIZEBUF msg) {
    DWORD layer = MSG_ReadByte(msg);
    DWORD payload_size = 0;
#ifdef DIAG_OUTPUT
    DWORD frame_count = 0;
    DWORD btn_singleplayer = 0;
    DWORD btn_multiplayer = 0;
    DWORD btn_options = 0;
    DWORD btn_credits = 0;
    DWORD btn_quit = 0;
    DWORD backdrop_frames = 0;
    DWORD backdrop_missing_textures = 0;
    DWORD gluebutton_frames = 0;
    DWORD gluebutton_missing_textures = 0;
    DWORD missing_log_lines = 0;
#endif
    SAFE_DELETE(cl.layout[layer], MemFree);
    DWORD start = msg->readcount;
    while (true) {
        UIFRAME ent = { 0 };
        DWORD bits = 0;
        if (msg->readcount + sizeof(WORD) * 2 > msg->cursize) {
            break;
        }
        DWORD nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        MSG_ReadDeltaUIFrame(msg, &ent, nument, bits);
        if (msg->readcount + sizeof(WORD) > msg->cursize) {
            break;
        }
#ifdef DIAG_OUTPUT
        frame_count++;
        if (ent.onclick) {
            if (!strcmp(ent.onclick, "menu singleplayer")) btn_singleplayer++;
            else if (!strcmp(ent.onclick, "menu multiplayer")) btn_multiplayer++;
            else if (!strcmp(ent.onclick, "menu options")) btn_options++;
            else if (!strcmp(ent.onclick, "menu credits")) btn_credits++;
            else if (!strcmp(ent.onclick, "menu quit")) btn_quit++;
        }
#endif
        ent.buffer.size = MSG_ReadShort(msg);
        if (msg->readcount > msg->cursize ||
            ent.buffer.size > msg->cursize - msg->readcount) {
            break;
        }

#ifdef DIAG_OUTPUT
        if (ent.buffer.size > 0) {
            LPBYTE typedata = msg->data + msg->readcount;
            if (ent.flags.type == FT_BACKDROP && ent.buffer.size >= sizeof(uiBackdrop_t)) {
                uiBackdrop_t const *bd = (uiBackdrop_t const *)typedata;
                backdrop_frames++;
                if (!bd->Background || !bd->EdgeFile) {
                    backdrop_missing_textures++;
                    if (Com_InMenuMode() && missing_log_lines < 16) {
                        DIAGF(
                                "  missing-backdrop frame=%u size=%u bg=%u edge=%u flags=0x%x cornersize=%.4f\n",
                                (unsigned)nument,
                                (unsigned)ent.buffer.size,
                                (unsigned)bd->Background,
                                (unsigned)bd->EdgeFile,
                                (unsigned)bd->CornerFlags,
                                bd->CornerSize);
                        missing_log_lines++;
                    }
                }
            }
            if ((ent.flags.type == FT_GLUETEXTBUTTON || ent.flags.type == FT_GLUEBUTTON) &&
                ent.buffer.size >= sizeof(uiGlueTextButton_t)) {
                uiGlueTextButton_t const *gb = (uiGlueTextButton_t const *)typedata;
                gluebutton_frames++;
                if (!gb->normal.Background || !gb->normal.EdgeFile) {
                    gluebutton_missing_textures++;
                    if (Com_InMenuMode() && missing_log_lines < 16) {
                        DIAGF(
                                "  missing-gluebutton frame=%u size=%u normal{bg=%u edge=%u} pushed{bg=%u edge=%u}\n",
                                (unsigned)nument,
                                (unsigned)ent.buffer.size,
                                (unsigned)gb->normal.Background,
                                (unsigned)gb->normal.EdgeFile,
                                (unsigned)gb->pushed.Background,
                                (unsigned)gb->pushed.EdgeFile);
                        missing_log_lines++;
                    }
                }
            }
        }
#endif

        msg->readcount += ent.buffer.size;
    }
    if (start > msg->cursize || msg->readcount > msg->cursize ||
        msg->readcount < start) { /* guard against malformed data and wraparound */
        return;
    }
    payload_size = msg->readcount - start;
    cl.layout[layer] = MemAlloc(sizeof(DWORD) + payload_size);
    memcpy(cl.layout[layer], &payload_size, sizeof(payload_size));
    memcpy((LPBYTE)cl.layout[layer] + sizeof(payload_size), msg->data + start, payload_size);

    if (Com_InMenuMode()) {
#ifdef DIAG_OUTPUT
        DIAGF(
            "CL_ParseLayout(menu): layer=%u bytes=%u frames=%u buttons{single=%u multi=%u options=%u credits=%u quit=%u} backdrops{%u missing=%u} gluebuttons{%u missing=%u}\n",
                (unsigned)layer,
                (unsigned)payload_size,
                (unsigned)frame_count,
                (unsigned)btn_singleplayer,
                (unsigned)btn_multiplayer,
                (unsigned)btn_options,
                (unsigned)btn_credits,
            (unsigned)btn_quit,
            (unsigned)backdrop_frames,
            (unsigned)backdrop_missing_textures,
            (unsigned)gluebutton_frames,
            (unsigned)gluebutton_missing_textures);
#endif
    }
}

void CL_ParseCursor(LPSIZEBUF msg) {
    DWORD bits = 0;
    SAFE_DELETE(cl.cursorEntity, MemFree);
    cl.cursorEntity = MemAlloc(sizeof(entityState_t));
    MSG_ReadEntityBits(msg, &bits);
    MSG_ReadDeltaEntity(msg, cl.cursorEntity, 0, bits);
    if (cl.cursorEntity->model == 0) {
        SAFE_DELETE(cl.cursorEntity, MemFree);
    }
}

void CL_MirrorMessage(LPSIZEBUF msg) {
    char buf[256] = { 0 };
    MSG_ReadString(msg, buf);
    MSG_WriteByte(&cls.netchan.message, clc_stringcmd);
    MSG_WriteString(&cls.netchan.message, buf);
}

/* Dispatch loop for a complete server message buffer.  Each iteration reads
 * one message-type byte and calls the matching handler.  An unknown type
 * stops processing and prints an error to stderr. */
void CL_ParseServerMessage(LPSIZEBUF msg) {
    BYTE pack_id = 0;
    while (MSG_Read(msg, &pack_id, 1)) {
        switch (pack_id) {
            case svc_bad:
                return;
            case svc_playerinfo:
                CL_ParsePlayerInfo(msg);
                break;
            case svc_spawnbaseline:
                CL_ParseBaseline(msg);
                break;
            case svc_packetentities:
                CL_ReadPacketEntities(msg);
                break;
            case svc_configstring:
                CL_ParseConfigString(msg);
                break;
            case svc_frame:
                CL_ParseFrame(msg);
                break;
            case svc_layout:
                CL_ParseLayout(msg);
                break;
            case svc_cursor:
                CL_ParseCursor(msg);
                break;
            case svc_mirror:
                CL_MirrorMessage(msg);
                break;
            case svc_temp_entity:
                CL_ParseTEnt(msg);
                break;
            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
