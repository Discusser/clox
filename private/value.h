#pragma once

#include "array.h"
#include <stddef.h>

typedef double lox_value;

DECLARE_LOX_ARRAY(lox_value, value_array);

// Helper function to print a `lox_value`
void print_value(lox_value value);
