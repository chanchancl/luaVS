/*
** $Id: lobject.h,v 2.116 2015/11/03 18:33:10 roberto Exp $
** Type definitions for Lua objects
** See Copyright Notice in lua.h
*/


#ifndef lobject_h
#define lobject_h


#include <stdarg.h>


#include "llimits.h"
#include "lua.h"


/*
** Extra tags for non-values
*/
#define LUA_TPROTO	LUA_NUMTAGS		/* function prototypes */
#define LUA_TDEADKEY	(LUA_NUMTAGS+1)		/* removed keys in tables */

/*
** number of all possible tags (including LUA_TNONE but excluding DEADKEY)
*/
#define LUA_TOTALTAGS	(LUA_TPROTO + 2)


/*
** tags for Tagged Values have the following use of bits:
** bits 0-3: actual tag (a LUA_T* value)
** bits 4-5: variant bits  不同的类型，有着不同的含义
** bit 6: whether value is collectable
** bit 7 6   5 4  3 2 1 0
**       gc        t a g       
*/


/*
** LUA_TFUNCTION variants:
** 0 - Lua function
** 1 - light C function
** 2 - regular C function (closure)
*/

/* Variant tags for functions */
// 分别代表 Lua function， light C function， regular C function
// light C，应该就是一个单一函数指针
// 而 regular 是含有闭包的
// 左移四位，就是 tag 的 4-5位
#define LUA_TLCL	(LUA_TFUNCTION | (0 << 4))  /* Lua closure */
#define LUA_TLCF	(LUA_TFUNCTION | (1 << 4))  /* light C function */
#define LUA_TCCL	(LUA_TFUNCTION | (2 << 4))  /* C closure */


/* Variant tags for strings */
// 字符串
// 短字符串，长字符串
#define LUA_TSHRSTR	(LUA_TSTRING | (0 << 4))  /* short strings */
#define LUA_TLNGSTR	(LUA_TSTRING | (1 << 4))  /* long strings */


/* Variant tags for numbers */
// 数字
// 浮点数，整数
#define LUA_TNUMFLT	(LUA_TNUMBER | (0 << 4))  /* float numbers */
#define LUA_TNUMINT	(LUA_TNUMBER | (1 << 4))  /* integer numbers */


/* Bit mark for collectable types */
// 可gc
#define BIT_ISCOLLECTABLE	(1 << 6)

/* mark a tag as collectable */
// 使 t 被标识为 可gc
#define ctb(t)			((t) | BIT_ISCOLLECTABLE)


/*
** Common type for all collectable objects
*/
typedef struct GCObject GCObject;


/*
** Common Header for all collectable objects (in macro form, to be
** included in other objects)
*/
#define CommonHeader	GCObject *next; lu_byte tt; lu_byte marked


/*
** Common type has only the common header
*/
// 注意，这里的 GCObject 仅仅是一个 Header
// 一个完整的 GCObject，是一个 union GCUnion 大小为 120 字节
struct GCObject {
  CommonHeader;
};




/*
** Tagged Values. This is the basic representation of values in Lua,
** an actual value plus a tag with its type.
*/

/*
** Union of all Lua values
*/
typedef union Value {
  GCObject *gc;    /* collectable objects */
  void *p;         /* light userdata */
  int b;           /* booleans */
  lua_CFunction f; /* light C functions */
  lua_Integer i;   /* integer numbers */
  lua_Number n;    /* float numbers */
} Value;

// 包括 值 和 类型
#define TValuefields	Value value_; int tt_

// Lua中的变量
typedef struct lua_TValue {
  TValuefields;
} TValue;



/* macro defining a nil value */
#define NILCONSTANT	{NULL}, LUA_TNIL

// 获得 TValue o 的 值 value_ (union Value)
#define val_(o)		((o)->value_)


/* raw type tag of a TValue */
// 获得 TValue o 的 类型 tt_
#define rttype(o)	((o)->tt_)

/* tag with no variants (bits 0-3) */
// 仅获取代表类型的 0-3 四个bit
//
#define novariant(x)	((x) & 0x0F)

/* type tag of a TValue (bits 0-3 for tags + variant bits 4-5) */
// o & 2^6-1  只取 o 的 0-5 六位 (也就是说不包含 是否可gc)
#define ttype(o)	(rttype(o) & 0x3F)

/* type tag of a TValue with no variants (bits 0-3) */
// 和 novariant一样
#define ttnov(o)	(novariant(rttype(o)))


