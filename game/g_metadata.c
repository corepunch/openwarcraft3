#include "g_local.h"
#include "g_metadata.h"
#include <sys/time.h>

static double NowSeconds(void)
{
    struct timeval tv;

    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
}

typedef struct sheet_tail_cache_entry_s {
    sheetRow_t *rows;
    sheetRow_t *tail;
    struct sheet_tail_cache_entry_s *next;
} sheet_tail_cache_entry_t;

static sheet_tail_cache_entry_t *sheet_tail_cache = NULL;

sheetRow_t *G_SheetTail(sheetRow_t *rows)
{
    sheet_tail_cache_entry_t *entry;
    sheet_tail_cache_entry_t *new_entry;
    sheetRow_t *tail;

    if (!rows) {
        return NULL;
    }

    for (entry = sheet_tail_cache; entry; entry = entry->next) {
        if (entry->rows == rows) {
            return entry->tail;
        }
    }

    tail = rows;
    while (tail->next) {
        tail = tail->next;
    }

    new_entry = (sheet_tail_cache_entry_t *)malloc(sizeof(*new_entry));
    if (!new_entry) {
        return tail;
    }
    new_entry->rows = rows;
    new_entry->tail = tail;
    new_entry->next = sheet_tail_cache;
    sheet_tail_cache = new_entry;
    return tail;
}

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
static sheetRow_t *abilityConfigsTail = NULL;
static sheetRow_t *commandFuncConfig = NULL;
static sheetRow_t *commandStringsConfig = NULL;

sheetRow_t *Doodads = NULL;

