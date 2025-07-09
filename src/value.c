#include "value.h"
#include "memory.h"
#include <stdio.h>

void initialize_value_array(lox_value_array *value_array) {
  value_array->size = 0;
  value_array->capacity = 0;
  value_array->values = NULL;
}

void free_value_array(lox_value_array *value_array) {
  FREE_ARRAY(lox_value, value_array->values, value_array->capacity);
  initialize_value_array(value_array);
}

void write_to_value_array(lox_value_array *value_array, lox_value value) {
  if (value_array->size >= value_array->capacity) {
    int previous_capacity = value_array->capacity;
    value_array->capacity = GROW_CAPACITY(value_array->capacity);
    value_array->values = GROW_ARRAY(lox_value, value_array->values,
                                     previous_capacity, value_array->capacity);
  }

  value_array->values[value_array->size] = value;
  value_array->size++;
}

void print_value(lox_value value) { printf("%g", value); }
