#include "scheme.h"

#include <stdarg.h>

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
static Atom atom(Token token)
{
    char *endptr;
    long num = strtol(token.s + token.start, &endptr, 0);
    return endptr == token.s + token.start
        ? (Atom) { .type = ATOM_SYMBOL, .symbol = mem_strdup(token.s + token.start, token.end - token.start) }
        : (Atom) { .type = ATOM_NUMBER, .number = num };
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
        return (Exp) { .type = EXP_ATOM, .atom = atom(token) };
    }
}

// Read a scheme expression from a string.
static Exp parse(const char *s)
{
    Tokenizer t = { .i = 0, .s = s, .len = strlen(s) };
    next_token(&t);
    return read_from_tokens(&t);
}

static Exp *expdup(Exp exp)
{
    Exp *mem = ALLOCATE(Exp, 1);
    memcpy(mem, &exp, sizeof(exp));
    mem->marked = false;
    return mem;
}

// An environment with some scheme standard procedures.
static Env standard_env()
{
    Env env = { .ht = HT_INIT_WITH_ALLOCATOR(reallocate), .outer = NULL };
    ht_install(&env.ht, "+",          expdup(mkcproc(scheme_sum)));
    ht_install(&env.ht, "-",          expdup(mkcproc(scheme_sub)));
    ht_install(&env.ht, "*",          expdup(mkcproc(scheme_mul)));
    ht_install(&env.ht, ">",          expdup(mkcproc(scheme_gt)));
    ht_install(&env.ht, "<",          expdup(mkcproc(scheme_lt)));
    ht_install(&env.ht, ">=",         expdup(mkcproc(scheme_ge)));
    ht_install(&env.ht, "<=",         expdup(mkcproc(scheme_le)));
    ht_install(&env.ht, "=",          expdup(mkcproc(scheme_eq)));
    ht_install(&env.ht, "begin",      expdup(mkcproc(scheme_begin)));
    ht_install(&env.ht, "list",       expdup(mkcproc(scheme_list)));
    ht_install(&env.ht, "pi",         expdup(mknum(3.14159265358979323846)));
    ht_install(&env.ht, "cons",       expdup(mkcproc(scheme_cons)));
    ht_install(&env.ht, "car",        expdup(mkcproc(scheme_car)));
    ht_install(&env.ht, "cdr",        expdup(mkcproc(scheme_cdr)));
    ht_install(&env.ht, "length",     expdup(mkcproc(scheme_length)));
    ht_install(&env.ht, "null?",      expdup(mkcproc(scheme_is_null)));
    ht_install(&env.ht, "eq?",        expdup(mkcproc(scheme_is_eq)));
    ht_install(&env.ht, "equal?",     expdup(mkcproc(scheme_equal)));
    ht_install(&env.ht, "not",        expdup(mkcproc(scheme_not)));
    ht_install(&env.ht, "and",        expdup(mkcproc(scheme_and)));
    ht_install(&env.ht, "or",         expdup(mkcproc(scheme_or)));
    ht_install(&env.ht, "append",     expdup(mkcproc(scheme_append)));
    ht_install(&env.ht, "apply",      expdup(mkcproc(scheme_apply)));
    ht_install(&env.ht, "list?",      expdup(mkcproc(scheme_is_list)));
    ht_install(&env.ht, "number?",    expdup(mkcproc(scheme_is_number)));
    ht_install(&env.ht, "procedure?", expdup(mkcproc(scheme_is_proc)));
    ht_install(&env.ht, "symbol?",    expdup(mkcproc(scheme_is_symbol)));
    return env;
}

// Find the innermost Env where var appears.
static inline Env *env_find(Env *env, char *var)
{
    if (!env)
        return NULL;
    if (ht_lookup(&env->ht, var, NULL))
        return env;
    return env_find(env->outer, var);
}

