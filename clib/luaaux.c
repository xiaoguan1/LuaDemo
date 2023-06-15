#include "luaaux.h"
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

lua_Integer luaL_tointeger(struct lua_State* L, int idx) {
	int isnum = 0;
	lua_Integer ret = lua_tointegerx(L, idx, &isnum);
	return ret;
}

typedef struct CallS {
	StkId func;
	int nresult;
} CallS;

static int f_call(lua_State* L, void* ud) {
	CallS* c = cast(CallS*, ud);
	luaD_call(L, c->func, c->nresult);
	return LUA_OK;
}

int luaL_pcall() {

}