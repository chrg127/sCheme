#pragma once

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdnoreturn.h>
#include <math.h>
#include <stdbool.h>
#include "vector.h"
#include "memory.h"

#undef VECTOR_ARRAY_ALLOC
#define VECTOR_ARRAY_ALLOC GROW_ARRAY

#undef VECTOR_ARRAY_FREE
#define VECTOR_ARRAY_FREE FREE_ARRAY

noreturn void die(const char *fmt, ...);

typedef char *Symbol;   // A Scheme Symbol is implemented as a C string
typedef double Number;  // A Scheme number is implemented as a C int

// A Scheme Atom is a Symbol or Number
// ... But we won't create an Atom struct, and will embed both Symbol and
// Number directly into Exp, for reasons explained below.

typedef struct Exp Exp;

// A Scheme List is implemented as a resizable array of expressions
typedef struct List {
    Exp *data;
    size_t size;
    size_t cap;
} List;

VECTOR_DECLARE_INIT(List, Exp, list);
VECTOR_DECLARE_ADD(List, Exp, list);
VECTOR_DECLARE_FREE(List, Exp, list);

// A native C procedure
typedef Exp (*CProc)(List args);

// A Scheme expression is either an Atom, a List, a C Procedure,
// a user-defined Procedure or void
typedef struct GCObject GCObject;

typedef enum ExpType {
    EXP_EMPTY = 0,
    EXP_VOID,
    EXP_NUMBER,
    EXP_SYMBOL,
    EXP_LIST,
    EXP_C_PROC,
    EXP_PROC,
    EXP_EOF,
} ExpType;

struct Exp {
    ExpType type;
    union {
        Number number;
        CProc cproc;
        GCObject *obj;
    };
};

typedef struct Env Env;

// A user-defined Scheme procedure
typedef struct Procedure {
    List params;
    Exp body;
    Env *env;
} Procedure;

// Lists, Symbols and user-defined procedure allocate memory.
// Other values do not. Keep the former in a separate object to simplify
// the garbage collector.
typedef struct GCObject {
    ExpType type;
    union {
        Symbol symbol;
        List list;
        Procedure proc;
    };
    bool marked;
    struct GCObject *next;
} GCObject;

// Some utilities for working with Exp.
static inline bool is_symbol(Exp exp) { return exp.type == EXP_SYMBOL; }
static inline bool is_number(Exp exp) { return exp.type == EXP_NUMBER; }
static inline bool is_list(Exp exp)   { return exp.type == EXP_LIST; }

static inline Exp mkobj(GCObject from)
{
    return (Exp) { .type = from.type, .obj = alloc_obj(from) };
}

static inline Exp mksym(Symbol s)
{
    return mkobj((GCObject) { .type = EXP_SYMBOL, .symbol = s });
}

static inline Exp mknum(double n)
{
    return (Exp) { .type = EXP_NUMBER, .number = n };
}

static inline Exp mklist(List l)
{
    return mkobj((GCObject) { .type = EXP_LIST, .list = l });
}

static inline Exp mkcproc(CProc cproc)
{
    return (Exp) { .type = EXP_C_PROC, .cproc = cproc };
}

static inline Exp mkproc(List params, Exp body, Env *env)
{
    return mkobj((GCObject) {
        .type = EXP_PROC,
        .proc = (Procedure) { .params = params, .body = body, .env = env, }
    });
}

#define SCHEME_TRUE mknum(1)
#define SCHEME_FALSE mknum(0)
#define AS_LIST(e) (e).obj->list
#define AS_PROC(e) (e).obj->proc
#define AS_SYM(e) (e).obj->symbol

Exp scheme_sum(List args);
Exp scheme_sub(List args);
Exp scheme_mul(List args);
Exp scheme_gt(List args);
Exp scheme_lt(List args);
Exp scheme_ge(List args);
Exp scheme_le(List args);
Exp scheme_eq(List args);
Exp scheme_abs(List args);
Exp scheme_begin(List args);
Exp scheme_list(List args);
Exp scheme_cons(List args);
Exp scheme_car(List args);
Exp scheme_cdr(List args);
Exp scheme_length(List args);
Exp scheme_is_null(List args);
Exp scheme_is_eq(List args);
Exp scheme_equal(List args);
Exp scheme_not(List args);
Exp scheme_and(List args);
Exp scheme_or(List args);
Exp scheme_append(List args);
Exp scheme_apply(List args);
Exp scheme_is_list(List args);
Exp scheme_is_number(List args);
Exp scheme_is_proc(List args);
Exp scheme_is_symbol(List args);

Exp eval(Exp x, Env *env);
Exp proc_call(Procedure *proc, List args);

