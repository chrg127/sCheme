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

typedef char *Symbol;   // A Scheme Symbol is implemented as a C string
typedef double Number;  // A Scheme number is implemented as a C int

// A Scheme Atom is a Symbol or Number
// ... But we won't create an Atom struct, and will embed Number directly into
// Exp for simplicity.

// Lists, Symbols and user-defined procedure allocate memory.
// Other values do not. Keep the former in a separate object to simplify
// the garbage collector.
typedef struct GCObject GCObject;

typedef struct Exp Exp;

// A Scheme List is implemented as a resizable array of expressions
VECTOR_DECLARE_STRUCT(List, Exp);
VECTOR_DECLARE_INIT(List, Exp, list);
VECTOR_DECLARE_ADD(List, Exp, list);
VECTOR_DECLARE_FREE(List, Exp, list);

// A native C procedure
typedef Exp (*CProc)(List args);

// A Scheme expression is either an Atom, a List, a C Procedure,
// a user-defined Procedure or void
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

// An environment: a hashtable of ("var": exp) pairs, with an outer Env.
typedef struct Env {
    GCObject *obj; // hashtable is contained here
    struct Env *outer;
} Env;

// A user-defined Scheme procedure
typedef struct Procedure {
    Exp params;
    Exp body;
    Env env;
} Procedure;

// Some utilities for working with Exp.
static inline bool is_symbol(Exp exp) { return exp.type == EXP_SYMBOL; }
static inline bool is_number(Exp exp) { return exp.type == EXP_NUMBER; }

static inline Exp mknum(double n)
{
    return (Exp) { .type = EXP_NUMBER, .number = n };
}

static inline Exp mkcproc(CProc cproc)
{
    return (Exp) { .type = EXP_C_PROC, .cproc = cproc };
}

#define SCHEME_TRUE mknum(1)
#define SCHEME_FALSE mknum(0)

Exp eval(Exp x, Env *env);
Exp proc_call(Procedure *proc, List args);
void repl();
void print();
void exec_string(const char *s);


