#include "luado.h"

void seterrobj(struct lua_State* L, int error) {
	lua_pushinteger(L, error);
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

void luaD_checkstack(struct lua_State* L, int need) {
	if (L->top + need > L->stack_last){
		// 现有的栈空间+需要的栈空间 已经大于 限制数量 则进行扩充
		luaD_growstack(L, need);
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
	ci->top = L->top + LUA_MINSTACK;	// 给本次调用预留的栈空间

	L->ci = ci;	// lua_State的ci字段只想当前的CallInfo调用

	return ci;
}

/**
 * 参数first_result：记录第一个返回值在栈中的位置
 * 参数nresult：是“实际的返回数量(返回值)”
 * 
 * 变量nwant：是“期望返回数量(返回值)”
*/
int luaD_poscall(struct lua_State* L, StkId first_result, int nresult) {
	StkId func = L->ci->func;
	int nwant = L->ci->nresult;

	switch(nwant) {
		case 0: {	// 期望返回0个，直接封顶！
			L->top = L->ci->func;
		} break;
		case 1:{	// 期望只返回1个！
			if (nresult == 0) {// 期望只返回1个,但实际上没有返回。故进行NULL处理。
				first_result->value_.p = NULL;
				first_result->tt_ = LUA_TNIL;
			}
			setobj(func, first_result);	// 把first_result 赋值给func
			first_result->value_.p = NULL;
			first_result->tt_ = LUA_TNIL;

			L->top = func + nwant;
		} break;
		case LUA_MULRET:{
			int nres = cast(int, L->top - first_result);// 第一个返回值和栈顶的距离表示返回数量
			int i;
			for (i = 0; i < nres; i++) {
				StkId current = first_result + i;
				setobj(func + i, current);
				current->value_.p = NULL;
				current->tt_ = LUA_TNIL;
			}
			L->top = func + nres;
		} break;
		default:
			if (nwant > nresult) {
				// 期望返回数量 > 实际返回数量，按期望返回数量为准！
				int i;
				for (i = 0; i < nwant; i++) {
					if (i < nresult) {
						StkId current = first_result + i;
						setobj(func + i, current);
						current->value_.p = NULL;
						current->tt_ = LUA_TNIL;
					}else{
						StkId stack = func + i;
						stack->tt_ = LUA_TNIL;
					}
				}
				L->top = func + nwant;
			}else{
				int i;
				for (i = 0; i < nresult; i++) {
					if (i < nwant) {
						// 返回数量 小于等于 nwant 数量，正常保存！
						StkId current = first_result + i;
						setobj(func + i, current);
						current->value_.p = NULL;
						current->tt_ = LUA_TNIL;
					}else{
						// 多出的返回数量，进行NULL处理！
						StkId stack = func + i;
						stack->value_.p = NULL;
						stack->tt_ = LUA_TNIL;
					}
				}
				L->top = func + nresult;
			} break;
	}

	// 丢弃掉本次已被调用完的CallInfo
	struct CallInfo* ci = L->ci;
	L->ci = ci->previous;			
	L->ci->next = NULL;

	// 释放掉ci指针的内存空间（luaaux.c的l_alloc函数）
	struct global_State* g = G(L);
	(*g->frealloc)(g->ud, ci, sizeof(struct CallInfo), 0);
		
	return LUA_OK;
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

#define LUA_TRY(L, c, a) if (_setjmp((c)->b) == 0) { a; }

#ifdef _WINDOWS_PLATFORM_
#define LUA_THROW(c) longjmp((c)->b, 1)
#else
#define LUA_THROW(c) _longjmp((c)->b, 1)
#endif

// lua_longjmp辅助异常处理(类似try...catch机制)
struct lua_longjmp {
	struct lua_longjmp* previous;
	jmp_buf b;
	int status;
};

int luaD_rawrunprotected(struct lua_State* L, Pfunc f, void* ud) {
	int old_ncalls = L->ncalls;

	struct lua_longjmp lj;
	lj.previous = L->errorjmp;	// previous指针记录前一个还未调用完的lua_longjmp结构
	lj.status = LUA_OK;
	L->errorjmp = &lj;

	// 在try...catch中执行f函数(luaaux.c文件中的f_call函数)
	LUA_TRY(L, L->errorjmp, (*f)(L, ud));

	L->errorjmp = lj.previous;
	L->ncalls = old_ncalls;
	return lj.status;
}

void luaD_throw(struct lua_State* L, int error) {
	struct global_State* g = G(L);
	if (L->errorjmp) {
		L->errorjmp->status = error; // 错误状态信息
		LUA_THROW(L->errorjmp);		 // 执行longjmp 回到setjmp函数执行点
	}else{
		if (g->panic)
			(*g->panic)(L);

		abort();	// 终止程序
	}
}

int luaD_pcall(struct lua_State* L, Pfunc f, void* ud, ptrdiff_t oldtop, ptrdiff_t ef) {
	int status;
	struct CallInfo* old_ci = L->ci;
	ptrdiff_t old_errorfunc = L->errorfunc;

	status = luaD_rawrunprotected(L, f, ud);

	if (status != LUA_OK) {	// 执行f不成功时，则进入。
		struct global_State* g = G(L);
		struct CallInfo* free_ci = L->ci;	// 当前调用函数
		while(free_ci) {
			if (free_ci == old_ci) {
				free_ci = free_ci->next;
				continue;
			}

			/** 目的操作
			 *  (前)链表：CallInfo结构体A <--> CallInfo结构体B <--> CallInfo结构体C
			 *  (后)链表：CallInfo结构体A <--> CallInfo结构体C
			*/
			// 相当于free_ci->previous->next = NULL;
			struct CallInfo* previous = free_ci->previous;
			previous->next = NULL;

			// 回收free_ci指针指向的CallInfo内存，并指向下一个调用。
			struct CallInfo* next = free_ci->next;
			(*g->frealloc)(g->ud,  free_ci, sizeof(struct CallInfo), 0);
			free_ci = next;
		}
		L->ci = old_ci;
		L->top = restorestack(L, oldtop);
		seterrobj(L, status);
	}
	L->errorfunc = old_errorfunc;
	return status;
}
