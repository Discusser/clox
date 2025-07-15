#include "debug.h"
#include "chunk.h"
#include "compiler.h"
#include "vm.h"
#include <stdio.h>

// Helper functions for printing specific instructions.
static int constant_instruction(const char *name, lox_chunk *chunk, int offset);
static int constant_long_instruction(const char *name, lox_chunk *chunk,
                                     int offset);
static int global_instruction(const char *name, lox_chunk *chunk, int offset);
static int global_long_instruction(const char *name, lox_chunk *chunk,
                                   int offset);
static int simple_instruction(const char *name, int offset);
static int local_instruction(const char *name, lox_chunk *chunk, int offset);

// This function returns a nil value if NDEBUG is set. Otherwise, it returns the
// name of the global variable with the given index.
static lox_value lox_get_global_name(uint16_t global);
static lox_value lox_get_local_name(uint16_t local);

extern lox_vm vm;
extern lox_compiler *compiler;

void lox_disassemble_chunk(lox_chunk *chunk, const char *name) {
  printf("== Chunk '%s' ==\n", name);

  for (int i = 0; i < chunk->code.size;) {
    i = lox_disassemble_instruction(chunk, i);
  }
}

int lox_disassemble_instruction(lox_chunk *chunk, int offset) {
  printf("LINE %-4d ", lox_chunk_get_offset_line(chunk, offset) + 1);
  printf("%04d ", offset);
  lox_op_code instruction = chunk->code.values[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_CONSTANT_LONG:
    return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
  case OP_NIL:
    return simple_instruction("OP_NIL", offset);
  case OP_TRUE:
    return simple_instruction("OP_TRUE", offset);
  case OP_FALSE:
    return simple_instruction("OP_FALSE", offset);
  case OP_EQ:
    return simple_instruction("OP_EQ", offset);
  case OP_NEQ:
    return simple_instruction("OP_NEQ", offset);
  case OP_GREATER:
    return simple_instruction("OP_GREATER", offset);
  case OP_GREATEREQ:
    return simple_instruction("OP_GREATEREQ", offset);
  case OP_LESS:
    return simple_instruction("OP_LESS", offset);
  case OP_LESSEQ:
    return simple_instruction("OP_LESSEQ", offset);
  case OP_ADD:
    return simple_instruction("OP_ADD", offset);
  case OP_SUBTRACT:
    return simple_instruction("OP_SUBTRACT", offset);
  case OP_MULTIPLY:
    return simple_instruction("OP_MULTIPLY", offset);
  case OP_DIVIDE:
    return simple_instruction("OP_DIVIDE", offset);
  case OP_NEGATE:
    return simple_instruction("OP_NEGATE", offset);
  case OP_NOT:
    return simple_instruction("OP_NOT", offset);
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  case OP_PRINT:
    return simple_instruction("OP_PRINT", offset);
  case OP_POP:
    return simple_instruction("OP_POP", offset);
  case OP_DEFINE_GLOBAL:
    return global_instruction("OP_DEFINE_GLOBAL", chunk, offset);
  case OP_DEFINE_GLOBAL_LONG:
    return global_long_instruction("OP_DEFINE_GLOBAL", chunk, offset);
  case OP_GET_GLOBAL:
    return global_instruction("OP_GET_GLOBAL", chunk, offset);
  case OP_GET_GLOBAL_LONG:
    return global_long_instruction("OP_GET_GLOBAL", chunk, offset);
  case OP_SET_GLOBAL:
    return global_instruction("OP_SET_GLOBAL", chunk, offset);
  case OP_SET_GLOBAL_LONG:
    return global_long_instruction("OP_SET_GLOBAL", chunk, offset);
  case OP_GET_LOCAL:
    return local_instruction("OP_GET_LOCAL", chunk, offset);
  case OP_SET_LOCAL:
    return local_instruction("OP_SET_LOCAL", chunk, offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}

static int constant_instruction(const char *name, lox_chunk *chunk,
                                int offset) {
  uint8_t constant = chunk->code.values[offset + 1];
  printf("%-16s index %4d value '", name, constant);
  lox_print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

static int constant_long_instruction(const char *name, lox_chunk *chunk,
                                     int offset) {
  uint16_t constant = *(uint16_t *)&chunk->code.values[offset + 1];
  printf("%-16s index %4d value '", name, constant);
  lox_print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 3;
}

static int global_instruction(const char *name, lox_chunk *chunk, int offset) {
  uint8_t global = *(uint8_t *)&chunk->code.values[offset + 1];
  printf("%-16s index %4d name  '", name, global);
  lox_print_value(lox_get_global_name(global));
  printf("'\n");
  return offset + 2;
}

static int global_long_instruction(const char *name, lox_chunk *chunk,
                                   int offset) {
  uint16_t global = *(uint16_t *)&chunk->code.values[offset + 1];
  printf("%-16s index %4d name  '", name, global);
  lox_print_value(lox_get_global_name(global));
  printf("'\n");
  return offset + 3;
}

static int simple_instruction(const char *name, int offset) {
  printf("%-16s\n", name);
  return offset + 1;
}

static int local_instruction(const char *name, lox_chunk *chunk, int offset) {
  uint8_t slot = chunk->code.values[offset + 1];
  printf("%-16s index %4d name  '", name, slot);
  lox_print_value(lox_get_local_name(slot));
  printf("'\n");
  return offset + 2;
}

static lox_value lox_get_global_name(uint16_t global) {
#ifndef NDEBUG
  lox_value value;
  if (!lox_hash_table_get(&vm.global_names, lox_value_from_number(global),
                          &value)) {
    fprintf(stderr, "Could not find variable name for index %i\n", global);
  }
  return value;
#else
  return lox_value_from_nil();
#endif
}

static lox_value lox_get_local_name(uint16_t local) {
#ifndef NDEBUG
  lox_value value;
  if (!lox_hash_table_get(&vm.local_names, lox_value_from_number(local),
                          &value)) {
    fprintf(stderr, "Could not find variable name for index %i\n", local);
  }
  return value;
#else
  return lox_value_from_nil();
#endif
}
