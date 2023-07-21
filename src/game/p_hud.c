#include "g_local.h"

#define INFO_PANEL_UNIT_DETAIL_WIDTH UI_SCALE(0.180f)
#define INFO_PANEL_UNIT_DETAIL_HEIGHT UI_SCALE(0.120f)

#define COMMAND_BUTTON_START 1

#define BUILDQUEUE_OFFSET 281

#define MULTISELECT_COLUMNS 6
#define MULTISELECT_OFFSX 310
#define MULTISELECT_OFFSY 500

#define COMMAND_BUTTON_SIZE UI_SCALE(0.0434)
#define COMMAND_BUTTONS_X UI_SCALE(0.2365)
#define COMMAND_BUTTONS_Y UI_SCALE(0.1131)

static void Init_SimpleProgressIndicator(void) {
    UI_FRAME(SimpleProgressIndicator);
    SimpleProgressIndicator->f.size.width = INFO_PANEL_UNIT_DETAIL_WIDTH;
    SimpleProgressIndicator->f.tex.index = UI_LoadTexture("SimpleXpBarConsole", true);
    SimpleProgressIndicator->f.tex.index2 = UI_LoadTexture("SimpleXpBarBorder", true);
    SimpleProgressIndicator->f.color = MAKE(COLOR32,160,0,160,255);
    UI_WriteFrameWithChildren(SimpleProgressIndicator);
}

static void Init_SimpleInfoPanelIconDamage(LPFRAMEDEF parent, LPEDICT unit) {
    DWORD const dmgBase = UNIT_ATTACK1_DAMAGE_BASE(unit->class_id);
    DWORD const dmgDice = UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(unit->class_id);
    DWORD const dmgNumSides = UNIT_ATTACK1_DAMAGE_SIDES_PER_DIE(unit->class_id);
    UI_FRAME(SimpleInfoPanelIconDamage);
    UI_CHILD_FRAME(InfoPanelIconBackdrop, SimpleInfoPanelIconDamage);
    UI_CHILD_FRAME(InfoPanelIconLevel, SimpleInfoPanelIconDamage);
    UI_CHILD_FRAME(InfoPanelIconValue, SimpleInfoPanelIconDamage);
    UI_SetParent(SimpleInfoPanelIconDamage, parent);
    UI_SetPoint(SimpleInfoPanelIconDamage, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0, UI_SCALE(-0.040));
    UI_SetText(InfoPanelIconLevel, "1");
    UI_SetText(InfoPanelIconValue, "%d - %d", dmgBase + dmgDice, dmgBase + dmgDice * dmgNumSides);
    UI_SetTexture(InfoPanelIconBackdrop, "InfoPanelIconDamagePierce", true);
}

static void Init_SimpleInfoPanelIconArmor(LPFRAMEDEF parent, LPEDICT unit) {
    UI_FRAME(SimpleInfoPanelIconArmor);
    UI_CHILD_VALUE(InfoPanelIconBackdrop, SimpleInfoPanelIconArmor, Texture, "InfoPanelIconArmorLarge", true);
    UI_CHILD_VALUE(InfoPanelIconLevel, SimpleInfoPanelIconArmor, Text, "1");
    UI_CHILD_VALUE(InfoPanelIconValue, SimpleInfoPanelIconArmor, Text, "4");
    UI_SetParent(SimpleInfoPanelIconArmor, parent);
    UI_SetPoint(SimpleInfoPanelIconArmor, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0, UI_SCALE(-0.0745));
}

