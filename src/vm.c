#include "vm.h"
#include "memory.h"
#include "object.h"
#include "value.h"
#include "chunk.h"
#include "compiler.h"
#include "debug.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_objects();

static void runtime_error(const char *format, ...);
static void reset_stack();
static void push(lox_value value);
static lox_value pop();
static lox_value *peek(int n);

static uint8_t read_byte();
static uint32_t read_word();
static lox_value read_constant();
static lox_value read_constant_long();

lox_vm vm;

void init_vm() {
  value_array_initialize(&vm.stack);
  value_array_grow_to(&vm.stack, LOX_STACK_INITIAL_SIZE, 0);
  lox_hash_table_init(&vm.strings);
  vm.objects = NULL;
  reset_stack();
}

void free_vm() {
  value_array_free(&vm.stack);
  lox_hash_table_free(&vm.strings);
  free_objects();
}

static void free_objects() {
  lox_object *curr = vm.objects;
  while (curr->next != NULL) {
    lox_object *tmp = curr->next;
    lox_object_free(curr);
    curr = curr->next;
  }
}

static void runtime_error(const char *format, ...) {
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);

  size_t instruction = vm.ip - vm.chunk->code.values - 1;
  int line = lox_chunk_get_offset_line(vm.chunk, instruction) + 1;
  fprintf(stderr, "[line %d] in script\n", line);
  reset_stack();
}

static void reset_stack() { vm.stack.size = 0; }

static void push(lox_value value) { value_array_push(&vm.stack, value); }

static lox_value pop() { return value_array_pop(&vm.stack); }

static lox_value *peek(int n) {
  return &vm.stack.values[vm.stack.size - 1 - n];
}

bool lox_is_falsey(lox_value value) {
  return value.type == VAL_NIL ||
         (value.type == VAL_BOOL && value.as.boolean == false);
}

bool lox_values_equal(lox_value lhs, lox_value rhs) {
  if (lhs.type != rhs.type)
    return false;
  switch (lhs.type) {
  case VAL_BOOL:
    return lhs.as.boolean == rhs.as.boolean;
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return lhs.as.number == rhs.as.number;
  case VAL_OBJECT: {
    return lhs.as.object == rhs.as.object;
  }
  default:
    return false;
  }
}

interpret_result interpret(const char *source) {
  lox_chunk chunk;
  lox_chunk_initialize(&chunk);

  if (!lox_compiler_compile(source, &chunk)) {
    lox_chunk_free(&chunk);
    return INTERPRET_COMPILE_ERROR;
  }

  vm.chunk = &chunk;
  vm.ip = vm.chunk->code.values;

  interpret_result result = run();
  lox_chunk_free(&chunk);

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
      lox_print_value(*elem);
      printf(" ]");
    }
    printf("\n");
    lox_disassemble_instruction(vm.chunk, vm.ip - vm.chunk->code.values);
#endif

    uint8_t instruction;
    switch (instruction = read_byte()) {
    case OP_RETURN:
      lox_print_value(pop());
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
      push(lox_value_from_nil());
      break;
    case OP_TRUE:
      push(lox_value_from_bool(true));
      break;
    case OP_FALSE:
      push(lox_value_from_bool(false));
      break;
    case OP_EQ: {
      lox_value rhs = pop();
      lox_value lhs = pop();
      push(lox_value_from_bool(lox_values_equal(rhs, lhs)));
      break;
    }
    case OP_NEQ: {
      lox_value rhs = pop();
      lox_value lhs = pop();
      push(lox_value_from_bool(!lox_values_equal(rhs, lhs)));
      break;
    }
    case OP_GREATER:
      BINARY_OP(lox_value_from_bool, >);
      break;
    case OP_GREATEREQ:
      BINARY_OP(lox_value_from_bool, >=);
      break;
    case OP_LESS:
      BINARY_OP(lox_value_from_bool, <);
      break;
    case OP_LESSEQ:
      BINARY_OP(lox_value_from_bool, <=);
      break;
    case OP_NEGATE: {
      lox_value value = pop();
      if (value.type == VAL_NUMBER) {
        push(lox_value_from_number(-value.as.number));
      } else {
        runtime_error("Operand must be a number.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_NOT: {
      lox_value value = pop();
      bool f = lox_is_falsey(value);
      push(lox_value_from_bool(f));
      break;
    }
    case OP_ADD: {
      lox_value *rhs = peek(0);
      lox_value *lhs = peek(1);
      if (lox_value_is_string(*lhs) && lox_value_is_string(*rhs)) {
        lox_object_string *rstr = (lox_object_string *)(pop().as.object);
        lox_object_string *lstr = (lox_object_string *)(pop().as.object);
        int length = lstr->length + rstr->length;
        char *chars = ALLOC_ARRAY(char, length + 1);
        memcpy(chars, lstr->chars, lstr->length);
        memcpy(chars + lstr->length, rstr->chars, rstr->length);
        chars[length] = '\0';
        push(lox_value_from_object(
            (lox_object *)lox_object_string_new_consume(chars, length, false)));
      } else if (lhs->type == VAL_NUMBER && rhs->type == VAL_NUMBER) {
        pop();
        pop();
        push(lox_value_from_number(lhs->as.number + rhs->as.number));
      } else {
        runtime_error("Operands must be numbers.");
        return INTERPRET_RUNTIME_ERROR;
      }
      break;
    }
    case OP_SUBTRACT:
      BINARY_OP(lox_value_from_number, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(lox_value_from_number, *);
      break;
    case OP_DIVIDE:
      BINARY_OP(lox_value_from_number, /);
      break;
    default:
      printf("Unknown instruction %i\n", instruction);
      return INTERPRET_RUNTIME_ERROR;
    }
  }

#undef BINARY_OP
}

static uint8_t read_byte() { return *vm.ip++; }

static uint32_t read_word() {
  uint32_t ret = *(uint32_t *)vm.ip;
  vm.ip += sizeof(uint32_t);
  return ret;
}

static lox_value read_constant() {
  return vm.chunk->constants.values[read_byte()];
}

static lox_value read_constant_long() {
  return vm.chunk->constants.values[read_word()];
}
