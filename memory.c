#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ht.h"
#include "scheme.h"
#include "env.h"
#include "vector.h"

// typedef struct Graystack {
//     GCObject **data;
//     size_t size;
//     size_t cap;
// } Graystack;

// VECTOR_DEFINE_INIT(Graystack, GCObject *, graystack)
// VECTOR_DEFINE_ADD(Graystack, GCObject *, graystack)
// VECTOR_DEFINE_FREE(Graystack, GCObject *, graystack)

static struct {
    size_t bytes_allocated;
    size_t next;
    Env *cur_env;
    GCObject *obj_list;
    Env *env_list;
    // Graystack graystack;
} gc = {
    .bytes_allocated = 0,
    .next = 8192,
    .cur_env = NULL,
    .obj_list = NULL,
    .env_list = NULL,
    // .graystack = VECTOR_INIT()
};

void mark_env(Env *e);

void mark_exp(Exp exp)
{
    if (exp.type != EXP_LIST && exp.type != EXP_PROC && exp.type != EXP_SYMBOL) {
        return;
    }
    if (exp.obj->marked) {
        return;
    }
    exp.obj->marked = true;
    switch (exp.type) {
    case EXP_LIST:
        for (size_t i = 0; i < exp.obj->list.size; i++) {
            mark_exp(exp.obj->list.data[i]);
        }
        break;
    case EXP_PROC:
        for (size_t i = 0; i < exp.obj->proc.params.size; i++) {
            mark_exp(exp.obj->proc.params.data[i]);
        }
        mark_exp(exp.obj->proc.body);
        mark_env(exp.obj->proc.env);
        break;
    default:
        break;
    }
    // graystack_add(&gc.graystack, obj);
}

void mark_env(Env *e)
{
    if (!e || e->marked) {
        return;
    }
    e->marked = true;
    HT_FOR_EACH(e->ht, entry) {
        if (!entry) {
            continue;
        }
        Exp exp = entry->value;
        mark_exp(exp);
    }
}

void free_obj(GCObject *o)
{
    switch (o->type) {
    case EXP_SYMBOL:
        FREE_ARRAY(char, o->symbol, strlen(o->symbol));
        break;
    case EXP_LIST:
        list_free(&o->list);
        break;
    case EXP_PROC:
        list_free(&o->proc.params);
        break;
    default:
        break;
    }
    FREE(GCObject, o);
}

void sweep_objects()
{
    GCObject *cur = gc.obj_list, *prev = NULL;
    while (cur) {
        if (cur->marked) {
            cur->marked = false;
            prev = cur;
            cur = cur->next;
        } else {
            GCObject *unreached = cur;
            cur = cur->next;
            if (prev) {
                prev->next = cur;
            } else {
                gc.obj_list = cur;
            }
            free_obj(unreached);
        }
    }
}

void free_env(Env *e)
{
    ht_free(&e->ht);
    FREE(Env, e);
}

void sweep_envs()
{
    Env *cur = gc.env_list, *prev = NULL;
    while (cur) {
        if (cur->marked) {
            prev = cur;
            cur = cur->next;
        } else {
            Env *unreached = cur;
            cur = cur->next;
            if (prev) {
                prev->next = cur;
            } else {
                gc.env_list = cur;
            }
            free_env(unreached);
        }
    }
}

/* public functions start here: */

void *reallocate(void *ptr, size_t old, size_t new)
{
    gc.bytes_allocated += new - old;

    if (new > old) {
        printf("allocating %ld bytes...\n", new - old);
        // gc_collect();
    } else if (new == 0) {
        printf("freeing %ld bytes...\n", old);
    }

    if (gc.bytes_allocated > gc.next) {
        // gc_collect();
    }

    if (new == 0) {
        free(ptr);
        return NULL;
    }

    void *res = realloc(ptr, new);
    if (!res) {
        abort();
    }
    return res;
}

void gc_collect()
{
    printf("collecting memory...\n");
    mark_env(gc.cur_env);
    sweep_objects();
    // sweep_envs();
}

void gc_set_current_env(Env *env)
{
    gc.cur_env = env;
}

char *mem_strdup(const char *s, size_t size)
{
    char *dup = ALLOCATE(char, size);
    memcpy(dup, s, size);
    return dup;
}

GCObject *alloc_obj(GCObject from)
{
    GCObject *obj = ALLOCATE(GCObject, 1);
    memcpy(obj, &from, sizeof(GCObject));
    obj->marked = false;
    obj->next = gc.obj_list;
    gc.obj_list = obj;
    return obj;
}

