#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include <stdio.h>

lox_vm vm;

void init_vm() {
  value_array_initialize(&vm.stack);
  reset_stack();
}

void free_vm() { value_array_free(&vm.stack); }

void reset_stack() { vm.stack.size = 0; }

void push(lox_value value) { value_array_push(&vm.stack, value); }

lox_value pop() { return value_array_pop(&vm.stack); }

interpret_result interpret(const char *source) {
  lox_chunk chunk;
  initialize_chunk(&chunk);

  if (!compile(source, &chunk)) {
    free_chunk(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code.values;

  interpret_result result = run();
  free_chunk(&chunk);

  return result;
}

interpret_result run() {
#define BINARY_OP(op)                                                          \
  do {                                                                         \
    double rhs = pop();                                                        \
    vm.stack.values[vm.stack.size - 1] op## = rhs;                             \
  } while (0)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("%-10s", "STACK");
    for (lox_value *elem = vm.stack.values;
         elem < vm.stack.values + vm.stack.size; elem++) {
      printf("[ ");
      print_value(*elem);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(vm.chunk, vm.ip - vm.chunk->code.values);
#endif

    uint8_t instruction;
    switch (instruction = read_byte()) {
    case OP_RETURN:
      print_value(pop());
      printf("\n");
      return INTERPRET_OK;
    case OP_CONSTANT: {
      lox_value constant = read_constant();
      push(constant);
      break;
    }
    case OP_CONSTANT_LONG: {
      lox_value constant = read_constant_long();
      push(constant);
      break;
    }
    case OP_ADD:
      BINARY_OP(+);
      break;
    case OP_SUBTRACT:
      BINARY_OP(-);
      break;
    case OP_MULTIPLY:
      BINARY_OP(*);
      break;
    case OP_DIVIDE:
      BINARY_OP(/);
      break;
    case OP_NEGATE:
      vm.stack.values[vm.stack.size - 1] *= -1;
      break;
    default:
      printf("Unknown instruction %i\n", instruction);
      return INTERPRET_RUNTIME_ERROR;
    }
  }

#undef BINARY_OP
}

uint8_t read_byte() { return *vm.ip++; }

uint32_t read_word() {
  uint32_t ret = *(uint32_t *)vm.ip;
  vm.ip += sizeof(uint32_t);
  return ret;
}

lox_value read_constant() { return vm.chunk->constants.values[read_byte()]; }

lox_value read_constant_long() {
  return vm.chunk->constants.values[read_word()];
}
