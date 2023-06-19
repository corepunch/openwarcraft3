#include <stdlib.h>

#include "g_local.h"

#define CLIENTCOMMAND(NAME) void CMD_##NAME(edict_t *ent, DWORD argc, LPCSTR argv[])

void Matrix4_fromViewAngles(LPCVECTOR3 target, LPCVECTOR3 angles, float distance, LPMATRIX4 output) {
    VECTOR3 const vieworg = Vector3_unm(target);
    Matrix4_identity(output);
    Matrix4_translate(output, &(VECTOR3){0, 0, -distance});
    Matrix4_rotate(output, angles, ROTATE_ZYX);
    Matrix4_translate(output, &vieworg);
}

void Matrix4_getPlayerMatrix(playerState_t const *ps, LPMATRIX4 output) {
    MATRIX4 proj, view;
    Matrix4_perspective(&proj, ps->fov, 1, 100, 5000);
    Matrix4_fromViewAngles(&ps->origin, &ps->viewangles, ps->distance, &view);
    Matrix4_multiply(&proj, &view, output);
}

bool G_TraceLineEntity(LPCLINE3 line, edict_t *ent) {
    SPHERE3 const sphere = {
        .center = ent->s.origin,
        .radius = ent->s.radius
    };
    return Line3_intersect_sphere3(line, &sphere, NULL);
}

void client_deselectentities(gclient_t *client) {
    FOR_LOOP(i, globals.num_edicts) {
        globals.edicts[i].selected &= ~(1 << client->ps.number);
    }
}

void client_selectentity(gclient_t *client, edict_t *ent) {
    ent->selected |= 1 << client->ps.number;
}

bool client_isentityselected(gclient_t *client, edict_t *ent) {
    return ent->selected & (1 << client->ps.number);
}

LINE3 client_getmouseline(gclient_t *client, LPCVECTOR2 p) {
    LINE3 line;
    MATRIX4 cameramat, invcammat;
    Matrix4_getPlayerMatrix(&client->ps, &cameramat);
    Matrix4_inverse(&cameramat, &invcammat);
    line.a = Matrix4_multiply_vector3(&invcammat, &(VECTOR3) { p->x, p->y, 0 });
    line.b = Matrix4_multiply_vector3(&invcammat, &(VECTOR3) { p->x, p->y, 1 });
    return line;
}

edict_t *client_getentityatpoint(gclient_t *client, LPCVECTOR2 point) {
    LINE3 line = client_getmouseline(client, point);
    FOR_LOOP(i, globals.num_edicts) {
        edict_t *ent = &globals.edicts[i];
        if (G_TraceLineEntity(&line, ent)) {
            return ent;
        }
    }
    return NULL;
}

void client_rectselectentities(gclient_t *client, LPCRECT rect) {
    MATRIX4 m;
    Matrix4_getPlayerMatrix(&client->ps, &m);
    client_deselectentities(client);
    FOR_LOOP(i, globals.num_edicts) {
        edict_t *ent = &globals.edicts[i];
        if (ent->s.player != client->ps.number)
            continue;
        VECTOR3 pos = Matrix4_multiply_vector3(&m, &ent->s.origin);
        if (Rect_contains(rect, (LPCVECTOR2)&pos)) {
            client_selectentity(client, ent);
        }
    }
}

void client_selectentityatpoint(gclient_t *client, LPCVECTOR2 point) {
    edict_t *ent = client_getentityatpoint(client, point);
    if (!ent || ent->s.player != client->ps.number)
        return;
    client_deselectentities(client);
    client_selectentity(client, ent);
}

VECTOR2 client_getpointlocation(gclient_t *client, LPCVECTOR2 point) {
    LINE3 line = client_getmouseline(client, point);
    static VECTOR3 targetorg = { 0 };
    gi.IntersectLineWithHeightmap(&line, &targetorg);
    return *((LPCVECTOR2)&targetorg);
}

CLIENTCOMMAND(BoxSelect) {
    if (argc < 5)
        return;
    gclient_t *cl = ent->client;
    VECTOR4 const box = { atof(argv[1]), atof(argv[2]), atof(argv[3]), atof(argv[4]) };
    RECT const rect = {
        MIN(box.x, box.z),
        MIN(box.y, box.w),
        fabs(box.x - box.z),
        fabs(box.y - box.w),
    };
    VECTOR2 const mouse = { rect.x, rect.y };
    cl->ps.origin.z = gi.GetHeightAtPoint(cl->ps.origin.x, cl->ps.origin.y);
    if (cl->menu.mouseup) {
        cl->menu.mouseup(ent, &mouse);
    } else if (fabs(rect.w) + fabs(rect.h) > 0.01) {
        client_rectselectentities(cl, &rect);
    } else {
        client_selectentityatpoint(cl, &mouse);
    }
    Get_Commands_f(ent);
}

void CMD_CancelCommand(edict_t *ent) {
    Get_Commands_f(ent);
}

CLIENTCOMMAND(Select) {
    bool selected = false;
    for (DWORD i = 1; i < argc; i++) {
        DWORD number = atoi(argv[i]);
        if (number >= globals.num_edicts)
            continue;
        edict_t *e = &globals.edicts[number];
        if (e->s.player == ent->client->ps.number) {
            if (!selected) {
                client_deselectentities(ent->client);
                selected = true;
            }
            client_selectentity(ent->client, e);
        }
    }
    if (selected) {
        Get_Commands_f(ent);
    }
}

CLIENTCOMMAND(CommandButton) {
    DWORD code = atoi(argv[1]);
    ent->client->lastcode = code;
    if (ent->client->menu.cmdbutton) {
        ent->client->menu.cmdbutton(ent, code);
    } else {
        ability_t *ability = GetAbilityByIndex(code);
        if (ability && ability->cmd) {
            ability->cmd(ent);
        }
    }
}

typedef struct {
    LPCSTR name;
    void (*func)(edict_t *ent, DWORD argc, LPCSTR argv[]);
} clientCommand_t;

clientCommand_t clientCommands[] = {
    { "code", CMD_CommandButton },
    { "select", CMD_Select },
    { NULL }
};

void G_ClientCommand(edict_t *ent, DWORD argc, LPCSTR argv[]) {
    for (clientCommand_t const *cmd = clientCommands; cmd->name; cmd++) {
        if (!strcmp(cmd->name, argv[0])) {
            cmd->func(ent, argc, argv);
            return;
        }
    }
}
