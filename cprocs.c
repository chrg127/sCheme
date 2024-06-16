// All scheme procedures that are inside the standard environment.

Exp scheme_sum(List args)
{
    double sum = 0;
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i])) die("+: not a number\n");
        sum += args.data[i].number;
    }
    return mknum(sum);
}

Exp scheme_sub(List args)
{
    if (args.size == 0) die("-: arity mismatch\n");
    if (!is_number(args.data[0])) die("-: not a number\n");
    if (args.size == 1) {
        return mknum(-args.data[0].number);
    }
    double sub = args.data[0].number;
    for (size_t i = 1; i < args.size; i++) {
        if (!is_number(args.data[i])) die("-: not a number\n");
        sub -= args.data[i].number;
    }
    return mknum(sub);
}

Exp scheme_mul(List args)
{
    double mul = 1;
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i])) die("*: not a number\n");
        mul *= args.data[i].number;
    }
    return mknum(mul);
}

Exp scheme_abs(List args)
{
    if (args.size != 1) die("=: arity mismatch\n");
    if (!is_number(args.data[0])) die("=: not a number\n");
    return mknum(fabs(args.data[0].number));
}

Exp scheme_gt(List args)
{
    if (args.size == 0) die(">: arity mismatch\n");
    if (args.size == 1) return SCHEME_TRUE;
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die(">: not a number\n");
    return mknum(args.data[0].number > args.data[1].number);
}

Exp scheme_lt(List args)
{
    if (args.size == 0) die("<: arity mismatch\n");
    if (args.size == 1) return SCHEME_TRUE;
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("<: not a number\n");
    return mknum(args.data[0].number < args.data[1].number);
}

Exp scheme_ge(List args)
{
    if (args.size == 0) die(">=: arity mismatch\n");
    if (args.size == 1) return SCHEME_TRUE;
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die(">=: not a number\n");
    return mknum(args.data[0].number >= args.data[1].number);
}

Exp scheme_le(List args)
{
    if (args.size == 0) die("<=: arity mismatch\n");
    if (args.size == 1) return SCHEME_TRUE;
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("<=: not a number\n");
    return mknum(args.data[0].number <= args.data[1].number);
}

Exp scheme_eq(List args)
{
    if (args.size == 0) die("=: arity mismatch\n");
    if (args.size == 1) return SCHEME_TRUE;
    if (!is_number(args.data[0]) || !is_number(args.data[1]))
        die("=: not a number\n");
    return mknum(args.data[0].number == args.data[1].number);
}

Exp scheme_not(List args)
{
    if (args.size != 1) die("not: arity mismatch\n");
    if (!is_number(args.data[0])) {
        return SCHEME_FALSE;
    }
    return mknum(args.data[0].number == 0 ? 1 : 0);
}

Exp scheme_and(List args)
{
    for (size_t i = 0; i < args.size; i++) {
        if (is_number(args.data[i]) && args.data[i].number == 0) {
            return SCHEME_FALSE;
        }
    }
    return args.data[args.size-1];
}

Exp scheme_or(List args)
{
    for (size_t i = 0; i < args.size; i++) {
        if (!is_number(args.data[i]) || args.data[i].number != 0) {
            return args.data[i];
        }
    }
    return SCHEME_FALSE;
}

Exp scheme_begin(List args)
{
    if (args.size == 0) die("begin: arity mismatch\n");
    return args.data[args.size-1];
}

Exp scheme_list(List args)
{
    List new_list = VECTOR_INIT();
    for (size_t i = 0; i < args.size; i++) {
        list_add(&new_list, args.data[i]);
    }
    return mklist(new_list);
}

Exp scheme_cons(List args)
{
    if (args.size != 2) die("cons: arity mismatch\n");
    if (args.data[1].type != EXP_LIST) die("cons: second arg must be a list\n");
    List res = VECTOR_INIT();
    list_add(&res, args.data[0]);
    for (size_t i = 0; i < AS_LIST(args.data[1]).size; i++) {
        list_add(&res, AS_LIST(args.data[1]).data[i]);
    }
    return mklist(res);
}

Exp scheme_car(List args)
{
    if (args.size != 1) die("car: arity mismatch\n");
    if (args.data[0].type != EXP_LIST) die("car: expected list\n");
    return AS_LIST(args.data[0]).data[0];
}

