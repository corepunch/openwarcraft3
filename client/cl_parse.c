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
    int count = 0;
    int previous = 0;
    while (true) {
        DWORD bits = 0;
        if (msg->readcount + sizeof(DWORD) + sizeof(WORD) > msg->cursize) {
            break;
        }
        int nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0)
            break;
        if (nument < 0 || nument >= MAX_CLIENT_ENTITIES) {
            fprintf(stderr,
                    "CL_ReadPacketEntities: bad entity %d bits=0x%x count=%d previous=%d read=%u size=%u frame=%d\n",
                    nument,
                    (unsigned)bits,
                    count,
                    previous,
                    (unsigned)msg->readcount,
                    (unsigned)msg->cursize,
                    cl.frame.serverframe);
            msg->readcount = msg->cursize;
            break;
        }
        previous = nument;
        count++;
        centity_t *ent = &cl.ents[nument];
        if (bits & (1u << U_REMOVE)) {
            memset(ent, 0, sizeof(centity_t));
            continue;
        }
        ent->prev = ent->current;
        MSG_ReadDeltaEntity(msg, &ent->current, nument, bits);
        if (ent->serverframe != cl.frame.serverframe - 1) {
            ent->prev = ent->current;
        }
        ent->serverframe = cl.frame.serverframe;
    }
    cl.num_entities = MAX_CLIENT_ENTITIES;
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
    if (index >= MAX_CLIENT_ENTITIES) {
        fprintf(stderr, "CL_ParseBaseline: bad entity %u\n", (unsigned)index);
        msg->readcount = msg->cursize;
        return;
    }
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
    DWORD plnum = MSG_ReadPlayerBits(msg, &bits);
    MSG_ReadDeltaPlayerState(msg, &cl.playerstate, plnum, bits);
    VECTOR2 server_origin = cl.playerstate.origin;

    cl.viewDef.camerastate[1] = cl.viewDef.camerastate[0];
    cl.viewDef.camerastate[0].origin.x = server_origin.x;
    cl.viewDef.camerastate[0].origin.y = server_origin.y;
    cl.viewDef.camerastate[0].origin.z = 0;
    cl.viewDef.camerastate[0].viewquat = cl.playerstate.viewquat;
    cl.viewDef.camerastate[0].viewangles = cl.playerstate.viewangles;
    cl.viewDef.camerastate[0].distance = cl.playerstate.distance;
    cl.viewDef.camerastate[0].fov = cl.playerstate.fov;

    if (cl.camera_prediction.active) {
        if (server_origin.x == cl.camera_prediction.origin.x &&
            server_origin.y == cl.camera_prediction.origin.y) {
            cl.camera_prediction.active = false;
        } else {
            cl.viewDef.camerastate[0].origin.x = cl.camera_prediction.origin.x;
            cl.viewDef.camerastate[0].origin.y = cl.camera_prediction.origin.y;
            cl.viewDef.camerastate[1].origin.x = cl.camera_prediction.origin.x;
            cl.viewDef.camerastate[1].origin.y = cl.camera_prediction.origin.y;
        }
    }
}

/* Receive an svc_layout message from the server.  The server serializes the
 * entire UI frame tree as a binary blob; the client stores the raw blob and
 * passes it to the renderer each frame without interpreting the contents. */
