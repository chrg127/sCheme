#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ht.h"
#include "scheme.h"

static struct {
    size_t bytes_allocated;
    size_t next;
    Env *cur_env;
    GCObject *obj_list;
} gc = {
    .bytes_allocated = 0,
    .next = 8192,
    .cur_env = NULL,
    .obj_list = NULL,
};

void *reallocate(void *ptr, size_t old, size_t new)
{
    gc.bytes_allocated += new - old;

    if (new > old) {
        printf("allocating %ld bytes...\n", new - old);
        // gc_collect();
    }

    if (gc.bytes_allocated > gc.next) {
        gc_collect();
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
}

void gc_set_current_env(Env *env)
{
    gc.cur_env = env;
}

char *mem_strdup(const char *s, size_t size)
{
    char *dup = reallocate(NULL, 0, size);
    memcpy(dup, s, size);
    return dup;
}

GCObject *mkobj()
{
    GCObject *obj = ALLOCATE(GCObject, 1);
    obj->marked = false;
    obj->next = gc.obj_list;
    gc.obj_list = obj;
    return obj;
}

