#include "scheme.h"
#include "vector.h"

#include <stdarg.h>

void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

// All scheme procedures that are inside the standard environment.
Exp scheme_sum(List args)
{
    double sum = 0;
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i])) die("+: not a number\n");
        sum += args.data[i].atom.number;
    }
    return mknum(sum);
}

Exp scheme_sub(List args)
{
    if (args.size == 0) die("-: arity mismatch\n");
    if (!is_number(args.data[0])) die("-: not a number\n");
    if (args.size == 1) {
        return mknum(-args.data[0].atom.number);
    }
    double sub = args.data[0].atom.number;
    for (size_t i = 1; i < args.size; i++) {
        if (!is_number(args.data[i])) die("-: not a number\n");
        sub -= args.data[i].atom.number;
    }
    return mknum(sub);
}

Exp scheme_mul(List args)
{
    double mul = 1;
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i])) die("*: not a number\n");
        mul *= args.data[i].atom.number;
    }
    return mknum(mul);
}

Exp scheme_gt(List args)
{
    if (args.size == 0) die(">: arity mismatch\n");
    if (args.size == 1) return mknum(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die(">: not a number\n");
    return mknum(args.data[0].atom.number > args.data[1].atom.number);
}

Exp scheme_lt(List args)
{
    if (args.size == 0) die("<: arity mismatch\n");
    if (args.size == 1) return mknum(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("<: not a number\n");
    return mknum(args.data[0].atom.number < args.data[1].atom.number);
}

Exp scheme_ge(List args)
{
    if (args.size == 0) die(">=: arity mismatch\n");
    if (args.size == 1) return mknum(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die(">=: not a number\n");
    return mknum(args.data[0].atom.number >= args.data[1].atom.number);
}

Exp scheme_le(List args)
{
    if (args.size == 0) die("<=: arity mismatch\n");
    if (args.size == 1) return mknum(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("<=: not a number\n");
    return mknum(args.data[0].atom.number <= args.data[1].atom.number);
}

Exp scheme_eq(List args)
{
    if (args.size == 0) die("=: arity mismatch\n");
    if (args.size == 1) return mknum(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("=: not a number\n");
    return mknum(args.data[0].atom.number == args.data[1].atom.number);
}

Exp scheme_not(List args)
{
    if (args.size != 1) die("not: arity mismatch\n");
    if (!is_number(args.data[0])) {
        return mknum(0);
    }
    return mknum(args.data[0].atom.number == 0 ? 1 : 0);
}

Exp scheme_and(List args)
{
    for (size_t i = 0; i < args.size; i++) {
        if (is_number(args.data[i]) && args.data[i].atom.number == 0) {
            return mknum(0);
        }
    }
    return args.data[args.size-1];
}

Exp scheme_or(List args)
{
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i]) || args.data[i].atom.number != 0) {
            return args.data[i];
        }
    }
    return mknum(0);
}

Exp scheme_abs(List args)
{
    if (args.size != 1) die("=: arity mismatch\n");
    if (!is_number(args.data[0])) die("=: not a number\n");
    return mknum(fabs(args.data[0].atom.number));
}

Exp scheme_begin(List args)
{
    if (args.size == 0) die("begin: arity mismatch\n");
    return args.data[args.size-1];
}

Exp scheme_list(List args)
{
    return (Exp) { .type = EXP_LIST, .list = args };
}

Exp scheme_cons(List args)
{
    if (args.size != 2) die("cons: arity mismatch\n");
    if (args.data[0].type != EXP_LIST) die("cons: second arg must be a list\n");
    List res = VECTOR_INIT();
    list_add(&res, args.data[0]);
    for (size_t i = 0; i < args.data[1].list.size; i++) {
        list_add(&res, args.data[1].list.data[i]);
    }
    return (Exp) { .type = EXP_LIST, .list = res };
}

Exp scheme_car(List args)
{
    if (args.size != 1) die("car: arity mismatch\n");
    if (args.data[0].type != EXP_LIST) die("car: expected list\n");
    return args.data[0].list.data[0];
}

Exp scheme_cdr(List args)
{
    if (args.size != 1) die("cdr: arity mismatch\n");
    if (args.data[0].type != EXP_LIST) die("cdr: expected list\n");
    List res = VECTOR_INIT();
    for (size_t i = 1; i < args.data[0].list.size; i++) {
        list_add(&res, args.data[0].list.data[i]);
    }
    return mklist(res);
}

Exp scheme_length(List args)
{
    if (args.size != 1) die("length: arity mismatch\n");
    if (args.data[0].type != EXP_LIST) die("length: not a list\n");
    return mknum(args.data[0].list.size);
}

Exp scheme_is_null(List args)
{
    if (args.size != 1) die("length: arity mismatch\n");
    return mknum(args.data[0].type != EXP_LIST ? false
                         : args.data[0].list.size == 0);
}

Exp scheme_is_eq(List args)
{
    if (args.size != 2) die("eq?: arity mismatch\n");
    if (args.data[0].type != args.data[1].type) {
        return SCHEME_FALSE;
    }
    Exp first = args.data[0], second = args.data[1];
    switch (first.type) {
    case EXP_ATOM:
        if (first.atom.type != second.atom.type) {
            return SCHEME_FALSE;
        }
        switch (first.atom.type) {
        case ATOM_NUMBER: return mknum(first.atom.number == second.atom.number);
        case ATOM_SYMBOL: return mknum(first.atom.symbol == second.atom.symbol);
        }
        break;
    case EXP_LIST:   return mknum(first.list.data == second.list.data);
    case EXP_C_PROC: return mknum(first.cproc == second.cproc);
    case EXP_PROC:   return mknum(first.proc == second.proc);
    }
    return SCHEME_FALSE;
}

static Exp _equal(Exp first, Exp second)
{
    switch (first.type) {
    case EXP_ATOM:
        if (first.atom.type != second.atom.type)
            return SCHEME_FALSE;
        switch (first.atom.type) {
        case ATOM_NUMBER: return mknum(first.atom.number == second.atom.number);
        case ATOM_SYMBOL: return mknum(first.atom.symbol == second.atom.symbol);
        }
        break;
    case EXP_LIST:
        if (first.list.size != second.list.size) {
            return SCHEME_FALSE;
        }
        for (size_t i = 0; i < first.list.size; i++) {
            Exp res = _equal(first.list.data[i], second.list.data[i]);
            if (res.atom.number == 0) {
                return SCHEME_FALSE;
            }
        }
        return SCHEME_TRUE;
    case EXP_C_PROC: return mknum(first.cproc == second.cproc);
    case EXP_PROC:   return mknum(first.proc == second.proc);
    }
    return SCHEME_FALSE;
}

Exp scheme_equal(List args)
{
    if (args.size != 2) die("eq?: arity mismatch\n");
    if (args.data[0].type != args.data[1].type) {
        return SCHEME_FALSE;
    }
    return _equal(args.data[0], args.data[1]);
}

// (append . lst)
Exp scheme_append(List args)
{
    List res = VECTOR_INIT();
    for (size_t i = 0; i < args.size; i++) {
        if (!is_list(args.data[i])) {
            die("append: argument #%d is not a list\n", i);
        }
        for (size_t j = 0; j < args.data[i].list.size; j++) {
            list_add(&res, args.data[i].list.data[j]);
        }
    }
    return mklist(res);
}

// (apply proc lst)
Exp scheme_apply(List args)
{
    if (args.size != 2) die("apply: arity mismatch\n");
    if (args.data[0].type != EXP_C_PROC && args.data[0].type != EXP_PROC) {
        die("apply: argument #1 must be a procedure\n");
    }
    if (args.data[1].type != EXP_LIST) {
        die("apply: argument #2 must be a list\n");
    }
    Exp proc = args.data[0];
    List proc_args = args.data[1].list;
    return proc.type == EXP_C_PROC ? proc.cproc(proc_args)
                                   : proc_call(proc.proc, proc_args);
}

Exp scheme_is_list(List args)
{
    if (args.size != 1) die("list?: arity mismatch\n");
    return mknum(args.data[0].type == EXP_LIST);
}

Exp scheme_is_number(List args)
{
    if (args.size != 1) die("number?: arity mismatch\n");
    return mknum(args.data[0].type == EXP_ATOM && args.data[0].atom.type == ATOM_NUMBER);
}

Exp scheme_is_proc(List args)
{
    if (args.size != 1) die("procedure?: arity mismatch\n");
    return mknum(args.data[0].type == EXP_PROC || args.data[0].type == EXP_C_PROC);
}

Exp scheme_is_symbol(List args)
{
    if (args.size != 1) die("symbol?: arity mismatch\n");
    return mknum(args.data[0].type == EXP_ATOM && args.data[0].atom.type == ATOM_SYMBOL);
}