static void Init_SimpleInfoPanelIconHero(LPFRAMEDEF parent, LPEDICT unit) {
    int hero_str = UNIT_STRENGTH(unit->class_id) + UNIT_STRENGTH_PER_LEVEL(unit->class_id) * unit->hero.level;
    int hero_agi = UNIT_AGILITY(unit->class_id) + UNIT_AGILITY_PER_LEVEL(unit->class_id) * unit->hero.level;
    int hero_int = UNIT_INTELLIGENCE(unit->class_id) + UNIT_INTELLIGENCE_PER_LEVEL(unit->class_id) * unit->hero.level;
    LPCSTR primary = UNIT_PRIMARY_ATTRIBUTE(unit->class_id);
    if (primary == NULL || strlen(primary) < 3)
        return;
    char icon[256] = {0};
    sprintf(icon, "InfoPanelIconHeroIcon%s", primary);
    UI_FRAME(SimpleInfoPanelIconHero);
    UI_CHILD_VALUE(InfoPanelIconHeroIcon, SimpleInfoPanelIconHero, Texture, icon, true);
    UI_CHILD_VALUE(InfoPanelIconHeroStrengthValue, SimpleInfoPanelIconHero, Text, "%d", hero_str);
    UI_CHILD_VALUE(InfoPanelIconHeroAgilityValue, SimpleInfoPanelIconHero, Text, "%d", hero_agi);
    UI_CHILD_VALUE(InfoPanelIconHeroIntellectValue, SimpleInfoPanelIconHero, Text, "%d", hero_int);
    UI_SetParent(SimpleInfoPanelIconHero, parent);
    UI_SetPoint(SimpleInfoPanelIconHero, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, UI_SCALE(0.1), UI_SCALE(-0.037));
}

static void Init_SimpleInfoPanelIconFood(LPFRAMEDEF parent, LPEDICT unit) {
    UI_FRAME(SimpleInfoPanelIconFood);
    UI_CHILD_VALUE(InfoPanelIconBackdrop, SimpleInfoPanelIconFood, Texture, "InfoPanelIconArmorLarge", true);
    UI_CHILD_VALUE(InfoPanelIconLevel, SimpleInfoPanelIconFood, Text, "1");
    UI_CHILD_VALUE(InfoPanelIconValue, SimpleInfoPanelIconFood, Text, "4");
    UI_SetParent(SimpleInfoPanelIconFood, parent);
    UI_SetPoint(SimpleInfoPanelIconFood, FRAMEPOINT_TOPLEFT, parent, FRAMEPOINT_TOPLEFT, 0, UI_SCALE(-0.0345));
}

static void Init_BuildQueue(LPFRAMEDEF infoPanel, LPEDICT unit) {
    UI_CHILD_FRAME(SimpleBuildTimeIndicator, infoPanel);
    UI_CHILD_VALUE(SimpleBuildingActionLabel, infoPanel, Text, "Constructing");
    UINAME configstring = { 0 };
    FRAMEDEF firstItem, imageList;
    
    UI_InitFrame(&firstItem, 201, FT_TEXTURE);
    UI_SetParent(&firstItem, infoPanel);
    UI_SetPoint(&firstItem, FRAMEPOINT_TOPLEFT, infoPanel, FRAMEPOINT_TOPLEFT, 100, -390);
    UI_SetSize(&firstItem, 280, 310);

    UI_CHILD_VALUE(SimpleBuildQueueBackdrop, infoPanel, Hidden, unit->currentmove->think == ai_birth);

    sprintf(configstring, "%x %x %x,", firstItem.f.number, SimpleBuildTimeIndicator->f.number, BUILDQUEUE_OFFSET);
    for (LPEDICT queue = unit->build; queue; queue = queue->build) {
        LPCSTR buttonClassName = GetClassName(queue->class_id);
        LPCSTR buttonArt = FindConfigValue(buttonClassName, STR_ART);
        LPSTR buffer = configstring+strlen(configstring);
        sprintf(buffer, "%x %x,", UI_LoadTexture(buttonArt, false), queue->s.number);
        if (queue->build == queue)
            break;
    }

    UI_InitFrame(&imageList, 200, FT_BUILDQUEUE);
    UI_SetParent(&imageList, infoPanel);
    UI_SetPoint(&imageList, FRAMEPOINT_TOPLEFT, infoPanel, FRAMEPOINT_TOPLEFT, 95, -800);
    UI_SetSize(&imageList, 200, 215);
    UI_SetText(&imageList, configstring);
    
    UI_WriteFrame(&firstItem);
    UI_WriteFrame(&imageList);
}

