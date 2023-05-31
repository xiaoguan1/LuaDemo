#include "luastate.h"

typedef struct LX {
	lu_byte extra_[LUA_EXTRASPACE];
	lua_State l;
} LX;

typedef struct LG {
	LX l;
	global_State g;
} LG;

static void stack_init(struct lua_State* L) {
	L->stack = (StkId)luaM_realloc(L, NULL, 0, LUA_STACKSIZE * sizeof(TValue));
	L->stack_size = LUA_STACKSIZE;
	L->stack_last = L->stack + LUA_STACKSIZE - LUA_EXTRASTACK;
	L->next = L->previous = NULL;
	L->status = LUA_OK;
	L->errorjmp = NULL;
	L->top = L->stack;
	L->errorfunc = 0;

	int i;
	for (i = 0; i < L->stack_size; i++) {
		setnilvalue(L->stack + i);
	}
	L->top ++;

	L->ci = &L->base_ci;
	L->ci->func = L->stack;
	L->ci->top = L->stack + LUA_MINSTACK;
	L->ci->previous = L->ci->next = NULL;
}

struct lua_State* lua_newstate(lua_Alloc alloc, void* ud){
	struct global_State* g;
	struct lua_State* L;

	struct LG* lg = (struct LG*)(*alloc)(ud, NULL, LUA_TTHREAD, sizeof(struct LG));
	if (!lg) {
		return NULL;
	}

	g = &lg->g;
	g->ud = ud;
	g->frealloc = alloc;
	g->panic = NULL;

	L = &lg->l.l;
	G(L) = g;
	g->mainthread = L;

	stack_init(L);
	return L;
}


#define fromstate(L) (cast(LX*, cast(lu_byte*, (L)) - offsetof(LX, 1)))

// 回收栈空间
static void free_stack(struct lua_State* L) {
	global_State* g = G(L);

	// 先g->frealloc 再 *(g->frealloc)
	(*g->frealloc)(g->ud, L->stack, sizeof(TValue), 0);

	L->stack = L->stack_last = L->top = NULL;
	L->stack_size = 0;
}

void lua_close(struct lua_State* L) {
	struct global_State* g = G(L);
	struct lua_State* L1 = g->mainthread;

	struct CallInfo* ci = &L1->base_ci;
	while (ci->next) {
		struct CallInfo* next = ci->next->next;
		struct CallInfo* free_ci = ci->next;

		(*g->frealloc)(g->ud, free_ci, sizeof(struct CallInfo), 0);
		ci = next;
	}

	free_stack(L1);
	(*g->frealloc)(g->ud, fromstate(L1), sizeof(LG), 0);
}

