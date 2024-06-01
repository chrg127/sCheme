#ifndef VECTOR_H_INCLUDED
#define VECTOR_H_INCLUDED

#define VECTOR_INIT(v, data_name) \
    do { \
        (v)->size = 0; \
        (v)->cap  = 0; \
        (v)->data_name = NULL; \
    } while (0)

static inline size_t vector_grow_cap(size_t old_cap)
{
    return old_cap < 8 ? 8 : old_cap * 2;
}

#define GROW_ARRAY(type, ptr, old, new) \
    (type *) realloc(ptr, new)

#define FREE_ARRAY(type, ptr, size) \
    free(ptr)

#define VECTOR_DECLARE_INIT(T, TVal, header)  void header##_init(T *arr)
#define VECTOR_DECLARE_FREE(T, TVal, header)  void header##_write(T *arr, TVal value)
#define VECTOR_DECLARE_WRITE(T, TVal, header) void header##_free(T *arr)
#define VECTOR_DECLARE_SEARCH(T, TVal, header) TVal *header##_search(T *arr, TVal value)
#define VECTOR_DECLARE_DELETE(T, TVal, header) void header##_delete(T *arr, TVal value)

#define VECTOR_DEFINE_INIT(T, TVal, header, data_name)  \
    void header##_init(T *arr)                          \
    {                                                   \
        arr->size = 0;                                  \
        arr->cap  = 0;                                  \
        arr->data_name = NULL;                          \
    }                                                   \

#define VECTOR_DEFINE_WRITE(T, TVal, header, data_name) \
    void header##_write(T *arr, TVal value)             \
    {                                                   \
        if (arr->cap < arr->size + 1) {                 \
            size_t old = arr->cap;                      \
            arr->cap = vector_grow_cap(old);            \
            arr->data_name = GROW_ARRAY(TVal, arr->data_name, old, arr->cap);         \
        }                                               \
        arr->data_name[arr->size++] = value;            \
    }                                                   \

#define VECTOR_DEFINE_FREE(T, TVal, header, data_name)  \
    void header##_free(T *arr)                          \
    {                                                   \
        FREE_ARRAY(TVal, arr->data_name, arr->cap);     \
        header##_init(arr);                             \
    }                                                   \

#define VECTOR_DEFINE_SEARCH(T, TVal, header, data_name) \
    TVal *header##_search(T *arr, TVal elem)             \
    {                                                    \
        for (size_t i = 0; i < arr->size; i++) {         \
            if (arr->data_name[i] == elem)               \
                return &arr->data_name[i];               \
        }                                                \
        return NULL;                                     \
    }                                                    \

#define VECTOR_DEFINE_DELETE(T, TVal, header, data_name) \
    void header##_delete(T *arr, TVal elem)              \
    {                                                    \
        TVal *p = header##_search(arr, elem);            \
        if (!p)                                          \
            return;                                      \
        memmove(p, p+1, arr->size - (p - arr->data));    \
        arr->size--;                                     \
    }                                                    \

// example
typedef unsigned char u8;

typedef struct {
    u8 *data;
    size_t size;
    size_t cap;
} VecU8;

VECTOR_DECLARE_INIT(VecU8, u8, u8vec);
VECTOR_DECLARE_WRITE(VecU8, u8, u8vec);
VECTOR_DECLARE_FREE(VecU8, u8, u8vec);
VECTOR_DECLARE_SEARCH(VecU8, u8, u8vec);
VECTOR_DECLARE_DELETE(VecU8, u8, u8vec);

#endif
