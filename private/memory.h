#pragma once

#include "common.h"
#include "table.h"
#include "value.h"
#include <stdio.h>

// Helper macros for allocating, reallocating, or freeing memory.
#define GROW_CAPACITY(capacity) lox_grow_capacity(capacity)
#define GROW_ARRAY(type, ptr, old_size, new_size)                              \
  (type *)lox_reallocate(ptr, sizeof(type) * (old_size),                       \
                         sizeof(type) * (new_size))
#define FREE_ARRAY(type, ptr, size)                                            \
  (ptr == NULL ? NULL : lox_reallocate(ptr, sizeof(type) * (size), 0))
#define FREE(type, ptr)                                                        \
  (ptr == NULL ? NULL : lox_reallocate(ptr, sizeof(type), 0))
#define ALLOC_ARRAY(type, nemb) lox_reallocate(NULL, 0, sizeof(type) * (nemb))
#define ALLOC_SIZE(size) lox_reallocate(NULL, 0, size)
#define ALLOC_TYPE(type) lox_reallocate(NULL, 0, sizeof(type))

// Wrapper function for `realloc` from `stdlib.h`. This also handles the case
// when new_size is 0, in which case the pointer is freed.
void *lox_reallocate(void *ptr, ssize_t old_size, ssize_t new_size);

// Implementing function for the GROW_CAPACITY macro. If capacity is smaller
// than LOX_ARRAY_MIN_CAPACITY, returns LOX_ARRAY_MIN_CAPACITY, else this
// function returns capacity * LOX_ARRAY_SCALE_FACTOR
int lox_grow_capacity(int capacity);

void collect_garbage();
void mark_roots();
void mark_value(lox_value value);
void mark_object(lox_object *obj);
void mark_table(lox_hash_table *table);
void mark_value_array(lox_value_array *array);
