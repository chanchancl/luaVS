/*
** $Id: lstate.c,v 2.133 2015/11/13 12:16:51 roberto Exp $
** Global State
** See Copyright Notice in lua.h
*/

#define lstate_c
#define LUA_CORE

#include "lprefix.h"


#include <stddef.h>
#include <string.h>

#include "lua.h"

#include "lapi.h"
#include "ldebug.h"
#include "ldo.h"
#include "lfunc.h"
#include "lgc.h"
#include "llex.h"
#include "lmem.h"
#include "lstate.h"
#include "lstring.h"
#include "ltable.h"
#include "ltm.h"


#if !defined(LUAI_GCPAUSE)
#define LUAI_GCPAUSE	200  /* 200% */
#endif

#if !defined(LUAI_GCMUL)
#define LUAI_GCMUL	200 /* GC runs 'twice the speed' of memory allocation */
#endif


/*
** a macro to help the creation of a unique random seed when a state is
** created; the seed is used to randomize hashes.
*/
#if !defined(luai_makeseed)
#include <time.h>
#define luai_makeseed()		cast(unsigned int, time(NULL))
#endif



/*
** thread state + extra space
*/
typedef struct LX {
  lu_byte extra_[LUA_EXTRASPACE];
  lua_State l;
} LX;


/*
** Main thread combines a thread state and the global state
*/
typedef struct LG {
  LX l;
  global_State g;
} LG;



#define fromstate(L)	(cast(LX *, cast(lu_byte *, (L)) - offsetof(LX, l)))


/*
** Compute an initial seed as random as possible. Rely on Address Space
** Layout Randomization (if present) to increase randomness..
*/
#define addbuff(b,p,e) \
  { size_t t = cast(size_t, e); \
    memcpy(b + p, &t, sizeof(t)); p += sizeof(t); }

static unsigned int makeseed (lua_State *L) {
  char buff[4 * sizeof(size_t)];
  unsigned int h = luai_makeseed();
  int p = 0;
  addbuff(buff, p, L);  /* heap variable */
  addbuff(buff, p, &h);  /* local variable */
  addbuff(buff, p, luaO_nilobject);  /* global variable */
  addbuff(buff, p, &lua_newstate);  /* public function */
  lua_assert(p == sizeof(buff));
  return luaS_hash(buff, p, h);
}


/*
** set GCdebt to a new value keeping the value (totalbytes + GCdebt)
** invariant (and avoiding underflows in 'totalbytes')
*/
void luaE_setdebt (global_State *g, l_mem debt) {
  l_mem tb = gettotalbytes(g);
  lua_assert(tb > 0);
  if (debt < tb - MAX_LMEM)
    debt = tb - MAX_LMEM;  /* will make 'totalbytes == MAX_LMEM' */
  g->totalbytes = tb - debt;
  g->GCdebt = debt;
}


CallInfo *luaE_extendCI (lua_State *L) {
  CallInfo *ci = luaM_new(L, CallInfo);
  lua_assert(L->ci->next == NULL);
  // 创建一个新的 ci，并添加进链表
  L->ci->next = ci;
  ci->previous = L->ci;
  ci->next = NULL;
  L->nci++;
  return ci;
}


/*
** free all CallInfo structures not in use by a thread
*/
void luaE_freeCI (lua_State *L) {
  CallInfo *ci = L->ci;
  CallInfo *next = ci->next;
  ci->next = NULL;
  while ((ci = next) != NULL) {
    next = ci->next;
    luaM_free(L, ci);
    L->nci--;
  }
}


/*
** free half of the CallInfo structures not in use by a thread
*/
void luaE_shrinkCI (lua_State *L) {
  CallInfo *ci = L->ci;
  CallInfo *next2;  /* next's next */
  /* while there are two nexts */
  while (ci->next != NULL && (next2 = ci->next->next) != NULL) {
    luaM_free(L, ci->next);  /* free next */
    L->nci--;
    ci->next = next2;  /* remove 'next' from the list */
    next2->previous = ci;
    ci = next2;  /* keep next's next */
  }
}

// 初始化栈
static void stack_init (lua_State *L1, lua_State *L) {
  int i; CallInfo *ci;
  /* initialize stack array */
  // 申请一个大小为 BASIC_STACK_SIZE 的栈，栈单位大小为 sizeof(TValue)
  // BASIC_STACK_SIZE : 2 * 栈的最小值， 2 * 20 = 40
  // 总大小 sizeof(TValue) * 40
  L1->stack = luaM_newvector(L, BASIC_STACK_SIZE, TValue);
  L1->stacksize = BASIC_STACK_SIZE;
  // 将栈元素的元素设为空值
  for (i = 0; i < BASIC_STACK_SIZE; i++)
    setnilvalue(L1->stack + i);  /* erase new stack */
  // 初始化栈顶
  L1->top = L1->stack;
  // 初始化栈的结束位置
  // 栈保留了 EXTRA_STACK 个位置，5个
  L1->stack_last = L1->stack + L1->stacksize - EXTRA_STACK;
  /* initialize first ci */
  // 初始化第一个 call info
  ci = &L1->base_ci;
  ci->next = ci->previous = NULL;
  ci->callstatus = 0;
  // ci func,在栈中的index
  ci->func = L1->top;
  // 将上面指定的entry初始化为nil
  setnilvalue(L1->top++);  /* 'function' entry for this 'ci' */
  // 这个调用 call_info 的栈，从 1 + 20 开始，
  ci->top = L1->top + LUA_MINSTACK;
  L1->ci = ci;
}


