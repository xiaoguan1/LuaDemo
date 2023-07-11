#include "luamem.h"
#include "../vm/luado.h"

/**
 * g->frealloc 是 luaaux.c中的 l_alloc 函数，专门开辟和销毁内存空间！
 * 
 * 注释：global_State中的GCdebt，当大于0时会触发gc机制！
*/
// 在g->frealloc的基础上进行封装
void* luaM_realloc(struct lua_State* L, void* ptr, size_t osize, size_t nsize) {
    struct global_State* g = G(L);
    int oldsize = ptr ? osize : 0;

    // 先g->frealloc 再 *(g->frealloc)
    void* ret = (*g->frealloc)(g->ud, ptr, oldsize, nsize);
    if (ret == NULL && nsize > 0) { // ??? 为什么加了nsize > 0就可以正常跑
        // 内存不足
        luaD_throw(L, LUA_ERRMEM);
    }

    g->GCdebt = g->GCdebt - oldsize + nsize;
    return ret;
}
