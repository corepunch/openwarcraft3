DWORD CreateTrigger(LPJASS j) {
    API_ALLOC(TRIGGER, value);
    return jass_pushhandle(j, value, "trigger");
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
    return jass_pushlighthandle(j, jass_gethosttrigger(j), "trigger");
}
DWORD GetTriggerEventId(LPJASS j) {
    return jass_pushhandle(j, 0, "eventid");
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
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterTimerEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //FLOAT timeout = jass_checknumber(j, 2);
    //BOOL periodic = jass_checkboolean(j, 3);
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterTimerExpireEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "timer");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterGameStateEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichState = jass_checkhandle(j, 2, "gamestate");
    //HANDLE opcode = jass_checkhandle(j, 3, "limitop");
    //FLOAT limitval = jass_checknumber(j, 4);
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterDialogEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichDialog = jass_checkhandle(j, 2, "dialog");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterDialogButtonEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichButton = jass_checkhandle(j, 2, "button");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterGameEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichGameEvent = jass_checkhandle(j, 2, "gameevent");
    return jass_pushhandle(j, 0, "event");
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
    return jass_pushhandle(j, 0, "region");
}
DWORD TriggerRegisterLeaveRegion(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichRegion = jass_checkhandle(j, 2, "region");
    //HANDLE filter = jass_checkhandle(j, 3, "boolexpr");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterTrackableHitEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "trackable");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterTrackableTrackEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE t = jass_checkhandle(j, 2, "trackable");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterPlayerEvent(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    LPMAPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    EVENTTYPE *whichPlayerEvent = jass_checkhandle(j, 3, "playerevent");
    LPEVENT evt = G_MakeEvent(*whichPlayerEvent);
    evt->subject = whichPlayer;
    evt->trigger = whichTrigger;
    return jass_pushlighthandle(j, evt, "event");
}
DWORD GetTriggerPlayer(LPJASS j) {
    return jass_pushhandle(j, 0, "player");
}
DWORD TriggerRegisterPlayerUnitEvent(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    LPMAPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    EVENTTYPE *whichPlayerUnitEvent = jass_checkhandle(j, 3, "playerunitevent");
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    LPEVENT evt = G_MakeEvent(*whichPlayerUnitEvent);
    evt->subject = whichPlayer;
    evt->trigger = whichTrigger;
    return jass_pushlighthandle(j, evt, "event");
}
DWORD TriggerRegisterPlayerAllianceChange(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPMAPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichAlliance = jass_checkhandle(j, 3, "alliancetype");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterPlayerStateEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPMAPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //HANDLE whichState = jass_checkhandle(j, 3, "playerstate");
    //HANDLE opcode = jass_checkhandle(j, 4, "limitop");
    //FLOAT limitval = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterPlayerChatEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //LPMAPPLAYER whichPlayer = jass_checkhandle(j, 2, "player");
    //LPCSTR chatMessageToDetect = jass_checkstring(j, 3);
    //BOOL exactMatchOnly = jass_checkboolean(j, 4);
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterDeathEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichWidget = jass_checkhandle(j, 2, "widget");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterUnitStateEvent(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //HANDLE whichState = jass_checkhandle(j, 3, "unitstate");
    //HANDLE opcode = jass_checkhandle(j, 4, "limitop");
    //FLOAT limitval = jass_checknumber(j, 5);
    return jass_pushhandle(j, 0, "event");
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
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerRegisterUnitInRange(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    //HANDLE whichUnit = jass_checkhandle(j, 2, "unit");
    //FLOAT range = jass_checknumber(j, 3);
    //HANDLE filter = jass_checkhandle(j, 4, "boolexpr");
    return jass_pushhandle(j, 0, "event");
}
DWORD TriggerAddCondition(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    gtriggercondition_t *condition = gi.MemAlloc(sizeof(gtriggercondition_t));
    condition->expr = jass_checkhandle(j, 2, "boolexpr");
    ADD_TO_LIST(condition, whichTrigger->conditions);
    return jass_pushlighthandle(j, condition, "triggercondition");
}
DWORD TriggerRemoveCondition(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    gtriggercondition_t *whichCondition = jass_checkhandle(j, 2, "triggercondition");
    gtriggercondition_t **prev = &whichTrigger->conditions;
    FOR_EACH_LIST(gtriggercondition_t, it, whichTrigger->conditions) {
        if (it == whichCondition) {
            *prev = it->next;
            gi.MemFree(it);
            break;
        }
        prev = &it->next;
    }
    return 0;
}
DWORD TriggerClearConditions(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    if (!whichTrigger->conditions)
        return 0;
    for (gtriggercondition_t *it = whichTrigger->conditions; it;) {
        gtriggercondition_t *next = it->next;
        gi.MemFree(it);
        it = next;
    }
    return 0;
}
DWORD TriggerAddAction(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    gtriggeraction_t *action = gi.MemAlloc(sizeof(gtriggeraction_t));
    action->func = jass_checkcode(j, 2);
    ADD_TO_LIST(action, whichTrigger->actions);
    return jass_pushlighthandle(j, action, "triggeraction");
}
DWORD TriggerRemoveAction(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    gtriggeraction_t *whichAction = jass_checkhandle(j, 2, "triggeraction");
    gtriggeraction_t **prev = &whichTrigger->actions;
    FOR_EACH_LIST(gtriggeraction_t, it, whichTrigger->actions) {
        if (it == whichAction) {
            *prev = it->next;
            gi.MemFree(it);
            break;
        }
        prev = &it->next;
    }
    return 0;
}
DWORD TriggerClearActions(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
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
    return jass_pushboolean(j, whichTrigger->actions ? true : false);
}
DWORD TriggerExecute(LPJASS j) {
    LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    jass_calltrigger(j, whichTrigger);
    return 0;
}
DWORD TriggerExecuteWait(LPJASS j) {
    //LPTRIGGER whichTrigger = jass_checkhandle(j, 1, "trigger");
    return 0;
}
DWORD GetTriggerUnit(LPJASS j) {
    return jass_pushhandle(j, 0, "unit");
}
DWORD GetTriggerWidget(LPJASS j) {
    return jass_pushhandle(j, 0, "widget");
}
