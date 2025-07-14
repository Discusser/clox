#include "value.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>
#include "object.h"

void value_array_initialize(lox_value_array *value_array) {
  value_array->size = 0;
  value_array->capacity = 0;
  value_array->values = ((void *)0);
}
void value_array_free(lox_value_array *value_array) {
  reallocate(value_array->values, sizeof(lox_value) * (value_array->capacity),
             0);
  value_array_initialize(value_array);
}
void value_array_push(lox_value_array *value_array, lox_value value) {
  if (value_array->size >= value_array->capacity) {
    value_array_grow(value_array);
  }
  value_array->values[value_array->size] = value;
  value_array->size++;
}
lox_value value_array_pop(lox_value_array *value_array) {
  return value_array->values[--value_array->size];
}
void value_array_grow(lox_value_array *value_array) {
  value_array_grow_to(value_array, value_array->size + 1, 0);
}
void value_array_grow_to(lox_value_array *value_array, size_t min_size,
                         int zeroed) {
  int previous_capacity = value_array->capacity;
  value_array->capacity =
      ((previous_capacity) < 8 ? 8 : (previous_capacity) * 2);
  if (min_size > value_array->capacity)
    value_array->capacity = min_size;
  value_array->values = (lox_value *)reallocate(
      value_array->values, sizeof(lox_value) * (previous_capacity),
      sizeof(lox_value) * (value_array->capacity));
  if (zeroed && value_array->capacity - previous_capacity > 0)
    memset(&value_array->values[previous_capacity], '\0',
           (value_array->capacity - previous_capacity) * sizeof(lox_value));
}
void value_array_grow_zeroed(lox_value_array *value_array) {
  value_array_grow_to(value_array, value_array->size + 1, 1);
};

void print_value(lox_value value) {
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
    print_object(value.as.object);
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

bool lox_value_is_object(lox_value value) { return value.type == VAL_OBJECT; }

bool lox_value_is_string(lox_value value) {
  return lox_value_is_object(value) && value.as.object->type == OBJ_STRING;
}
