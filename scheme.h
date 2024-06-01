#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#include "ht.h"
#include "vector.h"

typedef char *Symbol;   // A Scheme Symbol is implemented as a C string
typedef double Number;     // A Scheme number is implemented as a C int

// A Scheme Atom is a Symbol or Number
#define ATOM_SYMBOL 0
#define ATOM_NUMBER 1

typedef struct Atom {
    int type;
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

// An environment: a hashtable of ("var": exp) pairs, with an outer Env.
typedef struct Env {
    HashTable ht;
    struct Env *outer;
} Env;

// A user-defined Scheme procedure
typedef struct Procedure {
    List params;
    Exp *body;
    Env *env;
} Procedure;

// A Scheme expression is either an Atom, a List, a C Procedure,
// a user-defined, Procedure, or void
#define EXP_ATOM 0
#define EXP_LIST 1
#define EXP_C_PROC 2
#define EXP_VOID 3
#define EXP_PROC 4

typedef Exp (*CProc)(List args);

struct Exp {
    int type;
    union {
        Atom atom;
        List list;
        CProc cproc;
        Procedure proc;
    };
};

VECTOR_DECLARE_INIT(List, Exp, list);
VECTOR_DECLARE_ADD(List, Exp, list);
VECTOR_DECLARE_FREE(List, Exp, list);

// Some utilities for working with Exp.
static inline bool is_symbol(Exp exp) { return exp.type == EXP_ATOM && exp.atom.type == ATOM_SYMBOL; }
static inline bool is_number(Exp exp) { return exp.type == EXP_ATOM && exp.atom.type == ATOM_NUMBER; }

static inline Exp make_number_exp(double n)
{
    return (Exp) { .type = EXP_ATOM, .atom = (Atom) { .type = ATOM_NUMBER, .number = n } };
}

static inline Exp make_cproc_exp(CProc cproc)
{
    return (Exp) { .type = EXP_C_PROC, .cproc = cproc };
}

static inline Exp mklist(List l)
{
    return (Exp) { .type = EXP_LIST, .list = l };
}

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

Exp eval(Exp x, Env *env);

