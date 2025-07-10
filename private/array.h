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
  void name##_initialize(lox_##name *name);                                    \
  void name##_free(lox_##name *name);                                          \
  void name##_push(lox_##name *name, type value);                              \
  type name##_pop(lox_##name *name);                                           \
  void name##_grow(lox_##name *name);                                          \
  void name##_grow_to(lox_##name *name, size_t min_size, int zeroed);          \
  void name##_grow_zeroed(lox_##name *name);

// Creates all the function definitions for a dynamic array type. See also
// `DECLARE_LOX_ARRAY`
#define DEFINE_LOX_ARRAY(type, name)                                           \
  void name##_initialize(lox_##name *name) {                                   \
    name->size = 0;                                                            \
    name->capacity = 0;                                                        \
    name->values = NULL;                                                       \
  }                                                                            \
  void name##_free(lox_##name *name) {                                         \
    FREE_ARRAY(type, name->values, name->capacity);                            \
    name##_initialize(name);                                                   \
  }                                                                            \
  void name##_push(lox_##name *name, type value) {                             \
    if (name->size >= name->capacity) {                                        \
      name##_grow(name);                                                       \
    }                                                                          \
    name->values[name->size] = value;                                          \
    name->size++;                                                              \
  }                                                                            \
  type name##_pop(lox_##name *name) { return name->values[--name->size]; }     \
  void name##_grow(lox_##name *name) {                                         \
    name##_grow_to(name, name->size + 1, 0);                                   \
  }                                                                            \
  void name##_grow_to(lox_##name *name, size_t min_size, int zeroed) {         \
    int previous_capacity = name->capacity;                                    \
    name->capacity = GROW_CAPACITY(previous_capacity);                         \
    if (min_size > name->capacity)                                             \
      name->capacity = min_size;                                               \
    name->values =                                                             \
        GROW_ARRAY(type, name->values, previous_capacity, name->capacity);     \
    if (zeroed && name->capacity - previous_capacity > 0)                      \
      memset(&name->values[previous_capacity], '\0',                           \
             (name->capacity - previous_capacity) * sizeof(type));             \
  }                                                                            \
  void name##_grow_zeroed(lox_##name *name) {                                  \
    name##_grow_to(name, name->size + 1, 1);                                   \
  }

// Some useful array types
DECLARE_LOX_ARRAY(uint8_t, byte_array);
DECLARE_LOX_ARRAY(int, int_array);
