#pragma once

#include "array.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJECT,
} lox_value_type;

typedef struct lox_object lox_object;
typedef struct lox_object_string lox_object_string;

typedef struct {
  lox_value_type type;
  union {
    bool boolean;
    double number;
    lox_object *object;
  } as;
} lox_value;

DECLARE_LOX_ARRAY(lox_value, value_array);

// Helper function to print a `lox_value`
void print_value(lox_value value);

lox_value lox_value_from_bool(bool val);
lox_value lox_value_from_nil();
lox_value lox_value_from_number(double val);
lox_value lox_value_from_object(lox_object *object);

bool lox_value_is_object(lox_value value);
bool lox_value_is_string(lox_value value);
