#include "ht.h"
#include "scheme.h"

typedef enum GCObjectType {
    GC_SYMBOL = 3,
    GC_LIST = 4,
    GC_PROC = 5,
    GC_HT = 6,
} GCObjectType;

typedef struct GCObject {
    GCObjectType type;
    union {
        Symbol symbol;
        List list;
        Procedure proc;
        HashTable ht;
    };
    bool marked;
    struct GCObject *next;
} GCObject;

#define AS_LIST(e) (e).obj->list
#define AS_PROC(e) (e).obj->proc
#define AS_SYM(e) (e).obj->symbol

static inline bool is_obj(Exp exp)
{
    return exp.type == EXP_LIST || exp.type == EXP_PROC || exp.type == EXP_SYMBOL;
}

GCObject *alloc_obj(GCObject from);

static inline Exp mkobj(ExpType type, GCObject from)
{
    return (Exp) { .type = type, .obj = alloc_obj(from) };
}

static inline Exp mksym(Symbol s)
{
    return mkobj(EXP_SYMBOL, (GCObject) { .type = GC_SYMBOL, .symbol = s });
}

static inline Exp mklist(List l)
{
    return mkobj(EXP_LIST, (GCObject) { .type = GC_LIST, .list = l });
}

static inline Exp mkproc(Exp params, Exp body, Env env)
{
    return mkobj(EXP_PROC, (GCObject) {
        .type = GC_PROC,
        .proc = (Procedure) { .params = params, .body = body, .env = env, }
    });
}

static inline Env new_env(Env *outer)
{
    return (Env) {
        .obj = alloc_obj((GCObject) { .type = GC_HT, .ht = HT_INIT_WITH_ALLOCATOR(reallocate) }),
        .outer = outer,
    };
}

