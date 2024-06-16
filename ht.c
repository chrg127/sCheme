#include "ht.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "scheme.h"
#include "gcobject.h"

typedef uint32_t u32;
typedef uint8_t  u8;

// sample hash function
// algorithm: FNV-1a
static u32 hash_string(const char *str, size_t len)
{
    u32 hash = 2166136261u;
    for (size_t i = 0; i < len; i++) {
        hash ^= (u8) str[i];
        hash *= 16777619;
    }
    return hash;
}

// Key and value behavior.
// When changing types for HtKey and HtValue, you only need to
// modify these functions.
// Key types must have these traits: nullable, hashable and comparable
// Value types must have these traits: nullable

static inline bool is_empty_key(HtKey v)     { return v.type == EXP_EMPTY; }
static inline u32 hash(HtKey v)              { return hash_string(v.obj->symbol, strlen(v.obj->symbol)); }

static inline bool key_equal(HtKey a, HtKey b)
{
    return strcmp(a.obj->symbol, b.obj->symbol) == 0;
}

static inline bool is_empty_value(HtValue v) { return v.type == EXP_EMPTY; }

// note that empty entries and tombstone entries must both have empty keys
static inline void make_empty(HtEntry *entry)
{
    entry->key   = (Exp) { .type = EXP_EMPTY };
    entry->value = (Exp) { .type = EXP_EMPTY };
}

static inline void make_tombstone(HtEntry *entry)
{
    entry->key   = (Exp) { .type = EXP_EMPTY };
    entry->value = (Exp) { .type = 0xdeadbeef };
}



// the actual table implementation

#define HT_MAX_LOAD 0.75

static HtEntry *find_entry(HtEntry *entries, size_t cap, HtKey key)
{
    u32 i = hash(key) % cap;
    HtEntry *first_tombstone = NULL;
    for (;;) {
        HtEntry *ptr = &entries[i];
        // an entry is a tombstone if the key is empty and the value is not
        if (is_empty_key(ptr->key)) {
            if (is_empty_value(ptr->value))
                return first_tombstone != NULL ? first_tombstone : ptr;
            else if (first_tombstone == NULL)
                first_tombstone = ptr;
        } else if (key_equal(ptr->key, key))
            return ptr;
        i = (i + 1) & (cap - 1);
    }
}

static void adjust_cap(HashTable *tab, size_t cap)
{
    HtEntry *entries = (HtEntry *) tab->allocate(
        NULL, 0, sizeof(HtEntry) * cap
    );
    for (size_t i = 0; i < cap; i++)
        make_empty(&entries[i]);

    tab->size = 0;
    for (size_t i = 0; i < tab->cap; i++) {
        HtEntry *entry = &tab->entries[i];
        if (is_empty_key(entry->key))
            continue;
        HtEntry *dest = find_entry(entries, cap, entry->key);
        dest->key   = entry->key;
        dest->value = entry->value;
        tab->size++;
    }
    // free tab's array
    tab->allocate(tab->entries, sizeof(HtEntry) * tab->cap, 0);
    tab->entries = entries;
    tab->cap     = cap;
}

bool ht_install(HashTable *tab, HtKey key, HtValue value)
{
    // don't insert nil values
    if (is_empty_key(key))
        return false;

    if (tab->size + 1 > tab->cap * HT_MAX_LOAD) {
        size_t cap = tab->cap < 8 ? 8 : tab->cap * 2;
        adjust_cap(tab, cap);
    }

    HtEntry *entry = find_entry(tab->entries, tab->cap, key);
    bool is_new = is_empty_key(entry->key);
    if (is_new && is_empty_value(entry->value))
        tab->size++;
    entry->key   = key;
    entry->value = value;
    return is_new;
}

bool ht_lookup(HashTable *tab, HtKey key, HtValue *value)
{
    if (tab->size == 0)
        return false;
    HtEntry *entry = find_entry(tab->entries, tab->cap, key);
    if (is_empty_key(entry->key))
        return false;
    if (value)
        *value = entry->value;
    return true;
}

bool ht_delete(HashTable *tab, HtKey key)
{
    if (tab->size == 0)
        return false;
    HtEntry *entry = find_entry(tab->entries, tab->cap, key);
    if (is_empty_key(entry->key))
        return false;
    // place a tombstone
    make_tombstone(entry);
    return true;
}

void ht_add_all(HashTable *from, HashTable *to)
{
    for (size_t i = 0; i < from->cap; i++) {
        HtEntry *entry = &from->entries[i];
        if (is_empty_key(entry->key))
            ht_install(to, entry->key, entry->value);
    }
}

