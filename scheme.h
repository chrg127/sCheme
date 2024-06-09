#pragma once

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
typedef enum AtomType {
    ATOM_SYMBOL,
    ATOM_NUMBER
} AtomType;

typedef struct Atom {
    AtomType type;
    union {
        Symbol symbol;
        Number number;
    };
} Atom;

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
    EXP_VOID   = 0,
    EXP_ATOM   = 1,
    EXP_LIST   = 2,
    EXP_C_PROC = 3,
    EXP_PROC   = 4,
    EXP_EOF    = 5,
} ExpType;

struct Exp {
    ExpType type;
    union {
        Atom atom;
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

// Lists and user-defined procedure allocate memory, other values do not.
// Keep these in a separate object to simplify the garbage collector.
typedef struct GCObject {
    bool marked;
    struct GCObject *next;
    union {
        List list;
        Procedure proc;
    };
} GCObject;

// Some utilities for working with Exp.
static inline bool is_symbol(Exp exp)
{
    return exp.type == EXP_ATOM && exp.atom.type == ATOM_SYMBOL;
}

static inline bool is_number(Exp exp)
{
    return exp.type == EXP_ATOM && exp.atom.type == ATOM_NUMBER;
}

static inline bool is_list(Exp exp)
{
    return exp.type == EXP_LIST;
}

static inline Exp mknum(double n)
{
    return (Exp) { .type = EXP_ATOM, .atom = (Atom) { .type = ATOM_NUMBER, .number = n } };
}

static inline Exp mklist(List l)
{
    GCObject *obj = mkobj();
    obj->list = l;
    return (Exp) { .type = EXP_LIST, .obj = obj };
}

static inline Exp mkcproc(CProc cproc)
{
    return (Exp) { .type = EXP_C_PROC, .cproc = cproc };
}

static inline Exp mkproc(List params, Exp body, Env *env)
{
    GCObject *obj = mkobj();
    obj->proc = (Procedure) { .params = params, .body = body, .env = env, };
    return (Exp) { .type = EXP_PROC, .obj = obj };
}

#define SCHEME_TRUE mknum(1)
#define SCHEME_FALSE mknum(0)
#define AS_LIST(e) (e).obj->list
#define AS_PROC(e) (e).obj->proc

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

