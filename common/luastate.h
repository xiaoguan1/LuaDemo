#ifndef luastate_h
#define luastate_h

#include "luaobject.h"

#define LUA_EXTRASPACE sizeof(void*)
#define G(L) ((L)->l_G)

#define STEPMULADJ 200
#define GCSTEPMUL 200
#define GCSTEPSIZE		1024	// 1kb
#define GCPAUSE			100

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
	 * 当前新建GCObject的颜色。
	 * atomic阶段之前是while，atomic阶段之后会被设置为otherwhite。sweep阶段只清理颜色为white的GCObject。
	*/
	lu_byte currentwhite;

	/**
	 * 所有新创建的GCObject都会被放入allgc链表中。
	 * 		在sweep阶段会遍历allgc链表，将标记为白色的GCObject释放掉，
	 * 		将标记为黑色的GCObject重新设置为下一轮GC要被清理的白色。
	*/
	struct GCObject* allgc;

	/** 
	 * 记录当前要被清理或重新标记的GCObject（或者称为 记录当前sweep的进度）
	*/
	struct GCObject** sweepgc;

	/**
	 * 被标记为灰色的GCObject会被放入gray链表
	 * 		首次从白色被标记为灰色的时候，会被放入这个列表，
	 * 		放入这个列表的gc对象，是准备被propagate的对象。
	*/
	struct GCObject* gray;

	/**
	 * 已被标记为黑色的GCObject重新设置为灰色时，会被放入grayagain链表。
	 * 
	 * "向后barrier设置" 机制
	 * 例如：table对象，从黑色变回灰色时，会放入这个链表中。
	 * 		作用是避免table反复在黑色和灰色之间来回切换重复扫描。
	*/
	struct GCObject* grayagain;

	/**
	 * 虚拟机预设内存总大小。
	 * 	totalbytes 并不是虚拟机真实的内存大小，为了避免频繁执行 GC 步骤，
	 * 	Lua 虚拟机会预先"借贷"一部分内存，再将"借贷"值的绝对值加到 totalbytes 变量中，
	 * 	并且让 GCdebt 变量减去“借贷”值的绝对值。
	 * 	
	 * 	每当调用 luaC_newobj 函数、创建新的 GCObject时，会将新开辟的内存大小加到 GCdebt 变量中。
	 * 	当 GCdebt 的值小于0 时，不触发 GC 步骤操作;当GCdebt 变量大于 0 时，触发执行 GC 步骤操作。
	*/
	lu_mem totalbytes;

	/**
	 * GC 的"借贷"值。Lua 虚拟机真实的内存大小是 totalbytes+GCdebt 的结果
	 * 这是一个可以为负数的变量，主要用于控制gc触发的时机。当GCdebt>0时，才能触发gc流程。
	*/
	l_mem GCdebt;

	/**
	 * 一次 GC 步操可能会调用若干次 GC 模块的 single_step 函数。
	 * 每次执行 single_step 函数，处理的内存总量会被记录在 GCmemtrav 变量中。
	 * 每当 single_step 函数执行完时，它会返回，然后被置 0。
	 * 返回值会被累加到临时变量中，当临时变量达到一定值的时候会退出 GC 步骤操作
	 * 
	 * 
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

// 作用：中间介质 起到 GCObject 和 lua_State 相互转换
union GCUnion {
	struct GCObject gc;
	lua_State th;
};

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
TValue* index2addr(struct lua_State* L, int idx);

#endif