static void Init_SimpleInfoPanelBuildingDetail(LPFRAMEDEF bottomPanel, LPEDICT unit) {
    UI_FRAME(SimpleInfoPanelBuildingDetail);
    UI_CHILD_VALUE(SimpleBuildingNameValue, SimpleInfoPanelBuildingDetail, Text, UNIT_NAME(unit->class_id));
    UI_CHILD_VALUE(SimpleBuildTimeIndicator, SimpleInfoPanelBuildingDetail, Texture, "SimpleBuildTimeIndicator", true);
    UI_CHILD_VALUE(SimpleBuildingActionLabel, SimpleInfoPanelBuildingDetail, Text, "Training");
    UI_CHILD_VALUE(SimpleBuildingDescriptionValue, SimpleInfoPanelBuildingDetail, Text, " ");
    UI_CHILD_VALUE(SimpleBuildQueueBackdrop, SimpleInfoPanelBuildingDetail, Hidden, false);
    
    UI_SetTexture2(SimpleBuildTimeIndicator, "SimpleBuildTimeIndicatorBorder", true);
    UI_SetParent(SimpleInfoPanelBuildingDetail, bottomPanel);

//    Init_SimpleInfoPanelIconArmor(SimpleInfoPanelBuildingDetail, unit);
//    Init_SimpleInfoPanelIconFood(SimpleInfoPanelBuildingDetail, unit);
    Init_BuildQueue(SimpleInfoPanelBuildingDetail, unit);

    UI_WriteFrameWithChildren(SimpleInfoPanelBuildingDetail);
}

static void Init_SimpleInfoPanelUnitDetail(LPFRAMEDEF bottomPanel, LPEDICT unit) {
    LPCSTR unitTypeName = UNIT_NAME(unit->class_id);
    LPCSTR unitHeroName = UNIT_PROPER_NAMES(unit->class_id);
    
    UI_FRAME(SimpleInfoPanelUnitDetail);
    UI_CHILD_VALUE(SimpleNameValue, SimpleInfoPanelUnitDetail, Text, unitHeroName ? unitHeroName : unitTypeName);
    UI_CHILD_VALUE(SimpleClassValue, SimpleInfoPanelUnitDetail, Text, "Level %d %s", unit->hero.level, unitTypeName);
    
    Init_SimpleInfoPanelIconDamage(SimpleInfoPanelUnitDetail, unit);
    Init_SimpleInfoPanelIconArmor(SimpleInfoPanelUnitDetail, unit);
    Init_SimpleInfoPanelIconHero(SimpleInfoPanelUnitDetail, unit);
    
    UI_SetParent(SimpleInfoPanelUnitDetail, bottomPanel);
    UI_WriteFrameWithChildren(SimpleInfoPanelUnitDetail);
}

static LPFRAMEDEF Init_BottomPanel(void) {
    static FRAMEDEF BottomPanel;
    UI_InitFrame(&BottomPanel, 1, FT_SIMPLEFRAME);
    UI_SetSize(&BottomPanel, INFO_PANEL_UNIT_DETAIL_WIDTH, INFO_PANEL_UNIT_DETAIL_HEIGHT);
    UI_SetPointByNumber(&BottomPanel, FRAMEPOINT_BOTTOM, 0, FRAMEPOINT_BOTTOM, 0, 0);
    return &BottomPanel;
}

static LPCSTR GetBuildCommand(unitRace_t race) {
    switch (race) {
        case RACE_HUMAN: return STR_CmdBuildHuman;
        case RACE_ORC: return STR_CmdBuildOrc;
        case RACE_UNDEAD: return STR_CmdBuildUndead;
        case RACE_NIGHTELF: return STR_CmdBuildNightElf;
        default: return STR_CmdBuild;
    }
}

LPCSTR remove_quotes(LPCSTR text) {
    if (*text != '"')
        return text;
    static char text2[1024] = { 0 };
    memset(text2, 0, sizeof(text2));
    memcpy(text2, text+1, strlen(text)-2);
    return text2;
}

