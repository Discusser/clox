#include "native/native.h"
#include <time.h>

void define_natives(lox_vm *vm) { define_native(vm, "clock", clock_native, 0); }

lox_value clock_native(int argc, lox_value *argv) {
  return lox_value_from_number((double)clock() / CLOCKS_PER_SEC);
}
