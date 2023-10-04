#define UNIT_TYPED_ACCESS(NAME, FIELD, TYPE) \
DWORD SetUnit##NAME(LPJASS j) {  \
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");  \
    memcpy(&whichUnit->FIELD, jass_checkhandle(j, 2, #TYPE), sizeof(whichUnit->FIELD)); \
    return 0; \
}  \
DWORD GetUnit##NAME(LPJASS j) {  \
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");  \
    return jass_pushlighthandle(j, &whichUnit->FIELD, #TYPE); \
}

#define UNIT_ACCESS(NAME, FIELD) \
DWORD SetUnit##NAME(LPJASS j) {  \
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");  \
    whichUnit->FIELD = jass_checknumber(j, 2); \
    return 0; \
}  \
DWORD GetUnit##NAME(LPJASS j) {  \
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");  \
    return jass_pushnumber(j, whichUnit->FIELD); \
}

#define UNITINFO_ACCESS(FIELD) UNIT_ACCESS(FIELD, unitinfo.FIELD)

UNIT_TYPED_ACCESS(PositionLoc, s.origin2, location);
UNIT_ACCESS(X, s.origin.x);
UNIT_ACCESS(Y, s.origin.y);
UNITINFO_ACCESS(MoveSpeed);
UNITINFO_ACCESS(FlyHeight);
UNITINFO_ACCESS(TurnSpeed);
UNITINFO_ACCESS(PropWindow);
UNITINFO_ACCESS(AcquireRange);

DWORD GetUnitFacing(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    FLOAT facingAngle = whichUnit->s.angle;
    jass_pushnumber(j, RAD2DEG(facingAngle));
    return 1;
}

DWORD SetUnitFacing(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    FLOAT facingAngle = jass_checknumber(j, 2);
    whichUnit->s.angle = DEG2RAD(facingAngle);
    return 0;
}

DWORD SetUnitFacingTimed(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    FLOAT facingAngle = jass_checknumber(j, 2);
//    FLOAT duration = jass_checknumber(j, 3);
    whichUnit->s.angle = DEG2RAD(facingAngle);
    return 0;
}

DWORD KillUnit(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    whichUnit->health.value = 0;
    return 0;
}
DWORD RemoveUnit(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    if (whichUnit) {
        G_FreeEdict(whichUnit);
    }
    return 0;
}
DWORD ShowUnit(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    BOOL show = jass_checkboolean(j, 2);
    if (show) {
        whichUnit->s.renderfx &= ~RF_HIDDEN;
    } else {
        whichUnit->s.renderfx |= RF_HIDDEN;
    }
    return 0;
}

