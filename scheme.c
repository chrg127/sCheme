#include "scheme.h"
#include "ht.h"

VECTOR_DEFINE_INIT(List, Exp, list)
VECTOR_DEFINE_ADD(List, Exp, list)
VECTOR_DEFINE_FREE(List, Exp, list)

char *substr(const char *s, size_t i, size_t j)
{
    char *r = malloc(j - i + 1);
    memcpy(r, s + i, j - i);
    r[j - i] = '\0';
    return r;
}

// typedef struct Token {
//     const char *s;
//     size_t start, end;
// } Token;

typedef struct Tokenizer {
    const char *s;
    size_t len;
    size_t i;
    char *prev;
    char *cur;
} Tokenizer;

// Parse next token from string inside tokenizer.
char *next_token(Tokenizer *t)
{
    t->prev = t->cur;
    while (t->s[t->i] == ' ' || t->s[t->i] == '\n')
        t->i++;
    if (t->i == t->len) {
        t->cur = NULL;
        return t->cur;
    } else if (t->s[t->i] == '(' || t->s[t->i] == ')') {
        t->i++;
        t->cur = t->s[t->i-1] == '(' ? "(" : ")";
        return t->cur;
    }
    size_t start = t->i;
    while (t->s[t->i] != ' ' && t->s[t->i] != '\n'
        && t->s[t->i] != '(' && t->s[t->i] != ')' && t->i < t->len)
        t->i++;
    t->cur = substr(t->s, start, t->i);
    return t->cur;
}

// Numbers become numbers; every other token is a symbol.
Atom atom(char *token) {
    char *endptr;
    long num = strtol(token, &endptr, 0);
    return endptr == token ? (Atom) { .type = ATOM_SYMBOL, .symbol = token }
                           : (Atom) { .type = ATOM_NUMBER, .number = num   };
}

// Read an expression from a sequence of tokens.
Exp read_from_tokens(Tokenizer *t) {
    next_token(t);
    char *token = t->prev;
    if (token == NULL) {
        return (Exp) { .type = EXP_EOF };
    } else if (token[0] == '(') {
        List list;
        list_init(&list);
        while (t->cur != NULL && t->cur[0] != ')') {
            Exp exp = read_from_tokens(t);
            list_add(&list, exp);
        }
        if (t->cur == NULL) {
            fprintf(stderr, "error: unexpected EOF\n");
            exit(1);
        }
        next_token(t); // pop off ')'
        return (Exp) { .type = EXP_LIST, .list = list };
    } else if (token[0] == ')') {
        fprintf(stderr, "unexpected ')'\n");
        exit(1);
    } else {
        return (Exp) { .type = EXP_ATOM, .atom = atom(token) };
    }
}

// Read a scheme expression from a string.
Exp parse(const char *s)
{
    Tokenizer t = { .i = 0, .s = s, .len = strlen(s) };
    next_token(&t);
    return read_from_tokens(&t);
}

Exp *expdup(Exp exp)
{
    Exp *mem = malloc(sizeof(exp));
    memcpy(mem, &exp, sizeof(exp));
    return mem;
}

// An environment with some scheme standard procedures.
Env standard_env()
{
    Env env = { .ht = HT_INIT(), .outer = NULL };
    ht_install(&env.ht, "+",  expdup(make_cproc_exp(scheme_sum)));
    ht_install(&env.ht, "-",  expdup(make_cproc_exp(scheme_sub)));
    ht_install(&env.ht, "*",  expdup(make_cproc_exp(scheme_mul)));
    ht_install(&env.ht, ">",  expdup(make_cproc_exp(scheme_gt)));
    ht_install(&env.ht, "<",  expdup(make_cproc_exp(scheme_lt)));
    ht_install(&env.ht, ">=", expdup(make_cproc_exp(scheme_ge)));
    ht_install(&env.ht, "<=", expdup(make_cproc_exp(scheme_le)));
    ht_install(&env.ht, "=",  expdup(make_cproc_exp(scheme_eq)));
    ht_install(&env.ht, "begin", expdup(make_cproc_exp(scheme_begin)));
    ht_install(&env.ht, "list", expdup(make_cproc_exp(scheme_list)));
    ht_install(&env.ht, "pi", expdup(make_number_exp(3.14159265358979323846)));
    ht_install(&env.ht, "cons", expdup(make_cproc_exp(scheme_cons)));
    ht_install(&env.ht, "car", expdup(make_cproc_exp(scheme_car)));
    ht_install(&env.ht, "cdr", expdup(make_cproc_exp(scheme_cdr)));
    ht_install(&env.ht, "length", expdup(make_cproc_exp(scheme_length)));
    ht_install(&env.ht, "null?", expdup(make_cproc_exp(scheme_is_null)));
    // ht_install(&env.ht, "eq?", expdup(make_cproc_exp(scheme_is_eq)));
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

static inline Exp proc_call(Procedure *proc, List args)
{
    Env env = { .ht = HT_INIT(), .outer = proc->env };
    for (size_t i = 0; i < args.size; i++) {
        ht_install(&env.ht, proc->params.data[i].atom.symbol, expdup(args.data[i]));
    }
    return eval(*proc->body, &env);
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
            fprintf(stderr, "undefined symbol: %s\n", x.atom.symbol);
            exit(1);
        }
        void *value;
        bool found = ht_lookup(&e->ht, x.atom.symbol, &value);
        if (!found) {
            fprintf(stderr, "error: couldn't find %s in env\n", x.atom.symbol);
            exit(1);
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
            fprintf(stderr, "define: bad syntax\n");
            exit(1);
        }
        ht_install(&env->ht, symbol.atom.symbol, expdup(eval(exp, env)));
        return (Exp) { .type = EXP_VOID };
    } else if (is_symbol(op) && strcmp(op.atom.symbol, "set!") == 0) {
        // assignment
        Exp symbol = x.list.data[1];
        Exp exp    = x.list.data[2];
        if (!is_symbol(symbol)) {
            fprintf(stderr, "set!: bad syntax\n");
            exit(1);
        }
        Env *e = env_find(env, symbol.atom.symbol);
        if (!e) {
            fprintf(stderr, "undefined symbol: %s\n", symbol.atom.symbol);
            exit(1);
        }
        ht_install(&e->ht, symbol.atom.symbol, expdup(eval(exp, env)));
        return (Exp) { .type = EXP_VOID };
    } else if (is_symbol(op) && strcmp(op.atom.symbol, "lambda") == 0) {
        // procedure
        Exp params = x.list.data[1];
        Exp body   = x.list.data[2];
        return (Exp) {
            .type = EXP_PROC,
            .proc = {
                .params = params.list,
                .body   = expdup(body),
                .env    = env
            }
        };
    }
    // procedure call
    Exp proc = eval(op, env);
    if (proc.type != EXP_C_PROC && proc.type != EXP_PROC) {
        fprintf(stderr, "error: not a procedure\n");
        exit(1);
    }
    List args = VECTOR_INIT();
    for (size_t i = 1; i < x.list.size; i++) {
        list_add(&args, eval(x.list.data[i], env));
    }
    return proc.type == EXP_C_PROC
        ? proc.cproc(args)
        : proc_call(&proc.proc, args);
}

void print(Exp exp)
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
void repl()
{
    Env env = standard_env();
    while (true) {
        printf("sCheme> ");
        char input[BUFSIZ] = {0};
        fgets(input, sizeof(input), stdin);
        Exp val = eval(parse(input), &env);
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
