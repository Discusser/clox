#include "value.h"
#include "memory.h"
#include <stdio.h>
#include <string.h>

DEFINE_LOX_ARRAY(lox_value, value_array);

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
  }
}

lox_value bool_value(bool val) {
  return (lox_value){VAL_BOOL, {.boolean = val}};
}

lox_value nil_value() { return (lox_value){VAL_NIL, {.number = 0}}; }

lox_value number_value(double val) {
  return (lox_value){VAL_NUMBER, {.number = val}};
}
