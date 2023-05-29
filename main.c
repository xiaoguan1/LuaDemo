#include "clib/luaaux.h"

static int add_op(struct lua_State* L) {
    int left = luaL_tointeger(L, 2);
    int right = luaL_tointeger(L, -1);

    luaL_pushinteger(L, left + right);
    return 1;
}

int main(int argc, char** argv) {
    struct lua_State* L = luaL_newstate();			// 创建虚拟机状态实例
    luaL_pushcfunction(L, &add_op);					// 将要被调用的函数add_op入栈
    luaL_pushinteger(L, 1);							// 参数入栈
    luaL_pushinteger(L, 1);
    luaL_pcall(L, 2, 1);							// 调用add_op函数，并将结果push到栈中

    int result = luaL_tointeger(L, -1);				// 完成函数调用，栈顶就是add_op放入的结果
    printf("result is %d\n", result);
    luaL_pop(L);									// 结果出栈，保证栈的正确性

    printf("final stack size %d\n", luaL_stacksize(L));

    luaL_close(L);									// 销毁虚拟机状态实例

    system("pause");
    return 0;
}