JASS_API(SetUnitState,
(EDICT, whichUnit, "unit"),
(UNITSTATE, whichUnitState, "unitstate"),
(number, newVal))
{
    (&whichUnit->health.value)[*whichUnitState] = newVal;
}
//DWORD SetUnitState(LPJASS j) {
//    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
//    UNITSTATE *whichUnitState = jass_checkhandle(j, 2, "unitstate");
//    FLOAT newVal = jass_checknumber(j, 3);
//    (&whichUnit->health.value)[*whichUnitState] = newVal;
//    return 0;
//}
DWORD GetUnitState(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    UNITSTATE *whichUnitState = jass_checkhandle(j, 2, "unitstate");
    FLOAT value = (&whichUnit->health.value)[*whichUnitState];
    return jass_pushnumber(j, value);
}
DWORD SetUnitPosition(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    whichUnit->s.origin.x = jass_checknumber(j, 2);
    whichUnit->s.origin.y = jass_checknumber(j, 3);
    return 0;
}
DWORD GetUnitDefaultAcquireRange(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultTurnSpeed(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultPropWindow(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetUnitDefaultFlyHeight(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD SetUnitOwner(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPCPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
//    BOOL changeColor = jass_checkboolean(j, 3);
    whichUnit->s.player = PLAYER_NUM(whichPlayer);
    return 0;
}
DWORD SetUnitColor(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichColor = jass_checkhandle(j, 2, "playercolor");
    return 0;
}
DWORD SetUnitScale(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
//    FLOAT scaleX = jass_checknumber(j, 2);
//    FLOAT scaleY = jass_checknumber(j, 3);
    FLOAT scaleZ = jass_checknumber(j, 4);
    whichUnit->s.scale = scaleZ;
    return 0;
}
DWORD SetUnitTimeScale(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT timeScale = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitBlendTime(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT blendTime = jass_checknumber(j, 2);
    return 0;
}
DWORD SetUnitVertexColor(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
    return 0;
}
DWORD QueueUnitAnimation(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    return 0;
}
DWORD SetUnitAnimation(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPCSTR whichAnimation = jass_checkstring(j, 2);
    whichUnit->animation = gi.GetAnimation(whichUnit->s.model, whichAnimation);
    return 0;
}
DWORD SetUnitAnimationByIndex(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG whichAnimation = jass_checkinteger(j, 2);
    return 0;
}
DWORD SetUnitAnimationWithRarity(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR whichAnimation = jass_checkstring(j, 2);
    //HANDLE rarity = jass_checkhandle(j, 3, "raritycontrol");
    return 0;
}
DWORD AddUnitAnimationProperties(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR animProperties = jass_checkstring(j, 2);
    //BOOL add = jass_checkboolean(j, 3);
    return 0;
}

//JASS_API(SetUnitLookAt,
//(EDICT, whichUnit, "unit"),
//(string, whichBone),
//(EDICT, lookAtTarget, "unit"),
//(number, offsetX),
//(number, offsetY),
//(number, offsetZ))
//{
//}
DWORD SetUnitLookAt(LPJASS j) {
//    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
//    LPCSTR whichBone = jass_checkstring(j, 2);
//    HANDLE lookAtTarget = jass_checkhandle(j, 3, "unit");
//    FLOAT offsetX = jass_checknumber(j, 4);
//    FLOAT offsetY = jass_checknumber(j, 5);
//    FLOAT offsetZ = jass_checknumber(j, 6);
    return 0;
}
DWORD ResetUnitLookAt(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD SetUnitRescuable(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE byWhichPlayer = jass_checkhandle(j, 2, "player");
    //BOOL flag = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetUnitRescueRange(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT range = jass_checknumber(j, 2);
    return 0;
}
DWORD SetHeroStr(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newStr = jass_checkinteger(j, 2);
    //BOOL permanent = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetHeroAgi(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newAgi = jass_checkinteger(j, 2);
    //BOOL permanent = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetHeroInt(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newInt = jass_checkinteger(j, 2);
    //BOOL permanent = jass_checkboolean(j, 3);
    return 0;
}
DWORD GetHeroXP(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD SetHeroXP(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //LONG newXpVal = jass_checkinteger(j, 2);
    //BOOL showEyeCandy = jass_checkboolean(j, 3);
    return 0;
}
DWORD AddHeroXP(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //LONG xpToAdd = jass_checkinteger(j, 2);
    //BOOL showEyeCandy = jass_checkboolean(j, 3);
    return 0;
}
DWORD SetHeroLevel(LPJASS j) {
    LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    LONG level = jass_checkinteger(j, 2);
//    BOOL showEyeCandy = jass_checkboolean(j, 3);
    whichHero->hero.level = level;
    return 0;
}
DWORD GetHeroLevel(LPJASS j) {
    LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichHero->hero.level);
}
DWORD SuspendHeroXP(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsSuspendedXP(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD SelectHeroSkill(LPJASS j) {
    LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //LONG abilcode = jass_checkinteger(j, 2);
    return 0;
}
DWORD ReviveHero(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //BOOL doEyecandy = jass_checkboolean(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD ReviveHeroLoc(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    //HANDLE loc = jass_checkhandle(j, 2, "location");
    //BOOL doEyecandy = jass_checkboolean(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD SetUnitExploded(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL exploded = jass_checkboolean(j, 2);
    return 0;
}
DWORD SetUnitInvulnerable(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD PauseUnit(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsUnitPaused(LPJASS j) {
    //LPEDICT whichHero = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD SetUnitPathing(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD GetUnitPointValue(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD GetUnitPointValueByType(LPJASS j) {
    //LONG unitType = jass_checkinteger(j, 1);
    return jass_pushinteger(j, 0);
}
DWORD UnitAddItem(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPEDICT whichItem = jass_checkhandle(j, 2, "item");
//    printf("UnitAddItem %.4s %.4s\n", (LPCSTR)&whichUnit->class_id, (LPCSTR)&whichItem->class_id);
    if (unit_additem(whichUnit, whichItem->class_id)) {
        G_FreeEdict(whichItem);
        return jass_pushboolean(j, true);
    } else {
        return jass_pushboolean(j, false);
    }
}
DWORD UnitAddItemById(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LONG itemId = jass_checkinteger(j, 2);
    if (unit_additem(whichUnit, itemId)) {
        return jass_pushnullhandle(j, "item");
    } else {
        LPEDICT item = SP_SpawnAtLocation(itemId, 0, &whichUnit->s.origin2);
        return jass_pushlighthandle(j, item, "item");
    }
}
DWORD UnitAddItemToSlotById(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LONG itemId = jass_checkinteger(j, 2);
    LONG itemSlot = jass_checkinteger(j, 3);
    return jass_pushboolean(j, unit_additemtoslot(whichUnit, itemId, itemSlot));
}
DWORD UnitRemoveItem(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    return 0;
}
DWORD UnitRemoveItemFromSlot(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LONG itemSlot = jass_checkinteger(j, 2);
    if (whichUnit->inventory[itemSlot] != 0) {
        DWORD itemId = whichUnit->inventory[itemSlot];
        LPEDICT item = SP_SpawnAtLocation(itemId, 0, &whichUnit->s.origin2);
        return jass_pushlighthandle(j, item, "item");
    } else {
        return jass_pushnullhandle(j, "item");
    }
}
DWORD UnitHasItem(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    return jass_pushboolean(j, 0);
}
DWORD UnitItemInSlot(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG itemSlot = jass_checkinteger(j, 2);
    return jass_pushnullhandle(j, "item");
}
DWORD UnitUseItem(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    return jass_pushboolean(j, 0);
}
DWORD UnitUseItemPoint(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD UnitUseItemTarget(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichItem = jass_checkhandle(j, 2, "item");
    //HANDLE target = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD GetUnitLoc(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    API_ALLOC(VECTOR2, location);
    if (whichUnit) {
        *location = whichUnit->s.origin2;
    }
    return 1;
}
DWORD GetUnitDefaultMoveSpeed(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnumber(j, 0);
}
DWORD GetOwningPlayer(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPPLAYER player = G_GetPlayerByNumber(whichUnit->s.player);
    return jass_pushlighthandle(j, player, "player");
}
DWORD GetUnitTypeId(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD GetUnitRace(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushnullhandle(j, "race");
}
DWORD GetUnitName(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushstring(j, 0);
}
DWORD GetUnitFoodUsed(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD GetUnitFoodMade(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, 0);
}
DWORD IsUnitInGroup(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichGroup = jass_checkhandle(j, 2, "group");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInForce(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichForce = jass_checkhandle(j, 2, "force");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitOwnedByPlayer(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitAlly(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitEnemy(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitVisible(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitDetected(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInvisible(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitFogged(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitMasked(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitSelected(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitRace(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichRace = jass_checkhandle(j, 2, "race");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitType(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichUnitType = jass_checkhandle(j, 2, "unittype");
    return jass_pushboolean(j, 0);
}
DWORD IsUnit(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichSpecifiedUnit = jass_checkhandle(j, 2, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInRange(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE otherUnit = jass_checkhandle(j, 2, "unit");
    //FLOAT distance = jass_checknumber(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInRangeXY(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT distance = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInRangeLoc(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    //FLOAT distance = jass_checknumber(j, 3);
    return jass_pushboolean(j, 0);
}
DWORD IsUnitHidden(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitIllusion(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitInTransport(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichTransport = jass_checkhandle(j, 2, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsUnitLoaded(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD IsHeroUnitId(LPJASS j) {
    //LONG unitId = jass_checkinteger(j, 1);
    return jass_pushboolean(j, 0);
}
DWORD UnitShareVision(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //HANDLE whichPlayer = jass_checkhandle(j, 2, "player");
    //BOOL share = jass_checkboolean(j, 3);
    return 0;
}
DWORD UnitSuspendDecay(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL suspend = jass_checkboolean(j, 2);
    return 0;
}
DWORD UnitRemoveAbility(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG abilityId = jass_checkinteger(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD UnitRemoveBuffs(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL removePositive = jass_checkboolean(j, 2);
    //BOOL removeNegative = jass_checkboolean(j, 3);
    return 0;
}
DWORD UnitAddSleep(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL add = jass_checkboolean(j, 2);
    return 0;
}
DWORD UnitCanSleep(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD UnitAddSleepPerm(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //BOOL add = jass_checkboolean(j, 2);
    return 0;
}
DWORD UnitCanSleepPerm(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD UnitIsSleeping(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushboolean(j, 0);
}
DWORD UnitWakeUp(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return 0;
}
DWORD UnitApplyTimedLife(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG buffId = jass_checkinteger(j, 2);
    //FLOAT duration = jass_checknumber(j, 3);
    return 0;
}
DWORD IssueImmediateOrder(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPCSTR order = jass_checkstring(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD IssueImmediateOrderById(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD IssuePointOrder(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPCSTR order = jass_checkstring(j, 2);
    FLOAT x = jass_checknumber(j, 3);
    FLOAT y = jass_checknumber(j, 4);
    BOOL ret = unit_issueorder(whichUnit, order, &MAKE(VECTOR2, x, y));
    return jass_pushboolean(j, ret);
}
DWORD IssuePointOrderLoc(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPCSTR order = jass_checkstring(j, 2);
    LPCVECTOR2 whichLocation = jass_checkhandle(j, 3, "location");
    BOOL ret = unit_issueorder(whichUnit, order, whichLocation);
    return jass_pushboolean(j, ret);
}
DWORD IssuePointOrderById(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD IssuePointOrderByIdLoc(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    return jass_pushboolean(j, 0);
}
DWORD IssueTargetOrder(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    LPCSTR order = jass_checkstring(j, 2);
    LPEDICT targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, unit_issuetargetorder(whichUnit, order, targetWidget));
}
DWORD IssueTargetOrderById(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD IssueInstantTargetOrder(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    //HANDLE instantTargetWidget = jass_checkhandle(j, 4, "widget");
    return jass_pushboolean(j, 0);
}
DWORD IssueInstantTargetOrderById(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
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
DWORD SetResourceAmount(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    whichUnit->resources = jass_checkinteger(j, 2);
    return 0;
}
DWORD AddResourceAmount(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    whichUnit->resources += jass_checkinteger(j, 2);
    return 0;
}
DWORD GetResourceAmount(LPJASS j) {
    LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    return jass_pushinteger(j, whichUnit->resources);
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
DWORD UnitAddIndicator(LPJASS j) {
    //LPEDICT whichUnit = jass_checkhandle(j, 1, "unit");
    //LONG red = jass_checkinteger(j, 2);
    //LONG green = jass_checkinteger(j, 3);
    //LONG blue = jass_checkinteger(j, 4);
    //LONG alpha = jass_checkinteger(j, 5);
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
DWORD CreateUnit(LPJASS j) {
    LPPLAYER player = jass_checkhandle(j, 1, "player");
    LPEDICT unit =
    unit_createorfind(PLAYER_NUM(player),
                        jass_checkinteger(j, 2),
                        &MAKE(VECTOR2, jass_checknumber(j, 3), jass_checknumber(j, 4)),
                        jass_checknumber(j, 5));
    return jass_pushlighthandle(j, unit, "unit");
}
DWORD CreateUnitByName(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    return jass_pushnullhandle(j, "unit");
}
DWORD CreateUnitAtLoc(LPJASS j) {
    LPPLAYER player = jass_checkhandle(j, 1, "player");
    LPEDICT unit =
    unit_createorfind(PLAYER_NUM(player),
                      jass_checkinteger(j, 2),
                      jass_checkhandle(j, 3, "location"),
                      jass_checknumber(j, 4));
    return jass_pushlighthandle(j, unit, "unit");
}
DWORD CreateUnitAtLocByName(LPJASS j) {
    //HANDLE id = jass_checkhandle(j, 1, "player");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    //FLOAT face = jass_checknumber(j, 4);
    return jass_pushnullhandle(j, "unit");
}
DWORD CreateCorpse(LPJASS j) {
    //HANDLE whichPlayer = jass_checkhandle(j, 1, "player");
    //LONG unitid = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    //FLOAT face = jass_checknumber(j, 5);
    return jass_pushnullhandle(j, "unit");
}
DWORD CreateBlightedGoldmine(LPJASS j) {
    //HANDLE id = jass_checkhandle(j, 1, "player");
    //FLOAT x = jass_checknumber(j, 2);
    //FLOAT y = jass_checknumber(j, 3);
    //FLOAT face = jass_checknumber(j, 4);
    return jass_pushnullhandle(j, "unit");
}
