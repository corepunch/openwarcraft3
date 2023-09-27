extern LPPLAYER currentplayer;

DWORD class_id(LPCSTR str) { return *(DWORD *)str; }

#define CONVERT_FUNC(NAME, TYPE) \
DWORD Convert##NAME(LPJASS j) { \
    API_ALLOC(DWORD, TYPE); \
    *TYPE = jass_checkinteger(j, 1); \
    return 1; \
}

#define MATH_FUNC(NAME, FUNC, INPUT, OUTPUT) \
DWORD NAME(LPJASS j) { \
    return jass_push##OUTPUT(j, FUNC(jass_check##INPUT(j, 1))); \
}

#define MATH_FUNC2(NAME, FUNC, OUTPUT) \
DWORD NAME(LPJASS j) { \
    FLOAT arg1 = jass_checknumber(j, 1); \
    FLOAT arg2 = jass_checknumber(j, 2); \
    return jass_push##OUTPUT(j, FUNC(arg1, arg2)); \
}

CONVERT_FUNC(Race, race);
CONVERT_FUNC(AllianceType, alliancetype);
CONVERT_FUNC(RacePref, racepreference);
CONVERT_FUNC(IGameState, igamestate);
CONVERT_FUNC(FGameState, fgamestate);
CONVERT_FUNC(PlayerState, playerstate);
CONVERT_FUNC(PlayerGameResult, playergameresult);
CONVERT_FUNC(UnitState, unitstate);
CONVERT_FUNC(GameEvent, gameevent);
CONVERT_FUNC(PlayerEvent, playerevent);
CONVERT_FUNC(PlayerUnitEvent, playerunitevent);
CONVERT_FUNC(WidgetEvent, widgetevent);
CONVERT_FUNC(DialogEvent, dialogevent);
CONVERT_FUNC(UnitEvent, unitevent);
CONVERT_FUNC(LimitOp, limitop);
CONVERT_FUNC(UnitType, unittype);
CONVERT_FUNC(GameSpeed, gamespeed);
CONVERT_FUNC(Placement, placement);
CONVERT_FUNC(StartLocPrio, startlocprio);
CONVERT_FUNC(GameDifficulty, gamedifficulty);
CONVERT_FUNC(GameType, gametype);
CONVERT_FUNC(MapFlag, mapflag);
CONVERT_FUNC(MapVisibility, mapvisibility);
CONVERT_FUNC(MapSetting, mapsetting);
CONVERT_FUNC(MapDensity, mapdensity);
CONVERT_FUNC(MapControl, mapcontrol);
CONVERT_FUNC(PlayerColor, playercolor);
CONVERT_FUNC(PlayerSlotState, playerslotstate);
CONVERT_FUNC(VolumeGroup, volumegroup);
CONVERT_FUNC(CameraField, camerafield);
CONVERT_FUNC(BlendMode, blendmode);
CONVERT_FUNC(RarityControl, raritycontrol);
CONVERT_FUNC(TexMapFlags, texmapflags);
CONVERT_FUNC(FogState, fogstate);
CONVERT_FUNC(EffectType, effecttype);

MATH_FUNC(Deg2Rad, DEG2RAD, number, number);
MATH_FUNC(Rad2Deg, RAD2DEG, number, number);
MATH_FUNC(Sin, sin, number, number);
MATH_FUNC(Cos, cos, number, number);
MATH_FUNC(Tan, tan, number, number);
MATH_FUNC(Asin, asin, number, number);
MATH_FUNC(Acos, acos, number, number);
MATH_FUNC(Atan, atan, number, number);
MATH_FUNC(SquareRoot, sqrt, number, number);
MATH_FUNC(I2R, (FLOAT), integer, number);
MATH_FUNC(R2I, (LONG), number, integer);
MATH_FUNC2(Pow, pow, number);
MATH_FUNC2(Atan2, atan2, number);
MATH_FUNC(OrderId, class_id, string, integer);
MATH_FUNC(UnitId, class_id, string, integer);
MATH_FUNC(AbilityId, class_id, string, integer);
MATH_FUNC(OrderId2String, GetClassName, integer, string);
MATH_FUNC(UnitId2String, GetClassName, integer, string);
MATH_FUNC(AbilityId2String, GetClassName, integer, string);
MATH_FUNC(S2I, atoi, string, integer);
MATH_FUNC(S2R, atoi, string, number);

