#ifndef api_h
#define api_h

#include "g_local.h"

#define API_ALLOC(TYPE, NAME) TYPE *NAME = gi.MemAlloc(sizeof(TYPE));

KNOWN_AS(jass_function, JASSFUNC);
KNOWN_AS(jass_type, JASSTYPE);
KNOWN_AS(jass_var, JASSVAR);
KNOWN_AS(jass_module, JASSMODULE);
KNOWN_AS(vm_program, VMPROGRAM);

typedef enum {
    CAMERA_FIELD_TARGET_DISTANCE,
    CAMERA_FIELD_FARZ,
    CAMERA_FIELD_ANGLE_OF_ATTACK,
    CAMERA_FIELD_FIELD_OF_VIEW,
    CAMERA_FIELD_ROLL,
    CAMERA_FIELD_ROTATION,
    CAMERA_FIELD_ZOFFSET,
} CAMERAFIELD;

typedef enum {
    UNIT_STATE_LIFE,
    UNIT_STATE_MAX_LIFE,
    UNIT_STATE_MANA,
    UNIT_STATE_MAX_MANA,
} UNITSTATE;

typedef DWORD (*LPJASSCFUNCTION)(LPJASS);

typedef enum {
    jasstype_integer,
    jasstype_real,
    jasstype_string,
    jasstype_boolean,
    jasstype_code,
    jasstype_handle,
    jasstype_cfunction,
} JASSTYPEID;

struct jass_module {
    LPCSTR name;
    LPJASSCFUNCTION func;
};

typedef struct gtriggeraction_s {
    LPCJASSFUNC func;
    struct gtriggeraction_s *next;
} gtriggeraction_t;

typedef struct {
    gtriggeraction_t *actions;
} gtrigger_t;

typedef struct {
    UINAME campaign;
} ggamecache_t;

typedef struct {
    LPEDICT units[64];
    DWORD num_units;
} ggroup_t;

typedef struct {
    PATHSTR fileName;
    BOOL looping;
    BOOL is3D;
    BOOL stopwhenoutofrange;
    LONG fadeInRate;
    LONG fadeOutRate;
    DWORD duration;
} gsound_t;

struct vm_program {
    HANDLE data;
    DWORD size;
};

LONG jass_checkinteger(LPJASS j, int index);
FLOAT jass_checknumber(LPJASS j, int index);
BOOL jass_checkboolean(LPJASS j, int index);
LPCSTR jass_checkstring(LPJASS j, int index);
LPCJASSFUNC jass_checkcode(LPJASS j, int index);
HANDLE jass_checkhandle(LPJASS j, int index, LPCSTR type);
BOOL jass_toboolean(LPJASS j, int index);
void jass_call(LPJASS j, DWORD args);
JASSTYPEID jass_gettype(LPJASS j, int index);
DWORD jass_pushnull(LPJASS j);
DWORD jass_pushinteger(LPJASS j, LONG value);
DWORD jass_pushhandle(LPJASS j, HANDLE value, LPCSTR type);
DWORD jass_pushlighthandle(LPJASS j, HANDLE value, LPCSTR type);
DWORD jass_pushnumber(LPJASS j, FLOAT value);
DWORD jass_pushboolean(LPJASS j, BOOL value);
DWORD jass_pushstring(LPJASS j, LPCSTR value);
DWORD jass_pushstringlen(LPJASS j, LPCSTR value, DWORD len);
DWORD jass_pushfunction(LPJASS j, LPCJASSFUNC func);

