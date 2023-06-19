#include "g_local.h"

EDICT_FUNC(build_walk);

static EDICT_FUNC(ai_walk) {
//    if (M_DistanceToGoal(ent) <= M_MoveDistance(ent)) {
//        ent->stand(ent);
//    } else {
//        M_ChangeAngle(ent);
//        M_MoveInDirection(ent);
//    }
}

static umove_t build_move_walk = { "walk", ai_walk };
static umove_t build_move_stand = { "stand", ai_stand };

static void FillUnitData(entityState_t *ent, DWORD unit_id, LPCSTR anim) {
    PATHSTR buffer = { 0 };
    LPCSTR model_filename = UNIT_MODEL(unit_id);
    if (!model_filename)
        return;
    sprintf(buffer, "%s.mdx", model_filename);
    memset(ent, 0, sizeof(entityState_t));
    ent->model = gi.ModelIndex(buffer);
    ent->scale = UNIT_SCALING_VALUE(unit_id);
    ent->angle = -M_PI / 2;
    animation_t const *animation = gi.GetAnimation(ent->model, anim);
    if (animation) {
        ent->frame = animation->interval[0];
    }
}

static void BuildAtLocation(gclient_t *client, LPCVECTOR2 mouse) {
    VECTOR2 location = client_getpointlocation(client, mouse);
    edict_t *build = G_Spawn();
    build->class_id = client->lastcode;
    build->s.origin.x = location.x;
    build->s.origin.y = location.y;
    build->s.origin.z = gi.GetHeightAtPoint(build->s.origin.x, build->s.origin.y);
    build->s.scale = 1;
    build->s.angle = -M_PI / 2;
    SP_CallSpawn(build);
    build->birth(build);
}

void build_menu_buildnew(edict_t *ent, LPCVECTOR2 mouse) {
    entityState_t empty;
    BuildAtLocation(ent->client, mouse);
    memset(&empty, 0, sizeof(entityState_t));
    gi.WriteByte(svc_cursor);
    gi.WriteEntity(&empty);
    gi.unicast(ent);
}

void build_menu_selectlocation(edict_t *ent, DWORD building_id) {
    entityState_t cursor;
    FillUnitData(&cursor, building_id, "stand");
    UI_AddCancelButton(ent);
    gi.WriteByte(svc_cursor);
    gi.WriteEntity(&cursor);
    gi.unicast(ent);
    ent->client->menu.mouseup = build_menu_buildnew;
}

void build_command(edict_t *edict) {
    uiFrameDef_t *layer = UI_Clear();
    edict_t *ent = GetMainSelectedEntity(edict->client);
    LPCSTR builds = UNIT_BUILDS(ent->class_id);
    if (!builds)
        return;
    PARSE_LIST(builds, build, gi.ParserGetToken) {
        LPCSTR art = FindConfigValue(build, STR_ART);
        LPCSTR buttonpos = FindConfigValue(build, STR_BUTTONPOS);
        if (!art || !buttonpos)
            return;
        DWORD code = *((DWORD *)build);
        Add_CommandButtonCoded(layer, code, art, buttonpos);
    }
    UI_AddAbilityButton(layer, STR_CmdCancel);
    UI_WriteLayout(edict, layer, UI_COMMANDBAR);
    edict->client->menu.mouseup = build_menu_buildnew;
    edict->client->menu.cmdbutton = build_menu_selectlocation;
}

void build_start(edict_t *self, edict_t *target) {
//    self->goalentity = target;
    M_SetMove(self, &build_move_stand);
}

void SP_ability_build(ability_t *self) {
    self->use = build_start;
    self->cmd = build_command;
}
