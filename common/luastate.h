#ifndef luastate_h
#define luastate_h

#include "luaobject.h"

#define LUA_EXTRASPACE sizeof(void*)
#define G(L) ((L)->l_G)

typedef TValue* StkId;

struct CallInfo {
	StkId func;						// 被调用函数在栈中的位置
	StkId top;						// 被调用函数的栈顶位置
	int nresult;					// 有多少个返回值
	int callstatus;					// 调用状态
	struct CallInfo* next;			// 下一个调用
	struct CallInfo* previous;		// 上一个调用
};

typedef struct lua_State {
	StkId stack;					// 栈
	StkId stack_last;				// 从这里开始，栈不能被使用
	StkId top;						// 栈顶，调用函数时动态改变
	int stack_size;					// 栈的整体大小
	struct lua_longjmp* errorjmp;	// 保护模式中，要用到的结构，当异常抛出时，跳出逻辑
	int status;						// lua_State的状态
	struct lua_State* next;			// 下一个lua_State,通常创建协程时会产生
	struct lua_State* previous;
	struct CallInfo base_ci;		// 和lua_State生命周期一致的函数调用信息
	struct CallInfo* ci;			// 当前运作的CallInfo
	struct global_State* l_G;		// global_State指针
	ptrdiff_t errorfunc;			// 错误函数位于栈的哪个位置
	int ncalls;						// 进行多少次函数调用
} lua_State;

typedef struct global_State {
	/**
	 * 我们的lua_State其实是lua thread，某种程度上来说，它也是协程
	*/
	struct lua_State* mainthread;

	lua_Alloc frealloc;				// 一个可以自定义的内存分配函数

	/**
	 * 当我们自定义内存分配器时，可能要用到这个结构，但是我们用官方默认的版本，
	 * 因此它始终是NULL。
	*/
	void* ud;

	/**
	 * 当调用LUA_THROW接口时，如果当前不处于保护模式，那么会直接调用panic函数，
	 * panic函数通常是输出一些关键日志。
	*/
	lua_CFunction panic;
} global_State;

struct lua_State* lua_newstate(lua_Alloc alloc, void* ud);

#endif