DWORD ConvertRace(LPJASS j);
DWORD ConvertAllianceType(LPJASS j);
DWORD ConvertRacePref(LPJASS j);
DWORD ConvertIGameState(LPJASS j);
DWORD ConvertFGameState(LPJASS j);
DWORD ConvertPlayerState(LPJASS j);
DWORD ConvertPlayerGameResult(LPJASS j);
DWORD ConvertUnitState(LPJASS j);
DWORD ConvertGameEvent(LPJASS j);
DWORD ConvertPlayerEvent(LPJASS j);
DWORD ConvertPlayerUnitEvent(LPJASS j);
DWORD ConvertWidgetEvent(LPJASS j);
DWORD ConvertDialogEvent(LPJASS j);
DWORD ConvertUnitEvent(LPJASS j);
DWORD ConvertLimitOp(LPJASS j);
DWORD ConvertUnitType(LPJASS j);
DWORD ConvertGameSpeed(LPJASS j);
DWORD ConvertPlacement(LPJASS j);
DWORD ConvertStartLocPrio(LPJASS j);
DWORD ConvertGameDifficulty(LPJASS j);
DWORD ConvertGameType(LPJASS j);
DWORD ConvertMapFlag(LPJASS j);
DWORD ConvertMapVisibility(LPJASS j);
DWORD ConvertMapSetting(LPJASS j);
DWORD ConvertMapDensity(LPJASS j);
DWORD ConvertMapControl(LPJASS j);
DWORD ConvertPlayerColor(LPJASS j);
DWORD ConvertPlayerSlotState(LPJASS j);
DWORD ConvertVolumeGroup(LPJASS j);
DWORD ConvertCameraField(LPJASS j);
DWORD ConvertBlendMode(LPJASS j);
DWORD ConvertRarityControl(LPJASS j);
DWORD ConvertTexMapFlags(LPJASS j);
DWORD ConvertFogState(LPJASS j);
DWORD ConvertEffectType(LPJASS j);
DWORD OrderId(LPJASS j);
DWORD OrderId2String(LPJASS j);
DWORD UnitId(LPJASS j);
DWORD UnitId2String(LPJASS j);
DWORD AbilityId(LPJASS j);
DWORD AbilityId2String(LPJASS j);
DWORD Deg2Rad(LPJASS j);
DWORD Rad2Deg(LPJASS j);
DWORD Sin(LPJASS j);
DWORD Cos(LPJASS j);
DWORD Tan(LPJASS j);
DWORD Asin(LPJASS j);
DWORD Acos(LPJASS j);
DWORD Atan(LPJASS j);
DWORD Atan2(LPJASS j);
DWORD SquareRoot(LPJASS j);
DWORD Pow(LPJASS j);
DWORD I2R(LPJASS j);
DWORD R2I(LPJASS j);
DWORD I2S(LPJASS j);
DWORD R2S(LPJASS j);
DWORD R2SW(LPJASS j);
DWORD S2I(LPJASS j);
DWORD S2R(LPJASS j);
DWORD SubString(LPJASS j);
DWORD GetLocalizedString(LPJASS j);
DWORD GetLocalizedHotkey(LPJASS j);
DWORD SetMapName(LPJASS j);
DWORD SetMapDescription(LPJASS j);
DWORD SetTeams(LPJASS j);
DWORD SetPlayers(LPJASS j);
DWORD DefineStartLocation(LPJASS j);
DWORD DefineStartLocationLoc(LPJASS j);
DWORD SetStartLocPrioCount(LPJASS j);
DWORD SetStartLocPrio(LPJASS j);
DWORD GetStartLocPrioSlot(LPJASS j);
DWORD GetStartLocPrio(LPJASS j);
DWORD SetGameTypeSupported(LPJASS j);
DWORD SetMapFlag(LPJASS j);
DWORD SetGamePlacement(LPJASS j);
DWORD SetGameSpeed(LPJASS j);
DWORD SetGameDifficulty(LPJASS j);
DWORD SetResourceDensity(LPJASS j);
DWORD SetCreatureDensity(LPJASS j);
DWORD GetTeams(LPJASS j);
DWORD GetPlayers(LPJASS j);
DWORD IsGameTypeSupported(LPJASS j);
DWORD GetGameTypeSelected(LPJASS j);
DWORD IsMapFlagSet(LPJASS j);
DWORD GetGamePlacement(LPJASS j);
DWORD GetGameSpeed(LPJASS j);
DWORD GetGameDifficulty(LPJASS j);
DWORD GetResourceDensity(LPJASS j);
DWORD GetCreatureDensity(LPJASS j);
DWORD GetStartLocationX(LPJASS j);
DWORD GetStartLocationY(LPJASS j);
DWORD GetStartLocationLoc(LPJASS j);
DWORD SetPlayerTeam(LPJASS j);
DWORD SetPlayerStartLocation(LPJASS j);
DWORD ForcePlayerStartLocation(LPJASS j);
DWORD SetPlayerColor(LPJASS j);
DWORD SetPlayerAlliance(LPJASS j);
DWORD SetPlayerTaxRate(LPJASS j);
DWORD SetPlayerRacePreference(LPJASS j);
DWORD SetPlayerRaceSelectable(LPJASS j);
DWORD SetPlayerController(LPJASS j);
DWORD SetPlayerOnScoreScreen(LPJASS j);
DWORD GetPlayerTeam(LPJASS j);
DWORD GetPlayerStartLocation(LPJASS j);
DWORD GetPlayerColor(LPJASS j);
DWORD GetPlayerSelectable(LPJASS j);
DWORD GetPlayerController(LPJASS j);
DWORD GetPlayerSlotState(LPJASS j);
DWORD GetPlayerTaxRate(LPJASS j);
DWORD IsPlayerRacePrefSet(LPJASS j);
DWORD GetPlayerName(LPJASS j);
DWORD CreateTimer(LPJASS j);
DWORD DestroyTimer(LPJASS j);
DWORD TimerStart(LPJASS j);
DWORD TimerGetElapsed(LPJASS j);
DWORD TimerGetRemaining(LPJASS j);
DWORD TimerGetTimeout(LPJASS j);
DWORD PauseTimer(LPJASS j);
DWORD ResumeTimer(LPJASS j);
DWORD GetExpiredTimer(LPJASS j);
    DWORD CreateGroup(LPJASS j);
    DWORD GroupAddUnit(LPJASS j);
    DWORD GroupRemoveUnit(LPJASS j);
    DWORD GroupClear(LPJASS j);
DWORD GroupEnumUnitsOfType(LPJASS j);
DWORD GroupEnumUnitsOfPlayer(LPJASS j);
DWORD GroupEnumUnitsOfTypeCounted(LPJASS j);
DWORD GroupEnumUnitsInRect(LPJASS j);
DWORD GroupEnumUnitsInRectCounted(LPJASS j);
DWORD GroupEnumUnitsInRange(LPJASS j);
DWORD GroupEnumUnitsInRangeOfLoc(LPJASS j);
DWORD GroupEnumUnitsInRangeCounted(LPJASS j);
DWORD GroupEnumUnitsInRangeOfLocCounted(LPJASS j);
DWORD GroupEnumUnitsSelected(LPJASS j);
DWORD GroupImmediateOrder(LPJASS j);
DWORD GroupImmediateOrderById(LPJASS j);
DWORD GroupPointOrder(LPJASS j);
DWORD GroupPointOrderLoc(LPJASS j);
DWORD GroupPointOrderById(LPJASS j);
DWORD GroupPointOrderByIdLoc(LPJASS j);
DWORD GroupTargetOrder(LPJASS j);
DWORD GroupTargetOrderById(LPJASS j);
DWORD ForGroup(LPJASS j);
DWORD FirstOfGroup(LPJASS j);
DWORD CreateForce(LPJASS j);
DWORD ForceAddPlayer(LPJASS j);
DWORD ForceRemovePlayer(LPJASS j);
DWORD ForceClear(LPJASS j);
DWORD ForceEnumPlayers(LPJASS j);
DWORD ForceEnumPlayersCounted(LPJASS j);
DWORD ForceEnumAllies(LPJASS j);
DWORD ForceEnumEnemies(LPJASS j);
DWORD ForForce(LPJASS j);
    DWORD Rect(LPJASS j);
    DWORD RectFromLoc(LPJASS j);
