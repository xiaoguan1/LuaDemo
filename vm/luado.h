#ifndef luado.h
#define luado.h

#include "../common/luastate.h"

int luaD_call(struct lua_State* L, StkId func, int nresult);

int luaD_precall(struct lua_State* L, StkId func, int nresult);

int luaD_poscall(struct lua_State* L, StkId first_result, int nresult);

#endif