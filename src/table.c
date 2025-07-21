#include "table.h"
#include "memory.h"
#include "vm.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void lox_hash_table_init(lox_hash_table *table) {
  table->capacity = 0;
  table->count = 0;
  table->entries = NULL;
}

void lox_hash_table_free(lox_hash_table *table) {
  FREE_ARRAY(lox_hash_table_entry, table->entries, table->capacity);
  lox_hash_table_init(table);
}

void lox_hash_table_copy_to(lox_hash_table *from, lox_hash_table *to) {
  for (int i = 0; i < from->capacity; i++) {
    lox_hash_table_entry entry = from->entries[i];
    if (entry.key.type != VAL_EMPTY) {
      lox_hash_table_put(to, entry.key, entry.value);
    }
  }
}

bool lox_hash_table_put(lox_hash_table *table, lox_value key, lox_value value) {
  if (table->count + 1 > table->capacity * LOX_HASH_TABLE_LOAD_FACTOR) {
    int capacity = GROW_CAPACITY(table->capacity);
    lox_hash_table_resize(table, capacity);
  }

  lox_hash_table_entry *entry =
      lox_hash_table_find_entry(table->entries, table->capacity, key);
  bool is_new_key = entry->key.type == VAL_EMPTY;
  // Only increment the count if the entry is not a tombstone entry, because
  // removing entries does not decrease the count
  if (is_new_key && entry->value.type == VAL_NIL)
    table->count++;

  entry->key = key;
  entry->value = value;

  return is_new_key;
}

bool lox_hash_table_remove(lox_hash_table *table, lox_value key) {
  if (table->count == 0)
    return false;

  lox_hash_table_entry *entry =
      lox_hash_table_find_entry(table->entries, table->capacity, key);
  if (entry->key.type == VAL_EMPTY)
    return false;

  entry->key = lox_value_from_empty();
  // 'Tombstone' value to indicate that there used to be a value at this entry
  entry->value = lox_value_from_bool(true);

  return true;
}

bool lox_hash_table_get(lox_hash_table *table, lox_value key,
                        lox_value *value) {
  if (table->count == 0)
    return false;

  lox_hash_table_entry *entry =
      lox_hash_table_find_entry(table->entries, table->capacity, key);
  if (entry->key.type == VAL_EMPTY)
    return false;

  if (value != NULL)
    *value = entry->value;
  return true;
}

bool lox_hash_table_has(lox_hash_table *table, lox_value key) {
  return lox_hash_table_get(table, key, NULL);
}

lox_hash_table_entry *lox_hash_table_find_entry(lox_hash_table_entry *entries,
                                                int capacity, lox_value key) {
  if (capacity == 0)
    return NULL;

  assert(key.type != VAL_EMPTY);

  int index = lox_value_hash(key) % capacity;
  lox_hash_table_entry *tombstone = NULL;

  while (true) {
    lox_hash_table_entry *entry = &entries[index];
    if (entry->key.type == VAL_EMPTY) {
      if (entry->value.type == VAL_NIL) {
        return tombstone == NULL ? entry : tombstone;
      } else if (entry->value.type == VAL_BOOL &&
                 entry->value.as.boolean == true && tombstone == NULL) {
        tombstone = entry;
      }
    } else if (lox_values_equal(entry->key, key)) {
      return entry;
    }

    index = (index + 1) % capacity;
  }
}

lox_object_string *lox_hash_table_find_string(lox_hash_table *table,
                                              const char *chars, int length,
                                              uint32_t hash) {
  if (table->count == 0)
    return NULL;

  // NOTE: Is it possible for the table to be full of tombstones, making this
  // loop infinite?
  int index = hash % table->capacity;

  while (true) {
    lox_hash_table_entry *entry = &table->entries[index];
    if (entry->key.type == VAL_EMPTY) {
      // If the entry is not a tombstone and is empty, that means we haven't
      // found anything.
      if (entry->value.type == VAL_NIL) {
        return NULL;
      }
    } else if (lox_value_is_string(entry->key)) {
      lox_object_string *str = (lox_object_string *)entry->key.as.object;
      if (str->length == length && str->hash == hash &&
          memcmp(str->chars, chars, length) == 0) {
        return str;
      }
    }

    index = (index + 1) % table->capacity;
  }
}

void lox_hash_table_resize(lox_hash_table *table, int new_capacity) {
  lox_hash_table_entry *entries =
      ALLOC_ARRAY(lox_hash_table_entry, new_capacity);
  for (int i = 0; i < new_capacity; i++) {
    entries[i].key = lox_value_from_empty();
    entries[i].value = lox_value_from_nil();
  }

  table->count = 0;
  for (int i = 0; i < table->capacity; i++) {
    lox_hash_table_entry entry = table->entries[i];
    if (entry.key.type == VAL_EMPTY)
      continue;

    lox_hash_table_entry *new_entry =
        lox_hash_table_find_entry(entries, new_capacity, entry.key);
    new_entry->key = entry.key;
    new_entry->value = entry.value;
    table->count++;
  }

  FREE_ARRAY(lox_hash_table_entry, table->entries, table->capacity);
  table->entries = entries;
  table->capacity = new_capacity;
}

void lox_hash_table_remove_white(lox_hash_table *table) {
  for (int i = 0; i < table->capacity; i++) {
    lox_hash_table_entry entry = table->entries[i];
    if (entry.key.type != VAL_EMPTY && !entry.key.as.object->is_marked) {
      lox_hash_table_remove(table, entry.key);
    }
  }
}
