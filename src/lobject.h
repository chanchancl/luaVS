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
** bits 4-5: variant bits  ��ͬ�����ͣ����Ų�ͬ�ĺ���
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
// �ֱ���� Lua function�� light C function�� regular C function
// light C��Ӧ�þ���һ����һ����ָ��
// �� regular �Ǻ��бհ���
// ������λ������ tag �� 4-5λ
#define LUA_TLCL	(LUA_TFUNCTION | (0 << 4))  /* Lua closure */
#define LUA_TLCF	(LUA_TFUNCTION | (1 << 4))  /* light C function */
#define LUA_TCCL	(LUA_TFUNCTION | (2 << 4))  /* C closure */


/* Variant tags for strings */
// �ַ���
// ���ַ��������ַ���
#define LUA_TSHRSTR	(LUA_TSTRING | (0 << 4))  /* short strings */
#define LUA_TLNGSTR	(LUA_TSTRING | (1 << 4))  /* long strings */


/* Variant tags for numbers */
// ����
// ������������
#define LUA_TNUMFLT	(LUA_TNUMBER | (0 << 4))  /* float numbers */
#define LUA_TNUMINT	(LUA_TNUMBER | (1 << 4))  /* integer numbers */


/* Bit mark for collectable types */
// ��gc
#define BIT_ISCOLLECTABLE	(1 << 6)

/* mark a tag as collectable */
// ʹ t ����ʶΪ ��gc
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
// ע�⣬����� GCObject ������һ�� Header
// һ�������� GCObject����һ�� union GCUnion ��СΪ 120 �ֽ�
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

// ���� ֵ �� ����
#define TValuefields	Value value_; int tt_

// Lua�еı���
typedef struct lua_TValue {
  TValuefields;
} TValue;



/* macro defining a nil value */
#define NILCONSTANT	{NULL}, LUA_TNIL

// ��� TValue o �� ֵ value_ (union Value)
#define val_(o)		((o)->value_)


/* raw type tag of a TValue */
// ��� TValue o �� ���� tt_
#define rttype(o)	((o)->tt_)

/* tag with no variants (bits 0-3) */
// ��ȡ�������͵� 0-3 �ĸ�bit
//
#define novariant(x)	((x) & 0x0F)

/* type tag of a TValue (bits 0-3 for tags + variant bits 4-5) */
// o & 2^6-1  ֻȡ o �� 0-5 ��λ (Ҳ����˵������ �Ƿ��gc)
#define ttype(o)	(rttype(o) & 0x3F)

/* type tag of a TValue with no variants (bits 0-3) */
// �� novariantһ��
#define ttnov(o)	(novariant(rttype(o)))


/* Macros to test type */
// ��� tag o,t �Ƿ���ȫһ��
#define checktag(o,t)		(rttype(o) == (t))
// ��� o,t �����Ƿ�һ��
#define checktype(o,t)		(ttnov(o) == (t))
// ����ǲ��� number,float,integer
// LUA_TNUMFLT == LUA_TNUMBER | 0 << 4  �� 0-3��ʾ number��4-5λ����ʾ����float
// integer ͬ��
#define ttisnumber(o)		checktype((o), LUA_TNUMBER)
#define ttisfloat(o)		checktag((o), LUA_TNUMFLT)
#define ttisinteger(o)		checktag((o), LUA_TNUMINT)
// ��� �ǲ���nil
#define ttisnil(o)		checktag((o), LUA_TNIL)
// boolean
#define ttisboolean(o)		checktag((o), LUA_TBOOLEAN)
// light user data ���Ŀǰ��û�㶮����ʲô��
#define ttislightuserdata(o)	checktag((o), LUA_TLIGHTUSERDATA)
// string
#define ttisstring(o)		checktype((o), LUA_TSTRING)
// ע�⣬�����￪ʼ���õ��� ctb �ˣ���ʾ��������ͣ�����deadkey���ǿ�gc����
// shr string ��ʲô������ŶŶŶ������ short string��
#define ttisshrstring(o)	checktag((o), ctb(LUA_TSHRSTR))
// long string
#define ttislngstring(o)	checktag((o), ctb(LUA_TLNGSTR))
// ����ǲ��� table
#define ttistable(o)		checktag((o), ctb(LUA_TTABLE))
// ��� function
#define ttisfunction(o)		checktype(o, LUA_TFUNCTION)
// ��� �ǲ��Ǳհ� o & 2^5   0-5 λ
#define ttisclosure(o)		((rttype(o) & 0x1F) == LUA_TFUNCTION)
// ��� C �����հ����� Lua �հ�
#define ttisCclosure(o)		checktag((o), ctb(LUA_TCCL))
#define ttisLclosure(o)		checktag((o), ctb(LUA_TLCL))
// light c function
#define ttislcf(o)		checktag((o), LUA_TLCF)
// user data?
#define ttisfulluserdata(o)	checktag((o), ctb(LUA_TUSERDATA))
// ��� �ǲ��� thread
#define ttisthread(o)		checktag((o), ctb(LUA_TTHREAD))
// deadkey�� ��table�����
#define ttisdeadkey(o)		checktag((o), LUA_TDEADKEY)


