#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ht.h"
#include "scheme.h"
#include "gcobject.h"
#include "vector.h"

static struct {
    size_t bytes_allocated;
    size_t next;
    Env *cur_env;
    GCObject *obj_list;
    Env *env_list;
} gc = {
    .bytes_allocated = 0,
    .next = 8192,
    .cur_env = NULL,
    .obj_list = NULL,
    .env_list = NULL,
};

void mark_obj(GCObject *obj)
{
    if (!obj || obj->marked) {
        return;
    }
    obj->marked = true;
    switch (obj->type) {
    case GC_SYMBOL:
        break;
    case GC_LIST:
        for (size_t i = 0; i < obj->list.size; i++) {
            mark_obj(obj->list.data[i].obj);
        }
        break;
    case GC_PROC:
        mark_obj(obj->proc.params.obj);
        mark_obj(obj->proc.body.obj);
        mark_obj(obj->proc.env.obj);
        break;
    case GC_HT:
        HT_FOR_EACH(obj->ht, entry) {
            if (!entry) {
                continue;
            }
            mark_obj(entry->key.obj);
            ExpType type = entry->value.type;
            if (type == EXP_SYMBOL || type == EXP_LIST || type == EXP_PROC) {
                mark_obj(entry->value.obj);
            }
        }
    default:
        break;
    }
}

void free_obj(GCObject *o)
{
    switch (o->type) {
    case GC_SYMBOL:
        FREE_ARRAY(char, o->symbol, strlen(o->symbol));
        break;
    case GC_LIST:
        list_free(&o->list);
        break;
    case GC_PROC:
        break;
    case GC_HT:
        ht_free(&o->ht);
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
    mark_obj(gc.cur_env->obj);
    sweep_objects();
}

void gc_set_current_env(Env *env)
{
    gc.cur_env = env;
}

char *mem_strdup(const char *s, size_t size)
{
    char *dup = ALLOCATE(char, size+1);
    memcpy(dup, s, size);
    dup[size] = '\0';
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

