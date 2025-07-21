#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "memory.h"
#include "object.h"
#include "vm.h"

DEFINE_LOX_ARRAY(lox_value, value_array);

void lox_print_value(lox_value value) {
  switch (value.type) {
  case VAL_BOOL:
    printf("%s", value.as.boolean ? "true" : "false");
    break;
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUMBER: {
    char *buf;
    int len;
    if ((len = asprintf(&buf, "%f", value.as.number)) == -1) {
      runtime_error(
          "An internal error occurred while trying to print a number.");
    }
    bool found_dot = false;
    int previous_digit_index = -1;
    for (int i = 0; i < len; i++) {
      if (buf[i] == '.') {
        found_dot = true;
        previous_digit_index = i - 1;
      }
      if (found_dot) {
        if ('1' <= buf[i] && buf[i] <= '9')
          previous_digit_index = i;
      }
    }
    if (found_dot)
      len = previous_digit_index + 1;
    printf("%.*s", len, buf);
    free(buf);
    break;
  }
  case VAL_OBJECT:
    lox_print_object(value.as.object);
    break;
  case VAL_EMPTY:
    printf("EMPTY");
    break;
  }
}

inline lox_value lox_value_from_bool(bool val) {
  return (lox_value){VAL_BOOL, {.boolean = val}};
}

inline lox_value lox_value_from_nil() {
  return (lox_value){VAL_NIL, {.number = 0}};
}

inline lox_value lox_value_from_number(double val) {
  return (lox_value){VAL_NUMBER, {.number = val}};
}

inline lox_value lox_value_from_object(lox_object *object) {
  return (lox_value){VAL_OBJECT, {.object = object}};
}

inline lox_value lox_value_from_empty() {
  return (lox_value){VAL_EMPTY, {.number = 0}};
}

uint32_t lox_value_hash(lox_value value) {
  switch (value.type) {
  case VAL_BOOL:
    return value.as.boolean;
  case VAL_NIL:
    return 3;
  case VAL_NUMBER:
    return lox_value_hash_number(value.as.number);
  case VAL_OBJECT:
    // If this is a string, we can use the cached hash. If we don't have any
    // hash function provided, we just pass the pointer to the object. This can
    // result in getting the same hash for different objects, since uint32_t is
    // a smaller type than intptr_t on 64-bit architectures, but this is good
    // enough for a default implementation, since a good hash function cannot be
    // generalized to every object, given that a lox_object does not contain
    // much information.
    if (lox_value_is_string(value))
      return ((lox_object_string *)value.as.object)->hash;
    else
      // We cast to intptr_t first so that clangd doesn't complain about
      // uint32_t being smaller than the pointer address. I don't know if this
      // actually makes a difference.
      return (uint32_t)(intptr_t)value.as.object;
  case VAL_EMPTY:
  default:
    return 0;
  }
}

uint32_t lox_value_hash_number(double number) {
  union bitcast {
    double value;
    uint32_t ints[2];
  };

  union bitcast cast;
  cast.value = (number) + 1.0;
  return cast.ints[0] + cast.ints[1];
}

inline bool lox_value_is_object(lox_value value) {
  return value.type == VAL_OBJECT;
}

inline bool lox_value_is_string(lox_value value) {
  return lox_value_is_object(value) && value.as.object->type == OBJ_STRING;
}

inline bool lox_value_is_function(lox_value value) {
  return lox_value_is_object(value) && value.as.object->type == OBJ_FUNCTION;
}

inline bool lox_value_is_native(lox_value value) {
  return lox_value_is_object(value) && value.as.object->type == OBJ_NATIVE;
}
