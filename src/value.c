#include "value.h"
#include <stdio.h>
#include <string.h>
#include "memory.h"
#include "object.h"

DEFINE_LOX_ARRAY(lox_value, value_array);

void lox_print_value(lox_value value) {
  switch (value.type) {
  case VAL_BOOL:
    printf("%s", value.as.boolean ? "true" : "false");
    break;
  case VAL_NIL:
    printf("nil");
    break;
  case VAL_NUMBER:
    printf("%g", value.as.number);
    break;
  case VAL_OBJECT:
    lox_print_object(value.as.object);
    break;
  }
}

lox_value lox_value_from_bool(bool val) {
  return (lox_value){VAL_BOOL, {.boolean = val}};
}

lox_value lox_value_from_nil() { return (lox_value){VAL_NIL, {.number = 0}}; }

lox_value lox_value_from_number(double val) {
  return (lox_value){VAL_NUMBER, {.number = val}};
}

lox_value lox_value_from_object(lox_object *object) {
  return (lox_value){VAL_OBJECT, {.object = object}};
}

lox_value lox_value_from_empty() {
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

bool lox_value_is_object(lox_value value) { return value.type == VAL_OBJECT; }

bool lox_value_is_string(lox_value value) {
  return lox_value_is_object(value) && value.as.object->type == OBJ_STRING;
}
