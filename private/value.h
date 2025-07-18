#pragma once

#include "array.h"
#include "object.h"
#include <stddef.h>
#include <stdbool.h>

typedef enum {
  VAL_BOOL,
  VAL_NIL,
  VAL_NUMBER,
  VAL_OBJECT,
  // Similar to VAL_NIL, but used internally to represent a value that is not
  // present, especially useful in places where VAL_NIL values are allowed.
  // Similar to JavaScript's undefined, except that this is not exposed to the
  // end user
  VAL_EMPTY,
} lox_value_type;

// Every value in lox is represented by a lox_value. A lox_value can store a
// primitive type (number, boolean, nil), but also a lox_object, pointed to by
// lox_value.as.object.
typedef struct lox_value {
  lox_value_type type;
  union {
    bool boolean;
    double number;
    lox_object *object;
  } as;
} lox_value;

DECLARE_LOX_ARRAY(lox_value, value_array);

// Helper function to print a `lox_value`
void lox_print_value(lox_value value);

// Helper functions to create a lox_value from a certain type.
lox_value lox_value_from_bool(bool val);
lox_value lox_value_from_nil();
lox_value lox_value_from_number(double val);
lox_value lox_value_from_object(lox_object *object);
lox_value lox_value_from_empty();

uint32_t lox_value_hash(lox_value value);
uint32_t lox_value_hash_number(double number);

// Helper functions to check if a lox_value is a specific child of lox_object
bool lox_value_is_object(lox_value value);
bool lox_value_is_string(lox_value value);
bool lox_value_is_function(lox_value value);
bool lox_value_is_native(lox_value value);
