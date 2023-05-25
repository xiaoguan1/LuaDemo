#ifndef lua_h
#define lua_h

static int POINTER_SIZE = sizeof(void*);

#if POINTER_SIZE >= 8
#define LUA_INTEGER long
#define LUA_NUMBER double
#else
#define LUA_INTEGER int
#define LUA_NUMBER float
#endif

#endif