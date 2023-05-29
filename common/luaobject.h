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

// lua number type 
#define LUA_NUMINT (LUA_TNUMBER | (0 << 4))	// 十进制数值：1
#define LUA_NUMFLT (LUA_TNUMBER | (1 << 4))	// 十进制数值：17

// lua function type 
#define LUA_TLCL (LUA_TFUNCTION | (0 << 4))	// 十进制数值：7
#define LUA_TLCF (LUA_TFUNCTION | (1 << 4)) // 十进制数值：23
#define LUA_TCCL (LUA_TFUNCTION | (2 << 4)) // 十进制数值：39

// string type 
#define LUA_LNGSTR (LUA_TSTRING | (0 << 4))	// 十进制数值：4
#define LUA_SHRSTR (LUA_TSTRING | (1 << 4))	// 十进制数值：20

#endif

