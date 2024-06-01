#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "ht.h"
#include "vector.h"

typedef char *Symbol;   // A Scheme Symbol is implemented as a C string
typedef int Number;     // A Scheme number is implemented as a C int

// A Scheme Atom is a Symbol or Number
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

// A Scheme expression is an Atom or a List
struct Exp {
    int type;
    union {
        Atom atom;
        List list;
    };
};

VECTOR_DECLARE_INIT(List, Exp, list);
VECTOR_DECLARE_ADD(List, Exp, list);
VECTOR_DECLARE_FREE(List, Exp, list);

VECTOR_DEFINE_INIT(List, Exp, list)
VECTOR_DEFINE_ADD(List, Exp, list)
VECTOR_DEFINE_FREE(List, Exp, list)

char *substr(const char *s, size_t i, size_t j)
{
    char *r = malloc(j - i + 1);
    memcpy(r, s + i, j - i + 1);
    r[j - i] = '\0';
    return r;
}

typedef struct Tokenizer {
    size_t i;
    const char *s;
    size_t len;
} Tokenizer;

// Parse next token from string inside tokenizer.
char *next_token(Tokenizer *t)
{
    while (t->s[t->i] == ' ')
        t->i++;
    if (t->i == t->len) {
        return NULL;
    } else if (t->s[t->i] == '(' || t->s[t->i] == ')') {
        t->i++;
        return t->s[t->i-1] == '(' ? "(" : ")";
    }
    size_t start = t->i;
    while (t->s[t->i] != ' ' && t->s[t->i] != '(' && t->s[t->i] != ')' && t->i < t->len)
        t->i++;
    return substr(t->s, start, t->i);
}

// Numbers become numbers; every other token is a symbol.
Atom atom(char *token) {
    char *endptr;
    long num = strtol(token, &endptr, 0);
    return endptr == token ? (Atom) { .type = 0, .symbol = token }
                           : (Atom) { .type = 1, .number = num   };
}

// Read an expression from a sequence of tokens.
Exp read_from_tokens(Tokenizer *t) {
    char *token = next_token(t);
    if (token == NULL) {
        fprintf(stderr, "error: unexpected EOF\n");
        exit(1);
    }
    if (token[0] == '(') {
        List list;
        list_init(&list);
        Exp exp;
        while (exp = read_from_tokens(t), exp.type != 2) {
            list_add(&list, exp);
        }
        return (Exp) { .type = 1, .list = list };
    } else if (token[0] == ')') {
        return (Exp) { .type = 2 }; // erroneous unless catched by a '('
    } else {
        return (Exp) { .type = 0, .atom = atom(token) };
    }
}

// Read a scheme expression from a string.
Exp parse(const char *s)
{
    Tokenizer t = { .i = 0, .s = s, .len = strlen(s) };
    return read_from_tokens(&t);
}

void print_exp(Exp exp)
{
    switch (exp.type) {
    case 0:
        switch (exp.atom.type) {
        case 0: printf("%s\n", exp.atom.symbol); break;
        case 1: printf("%d\n", exp.atom.number); break;
        }
        break;
    case 1:
        for (size_t i = 0; i < exp.list.size; i++) {
            print_exp(exp.list.data[i]);
        }
        break;
    case 2:
        fprintf(stderr, "error: unexpected ')'\n");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        return 1;
    }
    Exp exp = parse(argv[1]);
    print_exp(exp);
    return 0;
}
