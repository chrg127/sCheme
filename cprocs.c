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
    return make_number_exp(sum);
}

Exp scheme_sub(List args)
{
    if (args.size == 0) die("-: arity mismatch\n");
    if (!is_number(args.data[0])) die("-: not a number\n");
    if (args.size == 1) {
        return make_number_exp(-args.data[0].atom.number);
    }
    double sub = args.data[0].atom.number;
    for (size_t i = 1; i < args.size; i++) {
        if (!is_number(args.data[i])) die("-: not a number\n");
        sub -= args.data[i].atom.number;
    }
    return make_number_exp(sub);
}

Exp scheme_mul(List args)
{
    double mul = 1;
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i])) die("*: not a number\n");
        mul *= args.data[i].atom.number;
    }
    return make_number_exp(mul);
}

Exp scheme_gt(List args)
{
    if (args.size == 0) die(">: arity mismatch\n");
    if (args.size == 1) return make_number_exp(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die(">: not a number\n");
    return make_number_exp(args.data[0].atom.number > args.data[1].atom.number);
}

Exp scheme_lt(List args)
{
    if (args.size == 0) die("<: arity mismatch\n");
    if (args.size == 1) return make_number_exp(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("<: not a number\n");
    return make_number_exp(args.data[0].atom.number < args.data[1].atom.number);
}

Exp scheme_ge(List args)
{
    if (args.size == 0) die(">=: arity mismatch\n");
    if (args.size == 1) return make_number_exp(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die(">=: not a number\n");
    return make_number_exp(args.data[0].atom.number >= args.data[1].atom.number);
}

Exp scheme_le(List args)
{
    if (args.size == 0) die("<=: arity mismatch\n");
    if (args.size == 1) return make_number_exp(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("<=: not a number\n");
    return make_number_exp(args.data[0].atom.number <= args.data[1].atom.number);
}

Exp scheme_eq(List args)
{
    if (args.size == 0) die("=: arity mismatch\n");
    if (args.size == 1) return make_number_exp(1);
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("=: not a number\n");
    return make_number_exp(args.data[0].atom.number == args.data[1].atom.number);
}

Exp scheme_abs(List args)
{
    if (args.size != 1) die("=: arity mismatch\n");
    if (!is_number(args.data[0])) die("=: not a number\n");
    return make_number_exp(fabs(args.data[0].atom.number));
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
    return make_number_exp(args.data[0].list.size);
}

Exp scheme_is_null(List args)
{
    if (args.size != 1) die("length: arity mismatch\n");
    return make_number_exp(args.data[0].type != EXP_LIST ? false
                         : args.data[0].list.size == 0);
}

// Exp scheme_is_eq(List args)
// {
//     if (args.size != 2) die("eq?: arity mismatch\n");
//     if (args.data[0].type != args.data[1].type)
//         return false;
//     Exp first = args.data[0], second = args.data[1];
//     switch (first.type) {
//     case EXP_ATOM:
//         if (first.atom.type != second.atom.type)
//             return false;
//         switch (first.atom.type) {
//         case EXP_NUMBER: return first.atom.number == second.atom.number;
//         case EXP_NUMBER: return first.atom.symbol == second.atom.symbol;
//         }
//         break;
//     case EXP_LIST: return first.list.data == second.list.data;
//     case EXP_C_PROC: return first.cproc == second.cproc
//     case EXP_PROC: return first.proc.params.data == second.proc.params.data
//                        && first.proc.body == second.proc.body
//                        && first.proc.env  == second.proc.env;
//     }
// }

// Exp scheme_equal(List args)
// {
//     if (args.size != 2) die("eq?: arity mismatch\n");
//     if (args.data[0].type != args.data[1].type)
//         return false;
//     Exp first = args.data[0], second = args.data[1];
//     switch (first.type) {
//     case EXP_ATOM:
//         if (first.atom.type != second.atom.type)
//             return false;
//         switch (first.atom.type) {
//         case EXP_NUMBER: return first.atom.number == second.atom.number;
//         case EXP_NUMBER: return first.atom.symbol == second.atom.symbol;
//         }
//         break;
//     case EXP_LIST:
//     case EXP_C_PROC: return first.cproc == second.cproc
//     case EXP_PROC: return first.proc.params.data == second.proc.params.data
//                        && first.proc.body == second.proc.body
//                        && first.proc.env  == second.proc.env;
//     }
// }

// still missing: append, apply, list?, max, min, not, number?,
// procedure?, symbol?
