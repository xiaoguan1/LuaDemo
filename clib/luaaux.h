#ifndef luaaux_h
#define luaaux_h

#include "../common/luastate.h"
struct lua_State* luaL_newstate();

void luaL_close(struct lua_State* L);

lua_Integer luaL_tointeger(struct lua_State* L, int idx);

int luaL_pcall(struct lua_State* L, int narg, int nresult);

#endif

