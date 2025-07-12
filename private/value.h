#pragma once

#include "array.h"
#include <stddef.h>
#include <stdlib.h>

typedef double lox_value;

DECLARE_LOX_ARRAY(lox_value, value_array);

// Helper function to print a `lox_value`
void print_value(lox_value value);

lox_value lox_value_from_string(const char *string);
