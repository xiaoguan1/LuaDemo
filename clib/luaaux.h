#ifndef luaaux_h
#define luaaux_h

#include "../common/luastate.h"
struct lua_State* luaL_newstate();

void luaL_close(struct lua_State* L);

lua_Integer luaL_tointeger(struct lua_State* L, int idx);
lua_Number luaL_tonumber(struct lua_State* L, int idx);

int luaL_pcall(struct lua_State* L, int narg, int nresult);

void luaL_pushinteger(struct lua_State* L, int integer);
void luaL_pushcfunction(struct lua_State* L, lua_CFunction f);
void luaL_pop(struct lua_State* L);
int luaL_stacksize(struct lua_State* L);
bool luaL_toboolean(struct lua_State* L, int idx);
void luaL_pushnumber(struct lua_State* L, float number);
void luaL_pushboolean(struct lua_State* L, bool boolean);
int luaL_isnil(struct lua_State* L, int idx);
void luaL_pushnil(struct lua_State* L);
TValue* luaL_index2addr(struct lua_State* L, int idx);

#endif
