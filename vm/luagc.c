#include "luagc.h"
#include "../common/luamem.h"

#define GCMAXSWEEPGCO 25

#define gettotalbytes(g) (g->totalbytes + g->GCdebt)
#define sweepwholelist(L, list) sweeplist(L, list, MAX_LUMEM)

struct GCObject* luaC_newobj(struct lua_State* L, int tt_, size_t size) {
	struct global_State* g = G(L);

	// GCObject 的初始化操作(创建gc对象)
	struct GCObject* obj = (struct GCObject*)luaM_realloc(L, NULL, 0, size);
	obj->marked = luaC_white(g); //标记为白色
	obj->tt_ = tt_;	//数据类型

	/** 以头插法的方式，将新创建的元素插入到allgc单链表里面 */
	obj->next = g->allgc;
	g->allgc = obj;

	return obj;
}

void reallymarkobject(struct lua_State* L, struct GCObject* gco) {
    struct global_State* g = G(L);
    white2gray(gco); // 把主线程lua_State的marked颜色从白色变为灰色

    switch(gco->tt_) {
        case LUA_TTHREAD:{
            linkgclist(gco2th(gco), g->gray); // 构建灰色链表
        } break;
        case LUA_TSTRING:{ // just for gc test now
            gray2black(gco);
            g->GCmemtrav += sizeof(struct TString);
        } break;
        default:break;
    }
}

static void setdebt(struct lua_State* L, l_mem debt) {
	struct global_State* g = G(L);
	lu_mem totalbytes = gettotalbytes(g);

	g->totalbytes = totalbytes - debt;
	g->GCdebt = debt;
}

//当内存是当前估计值的两倍时，将触发gc
static void setpause(struct lua_State* L) {
	struct global_State* g = G(L);
	l_mem estimate = g->GCestimate / GCPAUSE;
	estimate = (estimate * g->GCstepmul) >= MAX_LMEM ? MAX_LMEM : estimate * g->GCstepmul;

	l_mem debt = g->GCestimate - estimate;
	setdebt(L, debt);
}

// 返回要gc的字节数量
static l_mem get_debt(struct lua_State* L) {
	struct global_State* g = G(L);
	l_mem debt = g->GCdebt;

	if(debt <= 0) {
		return 0;
	}

	// 注意：+ 1， 是为了防止 debt / STEPMULADJ 是0~1之间的数，然后被int截断得到0！
	// 换个角度想，一次gc至少处理200字节，但至多处理MAX_LMEM字节。
	debt = debt / STEPMULADJ + 1;
	debt = debt >= (MAX_LMEM / STEPMULADJ) ? MAX_LMEM : debt * g->GCstepmul;
	return debt;
} 

// 标记根
static void restart_collection(struct lua_State* L) {
	struct global_State* g = G(L);

	// 重置gray和grayagain链表
	g->gray = g->grayagain = NULL;

	// g->mainthread 主线程的lua_State，目的暂不清楚！
	markobject(L, g->mainthread);
}

static lu_mem traversethread(struct lua_State* L, struct lua_State* th) {
	TValue* o = th->stack;
	for (; o < th->top; o++) {	// 目前o->tt_ 类型是string 或者 nil
		markvalue(L, o);
	}

	return sizeof(struct lua_State) + sizeof(TValue) * th->stack_size + sizeof(struct CallInfo) * th->nci;
}

static void propagatemark(struct lua_State* L) {
	struct global_State* g = G(L);

	if (!g->gray)
		return;

	struct GCObject* gco = g->gray;
	gray2black(gco);	// 主线程lua_State从灰色变为黑色
	lu_mem size = 0;
	switch (gco->tt_) {
		case LUA_TTHREAD:{
			black2gray(gco); // 主线程lua_State从黑色变为灰色
			struct lua_State* th = gco2th(gco);
			g->gray = th->gclist;
			linkgclist(th, g->grayagain);
			size = traversethread(L, th);
		} break;
		default:break;
	}
	g->GCmemtrav += size;
}