DWORD I2S(LPJASS j) {
    LONG i = jass_checkinteger(j, 1);
    char buffer[64] = { 0 };
    sprintf(buffer, "%d", i);
    return jass_pushstring(j, buffer);
}
DWORD R2S(LPJASS j) {
    FLOAT r = jass_checknumber(j, 1);
    char buffer[64] = { 0 };
    sprintf(buffer, "%f", r);
    return jass_pushstring(j, buffer);
}
DWORD R2SW(LPJASS j) {
    FLOAT r = jass_checknumber(j, 1);
    //LONG width = jass_checkinteger(j, 2);
    //LONG precision = jass_checkinteger(j, 3);
    char buffer[64] = { 0 };
    sprintf(buffer, "%f", r);
    return jass_pushstring(j, buffer);
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
    return jass_pushnullhandle(j, "startlocprio");
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
    return jass_pushnullhandle(j, "gametype");
}
DWORD IsMapFlagSet(LPJASS j) {
    //HANDLE whichMapFlag = jass_checkhandle(j, 1, "mapflag");
    return jass_pushboolean(j, 0);
}
DWORD GetGamePlacement(LPJASS j) {
    return jass_pushnullhandle(j, "placement");
}
DWORD GetGameSpeed(LPJASS j) {
    return jass_pushnullhandle(j, "gamespeed");
}
DWORD GetGameDifficulty(LPJASS j) {
    return jass_pushnullhandle(j, "gamedifficulty");
}
DWORD GetResourceDensity(LPJASS j) {
    return jass_pushnullhandle(j, "mapdensity");
}
DWORD GetCreatureDensity(LPJASS j) {
    return jass_pushnullhandle(j, "mapdensity");
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
    return jass_pushnullhandle(j, "location");
}

