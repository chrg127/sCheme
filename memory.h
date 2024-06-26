#pragma once

#include <stddef.h>

typedef struct Env Env;
typedef struct GCObject GCObject;

void *reallocate(void *ptr, size_t old, size_t new);
void gc_collect();
void gc_push_env(Env *env);
void gc_pop_env();
void gc_save(GCObject *obj);
void gc_unsave();
void gc_sweep();

#define ALLOCATE(type, count) \
    (type *) reallocate(NULL, 0, sizeof(type) * (count))

#define GROW_ARRAY(type, ptr, old, new) \
    (type *) reallocate(ptr, sizeof(type) * (old), sizeof(type) * (new))

#define FREE(type, ptr) reallocate(ptr, sizeof(type), 0)

#define FREE_ARRAY(type, ptr, old) \
    do { \
        reallocate(ptr, sizeof(type) * (old), 0); \
    } while (0)

