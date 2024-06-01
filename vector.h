#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

#define VECTOR_INIT(v) \
    do { \
        (v)->size = 0; \
        (v)->cap  = 0; \
        (v)->data = NULL; \
    } while (0)

static inline size_t vector_grow_cap(size_t old_cap)
{
    return old_cap < 8 ? 8 : old_cap * 2;
}

/*
 * Default definitions for allocating and freeing an array.
 * Modify them if you're using a custom allocator.
 */
#define VECTOR_ARRAY_ALLOC(type, ptr, old, new) \
    (type *) realloc(ptr, new)

#define VECTOR_ARRAY_FREE(type, ptr, size) \
    free(ptr)

#define VECTOR_DECLARE_INIT(T, TVal, header)  void header##_init(T *arr)
#define VECTOR_DECLARE_FREE(T, TVal, header)  void header##_add(T *arr, TVal value)
#define VECTOR_DECLARE_ADD(T, TVal, header) void header##_free(T *arr)
#define VECTOR_DECLARE_SEARCH(T, TVal, header) TVal *header##_search(T *arr, TVal value)
#define VECTOR_DECLARE_DELETE(T, TVal, header) void header##_delete(T *arr, TVal value)

#define VECTOR_DEFINE_INIT(T, TVal, header) \
void header##_init(T *arr)                  \
{                                           \
    arr->size = 0;                          \
    arr->cap  = 0;                          \
    arr->data = NULL;                       \
}                                           \

#define VECTOR_DEFINE_ADD(T, TVal, header) \
void header##_add(T *arr, TVal value)      \
{                                          \
    if (arr->cap < arr->size + 1) {        \
        size_t old = arr->cap;             \
        arr->cap = vector_grow_cap(old);   \
        arr->data = VECTOR_ARRAY_ALLOC(TVal, arr->data, old, arr->cap); \
    }                                      \
    arr->data[arr->size++] = value;        \
}                                          \

#define VECTOR_DEFINE_FREE(T, TVal, header)       \
void header##_free(T *arr)                        \
{                                                 \
    VECTOR_ARRAY_FREE(TVal, arr->data, arr->cap); \
    header##_init(arr);                           \
}                                                 \

#define VECTOR_DEFINE_SEARCH(T, TVal, header) \
TVal *header##_search(T *arr, TVal elem)      \
{                                             \
    for (size_t i = 0; i < arr->size; i++) {  \
        if (arr->data[i] == elem)             \
            return &arr->data[i];             \
    }                                         \
    return NULL;                              \
}                                             \

#define VECTOR_DEFINE_DELETE(T, TVal, header)       \
void header##_delete(T *arr, TVal elem)             \
{                                                   \
    TVal *p = header##_search(arr, elem);           \
    if (!p)                                         \
        return;                                     \
    memmove(p, p+1, arr->size - (p - arr->data));   \
    arr->size--;                                    \
}                                                   \

#endif