static void freestack (lua_State *L) {
  if (L->stack == NULL)
    return;  /* stack not completely built yet */
  L->ci = &L->base_ci;  /* free the entire 'ci' list */
  luaE_freeCI(L);
  lua_assert(L->nci == 0);
  luaM_freearray(L, L->stack, L->stacksize);  /* free stack array */
}


/*
** Create registry table and its predefined values
*/
static void init_registry (lua_State *L, global_State *g) {
  TValue temp;
  /* create registry */
  // 创建一个 Table?
  Table *registry = luaH_new(L);
  
  // luaH_new 返回的是 Table类型，所以要转换为 TValue
  // 然后再存储到 table中
  
  // 绑定到 global state 上
  sethvalue(L, &g->l_registry, registry);

  // 在table中添加两个元素，在 array 中
  // registry[2] == L
  // registry[1] == 另一个新建的table
  luaH_resize(L, registry, LUA_RIDX_LAST, 0);
  /* registry[LUA_RIDX_MAINTHREAD] = L */
  setthvalue(L, &temp, L);  /* temp = L */
  luaH_setint(L, registry, LUA_RIDX_MAINTHREAD, &temp);
  /* registry[LUA_RIDX_GLOBALS] = table of globals */
  sethvalue(L, &temp, luaH_new(L));  /* temp = new table (global table) */
  luaH_setint(L, registry, LUA_RIDX_GLOBALS, &temp);
}


/*
** open parts of the state that may cause memory-allocation errors.
** ('g->version' != NULL flags that the state was completely build)
*/
static void f_luaopen (lua_State *L, void *ud) {
  global_State *g = G(L);
  UNUSED(ud);
  // 初始化 lua_state 的栈
  stack_init(L, L);  /* init stack */
  // 创建 regisry，并初始化2个元素
  init_registry(L, g);
  // 初始化 String Table
  luaS_init(L);
  // 初始化一些 eventname
  luaT_init(L);
  // 初始化 token 字符串
  luaX_init(L);
  // 恢复gc的运行
  g->gcrunning = 1;  /* allow gc */
  // 设置lua的版本
  g->version = lua_version(NULL);
  luai_userstateopen(L);
}


/*
** preinitialize a thread with consistent values without allocating
** any memory (to avoid errors)
*/
static void preinit_thread (lua_State *L, global_State *g) {
  // 让每个 lua_state, 共享同一个 global_state
  G(L) = g;
  // 栈顶
  L->stack = NULL;
  // 调用链， 及调用链元素个数
  L->ci = NULL;
  L->nci = 0;
  // 栈的大小
  L->stacksize = 0;
  // upvalues
  L->twups = L;  /* thread has no upvalues */
  // errors recover point
  L->errorJmp = NULL;
  // calls 数量
  L->nCcalls = 0;
  // 钩子
  // Functions to be called by the debugger in specific events
  L->hook = NULL;
  L->hookmask = 0;
  L->basehookcount = 0;
  L->allowhook = 1;
  // L->hookCount = L->basehookcount
  resethookcount(L);
  // upvalues 的链表
  L->openupval = NULL;
  // 栈中不能 yield 的函数数量
  // number of non-yieldable calls in stack
  L->nny = 1;
  // state 的状态
  L->status = LUA_OK;
  // handle error
  L->errfunc = 0;
}


static void close_state (lua_State *L) {
  global_State *g = G(L);
  luaF_close(L, L->stack);  /* close all upvalues for this thread */
  luaC_freeallobjects(L);  /* collect all objects */
  if (g->version)  /* closing a fully built state? */
    luai_userstateclose(L);
  luaM_freearray(L, G(L)->strt.hash, G(L)->strt.size);
  freestack(L);
  lua_assert(gettotalbytes(g) == sizeof(LG));
  (*g->frealloc)(g->ud, fromstate(L), sizeof(LG), 0);  /* free main block */
}


