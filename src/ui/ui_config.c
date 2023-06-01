#include "ui_local.h"

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

static configValue_t *abilityConfigs = NULL;

void UI_InitConfigFiles(void) {
    for (LPCSTR *config = config_files; *config; config++) {
        configValue_t *current = imp.ParseConfig(*config);
        if (current) {
            PUSH_BACK(configValue_t, current, abilityConfigs);
        }
    }
}

void UI_ShutdownConfigFiles(void) {
    
}

LPCSTR UI_FindConfigValue(LPCSTR category, LPCSTR field) {
    return imp.FindConfigValue(abilityConfigs, category, field);
}

LPCSTR UI_GetClassName(DWORD class_id) {
    static char classname[5] = { 0 };
    memcpy(classname, &class_id, 4);
    return classname;
}