/* Macros to access values */
// check_exp(c,e) �� ���� lua_assert�� ������£��ȶ��� c , Ȼ������ֵ e
// ���û���壬��ֱ����ֵ e
// integer value
#define ivalue(o)	check_exp(ttisinteger(o), val_(o).i)
// float value
#define fltvalue(o)	check_exp(ttisfloat(o), val_(o).n)
// number value�������� int �� float
#define nvalue(o)	check_exp(ttisnumber(o), \
	(ttisinteger(o) ? cast_num(ivalue(o)) : fltvalue(o)))
// gc value, ��һ��ָ�� GCObject ��ָ��
#define gcvalue(o)	check_exp(iscollectable(o), val_(o).gc)
// light user data,  һ�� void* ָ��
#define pvalue(o)	check_exp(ttislightuserdata(o), val_(o).p)
// ��� string �� value
// ������ gco2ts �꣬�� GCObject* ת��Ϊ GCUnion �е� TString
// ��Ϊ GCUnion �еĳ�Ա������ CommonHeader �����ڴ沼��һ�£�����ֱ��ת��
#define tsvalue(o)	check_exp(ttisstring(o), gco2ts(val_(o).gc))
// full user data, ת��Ϊ UData
#define uvalue(o)	check_exp(ttisfulluserdata(o), gco2u(val_(o).gc))
// union Closure
#define clvalue(o)	check_exp(ttisclosure(o), gco2cl(val_(o).gc))
// Closure.LClosure
#define clLvalue(o)	check_exp(ttisLclosure(o), gco2lcl(val_(o).gc))
// Closure.CClosure
#define clCvalue(o)	check_exp(ttisCclosure(o), gco2ccl(val_(o).gc))
// light c function,  f(unction)value
#define fvalue(o)	check_exp(ttislcf(o), val_(o).f)
// ת��Ϊ struct Table
#define hvalue(o)	check_exp(ttistable(o), gco2t(val_(o).gc))
// boolean value
#define bvalue(o)	check_exp(ttisboolean(o), val_(o).b)
// ת��Ϊ struct lua_State, GCUnion �б�ע��
#define thvalue(o)	check_exp(ttisthread(o), gco2th(val_(o).gc))
/* a dead value may get the 'gc' field, but cannot access its contents */
#define deadvalue(o)	check_exp(ttisdeadkey(o), cast(void *, val_(o).gc))

// ����false,  false == nil || false
#define l_isfalse(o)	(ttisnil(o) || (ttisboolean(o) && bvalue(o) == 0))

// �Ƿ�� gc
#define iscollectable(o)	(rttype(o) & BIT_ISCOLLECTABLE)


/* Macros for internal tests */
// ��� gc���� ���ⲿ����(TValue)���� gc �ڲ�����(GCUnion)�Ƿ�һ�¡�����
#define righttt(obj)		(ttype(obj) == gcvalue(obj)->tt)

// ���obj�Ƿ���
// ����gc  ����  ��������ȷ����û��dead��
#define checkliveness(L,obj) \
	lua_longassert(!iscollectable(obj) || \
		(righttt(obj) && (L == NULL || !isdead(G(L),gcvalue(obj)))))


/* Macros to set values */
// �������� tt_
#define settt_(o,t)	((o)->tt_=(t))

// ���� obj Ϊ float x
#define setfltvalue(obj,x) \
  { TValue *io=(obj); val_(io).n=(x); settt_(io, LUA_TNUMFLT); }

// �ı� float Ϊ x
#define chgfltvalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisfloat(io)); val_(io).n=(x); }

// int
#define setivalue(obj,x) \
  { TValue *io=(obj); val_(io).i=(x); settt_(io, LUA_TNUMINT); }

#define chgivalue(obj,x) \
  { TValue *io=(obj); lua_assert(ttisinteger(io)); val_(io).i=(x); }