DWORD RemoveRect(LPJASS j);
    DWORD SetRect(LPJASS j);
    DWORD SetRectFromLoc(LPJASS j);
    DWORD MoveRectTo(LPJASS j);
    DWORD MoveRectToLoc(LPJASS j);
    DWORD GetRectCenterX(LPJASS j);
    DWORD GetRectCenterY(LPJASS j);
    DWORD GetRectMinX(LPJASS j);
    DWORD GetRectMinY(LPJASS j);
    DWORD GetRectMaxX(LPJASS j);
    DWORD GetRectMaxY(LPJASS j);
DWORD CreateRegion(LPJASS j);
DWORD RemoveRegion(LPJASS j);
DWORD RegionAddRect(LPJASS j);
DWORD RegionClearRect(LPJASS j);
DWORD RegionAddCell(LPJASS j);
DWORD RegionAddCellAtLoc(LPJASS j);
DWORD RegionClearCell(LPJASS j);
DWORD RegionClearCellAtLoc(LPJASS j);
    DWORD Location(LPJASS j);
DWORD RemoveLocation(LPJASS j);
    DWORD MoveLocation(LPJASS j);
    DWORD GetLocationX(LPJASS j);
    DWORD GetLocationY(LPJASS j);
DWORD IsUnitInRegion(LPJASS j);
DWORD IsPointInRegion(LPJASS j);
DWORD IsLocationInRegion(LPJASS j);
DWORD GetWorldBounds(LPJASS j);
    DWORD CreateTrigger(LPJASS j);
DWORD DestroyTrigger(LPJASS j);
DWORD ResetTrigger(LPJASS j);
DWORD EnableTrigger(LPJASS j);
DWORD DisableTrigger(LPJASS j);
DWORD IsTriggerEnabled(LPJASS j);
DWORD TriggerWaitOnSleeps(LPJASS j);
DWORD IsTriggerWaitOnSleeps(LPJASS j);
DWORD GetFilterUnit(LPJASS j);
DWORD GetEnumUnit(LPJASS j);
DWORD GetFilterDestructable(LPJASS j);
DWORD GetEnumDestructable(LPJASS j);
DWORD GetFilterPlayer(LPJASS j);
DWORD GetEnumPlayer(LPJASS j);
DWORD GetTriggeringTrigger(LPJASS j);
DWORD GetTriggerEventId(LPJASS j);
DWORD GetTriggerEvalCount(LPJASS j);
DWORD GetTriggerExecCount(LPJASS j);
DWORD ExecuteFunc(LPJASS j);
DWORD And(LPJASS j);
DWORD Or(LPJASS j);
DWORD Not(LPJASS j);
DWORD Condition(LPJASS j);
DWORD DestroyCondition(LPJASS j);
DWORD Filter(LPJASS j);
DWORD DestroyFilter(LPJASS j);
DWORD DestroyBoolExpr(LPJASS j);
DWORD TriggerRegisterVariableEvent(LPJASS j);
DWORD TriggerRegisterTimerEvent(LPJASS j);
DWORD TriggerRegisterTimerExpireEvent(LPJASS j);
DWORD TriggerRegisterGameStateEvent(LPJASS j);
DWORD TriggerRegisterDialogEvent(LPJASS j);
DWORD TriggerRegisterDialogButtonEvent(LPJASS j);
DWORD GetEventGameState(LPJASS j);
DWORD TriggerRegisterGameEvent(LPJASS j);
DWORD GetWinningPlayer(LPJASS j);
DWORD TriggerRegisterEnterRegion(LPJASS j);
DWORD GetTriggeringRegion(LPJASS j);
DWORD GetEnteringUnit(LPJASS j);
DWORD TriggerRegisterLeaveRegion(LPJASS j);
DWORD GetLeavingUnit(LPJASS j);
DWORD TriggerRegisterTrackableHitEvent(LPJASS j);
DWORD TriggerRegisterTrackableTrackEvent(LPJASS j);
DWORD GetTriggeringTrackable(LPJASS j);
DWORD GetClickedButton(LPJASS j);
DWORD GetClickedDialog(LPJASS j);
DWORD TriggerRegisterPlayerEvent(LPJASS j);
DWORD GetTriggerPlayer(LPJASS j);
DWORD TriggerRegisterPlayerUnitEvent(LPJASS j);
DWORD GetLevelingUnit(LPJASS j);
DWORD GetLearningUnit(LPJASS j);
DWORD GetLearnedSkill(LPJASS j);
DWORD GetLearnedSkillLevel(LPJASS j);
DWORD GetRevivableUnit(LPJASS j);
DWORD GetRevivingUnit(LPJASS j);
DWORD GetAttacker(LPJASS j);
DWORD GetRescuer(LPJASS j);
DWORD GetDyingUnit(LPJASS j);
DWORD GetKillingUnit(LPJASS j);
DWORD GetDecayingUnit(LPJASS j);
DWORD GetConstructingStructure(LPJASS j);
DWORD GetCancelledStructure(LPJASS j);
DWORD GetConstructedStructure(LPJASS j);
DWORD GetResearchingUnit(LPJASS j);
DWORD GetResearched(LPJASS j);
DWORD GetTrainedUnitType(LPJASS j);
DWORD GetTrainedUnit(LPJASS j);
DWORD GetDetectedUnit(LPJASS j);
DWORD GetSummoningUnit(LPJASS j);
DWORD GetSummonedUnit(LPJASS j);
DWORD GetTransportUnit(LPJASS j);
DWORD GetLoadedUnit(LPJASS j);
DWORD GetManipulatingUnit(LPJASS j);
DWORD GetManipulatedItem(LPJASS j);
DWORD GetOrderedUnit(LPJASS j);
DWORD GetIssuedOrderId(LPJASS j);
DWORD GetOrderPointX(LPJASS j);
DWORD GetOrderPointY(LPJASS j);
DWORD GetOrderPointLoc(LPJASS j);
DWORD GetOrderTarget(LPJASS j);
DWORD GetOrderTargetDestructable(LPJASS j);
DWORD GetOrderTargetItem(LPJASS j);
DWORD GetOrderTargetUnit(LPJASS j);
DWORD TriggerRegisterPlayerAllianceChange(LPJASS j);
DWORD TriggerRegisterPlayerStateEvent(LPJASS j);
DWORD GetEventPlayerState(LPJASS j);
DWORD TriggerRegisterPlayerChatEvent(LPJASS j);
DWORD GetEventPlayerChatString(LPJASS j);
DWORD GetEventPlayerChatStringMatched(LPJASS j);
DWORD TriggerRegisterDeathEvent(LPJASS j);
DWORD GetTriggerUnit(LPJASS j);
DWORD TriggerRegisterUnitStateEvent(LPJASS j);
DWORD GetEventUnitState(LPJASS j);
DWORD TriggerRegisterUnitEvent(LPJASS j);
DWORD GetEventDamage(LPJASS j);
DWORD GetEventDetectingPlayer(LPJASS j);
DWORD TriggerRegisterFilterUnitEvent(LPJASS j);
DWORD GetEventTargetUnit(LPJASS j);
DWORD TriggerRegisterUnitInRange(LPJASS j);
DWORD TriggerAddCondition(LPJASS j);
DWORD TriggerRemoveCondition(LPJASS j);
DWORD TriggerClearConditions(LPJASS j);
    DWORD TriggerAddAction(LPJASS j);
