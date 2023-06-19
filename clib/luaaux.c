#include "luaaux.h"
#include "../vm/luado.h"

static void* l_alloc(void* ud, void* ptr, size_t osize, size_t nsize) {
	(void)ud;
	(void)osize;

	if (nsize == 0) {
		free(ptr);
		return NULL;
	}

	return realloc(ptr, nsize);
}


struct lua_State* luaL_newstate() {
	struct lua_State* L = lua_newstate(&l_alloc, NULL);
	return L;
}

void luaL_close(struct lua_State* L) {
	lua_close(L);
}

void luaL_pushinteger(struct lua_State* L, int integer) {
    lua_pushinteger(L, integer);
}

void luaL_pushcfunction(struct lua_State* L, lua_CFunction f) {
	lua_pushcfunction(L, f);
}

void luaL_pop(struct lua_State* L) {
	lua_pop(L);
}

bool luaL_toboolean(struct lua_State* L, int idx) {
	lua_pushboolean(L, idx);
}

void luaL_pushboolean(struct lua_State* L, bool boolean) {
	lua_pushboolean(L, boolean);
}

int luaL_stacksize(struct lua_State* L) {
	return lua_gettop(L);
}

int luaL_isnil(struct lua_State* L, int idx) {
	return lua_isnil(L, idx);
}

void luaL_pushnumber(struct lua_State* L, float number) {
	lua_pushnumber(L, number);
}

lua_Integer luaL_tointeger(struct lua_State* L, int idx) {
	int isnum = 0;
	lua_Integer ret = lua_tointegerx(L, idx, &isnum);
	return ret;
}

lua_Number luaL_tonumber(struct lua_State* L, int idx) {
	int isnum = 0;
	lua_Number ret = lua_tonumberx(L, idx, &isnum);
	return ret;
}

// 保存调用信息临时数据的一个结构
typedef struct CallS {
	StkId func;
	int nresult;
} CallS;

static int f_call(lua_State* L, void* ud) {
	CallS* c = cast(CallS*, ud);
	luaD_call(L, c->func, c->nresult);
	return LUA_OK;
}

/**
 * 参数narg表示： 被调用的函数有多少个参数。
 * 参数nresult表示：期望有多少个返回值。
*/
int luaL_pcall(struct lua_State* L, int narg, int nresult) {
	int status = LUA_OK;
	CallS c;

	// 通过narg个参数来推断出被调用函数在栈中的位置
	c.func = L->top - (narg + 1);

	c.nresult = nresult;

	status = luaD_pcall(L, &f_call, &c, savestack(L, L->top), 0);
	return status;
}