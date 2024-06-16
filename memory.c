#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ht.h"
#include "scheme.h"
#include "gcobject.h"
#include "vector.h"

#define GC_HEAP_GROW_FACTOR 2

static struct {
    size_t bytes_allocated;
    size_t next;
    GCObject *obj_list;
    Env *env_list;
    GCObject *savestack[BUFSIZ];
    int sp;
    Env *envstack[BUFSIZ];
    int env_sp;
} gc = {
    .bytes_allocated = 0,
    .next = 1024 * 1024,
    .obj_list = NULL,
    .env_list = NULL,
    .sp = 0,
    .env_sp = 0,
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
            if (is_obj(obj->list.data[i])) {
                mark_obj(obj->list.data[i].obj);
            }
        }
        break;
    case GC_PROC:
        mark_obj(obj->proc.params.obj); // always a list
        if (is_obj(obj->proc.body)) {
            mark_obj(obj->proc.body.obj); // may just be a simple number...
        }
        mark_obj(obj->proc.env.obj);
        break;
    case GC_HT:
        HT_FOR_EACH(obj->ht, entry) {
            if (!entry) {
                continue;
            }
            mark_obj(entry->key.obj); // always a symbol
            if (is_obj(entry->value)) {
                mark_obj(entry->value.obj);
            }
        }
    default:
        break;
    }
}

void free_obj(GCObject *o)
{
#ifdef DEBUG
    printf("freeing object of type %d\n", o->type);
#endif
    switch (o->type) {
    case GC_SYMBOL:
        FREE_ARRAY(char, o->symbol, strlen(o->symbol) + 1);
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
    for (GCObject *obj = gc.obj_list; obj; obj = obj->next) {
        obj->marked = false;
    }
}

void *reallocate(void *ptr, size_t old, size_t new)
{
    gc.bytes_allocated += (new - old);

    if (new == 0) {
#ifdef DEBUG
        printf("freeing %ld bytes...\n", old);
#endif
        free(ptr);
        return NULL;
    }

    if (new > old) {
#ifdef DEBUG
        printf("allocating %ld bytes...\n", new - old);
#endif
        // gc_collect();
    }

    if (gc.bytes_allocated > gc.next) {
        // gc_collect();
        gc.next = gc.bytes_allocated * GC_HEAP_GROW_FACTOR;
    }

    void *res = realloc(ptr, new);
    if (!res) {
        abort();
    }
    return res;
}

void gc_collect()
{
#ifdef DEBUG
    printf("collecting memory...\n");
#endif
    for (int i = 0; i < gc.env_sp; i++) {
        mark_obj(gc.envstack[i]->obj);
    }
    for (int i = 0; i < gc.sp; i++) {
        mark_obj(gc.savestack[i]);
    }
    sweep_objects();
}

void gc_push_env(Env *env) { gc.envstack[gc.env_sp++] = env; }
void gc_pop_env()          { gc.env_sp--; }

void gc_save(GCObject *obj) { gc.savestack[gc.sp++] = obj; }
void gc_unsave()            { gc.sp--; }

void gc_sweep()
{
    sweep_objects();
#ifdef DEBUG
    if (gc.bytes_allocated == 0) {
        printf("hooray! nothing allocated anymore!\n");
    }
#endif
}

GCObject *alloc_obj(GCObject from)
{
    printf("allocating object of type %d\n", from.type);
    GCObject *obj = ALLOCATE(GCObject, 1);
    memcpy(obj, &from, sizeof(GCObject));
    obj->marked = false;
    obj->next = gc.obj_list;
    gc.obj_list = obj;
    return obj;
}

