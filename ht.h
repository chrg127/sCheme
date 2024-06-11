#ifndef HT_H_INCLUDED
#define HT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "scheme.h"

typedef char *HtKey;
typedef Exp HtValue;
typedef void *(*HtAllocator)(void *ptr, size_t old, size_t new);

typedef struct HtEntry {
    HtKey key;
    HtValue value;
} HtEntry;

typedef struct HashTable {
    size_t size;
    size_t cap;
    HtEntry *entries;
    HtAllocator allocate;
} HashTable;

static inline void *ht_default_allocator(void *ptr, size_t old, size_t new)
{
    (void) old;
    if (new == 0) {
        free(ptr);
        return NULL;
    }
    return realloc(ptr, new);
}

#define HT_INIT() \
{ .size = 0, .cap = 0, .entries = NULL, .allocate = ht_default_allocator }

#define HT_INIT_WITH_ALLOCATOR(allocator) \
{ .size = 0, .cap = 0, .entries = NULL, .allocate = allocator }

static inline void ht_init(HashTable *tab)
{
    tab->size    = 0;
    tab->cap     = 0;
    tab->entries = NULL;
    tab->allocate = ht_default_allocator;
}

static inline void ht_init_with_allocator(HashTable *tab, HtAllocator allocator)
{
    tab->size    = 0;
    tab->cap     = 0;
    tab->entries = NULL;
    tab->allocate = allocator;
}

static inline void ht_free(HashTable *tab)
{
    tab->allocate(tab->entries, sizeof(HtEntry) * tab->cap, 0);
    ht_init(tab);
}

// add (key, value) to tab and return whether it created (true)
// or modified (false) the key's value
bool ht_install(HashTable *tab, HtKey key, HtValue value);

// lookup key in tab (putting it in *value) and return if key was found
bool ht_lookup(HashTable *tab, HtKey key, HtValue *value);

// delete key from tab and return if key was actually deleted
bool ht_delete(HashTable *tab, HtKey key);

// copy all entries from another HashTable
void ht_add_all(HashTable *from, HashTable *to);

#define HT_FOR_EACH(tab, entry) \
    for (HtEntry *entry = (tab).entries; ((size_t) (entry - (tab).entries)) < (tab).cap; entry++)

#endif

