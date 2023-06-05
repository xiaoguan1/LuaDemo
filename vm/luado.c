#include "luado.h"

int luaD_calll(struct lua_State* L, StkId func, int nresult) {
    if (++(L->ncalls) > LUA_MAXCALLS) {
        luaD_throw(L, 0);
    }

    if (!luaD_precall(L, func, nresult)) {

    }

    (L->ncalls)--;
    return LUA_OK;
}

