#include "s_skills.h"

static void stub_cancel_command(LPEDICT clent) {
    UI_AddCancelButton(clent);
}

static void immolation_command(LPEDICT clent) {
    LPEDICT caster = G_GetMainSelectedUnit(clent->client);
    DWORD code = MAKEFOURCC('B', 'i', 'm', 'l');

    if (!caster) {
        return;
    }
    FOR_LOOP(i, MAX_UNIT_STATUSES) {
        if (caster->abilstatus[i].level && caster->abilstatus[i].code == code) {
            memset(&caster->abilstatus[i], 0, sizeof(caster->abilstatus[i]));
            return;
        }
    }
    unit_addstatus(caster, "Biml", 1);
}

ability_t a_immolation = {
    .cmd = immolation_command,
};

ability_t a_phoenix_fire = {0};
ability_t a_cold_arrows = {0};
ability_t a_invulnerable = {0};

ability_t a_goldmine_overlayed = {0};
ability_t a_blighted_goldmine = {0};
ability_t a_blight = {0};
ability_t a_acolyte_harvest = {
    .cmd = stub_cancel_command,
};
ability_t a_return_resources = {
    .cmd = stub_cancel_command,
};
ability_t a_wisp_harvest = {
    .cmd = stub_cancel_command,
};
ability_t a_harvest_lumber = {
    .cmd = stub_cancel_command,
};
ability_t a_repair_generic = {
    .cmd = stub_cancel_command,
};

ability_t a_entangle_goldmine = {
    .cmd = stub_cancel_command,
};
ability_t a_entangled_mine = {0};

ability_t a_cargo_hold = {0};
ability_t a_cargo_hold_burrow = {0};
ability_t a_cargo_hold_entangled_mine = {0};
ability_t a_stand_down = {
    .cmd = stub_cancel_command,
};
ability_t a_load = {
    .cmd = stub_cancel_command,
};
ability_t a_drop = {
    .cmd = stub_cancel_command,
};
ability_t a_drop_instant = {
    .cmd = stub_cancel_command,
};

ability_t a_inventory = {0};
ability_t a_shop_purchase_item = {
    .cmd = stub_cancel_command,
};
ability_t a_neutral_building = {0};
ability_t a_shop_sharing = {0};
ability_t a_couple_instant = {
    .cmd = stub_cancel_command,
};
