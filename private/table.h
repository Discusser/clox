#pragma once

#include "object.h"
#include "value.h"

typedef struct lox_object_string lox_object_string;

typedef struct lox_hash_table_entry {
  lox_value key;
  lox_value value;
} lox_hash_table_entry;

typedef struct lox_hash_table {
  int count;
  int capacity;
  lox_hash_table_entry *entries;
} lox_hash_table;

// Initializes a lox_hash_table pointer
void lox_hash_table_init(lox_hash_table *table);
// Frees a lox_hash_table pointer.
void lox_hash_table_free(lox_hash_table *table);

// Copies the contents of `from` into `to`
void lox_hash_table_copy_to(lox_hash_table *from, lox_hash_table *to);
// Puts an entry with the given key and value into the table, expanding the
// table if necessary. `key` should not be NULL. Returns true if the key didn't
// exist previously in the table, or false if it did, that is, the return value
// indicates if this key is a new key.
bool lox_hash_table_put(lox_hash_table *table, lox_value key, lox_value value);
// Removes an entry with the given key from the hash table. Returns true if the
// entry was removed, or false if there was no entry with the given key.
bool lox_hash_table_remove(lox_hash_table *table, lox_value key);
// Retrieves the value associated with a key. The value is stored in the `value`
// pointer passed to the function. Returns true if the key exists in the table,
// or false if it doesn't, in which case `value` is unmodified. If `value` is
// NULL, it is also unmodified.
bool lox_hash_table_get(lox_hash_table *table, lox_value key, lox_value *value);
// Returns true if the key exists in the table, or false if it doesn't. This is
// equivalent to lox_hash_table_get(table, key, NULL)
bool lox_hash_table_has(lox_hash_table *table, lox_value key);
// Finds the entry where a key should go, given a list of entries and a
// capacity. `key` should not be NULL
lox_hash_table_entry *lox_hash_table_find_entry(lox_hash_table_entry *entries,
                                                int capacity, lox_value key);
// Finds the key containing the given string. The hash is required for faster
// comparison, since two strings different hashes are certainly not equal. This
// only exists to 'bootstrap' lox_hash_table_find_entry by deduplicating
// strings.
lox_object_string *lox_hash_table_find_string(lox_hash_table *table,
                                              const char *chars, int length,
                                              uint32_t hash);
// Resizes a hash table to the given size.
void lox_hash_table_resize(lox_hash_table *table, int new_capacity);
void lox_hash_table_remove_white(lox_hash_table *table);
