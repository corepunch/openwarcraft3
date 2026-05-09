#include "g_local.h"
#include "g_metadata.h"

LPCSTR config_files[] = {
    "Units\\OrcAbilityStrings.txt",
    "Units\\HumanUnitFunc.txt",
    "Units\\OrcUpgradeFunc.txt",
    "Units\\CommandFunc.txt",
    "Units\\UndeadUpgradeFunc.txt",
    "Units\\CommonAbilityStrings.txt",
    "Units\\CommandStrings.txt",
    "Units\\UndeadUnitFunc.txt",
    "Units\\OrcUpgradeStrings.txt",
    "Units\\CommonAbilityFunc.txt",
    "Units\\CampaignUnitStrings.txt",
    "Units\\HumanAbilityFunc.txt",
    "Units\\ItemFunc.txt",
    "Units\\NeutralAbilityFunc.txt",
    "Units\\Telemetry.txt",
    "Units\\ItemStrings.txt",
    "Units\\NightElfUnitStrings.txt",
    "Units\\UnitGlobalStrings.txt",
    "Units\\UndeadAbilityFunc.txt",
    "Units\\ItemAbilityFunc.txt",
    "Units\\HumanUpgradeFunc.txt",
    "Units\\CampaignUnitFunc.txt",
    "Units\\NeutralUnitStrings.txt",
    "Units\\NeutralAbilityStrings.txt",
    "Units\\UndeadAbilityStrings.txt",
    "Units\\OrcUnitStrings.txt",
    "Units\\NightElfUpgradeStrings.txt",
    "Units\\OrcUnitFunc.txt",
    "Units\\NightElfUnitFunc.txt",
    "Units\\HumanUpgradeStrings.txt",
    "Units\\ItemAbilityStrings.txt",
    "Units\\HumanUnitStrings.txt",
    "Units\\NightElfUpgradeFunc.txt",
    "Units\\NeutralUnitFunc.txt",
    "Units\\HumanAbilityStrings.txt",
    "Units\\OrcAbilityFunc.txt",
    "Units\\NightElfAbilityStrings.txt",
    "Units\\UndeadUnitStrings.txt",
    "Units\\NightElfAbilityFunc.txt",
    "Units\\MiscData.txt",
    "Units\\UndeadUpgradeStrings.txt",
    NULL
};

LPCSTR profile_files[] = {
    "Units\\CampaignUnitFunc.txt",
    "Units\\CampaignUnitStrings.txt",
    "Units\\HumanUnitFunc.txt",
    "Units\\HumanUnitStrings.txt",
    "Units\\NeutralUnitFunc.txt",
    "Units\\NeutralUnitStrings.txt",
    "Units\\NightElfUnitFunc.txt",
    "Units\\NightElfUnitStrings.txt",
    "Units\\OrcUnitFunc.txt",
    "Units\\OrcUnitStrings.txt",
    "Units\\UndeadUnitFunc.txt",
    "Units\\UndeadUnitStrings.txt",
    // optionals
    "Units\\UnitSkin.txt",
    "Units\\UnitWeaponsFunc.txt",
    "Units\\UnitWeaponsSkin.txt",
    NULL
};

static sheetRow_t *abilityConfigs = NULL;

sheetRow_t *Doodads = NULL;

void G_SetConfigTable(sheetMetaData_t *metadatas, LPCSTR slk, sheetRow_t *table) {
    for (sheetMetaData_t *d = metadatas; d->id; d++) {
        if (!strcmp(d->slk, slk)) {
            d->table = table;
        }
    }
}

sheetMetaData_t *G_FindMetaData(sheetMetaData_t *metadatas, LPCSTR name) {
    for (sheetMetaData_t *d = metadatas; d->id; d++) {
        if (!strcmp(d->id, name)) {
            return d;
        }
    }
    return NULL;
}

LPCSTR UnitStringField(sheetMetaData_t *metadatas, DWORD unit_id, LPCSTR name) {
    FOR_LOOP(n, level.mapinfo->num_userCreatedUnits) {
        if (level.mapinfo->userCreatedUnits[n].newUnitID == unit_id) {
            unit_id = level.mapinfo->userCreatedUnits[n].originalUnitID;
//            printf("%.4s\n", &unit_id);
//            int a= 0;
        }
    }
    sheetMetaData_t *metadata = G_FindMetaData(metadatas, name);
    if (metadata && metadata->table) {
        return gi.FindSheetCell(metadata->table, GetClassName(unit_id), metadata->field);
    } else {
        return NULL;
    }
}

LONG UnitIntegerField(sheetMetaData_t *metadatas, DWORD unit_id, LPCSTR name) {
    LPCSTR str = UnitStringField(metadatas, unit_id, name);
    return str ? atoi(str) : 0;
}

BOOL UnitBooleanField(sheetMetaData_t *metadatas, DWORD unit_id, LPCSTR name) {
    LPCSTR str = UnitStringField(metadatas, unit_id, name);
    return str && (atoi(str) != 0 || !strcmp(str, "TRUE"));
}

FLOAT UnitRealField(sheetMetaData_t *metadatas, DWORD unit_id, LPCSTR name) {
    LPCSTR str = UnitStringField(metadatas, unit_id, name);
    return str ? atof(str) : 0;
}


void InitUnitData(void) {
    sheetRow_t *Profile = NULL;
    
    for (LPCSTR *config = config_files; *config; config++) {
        sheetRow_t *current = gi.ReadConfig(*config);
        if (current) {
            PUSH_BACK(sheetRow_t, current, abilityConfigs);
        }
    }
    
    for (LPCSTR *config = profile_files; *config; config++) {
        sheetRow_t *current = gi.ReadConfig(*config);
        if (current) {
            PUSH_BACK(sheetRow_t, current, Profile);
        }
    }
    
    sheetRow_t *DestructableData = gi.ReadSheet("Units\\DestructableData.slk");
    Doodads = gi.ReadSheet("Doodads\\Doodads.slk");
    
    G_SetConfigTable(UnitsMetaData, "Profile", Profile);
    G_SetConfigTable(UnitsMetaData, "UnitAbilities", gi.ReadSheet("Units\\UnitAbilities.slk"));
    G_SetConfigTable(UnitsMetaData, "UnitBalance", gi.ReadSheet("Units\\UnitBalance.slk"));
    G_SetConfigTable(UnitsMetaData, "UnitData", gi.ReadSheet("Units\\UnitData.slk"));
    G_SetConfigTable(UnitsMetaData, "UnitUI", gi.ReadSheet("Units\\UnitUI.slk"));
    G_SetConfigTable(UnitsMetaData, "UnitWeapons", gi.ReadSheet("Units\\UnitWeapons.slk"));
    
    G_SetConfigTable(DestructableMetaData, "DestructableData", DestructableData);
    
    G_SetConfigTable(ItemsMetaData, "ItemData", gi.ReadSheet("Units\\ItemData.slk"));
}

void ShutdownUnitData(void) {
}

LPCSTR FindConfigValue(LPCSTR category, LPCSTR field) {
    return gi.FindSheetCell(abilityConfigs, category, field);
}

LPCSTR GetClassName(DWORD class_id) {
    static char classname[5] = { 0 };
    memcpy(classname, &class_id, 4);
    return classname;
}
