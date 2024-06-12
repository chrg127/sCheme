#include "ht.h"

// An environment: a hashtable of ("var": exp) pairs, with an outer Env.
typedef struct Env {
    struct HashTable ht;
    struct Env *outer;
    // for gc:
    bool marked;
    struct Env *next;
} Env;

