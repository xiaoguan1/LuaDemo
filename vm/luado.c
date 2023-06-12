#include "luado.h"

void luaD_checkstack(struct lua_State* L, int need) {
	if (L->top + need > L->stack_last){
		// 现有的栈空间+需要的栈空间 已经大于 限制数量 则进行扩充
		luaD_growstack(L, need);
	}
}

// 拓展lua_State栈空间
void luaD_growstack(struct lua_State* L, int size) {
	if (L->stack_size > LUA_MAXSTACK) {
		luaD_throw(L, LUA_ERRERR);
	}

	int stack_size = L->stack_size * 2;
	int need_size = cast(int, L->top - L->stack) + size + LUA_EXTRASTACK;
	if (stack_size < need_size) {
		stack_size = need_size;
	}

	// 对 新的栈尺寸大小 进行检查比较
	if (stack_size > LUA_MAXSTACK) {
		stack_size = LUA_MAXSTACK + LUA_ERRORSTACK;
		LUA_ERROR(L, "stack overflow");
	}

	// 保存旧的L->stack地址
	TValue* old_stack = L->stack;

	/**
	 *  根据新的stack_size栈大小，进行扩充！
	 *  根据realloc特性，会保存原有的数据！(可复看下luaL_newstate函数)
	 **/
	L->stack = luaM_realloc(L, L->stack, L->stack_size, stack_size * sizeof(TValue));

	L->stack_size = stack_size;
	L->stack_last = L->stack + stack_size - LUA_EXTRASPACE;

	//top_diff 是 L->stack 扩充前的 长度距离(两指针) 
	int top_diff = cast(int, L->top - old_stack);
	// 根据 top_diff 的指针距离，更新L->top！
	L->top = restorestack(L, top_diff);

	// 更新每一个CallInfo调用的栈空间地址
	struct CallInfo* ci;
	ci = &L->base_ci;
	while(ci) {
		int func_diff = cast(int, ci->func - old_stack);
		int top_diff = cast(int, ci->top - old_stack);
		ci->func = restorestack(L, func_diff);
		ci->top = restorestack(L, top_diff);

		ci = ci->next;
	}
}

int luaD_call(struct lua_State* L, StkId func, int nresult) {
	if (++(L->ncalls) > LUA_MAXCALLS) { // lua_State 调用数量检查
		luaD_throw(L, 0);
	}

	if (!luaD_precall(L, func, nresult)) {

	}

	(L->ncalls)--;
	return LUA_OK;
}

static struct CallInfo* next_ci(struct lua_State* L, StkId func, int nresult) {
	struct global_State* g = G(L);	// 没用到

	// 创建CallInfo结构体
	struct CallInfo* ci;
	ci = luaM_realloc(L, NULL, 0, sizeof(struct CallInfo));

	// 初始化处理
	ci->next = NULL;

	/**
	 * 注意：L->ci 一开始是指向的 L->base_ci（可查看luastate.c的stack_init函数）
	 * 
	 * CallInfo结构体之间的关系类似于链表 previous 指向前一个调用，next 指向下一个调用。
	 **/
	ci->previous = L->ci;
	L->ci->next = ci;

	ci->nresult = nresult;
	ci->callstatus = LUA_OK;
	ci->func = func;

	// L->top 的值不能超过 L->ci->top，否则会抛出"stack overflow"的报错，这样做的目的是为了避免溢出!(?????)
	ci->top = L->top + LUA_MINSTACK;

	L->ci = ci;	// lua_State的ci字段只想当前的CallInfo调用

	return ci;
}

int luaD_precall(struct lua_State* L, StkId func, int nresult) {
	switch (func->tt_) {
		case LUA_TLCF:{
			lua_CFunction f = func->value_.f;

			ptrdiff_t func_diff = savestack(L, func);

			// 检查lua_State的空间是否充足，如果不充足，则需要扩展
			luaD_checkstack(L, LUA_MINSTACK);

			func = restorestack(L, func_diff);

			next_ci(L, func, nresult);

			// 对func指向的函数进行调用，并获取实际返回值的数量
			int n = (*f)(L);

			// 处理返回值（校验栈的空间）
			assert(L->ci->func + n <= L->ci->top);

			luaD_poscall(L, L->top - n, n);
			return 1;
		}
			break;
		default:
			break;
	}
	return 0;
}
