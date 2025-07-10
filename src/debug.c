#include "debug.h"
#include "chunk.h"
#include <stdio.h>

void disassemble_chunk(lox_chunk *chunk, const char *name) {
  printf("== Chunk '%s' ==\n", name);

  for (int i = 0; i < chunk->code.size;) {
    i = disassemble_instruction(chunk, i);
  }
}

int disassemble_instruction(lox_chunk *chunk, int offset) {
  printf("LINE %-4d ", get_line(chunk, offset) + 1);
  printf("%04d ", offset);
  uint8_t instruction = chunk->code.values[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_CONSTANT_LONG:
    return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
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
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}

int constant_instruction(const char *name, lox_chunk *chunk, int offset) {
  uint8_t constant = chunk->code.values[offset + 1];
  printf("%-16s index %4d value '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

int constant_long_instruction(const char *name, lox_chunk *chunk, int offset) {
  uint16_t constant = *(uint16_t *)&chunk->code.values[offset + 1];
  printf("%-16s index %4d value '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 5;
}

int simple_instruction(const char *name, int offset) {
  printf("%-16s\n", name);
  return offset + 1;
}
