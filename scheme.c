#include <stdarg.h>
#include "memory.h"
#include "scheme.h"
#include "ht.h"
#include "gcobject.h"

VECTOR_DEFINE_INIT(List, Exp, list)
VECTOR_DEFINE_ADD(List, Exp, list)
VECTOR_DEFINE_FREE(List, Exp, list)

noreturn void die(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    exit(1);
}

typedef struct Token {
    const char *s;
    size_t start, end;
} Token;

typedef struct Tokenizer {
    const char *s;
    size_t len;
    size_t i;
    Token prev;
    Token cur;
} Tokenizer;

// Parse next token from string inside tokenizer.
static Token next_token(Tokenizer *t)
{
    t->prev = t->cur;
    while (t->s[t->i] == ' ' || t->s[t->i] == '\n')
        t->i++;
    if (t->i == t->len) {
        t->cur = (Token) { .s = NULL, .start = 0, .end = 0 };
        return t->prev;
    } else if (t->s[t->i] == '(' || t->s[t->i] == ')') {
        t->i++;
        t->cur = (Token) { .s = t->s[t->i-1] == '(' ? "(" : ")", .start = 0, .end = 0 };
        return t->prev;
    }
    size_t start = t->i;
    while (t->s[t->i] != ' ' && t->s[t->i] != '\n'
        && t->s[t->i] != '(' && t->s[t->i] != ')' && t->i < t->len)
        t->i++;
    t->cur = (Token) { .s = t->s, .start = start, .end = t->i };
    return t->prev;
}

// Numbers become numbers; every other token is a symbol.
static Exp atom(Token token)
{
    char *endptr;
    long num = strtol(token.s + token.start, &endptr, 0);
    return endptr == token.s + token.start
        ? mksym(mem_strdup(token.s + token.start, token.end - token.start))
        : mknum(num);
}

// Read an expression from a sequence of tokens.
static Exp read_from_tokens(Tokenizer *t)
{
    Token token = next_token(t);
    if (token.s == NULL) {
        return (Exp) { .type = EXP_EOF };
    } else if (token.s[token.start] == '(') {
        List list = VECTOR_INIT();
        while (t->cur.s != NULL && t->cur.s[0] != ')') {
            Exp exp = read_from_tokens(t);
            list_add(&list, exp);
        }
        if (t->cur.s == NULL) {
            die("error: unexpected EOF\n");
        }
        next_token(t); // pop off ')'
        return mklist(list);
    } else if (token.s[token.start] == ')') {
        die("unexpected ')'\n");
    } else {
        return atom(token);
    }
}

// Read a scheme expression from a string.
static Exp parse(const char *s)
{
    Tokenizer t = { .i = 0, .s = s, .len = strlen(s) };
    next_token(&t);
    return read_from_tokens(&t);
}

// An environment with some scheme standard procedures.
static Env standard_env()
{
    Env env = new_env(NULL);
    ht_install(&env.obj->ht, mksym("+"),          mkcproc(scheme_sum));
    ht_install(&env.obj->ht, mksym("-"),          mkcproc(scheme_sub));
    ht_install(&env.obj->ht, mksym("*"),          mkcproc(scheme_mul));
    ht_install(&env.obj->ht, mksym(">"),          mkcproc(scheme_gt));
    ht_install(&env.obj->ht, mksym("<"),          mkcproc(scheme_lt));
    ht_install(&env.obj->ht, mksym(">="),         mkcproc(scheme_ge));
    ht_install(&env.obj->ht, mksym("<="),         mkcproc(scheme_le));
    ht_install(&env.obj->ht, mksym("="),          mkcproc(scheme_eq));
    ht_install(&env.obj->ht, mksym("begin"),      mkcproc(scheme_begin));
    ht_install(&env.obj->ht, mksym("list"),       mkcproc(scheme_list));
    ht_install(&env.obj->ht, mksym("pi"),         mknum(3.14159265358979323846));
    ht_install(&env.obj->ht, mksym("cons"),       mkcproc(scheme_cons));
    ht_install(&env.obj->ht, mksym("car"),        mkcproc(scheme_car));
    ht_install(&env.obj->ht, mksym("cdr"),        mkcproc(scheme_cdr));
    ht_install(&env.obj->ht, mksym("length"),     mkcproc(scheme_length));
    ht_install(&env.obj->ht, mksym("null?"),      mkcproc(scheme_is_null));
    ht_install(&env.obj->ht, mksym("eq?"),        mkcproc(scheme_is_eq));
    ht_install(&env.obj->ht, mksym("equal?"),     mkcproc(scheme_equal));
    ht_install(&env.obj->ht, mksym("not"),        mkcproc(scheme_not));
    ht_install(&env.obj->ht, mksym("and"),        mkcproc(scheme_and));
    ht_install(&env.obj->ht, mksym("or"),         mkcproc(scheme_or));
    ht_install(&env.obj->ht, mksym("append"),     mkcproc(scheme_append));
    ht_install(&env.obj->ht, mksym("apply"),      mkcproc(scheme_apply));
    ht_install(&env.obj->ht, mksym("list?"),      mkcproc(scheme_is_list));
    ht_install(&env.obj->ht, mksym("number?"),    mkcproc(scheme_is_number));
    ht_install(&env.obj->ht, mksym("procedure?"), mkcproc(scheme_is_proc));
    ht_install(&env.obj->ht, mksym("symbol?"),    mkcproc(scheme_is_symbol));
    return env;
}