void UI_AddCommandButton(LPCSTR code) {
    DWORD ToolTipGoldIcon = UI_LoadTexture("ToolTipGoldIcon", true);
    DWORD ToolTipLumberIcon = UI_LoadTexture("ToolTipLumberIcon", true);
//    DWORD ToolTipStonesIcon = UI_LoadTexture("ToolTipStonesIcon", true);
//    DWORD ToolTipManaIcon = UI_LoadTexture("ToolTipManaIcon", true);
    DWORD ToolTipSupplyIcon = UI_LoadTexture("ToolTipSupplyIcon", true);
    LPCSTR art = FindConfigValue(code, STR_ART);
    LPCSTR buttonpos = FindConfigValue(code, STR_BUTTONPOS);
    LPCSTR tip = FindConfigValue(code, STR_TIP);
    LPCSTR ubertip = FindConfigValue(code, STR_UBERTIP);
    FRAMEDEF button;
    if (!art) {
        gi.error("Not ART for %s", code);
        return;
    }
    if (!buttonpos) {
        gi.error("Not BUTTONPOS for %s", code);
        return;
    }
    char tooltip[1024] = { 0 };
    DWORD x, y;
    DWORD class_id = *(DWORD const*)code;
    DWORD gold_cost = UNIT_GOLD_COST(class_id);
    DWORD limber_cost = UNIT_LUMBER_COST(class_id);
    DWORD food_cost = UNIT_FOOD_USED(class_id);
    if (gold_cost > 0 || limber_cost > 0 || food_cost > 0) {
        sprintf(tooltip+strlen(tooltip), "%s|n", tip);
//        UI_CHILD_VALUE(ToolTipText, ToolTip, Text, "Peasant\n|cffffcc00<Icon,%d> 256   <Icon,%d> 128|\nGathers resources", ToolTipGoldIcon, ToolTipSupplyIcon);
        if (gold_cost > 0) {
            sprintf(tooltip+strlen(tooltip), "<Icon,%d> %d   ", ToolTipGoldIcon, gold_cost);
        }
        if (limber_cost > 0) {
            sprintf(tooltip+strlen(tooltip), "<Icon,%d> %d   ", ToolTipLumberIcon, limber_cost);
        }
        if (food_cost > 0) {
            sprintf(tooltip+strlen(tooltip), "<Icon,%d> %d   ", ToolTipSupplyIcon, food_cost);
        }
        sprintf(tooltip+strlen(tooltip), "|n%s", remove_quotes(ubertip));
    } else {
        sprintf(tooltip, "%s|n%s", tip, remove_quotes(ubertip));
    }
    sscanf(buttonpos, "%d,%d", &x, &y);
    DWORD bx = COMMAND_BUTTONS_X + COMMAND_BUTTON_SIZE * x;
    DWORD by = COMMAND_BUTTONS_Y - COMMAND_BUTTON_SIZE * y;
    UI_InitFrame(&button, x + (y << 2) + COMMAND_BUTTON_START, FT_COMMANDBUTTON);
    UI_SetTexture(&button, art, false);
    UI_SetSize(&button, COMMAND_BUTTON_SIZE, COMMAND_BUTTON_SIZE);
    UI_SetText(&button, code);
    UI_SetPointByNumber(&button, FRAMEPOINT_CENTER, UI_PARENT, FRAMEPOINT_BOTTOM, bx, by);
//    button.f.tex.index2 = UI_LoadTexture("CommandButtonActiveHighlight", true);
    button.f.tooltip = tooltip;
    button.f.flags.alphaMode = x==0 && y==0;
    UI_WriteFrame(&button);
}

void ui_portrait(LPGAMECLIENT client) {
    LPEDICT ent = G_GetMainSelectedUnit(client);
    FRAMEDEF portrait;
    UI_InitFrame(&portrait, COMMAND_BUTTON_START, FT_PORTRAIT);
    portrait.f.tex.index = ent->s.model;
    portrait.f.size.width = 800;
    portrait.f.size.height = 800;
    UI_SetPointByNumber(&portrait, FRAMEPOINT_BOTTOMLEFT, UI_PARENT, FRAMEPOINT_BOTTOMLEFT, 2150, 300);
    UI_WriteFrame(&portrait);
}

void Get_Portrait_f(LPEDICT edict) {
    UI_WriteLayout2(edict, ui_portrait, LAYER_PORTRAIT);
}

