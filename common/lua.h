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

// basic object type
#define LUA_TNUMBER 1
#define LUA_TLIGHTUSERDATA 2
#define LUA_TBOOLEAN 3
#define LUA_TSTRING 4
#define LUA_TNIL 5
#define LUA_TTABLE 6
#define LUA_TFUNCTION 7
#define LUA_TTHREAD 8
#define LUA_TNONE 9

#endif