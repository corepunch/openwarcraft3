#include "api.h"

DWORD ConvertRace(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "race");
}
DWORD ConvertAllianceType(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "alliancetype");
}
DWORD ConvertRacePref(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "racepreference");
}
DWORD ConvertIGameState(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "igamestate");
}
DWORD ConvertFGameState(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "fgamestate");
}
DWORD ConvertPlayerState(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "playerstate");
}
DWORD ConvertPlayerGameResult(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "playergameresult");
}
DWORD ConvertUnitState(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "unitstate");
}
DWORD ConvertGameEvent(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "gameevent");
}
DWORD ConvertPlayerEvent(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "playerevent");
}
DWORD ConvertPlayerUnitEvent(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "playerunitevent");
}
DWORD ConvertWidgetEvent(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "widgetevent");
}
DWORD ConvertDialogEvent(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "dialogevent");
}
DWORD ConvertUnitEvent(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "unitevent");
}
DWORD ConvertLimitOp(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "limitop");
}
DWORD ConvertUnitType(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "unittype");
}
DWORD ConvertGameSpeed(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "gamespeed");
}
DWORD ConvertPlacement(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "placement");
}
DWORD ConvertStartLocPrio(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "startlocprio");
}
DWORD ConvertGameDifficulty(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "gamedifficulty");
}
DWORD ConvertGameType(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "gametype");
}
DWORD ConvertMapFlag(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "mapflag");
}
DWORD ConvertMapVisibility(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "mapvisibility");
}
DWORD ConvertMapSetting(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "mapsetting");
}
DWORD ConvertMapDensity(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "mapdensity");
}
DWORD ConvertMapControl(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "mapcontrol");
}
DWORD ConvertPlayerColor(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "playercolor");
}
DWORD ConvertPlayerSlotState(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "playerslotstate");
}
DWORD ConvertVolumeGroup(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "volumegroup");
}
DWORD ConvertCameraField(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "camerafield");
}
DWORD ConvertBlendMode(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "blendmode");
}
DWORD ConvertRarityControl(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "raritycontrol");
}
DWORD ConvertTexMapFlags(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "texmapflags");
}
DWORD ConvertFogState(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "fogstate");
}
DWORD ConvertEffectType(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "effecttype");
}
DWORD OrderId(LPJASS j) {
    //LPCSTR orderIdString = jass_checkstring(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD OrderId2String(LPJASS j) {
    //LONG orderId = jass_checkinteger(j, 1);
    return jass_pushstring(j, 0);
}
DWORD UnitId(LPJASS j) {
    //LPCSTR unitIdString = jass_checkstring(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD UnitId2String(LPJASS j) {
    //LONG unitId = jass_checkinteger(j, 1);
    return jass_pushstring(j, 0);
}
DWORD AbilityId(LPJASS j) {
    //LPCSTR abilityIdString = jass_checkstring(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD AbilityId2String(LPJASS j) {
    //LONG abilityId = jass_checkinteger(j, 1);
    return jass_pushstring(j, 0);
}
DWORD Deg2Rad(LPJASS j) {
    //FLOAT degrees = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Rad2Deg(LPJASS j) {
    //FLOAT radians = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Sin(LPJASS j) {
    //FLOAT radians = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Cos(LPJASS j) {
    //FLOAT radians = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Tan(LPJASS j) {
    //FLOAT radians = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Asin(LPJASS j) {
    //FLOAT y = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Acos(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Atan(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Atan2(LPJASS j) {
    //FLOAT y = jass_checknumber(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    return jass_pushnumber(j, 0);
}
DWORD SquareRoot(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD Pow(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT power = jass_checknumber(j, 2);
    return jass_pushnumber(j, 0);
}
DWORD I2R(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD R2I(LPJASS j) {
    //FLOAT r = jass_checknumber(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD I2S(LPJASS j) {
    //LONG i = jass_checkinteger(j, 1);
    return jass_pushstring(j, 0);
}
DWORD R2S(LPJASS j) {
    //FLOAT r = jass_checknumber(j, 1);
    return jass_pushstring(j, 0);
}
DWORD R2SW(LPJASS j) {
    //FLOAT r = jass_checknumber(j, 1);
    //LONG width = jass_checkinteger(j, 2);
    //LONG precision = jass_checkinteger(j, 3);
    return jass_pushstring(j, 0);
}
DWORD S2I(LPJASS j) {
    //LPCSTR s = jass_checkstring(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD S2R(LPJASS j) {
    //LPCSTR s = jass_checkstring(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD SubString(LPJASS j) {
    //LPCSTR source = jass_checkstring(j, 1);
    //LONG start = jass_checkinteger(j, 2);
    //LONG end = jass_checkinteger(j, 3);
    return jass_pushstring(j, 0);
}
DWORD GetLocalizedString(LPJASS j) {
    //LPCSTR source = jass_checkstring(j, 1);
    return jass_pushstring(j, 0);
}
DWORD GetLocalizedHotkey(LPJASS j) {
    //LPCSTR source = jass_checkstring(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD SetMapName(LPJASS j) {
    //LPCSTR name = jass_checkstring(j, 1);
    return 0;
}
DWORD SetMapDescription(LPJASS j) {
    //LPCSTR description = jass_checkstring(j, 1);
    return 0;
}
DWORD SetTeams(LPJASS j) {
    //LONG teamcount = jass_checkinteger(j, 1);
    return 0;
}
DWORD SetPlayers(LPJASS j) {
    //LONG playercount = jass_checkinteger(j, 1);
    return 0;
}
DWORD DefineStartLocation(LPJASS j) {
    //LONG whichStartLoc = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD DefineStartLocationLoc(LPJASS j) {
    //LONG whichStartLoc = jass_checkinteger(j, 1);
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    return 0;
}
DWORD SetStartLocPrioCount(LPJASS j) {
    //LONG whichStartLoc = jass_checkinteger(j, 1);
    //LONG prioSlotCount = jass_checkinteger(j, 2);
    return 0;
}
DWORD SetStartLocPrio(LPJASS j) {
    //LONG whichStartLoc = jass_checkinteger(j, 1);
    //LONG prioSlotIndex = jass_checkinteger(j, 2);
    //LONG otherStartLocIndex = jass_checkinteger(j, 3);
    //HANDLE priority = jass_checkhandle(j, 4, "startlocprio");
    return 0;
}
DWORD GetStartLocPrioSlot(LPJASS j) {
    //LONG whichStartLoc = jass_checkinteger(j, 1);
    //LONG prioSlotIndex = jass_checkinteger(j, 2);
    return jass_pushinteger(j, 0);
}
DWORD GetStartLocPrio(LPJASS j) {
    //LONG whichStartLoc = jass_checkinteger(j, 1);
    //LONG prioSlotIndex = jass_checkinteger(j, 2);
    return jass_pushhandle(j, 0, "startlocprio");
}
DWORD SetGameTypeSupported(LPJASS j) {
    //HANDLE whichGameType = jass_checkhandle(j, 1, "gametype");
    //BOOL value = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetMapFlag(LPJASS j) {
    //HANDLE whichMapFlag = jass_checkhandle(j, 1, "mapflag");
    //BOOL value = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetGamePlacement(LPJASS j) {
    //HANDLE whichPlacementType = jass_checkhandle(j, 1, "placement");
    return 0;
}
DWORD SetGameSpeed(LPJASS j) {
    //HANDLE whichspeed = jass_checkhandle(j, 1, "gamespeed");
    return 0;
}
DWORD SetGameDifficulty(LPJASS j) {
    //HANDLE whichdifficulty = jass_checkhandle(j, 1, "gamedifficulty");
    return 0;
}
DWORD SetResourceDensity(LPJASS j) {
    //HANDLE whichdensity = jass_checkhandle(j, 1, "mapdensity");
    return 0;
}
DWORD SetCreatureDensity(LPJASS j) {
    //HANDLE whichdensity = jass_checkhandle(j, 1, "mapdensity");
    return 0;
}
DWORD GetTeams(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetPlayers(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD IsGameTypeSupported(LPJASS j) {
    //HANDLE whichGameType = jass_checkhandle(j, 1, "gametype");
    return jass_pushboolean(j, 0);
}
DWORD GetGameTypeSelected(LPJASS j) {
    return jass_pushhandle(j, 0, "gametype");
}
DWORD IsMapFlagSet(LPJASS j) {
    //HANDLE whichMapFlag = jass_checkhandle(j, 1, "mapflag");
    return jass_pushboolean(j, 0);
}
DWORD GetGamePlacement(LPJASS j) {
    return jass_pushhandle(j, 0, "placement");
}
DWORD GetGameSpeed(LPJASS j) {
    return jass_pushhandle(j, 0, "gamespeed");
}
DWORD GetGameDifficulty(LPJASS j) {
    return jass_pushhandle(j, 0, "gamedifficulty");
}
DWORD GetResourceDensity(LPJASS j) {
    return jass_pushhandle(j, 0, "mapdensity");
}
DWORD GetCreatureDensity(LPJASS j) {
    return jass_pushhandle(j, 0, "mapdensity");
}
DWORD GetStartLocationX(LPJASS j) {
    //LONG whichStartLocation = jass_checkinteger(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD GetStartLocationY(LPJASS j) {
    //LONG whichStartLocation = jass_checkinteger(j, 1);
    return jass_pushnumber(j, 0);
}
DWORD GetStartLocationLoc(LPJASS j) {
    //LONG whichStartLocation = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "location");
}
DWORD SetPlayerTeam(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG whichTeam = jass_checkinteger(j, 2);
    return 0;
}
DWORD SetPlayerStartLocation(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG startLocIndex = jass_checkinteger(j, 2);
    return 0;
}
DWORD ForcePlayerStartLocation(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG startLocIndex = jass_checkinteger(j, 2);
    return 0;
}
DWORD SetPlayerColor(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE color = jass_checkhandle(j, 2, "playercolor");
    return 0;
}
DWORD SetPlayerAlliance(LPJASS j) {
    //HANDLE sourcePlayer = jass_checkhandle(j, 1, "player");
    //HANDLE otherPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichAllianceSetting = jass_checkhandle(j, 3, "alliancetype");
    //BOOL value = jass_checkboolean(j, 4);
    return 0;
}
DWORD SetPlayerTaxRate(LPJASS j) {
    //HANDLE sourcePlayer = jass_checkhandle(j, 1, "player");
    //HANDLE otherPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichResource = jass_checkhandle(j, 3, "playerstate");
    //LONG rate = jass_checkinteger(j, 4);
    return 0;
}
DWORD SetPlayerRacePreference(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichRacePreference = jass_checkhandle(j, 2, "racepreference");
    return 0;
}
DWORD SetPlayerRaceSelectable(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //BOOL value = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetPlayerController(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE controlType = jass_checkhandle(j, 2, "mapcontrol");
    return 0;
}
DWORD SetPlayerOnScoreScreen(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD GetPlayerTeam(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushinteger(j, 0);
}
DWORD GetPlayerStartLocation(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushinteger(j, 0);
}
DWORD GetPlayerColor(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushhandle(j, 0, "playercolor");
}
DWORD GetPlayerSelectable(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushboolean(j, 0);
}
DWORD GetPlayerController(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushhandle(j, 0, "mapcontrol");
}
DWORD GetPlayerSlotState(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushhandle(j, 0, "playerslotstate");
}
DWORD GetPlayerTaxRate(LPJASS j) {
    //HANDLE sourcePlayer = jass_checkhandle(j, 1, "player");
    //HANDLE otherPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichResource = jass_checkhandle(j, 3, "playerstate");
    return jass_pushinteger(j, 0);
}
DWORD IsPlayerRacePrefSet(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE pref = jass_checkhandle(j, 2, "racepreference");
    return jass_pushboolean(j, 0);
}
DWORD GetPlayerName(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushstring(j, 0);
}
DWORD CreateTimer(LPJASS j) {
    return jass_pushhandle(j, 0, "timer");
}
DWORD DestroyTimer(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    return 0;
}
DWORD TimerStart(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    //FLOAT timeout = jass_checknumber(j, 2);
    //BOOL periodic = jass_checkboolean(j, 3);
    //LPCSTR handlerFunc = jass_checkcode(j, 4);
    return 0;
}
DWORD TimerGetElapsed(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    return jass_pushnumber(j, 0);
}
DWORD TimerGetRemaining(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    return jass_pushnumber(j, 0);
}
DWORD TimerGetTimeout(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    return jass_pushnumber(j, 0);
}
DWORD PauseTimer(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    return 0;
}
DWORD ResumeTimer(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    return 0;
}
DWORD GetExpiredTimer(LPJASS j) {
    return jass_pushhandle(j, 0, "timer");
}
DWORD CreateGroup(LPJASS j) {
    return jass_pushhandle(j, 0, "group");
}
DWORD GroupAddUnit(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    return 0;
}
DWORD GroupRemoveUnit(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    return 0;
}
DWORD GroupClear(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    return 0;
}
DWORD GroupEnumUnitsOfType(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD GroupEnumUnitsOfPlayer(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD GroupEnumUnitsOfTypeCounted(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    //LONG countLimit = jass_checkinteger(j, 4);
    return 0;
}
DWORD GroupEnumUnitsInRect(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE r = jass_checkhandle(j, 2, "rect");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD GroupEnumUnitsInRectCounted(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE r = jass_checkhandle(j, 2, "rect");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    //LONG countLimit = jass_checkinteger(j, 4);
    return 0;
}
DWORD GroupEnumUnitsInRange(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT radius = jass_checknumber(j, 4);
    //HANDLE filter = jass_checkhandle(j, 5, "boolexpr");
    return 0;
}
DWORD GroupEnumUnitsInRangeOfLoc(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    //FLOAT radius = jass_checknumber(j, 3);
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    return 0;
}
DWORD GroupEnumUnitsInRangeCounted(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT radius = jass_checknumber(j, 4);
    //HANDLE filter = jass_checkhandle(j, 5, "boolexpr");
    //LONG countLimit = jass_checkinteger(j, 6);
    return 0;
}
DWORD GroupEnumUnitsInRangeOfLocCounted(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    //FLOAT radius = jass_checknumber(j, 3);
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    //LONG countLimit = jass_checkinteger(j, 5);
    return 0;
}
DWORD GroupEnumUnitsSelected(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD GroupImmediateOrder(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD GroupImmediateOrderById(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrder(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrderLoc(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrderById(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrderByIdLoc(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    return jass_pushboolean(j, 0);
}
DWORD GroupTargetOrder(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD GroupTargetOrderById(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD ForGroup(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR callback = jass_checkcode(j, 2);
    return 0;
}
DWORD FirstOfGroup(LPJASS j) {
    //HANDLE whichGroup = jass_checkhandle(j, 1, "group");
    return jass_pushhandle(j, 0, "unit");
}
DWORD CreateForce(LPJASS j) {
    return jass_pushhandle(j, 0, "force");
}
DWORD ForceAddPlayer(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return 0;
}
DWORD ForceRemovePlayer(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return 0;
}
DWORD ForceClear(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    return 0;
}
DWORD ForceEnumPlayers(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE filter = jass_checkhandle(j, 2, "boolexpr");
    return 0;
}
DWORD ForceEnumPlayersCounted(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE filter = jass_checkhandle(j, 2, "boolexpr");
    //LONG countLimit = jass_checkinteger(j, 3);
    return 0;
}
DWORD ForceEnumAllies(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD ForceEnumEnemies(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD ForForce(LPJASS j) {
    //HANDLE whichForce = jass_checkhandle(j, 1, "force");
    //LPCSTR callback = jass_checkcode(j, 2);
    return 0;
}
DWORD Rect(LPJASS j) {
    //FLOAT minx = jass_checknumber(j, 1);
    //FLOAT miny = jass_checknumber(j, 2);
    //FLOAT maxx = jass_checknumber(j, 3);
    //FLOAT maxy = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "rect");
}
DWORD RectFromLoc(LPJASS j) {
    //HANDLE min = jass_checkhandle(j, 1, "location");
    //HANDLE max = jass_checkhandle(j, 2, "location");
    return jass_pushhandle(j, 0, "rect");
}
DWORD RemoveRect(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return 0;
}
DWORD SetRect(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    //FLOAT minx = jass_checknumber(j, 2);
    //FLOAT miny = jass_checknumber(j, 3);
    //FLOAT maxx = jass_checknumber(j, 4);
    //FLOAT maxy = jass_checknumber(j, 5);
    return 0;
}
DWORD SetRectFromLoc(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    //HANDLE min = jass_checkhandle(j, 2, "location");
    //HANDLE max = jass_checkhandle(j, 3, "location");
    return 0;
}
DWORD MoveRectTo(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    //FLOAT newCenterX = jass_checknumber(j, 2);
    //FLOAT newCenterY = jass_checknumber(j, 3);
    return 0;
}
DWORD MoveRectToLoc(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    //HANDLE newCenterLoc = jass_checkhandle(j, 2, "location");
    return 0;
}
DWORD GetRectCenterX(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, 0);
}
DWORD GetRectCenterY(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, 0);
}
DWORD GetRectMinX(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, 0);
}
DWORD GetRectMinY(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, 0);
}
DWORD GetRectMaxX(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, 0);
}
DWORD GetRectMaxY(LPJASS j) {
    //HANDLE whichRect = jass_checkhandle(j, 1, "rect");
    return jass_pushnumber(j, 0);
}
DWORD CreateRegion(LPJASS j) {
    return jass_pushhandle(j, 0, "region");
}
DWORD RemoveRegion(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    return 0;
}
DWORD RegionAddRect(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE r = jass_checkhandle(j, 2, "rect");
    return 0;
}
DWORD RegionClearRect(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE r = jass_checkhandle(j, 2, "rect");
    return 0;
}
DWORD RegionAddCell(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD RegionAddCellAtLoc(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    return 0;
}
DWORD RegionClearCell(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD RegionClearCellAtLoc(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    return 0;
}
DWORD Location(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    return jass_pushhandle(j, 0, "location");
}
DWORD RemoveLocation(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    return 0;
}
DWORD MoveLocation(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    //FLOAT newX = jass_checknumber(j, 2);
    //FLOAT newY = jass_checknumber(j, 3);
    return 0;
}
DWORD GetLocationX(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    return jass_pushnumber(j, 0);
}
DWORD GetLocationY(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    return jass_pushnumber(j, 0);
}
DWORD IsUnitInRegion(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsPointInRegion(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD IsLocationInRegion(LPJASS j) {
    //HANDLE whichRegion = jass_checkhandle(j, 1, "region");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    return jass_pushboolean(j, 0);
}
DWORD GetWorldBounds(LPJASS j) {
    return jass_pushhandle(j, 0, "rect");
}
DWORD CreateTrigger(LPJASS j) {
    return jass_pushhandle(j, 0, "trigger");
}
DWORD DestroyTrigger(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD ResetTrigger(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD EnableTrigger(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD DisableTrigger(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD IsTriggerEnabled(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushboolean(j, 0);
}
DWORD TriggerWaitOnSleeps(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsTriggerWaitOnSleeps(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushboolean(j, 0);
}
DWORD GetFilterUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetEnumUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetFilterDestructable(LPJASS j) {
    return jass_pushhandle(j, 0, "destructable");
}
DWORD GetEnumDestructable(LPJASS j) {
    return jass_pushhandle(j, 0, "destructable");
}
DWORD GetFilterPlayer(LPJASS j) {
    return jass_pushhandle(j, 0, "player");
}
DWORD GetEnumPlayer(LPJASS j) {
    return jass_pushhandle(j, 0, "player");
}
DWORD GetTriggeringTrigger(LPJASS j) {
    return jass_pushhandle(j, 0, "trigger");
}
DWORD GetTriggerEventId(LPJASS j) {
    return jass_pushhandle(j, 0, "eventid");
}
DWORD GetTriggerEvalCount(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushinteger(j, 0);
}
DWORD GetTriggerExecCount(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushinteger(j, 0);
}
DWORD ExecuteFunc(LPJASS j) {
    //LPCSTR funcName = jass_checkstring(j, 1);
    return 0;
}
DWORD And(LPJASS j) {
    //HANDLE operandA = jass_checkhandle(j, 1, "boolexpr");
    //HANDLE operandB = jass_checkhandle(j, 2, "boolexpr");
    return jass_pushhandle(j, 0, "boolexpr");
}
DWORD Or(LPJASS j) {
    //HANDLE operandA = jass_checkhandle(j, 1, "boolexpr");
    //HANDLE operandB = jass_checkhandle(j, 2, "boolexpr");
    return jass_pushhandle(j, 0, "boolexpr");
}
DWORD Not(LPJASS j) {
    //HANDLE operand = jass_checkhandle(j, 1, "boolexpr");
    return jass_pushhandle(j, 0, "boolexpr");
}
DWORD Condition(LPJASS j) {
    //LPCSTR func = jass_checkcode(j, 1);
    return jass_pushhandle(j, 0, "conditionfunc");
}
DWORD DestroyCondition(LPJASS j) {
    //HANDLE c = jass_checkhandle(j, 1, "conditionfunc");
    return 0;
}
DWORD Filter(LPJASS j) {
    //LPCSTR func = jass_checkcode(j, 1);
    return jass_pushhandle(j, 0, "filterfunc");
}
DWORD DestroyFilter(LPJASS j) {
    //HANDLE f = jass_checkhandle(j, 1, "filterfunc");
    return 0;
}
DWORD DestroyBoolExpr(LPJASS j) {
    //HANDLE e = jass_checkhandle(j, 1, "boolexpr");
    return 0;
}
DWORD TriggerRegisterVariableEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPCSTR varName = jass_checkstring(j, 2);
    //HANDLE opcode = jass_checkhandle(j, 3, "limitop");
    //FLOAT limitval = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterTimerEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //FLOAT timeout = jass_checknumber(j, 2);
    //BOOL periodic = jass_checkboolean(j, 3);
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterTimerExpireEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "timer");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterGameStateEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichState = jass_checkhandle(j, 2, "gamestate");
    //HANDLE opcode = jass_checkhandle(j, 3, "limitop");
    //FLOAT limitval = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterDialogEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichDialog = jass_checkhandle(j, 2, "dialog");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterDialogButtonEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichButton = jass_checkhandle(j, 2, "button");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetEventGameState(LPJASS j) {
    return jass_pushhandle(j, 0, "gamestate");
}
DWORD TriggerRegisterGameEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichGameEvent = jass_checkhandle(j, 2, "gameevent");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetWinningPlayer(LPJASS j) {
    return jass_pushhandle(j, 0, "player");
}
DWORD TriggerRegisterEnterRegion(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichRegion = jass_checkhandle(j, 2, "region");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetTriggeringRegion(LPJASS j) {
    return jass_pushhandle(j, 0, "region");
}
DWORD GetEnteringUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD TriggerRegisterLeaveRegion(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichRegion = jass_checkhandle(j, 2, "region");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetLeavingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD TriggerRegisterTrackableHitEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "trackable");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterTrackableTrackEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "trackable");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetTriggeringTrackable(LPJASS j) {
    return jass_pushhandle(j, 0, "trackable");
}
DWORD GetClickedButton(LPJASS j) {
    return jass_pushhandle(j, 0, "button");
}
DWORD GetClickedDialog(LPJASS j) {
    return jass_pushhandle(j, 0, "dialog");
}
DWORD TriggerRegisterPlayerEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichPlayerEvent = jass_checkhandle(j, 3, "playerevent");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetTriggerPlayer(LPJASS j) {
    return jass_pushhandle(j, 0, "player");
}
DWORD TriggerRegisterPlayerUnitEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichPlayerUnitEvent = jass_checkhandle(j, 3, "playerunitevent");
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetLevelingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetLearningUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetLearnedSkill(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetLearnedSkillLevel(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetRevivableUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetRevivingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetAttacker(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetRescuer(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetDyingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetKillingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetDecayingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetConstructingStructure(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetCancelledStructure(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetConstructedStructure(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetResearchingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetResearched(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetTrainedUnitType(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetTrainedUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetDetectedUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetSummoningUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetSummonedUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetTransportUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetLoadedUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetManipulatingUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetManipulatedItem(LPJASS j) {
    return jass_pushhandle(j, 0, "item");
}
DWORD GetOrderedUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetIssuedOrderId(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetOrderPointX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetOrderPointY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetOrderPointLoc(LPJASS j) {
    return jass_pushhandle(j, 0, "location");
}
DWORD GetOrderTarget(LPJASS j) {
    return jass_pushhandle(j, 0, "widget");
}
DWORD GetOrderTargetDestructable(LPJASS j) {
    return jass_pushhandle(j, 0, "destructable");
}
DWORD GetOrderTargetItem(LPJASS j) {
    return jass_pushhandle(j, 0, "item");
}
DWORD GetOrderTargetUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD TriggerRegisterPlayerAllianceChange(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichAlliance = jass_checkhandle(j, 3, "alliancetype");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterPlayerStateEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichState = jass_checkhandle(j, 3, "playerstate");
    //HANDLE opcode = jass_checkhandle(j, 4, "limitop");
    //FLOAT limitval = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "event");
}
DWORD GetEventPlayerState(LPJASS j) {
    return jass_pushhandle(j, 0, "playerstate");
}
DWORD TriggerRegisterPlayerChatEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //LPCSTR chatMessageToDetect = jass_checkstring(j, 3);
    //BOOL exactMatchOnly = jass_checkboolean(j, 4);
    return jass_pushhandle(j, 0, "event");
}
DWORD GetEventPlayerChatString(LPJASS j) {
    return jass_pushstring(j, 0);
}
DWORD GetEventPlayerChatStringMatched(LPJASS j) {
    return jass_pushstring(j, 0);
}
DWORD TriggerRegisterDeathEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichWidget = jass_checkhandle(j, 2, "widget");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetTriggerUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD TriggerRegisterUnitStateEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //HANDLE whichState = jass_checkhandle(j, 3, "unitstate");
    //HANDLE opcode = jass_checkhandle(j, 4, "limitop");
    //FLOAT limitval = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "event");
}
DWORD GetEventUnitState(LPJASS j) {
    return jass_pushhandle(j, 0, "unitstate");
}
DWORD TriggerRegisterUnitEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //HANDLE whichEvent = jass_checkhandle(j, 3, "unitevent");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetEventDamage(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetEventDetectingPlayer(LPJASS j) {
    return jass_pushhandle(j, 0, "player");
}
DWORD TriggerRegisterFilterUnitEvent(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //HANDLE whichEvent = jass_checkhandle(j, 3, "unitevent");
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    return jass_pushhandle(j, 0, "event");
}
DWORD GetEventTargetUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD TriggerRegisterUnitInRange(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //FLOAT range = jass_checknumber(j, 3);
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerAddCondition(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE condition = jass_checkhandle(j, 2, "boolexpr");
    return jass_pushhandle(j, 0, "triggercondition");
}
DWORD TriggerRemoveCondition(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichCondition = jass_checkhandle(j, 2, "triggercondition");
    return 0;
}
DWORD TriggerClearConditions(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD TriggerAddAction(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPCSTR actionFunc = jass_checkcode(j, 2);
    return jass_pushhandle(j, 0, "triggeraction");
}
DWORD TriggerRemoveAction(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichAction = jass_checkhandle(j, 2, "triggeraction");
    return 0;
}
DWORD TriggerClearActions(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD TriggerSleepAction(LPJASS j) {
    //FLOAT timeout = jass_checknumber(j, 1);
    return 0;
}
DWORD TriggerWaitForSound(LPJASS j) {
    //HANDLE s = jass_checkhandle(j, 1, "sound");
    //FLOAT offset = jass_checknumber(j, 2);
    return 0;
}
DWORD TriggerEvaluate(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushboolean(j, 0);
}
DWORD TriggerExecute(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD TriggerExecuteWait(LPJASS j) {
    //HANDLE whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD GetWidgetLife(LPJASS j) {
    //HANDLE whichWidget = jass_checkhandle(j, 1, "widget");
    return jass_pushnumber(j, 0);
}
DWORD SetWidgetLife(LPJASS j) {
    //HANDLE whichWidget = jass_checkhandle(j, 1, "widget");
    //FLOAT newLife = jass_checknumber(j, 2);
    return 0;
}
DWORD GetWidgetX(LPJASS j) {
    //HANDLE whichWidget = jass_checkhandle(j, 1, "widget");
    return jass_pushnumber(j, 0);
}
DWORD GetWidgetY(LPJASS j) {
    //HANDLE whichWidget = jass_checkhandle(j, 1, "widget");
    return jass_pushnumber(j, 0);
}
DWORD GetTriggerWidget(LPJASS j) {
    return jass_pushhandle(j, 0, "widget");
}
DWORD CreateDestructable(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT face = jass_checknumber(j, 4);
    //FLOAT scale = jass_checknumber(j, 5);
    //LONG variation = jass_checkinteger(j, 6);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD CreateDestructableZ(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT z = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    //FLOAT scale = jass_checknumber(j, 6);
    //LONG variation = jass_checkinteger(j, 7);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD CreateDeadDestructable(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT face = jass_checknumber(j, 4);
    //FLOAT scale = jass_checknumber(j, 5);
    //LONG variation = jass_checkinteger(j, 6);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD CreateDeadDestructableZ(LPJASS j) {
    //LONG objectid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT z = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    //FLOAT scale = jass_checknumber(j, 6);
    //LONG variation = jass_checkinteger(j, 7);
    return jass_pushhandle(j, 0, "destructable");
}
DWORD RemoveDestructable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return 0;
}
DWORD KillDestructable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return 0;
}
DWORD SetDestructableInvulnerable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsDestructableInvulnerable(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushboolean(j, 0);
}
DWORD EnumDestructablesInRect(LPJASS j) {
    //HANDLE r = jass_checkhandle(j, 1, "rect");
    //HANDLE filter = jass_checkhandle(j, 2, "boolexpr");
    //LPCSTR actionFunc = jass_checkcode(j, 3);
    return 0;
}
DWORD GetDestructableTypeId(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushinteger(j, 0);
}
DWORD GetDestructableX(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD GetDestructableY(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD SetDestructableLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //FLOAT life = jass_checknumber(j, 2);
    return 0;
}
DWORD GetDestructableLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD SetDestructableMaxLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //FLOAT max = jass_checknumber(j, 2);
    return 0;
}
DWORD GetDestructableMaxLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    return jass_pushnumber(j, 0);
}
DWORD DestructableRestoreLife(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //FLOAT life = jass_checknumber(j, 2);
    //BOOL birth = jass_checkboolean(j, 3);
    return 0;
}
DWORD QueueDestructableAnimation(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    return 0;
}
DWORD SetDestructableAnimation(LPJASS j) {
    //HANDLE d = jass_checkhandle(j, 1, "destructable");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    return 0;
}
DWORD CreateItem(LPJASS j) {
    //LONG itemid = jass_checkinteger(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return jass_pushhandle(j, 0, "item");
}
DWORD RemoveItem(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    return 0;
}
DWORD GetItemPlayer(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    return jass_pushhandle(j, 0, "player");
}
DWORD GetItemTypeId(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    return jass_pushinteger(j, 0);
}
DWORD GetItemX(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    return jass_pushnumber(j, 0);
}
DWORD GetItemY(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    return jass_pushnumber(j, 0);
}
DWORD SetItemPosition(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD SetItemDropOnDeath(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetItemDroppable(LPJASS j) {
    //HANDLE i = jass_checkhandle(j, 1, "item");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetItemPlayer(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //BOOL changeColor = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetItemInvulnerable(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsItemInvulnerable(LPJASS j) {
    //HANDLE whichItem = jass_checkhandle(j, 1, "item");
    return jass_pushboolean(j, 0);
}
DWORD CreateUnit(LPJASS j) {
    //HANDLE id = jass_checkhandle(j, 1, "player");
    //LONG unitid = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "unit");
}
DWORD CreateUnitByName(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "unit");
}
DWORD CreateUnitAtLoc(LPJASS j) {
    //HANDLE id = jass_checkhandle(j, 1, "player");
    //LONG unitid = jass_checkinteger(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    //FLOAT face = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "unit");
}
DWORD CreateUnitAtLocByName(LPJASS j) {
    //HANDLE id = jass_checkhandle(j, 1, "player");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    //FLOAT face = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "unit");
}
DWORD CreateCorpse(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG unitid = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "unit");
}
DWORD KillUnit(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD RemoveUnit(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD ShowUnit(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL show = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetUnitState(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichUnitState = jass_checkhandle(j, 2, "unitstate");
    //FLOAT newVal = jass_checknumber(j, 3);
    return 0;
}
DWORD SetUnitX(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newX = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitY(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newY = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitPosition(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newX = jass_checknumber(j, 2);
    //FLOAT newY = jass_checknumber(j, 3);
    return 0;
}
DWORD SetUnitPositionLoc(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    return 0;
}
DWORD SetUnitFacing(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT facingAngle = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitFacingTimed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT facingAngle = jass_checknumber(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD SetUnitMoveSpeed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newSpeed = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitFlyHeight(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newHeight = jass_checknumber(j, 2);
    //FLOAT rate = jass_checknumber(j, 3);
    return 0;
}
DWORD SetUnitTurnSpeed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newTurnSpeed = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitPropWindow(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newPropWindowAngle = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitAcquireRange(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT newAcquireRange = jass_checknumber(j, 2);
    return 0;
}
DWORD GetUnitAcquireRange(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitTurnSpeed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitPropWindow(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitFlyHeight(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultAcquireRange(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultTurnSpeed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultPropWindow(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultFlyHeight(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD SetUnitOwner(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //BOOL changeColor = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetUnitColor(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichColor = jass_checkhandle(j, 2, "playercolor");
    return 0;
}
DWORD SetUnitScale(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT scaleX = jass_checknumber(j, 2);
    //FLOAT scaleY = jass_checknumber(j, 3);
    //FLOAT scaleZ = jass_checknumber(j, 4);
    return 0;
}
DWORD SetUnitTimeScale(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT timeScale = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitBlendTime(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT blendTime = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitVertexColor(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD QueueUnitAnimation(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    return 0;
}
DWORD SetUnitAnimation(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    return 0;
}
DWORD SetUnitAnimationByIndex(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG whichAnimation = jass_checkinteger(j, 2);
    return 0;
}
DWORD SetUnitAnimationWithRarity(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    //HANDLE rarity = jass_checkhandle(j, 3, "raritycontrol");
    return 0;
}
DWORD AddUnitAnimationProperties(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR animProperties = jass_checkstring(j, 2);
    //BOOL add = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetUnitLookAt(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR whichBone = jass_checkstring(j, 2);
    //HANDLE lookAtTarget = jass_checkhandle(j, 3, "unit");
    //FLOAT offsetX = jass_checknumber(j, 4);
    //FLOAT offsetY = jass_checknumber(j, 5);
    //FLOAT offsetZ = jass_checknumber(j, 6);
    return 0;
}
DWORD ResetUnitLookAt(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD SetUnitRescuable(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE byWhichPlayer = jass_checkhandle(j, 2, "player");
    //BOOL flag = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetUnitRescueRange(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT range = jass_checknumber(j, 2);
    return 0;
}
DWORD SetHeroStr(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newStr = jass_checkinteger(j, 2);
    //BOOL permanent = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetHeroAgi(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newAgi = jass_checkinteger(j, 2);
    //BOOL permanent = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetHeroInt(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newInt = jass_checkinteger(j, 2);
    //BOOL permanent = jass_checkboolean(j, 3);
    return 0;
}
DWORD GetHeroXP(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD SetHeroXP(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newXpVal = jass_checkinteger(j, 2);
    //BOOL showEyeCandy = jass_checkboolean(j, 3);
    return 0;
}
DWORD AddHeroXP(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //LONG xpToAdd = jass_checkinteger(j, 2);
    //BOOL showEyeCandy = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetHeroLevel(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //LONG level = jass_checkinteger(j, 2);
    //BOOL showEyeCandy = jass_checkboolean(j, 3);
    return 0;
}
DWORD GetHeroLevel(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD SuspendHeroXP(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsSuspendedXP(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD SelectHeroSkill(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //LONG abilcode = jass_checkinteger(j, 2);
    return 0;
}
DWORD ReviveHero(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //BOOL doEyecandy = jass_checkboolean(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD ReviveHeroLoc(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    //HANDLE loc = jass_checkhandle(j, 2, "location");
    //BOOL doEyecandy = jass_checkboolean(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD SetUnitExploded(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL exploded = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetUnitInvulnerable(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD PauseUnit(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsUnitPaused(LPJASS j) {
    //HANDLE whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD SetUnitPathing(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD ClearSelection(LPJASS j) {
    return 0;
}
DWORD SelectUnit(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD GetUnitPointValue(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD GetUnitPointValueByType(LPJASS j) {
    //LONG unitType = jass_checkinteger(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD UnitAddItem(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    return jass_pushboolean(j, 0);
}
DWORD UnitAddItemById(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG itemId = jass_checkinteger(j, 2);
    return jass_pushhandle(j, 0, "item");
}
DWORD UnitAddItemToSlotById(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG itemId = jass_checkinteger(j, 2);
    //LONG itemSlot = jass_checkinteger(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD UnitRemoveItem(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    return 0;
}
DWORD UnitRemoveItemFromSlot(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG itemSlot = jass_checkinteger(j, 2);
    return jass_pushhandle(j, 0, "item");
}
DWORD UnitHasItem(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    return jass_pushboolean(j, 0);
}
DWORD UnitItemInSlot(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG itemSlot = jass_checkinteger(j, 2);
    return jass_pushhandle(j, 0, "item");
}
DWORD UnitUseItem(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    return jass_pushboolean(j, 0);
}
DWORD UnitUseItemPoint(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD UnitUseItemTarget(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    //HANDLE target = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD GetUnitX(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitY(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitLoc(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushhandle(j, 0, "location");
}
DWORD GetUnitFacing(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitMoveSpeed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultMoveSpeed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitState(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichUnitState = jass_checkhandle(j, 2, "unitstate");
    return jass_pushnumber(j, 0);
}
DWORD GetOwningPlayer(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushhandle(j, 0, "player");
}
DWORD GetUnitTypeId(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD GetUnitRace(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushhandle(j, 0, "race");
}
DWORD GetUnitName(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushstring(j, 0);
}
DWORD GetUnitFoodUsed(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD GetUnitFoodMade(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD GetFoodMade(LPJASS j) {
    //LONG unitId = jass_checkinteger(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD IsUnitInGroup(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichGroup = jass_checkhandle(j, 2, "group");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInForce(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichForce = jass_checkhandle(j, 2, "force");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitOwnedByPlayer(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitAlly(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitEnemy(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitVisible(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitDetected(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInvisible(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitFogged(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitMasked(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitSelected(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitRace(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichRace = jass_checkhandle(j, 2, "race");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitType(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichUnitType = jass_checkhandle(j, 2, "unittype");
    return jass_pushboolean(j, 0);
}
DWORD IsUnit(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichSpecifiedUnit = jass_checkhandle(j, 2, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInRange(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE otherUnit = jass_checkhandle(j, 2, "unit");
    //FLOAT distance = jass_checknumber(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInRangeXY(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT distance = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInRangeLoc(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    //FLOAT distance = jass_checknumber(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD IsUnitHidden(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitIllusion(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInTransport(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichTransport = jass_checkhandle(j, 2, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitLoaded(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsHeroUnitId(LPJASS j) {
    //LONG unitId = jass_checkinteger(j, 1);
    return jass_pushboolean(j, 0);
}
DWORD UnitShareVision(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //BOOL share = jass_checkboolean(j, 3);
    return 0;
}
DWORD UnitSuspendDecay(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL suspend = jass_checkboolean(j, 2);
    return 0;
}
DWORD UnitRemoveAbility(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG abilityId = jass_checkinteger(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD UnitRemoveBuffs(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL removePositive = jass_checkboolean(j, 2);
    //BOOL removeNegative = jass_checkboolean(j, 3);
    return 0;
}
DWORD UnitAddSleep(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL add = jass_checkboolean(j, 2);
    return 0;
}
DWORD UnitCanSleep(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD UnitAddSleepPerm(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL add = jass_checkboolean(j, 2);
    return 0;
}
DWORD UnitCanSleepPerm(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD UnitIsSleeping(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD UnitWakeUp(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD UnitApplyTimedLife(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG buffId = jass_checkinteger(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD IssueImmediateOrder(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR order = jass_checkstring(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD IssueImmediateOrderById(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD IssuePointOrder(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR order = jass_checkstring(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD IssuePointOrderLoc(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    return jass_pushboolean(j, 0);
}
DWORD IssuePointOrderById(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD IssuePointOrderByIdLoc(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    return jass_pushboolean(j, 0);
}
DWORD IssueTargetOrder(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD IssueTargetOrderById(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD IssueInstantTargetOrder(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    //HANDLE instantTargetWidget = jass_checkhandle(j, 4, "widget");
    return jass_pushboolean(j, 0);
}
DWORD IssueInstantTargetOrderById(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    //HANDLE instantTargetWidget = jass_checkhandle(j, 4, "widget");
    return jass_pushboolean(j, 0);
}
DWORD IssueBuildOrder(LPJASS j) {
    //HANDLE whichPeon = jass_checkhandle(j, 1, "unit");
    //LPCSTR unitToBuild = jass_checkstring(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD IssueBuildOrderById(LPJASS j) {
    //HANDLE whichPeon = jass_checkhandle(j, 1, "unit");
    //LONG unitId = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD IssueNeutralImmediateOrder(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE neutralStructure = jass_checkhandle(j, 2, "unit");
    //LPCSTR unitToBuild = jass_checkstring(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD IssueNeutralImmediateOrderById(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE neutralStructure = jass_checkhandle(j, 2, "unit");
    //LONG unitId = jass_checkinteger(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD IssueNeutralPointOrder(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE neutralStructure = jass_checkhandle(j, 2, "unit");
    //LPCSTR unitToBuild = jass_checkstring(j, 3);
    //FLOAT x = jass_checknumber(j, 4);
    //FLOAT y = jass_checknumber(j, 5);
    return jass_pushboolean(j, 0);
}
DWORD IssueNeutralPointOrderById(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE neutralStructure = jass_checkhandle(j, 2, "unit");
    //LONG unitId = jass_checkinteger(j, 3);
    //FLOAT x = jass_checknumber(j, 4);
    //FLOAT y = jass_checknumber(j, 5);
    return jass_pushboolean(j, 0);
}
DWORD IssueNeutralTargetOrder(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE neutralStructure = jass_checkhandle(j, 2, "unit");
    //LPCSTR unitToBuild = jass_checkstring(j, 3);
    //HANDLE target = jass_checkhandle(j, 4, "widget");
    return jass_pushboolean(j, 0);
}
DWORD IssueNeutralTargetOrderById(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE neutralStructure = jass_checkhandle(j, 2, "unit");
    //LONG unitId = jass_checkinteger(j, 3);
    //HANDLE target = jass_checkhandle(j, 4, "widget");
    return jass_pushboolean(j, 0);
}
DWORD SetResourceAmount(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG amount = jass_checkinteger(j, 2);
    return 0;
}
DWORD AddResourceAmount(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG amount = jass_checkinteger(j, 2);
    return 0;
}
DWORD GetResourceAmount(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD WaygateGetDestinationX(LPJASS j) {
    //HANDLE waygate = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD WaygateGetDestinationY(LPJASS j) {
    //HANDLE waygate = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD WaygateSetDestination(LPJASS j) {
    //HANDLE waygate = jass_checkhandle(j, 1, "unit");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return 0;
}
DWORD WaygateActivate(LPJASS j) {
    //HANDLE waygate = jass_checkhandle(j, 1, "unit");
    //BOOL activate = jass_checkboolean(j, 2);
    return 0;
}
DWORD WaygateIsActive(LPJASS j) {
    //HANDLE waygate = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD Player(LPJASS j) {
    //LONG number = jass_checkinteger(j, 1);
    return jass_pushhandle(j, 0, "player");
}
DWORD GetLocalPlayer(LPJASS j) {
    return jass_pushhandle(j, 0, "player");
}
DWORD IsPlayerAlly(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE otherPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsPlayerEnemy(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE otherPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsPlayerInForce(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichForce = jass_checkhandle(j, 2, "force");
    return jass_pushboolean(j, 0);
}
DWORD IsPlayerObserver(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsVisibleToPlayer(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //HANDLE whichPlayer = jass_checkhandle(j, 3, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsLocationVisibleToPlayer(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsFoggedToPlayer(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //HANDLE whichPlayer = jass_checkhandle(j, 3, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsLocationFoggedToPlayer(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsMaskedToPlayer(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //HANDLE whichPlayer = jass_checkhandle(j, 3, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsLocationMaskedToPlayer(LPJASS j) {
    //HANDLE whichLocation = jass_checkhandle(j, 1, "location");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD GetPlayerRace(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushhandle(j, 0, "race");
}
DWORD GetPlayerId(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushinteger(j, 0);
}
DWORD GetPlayerUnitCount(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //BOOL includeIncomplete = jass_checkboolean(j, 2);
    return jass_pushinteger(j, 0);
}
DWORD GetPlayerTypedUnitCount(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LPCSTR unitName = jass_checkstring(j, 2);
    //BOOL includeIncomplete = jass_checkboolean(j, 3);
    //BOOL includeUpgrades = jass_checkboolean(j, 4);
    return jass_pushinteger(j, 0);
}
DWORD GetPlayerStructureCount(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //BOOL includeIncomplete = jass_checkboolean(j, 2);
    return jass_pushinteger(j, 0);
}
DWORD GetPlayerState(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichPlayerState = jass_checkhandle(j, 2, "playerstate");
    return jass_pushinteger(j, 0);
}
DWORD GetPlayerAlliance(LPJASS j) {
    //HANDLE sourcePlayer = jass_checkhandle(j, 1, "player");
    //HANDLE otherPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichAllianceSetting = jass_checkhandle(j, 3, "alliancetype");
    return jass_pushboolean(j, 0);
}
DWORD GetPlayerHandicap(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushnumber(j, 0);
}
DWORD GetPlayerHandicapXP(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushnumber(j, 0);
}
DWORD SetPlayerHandicap(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //FLOAT handicap = jass_checknumber(j, 2);
    return 0;
}
DWORD SetPlayerHandicapXP(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //FLOAT handicap = jass_checknumber(j, 2);
    return 0;
}
DWORD SetPlayerTechMaxAllowed(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG techid = jass_checkinteger(j, 2);
    //LONG maximum = jass_checkinteger(j, 3);
    return 0;
}
DWORD GetPlayerTechMaxAllowed(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG techid = jass_checkinteger(j, 2);
    return jass_pushinteger(j, 0);
}
DWORD AddPlayerTechResearched(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG techid = jass_checkinteger(j, 2);
    //LONG levels = jass_checkinteger(j, 3);
    return 0;
}
DWORD SetPlayerTechResearched(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG techid = jass_checkinteger(j, 2);
    //LONG setToLevel = jass_checkinteger(j, 3);
    return 0;
}
DWORD GetPlayerTechResearched(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG techid = jass_checkinteger(j, 2);
    //BOOL specificonly = jass_checkboolean(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD GetPlayerTechCount(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG techid = jass_checkinteger(j, 2);
    //BOOL specificonly = jass_checkboolean(j, 3);
    return jass_pushinteger(j, 0);
}
DWORD SetPlayerAbilityAvailable(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG abilid = jass_checkinteger(j, 2);
    //BOOL avail = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetPlayerState(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichPlayerState = jass_checkhandle(j, 2, "playerstate");
    //LONG value = jass_checkinteger(j, 3);
    return 0;
}
DWORD RemovePlayer(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE gameResult = jass_checkhandle(j, 2, "playergameresult");
    return 0;
}
DWORD CachePlayerHeroData(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    return 0;
}
DWORD SetFogStateRect(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichState = jass_checkhandle(j, 2, "fogstate");
    //HANDLE where = jass_checkhandle(j, 3, "rect");
    //BOOL useSharedVision = jass_checkboolean(j, 4);
    return 0;
}
DWORD SetFogStateRadius(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichState = jass_checkhandle(j, 2, "fogstate");
    //FLOAT centerx = jass_checknumber(j, 3);
    //FLOAT centerY = jass_checknumber(j, 4);
    //FLOAT radius = jass_checknumber(j, 5);
    //BOOL useSharedVision = jass_checkboolean(j, 6);
    return 0;
}
DWORD SetFogStateRadiusLoc(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichState = jass_checkhandle(j, 2, "fogstate");
    //HANDLE center = jass_checkhandle(j, 3, "location");
    //FLOAT radius = jass_checknumber(j, 4);
    //BOOL useSharedVision = jass_checkboolean(j, 5);
    return 0;
}
DWORD FogMaskEnable(LPJASS j) {
    //BOOL enable = jass_checkboolean(j, 1);
    return 0;
}
DWORD IsFogMaskEnabled(LPJASS j) {
    return jass_pushboolean(j, 0);
}
DWORD FogEnable(LPJASS j) {
    //BOOL enable = jass_checkboolean(j, 1);
    return 0;
}
DWORD IsFogEnabled(LPJASS j) {
    return jass_pushboolean(j, 0);
}
DWORD CreateFogModifierRect(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichState = jass_checkhandle(j, 2, "fogstate");
    //HANDLE where = jass_checkhandle(j, 3, "rect");
    //BOOL useSharedVision = jass_checkboolean(j, 4);
    //BOOL afterUnits = jass_checkboolean(j, 5);
    return jass_pushhandle(j, 0, "fogmodifier");
}
DWORD CreateFogModifierRadius(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichState = jass_checkhandle(j, 2, "fogstate");
    //FLOAT centerx = jass_checknumber(j, 3);
    //FLOAT centerY = jass_checknumber(j, 4);
    //FLOAT radius = jass_checknumber(j, 5);
    //BOOL useSharedVision = jass_checkboolean(j, 6);
    //BOOL afterUnits = jass_checkboolean(j, 7);
    return jass_pushhandle(j, 0, "fogmodifier");
}
DWORD CreateFogModifierRadiusLoc(LPJASS j) {
    //HANDLE forWhichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichState = jass_checkhandle(j, 2, "fogstate");
    //HANDLE center = jass_checkhandle(j, 3, "location");
    //FLOAT radius = jass_checknumber(j, 4);
    //BOOL useSharedVision = jass_checkboolean(j, 5);
    //BOOL afterUnits = jass_checkboolean(j, 6);
    return jass_pushhandle(j, 0, "fogmodifier");
}
DWORD DestroyFogModifier(LPJASS j) {
    //HANDLE whichFogModifier = jass_checkhandle(j, 1, "fogmodifier");
    return 0;
}
DWORD FogModifierStart(LPJASS j) {
    //HANDLE whichFogModifier = jass_checkhandle(j, 1, "fogmodifier");
    return 0;
}
DWORD FogModifierStop(LPJASS j) {
    //HANDLE whichFogModifier = jass_checkhandle(j, 1, "fogmodifier");
    return 0;
}
DWORD EndGame(LPJASS j) {
    //BOOL doScoreScreen = jass_checkboolean(j, 1);
    return 0;
}
DWORD ChangeLevel(LPJASS j) {
    //LPCSTR newLevel = jass_checkstring(j, 1);
    //BOOL doScoreScreen = jass_checkboolean(j, 2);
    return 0;
}
DWORD RestartGame(LPJASS j) {
    //BOOL doScoreScreen = jass_checkboolean(j, 1);
    return 0;
}
DWORD ReloadGame(LPJASS j) {
    return 0;
}
DWORD SetCampaignMenuRace(LPJASS j) {
    //HANDLE r = jass_checkhandle(j, 1, "race");
    return 0;
}
DWORD ForceCampaignSelectScreen(LPJASS j) {
    return 0;
}
DWORD SyncSelections(LPJASS j) {
    return 0;
}
DWORD SetFloatGameState(LPJASS j) {
    //HANDLE whichFloatGameState = jass_checkhandle(j, 1, "fgamestate");
    //FLOAT value = jass_checknumber(j, 2);
    return 0;
}
DWORD GetFloatGameState(LPJASS j) {
    //HANDLE whichFloatGameState = jass_checkhandle(j, 1, "fgamestate");
    return jass_pushnumber(j, 0);
}
DWORD SetIntegerGameState(LPJASS j) {
    //HANDLE whichIntegerGameState = jass_checkhandle(j, 1, "igamestate");
    //LONG value = jass_checkinteger(j, 2);
    return 0;
}
DWORD GetIntegerGameState(LPJASS j) {
    //HANDLE whichIntegerGameState = jass_checkhandle(j, 1, "igamestate");
    return jass_pushinteger(j, 0);
}
DWORD SetTutorialCleared(LPJASS j) {
    //BOOL cleared = jass_checkboolean(j, 1);
    return 0;
}
DWORD SetMissionAvailable(LPJASS j) {
    //LONG campaignNumber = jass_checkinteger(j, 1);
    //LONG missionNumber = jass_checkinteger(j, 2);
    //BOOL available = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetCampaignAvailable(LPJASS j) {
    //LONG campaignNumber = jass_checkinteger(j, 1);
    //BOOL available = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetOpCinematicAvailable(LPJASS j) {
    //LONG campaignNumber = jass_checkinteger(j, 1);
    //BOOL available = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetEdCinematicAvailable(LPJASS j) {
    //LONG campaignNumber = jass_checkinteger(j, 1);
    //BOOL available = jass_checkboolean(j, 2);
    return 0;
}
DWORD GetDefaultDifficulty(LPJASS j) {
    return jass_pushhandle(j, 0, "gamedifficulty");
}
DWORD SetDefaultDifficulty(LPJASS j) {
    //HANDLE g = jass_checkhandle(j, 1, "gamedifficulty");
    return 0;
}
DWORD DialogCreate(LPJASS j) {
    return jass_pushhandle(j, 0, "dialog");
}
DWORD DialogDestroy(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "dialog");
    return 0;
}
DWORD DialogSetAsync(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "dialog");
    return 0;
}
DWORD DialogClear(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "dialog");
    return 0;
}
DWORD DialogSetMessage(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "dialog");
    //LPCSTR messageText = jass_checkstring(j, 2);
    return 0;
}
DWORD DialogAddButton(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "dialog");
    //LPCSTR buttonText = jass_checkstring(j, 2);
    //LONG hotkey = jass_checkinteger(j, 3);
    return jass_pushhandle(j, 0, "button");
}
DWORD DialogDisplay(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichDialog = jass_checkhandle(j, 2, "dialog");
    //BOOL flag = jass_checkboolean(j, 3);
    return 0;
}
DWORD InitGameCache(LPJASS j) {
    //LPCSTR campaignFile = jass_checkstring(j, 1);
    return jass_pushhandle(j, 0, "gamecache");
}
DWORD SaveGameCache(LPJASS j) {
    //HANDLE whichCache = jass_checkhandle(j, 1, "gamecache");
    return jass_pushboolean(j, 0);
}
DWORD StoreInteger(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    //LONG value = jass_checkinteger(j, 4);
    return 0;
}
DWORD StoreReal(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    //FLOAT value = jass_checknumber(j, 4);
    return 0;
}
DWORD StoreBoolean(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    //BOOL value = jass_checkboolean(j, 4);
    return 0;
}
DWORD StoreUnit(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    //HANDLE whichUnit = jass_checkhandle(j, 4, "unit");
    return jass_pushboolean(j, 0);
}
DWORD SyncStoredInteger(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    return 0;
}
DWORD SyncStoredReal(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    return 0;
}
DWORD SyncStoredBoolean(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    return 0;
}
DWORD SyncStoredUnit(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    return 0;
}
DWORD GetStoredInteger(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    return jass_pushinteger(j, 0);
}
DWORD GetStoredReal(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    return jass_pushnumber(j, 0);
}
DWORD GetStoredBoolean(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD RestoreUnit(LPJASS j) {
    //HANDLE cache = jass_checkhandle(j, 1, "gamecache");
    //LPCSTR missionKey = jass_checkstring(j, 2);
    //LPCSTR key = jass_checkstring(j, 3);
    //HANDLE forWhichPlayer = jass_checkhandle(j, 4, "player");
    //FLOAT x = jass_checknumber(j, 5);
    //FLOAT y = jass_checknumber(j, 6);
    //FLOAT facing = jass_checknumber(j, 7);
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetRandomInt(LPJASS j) {
    //LONG lowBound = jass_checkinteger(j, 1);
    //LONG highBound = jass_checkinteger(j, 2);
    return jass_pushinteger(j, 0);
}
DWORD GetRandomReal(LPJASS j) {
    //FLOAT lowBound = jass_checknumber(j, 1);
    //FLOAT highBound = jass_checknumber(j, 2);
    return jass_pushnumber(j, 0);
}
DWORD CreateUnitPool(LPJASS j) {
    return jass_pushhandle(j, 0, "unitpool");
}
DWORD DestroyUnitPool(LPJASS j) {
    //HANDLE whichPool = jass_checkhandle(j, 1, "unitpool");
    return 0;
}
DWORD UnitPoolAddUnitType(LPJASS j) {
    //HANDLE whichPool = jass_checkhandle(j, 1, "unitpool");
    //LONG unitId = jass_checkinteger(j, 2);
    //FLOAT weight = jass_checknumber(j, 3);
    return 0;
}
DWORD UnitPoolRemoveUnitType(LPJASS j) {
    //HANDLE whichPool = jass_checkhandle(j, 1, "unitpool");
    //LONG unitId = jass_checkinteger(j, 2);
    return 0;
}
DWORD PlaceRandomUnit(LPJASS j) {
    //HANDLE whichPool = jass_checkhandle(j, 1, "unitpool");
    //HANDLE forWhichPlayer = jass_checkhandle(j, 2, "player");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    //FLOAT facing = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "unit");
}
DWORD CreateItemPool(LPJASS j) {
    return jass_pushhandle(j, 0, "itempool");
}
DWORD DestroyItemPool(LPJASS j) {
    //HANDLE whichItemPool = jass_checkhandle(j, 1, "itempool");
    return 0;
}
DWORD ItemPoolAddItemType(LPJASS j) {
    //HANDLE whichItemPool = jass_checkhandle(j, 1, "itempool");
    //LONG itemId = jass_checkinteger(j, 2);
    //FLOAT weight = jass_checknumber(j, 3);
    return 0;
}
DWORD ItemPoolRemoveItemType(LPJASS j) {
    //HANDLE whichItemPool = jass_checkhandle(j, 1, "itempool");
    //LONG itemId = jass_checkinteger(j, 2);
    return 0;
}
DWORD PlaceRandomItem(LPJASS j) {
    //HANDLE whichItemPool = jass_checkhandle(j, 1, "itempool");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return jass_pushhandle(j, 0, "item");
}
DWORD ChooseRandomCreep(LPJASS j) {
    //LONG level = jass_checkinteger(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD ChooseRandomNPBuilding(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD ChooseRandomItem(LPJASS j) {
    //LONG level = jass_checkinteger(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD SetRandomSeed(LPJASS j) {
    //LONG seed = jass_checkinteger(j, 1);
    return 0;
}
DWORD SetTerrainFog(LPJASS j) {
    //FLOAT a = jass_checknumber(j, 1);
    //FLOAT b = jass_checknumber(j, 2);
    //FLOAT c = jass_checknumber(j, 3);
    //FLOAT d = jass_checknumber(j, 4);
    //FLOAT e = jass_checknumber(j, 5);
    return 0;
}
DWORD ResetTerrainFog(LPJASS j) {
    return 0;
}
DWORD SetUnitFog(LPJASS j) {
    //FLOAT a = jass_checknumber(j, 1);
    //FLOAT b = jass_checknumber(j, 2);
    //FLOAT c = jass_checknumber(j, 3);
    //FLOAT d = jass_checknumber(j, 4);
    //FLOAT e = jass_checknumber(j, 5);
    return 0;
}
DWORD SetTerrainFogEx(LPJASS j) {
    //LONG style = jass_checkinteger(j, 1);
    //FLOAT zstart = jass_checknumber(j, 2);
    //FLOAT zend = jass_checknumber(j, 3);
    //FLOAT density = jass_checknumber(j, 4);
    //FLOAT red = jass_checknumber(j, 5);
    //FLOAT green = jass_checknumber(j, 6);
    //FLOAT blue = jass_checknumber(j, 7);
    return 0;
}
DWORD DisplayTextToPlayer(LPJASS j) {
    //HANDLE toPlayer = jass_checkhandle(j, 1, "player");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //LPCSTR message = jass_checkstring(j, 4);
    return 0;
}
DWORD DisplayTimedTextToPlayer(LPJASS j) {
    //HANDLE toPlayer = jass_checkhandle(j, 1, "player");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT duration = jass_checknumber(j, 4);
    //LPCSTR message = jass_checkstring(j, 5);
    return 0;
}
DWORD DisplayTimedTextFromPlayer(LPJASS j) {
    //HANDLE toPlayer = jass_checkhandle(j, 1, "player");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT duration = jass_checknumber(j, 4);
    //LPCSTR message = jass_checkstring(j, 5);
    return 0;
}
DWORD ClearTextMessages(LPJASS j) {
    return 0;
}
DWORD SetDayNightModels(LPJASS j) {
    LPCSTR terrainDNCFile = jass_checkstring(j, 1);
    LPCSTR unitDNCFile = jass_checkstring(j, 2);
    return 0;
}
DWORD SetSkyModel(LPJASS j) {
    //LPCSTR skyModelFile = jass_checkstring(j, 1);
    return 0;
}
DWORD EnableUserControl(LPJASS j) {
    //BOOL b = jass_checkboolean(j, 1);
    return 0;
}
DWORD SuspendTimeOfDay(LPJASS j) {
    //BOOL b = jass_checkboolean(j, 1);
    return 0;
}
DWORD SetTimeOfDayScale(LPJASS j) {
    //FLOAT r = jass_checknumber(j, 1);
    return 0;
}
DWORD GetTimeOfDayScale(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD ShowInterface(LPJASS j) {
    //BOOL flag = jass_checkboolean(j, 1);
    //FLOAT fadeDuration = jass_checknumber(j, 2);
    return 0;
}
DWORD PauseGame(LPJASS j) {
    //BOOL flag = jass_checkboolean(j, 1);
    return 0;
}
DWORD UnitAddIndicator(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD AddIndicator(LPJASS j) {
    //HANDLE whichWidget = jass_checkhandle(j, 1, "widget");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD PingMinimap(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD EnableOcclusion(LPJASS j) {
    //BOOL flag = jass_checkboolean(j, 1);
    return 0;
}
DWORD SetIntroShotText(LPJASS j) {
    //LPCSTR introText = jass_checkstring(j, 1);
    return 0;
}
DWORD SetIntroShotModel(LPJASS j) {
    //LPCSTR introModelPath = jass_checkstring(j, 1);
    return 0;
}
DWORD EnableWorldFogBoundary(LPJASS j) {
    //BOOL b = jass_checkboolean(j, 1);
    return 0;
}
DWORD PlayCinematic(LPJASS j) {
    //LPCSTR movieName = jass_checkstring(j, 1);
    return 0;
}
DWORD ForceUIKey(LPJASS j) {
    //LPCSTR key = jass_checkstring(j, 1);
    return 0;
}
DWORD ForceUICancel(LPJASS j) {
    return 0;
}
DWORD DisplayLoadDialog(LPJASS j) {
    return 0;
}
DWORD CreateTrackable(LPJASS j) {
    //LPCSTR trackableModelPath = jass_checkstring(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT facing = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "trackable");
}
DWORD CreateQuest(LPJASS j) {
    return jass_pushhandle(j, 0, "quest");
}
DWORD DestroyQuest(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    return 0;
}
DWORD QuestSetTitle(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //LPCSTR title = jass_checkstring(j, 2);
    return 0;
}
DWORD QuestSetDescription(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //LPCSTR description = jass_checkstring(j, 2);
    return 0;
}
DWORD QuestSetIconPath(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //LPCSTR iconPath = jass_checkstring(j, 2);
    return 0;
}
DWORD QuestSetRequired(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //BOOL required = jass_checkboolean(j, 2);
    return 0;
}
DWORD QuestSetCompleted(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //BOOL completed = jass_checkboolean(j, 2);
    return 0;
}
DWORD QuestSetDiscovered(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //BOOL discovered = jass_checkboolean(j, 2);
    return 0;
}
DWORD QuestSetFailed(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //BOOL failed = jass_checkboolean(j, 2);
    return 0;
}
DWORD QuestSetEnabled(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    //BOOL enabled = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsQuestRequired(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    return jass_pushboolean(j, 0);
}
DWORD IsQuestCompleted(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    return jass_pushboolean(j, 0);
}
DWORD IsQuestDiscovered(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    return jass_pushboolean(j, 0);
}
DWORD IsQuestFailed(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    return jass_pushboolean(j, 0);
}
DWORD IsQuestEnabled(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    return jass_pushboolean(j, 0);
}
DWORD QuestCreateItem(LPJASS j) {
    //HANDLE whichQuest = jass_checkhandle(j, 1, "quest");
    return jass_pushhandle(j, 0, "questitem");
}
DWORD QuestItemSetDescription(LPJASS j) {
    //HANDLE whichQuestItem = jass_checkhandle(j, 1, "questitem");
    //LPCSTR description = jass_checkstring(j, 2);
    return 0;
}
DWORD QuestItemSetCompleted(LPJASS j) {
    //HANDLE whichQuestItem = jass_checkhandle(j, 1, "questitem");
    //BOOL completed = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsQuestItemCompleted(LPJASS j) {
    //HANDLE whichQuestItem = jass_checkhandle(j, 1, "questitem");
    return jass_pushboolean(j, 0);
}
DWORD CreateDefeatCondition(LPJASS j) {
    return jass_pushhandle(j, 0, "defeatcondition");
}
DWORD DestroyDefeatCondition(LPJASS j) {
    //HANDLE whichCondition = jass_checkhandle(j, 1, "defeatcondition");
    return 0;
}
DWORD DefeatConditionSetDescription(LPJASS j) {
    //HANDLE whichCondition = jass_checkhandle(j, 1, "defeatcondition");
    //LPCSTR description = jass_checkstring(j, 2);
    return 0;
}
DWORD FlashQuestDialogButton(LPJASS j) {
    return 0;
}
DWORD ForceQuestDialogUpdate(LPJASS j) {
    return 0;
}
DWORD CreateTimerDialog(LPJASS j) {
    //HANDLE t = jass_checkhandle(j, 1, "timer");
    return jass_pushhandle(j, 0, "timerdialog");
}
DWORD DestroyTimerDialog(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "timerdialog");
    return 0;
}
DWORD TimerDialogSetTitle(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "timerdialog");
    //LPCSTR title = jass_checkstring(j, 2);
    return 0;
}
DWORD TimerDialogSetTitleColor(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "timerdialog");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD TimerDialogSetTimeColor(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "timerdialog");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD TimerDialogSetSpeed(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "timerdialog");
    //FLOAT speedMultFactor = jass_checknumber(j, 2);
    return 0;
}
DWORD TimerDialogDisplay(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "timerdialog");
    //BOOL display = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsTimerDialogDisplayed(LPJASS j) {
    //HANDLE whichDialog = jass_checkhandle(j, 1, "timerdialog");
    return jass_pushboolean(j, 0);
}
DWORD CreateLeaderboard(LPJASS j) {
    return jass_pushhandle(j, 0, "leaderboard");
}
DWORD DestroyLeaderboard(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    return 0;
}
DWORD LeaderboardDisplay(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //BOOL show = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsLeaderboardDisplayed(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    return jass_pushboolean(j, 0);
}
DWORD LeaderboardGetItemCount(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    return jass_pushinteger(j, 0);
}
DWORD LeaderboardSetSizeByItemCount(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG count = jass_checkinteger(j, 2);
    return 0;
}
DWORD LeaderboardAddItem(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LPCSTR label = jass_checkstring(j, 2);
    //LONG value = jass_checkinteger(j, 3);
    //HANDLE p = jass_checkhandle(j, 4, "player");
    return 0;
}
DWORD LeaderboardRemoveItem(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG index = jass_checkinteger(j, 2);
    return 0;
}
DWORD LeaderboardRemovePlayerItem(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //HANDLE p = jass_checkhandle(j, 2, "player");
    return 0;
}
DWORD LeaderboardClear(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    return 0;
}
DWORD LeaderboardSortItemsByValue(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //BOOL ascending = jass_checkboolean(j, 2);
    return 0;
}
DWORD LeaderboardSortItemsByPlayer(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //BOOL ascending = jass_checkboolean(j, 2);
    return 0;
}
DWORD LeaderboardSortItemsByLabel(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //BOOL ascending = jass_checkboolean(j, 2);
    return 0;
}
DWORD LeaderboardHasPlayerItem(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //HANDLE p = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD LeaderboardGetPlayerIndex(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //HANDLE p = jass_checkhandle(j, 2, "player");
    return jass_pushinteger(j, 0);
}
DWORD LeaderboardSetLabel(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LPCSTR label = jass_checkstring(j, 2);
    return 0;
}
DWORD LeaderboardGetLabelText(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    return jass_pushstring(j, 0);
}
DWORD PlayerSetLeaderboard(LPJASS j) {
    //HANDLE toPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE lb = jass_checkhandle(j, 2, "leaderboard");
    return 0;
}
DWORD PlayerGetLeaderboard(LPJASS j) {
    //HANDLE toPlayer = jass_checkhandle(j, 1, "player");
    return jass_pushhandle(j, 0, "leaderboard");
}
DWORD LeaderboardSetLabelColor(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD LeaderboardSetValueColor(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD LeaderboardSetStyle(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //BOOL showLabel = jass_checkboolean(j, 2);
    //BOOL showNames = jass_checkboolean(j, 3);
    //BOOL showValues = jass_checkboolean(j, 4);
    //BOOL showIcons = jass_checkboolean(j, 5);
    return 0;
}
DWORD LeaderboardSetItemValue(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG whichItem = jass_checkinteger(j, 2);
    //LONG val = jass_checkinteger(j, 3);
    return 0;
}
DWORD LeaderboardSetItemLabel(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG whichItem = jass_checkinteger(j, 2);
    //LPCSTR val = jass_checkstring(j, 3);
    return 0;
}
DWORD LeaderboardSetItemStyle(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG whichItem = jass_checkinteger(j, 2);
    //BOOL showLabel = jass_checkboolean(j, 3);
    //BOOL showValue = jass_checkboolean(j, 4);
    //BOOL showIcon = jass_checkboolean(j, 5);
    return 0;
}
DWORD LeaderboardSetItemLabelColor(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG whichItem = jass_checkinteger(j, 2);
    //LONG red = jass_checkinteger(j, 3);
    //LONG green = jass_checkinteger(j, 4);
    //LONG blue = jass_checkinteger(j, 5);
    //LONG alpha = jass_checkinteger(j, 6);
    return 0;
}
DWORD LeaderboardSetItemValueColor(LPJASS j) {
    //HANDLE lb = jass_checkhandle(j, 1, "leaderboard");
    //LONG whichItem = jass_checkinteger(j, 2);
    //LONG red = jass_checkinteger(j, 3);
    //LONG green = jass_checkinteger(j, 4);
    //LONG blue = jass_checkinteger(j, 5);
    //LONG alpha = jass_checkinteger(j, 6);
    return 0;
}
DWORD SetCameraPosition(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    return 0;
}
DWORD SetCameraQuickPosition(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    return 0;
}
DWORD SetCameraBounds(LPJASS j) {
    FLOAT x1 = jass_checknumber(j, 1);
    FLOAT y1 = jass_checknumber(j, 2);
    FLOAT x2 = jass_checknumber(j, 3);
    FLOAT y2 = jass_checknumber(j, 4);
    FLOAT x3 = jass_checknumber(j, 5);
    FLOAT y3 = jass_checknumber(j, 6);
    FLOAT x4 = jass_checknumber(j, 7);
    FLOAT y4 = jass_checknumber(j, 8);
    return 0;
}
DWORD StopCamera(LPJASS j) {
    return 0;
}
DWORD ResetToGameCamera(LPJASS j) {
    //FLOAT duration = jass_checknumber(j, 1);
    return 0;
}
DWORD PanCameraTo(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    return 0;
}
DWORD PanCameraToTimed(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD PanCameraToWithZ(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //FLOAT zOffsetDest = jass_checknumber(j, 3);
    return 0;
}
DWORD PanCameraToTimedWithZ(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //FLOAT zOffsetDest = jass_checknumber(j, 3);
    //FLOAT duration = jass_checknumber(j, 4);
    return 0;
}
DWORD SetCinematicCamera(LPJASS j) {
    //LPCSTR cameraModelFile = jass_checkstring(j, 1);
    return 0;
}
DWORD SetCameraField(LPJASS j) {
    //HANDLE whichField = jass_checkhandle(j, 1, "camerafield");
    //FLOAT value = jass_checknumber(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD AdjustCameraField(LPJASS j) {
    //HANDLE whichField = jass_checkhandle(j, 1, "camerafield");
    //FLOAT offset = jass_checknumber(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD SetCameraTargetController(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT xoffset = jass_checknumber(j, 2);
    //FLOAT yoffset = jass_checknumber(j, 3);
    //BOOL inheritOrientation = jass_checkboolean(j, 4);
    return 0;
}
DWORD SetCameraOrientController(LPJASS j) {
    //HANDLE whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT xoffset = jass_checknumber(j, 2);
    //FLOAT yoffset = jass_checknumber(j, 3);
    return 0;
}
DWORD CreateCameraSetup(LPJASS j) {
    return jass_pushhandle(j, 0, "camerasetup");
}
DWORD CameraSetupSetField(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //HANDLE whichField = jass_checkhandle(j, 2, "camerafield");
    //FLOAT value = jass_checknumber(j, 3);
    //FLOAT duration = jass_checknumber(j, 4);
    return 0;
}
DWORD CameraSetupGetField(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //HANDLE whichField = jass_checkhandle(j, 2, "camerafield");
    return jass_pushnumber(j, 0);
}
DWORD CameraSetupSetDestPosition(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT duration = jass_checknumber(j, 4);
    return 0;
}
DWORD CameraSetupGetDestPositionLoc(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    return jass_pushhandle(j, 0, "location");
}
DWORD CameraSetupGetDestPositionX(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    return jass_pushnumber(j, 0);
}
DWORD CameraSetupGetDestPositionY(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    return jass_pushnumber(j, 0);
}
DWORD CameraSetupApply(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //BOOL doPan = jass_checkboolean(j, 2);
    //BOOL panTimed = jass_checkboolean(j, 3);
    return 0;
}
DWORD CameraSetupApplyWithZ(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //FLOAT zDestOffset = jass_checknumber(j, 2);
    return 0;
}
DWORD CameraSetupApplyForceDuration(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //BOOL doPan = jass_checkboolean(j, 2);
    //FLOAT forceDuration = jass_checknumber(j, 3);
    return 0;
}
DWORD CameraSetupApplyForceDurationWithZ(LPJASS j) {
    //HANDLE whichSetup = jass_checkhandle(j, 1, "camerasetup");
    //FLOAT zDestOffset = jass_checknumber(j, 2);
    //FLOAT forceDuration = jass_checknumber(j, 3);
    return 0;
}
DWORD CameraSetTargetNoise(LPJASS j) {
    //FLOAT mag = jass_checknumber(j, 1);
    //FLOAT velocity = jass_checknumber(j, 2);
    return 0;
}
DWORD CameraSetSourceNoise(LPJASS j) {
    //FLOAT mag = jass_checknumber(j, 1);
    //FLOAT velocity = jass_checknumber(j, 2);
    return 0;
}
DWORD CameraSetSmoothingFactor(LPJASS j) {
    //FLOAT factor = jass_checknumber(j, 1);
    return 0;
}
DWORD SetCineFilterTexture(LPJASS j) {
    //LPCSTR filename = jass_checkstring(j, 1);
    return 0;
}
DWORD SetCineFilterBlendMode(LPJASS j) {
    //HANDLE whichMode = jass_checkhandle(j, 1, "blendmode");
    return 0;
}
DWORD SetCineFilterTexMapFlags(LPJASS j) {
    //HANDLE whichFlags = jass_checkhandle(j, 1, "texmapflags");
    return 0;
}
DWORD SetCineFilterStartUV(LPJASS j) {
    //FLOAT minu = jass_checknumber(j, 1);
    //FLOAT minv = jass_checknumber(j, 2);
    //FLOAT maxu = jass_checknumber(j, 3);
    //FLOAT maxv = jass_checknumber(j, 4);
    return 0;
}
DWORD SetCineFilterEndUV(LPJASS j) {
    //FLOAT minu = jass_checknumber(j, 1);
    //FLOAT minv = jass_checknumber(j, 2);
    //FLOAT maxu = jass_checknumber(j, 3);
    //FLOAT maxv = jass_checknumber(j, 4);
    return 0;
}
DWORD SetCineFilterStartColor(LPJASS j) {
    //LONG red = jass_checkinteger(j, 1);
    //LONG green = jass_checkinteger(j, 2);
    //LONG blue = jass_checkinteger(j, 3);
    //LONG alpha = jass_checkinteger(j, 4);
    return 0;
}
DWORD SetCineFilterEndColor(LPJASS j) {
    //LONG red = jass_checkinteger(j, 1);
    //LONG green = jass_checkinteger(j, 2);
    //LONG blue = jass_checkinteger(j, 3);
    //LONG alpha = jass_checkinteger(j, 4);
    return 0;
}
DWORD SetCineFilterDuration(LPJASS j) {
    //FLOAT duration = jass_checknumber(j, 1);
    return 0;
}
DWORD DisplayCineFilter(LPJASS j) {
    //BOOL flag = jass_checkboolean(j, 1);
    return 0;
}
DWORD IsCineFilterDisplayed(LPJASS j) {
    return jass_pushboolean(j, 0);
}
DWORD SetCinematicScene(LPJASS j) {
    //LONG portraitUnitId = jass_checkinteger(j, 1);
    //HANDLE color = jass_checkhandle(j, 2, "playercolor");
    //LPCSTR speakerTitle = jass_checkstring(j, 3);
    //LPCSTR text = jass_checkstring(j, 4);
    //FLOAT sceneDuration = jass_checknumber(j, 5);
    //FLOAT voiceoverDuration = jass_checknumber(j, 6);
    return 0;
}
DWORD EndCinematicScene(LPJASS j) {
    return 0;
}
DWORD GetCameraMargin(LPJASS j) {
    LONG whichMargin = jass_checkinteger(j, 1);
    mapCameraBounds_t const *bounds = &game_state.mapinfo->cameraBounds;
    switch (whichMargin) {
        case 0: jass_pushnumber(j, bounds->margin.left * TILESIZE); break;
        case 1: jass_pushnumber(j, bounds->margin.right * TILESIZE); break;
        case 2: jass_pushnumber(j, bounds->margin.top * TILESIZE); break;
        case 3: jass_pushnumber(j, bounds->margin.bottom * TILESIZE); break;
        default: jass_pushnull(j);
    }
    return 1;
}
DWORD GetCameraBoundMinX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraBoundMinY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraBoundMaxX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraBoundMaxY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraField(LPJASS j) {
    //HANDLE whichField = jass_checkhandle(j, 1, "camerafield");
    return jass_pushnumber(j, 0);
}
DWORD GetCameraTargetPositionX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraTargetPositionY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraTargetPositionZ(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraTargetPositionLoc(LPJASS j) {
    return jass_pushhandle(j, 0, "location");
}
DWORD GetCameraEyePositionX(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraEyePositionY(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraEyePositionZ(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetCameraEyePositionLoc(LPJASS j) {
    return jass_pushhandle(j, 0, "location");
}
DWORD NewSoundEnvironment(LPJASS j) {
    //LPCSTR environmentName = jass_checkstring(j, 1);
    return 0;
}
DWORD CreateSound(LPJASS j) {
    //LPCSTR fileName = jass_checkstring(j, 1);
    //BOOL looping = jass_checkboolean(j, 2);
    //BOOL is3D = jass_checkboolean(j, 3);
    //BOOL stopwhenoutofrange = jass_checkboolean(j, 4);
    //LONG fadeInRate = jass_checkinteger(j, 5);
    //LONG fadeOutRate = jass_checkinteger(j, 6);
    //LPCSTR eaxSetting = jass_checkstring(j, 7);
    return jass_pushhandle(j, 0, "sound");
}
DWORD CreateSoundFilenameWithLabel(LPJASS j) {
    //LPCSTR fileName = jass_checkstring(j, 1);
    //BOOL looping = jass_checkboolean(j, 2);
    //BOOL is3D = jass_checkboolean(j, 3);
    //BOOL stopwhenoutofrange = jass_checkboolean(j, 4);
    //LONG fadeInRate = jass_checkinteger(j, 5);
    //LONG fadeOutRate = jass_checkinteger(j, 6);
    //LPCSTR SLKEntryName = jass_checkstring(j, 7);
    return jass_pushhandle(j, 0, "sound");
}
DWORD CreateSoundFromLabel(LPJASS j) {
    //LPCSTR soundLabel = jass_checkstring(j, 1);
    //BOOL looping = jass_checkboolean(j, 2);
    //BOOL is3D = jass_checkboolean(j, 3);
    //BOOL stopwhenoutofrange = jass_checkboolean(j, 4);
    //LONG fadeInRate = jass_checkinteger(j, 5);
    //LONG fadeOutRate = jass_checkinteger(j, 6);
    return jass_pushhandle(j, 0, "sound");
}
DWORD CreateMIDISound(LPJASS j) {
    //LPCSTR soundLabel = jass_checkstring(j, 1);
    //LONG fadeInRate = jass_checkinteger(j, 2);
    //LONG fadeOutRate = jass_checkinteger(j, 3);
    return jass_pushhandle(j, 0, "sound");
}
DWORD SetSoundParamsFromLabel(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //LPCSTR soundLabel = jass_checkstring(j, 2);
    return 0;
}
DWORD SetSoundDistanceCutoff(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //FLOAT cutoff = jass_checknumber(j, 2);
    return 0;
}
DWORD SetSoundChannel(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //LONG channel = jass_checkinteger(j, 2);
    return 0;
}
DWORD SetSoundVolume(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //LONG volume = jass_checkinteger(j, 2);
    return 0;
}
DWORD SetSoundPitch(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //FLOAT pitch = jass_checknumber(j, 2);
    return 0;
}
DWORD SetSoundDistances(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //FLOAT minDist = jass_checknumber(j, 2);
    //FLOAT maxDist = jass_checknumber(j, 3);
    return 0;
}
DWORD SetSoundConeAngles(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //FLOAT inside = jass_checknumber(j, 2);
    //FLOAT outside = jass_checknumber(j, 3);
    //LONG outsideVolume = jass_checkinteger(j, 4);
    return 0;
}
DWORD SetSoundConeOrientation(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT z = jass_checknumber(j, 4);
    return 0;
}
DWORD SetSoundPosition(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT z = jass_checknumber(j, 4);
    return 0;
}
DWORD SetSoundVelocity(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT z = jass_checknumber(j, 4);
    return 0;
}
DWORD AttachSoundToUnit(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    return 0;
}
DWORD StartSound(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    return 0;
}
DWORD StopSound(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //BOOL killWhenDone = jass_checkboolean(j, 2);
    //BOOL fadeOut = jass_checkboolean(j, 3);
    return 0;
}
DWORD KillSoundWhenDone(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    return 0;
}
DWORD SetMusicVolume(LPJASS j) {
    //LONG volume = jass_checkinteger(j, 1);
    return 0;
}
DWORD PlayMusic(LPJASS j) {
    //LPCSTR musicName = jass_checkstring(j, 1);
    return 0;
}
DWORD SetMapMusic(LPJASS j) {
    //LPCSTR musicName = jass_checkstring(j, 1);
    //BOOL random = jass_checkboolean(j, 2);
    //LONG index = jass_checkinteger(j, 3);
    return 0;
}
DWORD ClearMapMusic(LPJASS j) {
    return 0;
}
DWORD PlayThematicMusic(LPJASS j) {
    //LPCSTR musicFileName = jass_checkstring(j, 1);
    return 0;
}
DWORD EndThematicMusic(LPJASS j) {
    return 0;
}
DWORD StopMusic(LPJASS j) {
    //BOOL fadeOut = jass_checkboolean(j, 1);
    return 0;
}
DWORD ResumeMusic(LPJASS j) {
    return 0;
}
DWORD SetSoundDuration(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //LONG duration = jass_checkinteger(j, 2);
    return 0;
}
DWORD GetSoundDuration(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    return jass_pushinteger(j, 0);
}
DWORD GetSoundFileDuration(LPJASS j) {
    //LPCSTR musicFileName = jass_checkstring(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD VolumeGroupSetVolume(LPJASS j) {
    //HANDLE vgroup = jass_checkhandle(j, 1, "volumegroup");
    //FLOAT scale = jass_checknumber(j, 2);
    return 0;
}
DWORD VolumeGroupReset(LPJASS j) {
    return 0;
}
DWORD GetSoundIsPlaying(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    return jass_pushboolean(j, 0);
}
DWORD GetSoundIsLoading(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    return jass_pushboolean(j, 0);
}
DWORD RegisterStackedSound(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //BOOL byPosition = jass_checkboolean(j, 2);
    //FLOAT rectwidth = jass_checknumber(j, 3);
    //FLOAT rectheight = jass_checknumber(j, 4);
    return 0;
}
DWORD UnregisterStackedSound(LPJASS j) {
    //HANDLE soundHandle = jass_checkhandle(j, 1, "sound");
    //BOOL byPosition = jass_checkboolean(j, 2);
    //FLOAT rectwidth = jass_checknumber(j, 3);
    //FLOAT rectheight = jass_checknumber(j, 4);
    return 0;
}
DWORD AddWeatherEffect(LPJASS j) {
    //HANDLE where = jass_checkhandle(j, 1, "rect");
    //LONG effectID = jass_checkinteger(j, 2);
    return jass_pushhandle(j, 0, "weathereffect");
}
DWORD RemoveWeatherEffect(LPJASS j) {
    //HANDLE whichEffect = jass_checkhandle(j, 1, "weathereffect");
    return 0;
}
DWORD EnableWeatherEffect(LPJASS j) {
    //HANDLE whichEffect = jass_checkhandle(j, 1, "weathereffect");
    //BOOL enable = jass_checkboolean(j, 2);
    return 0;
}
DWORD AddSpecialEffect(LPJASS j) {
    //LPCSTR modelName = jass_checkstring(j, 1);
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    return jass_pushhandle(j, 0, "effect");
}
DWORD AddSpecialEffectLoc(LPJASS j) {
    //LPCSTR modelName = jass_checkstring(j, 1);
    //HANDLE where = jass_checkhandle(j, 2, "location");
    return jass_pushhandle(j, 0, "effect");
}
DWORD AddSpecialEffectTarget(LPJASS j) {
    //LPCSTR modelName = jass_checkstring(j, 1);
    //HANDLE targetWidget = jass_checkhandle(j, 2, "widget");
    //LPCSTR attachPointName = jass_checkstring(j, 3);
    return jass_pushhandle(j, 0, "effect");
}
DWORD DestroyEffect(LPJASS j) {
    //HANDLE whichEffect = jass_checkhandle(j, 1, "effect");
    return 0;
}
DWORD AddSpellEffect(LPJASS j) {
    //LPCSTR abilityString = jass_checkstring(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "effect");
}
DWORD AddSpellEffectLoc(LPJASS j) {
    //LPCSTR abilityString = jass_checkstring(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE where = jass_checkhandle(j, 3, "location");
    return jass_pushhandle(j, 0, "effect");
}
DWORD AddSpellEffectById(LPJASS j) {
    //LONG abilityId = jass_checkinteger(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "effect");
}
DWORD AddSpellEffectByIdLoc(LPJASS j) {
    //LONG abilityId = jass_checkinteger(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE where = jass_checkhandle(j, 3, "location");
    return jass_pushhandle(j, 0, "effect");
}
DWORD AddSpellEffectTarget(LPJASS j) {
    //LPCSTR modelName = jass_checkstring(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    //LPCSTR attachPoint = jass_checkstring(j, 4);
    return jass_pushhandle(j, 0, "effect");
}
DWORD AddSpellEffectTargetById(LPJASS j) {
    //LONG abilityId = jass_checkinteger(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    //LPCSTR attachPoint = jass_checkstring(j, 4);
    return jass_pushhandle(j, 0, "effect");
}
DWORD SetBlight(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT radius = jass_checknumber(j, 4);
    //BOOL addBlight = jass_checkboolean(j, 5);
    return 0;
}
DWORD SetBlightRect(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE r = jass_checkhandle(j, 2, "rect");
    //BOOL addBlight = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetBlightPoint(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //BOOL addBlight = jass_checkboolean(j, 4);
    return 0;
}
DWORD SetBlightLoc(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    //FLOAT radius = jass_checknumber(j, 3);
    //BOOL addBlight = jass_checkboolean(j, 4);
    return 0;
}
DWORD CreateBlightedGoldmine(LPJASS j) {
    //HANDLE id = jass_checkhandle(j, 1, "player");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT face = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "unit");
}
DWORD SetDoodadAnimation(LPJASS j) {
    //FLOAT x = jass_checknumber(j, 1);
    //FLOAT y = jass_checknumber(j, 2);
    //FLOAT radius = jass_checknumber(j, 3);
    //LONG doodadID = jass_checkinteger(j, 4);
    //BOOL nearestOnly = jass_checkboolean(j, 5);
    //LPCSTR animName = jass_checkstring(j, 6);
    //BOOL animRandom = jass_checkboolean(j, 7);
    return 0;
}
DWORD SetDoodadAnimationRect(LPJASS j) {
    //HANDLE r = jass_checkhandle(j, 1, "rect");
    //LONG doodadID = jass_checkinteger(j, 2);
    //LPCSTR animName = jass_checkstring(j, 3);
    //BOOL animRandom = jass_checkboolean(j, 4);
    return 0;
}
DWORD StartMeleeAI(LPJASS j) {
    //HANDLE num = jass_checkhandle(j, 1, "player");
    //LPCSTR script = jass_checkstring(j, 2);
    return 0;
}
DWORD StartCampaignAI(LPJASS j) {
    //HANDLE num = jass_checkhandle(j, 1, "player");
    //LPCSTR script = jass_checkstring(j, 2);
    return 0;
}
DWORD CommandAI(LPJASS j) {
    //HANDLE num = jass_checkhandle(j, 1, "player");
    //LONG command = jass_checkinteger(j, 2);
    //LONG data = jass_checkinteger(j, 3);
    return 0;
}
DWORD PauseCompAI(LPJASS j) {
    //HANDLE p = jass_checkhandle(j, 1, "player");
    //BOOL pause = jass_checkboolean(j, 2);
    return 0;
}
DWORD RemoveGuardPosition(LPJASS j) {
    //HANDLE hUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD RecycleGuardPosition(LPJASS j) {
    //HANDLE hUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD RemoveAllGuardPositions(LPJASS j) {
    //HANDLE num = jass_checkhandle(j, 1, "player");
    return 0;
}
DWORD Cheat(LPJASS j) {
    //LPCSTR cheatStr = jass_checkstring(j, 1);
    return 0;
}
DWORD IsNoVictoryCheat(LPJASS j) {
    return jass_pushboolean(j, 0);
}
DWORD IsNoDefeatCheat(LPJASS j) {
    return jass_pushboolean(j, 0);
}
DWORD Preload(LPJASS j) {
    //LPCSTR filename = jass_checkstring(j, 1);
    return 0;
}
DWORD PreloadEnd(LPJASS j) {
    //FLOAT timeout = jass_checknumber(j, 1);
    return 0;
}
DWORD PreloadGenClear(LPJASS j) {
    return 0;
}
DWORD PreloadGenStart(LPJASS j) {
    return 0;
}
DWORD PreloadGenEnd(LPJASS j) {
    //LPCSTR filename = jass_checkstring(j, 1);
    return 0;
}
DWORD Preloader(LPJASS j) {
    //LPCSTR filename = jass_checkstring(j, 1);
    return 0;
}
