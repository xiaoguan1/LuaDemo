
#ifndef luagc_h
#define luagc_h

#include "../common/luastate.h"

#define obj2gco(o) (&cast(union GCUnion*, o)->gc)
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


// 颜色
#define WHITE0BIT 0
#define WHITE1BIT 1
#define BLACKBIT 2

// WHITEBITS 相当于 ((1<<0) | (1<<1))
#define WHITEBITS bit2mask(WHITE0BIT, WHITE1BIT)

#define luaC_white(g) (g->currentwhite & WHITEBITS)
#define otherwhite(g) (g->currentwhite ^ WHITEBITS)

// 颜色判断
#define iswhite(o) testbits((o)->marked, WHITEBITS)
#define isgray(o) (!testbits((o)->masked, bitmask(BLACKBIT) | WHITEBITS))
#define isblack(o) testbit((o)->masked, bitmask(BLACKBIT))

#endif