DWORD CreateTimer(LPJASS j) {
    return jass_pushnullhandle(j, "timer");
}
DWORD DestroyTimer(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    return 0;
}
DWORD TimerStart(LPJASS j) {
    //HANDLE whichTimer = jass_checkhandle(j, 1, "timer");
    //FLOAT timeout = jass_checknumber(j, 2);
    //BOOL periodic = jass_checkboolean(j, 3);
    //LPCJASSFUNC handlerFunc = jass_checkcode(j, 4);
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
    return jass_pushnullhandle(j, "timer");
}
DWORD CreateForce(LPJASS j) {
    API_ALLOC(DWORD, force);
    return 1;
}
DWORD ForceAddPlayer(LPJASS j) {
    LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    *whichForce |= 1 << PLAYER_NUM(whichPlayer);
    return 0;
}
DWORD ForceRemovePlayer(LPJASS j) {
    LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    *whichForce &= ~(1 << PLAYER_NUM(whichPlayer));
    return 0;
}
DWORD ForceClear(LPJASS j) {
    LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    *whichForce = 0;
    return 0;
}
DWORD ForceEnumPlayers(LPJASS j) {
    //LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE filter = jass_checkhandle(j, 2, "boolexpr");
    return 0;
}
DWORD ForceEnumPlayersCounted(LPJASS j) {
    //LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    //HANDLE filter = jass_checkhandle(j, 2, "boolexpr");
    //LONG countLimit = jass_checkinteger(j, 3);
    return 0;
}
DWORD ForceEnumAllies(LPJASS j) {
    //LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    //LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD ForceEnumEnemies(LPJASS j) {
    //LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    //LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD ForForce(LPJASS j) {
    //LPDWORD whichForce = jass_checkhandle(j, 1, "force");
    //LPCJASSFUNC callback = jass_checkcode(j, 2);
    return 0;
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
    return jass_pushnullhandle(j, "rect");
}
DWORD GetFilterUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetEnumUnit(LPJASS j) {
    extern LPEDICT currentunit;
    return jass_pushlighthandle(j, currentunit, "unit");
}
DWORD GetFilterDestructable(LPJASS j) {
    return jass_pushnullhandle(j, "destructable");
}
DWORD GetEnumDestructable(LPJASS j) {
    return jass_pushnullhandle(j, "destructable");
}
DWORD GetFilterPlayer(LPJASS j) {
    return jass_pushnullhandle(j, "player");
}
DWORD GetEnumPlayer(LPJASS j) {
    return jass_pushnullhandle(j, "player");
}
DWORD ExecuteFunc(LPJASS j) {
    //LPCSTR funcName = jass_checkstring(j, 1);
    return 0;
}
DWORD And(LPJASS j) {
    //HANDLE operandA = jass_checkhandle(j, 1, "boolexpr");
    //HANDLE operandB = jass_checkhandle(j, 2, "boolexpr");
    return jass_pushnullhandle(j, "boolexpr");
}
DWORD Or(LPJASS j) {
    //HANDLE operandA = jass_checkhandle(j, 1, "boolexpr");
    //HANDLE operandB = jass_checkhandle(j, 2, "boolexpr");
    return jass_pushnullhandle(j, "boolexpr");
}
DWORD Not(LPJASS j) {
    //HANDLE operand = jass_checkhandle(j, 1, "boolexpr");
    return jass_pushnullhandle(j, "boolexpr");
}
DWORD Condition(LPJASS j) {
    LPCJASSFUNC func = jass_checkcode(j, 1);
    return jass_pushlighthandle(j, (HANDLE)func, "conditionfunc");
}
DWORD DestroyCondition(LPJASS j) {
    //HANDLE c = jass_checkhandle(j, 1, "conditionfunc");
    return 0;
}
DWORD Filter(LPJASS j) {
    //LPCJASSFUNC func = jass_checkcode(j, 1);
    return jass_pushnullhandle(j, "filterfunc");
}
DWORD DestroyFilter(LPJASS j) {
    //HANDLE f = jass_checkhandle(j, 1, "filterfunc");
    return 0;
}
DWORD DestroyBoolExpr(LPJASS j) {
    //HANDLE e = jass_checkhandle(j, 1, "boolexpr");
    return 0;
}
DWORD GetEventGameState(LPJASS j) {
    return jass_pushnullhandle(j, "gamestate");
}
DWORD GetWinningPlayer(LPJASS j) {
    return jass_pushnullhandle(j, "player");
}
DWORD GetEnteringUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->trigger, "unit");
}
DWORD GetLeavingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetTriggeringTrackable(LPJASS j) {
    return jass_pushnullhandle(j, "trackable");
}
DWORD GetClickedButton(LPJASS j) {
    return jass_pushnullhandle(j, "button");
}
DWORD GetClickedDialog(LPJASS j) {
    return jass_pushnullhandle(j, "dialog");
}
DWORD GetLevelingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetLearningUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetLearnedSkill(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetLearnedSkillLevel(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetRevivableUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetRevivingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetAttacker(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetRescuer(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetDyingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetKillingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetDecayingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetConstructingStructure(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetCancelledStructure(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetConstructedStructure(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetResearchingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetResearched(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetTrainedUnitType(LPJASS j) {
    return jass_pushinteger(j, 0);
}
DWORD GetTrainedUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetDetectedUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetSummoningUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetSummonedUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetTransportUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetLoadedUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetManipulatingUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetOrderedUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
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
    return jass_pushnullhandle(j, "location");
}
DWORD GetOrderTarget(LPJASS j) {
    return jass_pushnullhandle(j, "widget");
}
DWORD GetOrderTargetDestructable(LPJASS j) {
    return jass_pushnullhandle(j, "destructable");
}
DWORD GetOrderTargetUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetEventPlayerState(LPJASS j) {
    return jass_pushnullhandle(j, "playerstate");
}
DWORD GetEventPlayerChatString(LPJASS j) {
    return jass_pushstring(j, 0);
}
DWORD GetEventPlayerChatStringMatched(LPJASS j) {
    return jass_pushstring(j, 0);
}
DWORD GetEventUnitState(LPJASS j) {
    return jass_pushnullhandle(j, "unitstate");
}
DWORD GetEventDamage(LPJASS j) {
    return jass_pushnumber(j, 0);
}
DWORD GetEventDetectingPlayer(LPJASS j) {
    return jass_pushnullhandle(j, "player");
}
DWORD GetEventTargetUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
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
DWORD GetFoodMade(LPJASS j) {
    //LONG unitId = jass_checkinteger(j, 1);
    return jass_pushinteger(j, 0);
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
    return jass_pushnullhandle(j, "gamedifficulty");
}
DWORD SetDefaultDifficulty(LPJASS j) {
    //HANDLE g = jass_checkhandle(j, 1, "gamedifficulty");
    return 0;
}
DWORD DialogCreate(LPJASS j) {
    return jass_pushnullhandle(j, "dialog");
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
    return jass_pushnullhandle(j, "button");
}
DWORD DialogDisplay(LPJASS j) {
    //LPPLAYER whichPlayer = jass_checkhandle(j, 1, "player");
    //HANDLE whichDialog = jass_checkhandle(j, 2, "dialog");
    //BOOL flag = jass_checkboolean(j, 3);
    return 0;
}
DWORD InitGameCache(LPJASS j) {
    API_ALLOC(ggamecache_t, gamecache);
    LPCSTR campaignFile = jass_checkstring(j, 1);
    strcpy(gamecache->campaign, campaignFile);
    return 1;
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
//    ggamecache_t const *cache = jass_checkhandle(j, 1, "gamecache");
//    LPCSTR missionKey = jass_checkstring(j, 2);
//    LPCSTR key = jass_checkstring(j, 3);
//    LPCPLAYER forWhichPlayer = jass_checkhandle(j, 4, "player");
//    FLOAT x = jass_checknumber(j, 5);
//    FLOAT y = jass_checknumber(j, 6);
//    FLOAT facing = jass_checknumber(j, 7);
//    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
    return jass_pushnull(j);
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
    return jass_pushnullhandle(j, "unitpool");
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
    //LPMAPPLAYER forWhichPlayer = jass_checkhandle(j, 2, "player");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    //FLOAT facing = jass_checknumber(j, 5);
    return jass_pushnullhandle(j, "unit");
}
DWORD CreateItemPool(LPJASS j) {
    return jass_pushnullhandle(j, "itempool");
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
    return jass_pushnullhandle(j, "item");
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
    BOOL b = jass_checkboolean(j, 1);
    PLAYER_CLIENT(currentplayer)->no_control = !b;
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
    BOOL flag = jass_checkboolean(j, 1);
    FLOAT fadeDuration = jass_checknumber(j, 2);
    LPPLAYER player = currentplayer;
    if (player) {
        UI_ShowInterface(PLAYER_ENT(player), flag, fadeDuration);
    }
    return 0;
}
DWORD PauseGame(LPJASS j) {
    //BOOL flag = jass_checkboolean(j, 1);
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
    return jass_pushnullhandle(j, "trackable");
}
DWORD CreateTimerDialog(LPJASS j) {
    //HANDLE t = jass_checkhandle(j, 1, "timer");
    return jass_pushnullhandle(j, "timerdialog");
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
    LPCSTR speakerTitle = jass_checkstring(j, 3);
    LPCSTR text = jass_checkstring(j, 4);
    //FLOAT sceneDuration = jass_checknumber(j, 5);
    //FLOAT voiceoverDuration = jass_checknumber(j, 6);
    currentplayer->texts[PLAYERTEXT_SPEAKER] = G_LevelString(speakerTitle);
    currentplayer->texts[PLAYERTEXT_DIALOGUE] = G_LevelString(text);
    return 0;
}
DWORD EndCinematicScene(LPJASS j) {
    if (currentplayer) {
        currentplayer->texts[PLAYERTEXT_SPEAKER] = "";
        currentplayer->texts[PLAYERTEXT_DIALOGUE] = "";
    }
    return 0;
}
DWORD NewSoundEnvironment(LPJASS j) {
    //LPCSTR environmentName = jass_checkstring(j, 1);
    return 0;
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