static void AppendSheetRows(sheetRow_t **head, sheetRow_t **tail, sheetRow_t *rows)
{
    sheetRow_t *rows_tail;

    if (!rows) {
        return;
    }

    rows_tail = G_SheetTail(rows);

    if (*tail) {
        (*tail)->next = rows;
    } else {
        *head = rows;
    }
    *tail = rows_tail;
}

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
    sheetRow_t *profileTail = NULL;
    double phase_start;
    double init_start = NowSeconds();

    abilityConfigs = NULL;
    abilityConfigsTail = NULL;
    commandFuncConfig = NULL;
    commandStringsConfig = NULL;
    Doodads = NULL;
    
    fprintf(stderr, "InitUnitData: load config_files\n");
    phase_start = NowSeconds();
    for (LPCSTR *config = config_files; *config; config++) {
        double file_start = NowSeconds();
        sheetRow_t *current = gi.ReadConfig(*config);
        fprintf(stderr, "InitUnitData: ReadConfig %s %.3f s\n", *config, NowSeconds() - file_start);
        if (current) {
            AppendSheetRows(&abilityConfigs, &abilityConfigsTail, current);
            if (!strcmp(*config, "Units\\CommandFunc.txt")) {
                commandFuncConfig = current;
            } else if (!strcmp(*config, "Units\\CommandStrings.txt")) {
                commandStringsConfig = current;
            }
        }
    }
    fprintf(stderr, "InitUnitData: config_files total %.3f s\n", NowSeconds() - phase_start);
    
    fprintf(stderr, "InitUnitData: load profile_files\n");
    phase_start = NowSeconds();
    for (LPCSTR *config = profile_files; *config; config++) {
        fprintf(stderr, "InitUnitData: begin ReadConfig %s\n", *config);
        double file_start = NowSeconds();
        sheetRow_t *current = gi.ReadConfig(*config);
        fprintf(stderr, "InitUnitData: ReadConfig %s %.3f s\n", *config, NowSeconds() - file_start);
        if (current) {
            double append_start = NowSeconds();
            AppendSheetRows(&Profile, &profileTail, current);
            fprintf(stderr, "InitUnitData: append profile %s %.3f s\n", *config, NowSeconds() - append_start);
        }
    }
    fprintf(stderr, "InitUnitData: profile_files total %.3f s\n", NowSeconds() - phase_start);
    
    fprintf(stderr, "InitUnitData: load slk tables\n");
    phase_start = NowSeconds();
    sheetRow_t *DestructableData = gi.ReadSheet("Units\\DestructableData.slk");
    fprintf(stderr, "InitUnitData: ReadSheet Units\\DestructableData.slk %.3f s\n", NowSeconds() - phase_start);
    phase_start = NowSeconds();
    Doodads = gi.ReadSheet("Doodads\\Doodads.slk");
    fprintf(stderr, "InitUnitData: ReadSheet Doodads\\Doodads.slk %.3f s\n", NowSeconds() - phase_start);
    
    fprintf(stderr, "InitUnitData: set metadata tables\n");
    phase_start = NowSeconds();
    G_SetConfigTable(UnitsMetaData, "Profile", Profile);
    {
        fprintf(stderr, "InitUnitData: begin ReadSheet Units\\UnitAbilities.slk\n");
        double file_start = NowSeconds();
        G_SetConfigTable(UnitsMetaData, "UnitAbilities", gi.ReadSheet("Units\\UnitAbilities.slk"));
        fprintf(stderr, "InitUnitData: ReadSheet Units\\UnitAbilities.slk %.3f s\n", NowSeconds() - file_start);
    }
    {
        fprintf(stderr, "InitUnitData: begin ReadSheet Units\\UnitBalance.slk\n");
        double file_start = NowSeconds();
        G_SetConfigTable(UnitsMetaData, "UnitBalance", gi.ReadSheet("Units\\UnitBalance.slk"));
        fprintf(stderr, "InitUnitData: ReadSheet Units\\UnitBalance.slk %.3f s\n", NowSeconds() - file_start);
    }
    {
        fprintf(stderr, "InitUnitData: begin ReadSheet Units\\UnitData.slk\n");
        double file_start = NowSeconds();
        G_SetConfigTable(UnitsMetaData, "UnitData", gi.ReadSheet("Units\\UnitData.slk"));
        fprintf(stderr, "InitUnitData: ReadSheet Units\\UnitData.slk %.3f s\n", NowSeconds() - file_start);
    }
    {
        fprintf(stderr, "InitUnitData: begin ReadSheet Units\\UnitUI.slk\n");
        double file_start = NowSeconds();
        G_SetConfigTable(UnitsMetaData, "UnitUI", gi.ReadSheet("Units\\UnitUI.slk"));
        fprintf(stderr, "InitUnitData: ReadSheet Units\\UnitUI.slk %.3f s\n", NowSeconds() - file_start);
    }
    {
        fprintf(stderr, "InitUnitData: begin ReadSheet Units\\UnitWeapons.slk\n");
        double file_start = NowSeconds();
        G_SetConfigTable(UnitsMetaData, "UnitWeapons", gi.ReadSheet("Units\\UnitWeapons.slk"));
        fprintf(stderr, "InitUnitData: ReadSheet Units\\UnitWeapons.slk %.3f s\n", NowSeconds() - file_start);
    }
    
    G_SetConfigTable(DestructableMetaData, "DestructableData", DestructableData);
    
    {
        fprintf(stderr, "InitUnitData: begin ReadSheet Units\\ItemData.slk\n");
        double file_start = NowSeconds();
        G_SetConfigTable(ItemsMetaData, "ItemData", gi.ReadSheet("Units\\ItemData.slk"));
        fprintf(stderr, "InitUnitData: ReadSheet Units\\ItemData.slk %.3f s\n", NowSeconds() - file_start);
    }
    fprintf(stderr, "InitUnitData: set metadata tables total %.3f s\n", NowSeconds() - phase_start);
    fprintf(stderr, "InitUnitData: complete %.3f s\n", NowSeconds() - init_start);
}

void ShutdownUnitData(void) {
}

LPCSTR FindConfigValue(LPCSTR category, LPCSTR field) {
    LPCSTR value;

    if (!strncmp(category, "Cmd", 3)) {
        if (commandFuncConfig) {
            value = gi.FindSheetCell(commandFuncConfig, category, field);
            if (value) {
                return value;
            }
        }
        if (commandStringsConfig) {
            value = gi.FindSheetCell(commandStringsConfig, category, field);
            if (value) {
                return value;
            }
        }
    }

    return gi.FindSheetCell(abilityConfigs, category, field);
}

LPCSTR GetClassName(DWORD class_id) {
    static char classname[5] = { 0 };
    memcpy(classname, &class_id, 4);
    return classname;
}
