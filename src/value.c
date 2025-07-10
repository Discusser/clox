#include "value.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

DEFINE_LOX_ARRAY(lox_value, value_array);

void print_value(lox_value value) { printf("%g", value); }