/* Macros to test type */
// 检测 tag o,t 是否完全一致
#define checktag(o,t)		(rttype(o) == (t))
// 检测 o,t 类型是否一样
#define checktype(o,t)		(ttnov(o) == (t))
// 检测是不是 number,float,integer
// LUA_TNUMFLT == LUA_TNUMBER | 0 << 4  即 0-3表示 number，4-5位，表示这是float
// integer 同理
#define ttisnumber(o)		checktype((o), LUA_TNUMBER)
#define ttisfloat(o)		checktag((o), LUA_TNUMFLT)
#define ttisinteger(o)		checktag((o), LUA_TNUMINT)
// 检测 是不是nil
#define ttisnil(o)		checktag((o), LUA_TNIL)
// boolean
#define ttisboolean(o)		checktag((o), LUA_TBOOLEAN)
// light user data 这个目前还没搞懂是做什么的
#define ttislightuserdata(o)	checktag((o), LUA_TLIGHTUSERDATA)
// string
#define ttisstring(o)		checktype((o), LUA_TSTRING)
// 注意，从这里开始，用的是 ctb 了，表示下面的类型，除了deadkey都是可gc类型
// shr string 是什么鬼。。。哦哦哦。。是 short string，
#define ttisshrstring(o)	checktag((o), ctb(LUA_TSHRSTR))
// long string
#define ttislngstring(o)	checktag((o), ctb(LUA_TLNGSTR))
// 检测是不是 table
#define ttistable(o)		checktag((o), ctb(LUA_TTABLE))
// 检测 function
#define ttisfunction(o)		checktype(o, LUA_TFUNCTION)
// 检测 是不是闭包 o & 2^5   0-5 位
#define ttisclosure(o)		((rttype(o) & 0x1F) == LUA_TFUNCTION)
// 检测 C 函数闭包，和 Lua 闭包
#define ttisCclosure(o)		checktag((o), ctb(LUA_TCCL))
#define ttisLclosure(o)		checktag((o), ctb(LUA_TLCL))
// light c function
#define ttislcf(o)		checktag((o), LUA_TLCF)
// user data?
#define ttisfulluserdata(o)	checktag((o), ctb(LUA_TUSERDATA))
// 检测 是不是 thread
#define ttisthread(o)		checktag((o), ctb(LUA_TTHREAD))
// deadkey， 在table里见过
#define ttisdeadkey(o)		checktag((o), LUA_TDEADKEY)


/* Macros to access values */
// check_exp(c,e) 在 定义 lua_assert宏 的情况下，先断言 c , 然后再求值 e
// 如果没定义，则直接求值 e
// integer value
#define ivalue(o)	check_exp(ttisinteger(o), val_(o).i)
// float value
#define fltvalue(o)	check_exp(ttisfloat(o), val_(o).n)
// number value，可以是 int 或 float
#define nvalue(o)	check_exp(ttisnumber(o), \
	(ttisinteger(o) ? cast_num(ivalue(o)) : fltvalue(o)))
// gc value, 是一个指向 GCObject 的指针
#define gcvalue(o)	check_exp(iscollectable(o), val_(o).gc)
// light user data,  一个 void* 指针
#define pvalue(o)	check_exp(ttislightuserdata(o), val_(o).p)
// 获得 string 的 value
// 这里用 gco2ts 宏，把 GCObject* 转换为 GCUnion 中的 TString
// 因为 GCUnion 中的成员，都是 CommonHeader 所以内存布局一致，可以直接转换
#define tsvalue(o)	check_exp(ttisstring(o), gco2ts(val_(o).gc))
// full user data, 转换为 UData
#define uvalue(o)	check_exp(ttisfulluserdata(o), gco2u(val_(o).gc))
// union Closure
#define clvalue(o)	check_exp(ttisclosure(o), gco2cl(val_(o).gc))
// Closure.LClosure
#define clLvalue(o)	check_exp(ttisLclosure(o), gco2lcl(val_(o).gc))
// Closure.CClosure
#define clCvalue(o)	check_exp(ttisCclosure(o), gco2ccl(val_(o).gc))
// light c function,  f(unction)value
#define fvalue(o)	check_exp(ttislcf(o), val_(o).f)
// 转换为 struct Table
#define hvalue(o)	check_exp(ttistable(o), gco2t(val_(o).gc))
// boolean value
#define bvalue(o)	check_exp(ttisboolean(o), val_(o).b)
// 转换为 struct lua_State, GCUnion 中标注的
#define thvalue(o)	check_exp(ttisthread(o), gco2th(val_(o).gc))
/* a dead value may get the 'gc' field, but cannot access its contents */
#define deadvalue(o)	check_exp(ttisdeadkey(o), cast(void *, val_(o).gc))

