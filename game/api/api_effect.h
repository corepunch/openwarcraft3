DWORD AddWeatherEffect(LPJASS j) {
    //HANDLE where = jass_checkhandle(j, 1, "rect");
    //LONG effectID = jass_checkinteger(j, 2);
    return jass_pushnullhandle(j, "weathereffect");
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
    return jass_pushnullhandle(j, "effect");
}
DWORD AddSpecialEffectLoc(LPJASS j) {
    //LPCSTR modelName = jass_checkstring(j, 1);
    //HANDLE where = jass_checkhandle(j, 2, "location");
    return jass_pushnullhandle(j, "effect");
}
DWORD AddSpecialEffectTarget(LPJASS j) {
    LPCSTR modelName = jass_checkstring(j, 1);
    LPEDICT targetWidget = jass_checkhandle(j, 2, "widget");
    LPCSTR attachPointName = jass_checkstring(j, 3);
    targetWidget->s.model2 = gi.ModelIndex(modelName);
    if (!strcmp(attachPointName, "overhead")) {
        targetWidget->s.renderfx |= RF_ATTACH_OVERHEAD;
    }
    return jass_pushlighthandle(j, targetWidget, "effect");
}
DWORD DestroyEffect(LPJASS j) {
    LPEDICT whichEffect = jass_checkhandle(j, 1, "effect");
    whichEffect->s.model2 = 0;
    return 0;
}
DWORD AddSpellEffect(LPJASS j) {
    //LPCSTR abilityString = jass_checkstring(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushnullhandle(j, "effect");
}
DWORD AddSpellEffectLoc(LPJASS j) {
    //LPCSTR abilityString = jass_checkstring(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE where = jass_checkhandle(j, 3, "location");
    return jass_pushnullhandle(j, "effect");
}
DWORD AddSpellEffectById(LPJASS j) {
    //LONG abilityId = jass_checkinteger(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushnullhandle(j, "effect");
}
DWORD AddSpellEffectByIdLoc(LPJASS j) {
    //LONG abilityId = jass_checkinteger(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE where = jass_checkhandle(j, 3, "location");
    return jass_pushnullhandle(j, "effect");
}
DWORD AddSpellEffectTarget(LPJASS j) {
    //LPCSTR modelName = jass_checkstring(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    //LPCSTR attachPoint = jass_checkstring(j, 4);
    return jass_pushnullhandle(j, "effect");
}
DWORD AddSpellEffectTargetById(LPJASS j) {
    //LONG abilityId = jass_checkinteger(j, 1);
    //HANDLE t = jass_checkhandle(j, 2, "effecttype");
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    //LPCSTR attachPoint = jass_checkstring(j, 4);
    return jass_pushnullhandle(j, "effect");
}