// ���ÿ�ֵ nil
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

// obj = x��
// obj.tt_ = ctb(x.tt_)
#define setgcovalue(L,obj,x) \
  { TValue *io = (obj); GCObject *i_g=(x); \
    val_(io).gc = i_g; settt_(io, ctb(i_g->tt)); }

// string
// ����о���һ��Ӧ������һ�����ͼ�⣬������Ͳ����Ļ����������ᱨ��
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
// �� x ��ֵ�� obj
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
  CommonHeader; // gc ����
  lu_byte extra;  /* reserved words for short strings; "has hash" for longs */
  // byte ���ַ����ĳ��� 0 - 2^8-1(255)
  lu_byte shrlen;  /* length for short strings */
  // �ַ����� hash 
  unsigned int hash;
  // ��������������²²�:
  // 1. ���ַ�����Ҫ�õ� lnglen�����Բ������ hash table ��
  // 2. ���ַ���(����С�ڵ���255),�п��ܷ��� hash table ��
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
// һ��TString���洢�ַ�����ʵ���ڴ棬������TString����
#define getstr(ts)  \
  check_exp(sizeof((ts)->extra), cast(char *, (ts)) + sizeof(UTString))


/* get the actual string (array of bytes) from a Lua value */
// ���ʵ�ʵ��ַ���
#define svalue(o)       getstr(tsvalue(o))

/* get string length from 'TString *s' */
// �ַ�������,���� short or long ��һ������ 
// ע�� s �� TString*
#define tsslen(s)	((s)->tt == LUA_TSHRSTR ? (s)->shrlen : (s)->u.lnglen)

/* get string length from 'TValue *o' */
// ͬ�ϣ��� o �� TValue*
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
// ��� UserData ��ʵ��ֵ
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
** �ֲ���������Ϣ�� ���� debug
*/
typedef struct LocVar {
  TString *varname; // ��������
  int startpc;  /* first point where variable is active */
  int endpc;    /* first point where variable is dead */
} LocVar;


/*
** Function Prototypes
*/
typedef struct Proto {
  CommonHeader; // GC����
  // �����̶��Ĳ�������
  lu_byte numparams;  /* number of fixed parameters */
  lu_byte is_vararg;  /* 2: declared vararg; 1: uses vararg */
  lu_byte maxstacksize;  /* number of registers needed by this function */
  // upvalue������
  int sizeupvalues;  /* size of 'upvalues' */
  // ����������
  int sizek;  /* size of 'k' */
  // �ֽ���ĳ���
  int sizecode;
  // ӳ����Ϣ������
  int sizelineinfo;
  // ��Ƕ����������
  int sizep;  /* size of 'p' */
  // �ֲ�������Ϣ������
  int sizelocvars;
  // һЩ���� debug����Ϣ
  int linedefined;  /* debug information  */
  int lastlinedefined;  /* debug information  */
  // ����ʹ�õĳ���
  TValue *k;  /* constants used by the function */
  // �ú������ֽ���
  Instruction *code;  /* opcodes */
  // ��Ƕ����
  struct Proto **p;  /* functions defined inside the function */
  // �ֽ��� -> Դ�� ��ӳ��
  int *lineinfo;  /* map from opcodes to source lines (debug information) */
  // �ֲ�������Ϣ
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

// �հ�ͷ
// ˵��: 1.�հ�����gc���ͣ�
//		 2.upvalue �������� gc�б�
#define ClosureHeader \
	CommonHeader; lu_byte nupvalues; GCObject *gclist

// C �����ıհ�
typedef struct CClosure {
  ClosureHeader;
  // ����ʵ��
  lua_CFunction f; 
  // upvalue ����
  TValue upvalue[1];  /* list of upvalues */
} CClosure;


// Lua �����ıհ�
typedef struct LClosure {
  ClosureHeader;
  struct Proto *p;
  UpVal *upvals[1];  /* list of upvalues */
} LClosure;

// �հ����ݽṹ
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


// Table ���ݽṹ
typedef struct Table {
  // GC �����CommonHeader
  CommonHeader;
  lu_byte flags;  /* 1<<p means tagmethod(p) is not present */
  lu_byte lsizenode;  /* log2 of size of 'node' array */
  // ����Ĵ�С
  unsigned int sizearray;  /* size of 'array' array */
  // ����
  TValue *array;  /* array part */
  // ������,Ԫ���� key-value ��ֵ��
  Node *node;
  Node *lastfree;  /* any free position is before this position */
  struct Table *metatable; // Ԫ��
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

