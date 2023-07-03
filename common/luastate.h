#ifndef luastate_h
#define luastate_h

#include "luaobject.h"

#define LUA_EXTRASPACE sizeof(void*)
#define G(L) ((L)->l_G)

typedef TValue* StkId;

union GCUnion {
	struct GCObject gc;
	lua_State th;
};

struct CallInfo {
	StkId func;						// 被调用函数在栈中的位置
	StkId top;						// 被调用函数的栈顶位置
	int nresult;					// 有多少个返回值
	int callstatus;					// 调用状态
	struct CallInfo* next;			// 下一个调用
	struct CallInfo* previous;		// 上一个调用
};

typedef struct lua_State {
	CommonHeader;					// gc的头部

	StkId stack;					// 栈
	StkId stack_last;				// 从这里开始，栈不能被使用
	StkId top;						// 栈顶，调用函数时动态改变
	int stack_size;					// 栈的整体大小
	struct lua_longjmp* errorjmp;	// 保护模式中，要用到的结构，当异常抛出时，跳出逻辑
	int status;						// lua_State的状态
	// struct lua_State* next;			// 下一个lua_State,通常创建协程时会产生
	struct lua_State* previous;
	struct CallInfo base_ci;		// 和lua_State生命周期一致的函数调用信息
	struct CallInfo* ci;			// 当前运作的CallInfo
	int nci;
	struct global_State* l_G;		// global_State指针
	ptrdiff_t errorfunc;			// 错误函数位于栈的哪个位置
	int ncalls;						// 进行多少次函数调用

	/**
	 * 当lua_State对象在gray链表时，它指向gray链表中的下一个灰色对象的地址
	 * 当lua_State对象在grayagain链表时，它指向grayagain链表中下一个灰色对象的地址
	 * 其实就是相当于gray或者grayagain链表中，数据对象的next指针
	 */
	struct GCObject* gclist;
} lua_State;

typedef struct global_State {
	/** 我们的lua_State其实是lua thread，某种程度上来说，它也是协程 */
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

	/** gc的相关字段 **/
	/**
	 * 标记gc当前处于那个阶段
	 * 		阶段分别为：GCSpause、GCSpropagate、GCSatomic、GCSinsideatomic、GCSsweepgc和GCSsweepend
	*/
	lu_byte gcstate;

	/**
	 * 当前gc是哪种白色，10(二进制) 与 01(二进制) 中的一种，在gc的atomic阶段结束时，会从一种切换为另一种。
	*/
	lu_byte currentwhite;

	/**
	 * 这是一个单向链表，所有新建的gc对象，都要放入到这个链表中，放入的方式是直接放在链表的头部。
	*/
	struct GCObject* allgc;

	/**
	 * 这个变量用于记录当前sweep的进度。
	*/
	struct GCObject** sweepgc;

	/**
	 * gc对象，
	 * 首次从白色被标记为灰色的时候，会被放入这个列表，放入这个列表的gc对象，是准备被propagate的对象。
	*/
	struct GCObject* gray;

	/**
	 * "向后barrier设置" 机制
	 * 例如：table对象，从黑色变回灰色时，会放入这个链表中。
	 * 		作用是避免table反复在黑色和灰色之间来回切换重复扫描。
	*/
	struct GCObject* grayagagin;

	/**
	 * 记录开辟内存字节大小的变量之一，真实的大小是totalbytes+GCdebt。
	*/
	lu_mem totalbytes;

	/**
	 * 这是一个可以为负数的变量，主要用于控制gc触发的时机。当GCdebt>0时，才能触发gc流程。
	*/
	l_mem GCdebt;

	/**
	 * 每次进行gc操作时，所遍历的对象字节大小之和，单位是byte，
	 * 当其值大于单步执行的内存上限时，gc终止！！！
	*/
	lu_mem GCmemtrav;

	/**
	 * GCestimate：在sweep阶段结束时，会被重新计算。本质是totalbytes+GCdebt，
	 * 作用：
	 * 		在本轮gc结束时，将自身扩充两倍大小，然后让真实大小减去扩充后的自己得到差debt，
	 * 		然后totalbytes会等于扩充后的自己，而GCdebt则会被负数debt赋值，
	 * 		就是是说下一次执行gc流程，要在有|debt|个bytes内存被开辟后，才会开始。
	 * 
	 * 目的：避免gc太过频繁。
	*/
	lu_mem GCestimate;

	/**  一个和GC单次处理多少字节相关的参数。 */
	int GCstepmul;
} global_State;

struct lua_State* lua_newstate(lua_Alloc alloc, void* ud);
void lua_close(struct lua_State* L);


void setivalue(StkId target, int integer);			// push int
void setfvalue(StkId target, lua_CFunction f);		// push light C function
void setfltvalue(StkId target, float number);		// push float
void setbvalue(StkId target, bool b);				// push bool
void setnilvalue(StkId target);						// push nil
void setpvalue(StkId target, void* p);				// push 指针

void setobj(StkId target, StkId value);				// push TValue

void increate_top(struct lua_State* L);

void lua_pushinteger(struct lua_State* L, int integer);
void lua_pushcfunction(struct lua_State* L, lua_CFunction f);
void lua_pushnumber(struct lua_State* L, float number);
void lua_pushboolean(struct lua_State* L, bool b);
void lua_pushnil(struct lua_State* L);
void lua_pushlightuserdata(struct lua_State* L, void* p);


// 出栈
void lua_settop(struct lua_State* L, int idx);
int lua_gettop(struct lua_State* L);
void lua_pop(struct lua_State* L);

// 获取栈上的值
lua_Integer lua_tointegerx(struct lua_State* L, int id, int* isnum);
lua_Number lua_tonumberx(struct lua_State* L, int idx, int* isnum);
bool lua_toboolean(struct lua_State* L, int idx);
int lua_isnil(struct lua_State* L, int idx);


#endif