LUA_API lua_State *lua_newthread (lua_State *L) {
  global_State *g = G(L);
  lua_State *L1;
  lua_lock(L);
  luaC_checkGC(L);
  /* create new thread */
  L1 = &cast(LX *, luaM_newobject(L, LUA_TTHREAD, sizeof(LX)))->l;
  L1->marked = luaC_white(g);
  L1->tt = LUA_TTHREAD;
  /* link it on list 'allgc' */
  L1->next = g->allgc;
  g->allgc = obj2gco(L1);
  /* anchor it on L stack */
  setthvalue(L, L->top, L1);
  api_incr_top(L);
  preinit_thread(L1, g);
  L1->hookmask = L->hookmask;
  L1->basehookcount = L->basehookcount;
  L1->hook = L->hook;
  resethookcount(L1);
  /* initialize L1 extra space */
  memcpy(lua_getextraspace(L1), lua_getextraspace(g->mainthread),
         LUA_EXTRASPACE);
  luai_userstatethread(L, L1);
  stack_init(L1, L);  /* init stack */
  lua_unlock(L);
  return L1;
}


void luaE_freethread (lua_State *L, lua_State *L1) {
  LX *l = fromstate(L1);
  luaF_close(L1, L1->stack);  /* close all upvalues for this thread */
  lua_assert(L1->openupval == NULL);
  luai_userstatefree(L, L1);
  freestack(L1);
  luaM_free(L, l);
}


LUA_API lua_State *lua_newstate (lua_Alloc f, void *ud) {
  int i;
  lua_State *L;
  global_State *g;
  // 使用指定的内存分配器 f 申请内存
  LG *l = cast(LG *, (*f)(ud, NULL, LUA_TTHREAD, sizeof(LG)));

  // 请求内存失败
  if (l == NULL) return NULL;

  // L = 当前线程的 lua_State
  L = &l->l.l;
  // global state 由所有的线程共享，
  // 由初始线程创建
  g = &l->g;
  // 线程的 单链表
  L->next = NULL;
  // 设置自己的类型为线程， 也就是说一个lua_state 可能等价一个线程
  L->tt = LUA_TTHREAD;
  // 用于gc的标志位
  g->currentwhite = bitmask(WHITE0BIT);
  L->marked = luaC_white(g);

  // 初始化线程
  preinit_thread(L, g);
  // 记录分配/释放函数
  g->frealloc = f;
  // auxiliary data to 'frealloc'
  g->ud = ud;
  // 设置主函数
  g->mainthread = L;
  // 设置随机数种子
  g->seed = makeseed(L);
  // 在创建state期间，关闭GC
  g->gcrunning = 0;  /* no GC while building state */
  // 不需要回收的内存
  g->GCestimate = 0;
  // 全局的 hash table
  g->strt.size = g->strt.nuse = 0;
  g->strt.hash = NULL;
  setnilvalue(&g->l_registry);
  // 在不被保护的状态下，发生错误时，调用的函数
  g->panic = NULL;
  // 解释器版本
  g->version = NULL;
  // 设置GC状态， 暂停
  g->gcstate = GCSpause;
  // 设置GC类型
  // 两种，一种是正常gc，另一种只在内存分配失败时gc
  g->gckind = KGC_NORMAL;
  // 各种 gc 元素的链表
  // allgc, 所有 可以 gc的元素链表
  // finobj, 可能：有析构函数的gc元素 链表
  // tobefnz, userdate 链表
  // fixedgc, 不能被gc的元素链表
  g->allgc = g->finobj = g->tobefnz = g->fixedgc = NULL;
  // 当前GC的位置
  g->sweepgc = NULL;
  // 灰色元素 的链表
  g->gray = g->grayagain = NULL;
  // 弱引用
  g->weak = g->ephemeron = g->allweak = NULL;
  g->twups = NULL;
  // 当前LUA 所申请的所有内存，现在只申请了一个 LG 对象的内存
  g->totalbytes = sizeof(LG);

  // 可以被gc，但尚未释放的内存大小
  g->GCdebt = 0;
  // 每个gc 动作中，finalizer 调用的次数
  g->gcfinnum = 0;
  // 两次 gc 动作的间隔
  g->gcpause = LUAI_GCPAUSE;
  // GC的颗粒度
  g->gcstepmul = LUAI_GCMUL;

  // 初始化基础类型的 元数据 metedata
  for (i=0; i < LUA_NUMTAGS; i++) 
	  g->mt[i] = NULL;

  // 在保护模式下运行 f_luaopen 函数，设置一些lua的基本参数
  // 据介绍是，f_luaopen，中要申请内存，可能引发 alloc_error
  if (luaD_rawrunprotected(L, f_luaopen, NULL) != LUA_OK) {
    /* memory allocation error: free partial state */
    close_state(L);
    L = NULL;
  }
  return L;
}


LUA_API void lua_close (lua_State *L) {
  L = G(L)->mainthread;  /* only the main thread can be closed */
  lua_lock(L);
  close_state(L);
}