// 定义false,  false == nil || false
#define l_isfalse(o)	(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))

// 是否可 gc
#define iscollectable(o)	(rttype(o) & BIT_ISCOLLECTABLE)


/* Macros for internal tests */
// 检测 gc类型 的外部类型(TValue)，和 gc 内部类型(GCUnion)是否一致。。。
#define righttt(obj)		(ttype(obj) == gcvalue(obj)->tt)

// 这个不知道干啥
#define checkliveness(L,obj) \
	lua_longassert(!iscollectable(obj) || \
		(righttt(obj) && (L == NULL || !isdead(G(L),gcvalue(obj)))))


/* Macros to set values */
// 设置类型 tt_
#define settt_(o,t)	((o)->tt_=(t))

// 设置 obj 为 float x
#define setfltvalue(obj,x) \
  { TValue *io=(obj); val_(io).n=(x); settt_(io, LUA_TNUMFLT); }

// 改变 float 为 x
#define chgfltvalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisfloat(io)); val_(io).n=(x); }

// int
#define setivalue(obj,x) \
  { TValue *io=(obj); val_(io).i=(x); settt_(io, LUA_TNUMINT); }

#define chgivalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisinteger(io)); val_(io).i=(x); }

// 设置空值 nil
#define setnilvalue(obj) settt_(obj, LUA_TNIL)

// light c function
#define setfvalue(obj,x) \
  { TValue *io=(obj); val_(io).f=(x); settt_(io, LUA_TLCF); }

// light user data
#define setpvalue(obj,x) \
  { TValue *io=(obj); val_(io).p=(x); settt_(io, LUA_TLIGHTUSERDATA); }

// boolean 
#define setbvalue(obj,x) \
  { TValue *io=(obj); val_(io).b=(x); settt_(io, LUA_TBOOLEAN); }

// obj = x，
// obj.tt_ = ctb(x.tt_)
#define setgcovalue(L,obj,x) \
  { TValue *io = (obj); GCObject *i_g=(x); \
    val_(io).gc = i_g; settt_(io, ctb(i_g->tt)); }

// string
// 这里感觉第一行应该是做一个类型检测，如果类型不符的话，编译器会报错
#define setsvalue(L,obj,x) \
  { TValue *io = (obj); TString *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(x_->tt)); \
    checkliveness(L,io); }

// full user data
#define setuvalue(L,obj,x) \
  { TValue *io = (obj); Udata *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TUSERDATA)); \
    checkliveness(L,io); }

// thread
#define setthvalue(L,obj,x) \
  { TValue *io = (obj); lua_State *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTHREAD)); \
    checkliveness(L,io); }

// Lua Closure
#define setclLvalue(L,obj,x) \
  { TValue *io = (obj); LClosure *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TLCL)); \
    checkliveness(L,io); }

// C Closure
#define setclCvalue(L,obj,x) \
  { TValue *io = (obj); CClosure *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TCCL)); \
    checkliveness(L,io); }

// Table
#define sethvalue(L,obj,x) \
  { TValue *io = (obj); Table *x_ = (x); \
    val_(io).gc = obj2gco(x_); settt_(io, ctb(LUA_TTABLE)); \
    checkliveness(L,io); }

// deadkey
#define setdeadvalue(obj)	settt_(obj, LUA_TDEADKEY)


// obj1 = obj2
#define setobj(L,obj1,obj2) \
	{ TValue *io1=(obj1); *io1 = *(obj2); \
	  (void)L; checkliveness(L,io1); }


/*
** different types of assignments, according to destination
*/

/* from stack to (same) stack */
#define setobjs2s	setobj
/* to stack (not from same stack) */
#define setobj2s	setobj
#define setsvalue2s	setsvalue
#define sethvalue2s	sethvalue
#define setptvalue2s	setptvalue
/* from table to same table */
#define setobjt2t	setobj
/* to new object */
#define setobj2n	setobj
#define setsvalue2n	setsvalue

/* to table (define it as an expression to be used in macros) */
#define setobj2t(L,o1,o2)  ((void)L, *(o1)=*(o2), checkliveness(L,(o1)))




/*
** {======================================================
** types and prototypes
** =======================================================
*/