void CL_ParseLayout(LPSIZEBUF msg) {
    DWORD layer = MSG_ReadByte(msg);
    DWORD payload_size = 0;
    BOOL terminated = false;

    if (layer >= MAX_LAYOUT_LAYERS) {
        fprintf(stderr, "CL_ParseLayout: bad layer %u\n", (unsigned)layer);
        msg->readcount = msg->cursize;
        return;
    }

    if (ui.ClearLayoutLayer) {
        ui.ClearLayoutLayer(layer);
    }
    SAFE_DELETE(cl.layout[layer], MemFree);
    DWORD start = msg->readcount;
    while (true) {
        UIFRAME ent = { 0 };
        DWORD bits = 0;
        if (msg->readcount + sizeof(DWORD) + sizeof(WORD) > msg->cursize) {
            break;
        }
        DWORD nument = MSG_ReadEntityBits(msg, &bits);
        if (nument == 0 && bits == 0) {
            terminated = true;
            break;
        }
        MSG_ReadDeltaUIFrame(msg, &ent, nument, bits);
        if (msg->readcount + sizeof(WORD) > msg->cursize) {
            break;
        }
        ent.buffer.size = MSG_ReadShort(msg);
        if (msg->readcount > msg->cursize ||
            ent.buffer.size > msg->cursize - msg->readcount) {
            break;
        }

        msg->readcount += ent.buffer.size;
    }
    if (!terminated) {
        fprintf(stderr, "CL_ParseLayout: malformed layer %u\n", (unsigned)layer);
        msg->readcount = msg->cursize;
        return;
    }
    if (start > msg->cursize || msg->readcount > msg->cursize ||
        msg->readcount < start) { /* guard against malformed data and wraparound */
        msg->readcount = msg->cursize;
        return;
    }
    payload_size = msg->readcount - start;
    cl.layout[layer] = MemAlloc(sizeof(DWORD) + payload_size);
    memcpy(cl.layout[layer], &payload_size, sizeof(payload_size));
    memcpy((LPBYTE)cl.layout[layer] + sizeof(payload_size), msg->data + start, payload_size);
    if (ui.SetLayoutLayer) {
        ui.SetLayoutLayer(layer, cl.layout[layer]);
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

static BOOL CL_EnsureFogOfWarSize(DWORD width, DWORD height) {
    DWORD cells;

    if (!width || !height) {
        return false;
    }
    if (cl.fow.width == width && cl.fow.height == height &&
        cl.fow.visible && cl.fow.explored && cl.fow.texture)
    {
        return true;
    }

    SAFE_DELETE(cl.fow.visible, MemFree);
    SAFE_DELETE(cl.fow.explored, MemFree);
    SAFE_DELETE(cl.fow.texture, MemFree);
    cl.fow.width = width;
    cl.fow.height = height;
    cells = width * height;
    cl.fow.visible = MemAlloc(cells);
    cl.fow.explored = MemAlloc(cells);
    cl.fow.texture = MemAlloc(cells);
    if (!cl.fow.visible || !cl.fow.explored || !cl.fow.texture) {
        SAFE_DELETE(cl.fow.visible, MemFree);
        SAFE_DELETE(cl.fow.explored, MemFree);
        SAFE_DELETE(cl.fow.texture, MemFree);
        cl.fow.width = 0;
        cl.fow.height = 0;
        return false;
    }
    memset(cl.fow.visible, 0, cells);
    memset(cl.fow.explored, 0, cells);
    memset(cl.fow.texture, 0, cells);
    return true;
}

static void CL_ClearFogRows(BYTE *plane, DWORD first_row, DWORD row_count) {
    if (!plane || first_row >= cl.fow.height) {
        return;
    }
    row_count = MIN(row_count, cl.fow.height - first_row);
    memset(plane + first_row * cl.fow.width, 0, row_count * cl.fow.width);
}

static void CL_UpdateFogTexture(void) {
    DWORD cells = cl.fow.width * cl.fow.height;

    if (!cl.fow.texture || !cl.fow.visible || !cl.fow.explored) {
        return;
    }
    FOR_LOOP(i, cells) {
        cl.fow.texture[i] = cl.fow.visible[i] ? 255 : (cl.fow.explored[i] ? 128 : 0);
    }
    if (re.SetFogOfWarData) {
        re.SetFogOfWarData(cl.fow.width, cl.fow.height, cl.fow.texture);
    }
}

static BYTE *CL_FogPlaneForStreamIndex(DWORD flags, DWORD stream_index) {
    DWORD index = 0;

    if (flags & FOW_MSG_VISIBLE_PLANE) {
        if (stream_index == index) {
            return cl.fow.visible;
        }
        index++;
    }
    if (flags & FOW_MSG_EXPLORED_PLANE) {
        if (stream_index == index) {
            return cl.fow.explored;
        }
    }
    return NULL;
}

static BOOL CL_ValidateFogRLE(BYTE const *payload, DWORD payload_bytes, DWORD expected_bits) {
    DWORD bits = 0;

    if (!payload || payload_bytes < 2 || expected_bits == 0 ||
        (payload[0] != 0 && payload[0] != 1))
    {
        return false;
    }

    for (DWORD i = 1; i < payload_bytes; i++) {
        DWORD len = payload[i];
        if (bits == expected_bits) {
            return false;
        }
        bits += len;
        if (bits > expected_bits) {
            return false;
        }
    }
    return bits == expected_bits;
}

static void CL_UnpackFogRLE(BYTE const *payload,
                            DWORD payload_bytes,
                            DWORD flags,
                            DWORD first_row,
                            DWORD row_count)
{
    DWORD plane_bits = cl.fow.width * row_count;
    DWORD decoded = 0;
    BYTE value = payload[0] ? 1 : 0;

    for (DWORD i = 1; i < payload_bytes; i++) {
        DWORD len = payload[i];

        FOR_LOOP(j, len) {
            DWORD plane_index = decoded / plane_bits;
            DWORD bit_index = decoded % plane_bits;
            DWORD row = bit_index / cl.fow.width;
            DWORD x = bit_index % cl.fow.width;
            BYTE *plane = CL_FogPlaneForStreamIndex(flags, plane_index);

            if (plane) {
                plane[(first_row + row) * cl.fow.width + x] = value;
            }
            decoded++;
        }

        if (len != 255) {
            value = !value;
        }
    }
}

void CL_ParseFogOfWar(LPSIZEBUF msg) {
    DWORD flags = MSG_ReadByte(msg);
    DWORD width = MSG_ReadShort(msg);
    DWORD height = MSG_ReadShort(msg);
    DWORD first_row = MSG_ReadShort(msg);
    DWORD row_count = MSG_ReadShort(msg);
    DWORD payload_bytes = MSG_ReadShort(msg);
    DWORD plane_count = 0;
    DWORD expected_bits;
    BYTE const *payload;

    if (flags & FOW_MSG_VISIBLE_PLANE) {
        plane_count++;
    }
    if (flags & FOW_MSG_EXPLORED_PLANE) {
        plane_count++;
    }
    expected_bits = width * row_count * plane_count;
    if (!plane_count || !width || !height || !(flags & FOW_MSG_RLE) ||
        first_row >= height || row_count > height - first_row ||
        payload_bytes > msg->cursize - msg->readcount)
    {
        msg->readcount = MIN(msg->cursize, msg->readcount + payload_bytes);
        return;
    }
    payload = msg->data + msg->readcount;
    if (!CL_ValidateFogRLE(payload, payload_bytes, expected_bits)) {
        msg->readcount = MIN(msg->cursize, msg->readcount + payload_bytes);
        return;
    }
    if (!CL_EnsureFogOfWarSize(width, height)) {
        msg->readcount = MIN(msg->cursize, msg->readcount + payload_bytes);
        return;
    }

    if (flags & FOW_MSG_FULL) {
        CL_ClearFogRows(cl.fow.visible, first_row, row_count);
        CL_ClearFogRows(cl.fow.explored, first_row, row_count);
    }
    CL_UnpackFogRLE(payload, payload_bytes, flags, first_row, row_count);
    msg->readcount += payload_bytes;
    CL_UpdateFogTexture();
}

void CL_MirrorMessage(LPSIZEBUF msg) {
    char buf[256] = { 0 };
    MSG_ReadString(msg, buf);
    if (!strcmp(buf, "begin")) {
        cls.state = *cl.configstrings[CS_WORLD] ? ca_active : ca_connected;
        cl.pending_begin = true;
        return;
    }
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
            case svc_fogofwar:
                CL_ParseFogOfWar(msg);
                break;
            case svc_temp_entity:
                CL_ParseTEnt(msg);
                break;                
            // Phase 8: Unit UI data
            case svc_unit_ui:
                CL_ParseUnitUI(msg);
                break;
                            default:
                fprintf(stderr, "Unknown message %d\n", pack_id);
                return;
        }
    }
}
