#include "g_local.h"

EDICT_FUNC(build_walk);
EDICT_FUNC(build_build);

static EDICT_FUNC(ai_walk) {
    if (M_DistanceToGoal(ent) <= M_MoveDistance(ent)) {
        build_build(ent);
    } else {
        M_ChangeAngle(ent);
        M_MoveInDirection(ent);
    }
}

static EDICT_FUNC(ai_build) {
    
}

static umove_t build_move_walk = { "walk", ai_walk };
static umove_t build_move_stand = { "stand", ai_stand };
static umove_t build_move_build = { "stand", ai_build };

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

EDICT_FUNC(build_build) {
    if (player_pay(G_GetPlayerByNumber(ent->s.player), ent->build_project)) {
        SP_SpawnAtLocation(ent->build_project, ent->s.player, &ent->s.origin2);
        M_SetMove(ent, &build_move_build);
        ent->selected = 0;
    } else {
        ent->stand(ent);
    }
}

bool build_menu_send_builder(edict_t *clent, LPCVECTOR2 location) {
    edict_t *waypoint = Waypoint_add(location);
    FOR_SELECTED_UNITS(clent->client, ent) {
        ent->goalentity = waypoint;
        ent->build_project = clent->build_project;
        M_SetMove(ent, &build_move_walk);
    }
    entityState_t empty;
    memset(&empty, 0, sizeof(entityState_t));
    gi.WriteByte(svc_cursor);
    gi.WriteEntity(&empty);
    gi.unicast(clent);
    return true;
}

void build_menu_selectlocation(edict_t *ent, DWORD building_id) {
    entityState_t cursor;
    FillUnitData(&cursor, building_id, "stand");
    UI_AddCancelButton(ent);
    gi.WriteByte(svc_cursor);
    gi.WriteEntity(&cursor);
    gi.unicast(ent);
    ent->client->menu.on_location_selected = build_menu_send_builder;
    ent->build_project = building_id;
}

void build_command(edict_t *edict) {
    uiFrameDef_t *layer = UI_EmptyScreen();
    edict_t *ent = G_GetMainSelectedEntity(edict->client);
    LPCSTR builds = UNIT_BUILDS(ent->class_id);
    if (!builds)
        return;
    PARSE_LIST(builds, build, getNextSegment) {
        LPCSTR art = FindConfigValue(build, STR_ART);
        LPCSTR buttonpos = FindConfigValue(build, STR_BUTTONPOS);
        if (!art || !buttonpos)
            return;
        DWORD code = *((DWORD *)build);
        Add_CommandButtonCoded(layer, code, art, buttonpos);
    }
    UI_AddAbilityButton(layer, STR_CmdCancel);
    UI_WriteLayout(edict, layer, LAYER_COMMANDBAR);
    edict->client->menu.cmdbutton = build_menu_selectlocation;
}

void build_start(edict_t *self, edict_t *target) {
//    self->goalentity = target;
    M_SetMove(self, &build_move_stand);
}

void SP_ability_build(ability_t *self) {
    self->cmd = build_command;
}
