#include "luamem.h"
#include "../vm/luado.h"

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
