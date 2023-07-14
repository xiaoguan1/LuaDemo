
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

/**o是TValue结构体，从TValue结构体中获取Value.gc字段。
 * value.gc字段 本质上就是 CommonHeader gc头部
*/
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
/**判断是否为白色，因为白色有两种：0000 0001 和 0000 0010
 * WHITEBITS是这两种白色的或运算结果（0000 0011）
 * 所以如果 (o)->marked 是白色的话，进行与运算(&) 一定是 大于0。故为true！
*/
#define iswhite(o) testbits((o)->marked, WHITEBITS)

/**判断是否为灰色，因为灰色就一种：0000 0000，也就是数值0。
 * bitmask(BLACKBIT) | WHITEBITS) 黑色和白色的或运算 0000 0100 | 0000 0011 = 0000 0111
 * 如果 testbits((o)->marked, bitmask(BLACKBIT) | WHITEBITS) 为false，则表示既不是黑色也不是白色。那就是灰色！
*/
#define isgray(o) (!testbits((o)->marked, bitmask(BLACKBIT) | WHITEBITS))

/**判断是否为黑色，其思路和iswhite宏一样*/
#define isblack(o) testbit((o)->marked, bitmask(BLACKBIT))

/**
 * 判读是否不可达(是否已死)
 * 		true：不可达(已死)
 * 		false：可达(未死)
 * 
 * 解析：
 * 		m：代表颜色、ow 表示判断是为不可达的标志位(目前是1)
 * 
 * 		“m ^ WHITEBITS” 表示只处理白色[01(二进制)、10(二进制)] 且 取相反数!
 * 			注释：黑色必活(可达)，因为其结果是true
 * 
 * 		“& (ow)” 表示对 二进制的1号位置 进行与操作，如果是true 则表示可达
 * 			注意：
 * 				上面的异或操作，已经按位取反了。
 * 				当然，也可以ow用(0000 0010) 但前面的！要去掉
*/
#define isdeadm(ow, m) (!((m ^ WHITEBITS) & (ow)))


// 颜色转换
/**白色转灰色（前提条件 o->marked是白色）
 * 对bitmask(WHITEBITS)进行非操作 (lu_byte)(~bitmask(WHITEBITS)) 得到 1111 1100
 * (o)->marked 和 (lu_byte)(~WHITEBITS) 进行与运算，得出的结果为0！
 * 注释:灰色为0
*/
#define white2gray(o) resetbits((o)->marked, WHITEBITS)	// 白色转灰色

/**灰色转黑色（前提条件 o->marked是灰色）
 * bitmask(BLACKBIT) 得到 0000 0100 （黑色为4）
 * (o)->marked 和 bitmask(BLACKBIT) 进行或运算，即可得到黑色！
*/
#define gray2black(o) l_setbit((o)->marked, BLACKBIT)	// 灰色转黑色

/**黑色转灰色（前提条件 o->marked是黑色）
 * 对bitmask(BLACKBIT)进行非操作 (lu_byte)(~bitmask(BLACKBIT)) 得到 1111 1011
 * (o)->marked 和 (lu_byte)(~bitmask(BLACKBIT)) 进行与运算，得出的结果为0！
 * 
 * 注释：black2gray宏 和 white2gray宏 的思路是一样的!
*/
#define black2gray(o) resetbit((o)->marked, BLACKBIT)	// 黑色转灰色


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
