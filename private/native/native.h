#pragma once

#include "object.h"
#include "vm.h"

void define_natives(lox_vm *vm);

lox_value clock_native(int argc, lox_value *argv);
