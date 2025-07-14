#include "memory.h"
#include "common.h"
#include <stdlib.h>

void *lox_reallocate(void *ptr, size_t old_size, size_t new_size) {
  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);
  if (result == NULL)
    exit(1);
  return result;
}

int lox_grow_capacity(int capacity) {
  return capacity < LOX_ARRAY_MIN_CAPACITY ? LOX_ARRAY_MIN_CAPACITY
                                           : capacity * LOX_ARRAY_SCALE_FACTOR;
}
