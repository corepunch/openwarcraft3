DWORD CreateTrigger(LPJASS j) {
    API_ALLOC(TRIGGER, trigger);
    return 1;
}
DWORD DestroyTrigger(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD ResetTrigger(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD EnableTrigger(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    whichTrigger->disabled = false;
    return 0;
}
DWORD DisableTrigger(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    whichTrigger->disabled = true;
    return 0;
}
DWORD IsTriggerEnabled(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushboolean(j, !whichTrigger->disabled);
}
DWORD TriggerWaitOnSleeps(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //BOOL flag = jass_checkboolean(j, 2);
    return 0;
}
DWORD IsTriggerWaitOnSleeps(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushboolean(j, 0);
}
DWORD GetTriggeringTrigger(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->trigger, "trigger");
}
DWORD GetTriggerEventId(LPJASS j) {
    return jass_pushnullhandle(j, "eventid");
}
DWORD GetTriggerEvalCount(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushinteger(j, 0);
}
DWORD GetTriggerExecCount(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return jass_pushinteger(j, 0);
}
DWORD TriggerRegisterVariableEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPCSTR varName = jass_checkstring(j, 2);
    //HANDLE opcode = jass_checkhandle(j, 3, "limitop");
    //FLOAT limitval = jass_checknumber(j, 4);
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterTimerEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //FLOAT timeout = jass_checknumber(j, 2);
    //BOOL periodic = jass_checkboolean(j, 3);
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterTimerExpireEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "timer");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterGameStateEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichState = jass_checkhandle(j, 2, "gamestate");
    //HANDLE opcode = jass_checkhandle(j, 3, "limitop");
    //FLOAT limitval = jass_checknumber(j, 4);
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterDialogEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichDialog = jass_checkhandle(j, 2, "dialog");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterDialogButtonEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichButton = jass_checkhandle(j, 2, "button");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterGameEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichGameEvent = jass_checkhandle(j, 2, "gameevent");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterEnterRegion(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    LPREGION whichRegion = jass_checkhandle(j, 2, "region");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    LPEVENT evt = G_MakeEvent(EVENT_GAME_ENTER_REGION);
    evt->trigger = whichTrigger;
    evt->region = *whichRegion;
    return jass_pushlighthandle(j, evt, "event");
}
DWORD GetTriggeringRegion(LPJASS j) {
    return jass_pushnullhandle(j, "region");
}
DWORD TriggerRegisterLeaveRegion(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichRegion = jass_checkhandle(j, 2, "region");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterTrackableHitEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "trackable");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterTrackableTrackEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "trackable");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterPlayerEvent(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    EVENTTYPE *whichPlayerEvent = jass_checkhandle(j, 3, "playerevent");
    LPEVENT evt = G_MakeEvent(*whichPlayerEvent);
    evt->subject = PLAYER_ENT(whichPlayer);
    evt->trigger = whichTrigger;
    return jass_pushlighthandle(j, evt, "event");
}
DWORD GetTriggerPlayer(LPJASS j) {
    return jass_pushnullhandle(j, "player");
}
DWORD TriggerRegisterPlayerUnitEvent(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    EVENTTYPE *whichPlayerUnitEvent = jass_checkhandle(j, 3, "playerunitevent");
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    LPEVENT evt = G_MakeEvent(*whichPlayerUnitEvent);
    evt->subject = PLAYER_ENT(whichPlayer);
    evt->trigger = whichTrigger;
    return jass_pushlighthandle(j, evt, "event");
}
DWORD TriggerRegisterPlayerAllianceChange(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichAlliance = jass_checkhandle(j, 3, "alliancetype");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterPlayerStateEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichState = jass_checkhandle(j, 3, "playerstate");
    //HANDLE opcode = jass_checkhandle(j, 4, "limitop");
    //FLOAT limitval = jass_checknumber(j, 5);
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterPlayerChatEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //LPCSTR chatMessageToDetect = jass_checkstring(j, 3);
    //BOOL exactMatchOnly = jass_checkboolean(j, 4);
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterDeathEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichWidget = jass_checkhandle(j, 2, "widget");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterUnitStateEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //HANDLE whichState = jass_checkhandle(j, 3, "unitstate");
    //HANDLE opcode = jass_checkhandle(j, 4, "limitop");
    //FLOAT limitval = jass_checknumber(j, 5);
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterUnitEvent(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    LPEDICT whichUnit = jass_checkhandle(j, 2, "unit");
    EVENTTYPE *whichEvent = jass_checkhandle(j, 3, "unitevent");
    LPEVENT evt = G_MakeEvent(*whichEvent);
    evt->subject = whichUnit;
    evt->trigger = whichTrigger;
    return jass_pushlighthandle(j, evt, "event");
}
DWORD TriggerRegisterFilterUnitEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //HANDLE whichEvent = jass_checkhandle(j, 3, "unitevent");
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    return jass_pushnullhandle(j, "event");
}
DWORD TriggerRegisterUnitInRange(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    LPEDICT whichUnit = jass_checkhandle(j, 2, "unit");
    FLOAT range = jass_checknumber(j, 3);
//    HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    LPEVENT evt = G_MakeEvent(EVENT_UNIT_IN_RANGE);
    evt->subject = whichUnit;
    evt->trigger = whichTrigger;
    evt->range = range;
    return jass_pushlighthandle(j, evt, "event");
}
DWORD TriggerAddCondition(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    TRIGGERCONDITION *condition = gi.MemAlloc(sizeof(TRIGGERCONDITION));
    condition->expr = jass_checkhandle(j, 2, "boolexpr");
    ADD_TO_LIST(condition, whichTrigger->conditions);
    return jass_pushlighthandle(j, condition, "triggercondition");
}
DWORD TriggerRemoveCondition(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    TRIGGERCONDITION *whichCondition = jass_checkhandle(j, 2, "triggercondition");
    REMOVE_FROM_LIST(TRIGGERCONDITION, whichCondition, whichTrigger->conditions, gi.MemFree);
    return 0;
}
DWORD TriggerClearConditions(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    DELETE_LIST(TRIGGERCONDITION, whichTrigger->conditions, gi.MemFree);
    return 0;
}
DWORD TriggerAddAction(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    TRIGGERACTION *action = gi.MemAlloc(sizeof(TRIGGERACTION));
    action->func = jass_checkcode(j, 2);
    ADD_TO_LIST(action, whichTrigger->actions);
    return jass_pushlighthandle(j, action, "triggeraction");
}
DWORD TriggerRemoveAction(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    TRIGGERACTION *whichAction = jass_checkhandle(j, 2, "triggeraction");
    REMOVE_FROM_LIST(TRIGGERACTION, whichAction, whichTrigger->actions, gi.MemFree);
    return 0;
}
DWORD TriggerClearActions(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    DELETE_LIST(TRIGGERACTION, whichTrigger->actions, gi.MemFree);
    return 0;
}
DWORD TriggerSleepAction(LPJASS j) {
    FLOAT timeout = jass_checknumber(j, 1);
    gi.Sleep(timeout * 1000);
    return 0;
}
DWORD TriggerWaitForSound(LPJASS j) {
    gsound_t *s = jass_checkhandle(j, 1, "sound");
    FLOAT offset = jass_checknumber(j, 2);
    gi.Sleep(s->duration * 0.1 + offset * 1000);
    return 0;
}
DWORD TriggerEvaluate(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    FOR_EACH_LIST(TRIGGERCONDITION, cond, whichTrigger->conditions) {
        jass_pushfunction(j, cond->expr);
        if (jass_call(j, 0) != 1 || !jass_popboolean(j)) {
            return jass_pushboolean(j, false);
        }
    }
    return jass_pushboolean(j, true);
}
DWORD TriggerExecute(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    FOR_EACH_LIST(TRIGGERACTION, action, whichTrigger->actions) {
        jass_pushfunction(j, action->func);
        jass_call(j, 0);
    }
    return 0;
}
DWORD TriggerExecuteWait(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD GetTriggerUnit(LPJASS j) {
    return jass_pushlighthandle(j, jass_getcontext(j)->unit, "unit");
}
DWORD GetTriggerWidget(LPJASS j) {
    return jass_pushnullhandle(j, "widget");
}
