#ifndef HT_H_INCLUDED
#define HT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>

#define HT_ALLOCATE(type, count) \
    (type *) calloc((count), sizeof(type))
#define HT_FREE_ARRAY(type, ptr, old) free(ptr);

typedef char *HtKey;
typedef void *HtValue;

typedef struct {
    HtValue key;
    HtValue value;
} Entry;

typedef struct HashTable {
    size_t size;
    size_t cap;
    Entry *entries;
} HashTable;

#define HT_INIT() { .size = 0, .cap = 0, .entries = NULL }

static inline void ht_init(HashTable *tab)
{
    tab->size    = 0;
    tab->cap     = 0;
    tab->entries = NULL;
}

static inline void ht_free(HashTable *tab)
{
    HT_FREE_ARRAY(Entry, tab->entries, tab->cap);
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
    for (Entry *entry = tab->entries; ((size_t) (entry - tab->entries)) < tab->cap; entry++)

#endif