Exp scheme_cdr(List args)
{
    if (args.size != 1) die("cdr: arity mismatch\n");
    if (args.data[0].type != EXP_LIST) die("cdr: expected list\n");
    List res = VECTOR_INIT();
    for (size_t i = 1; i < AS_LIST(args.data[0]).size; i++) {
        list_add(&res, AS_LIST(args.data[0]).data[i]);
    }
    return mklist(res);
}

Exp scheme_length(List args)
{
    if (args.size != 1) die("length: arity mismatch\n");
    if (args.data[0].type != EXP_LIST) die("length: not a list\n");
    return mknum(AS_LIST(args.data[0]).size);
}

Exp scheme_is_null(List args)
{
    if (args.size != 1) die("length: arity mismatch\n");
    return args.data[0].type != EXP_LIST
        ? SCHEME_FALSE
        : mknum(AS_LIST(args.data[0]).size == 0);
}

Exp scheme_is_eq(List args)
{
    if (args.size != 2) die("eq?: arity mismatch\n");
    if (args.data[0].type != args.data[1].type) {
        return SCHEME_FALSE;
    }
    Exp first = args.data[0], second = args.data[1];
    switch (first.type) {
    case EXP_EMPTY:  return SCHEME_TRUE;
    case EXP_NUMBER: return mknum(first.number == second.number);
    case EXP_SYMBOL:
    case EXP_LIST:
    case EXP_PROC:   return mknum(first.obj == second.obj);
    case EXP_C_PROC: return mknum(first.cproc == second.cproc);
    case EXP_VOID:   return SCHEME_TRUE;
    case EXP_EOF:    return SCHEME_TRUE;
    }
    return SCHEME_FALSE;
}

static Exp list_equal(Exp first, Exp second)
{
    if (AS_LIST(first).size != AS_LIST(second).size) {
        return SCHEME_FALSE;
    }
    for (size_t i = 0; i < AS_LIST(first).size; i++) {
        Exp res = list_equal(AS_LIST(first).data[i], AS_LIST(second).data[i]);
        if (res.number == 0) {
            return SCHEME_FALSE;
        }
    }
    return SCHEME_TRUE;
}

Exp scheme_equal(List args)
{
    if (args.size != 2) die("equal?: arity mismatch\n");
    if (args.data[0].type != args.data[1].type) {
        return SCHEME_FALSE;
    }
    switch (args.data[0].type) {
    case EXP_EMPTY:
    case EXP_NUMBER:
    case EXP_C_PROC:
    case EXP_PROC:
    case EXP_VOID:
    case EXP_EOF:
        return scheme_is_eq(args);
    case EXP_SYMBOL:
        return mknum(strcmp(AS_SYM(args.data[0]), AS_SYM(args.data[1])) == 0);
    case EXP_LIST:
        return list_equal(args.data[0], args.data[1]);
    }
    return SCHEME_FALSE;
}

// (append . lst)
Exp scheme_append(List args)
{
    List res = VECTOR_INIT();
    for (size_t i = 0; i < args.size; i++) {
        if (args.data[i].type != EXP_LIST) {
            die("append: argument #%d is not a list\n", i);
        }
        for (size_t j = 0; j < AS_LIST(args.data[i]).size; j++) {
            list_add(&res, AS_LIST(args.data[i]).data[j]);
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
    List proc_args = AS_LIST(args.data[1]);
    return proc.type == EXP_C_PROC ? proc.cproc(proc_args)
                                   : proc_call(&AS_PROC(proc), proc_args);
}

Exp scheme_is_list(List args)
{
    if (args.size != 1) die("list?: arity mismatch\n");
    return mknum(args.data[0].type == EXP_LIST);
}

Exp scheme_is_number(List args)
{
    if (args.size != 1) die("number?: arity mismatch\n");
    return mknum(is_number(args.data[0]));
}

Exp scheme_is_proc(List args)
{
    if (args.size != 1) die("procedure?: arity mismatch\n");
    return mknum(args.data[0].type == EXP_PROC || args.data[0].type == EXP_C_PROC);
}

Exp scheme_is_symbol(List args)
{
    if (args.size != 1) die("symbol?: arity mismatch\n");
    return mknum(is_symbol(args.data[0]));
}

Exp scheme_display(List args)
{
  if (args.size != 1) die("display: arity mismatch\n");
  print(args.data[0]);
  return (Exp) { .type = EXP_VOID };
}

Exp scheme_newline(List args)
{
  if (args.size != 0) die("newline: arity mismatch\n");
  printf("\n");
  return (Exp) { .type = EXP_VOID };
}

