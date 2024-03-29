#ifndef luaobject_h
#define luaobject_h

#include "lua.h"

typedef struct lua_State lua_State;

typedef LUA_INTEGER lua_Integer;
typedef LUA_NUMBER lua_Number;
typedef unsigned char lu_byte;
typedef int (*lua_CFunction)(lua_State* L);
typedef void* (*lua_Alloc)(void* ud, void* ptr, size_t osize, size_t nsize);

// gc
/**
 * CommonHeader一共有3个字段
 * 		next：在allgc链表中，指定下一个gc对象的指针
 * 		tt_：记录gc对象的类型，不同的gc对象在propagate阶段有不同的处理逻辑
 * 		marked：用来标记gc对象颜色用的（白、灰、黑）
*/
#define CommonHeader struct GCObject* next; lu_byte tt_; lu_byte marked
#define LUA_GCSTEPMUL 200

struct GCObject {
	CommonHeader;
};

typedef union lua_Value {
	struct GCObject* gc;	// gc结构
	void* p;				// light userdata类型变量
	int b;					// 布尔值boolean: 1 = true, 0 = false
	lua_Integer i;			// 整型变量
	lua_Number n;			// 浮点型变量
	lua_CFunction f;		// Light C Functions 类型变量(函数指针)
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


typedef struct TString {
	CommonHeader;		// 这是所有GC对象的公共头部。

	unsigned int hash;	// 该字符串body的哈希值

	/**
	 * TString为长字符串时：
	 * 		extra=0表示该字符串未进行hash运算
	 * 		extra=1时表示该字符串已经进行过hash运算
	 * 
	 * TString为短字符串时：
	 * 		extra=0时表示它需要被gc托管
	 * 		extra=1时表示该字符串不会被gc回收
	 */
	lu_byte extra;

	// 仅对短字符串有效，表示短字符串的长度，既String Body的长度是多少。
	lu_byte shrlen;

	union {
		/**
		 * 当TString为短字符串时：hnext域有效
		 * 		当全局字符串表，有其他字符串hash冲突时，
		 * 		会将这些冲突的TString实例链接成单向链表，
		 * 		hnext的作用则是指向下一个对象在哪个位置。
		 * 
		 * 当TString为长字符串时：lnglen域生效。
		 * 		表示长字符串的长度也就是String Body的长度是多少
		 */
		struct TString* hnext;
		size_t lnglen;
	} u;

	/**
	 * 用来标记String Body起始位置用的!
	 * 并配合shrlen或lnglen，我们能够找到String Body的结束位置在哪里。
	 */
	char contents[1];
} TString;

#endif
