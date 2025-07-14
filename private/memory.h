#pragma once

#include "common.h"

// Helper macros for allocating, reallocating, or freeing memory.
#define GROW_CAPACITY(capacity) lox_grow_capacity(capacity)
#define GROW_ARRAY(type, ptr, old_size, new_size)                              \
  (type *)lox_reallocate(ptr, sizeof(type) * (old_size),                       \
                         sizeof(type) * (new_size))
#define FREE_ARRAY(type, ptr, size)                                            \
  lox_reallocate(ptr, sizeof(type) * (size), 0)
#define FREE(type, ptr) lox_reallocate(ptr, sizeof(type), 0)
#define ALLOC_ARRAY(type, nemb) lox_reallocate(NULL, 0, sizeof(type) * (nemb))
#define ALLOC_SIZE(size) lox_reallocate(NULL, 0, size)
#define ALLOC_TYPE(type) lox_reallocate(NULL, 0, sizeof(type))

// Wrapper function for `realloc` from `stdlib.h`. This also handles the case
// when new_size is 0, in which case the pointer is freed.
void *lox_reallocate(void *ptr, size_t old_size, size_t new_size);

// Implementing function for the GROW_CAPACITY macro. If capacity is smaller
// than LOX_ARRAY_MIN_CAPACITY, returns LOX_ARRAY_MIN_CAPACITY, else this
// function returns capacity * LOX_ARRAY_SCALE_FACTOR
int lox_grow_capacity(int capacity);
