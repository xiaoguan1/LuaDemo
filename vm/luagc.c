#include "luagc.h"
#include "../common/luamem.h"

struct GCObject* luaC_newobj(struct lua_State* L, int tt_, size_t size) {
	struct global_State* g = G(L);

	// GCObject 的初始化操作
	struct GCObject* obj = (struct GCObject*)luaM_realloc(L, NULL, 0, size);
	obj->marked = luaC_white(g);
	obj->next = g->allgc; //标记为白色
	obj->tt_ = tt_;	//数据类型
	g->allgc = obj;

	retun obj;
}

void luaC_step(struct lua_State* L) {
	struct global_State* g = G(L);
	l_mem debt = get_debt(L);
	do {
		l_mem work = singlestep(L);
		debt -= work;
	}while(debt > -GCSTEPSIZE && G(L)->gcstate != GCSpause);

	if (G(L)->gcstate == GCSpause) {
		setpause(L);
	}else{
		debt = debt / g->GCstepmul * STEPMULADJ;
		setdebt(L, debt);
	}
}
