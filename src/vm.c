#include "vm.h"
#include "debug.h"
#include <stdio.h>

vm virtual_machine;

void init_vm() {
  value_array_initialize(&virtual_machine.stack);
  reset_stack();
}

void free_vm() { value_array_free(&virtual_machine.stack); }

void reset_stack() { virtual_machine.stack.size = 0; }

void push(lox_value value) { value_array_push(&virtual_machine.stack, value); }

lox_value pop() { return value_array_pop(&virtual_machine.stack); }

interpret_result interpret(Chunk *chunk) {
  virtual_machine.chunk = chunk;
  virtual_machine.ip = virtual_machine.chunk->code.values;
  return run();
}

interpret_result run() {
#define BINARY_OP(op)                                                          \
  do {                                                                         \
    double rhs = pop();                                                        \
    double lhs = pop();                                                        \
    push(lhs op rhs);                                                          \
  } while (0)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("%-10s", "STACK");
    for (lox_value *elem = virtual_machine.stack.values;
         elem < virtual_machine.stack.values + virtual_machine.stack.size;
         elem++) {
      printf("[ ");
      print_value(*elem);
      printf(" ]");
    }
    printf("\n");
    disassemble_instruction(virtual_machine.chunk,
                            virtual_machine.ip -
                                virtual_machine.chunk->code.values);
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
      virtual_machine.stack.values[virtual_machine.stack.size - 1] *= -1;
      break;
    default:
      printf("Unknown instruction %i\n", instruction);
      return INTERPRET_RUNTIME_ERROR;
    }
  }

#undef BINARY_OP
}

uint8_t read_byte() { return *virtual_machine.ip++; }

uint32_t read_word() {
  uint32_t ret = *(uint32_t *)virtual_machine.ip;
  virtual_machine.ip += sizeof(uint32_t);
  return ret;
}

lox_value read_constant() {
  return virtual_machine.chunk->constants.values[read_byte()];
}

lox_value read_constant_long() {
  return virtual_machine.chunk->constants.values[read_word()];
}
