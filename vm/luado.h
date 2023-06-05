#ifndef luado.h
#define luado.h

#include "../common/luastate.h"

int luaD_call(struct lua_State* L, StkId func, int nresult);

int luaD_precall(struct lua_State* L, StkId func, int nresult);

#endif