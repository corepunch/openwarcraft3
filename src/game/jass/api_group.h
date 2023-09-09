void group_add_entity(ggroup_t *group, LPEDICT ent) {
    assert(group->num_units < MAX_GROUP_SIZE);
    group->units[group->num_units++] = ent;
}

DWORD CreateGroup(LPJASS j) {
    API_ALLOC(ggroup_t, group);
    return 1;
}
DWORD GroupAddUnit(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    LPEDICT whichUnit = jass_checkhandle(j, 2, "unit");
    whichGroup->units[whichGroup->num_units++] = whichUnit;
    return 0;
}
DWORD GroupRemoveUnit(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    LPEDICT whichUnit = jass_checkhandle(j, 2, "unit");
    FOR_LOOP(i, whichGroup->num_units) {
        if (whichGroup->units[i] == whichUnit) {
            for (DWORD j = i; j < whichGroup->num_units - 1; j++) {
                whichGroup->units[j] = whichGroup->units[j + 1];
            }
            whichGroup->num_units--;
            return 0;
        }
    }
    return 0;
}
DWORD GroupClear(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    whichGroup->num_units = 0;
    return 0;
}
DWORD GroupEnumUnitsOfType(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD GroupEnumUnitsOfPlayer(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (ent->s.player == PLAYER_NUM(whichPlayer)) {
            group_add_entity(whichGroup, ent);
        }
    }
    return 0;
}
DWORD GroupEnumUnitsOfTypeCounted(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR unitname = jass_checkstring(j, 2);
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    //LONG countLimit = jass_checkinteger(j, 4);
    return 0;
}
DWORD GroupEnumUnitsInRect(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    LPBOX2 r = jass_checkhandle(j, 2, "rect");
//    HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (Box2_containsPoint(r, &ent->s.origin2)) {
            group_add_entity(whichGroup, ent);
        }
    }
    return 0;
}

DWORD GroupEnumUnitsInRectCounted(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    HANDLE r = jass_checkhandle(j, 2, "rect");
//    HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    LONG countLimit = jass_checkinteger(j, 4);
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (Box2_containsPoint(r, &ent->s.origin2) && countLimit > 0) {
            group_add_entity(whichGroup, ent);
            countLimit--;
        }
    }
    return 0;
}
DWORD GroupEnumUnitsInRange(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    FLOAT x = jass_checknumber(j, 2);
    FLOAT y = jass_checknumber(j, 3);
    FLOAT radius = jass_checknumber(j, 4);
    //HANDLE filter = jass_checkhandle(j, 5, "boolexpr");
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (Vector2_distance(&ent->s.origin2, &MAKE(VECTOR2, x, y)) < radius) {
            group_add_entity(whichGroup, ent);
        }
    }
    return 0;
}
DWORD GroupEnumUnitsInRangeOfLoc(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    LPCVECTOR2 whichLocation = jass_checkhandle(j, 2, "location");
    FLOAT radius = jass_checknumber(j, 3);
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (Vector2_distance(&ent->s.origin2, whichLocation) < radius) {
            group_add_entity(whichGroup, ent);
        }
    }
    return 0;
}
DWORD GroupEnumUnitsInRangeCounted(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    FLOAT x = jass_checknumber(j, 2);
    FLOAT y = jass_checknumber(j, 3);
    FLOAT radius = jass_checknumber(j, 4);
    //HANDLE filter = jass_checkhandle(j, 5, "boolexpr");
    LONG countLimit = jass_checkinteger(j, 6);
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (Vector2_distance(&ent->s.origin2, &MAKE(VECTOR2, x, y)) < radius && countLimit > 0) {
            group_add_entity(whichGroup, ent);
            countLimit--;
        }
    }
    return 0;
}
DWORD GroupEnumUnitsInRangeOfLocCounted(LPJASS j) {
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    HANDLE whichLocation = jass_checkhandle(j, 2, "location");
    FLOAT radius = jass_checknumber(j, 3);
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    LONG countLimit = jass_checkinteger(j, 5);
    FOR_LOOP(i, globals.num_edicts) {
        LPEDICT ent = &globals.edicts[i];
        if (Vector2_distance(&ent->s.origin2, whichLocation) < radius && countLimit > 0) {
            group_add_entity(whichGroup, ent);
            countLimit--;
        }
    }
    return 0;
}
DWORD GroupEnumUnitsSelected(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return 0;
}
DWORD GroupImmediateOrder(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD GroupImmediateOrderById(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrder(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrderLoc(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrderById(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    //FLOAT x = jass_checknumber(j, 3);
    //FLOAT y = jass_checknumber(j, 4);
    return jass_pushboolean(j, 0);
}
DWORD GroupPointOrderByIdLoc(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE whichLocation = jass_checkhandle(j, 3, "location");
    return jass_pushboolean(j, 0);
}
DWORD GroupTargetOrder(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LPCSTR order = jass_checkstring(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD GroupTargetOrderById(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    //LONG order = jass_checkinteger(j, 2);
    //HANDLE targetWidget = jass_checkhandle(j, 3, "widget");
    return jass_pushboolean(j, 0);
}
DWORD ForGroup(LPJASS j) {
    extern LPEDICT currentunit;
    ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    LPCJASSFUNC callback = jass_checkcode(j, 2);
    FOR_LOOP(i, whichGroup->num_units) {
        currentunit = whichGroup->units[i];
        jass_pushfunction(j, callback);
        jass_call(j, 0);
    }
    return 0;
}
DWORD FirstOfGroup(LPJASS j) {
    //ggroup_t *whichGroup = jass_checkhandle(j, 1, "group");
    return jass_pushnullhandle(j, "unit");
}