void ui_unit_commands(LPGAMECLIENT client) {
    LPEDICT ent = G_GetMainSelectedUnit(client);
    if (!ent || ent->currentmove->think == ai_birth) return;
    LPCSTR abilities = UNIT_ABILITIES_NORMAL(ent->class_id);
    LPCSTR trains = UNIT_TRAINS(ent->class_id);
    if (UNIT_SPEED(ent->class_id) > 0) {
        UI_AddCommandButton(STR_CmdMove);
        UI_AddCommandButton(STR_CmdHoldPos);
        UI_AddCommandButton(STR_CmdPatrol);
        UI_AddCommandButton(STR_CmdStop);
    }
    if (UNIT_ATTACK1_DAMAGE_NUMBER_OF_DICE(ent->class_id) != 0) {
        UI_AddCommandButton(STR_CmdAttack);
    }
    if (UNIT_BUILDS(ent->class_id)) {
        UI_AddCommandButton(GetBuildCommand(RACE_HUMAN));
    }
    if (abilities) {
        PARSE_LIST(abilities, abil, getNextSegment) {
            LPCSTR code = gi.FindSheetCell(game.config.abilities, abil, "code");
            if (code && FindAbilityByClassname(code)) {
                UI_AddCommandButton(code);
            } else {
                gi.error("Ability not implemented: %s - %s", abil, gi.FindSheetCell(game.config.abilities, abil, "comments"));
            }
        }
    }
    if (trains) {
        PARSE_LIST(trains, unit, getNextSegment) {
            UI_AddCommandButton(unit);
        }
    }
}

DWORD NumSelectedUnits(LPGAMECLIENT client) {
    DWORD selent = 0;
    FOR_SELECTED_UNITS(client, ent) {
        selent++;
    }
    return selent;
}

void Init_SimpleInfoPanelMultiselect(LPGAMECLIENT client) {
    DWORD SimpleHpBarConsole = gi.ImageIndex("SimpleHpBarConsole");
    DWORD SimpleManaBarConsole = gi.ImageIndex("SimpleManaBarConsole");
    FRAMEDEF multiselect;
    UINAME config = { 0 };
    
    UI_InitFrame(&multiselect, 1, FT_MULTISELECT);
    
    sprintf(config, "%x %x %x %x %x,", SimpleHpBarConsole, SimpleManaBarConsole, MULTISELECT_COLUMNS, MULTISELECT_OFFSX, MULTISELECT_OFFSY);

    FOR_SELECTED_UNITS(client, ent) {
        LPCSTR art = FindConfigValue(GetClassName(ent->class_id), STR_ART);
        sprintf(config + strlen(config), "%x %x,", gi.ImageIndex(art), ent->s.number);
    }
    
    multiselect.f.text = config;
    multiselect.f.size.width = 250;
    multiselect.f.size.height = 250;
    
    UI_SetPointByNumber(&multiselect,
                        FRAMEPOINT_CENTER,
                        UI_PARENT,
                        FRAMEPOINT_BOTTOMLEFT,
                        3260,
                        820 );

    UI_WriteFrame(&multiselect);
}

void ui_unit_info(LPGAMECLIENT client) {
    if (NumSelectedUnits(client) > 1) {
        Init_SimpleInfoPanelMultiselect(client);
//        DWORD selent = 0;
//        FOR_SELECTED_UNITS(client, ent) {
//            HUD_MultiselectIcon(ent, selent++);
//        }
    } else if (NumSelectedUnits(client) > 0) {
        LPEDICT ent = G_GetMainSelectedUnit(client);
        LPFRAMEDEF bottomPanel = Init_BottomPanel();
        UI_WriteFrame(bottomPanel);
        if (ent->build) {
            Init_SimpleInfoPanelBuildingDetail(bottomPanel, ent);
        } else {
            Init_SimpleInfoPanelUnitDetail(bottomPanel, ent);
            Init_SimpleProgressIndicator();
        }
    }
}

void ui_cancel_only(LPGAMECLIENT client) {
    UI_AddCommandButton(STR_CmdCancel);
}

void Get_Commands_f(LPEDICT edict) {
    UI_WriteLayout2(edict, ui_unit_commands, LAYER_COMMANDBAR);
    UI_WriteLayout2(edict, ui_unit_info, LAYER_INFOPANEL);
    memset(&edict->client->menu, 0, sizeof(menu_t));
}

void UI_AddCancelButton(LPEDICT ent) {
    UI_WriteLayout2(ent, ui_cancel_only, LAYER_COMMANDBAR);
    memset(&ent->client->menu, 0, sizeof(menu_t));
}
