#pragma once

#include <stddef.h>
#include <stdint.h>

// Creates all the type definitions and function declarations for a dynamic
// array type. This should be used in the header file, and in the corresponding
// source file, `DEFINE_LOX_ARRAY` should be called with the same arguments.
#define DECLARE_LOX_ARRAY(type, name)                                          \
  typedef struct {                                                             \
    int capacity;                                                              \
    int size;                                                                  \
    type *values;                                                              \
  } lox_##name;                                                                \
  void lox_##name##_initialize(lox_##name *name);                              \
  void lox_##name##_free(lox_##name *name);                                    \
  void lox_##name##_push(lox_##name *name, type value);                        \
  type lox_##name##_pop(lox_##name *name);                                     \
  void lox_##name##_grow(lox_##name *name);                                    \
  void lox_##name##_grow_to(lox_##name *name, size_t min_size, int zeroed);    \
  void lox_##name##_grow_zeroed(lox_##name *name);                             \
  void lox_##name##_resize(lox_##name *name, size_t size);                     \
  void lox_##name##_clear(lox_##name *name);

// Creates all the function definitions for a dynamic array type. See also
// `DECLARE_LOX_ARRAY`
#define DEFINE_LOX_ARRAY(type, name)                                           \
  void lox_##name##_initialize(lox_##name *name) {                             \
    name->size = 0;                                                            \
    name->capacity = 0;                                                        \
    name->values = NULL;                                                       \
  }                                                                            \
  void lox_##name##_free(lox_##name *name) {                                   \
    FREE_ARRAY(type, name->values, name->capacity);                            \
    lox_##name##_initialize(name);                                             \
  }                                                                            \
  void lox_##name##_push(lox_##name *name, type value) {                       \
    if (name->size >= name->capacity) {                                        \
      lox_##name##_grow(name);                                                 \
    }                                                                          \
    name->values[name->size++] = value;                                        \
  }                                                                            \
  inline type lox_##name##_pop(lox_##name *name) {                             \
    return name->values[--name->size];                                         \
  }                                                                            \
  void lox_##name##_grow(lox_##name *name) {                                   \
    lox_##name##_grow_to(name, name->size + 1, 0);                             \
  }                                                                            \
  void lox_##name##_grow_to(lox_##name *name, size_t min_size, int zeroed) {   \
    int previous_capacity = name->capacity;                                    \
    int capacity = name->capacity;                                             \
    capacity = GROW_CAPACITY(capacity);                                        \
    if (min_size > capacity)                                                   \
      capacity = min_size;                                                     \
    lox_##name##_resize(name, capacity);                                       \
    if (zeroed && name->capacity - previous_capacity > 0)                      \
      memset(&name->values[previous_capacity], '\0',                           \
             (name->capacity - previous_capacity) * sizeof(type));             \
  }                                                                            \
  void lox_##name##_grow_zeroed(lox_##name *name) {                            \
    lox_##name##_grow_to(name, name->size + 1, 1);                             \
  }                                                                            \
  void lox_##name##_resize(lox_##name *name, size_t size) {                    \
    name->values = GROW_ARRAY(type, name->values, name->capacity, size);       \
    name->capacity = size;                                                     \
    if (size < name->size)                                                     \
      name->size = size;                                                       \
  }                                                                            \
  void lox_##name##_clear(lox_##name *name) { lox_##name##_free(name); }

// Some useful array types
DECLARE_LOX_ARRAY(uint8_t, byte_array);
DECLARE_LOX_ARRAY(int, int_array);
