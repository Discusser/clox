#pragma once

#include "common.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) * 2)
#define GROW_ARRAY(type, ptr, old_size, new_size)                              \
  (type *)reallocate(ptr, sizeof(type) * (old_size), sizeof(type) * (new_size))
#define FREE_ARRAY(type, ptr, size) reallocate(ptr, sizeof(type) * (size), 0)
#define FREE(type, ptr) reallocate(ptr, sizeof(type), 0)
#define ALLOC_ARRAY(type, nemb) reallocate(NULL, 0, sizeof(type) * (nemb))
#define ALLOC_SIZE(size) reallocate(NULL, 0, size)
#define ALLOC_TYPE(type) reallocate(NULL, 0, sizeof(type))

// Wrapper function for `realloc` from `stdlib.h`. This also handles the case
// when new_size is 0, in which case the pointer is freed.
void *reallocate(void *ptr, size_t old_size, size_t new_size);
