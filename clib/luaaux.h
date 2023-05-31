#ifndef luaaux_h
#define luaaux_h

#include "../common/luastate.h"
struct lua_State* luaL_newstate();

void luaL_close(struct lua_State* L);

#endif

