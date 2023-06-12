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

	L->ci = &L->base_ci;	// L->ci 指针指向了 L->base_ci 的 CallInfo结构体!!!
	L->ci->func = L->stack;
	L->ci->top = L->stack + LUA_MINSTACK;
	L->ci->previous = L->ci->next = NULL;	// 相当于L->base_ci->previous = L->base_ci->next = NULL;
}

// 创建lua_State
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

#define fromstate(L) (cast(LX*, cast(lu_byte*, (L)) - offsetof(LX, l)))

// 回收栈空间
static void free_stack(struct lua_State* L) {
	global_State* g = G(L);

	// 先g->frealloc 再 *(g->frealloc)
	(*g->frealloc)(g->ud, L->stack, sizeof(TValue), 0);

	L->stack = L->stack_last = L->top = NULL;
	L->stack_size = 0;
}

// 回收lua_State
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


//参数入栈
void setivalue(StkId target, int integer){
	target->value_.i = integer;
	target->tt_ = LUA_NUMINT;
}

void setfvalue(StkId target, lua_CFunction f){
	target->value_.f = f;
	target->tt_ = LUA_TLCF;
}

void setfltvalue(StkId target, float number){
	target->value_.n = number;
	target->tt_ = LUA_NUMFLT;
}

void setbvalue(StkId target, bool b){
	target->value_.b = b;
	target->tt_ = LUA_TBOOLEAN;
}

void setnilvalue(StkId target){
	target->tt_ = LUA_TNIL;
}

void setpvalue(StkId target, void* p){
	target->value_.p = p;
	target->tt_ = LUA_TLIGHTUSERDATA;
}

void setobj(StkId target, StkId value){
	target->value_ = value->value_;
	target->tt_ = value->tt_;
}

void increate_top(struct lua_State* L) {
	L->top++;
	assert(L->top <= L->ci->top);
}

void lua_pushinteger(struct lua_State* L, int integer){
	setivalue(L->top, integer);
	increate_top(L);
}

void lua_pushcfunction(struct lua_State* L, lua_CFunction f) {
	setfvalue(L->top, f);
	increate_top(L);
}

void lua_pushnumber(struct lua_State* L, float number) {
	setfltvalue(L->top, number);
	increate_top(L);
}

void lua_pushboolean(struct lua_State* L, bool b) {
	setbvalue(L->top, b);
	increate_top(L);
}

void lua_pushnil(struct lua_State* L) {
	setnilvalue(L);
	increate_top(L);
}

void lua_pushlightuserdata(struct lua_State* L, void* p) {
	setpvalue(L, p);
	increate_top(L);
}

// 出栈
int lua_gettop(struct lua_State* L) {
	return cast(int, L->top - (L->ci->func + 1));
}

void lua_settop(struct lua_State* L, int idx) {
	StkId func = L->ci->func;
	if (idx >= 0) {
		assert(idx <= L->stack_last - (func + 1));
		while (L->top < (func + 1) + idx) {
			setnilvalue(L->top++);
		}
		L->top = func + 1 + idx;
	}else{
		assert(L->top + idx > func);
		L->top = L->top + idx;
	}
}

void lua_pop(struct lua_State* L) {
	lua_settop(L, -1);
}

// 根据索引获取
static TValue* index2addr(struct lua_State* L, int idx) {
	if (idx >= 0) {
		assert(L->ci->func + idx < L->ci->top);
		return L->ci->func + idx;
	}else{
		assert(L->ci->top + idx > L->ci->func);
		return L->ci->top + idx;
	}
}

// 获取栈的值
lua_Integer lua_tointegerx(struct lua_State* L, int idx, int* isnum) {
	lua_Integer ret = 0;
	TValue* addr = index2addr(L, idx);
	if (addr->tt_ == LUA_NUMINT) {
		ret = addr->value_.i;
		*isnum = 1;
	}else{
		*isnum = 0;
		LUA_ERROR(L, "can not convert to integer!\n");
	}
	return ret;
}

lua_Number lua_tonumberx(struct lua_State* L, int idx, int* isnum) {
	lua_Number ret = 0.0f;
	TValue* addr = index2addr(L, idx);
	if (addr->tt_ == LUA_NUMFLT) {
		ret = addr->value_.i;
		*isnum = 1;
	}else{
		*isnum = 0;
		LUA_ERROR(L, "can not convert to number!\n");
	}
	return ret;
}

bool lua_toboolean(struct lua_State* L, int idx){
	TValue* addr = index2addr(L, idx);
	// nil值默认为false、其类型只要b为0照样为false（有转换的意味）
	return !(addr->tt_ == LUA_TNIL || addr->value_.b == 0);
}

int lua_isnil(struct lua_State* L, int idx) {
	TValue* addr = index2addr(L, idx);
	return addr->tt_ == LUA_TNIL;
}
