#ifndef luaobject_h
#define luaobject_h

#include "lua.h"

typedef struct lua_State lua_State;

typedef LUA_INTEGER lua_Integer;
typedef LUA_NUMBER lua_Number;
typedef unsigned char lu_byte;
typedef int (*lua_CFunction)(lua_State* L);


typedef union lua_Value {
	void* p;			// light userdata
	int b;				// boolean: 1 = true, 0 = false
	lua_Integer i;
	lua_Number n;
	lua_CFunction f;
} Value;

typedef struct lua_TValue {
	Value value_;
	int tt_;
} TValue;


#endif