#ifndef lua_h
#define lua_h

#include <stdarg.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <assert.h>

#define lua_assert(c) ((void)0)
#define check_exp(c, e) (lua_assert(c), e)

static int POINTER_SIZE = sizeof(void*);

#if POINTER_SIZE >= 8
#define LUA_INTEGER long
#define LUA_NUMBER double
#else
#define LUA_INTEGER int
#define LUA_NUMBER float
#endif

// ERROR CODE
#define LUA_OK 0
#define LUA_ERRERR 1
#define LUA_ERRMEM 2
#define LUA_ERRRUN 3

#define cast(t, exp) ((t)(exp))
#define savestack(L, o) ((o) - (L)->stack)
#define restorestack(L, o) ((L)->stack + (o))

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

// stack define
#define LUA_MINSTACK 20
#define LUA_STACKSIZE (2 * LUA_MINSTACK)
#define LUA_EXTRASTACK 5
#define LUA_MAXSTACK 15000  // lua_State栈的stack_size最大空间
#define LUA_ERRORSTACK 200
#define LUA_MULRET -1
#define LUA_MAXCALLS 200


// error tips
#define LUA_ERROR(L, s) printf("LUA ERROR:%s", s);


// mem define
typedef size_t      lu_mem; // 32位 typedef unsigned int size_t; | 64位 typedef unsigned long size_t;
typedef ptrdiff_t   l_mem;

#define MAX_LUMEM ((lu_mem)(~(lu_mem)0))
#define MAX_LMEM  (MAX_LUMEM >> 1)
#endif