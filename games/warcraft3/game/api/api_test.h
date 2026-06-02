/*
 * api_test.h — JASS test-assertion natives.
 *
 * BJassError(msg)          — unconditionally raise a runtime error.
 * BJassAssert(cond, msg)   — raise if cond is false.
 *
 * Both abort the current coroutine via jass_rterror(), which uses
 * setjmp/longjmp.  They are the JASS equivalent of Lua's error().
 */

DWORD BJassError(LPJASS j) {
    LPCSTR msg = jass_checkstring(j, 1);
    jass_rterror(j, msg);
    return 0;
}

DWORD BJassAssert(LPJASS j) {
    BOOL cond = jass_checkboolean(j, 1);
    if (!cond) {
        LPCSTR msg = jass_checkstring(j, 2);
        char buf[640];
        snprintf(buf, sizeof(buf), "assertion failed: %s", msg ? msg : "(no message)");
        jass_rterror(j, buf);
    }
    return 0;
}