Exp proc_call(Procedure *proc, List args)
{
    Env env = { .ht = HT_INIT_WITH_ALLOCATOR(reallocate), .outer = proc->env };
    gc_set_current_env(&env);
    for (size_t i = 0; i < args.size; i++) {
        ht_install(&env.ht, proc->params.data[i].atom.symbol, expdup(args.data[i]));
    }
    Exp exp = eval(*proc->body, &env);
    gc_set_current_env(env.outer);
    return exp;
}

// Evaluate an expression in an environment.
Exp eval(Exp x, Env *env)
{
    if (x.type == EXP_EOF) {
        return x;
    } else if (is_symbol(x)) {
        // variable reference
        Env *e = env_find(env, x.atom.symbol);
        if (!e) {
            die("undefined symbol: %s\n", x.atom.symbol);
        }
        void *value;
        bool found = ht_lookup(&e->ht, x.atom.symbol, &value);
        if (!found) {
            die("error: couldn't find %s in env\n", x.atom.symbol);
        }
        return *((Exp *) value);
    } else if (is_number(x)) {
        // constant number
        return x;
    }
    Exp op = x.list.data[0];
    if (is_symbol(op) && strcmp(op.atom.symbol, "quote") == 0) {
        return x.list.data[1];
    } else if (is_symbol(op) && strcmp(op.atom.symbol, "if") == 0) {
        // conditional
        Exp test        = x.list.data[1];
        Exp conseq      = x.list.data[2];
        Exp alt         = x.list.data[3];
        Exp test_result = eval(test, env);
        Exp exp = !is_number(test_result)
               || (is_number(test_result) && test_result.atom.number != 0)
               ? conseq : alt;
        return eval(exp, env);
    } else if (is_symbol(op) && strcmp(op.atom.symbol, "define") == 0) {
        // definition
        Exp symbol = x.list.data[1];
        Exp exp    = x.list.data[2];
        if (!is_symbol(symbol)) {
            die("define: bad syntax\n");
        }
        ht_install(&env->ht, symbol.atom.symbol, expdup(eval(exp, env)));
        return (Exp) { .type = EXP_VOID };
    } else if (is_symbol(op) && strcmp(op.atom.symbol, "set!") == 0) {
        // assignment
        Exp symbol = x.list.data[1];
        Exp exp    = x.list.data[2];
        if (!is_symbol(symbol)) {
            die("set!: bad syntax\n");
        }
        Env *e = env_find(env, symbol.atom.symbol);
        if (!e) {
            die("undefined symbol: %s\n", symbol.atom.symbol);
        }
        ht_install(&e->ht, symbol.atom.symbol, expdup(eval(exp, env)));
        return (Exp) { .type = EXP_VOID };
    } else if (is_symbol(op) && strcmp(op.atom.symbol, "lambda") == 0) {
        // procedure
        Exp params = x.list.data[1];
        Exp body   = x.list.data[2];
        return mkproc(params.list, expdup(body), env);
    }
    // procedure call
    Exp proc = eval(op, env);
    if (proc.type != EXP_C_PROC && proc.type != EXP_PROC) {
        die("error: not a procedure\n");
    }
    List args = VECTOR_INIT();
    for (size_t i = 1; i < x.list.size; i++) {
        list_add(&args, eval(x.list.data[i], env));
    }
    return proc.type == EXP_C_PROC
        ? proc.cproc(args)
        : proc_call(&proc.proc, args);
}

static void print(Exp exp)
{
    switch (exp.type) {
    case EXP_ATOM:
        switch (exp.atom.type) {
        case ATOM_SYMBOL: printf("%s", exp.atom.symbol); break;
        case ATOM_NUMBER: printf("%g", exp.atom.number); break;
        }
        break;
    case EXP_LIST:
        printf("(");
        for (size_t i = 0; i < exp.list.size; i++) {
            print(exp.list.data[i]);
            if (i != exp.list.size-1) {
                printf(" ");
            }
        }
        printf(")");
        break;
    case EXP_C_PROC:
        printf("<built-in C proc>");
        break;
    case EXP_VOID:
        printf("<void>");
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
    }
}

int main()
{
    repl();
    return 0;
}