DWORD TriggerRemoveAction(LPJASS j);
DWORD TriggerClearActions(LPJASS j);
DWORD TriggerSleepAction(LPJASS j);
DWORD TriggerWaitForSound(LPJASS j);
    DWORD TriggerEvaluate(LPJASS j);
    DWORD TriggerExecute(LPJASS j);
DWORD TriggerExecuteWait(LPJASS j);
DWORD GetWidgetLife(LPJASS j);
DWORD SetWidgetLife(LPJASS j);
DWORD GetWidgetX(LPJASS j);
DWORD GetWidgetY(LPJASS j);
DWORD GetTriggerWidget(LPJASS j);
DWORD CreateDestructable(LPJASS j);
DWORD CreateDestructableZ(LPJASS j);
DWORD CreateDeadDestructable(LPJASS j);
DWORD CreateDeadDestructableZ(LPJASS j);
DWORD RemoveDestructable(LPJASS j);
DWORD KillDestructable(LPJASS j);
DWORD SetDestructableInvulnerable(LPJASS j);
DWORD IsDestructableInvulnerable(LPJASS j);
DWORD EnumDestructablesInRect(LPJASS j);
DWORD GetDestructableTypeId(LPJASS j);
DWORD GetDestructableX(LPJASS j);
DWORD GetDestructableY(LPJASS j);
DWORD SetDestructableLife(LPJASS j);
DWORD GetDestructableLife(LPJASS j);
DWORD SetDestructableMaxLife(LPJASS j);
DWORD GetDestructableMaxLife(LPJASS j);
DWORD DestructableRestoreLife(LPJASS j);
DWORD QueueDestructableAnimation(LPJASS j);
DWORD SetDestructableAnimation(LPJASS j);
DWORD CreateItem(LPJASS j);
DWORD RemoveItem(LPJASS j);
DWORD GetItemPlayer(LPJASS j);
DWORD GetItemTypeId(LPJASS j);
DWORD GetItemX(LPJASS j);
DWORD GetItemY(LPJASS j);
DWORD SetItemPosition(LPJASS j);
DWORD SetItemDropOnDeath(LPJASS j);
DWORD SetItemDroppable(LPJASS j);
DWORD SetItemPlayer(LPJASS j);
DWORD SetItemInvulnerable(LPJASS j);
DWORD IsItemInvulnerable(LPJASS j);
DWORD CreateUnit(LPJASS j);
DWORD CreateUnitByName(LPJASS j);
    DWORD CreateUnitAtLoc(LPJASS j);
DWORD CreateUnitAtLocByName(LPJASS j);
DWORD CreateCorpse(LPJASS j);
DWORD KillUnit(LPJASS j);
DWORD RemoveUnit(LPJASS j);
DWORD ShowUnit(LPJASS j);
DWORD SetUnitState(LPJASS j);
DWORD SetUnitX(LPJASS j);
DWORD SetUnitY(LPJASS j);
DWORD SetUnitPosition(LPJASS j);
DWORD SetUnitPositionLoc(LPJASS j);
DWORD SetUnitFacing(LPJASS j);
DWORD SetUnitFacingTimed(LPJASS j);
    DWORD SetUnitMoveSpeed(LPJASS j);
    DWORD SetUnitFlyHeight(LPJASS j);
    DWORD SetUnitTurnSpeed(LPJASS j);
    DWORD SetUnitPropWindow(LPJASS j);
    DWORD SetUnitAcquireRange(LPJASS j);
    DWORD GetUnitAcquireRange(LPJASS j);
    DWORD GetUnitTurnSpeed(LPJASS j);
    DWORD GetUnitPropWindow(LPJASS j);
    DWORD GetUnitFlyHeight(LPJASS j);
DWORD GetUnitDefaultAcquireRange(LPJASS j);
DWORD GetUnitDefaultTurnSpeed(LPJASS j);
DWORD GetUnitDefaultPropWindow(LPJASS j);
DWORD GetUnitDefaultFlyHeight(LPJASS j);
DWORD SetUnitOwner(LPJASS j);
DWORD SetUnitColor(LPJASS j);
DWORD SetUnitScale(LPJASS j);
DWORD SetUnitTimeScale(LPJASS j);
DWORD SetUnitBlendTime(LPJASS j);
DWORD SetUnitVertexColor(LPJASS j);
DWORD QueueUnitAnimation(LPJASS j);
DWORD SetUnitAnimation(LPJASS j);
DWORD SetUnitAnimationByIndex(LPJASS j);
DWORD SetUnitAnimationWithRarity(LPJASS j);
DWORD AddUnitAnimationProperties(LPJASS j);
DWORD SetUnitLookAt(LPJASS j);
DWORD ResetUnitLookAt(LPJASS j);
DWORD SetUnitRescuable(LPJASS j);
DWORD SetUnitRescueRange(LPJASS j);
DWORD SetHeroStr(LPJASS j);
DWORD SetHeroAgi(LPJASS j);
DWORD SetHeroInt(LPJASS j);
DWORD GetHeroXP(LPJASS j);
DWORD SetHeroXP(LPJASS j);
DWORD AddHeroXP(LPJASS j);
    DWORD SetHeroLevel(LPJASS j);
    DWORD GetHeroLevel(LPJASS j);
