#ifndef HT_H_INCLUDED
#define HT_H_INCLUDED

#include <stdbool.h>
#include <stddef.h>

// These typedefs are provided as examples. Redefine them to whatever you want.
typedef char * HtKey;
typedef char * HtValue;

typedef struct {
    HtValue key;
    HtValue value;
} Entry;

typedef struct HashTable {
    size_t size;
    size_t cap;
    Entry *entries;
} HashTable;

/*
 * install() returns whether it created (true) or modified (false) the key's value
 * lookup() returns whether the lookup went well
 * delete() returns whether the delete went well
 * add_all() copies elements from another ht
 */

void ht_init(HashTable *tab);
void ht_free(HashTable *tab);
bool ht_install(HashTable *tab, HtKey key, HtValue value);
bool ht_lookup(HashTable *tab, HtKey key, HtValue *value);
bool ht_delete(HashTable *tab, HtKey key);
void ht_add_all(HashTable *from, HashTable *to);
bool ht_empty_entry(Entry *entry);

#define HT_FOR_EACH(tab, entry) \
    for (Entry *entry = tab->entries; ((size_t) (entry - tab->entries)) < tab->cap; entry++)

#endif