static void propagateall(struct lua_State* L) {
	struct global_State* g = G(L);
	while(g->gray) {
		propagatemark(L);
	}
}

static void atomic(struct lua_State* L) {
	struct global_State* g = G(L);
	g->gray = g->grayagain;
	g->grayagain = NULL;

	g->gcstate = GCSinsideatomic;
	propagateall(L);
	g->currentwhite = cast(lu_byte, otherwhite(g));
}

static lu_mem freeobj (struct lua_State* L, struct GCObject* gco) {
	switch (gco->tt_) {
		case LUA_TSTRING: {
			lu_mem sz = sizeof(TString);
			luaM_free(L, gco, sz);
			return sz;
		} break;
		default: {
			lua_assert(0);
		} break;
	}
}

static struct GCObject** sweeplist(struct lua_State* L, struct GCObject** p, size_t count) {
	struct global_State* g = G(L);
	lu_byte ow = otherwhite(g);
	while (*p != NULL && count > 0) {
		lu_byte marked = (*p)->marked;
		if (isdeadm(ow, marked)) {
			struct GCObject* gco = *p;
			*p = (*p)->next;
			g->GCmemtrav += freeobj(L, gco);
		}else{
			(*p)->marked &= cast(lu_byte, ~(bitmask(BLACKBIT) | WHITEBITS));
			(*p)->marked |= luaC_white(g);
			p = &((*p)->next);
		}
		count--;
	}
	return (*p) == NULL ? NULL : p;
}

static void entersweep(struct lua_State* L) {
	struct global_State* g = G(L);
	g->gcstate = GCSsweepallgc;
	g->sweepgc = sweeplist(L, &g->allgc, 1);
}

static void sweepstep(struct lua_State* L) {
	struct global_State* g = G(L);
	if (g->sweepgc) {
		g->sweepgc = sweeplist(L, g->sweepgc, GCMAXSWEEPGCO);
		g->GCestimate = gettotalbytes(g);

		if (g->sweepgc) {
			return;
		}
	}
	g->gcstate = GCSsweepend;
	g->sweepgc = NULL;
}

static lu_mem singlestep(struct lua_State* L) {
	struct global_State* g = G(L);
	switch (g->gcstate) {
		case GCSpause: {
			g->GCmemtrav = 0;
			restart_collection(L);
			g->gcstate = GCSpropagate;
			return g->GCmemtrav;
		} break;
		case GCSpropagate: {
			g->GCmemtrav = 0;
			propagatemark(L);
			if (g->gray == NULL) {
				g->gcstate = GCSatomic;
			}
			return g->GCmemtrav;
		} break;
		case GCSatomic: {
			g->GCmemtrav = 0;
			if (g->gray){
				propagateall(L);
			}
			atomic(L);
			entersweep(L);
			g->GCestimate = gettotalbytes(g);
			return g->GCmemtrav;
		} break;
		case GCSsweepallgc: {
			g->GCmemtrav = 0;
			sweepstep(L);
			return g->GCmemtrav;
		} break;
		case GCSsweepend: {
			g->GCmemtrav = 0;
			g->gcstate = GCSpause;
			return 0;
		} break;
		default: break;
	}
	return g->GCmemtrav;
}

void luaC_step(struct lua_State* L) {
	struct global_State* g = G(L);

	l_mem debt = get_debt(L);	// 获取debt字节大小
	do {
		l_mem work = singlestep(L);
		debt -= work;
	}while(debt > -GCSTEPSIZE && G(L)->gcstate != GCSpause);
	if (G(L)->gcstate == GCSpause) {
		setpause(L);
	}else{
		debt = g->GCdebt / STEPMULADJ * g->GCstepmul;
		setdebt(L, debt);
	}
}

void luaC_freeallobjects(struct lua_State* L) {
	struct global_State* g = G(L);
	g->currentwhite = WHITEBITS;
	sweepwholelist(L, &g->allgc);
}