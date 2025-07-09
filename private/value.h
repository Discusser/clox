#pragma once

#include "common.h"

typedef double lox_value;

typedef struct {
  int capacity;
  int size;
  lox_value *values;
} lox_value_array;

void initialize_value_array(lox_value_array *value_array);
void free_value_array(lox_value_array *value_array);
void write_to_value_array(lox_value_array *value_array, lox_value value);
void print_value(lox_value value);
