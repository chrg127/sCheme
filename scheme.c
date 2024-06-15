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
        Exp list_exp = mklist((List) VECTOR_INIT());
        List *list = &list_exp.obj->list;
        gc_save(list_exp.obj);
        while (t->cur.s != NULL && t->cur.s[0] != ')') {
            Exp exp = read_from_tokens(t);
            if (is_obj(exp)) {
                gc_save(exp.obj);
            }
            list_add(list, exp);
            if (is_obj(exp)) {
                gc_unsave();
            }
        }
        if (t->cur.s == NULL) {
            die("error: unexpected EOF\n");
        }
        next_token(t); // pop off ')'
        gc_unsave(list_exp.obj);
        return list_exp;
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

static void add_env(Env *env, Exp symbol, Exp exp)
{
    gc_save(symbol.obj);
    if (is_obj(exp)) {
        gc_save(exp.obj);
    }
    ht_install(&env->obj->ht, symbol, exp);
    if (is_obj(exp)) {
        gc_unsave();
    }
    gc_unsave();
}

// An environment with some scheme standard procedures.
static Env standard_env()
{
    Env env = new_env(NULL);
    gc_push_env(&env);
    add_env(&env, mkcsym("+"),          mkcproc(scheme_sum));
    add_env(&env, mkcsym("-"),          mkcproc(scheme_sub));
    add_env(&env, mkcsym("*"),          mkcproc(scheme_mul));
    add_env(&env, mkcsym(">"),          mkcproc(scheme_gt));
    add_env(&env, mkcsym("<"),          mkcproc(scheme_lt));
    add_env(&env, mkcsym(">="),         mkcproc(scheme_ge));
    add_env(&env, mkcsym("<="),         mkcproc(scheme_le));
    add_env(&env, mkcsym("="),          mkcproc(scheme_eq));
    add_env(&env, mkcsym("begin"),      mkcproc(scheme_begin));
    add_env(&env, mkcsym("list"),       mkcproc(scheme_list));
    add_env(&env, mkcsym("pi"),         mknum(3.14159265358979323846));
    add_env(&env, mkcsym("cons"),       mkcproc(scheme_cons));
    add_env(&env, mkcsym("car"),        mkcproc(scheme_car));
    add_env(&env, mkcsym("cdr"),        mkcproc(scheme_cdr));
    add_env(&env, mkcsym("length"),     mkcproc(scheme_length));
    add_env(&env, mkcsym("null?"),      mkcproc(scheme_is_null));
    add_env(&env, mkcsym("eq?"),        mkcproc(scheme_is_eq));
    add_env(&env, mkcsym("equal?"),     mkcproc(scheme_equal));
    add_env(&env, mkcsym("not"),        mkcproc(scheme_not));
    add_env(&env, mkcsym("and"),        mkcproc(scheme_and));
    add_env(&env, mkcsym("or"),         mkcproc(scheme_or));
    add_env(&env, mkcsym("append"),     mkcproc(scheme_append));
    add_env(&env, mkcsym("apply"),      mkcproc(scheme_apply));
    add_env(&env, mkcsym("list?"),      mkcproc(scheme_is_list));
    add_env(&env, mkcsym("number?"),    mkcproc(scheme_is_number));
    add_env(&env, mkcsym("procedure?"), mkcproc(scheme_is_proc));
    add_env(&env, mkcsym("symbol?"),    mkcproc(scheme_is_symbol));
    gc_pop_env();
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
    gc_push_env(&env);
    for (size_t i = 0; i < args.size; i++) {
        add_env(&env, AS_LIST(proc->params).data[i], args.data[i]);
    }
    Exp exp = eval(proc->body, &env);
    gc_pop_env();
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
    if (l.size == 0) {
        die("missing procedure expression\n");
    }
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
        add_env(env, l.data[1], eval(exp, env));
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
        add_env(e, l.data[1], eval(exp, env));
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
    // wrap args list in an object. this is to make sure it will be found
    // by the garbage collector.
    // procedure calls may not use the underlying list to create new objects.
    Exp args = mklist((List) VECTOR_INIT());
    gc_save(args.obj);
    for (size_t i = 1; i < l.size; i++) {
        Exp new_elem = eval(l.data[i], env);
        if (is_obj(new_elem)) {
            gc_save(new_elem.obj);
        }
        list_add(&AS_LIST(args), new_elem);
        if (is_obj(new_elem)) {
            gc_unsave(new_elem.obj);
        }
    }
    Exp res = proc.type == EXP_C_PROC
        ? proc.cproc(AS_LIST(args))
        : proc_call(&AS_PROC(proc), AS_LIST(args));
    gc_unsave();
    return res;
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
    gc_push_env(&env);
    while (true) {
        printf("sCheme> ");
        char input[BUFSIZ] = {0};
        fgets(input, sizeof(input), stdin);
        Exp parsed = parse(input);
        printf("parsed = ");
        print(parsed);
        printf("\n");
        if (is_obj(parsed)) {
            gc_save(parsed.obj);
        }
        Exp val = eval(parsed, &env);
        if (val.type == EXP_EOF) {
            printf("\n");
            return;
        }
        print(val);
        if (is_obj(parsed)) {
            gc_unsave();
        }
        printf("\n");
        gc_collect();
    }
}

int main()
{
    repl();
    return 0;
}

