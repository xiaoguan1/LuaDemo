// #include "clib/luaaux.h"

// static int add_op(struct lua_State* L) {
//     int left = luaL_tointeger(L, -2);
//     int right = luaL_tointeger(L, -1);

//     luaL_pushinteger(L, left + right);
//     return 1;
// }

// int main(int argc, char** argv) {
//     struct lua_State* L = luaL_newstate();			// 创建虚拟机状态实例
//     luaL_pushcfunction(L, &add_op);					// 将要被调用的函数add_op入栈
//     luaL_pushinteger(L, 3);							// 参数入栈
//     luaL_pushinteger(L, 1);
//     luaL_pcall(L, 2, 1);							// 调用add_op函数，并将结果push到栈中

//     int result = luaL_tointeger(L, -1);				// 完成函数调用，栈顶就是add_op放入的结果
//     printf("result is %d\n", result);
//     luaL_pop(L);									// 结果出栈，保证栈的正确性

//     printf("final stack size %d\n", luaL_stacksize(L));

//     luaL_close(L);									// 销毁虚拟机状态实例

//     // system("pause");
//     return 0;
// }




#include "./clib/luaaux.h"
#include "./vm/luagc.h"
#include <time.h>

#define ELEMENTNUM 5
int main(int argc, char** argv) {
   struct lua_State* L = luaL_newstate(); 

   int i = 0;
   for (; i < ELEMENTNUM; i ++) {
       luaL_pushnil(L);
   }

   int start_time = time(NULL);
   int end_time = time(NULL);
   size_t max_bytes = 0;
   struct global_State* g = G(L);
   int j = 0;
//    for (; j < 500000000; j ++) {
        // TValue* o = luaL_index2addr(L, (j % ELEMENTNUM) + 1);
        // struct GCObject* gco = luaC_newobj(L, LUA_TSTRING, sizeof(TString));
        // o->value_.gc = gco;
        // o->tt_ = LUA_TSTRING;
        // luaC_checkgc(L);

        // if ((g->totalbytes + g->GCdebt) > max_bytes) {
        //     max_bytes = g->totalbytes + g->GCdebt;
        // }

        // if (j % 1000 == 0) {
        //     printf("timestamp:%d totalbytes:%f kb \n", (int)time(NULL), (float)(g->totalbytes + g->GCdebt) / 1024.0f);
        // }
//    }
//    end_time = time(NULL);
//    printf("finish test start_time:%d end_time:%d max_bytes:%f kb \n", start_time, end_time, (float)max_bytes / 1024.0f);

//    luaL_close(L);


    return 0;
}
