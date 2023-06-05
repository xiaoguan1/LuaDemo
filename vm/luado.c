#include "luado.h"

int luaD_calll(struct lua_State* L, StkId func, int nresult) {
	if (++(L->ncalls) > LUA_MAXCALLS) { // lua_State 调用数量检查
		luaD_throw(L, 0);
	}

	if (!luaD_precall(L, func, nresult)) {

	}

	(L->ncalls)--;
	return LUA_OK;
}


int luaD_precall(struct lua_State* L, StkId func, int nresult) {
	switch (func->tt_) {
		case LUA_TLCF:{
			lua_CFunction f = func->value_.f;

			ptrdiff_t func_diff = savestack(L, func);
			luaD_checkstack(L, LUA_MINSTACK);
			func = restorestack(L, func_diff);

			next_ci(L, func, nresult);
			int n = (*f)(L);
			assert(L->ci->func + n <= L->ci->top);
			return 1;
		}
			break;
		default:
			break;
	}
	return 0;
}
