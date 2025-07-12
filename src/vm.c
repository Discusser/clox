#include "vm.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include <stdarg.h>
#include <stdio.h>

lox_vm vm;

void init_vm() {
  value_array_initialize(&vm.stack);
  reset_stack();
}

void free_vm() { value_array_free(&vm.stack); }

void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);

  size_t instruction = vm.ip - vm.chunk->code.values - 1;
  int line = get_line(vm.chunk, instruction) + 1;
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

void reset_stack() { vm.stack.size = 0; }

void push(lox_value value) { value_array_push(&vm.stack, value); }

lox_value pop() { return value_array_pop(&vm.stack); }

bool is_falsey(lox_value value) {
  return value.type == VAL_NIL ||
         (value.type == VAL_BOOL && value.as.boolean == false);
}

bool values_equal(lox_value lhs, lox_value rhs) {
  if (lhs.type != rhs.type)
    return false;
  switch (lhs.type) {
  case VAL_BOOL:
    return lhs.as.boolean == rhs.as.boolean;
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return lhs.as.number == rhs.as.number;
  default:
    return false;
  }
}

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
#define BINARY_OP(make_value, op)                                              \
  do {                                                                         \
    lox_value rhs = pop();                                                     \
    lox_value *lhs = &vm.stack.values[vm.stack.size - 1];                      \
    if (lhs->type == VAL_NUMBER && rhs.type == VAL_NUMBER) {                   \
      *lhs = make_value(lhs->as.number op rhs.as.number);                      \
    } else {                                                                   \
      runtime_error("Operands must be numbers.");                              \
      return INTERPRET_RUNTIME_ERROR;                                          \
    }                                                                          \
  } while (false)

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
    case OP_NIL:
      push(nil_value());
      break;
    case OP_TRUE:
      push(bool_value(true));
      break;
    case OP_FALSE:
      push(bool_value(false));
      break;
    case OP_EQ: {
      lox_value rhs = pop();
      lox_value *value = &vm.stack.values[vm.stack.size - 1];
      *value = bool_value(values_equal(rhs, *value));
      break;
    }
    case OP_NEQ: {
      lox_value rhs = pop();
      lox_value *value = &vm.stack.values[vm.stack.size - 1];
      *value = bool_value(!values_equal(rhs, *value));
      break;
    }
    case OP_GREATER:
      BINARY_OP(bool_value, >);
      break;
    case OP_GREATEREQ:
      BINARY_OP(bool_value, >=);
      break;
    case OP_LESS:
      BINARY_OP(bool_value, <);
      break;
    case OP_LESSEQ:
      BINARY_OP(bool_value, <=);
      break;
    case OP_NEGATE: {
      lox_value value = vm.stack.values[vm.stack.size - 1];
      if (value.type == VAL_NUMBER) {
        value.as.number *= -1;
      } else {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_NOT: {
      lox_value *value = &vm.stack.values[vm.stack.size - 1];
      bool f = is_falsey(*value);
      *value = bool_value(f);
      break;
    }
    case OP_ADD:
      BINARY_OP(number_value, +);
      break;
    case OP_SUBTRACT:
      BINARY_OP(number_value, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(number_value, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(number_value, /);
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
