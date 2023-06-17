#ifndef luado.h
#define luado.h

#include "../common/luastate.h"
#include "../common/luamem.h"
#include <setjmp.h>

typedef int (*Pfunc)(struct lua_State* L, void* ud);    // 函数指针

int luaD_call(struct lua_State* L, StkId func, int nresult);

int luaD_precall(struct lua_State* L, StkId func, int nresult);

int luaD_poscall(struct lua_State* L, StkId first_result, int nresult);

int luaD_pcall(struct lua_State* L, Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef);

int luaD_rawrunprotected(struct lua_State* L, Pfunc f, void* ud);

#endif