// Find the innermost Env where var appears.
static inline Env *env_find(Env *env, Exp var)
{
    if (!env)
        return NULL;
    if (ht_lookup(&env->obj->ht, var, NULL))
        return env;
    return env_find(env->outer, var);
}

Exp proc_call(Procedure *proc, List args)
{
    Env env = new_env(&proc->env);
    gc_set_current_env(&env);
    for (size_t i = 0; i < args.size; i++) {
        ht_install(&env.obj->ht, AS_LIST(proc->params).data[i], args.data[i]);
    }
    Exp exp = eval(proc->body, &env);
    gc_set_current_env(env.outer);
    return exp;
}

// Evaluate an expression in an environment.
Exp eval(Exp x, Env *env)
{
    if (x.type == EXP_EOF) {
        return x;
    } else if (is_symbol(x)) {
        Symbol s = AS_SYM(x);
        // variable reference
        Env *e = env_find(env, x);
        if (!e) {
            die("undefined symbol: %s\n", s);
        }
        Exp value;
        bool found = ht_lookup(&e->obj->ht, x, &value);
        if (!found) {
            die("error: couldn't find %s in env\n", s);
        }
        return value;
    } else if (is_number(x)) {
        // constant number
        return x;
    }
    List l = AS_LIST(x);
    Exp op = l.data[0];
    if (is_symbol(op) && strcmp(AS_SYM(op), "quote") == 0) {
        return l.data[1];
    } else if (is_symbol(op) && strcmp(AS_SYM(op), "if") == 0) {
        // conditional
        Exp test        = l.data[1];
        Exp conseq      = l.data[2];
        Exp alt         = l.data[3];
        Exp test_result = eval(test, env);
        Exp exp = !is_number(test_result)
               || (is_number(test_result) && test_result.number != 0)
               ? conseq : alt;
        return eval(exp, env);
    } else if (is_symbol(op) && strcmp(AS_SYM(op), "define") == 0) {
        // definition
        if (!is_symbol(l.data[1])) {
            die("define: bad syntax\n");
        }
        Exp exp = l.data[2];
        ht_install(&env->obj->ht, l.data[1], eval(exp, env));
        return (Exp) { .type = EXP_VOID };
    } else if (is_symbol(op) && strcmp(AS_SYM(op), "set!") == 0) {
        // assignment
        if (!is_symbol(l.data[1])) {
            die("set!: bad syntax\n");
        }
        Exp exp = l.data[2];
        Env *e = env_find(env, l.data[1]);
        if (!e) {
            die("undefined symbol: %s\n", AS_SYM(l.data[1]));
        }
        ht_install(&e->obj->ht, l.data[1], eval(exp, env));
        return (Exp) { .type = EXP_VOID };
    } else if (is_symbol(op) && strcmp(AS_SYM(op), "lambda") == 0) {
        // procedure
        Exp params = l.data[1];
        Exp body   = l.data[2];
        return mkproc(params, body, *env);
    }
    // procedure call
    Exp proc = eval(op, env);
    if (proc.type != EXP_C_PROC && proc.type != EXP_PROC) {
        die("error: not a procedure\n");
    }
    // wrap new list in an object. this is to make sure it will be found
    // by the garbage collector.
    Exp args = mklist((List) VECTOR_INIT());
    for (size_t i = 1; i < l.size; i++) {
        list_add(&AS_LIST(args), eval(l.data[i], env));
    }
    return proc.type == EXP_C_PROC
        ? proc.cproc(AS_LIST(args))
        : proc_call(&AS_PROC(proc), AS_LIST(args));
}

static void print(Exp exp)
{
    switch (exp.type) {
    case EXP_EMPTY: break;
    case EXP_SYMBOL: printf("%s", AS_SYM(exp)); break;
    case EXP_NUMBER: printf("%g", exp.number); break;
    case EXP_LIST:
        printf("(");
        for (size_t i = 0; i < AS_LIST(exp).size; i++) {
            print(AS_LIST(exp).data[i]);
            if (i != AS_LIST(exp).size-1) {
                printf(" ");
            }
        }
        printf(")");
        break;
    case EXP_C_PROC:
        printf("<#c-procedure>");
        break;
    case EXP_PROC:
        printf("<#procedure>");
        break;
    case EXP_VOID:
        printf("<#void>");
        break;
    case EXP_EOF:
        break;
    }
}

// A prompt-read-eval-print loop.
static void repl()
{
    Env env = standard_env();
    gc_set_current_env(&env);
    while (true) {
        printf("sCheme> ");
        char input[BUFSIZ] = {0};
        fgets(input, sizeof(input), stdin);
        Exp parsed = parse(input);
        printf("parsed = ");
        print(parsed);
        printf("\n");
        Exp val = eval(parsed, &env);
        if (val.type == EXP_EOF) {
            printf("\n");
            return;
        }
        print(val);
        printf("\n");
        gc_collect();
    }
}

int main()
{
    repl();
    return 0;
}