typedef TValue *StkId;  /* index to stack elements */




/*
** Header for string value; string bytes follow the end of this structure
** (aligned according to 'UTString'; see next).
*/
typedef struct TString {
  CommonHeader; // gc 对象
  lu_byte extra;  /* reserved words for short strings; "has hash" for longs */
  // byte 短字符串的长度 0 - 2^8-1(255)
  lu_byte shrlen;  /* length for short strings */
  // 字符串的 hash 
  unsigned int hash;
  // 从这里可以做如下猜测:
  // 1. 长字符串需要用到 lnglen，所以不会放在 hash table 里
  // 2. 短字符串(长度小于等于255),有可能放在 hash table 里
  union {
    size_t lnglen;  /* length for long strings */
    struct TString *hnext;  /* linked list for hash table */
  } u;
} TString;


/*
** Ensures that address after this type is always fully aligned.
*/
typedef union UTString {
  L_Umaxalign dummy;  /* ensures maximum alignment for strings */
  TString tsv;
} UTString;


/*
** Get the actual string (array of bytes) from a 'TString'.
** (Access to 'extra' ensures that value is really a 'TString'.)
*/
// 一个TString，存储字符串的实际内存，紧跟在TString后面
#define getstr(ts)  \
  check_exp(sizeof((ts)->extra), cast(char *, (ts)) + sizeof(UTString))


/* get the actual string (array of bytes) from a Lua value */
// 获得实际的字符串
#define svalue(o)       getstr(tsvalue(o))

/* get string length from 'TString *s' */
// 字符串长度,根据 short or long 做一个区分 
// 注意 s 是 TString*
#define tsslen(s)	((s)->tt == LUA_TSHRSTR ? (s)->shrlen : (s)->u.lnglen)

/* get string length from 'TValue *o' */
// 同上，但 o 是 TValue*
#define vslen(o)	tsslen(tsvalue(o))


/*
** Header for userdata; memory area follows the end of this structure
** (aligned according to 'UUdata'; see next).
*/
typedef struct Udata {
  CommonHeader;
  lu_byte ttuv_;  /* user value's tag */
  struct Table *metatable;
  size_t len;  /* number of bytes */
  union Value user_;  /* user value */
} Udata;


/*
** Ensures that address after this type is always fully aligned.
*/
typedef union UUdata {
  L_Umaxalign dummy;  /* ensures maximum alignment for 'local' udata */
  Udata uv;
} UUdata;


/*
**  Get the address of memory block inside 'Udata'.
** (Access to 'ttuv_' ensures that value is really a 'Udata'.)
*/
// 获得 UserData 的实际值
#define getudatamem(u)  \
  check_exp(sizeof((u)->ttuv_), (cast(char*, (u)) + sizeof(UUdata)))


#define setuservalue(L,u,o) \
	{ const TValue *io=(o); Udata *iu = (u); \
	  iu->user_ = io->value_; iu->ttuv_ = rttype(io); \
	  checkliveness(L,io); }


#define getuservalue(L,u,o) \
	{ TValue *io=(o); const Udata *iu = (u); \
	  io->value_ = iu->user_; settt_(io, iu->ttuv_); \
	  checkliveness(L,io); }


/*
** Description of an upvalue for function prototypes
*/
typedef struct Upvaldesc {
  TString *name;  /* upvalue name (for debug information) */
  lu_byte instack;  /* whether it is in stack (register) */
  lu_byte idx;  /* index of upvalue (in stack or in outer function's list) */
} Upvaldesc;


/*
** Description of a local variable for function prototypes
** (used for debug information)
** 局部变量的信息， 用作 debug
*/
typedef struct LocVar {
  TString *varname; // 变量名字
  int startpc;  /* first point where variable is active */
  int endpc;    /* first point where variable is dead */
} LocVar;


