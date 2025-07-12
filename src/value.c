#include "value.h"
#include "memory.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

DEFINE_LOX_ARRAY(lox_value, value_array);

void print_value(lox_value value) { printf("%g", value); }

lox_value lox_value_from_string(const char *string) {
  return strtod(string, NULL);
}
