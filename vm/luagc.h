
#ifndef luagc_h
#define luagc_h

#include "../common/luastate.h"

/**
 * lua_State 转换为 GCObject(本质上就是CommonHeader 宏)
 * 注释：
 * 		本质上就是获取 lua_State继承过来的 CommonHeader 宏
 * 		CommonHeader宏 在创建lua_State时有对其进行初始化
 * 		GCUnion联合体只是转化的中间介质
*/
#define obj2gco(o) (&cast(union GCUnion*, o)->gc)

// GCObject(本质上就是CommonHeader 宏) 转换为 lua_State
#define gco2th(o) check_exp((o)->tt_ == LUA_TTHREAD, &cast(union GCUnion*, o)->th)

#define gcvalue(o) ((o)->value_.gc)

// Bit operation
#define bitmask(b) (1 << b)
#define bit2mask(b1, b2) (bitmask(b1) | bitmask(b2))

#define resetbits(x, m) ((x) &= cast(lu_byte, ~(m)))
#define setbits(x, m) ((x) |= (m))
#define testbits(x, m) ((x) & (m))
#define resetbit(x, b) resetbits(x, bitmask(b))
#define l_setbit(x, b) setbits(x, bitmask(b))
#define testbit(x, b) testbits(x, bitmask(b))


// 颜色（移位符、搭配bitmask宏使用）
#define WHITE0BIT 0     // bitmask(WHITE0BIT) 为 0000 0001 白色
#define WHITE1BIT 1     // bitmask(WHITE1BIT) 为 0000 0010 白色
#define BLACKBIT 2      // bitmask(BLACKBIT)  为 0000 0100 黑色

// WHITEBITS 相当于 ((1<<0) | (1<<1))
#define WHITEBITS bit2mask(WHITE0BIT, WHITE1BIT)

// 标记对象为白色
#define luaC_white(g) (g->currentwhite & WHITEBITS)

// 切换不同白色(^ 异或操作)
#define otherwhite(g) (g->currentwhite ^ WHITEBITS)

// 颜色判断
#define iswhite(o) testbits((o)->marked, WHITEBITS)
#define isgray(o) (!testbits((o)->marked, bitmask(BLACKBIT) | WHITEBITS))
#define isblack(o) testbit((o)->marked, bitmask(BLACKBIT))
#define isdeadm(ow, m) (!((m ^ WHITEBITS) & (ow)))

// GCState（gc执行时的状态）
#define GCSpause        0
#define GCSpropagate    1
#define GCSatomic       2
#define GCSinsideatomic 3
#define GCSsweepallgc   4
#define GCSsweepend     5

#define iscollectable(o) \
    ((o)->tt_ == LUA_TTHREAD || \
	 (o)->tt_ == LUA_SHRSTR  || \
	 (o)->tt_ == LUA_LNGSTR  || \
	 (o)->tt_ == LUA_TTABLE  || \
	 (o)->tt_ == LUA_TLCL    || \
	 (o)->tt_ == LUA_TCCL)

#define markobject(L, o) if (iswhite(o)) { reallymarkobject(L, obj2gco(o)); }
#define markvalue(L, o)  if (iscollectable(o) && iswhite(gcvalue(o))) { reallymarkobject(L, gcvalue(o)); }

/**
 * global_State 简称 g
 * lua_State 简称 L
 * 		prev 本质上是 g 中的 gay或者grayagain 字段
 * 
 * 		用途：
 * 			lua_State的gclist 挂载 g->gay 或者 g->grayagain
 * 			g->gay 或者 g->grayagain 挂载 L的 CommonHeader宏
 * 
 * 		作用：构建灰色链表？
*/
#define linkgclist(L, prev) { L->gclist = prev; prev = obj2gco(L); }

/** 函数声明 */
struct GCObject* luaC_newobj(struct lua_State* L, int tt_, size_t size);
void luaC_step(struct lua_State* L);
void reallymarkobject(struct lua_State* L, struct GCObject* gc);
void luaC_freeallobjects(struct lua_State* L);

// gc的检查和处理
#define luaC_condgc(pre, L, pos) if (G(L)->GCdebt > 0) { pre; luaC_step(L); pos; }
#define luaC_checkgc(L) luaC_condgc((void)0, L, (void)0)

#endif