/*
** Function Prototypes
*/
typedef struct Proto {
  CommonHeader; // GC对象
  // 函数固定的参数个数
  lu_byte numparams;  /* number of fixed parameters */
  lu_byte is_vararg;  /* 2: declared vararg; 1: uses vararg */
  lu_byte maxstacksize;  /* number of registers needed by this function */
  // upvalue的数量
  int sizeupvalues;  /* size of 'upvalues' */
  // 常量的数量
  int sizek;  /* size of 'k' */
  // 字节码的长度
  int sizecode;
  // 映射信息的数量
  int sizelineinfo;
  // 内嵌函数的数量
  int sizep;  /* size of 'p' */
  // 局部变量信息的数量
  int sizelocvars;
  // 一些用于 debug的信息
  int linedefined;  /* debug information  */
  int lastlinedefined;  /* debug information  */
  // 函数使用的常量
  TValue *k;  /* constants used by the function */
  // 该函数的字节码
  Instruction *code;  /* opcodes */
  // 内嵌函数
  struct Proto **p;  /* functions defined inside the function */
  // 字节码 -> 源码 的映射
  int *lineinfo;  /* map from opcodes to source lines (debug information) */
  // 局部变量信息
  LocVar *locvars;  /* information about local variables (debug information) */

  //
  Upvaldesc *upvalues;  /* upvalue information */
  struct LClosure *cache;  /* last-created closure with this prototype */
  TString  *source;  /* used for debug information */
  GCObject *gclist;
} Proto;



/*
** Lua Upvalues
*/
typedef struct UpVal UpVal;


/*
** Closures
*/

// 闭包头
// 说明: 1.闭包属于gc类型，
//		 2.upvalue 的数量， gc列表
#define ClosureHeader \
	CommonHeader; lu_byte nupvalues; GCObject *gclist

// C 函数的闭包
typedef struct CClosure {
  ClosureHeader;
  // 函数实体
  lua_CFunction f; 
  // upvalue 链表
  TValue upvalue[1];  /* list of upvalues */
} CClosure;


// Lua 函数的闭包
typedef struct LClosure {
  ClosureHeader;
  struct Proto *p;
  UpVal *upvals[1];  /* list of upvalues */
} LClosure;

// 闭包数据结构
typedef union Closure {
  CClosure c;
  LClosure l;
} Closure;


#define isLfunction(o)	ttisLclosure(o)

#define getproto(o)	(clLvalue(o)->p)


/*
** Tables
*/

typedef union TKey {
  struct {
    TValuefields;
    int next;  /* for chaining (offset for next node) */
  } nk;
  TValue tvk;
} TKey;


/* copy a value into a key without messing up field 'next' */
#define setnodekey(L,key,obj) \
	{ TKey *k_=(key); const TValue *io_=(obj); \
	  k_->nk.value_ = io_->value_; k_->nk.tt_ = io_->tt_; \
	  (void)L; checkliveness(L,io_); }


typedef struct Node {
  TValue i_val;
  TKey i_key;
} Node;


// Table 数据结构
typedef struct Table {
  // GC 对象的CommonHeader
  CommonHeader;
  lu_byte flags;  /* 1<<p means tagmethod(p) is not present */
  lu_byte lsizenode;  /* log2 of size of 'node' array */
  // 数组的大小
  unsigned int sizearray;  /* size of 'array' array */
  // 数组
  TValue *array;  /* array part */
  // 单链表,元素是 key-value 键值对
  Node *node;
  Node *lastfree;  /* any free position is before this position */
  struct Table *metatable; // 元table ?
  GCObject *gclist;
} Table;



/*
** 'module' operation for hashing (size is always a power of 2)
*/
#define lmod(s,size) \
	(check_exp((size&(size-1))==0, (cast(int, (s) & ((size)-1)))))


#define twoto(x)	(1<<(x))
#define sizenode(t)	(twoto((t)->lsizenode))


/*
** (address of) a fixed nil value
*/
#define luaO_nilobject		(&luaO_nilobject_)


LUAI_DDEC const TValue luaO_nilobject_;

/* size of buffer for 'luaO_utf8esc' function */
#define UTF8BUFFSZ	8

LUAI_FUNC int luaO_int2fb (unsigned int x);
LUAI_FUNC int luaO_fb2int (int x);
LUAI_FUNC int luaO_utf8esc (char *buff, unsigned long x);
LUAI_FUNC int luaO_ceillog2 (unsigned int x);
LUAI_FUNC void luaO_arith (lua_State *L, int op, const TValue *p1,
                           const TValue *p2, TValue *res);
LUAI_FUNC size_t luaO_str2num (const char *s, TValue *o);
LUAI_FUNC int luaO_hexavalue (int c);
LUAI_FUNC void luaO_tostring (lua_State *L, StkId obj);
LUAI_FUNC const char *luaO_pushvfstring (lua_State *L, const char *fmt,
                                                       va_list argp);
LUAI_FUNC const char *luaO_pushfstring (lua_State *L, const char *fmt, ...);
LUAI_FUNC void luaO_chunkid (char *out, const char *source, size_t len);


#endif

