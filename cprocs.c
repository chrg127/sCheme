#include "scheme.h"

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
    int sum = 0;
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i])) die("+: not a number\n");
        sum += args.data[i].atom.number;
    }
    return make_number_exp(sum);
}

Exp scheme_sub(List args)
{
    int sub = 0;
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i])) die("-: not a number\n");
        sub -= args.data[i].atom.number;
    }
    return make_number_exp(sub);
}

Exp scheme_mul(List args)
{
    int mul = 1;
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
    return make_number_exp(abs(args.data[0].atom.number));
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