DWORD SuspendHeroXP(LPJASS j);
DWORD IsSuspendedXP(LPJASS j);
DWORD SelectHeroSkill(LPJASS j);
DWORD ReviveHero(LPJASS j);
DWORD ReviveHeroLoc(LPJASS j);
DWORD SetUnitExploded(LPJASS j);
DWORD SetUnitInvulnerable(LPJASS j);
DWORD PauseUnit(LPJASS j);
DWORD IsUnitPaused(LPJASS j);
DWORD SetUnitPathing(LPJASS j);
DWORD ClearSelection(LPJASS j);
DWORD SelectUnit(LPJASS j);
DWORD GetUnitPointValue(LPJASS j);
DWORD GetUnitPointValueByType(LPJASS j);
DWORD UnitAddItem(LPJASS j);
DWORD UnitAddItemById(LPJASS j);
DWORD UnitAddItemToSlotById(LPJASS j);
DWORD UnitRemoveItem(LPJASS j);
DWORD UnitRemoveItemFromSlot(LPJASS j);
DWORD UnitHasItem(LPJASS j);
DWORD UnitItemInSlot(LPJASS j);
DWORD UnitUseItem(LPJASS j);
DWORD UnitUseItemPoint(LPJASS j);
DWORD UnitUseItemTarget(LPJASS j);
DWORD GetUnitX(LPJASS j);
DWORD GetUnitY(LPJASS j);
DWORD GetUnitLoc(LPJASS j);
DWORD GetUnitFacing(LPJASS j);
DWORD GetUnitMoveSpeed(LPJASS j);
DWORD GetUnitDefaultMoveSpeed(LPJASS j);
DWORD GetUnitState(LPJASS j);
DWORD GetOwningPlayer(LPJASS j);
DWORD GetUnitTypeId(LPJASS j);
DWORD GetUnitRace(LPJASS j);
DWORD GetUnitName(LPJASS j);
DWORD GetUnitFoodUsed(LPJASS j);
DWORD GetUnitFoodMade(LPJASS j);
DWORD GetFoodMade(LPJASS j);
DWORD IsUnitInGroup(LPJASS j);
DWORD IsUnitInForce(LPJASS j);
DWORD IsUnitOwnedByPlayer(LPJASS j);
DWORD IsUnitAlly(LPJASS j);
DWORD IsUnitEnemy(LPJASS j);
DWORD IsUnitVisible(LPJASS j);
DWORD IsUnitDetected(LPJASS j);
DWORD IsUnitInvisible(LPJASS j);
DWORD IsUnitFogged(LPJASS j);
DWORD IsUnitMasked(LPJASS j);
DWORD IsUnitSelected(LPJASS j);
DWORD IsUnitRace(LPJASS j);
DWORD IsUnitType(LPJASS j);
DWORD IsUnit(LPJASS j);
DWORD IsUnitInRange(LPJASS j);
DWORD IsUnitInRangeXY(LPJASS j);
DWORD IsUnitInRangeLoc(LPJASS j);
DWORD IsUnitHidden(LPJASS j);
DWORD IsUnitIllusion(LPJASS j);
DWORD IsUnitInTransport(LPJASS j);
DWORD IsUnitLoaded(LPJASS j);
DWORD IsHeroUnitId(LPJASS j);
DWORD UnitShareVision(LPJASS j);
DWORD UnitSuspendDecay(LPJASS j);
DWORD UnitRemoveAbility(LPJASS j);
DWORD UnitRemoveBuffs(LPJASS j);
DWORD UnitAddSleep(LPJASS j);
DWORD UnitCanSleep(LPJASS j);
DWORD UnitAddSleepPerm(LPJASS j);
DWORD UnitCanSleepPerm(LPJASS j);
DWORD UnitIsSleeping(LPJASS j);
DWORD UnitWakeUp(LPJASS j);
DWORD UnitApplyTimedLife(LPJASS j);
DWORD IssueImmediateOrder(LPJASS j);
DWORD IssueImmediateOrderById(LPJASS j);
    DWORD IssuePointOrder(LPJASS j);
    DWORD IssuePointOrderLoc(LPJASS j);
DWORD IssuePointOrderById(LPJASS j);
DWORD IssuePointOrderByIdLoc(LPJASS j);
DWORD IssueTargetOrder(LPJASS j);
DWORD IssueTargetOrderById(LPJASS j);
DWORD IssueInstantTargetOrder(LPJASS j);
DWORD IssueInstantTargetOrderById(LPJASS j);
DWORD IssueBuildOrder(LPJASS j);
DWORD IssueBuildOrderById(LPJASS j);
DWORD IssueNeutralImmediateOrder(LPJASS j);
DWORD IssueNeutralImmediateOrderById(LPJASS j);
DWORD IssueNeutralPointOrder(LPJASS j);
DWORD IssueNeutralPointOrderById(LPJASS j);
DWORD IssueNeutralTargetOrder(LPJASS j);
DWORD IssueNeutralTargetOrderById(LPJASS j);
    DWORD SetResourceAmount(LPJASS j);
    DWORD AddResourceAmount(LPJASS j);
    DWORD GetResourceAmount(LPJASS j);
DWORD WaygateGetDestinationX(LPJASS j);
DWORD WaygateGetDestinationY(LPJASS j);
DWORD WaygateSetDestination(LPJASS j);
DWORD WaygateActivate(LPJASS j);
DWORD WaygateIsActive(LPJASS j);
    DWORD Player(LPJASS j);
