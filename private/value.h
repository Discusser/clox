#pragma once

#include "array.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
} lox_value_type;

typedef struct {
  lox_value_type type;
  union {
    bool boolean;
    double number;
  } as;
} lox_value;

DECLARE_LOX_ARRAY(lox_value, value_array);

// Helper function to print a `lox_value`
void print_value(lox_value value);

lox_value bool_value(bool val);
lox_value nil_value();
lox_value number_value(double val);