DWORD GetLocalPlayer(LPJASS j);
DWORD IsPlayerAlly(LPJASS j);
DWORD IsPlayerEnemy(LPJASS j);
DWORD IsPlayerInForce(LPJASS j);
DWORD IsPlayerObserver(LPJASS j);
DWORD IsVisibleToPlayer(LPJASS j);
DWORD IsLocationVisibleToPlayer(LPJASS j);
DWORD IsFoggedToPlayer(LPJASS j);
DWORD IsLocationFoggedToPlayer(LPJASS j);
DWORD IsMaskedToPlayer(LPJASS j);
DWORD IsLocationMaskedToPlayer(LPJASS j);
DWORD GetPlayerRace(LPJASS j);
DWORD GetPlayerId(LPJASS j);
DWORD GetPlayerUnitCount(LPJASS j);
DWORD GetPlayerTypedUnitCount(LPJASS j);
DWORD GetPlayerStructureCount(LPJASS j);
DWORD GetPlayerState(LPJASS j);
DWORD GetPlayerAlliance(LPJASS j);
DWORD GetPlayerHandicap(LPJASS j);
DWORD GetPlayerHandicapXP(LPJASS j);
DWORD SetPlayerHandicap(LPJASS j);
DWORD SetPlayerHandicapXP(LPJASS j);
DWORD SetPlayerTechMaxAllowed(LPJASS j);
DWORD GetPlayerTechMaxAllowed(LPJASS j);
DWORD AddPlayerTechResearched(LPJASS j);
DWORD SetPlayerTechResearched(LPJASS j);
DWORD GetPlayerTechResearched(LPJASS j);
DWORD GetPlayerTechCount(LPJASS j);
DWORD SetPlayerAbilityAvailable(LPJASS j);
DWORD SetPlayerState(LPJASS j);
DWORD RemovePlayer(LPJASS j);
DWORD CachePlayerHeroData(LPJASS j);
DWORD SetFogStateRect(LPJASS j);
DWORD SetFogStateRadius(LPJASS j);
DWORD SetFogStateRadiusLoc(LPJASS j);
DWORD FogMaskEnable(LPJASS j);
DWORD IsFogMaskEnabled(LPJASS j);
DWORD FogEnable(LPJASS j);
DWORD IsFogEnabled(LPJASS j);
DWORD CreateFogModifierRect(LPJASS j);
DWORD CreateFogModifierRadius(LPJASS j);
DWORD CreateFogModifierRadiusLoc(LPJASS j);
DWORD DestroyFogModifier(LPJASS j);
DWORD FogModifierStart(LPJASS j);
DWORD FogModifierStop(LPJASS j);
DWORD EndGame(LPJASS j);
DWORD ChangeLevel(LPJASS j);
DWORD RestartGame(LPJASS j);
DWORD ReloadGame(LPJASS j);
DWORD SetCampaignMenuRace(LPJASS j);
DWORD ForceCampaignSelectScreen(LPJASS j);
DWORD SyncSelections(LPJASS j);
DWORD SetFloatGameState(LPJASS j);
DWORD GetFloatGameState(LPJASS j);
DWORD SetIntegerGameState(LPJASS j);
DWORD GetIntegerGameState(LPJASS j);
DWORD SetTutorialCleared(LPJASS j);
DWORD SetMissionAvailable(LPJASS j);
DWORD SetCampaignAvailable(LPJASS j);
DWORD SetOpCinematicAvailable(LPJASS j);
DWORD SetEdCinematicAvailable(LPJASS j);
DWORD GetDefaultDifficulty(LPJASS j);
DWORD SetDefaultDifficulty(LPJASS j);
DWORD DialogCreate(LPJASS j);
DWORD DialogDestroy(LPJASS j);
DWORD DialogSetAsync(LPJASS j);
DWORD DialogClear(LPJASS j);
DWORD DialogSetMessage(LPJASS j);
DWORD DialogAddButton(LPJASS j);
DWORD DialogDisplay(LPJASS j);
DWORD InitGameCache(LPJASS j);
DWORD SaveGameCache(LPJASS j);
DWORD StoreInteger(LPJASS j);
DWORD StoreReal(LPJASS j);
DWORD StoreBoolean(LPJASS j);
DWORD StoreUnit(LPJASS j);
DWORD SyncStoredInteger(LPJASS j);
DWORD SyncStoredReal(LPJASS j);
DWORD SyncStoredBoolean(LPJASS j);
DWORD SyncStoredUnit(LPJASS j);
DWORD GetStoredInteger(LPJASS j);
DWORD GetStoredReal(LPJASS j);
DWORD GetStoredBoolean(LPJASS j);
DWORD RestoreUnit(LPJASS j);
DWORD GetRandomInt(LPJASS j);
DWORD GetRandomReal(LPJASS j);
DWORD CreateUnitPool(LPJASS j);
DWORD DestroyUnitPool(LPJASS j);
DWORD UnitPoolAddUnitType(LPJASS j);
DWORD UnitPoolRemoveUnitType(LPJASS j);
DWORD PlaceRandomUnit(LPJASS j);
DWORD CreateItemPool(LPJASS j);
DWORD DestroyItemPool(LPJASS j);
DWORD ItemPoolAddItemType(LPJASS j);
DWORD ItemPoolRemoveItemType(LPJASS j);
DWORD PlaceRandomItem(LPJASS j);
DWORD ChooseRandomCreep(LPJASS j);
DWORD ChooseRandomNPBuilding(LPJASS j);
DWORD ChooseRandomItem(LPJASS j);
DWORD SetRandomSeed(LPJASS j);
DWORD SetTerrainFog(LPJASS j);
DWORD ResetTerrainFog(LPJASS j);
DWORD SetUnitFog(LPJASS j);
DWORD SetTerrainFogEx(LPJASS j);
DWORD DisplayTextToPlayer(LPJASS j);
DWORD DisplayTimedTextToPlayer(LPJASS j);
DWORD DisplayTimedTextFromPlayer(LPJASS j);
DWORD ClearTextMessages(LPJASS j);
DWORD SetDayNightModels(LPJASS j);
DWORD SetSkyModel(LPJASS j);
DWORD EnableUserControl(LPJASS j);
DWORD SuspendTimeOfDay(LPJASS j);
DWORD SetTimeOfDayScale(LPJASS j);
DWORD GetTimeOfDayScale(LPJASS j);
DWORD ShowInterface(LPJASS j);
DWORD PauseGame(LPJASS j);
DWORD UnitAddIndicator(LPJASS j);
DWORD AddIndicator(LPJASS j);
DWORD PingMinimap(LPJASS j);
DWORD EnableOcclusion(LPJASS j);
DWORD SetIntroShotText(LPJASS j);
DWORD SetIntroShotModel(LPJASS j);
DWORD EnableWorldFogBoundary(LPJASS j);
DWORD PlayCinematic(LPJASS j);
DWORD ForceUIKey(LPJASS j);
DWORD ForceUICancel(LPJASS j);
DWORD DisplayLoadDialog(LPJASS j);
DWORD CreateTrackable(LPJASS j);
DWORD CreateQuest(LPJASS j);
DWORD DestroyQuest(LPJASS j);
DWORD QuestSetTitle(LPJASS j);
DWORD QuestSetDescription(LPJASS j);
DWORD QuestSetIconPath(LPJASS j);
DWORD QuestSetRequired(LPJASS j);
DWORD QuestSetCompleted(LPJASS j);
DWORD QuestSetDiscovered(LPJASS j);
DWORD QuestSetFailed(LPJASS j);
DWORD QuestSetEnabled(LPJASS j);
DWORD IsQuestRequired(LPJASS j);
DWORD IsQuestCompleted(LPJASS j);
DWORD IsQuestDiscovered(LPJASS j);
DWORD IsQuestFailed(LPJASS j);
DWORD IsQuestEnabled(LPJASS j);
DWORD QuestCreateItem(LPJASS j);
DWORD QuestItemSetDescription(LPJASS j);
DWORD QuestItemSetCompleted(LPJASS j);
DWORD IsQuestItemCompleted(LPJASS j);
DWORD CreateDefeatCondition(LPJASS j);
DWORD DestroyDefeatCondition(LPJASS j);
DWORD DefeatConditionSetDescription(LPJASS j);
DWORD FlashQuestDialogButton(LPJASS j);
DWORD ForceQuestDialogUpdate(LPJASS j);
DWORD CreateTimerDialog(LPJASS j);
DWORD DestroyTimerDialog(LPJASS j);
DWORD TimerDialogSetTitle(LPJASS j);
DWORD TimerDialogSetTitleColor(LPJASS j);
DWORD TimerDialogSetTimeColor(LPJASS j);
DWORD TimerDialogSetSpeed(LPJASS j);
DWORD TimerDialogDisplay(LPJASS j);
DWORD IsTimerDialogDisplayed(LPJASS j);
DWORD CreateLeaderboard(LPJASS j);
DWORD DestroyLeaderboard(LPJASS j);
DWORD LeaderboardDisplay(LPJASS j);
DWORD IsLeaderboardDisplayed(LPJASS j);
DWORD LeaderboardGetItemCount(LPJASS j);
DWORD LeaderboardSetSizeByItemCount(LPJASS j);
DWORD LeaderboardAddItem(LPJASS j);
DWORD LeaderboardRemoveItem(LPJASS j);
DWORD LeaderboardRemovePlayerItem(LPJASS j);
DWORD LeaderboardClear(LPJASS j);
DWORD LeaderboardSortItemsByValue(LPJASS j);
DWORD LeaderboardSortItemsByPlayer(LPJASS j);
DWORD LeaderboardSortItemsByLabel(LPJASS j);
DWORD LeaderboardHasPlayerItem(LPJASS j);
DWORD LeaderboardGetPlayerIndex(LPJASS j);
DWORD LeaderboardSetLabel(LPJASS j);
DWORD LeaderboardGetLabelText(LPJASS j);
DWORD PlayerSetLeaderboard(LPJASS j);
DWORD PlayerGetLeaderboard(LPJASS j);
DWORD LeaderboardSetLabelColor(LPJASS j);
DWORD LeaderboardSetValueColor(LPJASS j);
DWORD LeaderboardSetStyle(LPJASS j);
DWORD LeaderboardSetItemValue(LPJASS j);
DWORD LeaderboardSetItemLabel(LPJASS j);
DWORD LeaderboardSetItemStyle(LPJASS j);
DWORD LeaderboardSetItemLabelColor(LPJASS j);
DWORD LeaderboardSetItemValueColor(LPJASS j);
DWORD SetCameraPosition(LPJASS j);
DWORD SetCameraQuickPosition(LPJASS j);
DWORD SetCameraBounds(LPJASS j);
DWORD StopCamera(LPJASS j);
DWORD ResetToGameCamera(LPJASS j);
DWORD PanCameraTo(LPJASS j);
DWORD PanCameraToTimed(LPJASS j);
DWORD PanCameraToWithZ(LPJASS j);
DWORD PanCameraToTimedWithZ(LPJASS j);
DWORD SetCinematicCamera(LPJASS j);
DWORD SetCameraField(LPJASS j);
DWORD AdjustCameraField(LPJASS j);
DWORD SetCameraTargetController(LPJASS j);
DWORD SetCameraOrientController(LPJASS j);
DWORD CreateCameraSetup(LPJASS j);
DWORD CameraSetupSetField(LPJASS j);
DWORD CameraSetupGetField(LPJASS j);
DWORD CameraSetupSetDestPosition(LPJASS j);
DWORD CameraSetupGetDestPositionLoc(LPJASS j);
DWORD CameraSetupGetDestPositionX(LPJASS j);
DWORD CameraSetupGetDestPositionY(LPJASS j);
DWORD CameraSetupApply(LPJASS j);
DWORD CameraSetupApplyWithZ(LPJASS j);
DWORD CameraSetupApplyForceDuration(LPJASS j);
DWORD CameraSetupApplyForceDurationWithZ(LPJASS j);
DWORD CameraSetTargetNoise(LPJASS j);
DWORD CameraSetSourceNoise(LPJASS j);
DWORD CameraSetSmoothingFactor(LPJASS j);
DWORD SetCineFilterTexture(LPJASS j);
DWORD SetCineFilterBlendMode(LPJASS j);
DWORD SetCineFilterTexMapFlags(LPJASS j);
DWORD SetCineFilterStartUV(LPJASS j);
DWORD SetCineFilterEndUV(LPJASS j);
DWORD SetCineFilterStartColor(LPJASS j);
DWORD SetCineFilterEndColor(LPJASS j);
DWORD SetCineFilterDuration(LPJASS j);
DWORD DisplayCineFilter(LPJASS j);
DWORD IsCineFilterDisplayed(LPJASS j);
DWORD SetCinematicScene(LPJASS j);
DWORD EndCinematicScene(LPJASS j);
DWORD GetCameraMargin(LPJASS j);
DWORD GetCameraBoundMinX(LPJASS j);
DWORD GetCameraBoundMinY(LPJASS j);
DWORD GetCameraBoundMaxX(LPJASS j);
DWORD GetCameraBoundMaxY(LPJASS j);
DWORD GetCameraField(LPJASS j);
DWORD GetCameraTargetPositionX(LPJASS j);
DWORD GetCameraTargetPositionY(LPJASS j);
DWORD GetCameraTargetPositionZ(LPJASS j);
DWORD GetCameraTargetPositionLoc(LPJASS j);
DWORD GetCameraEyePositionX(LPJASS j);
DWORD GetCameraEyePositionY(LPJASS j);
DWORD GetCameraEyePositionZ(LPJASS j);
DWORD GetCameraEyePositionLoc(LPJASS j);
DWORD NewSoundEnvironment(LPJASS j);
DWORD CreateSound(LPJASS j);
DWORD CreateSoundFilenameWithLabel(LPJASS j);
DWORD CreateSoundFromLabel(LPJASS j);
DWORD CreateMIDISound(LPJASS j);
DWORD SetSoundParamsFromLabel(LPJASS j);
DWORD SetSoundDistanceCutoff(LPJASS j);
DWORD SetSoundChannel(LPJASS j);
DWORD SetSoundVolume(LPJASS j);
DWORD SetSoundPitch(LPJASS j);
DWORD SetSoundDistances(LPJASS j);
DWORD SetSoundConeAngles(LPJASS j);
DWORD SetSoundConeOrientation(LPJASS j);
DWORD SetSoundPosition(LPJASS j);
DWORD SetSoundVelocity(LPJASS j);
DWORD AttachSoundToUnit(LPJASS j);
DWORD StartSound(LPJASS j);
DWORD StopSound(LPJASS j);
DWORD KillSoundWhenDone(LPJASS j);
DWORD SetMusicVolume(LPJASS j);
DWORD PlayMusic(LPJASS j);
DWORD SetMapMusic(LPJASS j);
DWORD ClearMapMusic(LPJASS j);
DWORD PlayThematicMusic(LPJASS j);
DWORD EndThematicMusic(LPJASS j);
DWORD StopMusic(LPJASS j);
DWORD ResumeMusic(LPJASS j);
DWORD SetSoundDuration(LPJASS j);
DWORD GetSoundDuration(LPJASS j);
DWORD GetSoundFileDuration(LPJASS j);
DWORD VolumeGroupSetVolume(LPJASS j);
DWORD VolumeGroupReset(LPJASS j);
DWORD GetSoundIsPlaying(LPJASS j);
DWORD GetSoundIsLoading(LPJASS j);
DWORD RegisterStackedSound(LPJASS j);
DWORD UnregisterStackedSound(LPJASS j);
DWORD AddWeatherEffect(LPJASS j);
DWORD RemoveWeatherEffect(LPJASS j);
DWORD EnableWeatherEffect(LPJASS j);
DWORD AddSpecialEffect(LPJASS j);
DWORD AddSpecialEffectLoc(LPJASS j);
DWORD AddSpecialEffectTarget(LPJASS j);
DWORD DestroyEffect(LPJASS j);
DWORD AddSpellEffect(LPJASS j);
DWORD AddSpellEffectLoc(LPJASS j);
DWORD AddSpellEffectById(LPJASS j);
DWORD AddSpellEffectByIdLoc(LPJASS j);
DWORD AddSpellEffectTarget(LPJASS j);
DWORD AddSpellEffectTargetById(LPJASS j);
DWORD SetBlight(LPJASS j);
DWORD SetBlightRect(LPJASS j);
DWORD SetBlightPoint(LPJASS j);
DWORD SetBlightLoc(LPJASS j);
DWORD CreateBlightedGoldmine(LPJASS j);
DWORD SetDoodadAnimation(LPJASS j);
DWORD SetDoodadAnimationRect(LPJASS j);
DWORD StartMeleeAI(LPJASS j);
DWORD StartCampaignAI(LPJASS j);
DWORD CommandAI(LPJASS j);
DWORD PauseCompAI(LPJASS j);
DWORD RemoveGuardPosition(LPJASS j);
DWORD RecycleGuardPosition(LPJASS j);
DWORD RemoveAllGuardPositions(LPJASS j);
DWORD Cheat(LPJASS j);
DWORD IsNoVictoryCheat(LPJASS j);
DWORD IsNoDefeatCheat(LPJASS j);
DWORD Preload(LPJASS j);
DWORD PreloadEnd(LPJASS j);
DWORD PreloadGenClear(LPJASS j);
DWORD PreloadGenStart(LPJASS j);
DWORD PreloadGenEnd(LPJASS j);
DWORD Preloader(LPJASS j);

#endif
