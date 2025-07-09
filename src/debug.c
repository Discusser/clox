#include "debug.h"
#include "chunk.h"
#include <stdio.h>

void disassemble_chunk(Chunk *chunk, const char *name) {
  printf("== Chunk '%s' ==\n", name);

  for (int i = 0; i < chunk->size;) {
    i = disassemble_instruction(chunk, i);
  }
}

int disassemble_instruction(Chunk *chunk, int offset) {
  printf("LINE %-4d ", get_line(chunk, offset) + 1);
  printf("%04d ", offset);
  uint8_t instruction = chunk->code[offset];
  switch (instruction) {
  case OP_CONSTANT:
    return constant_instruction("OP_CONSTANT", chunk, offset);
  case OP_CONSTANT_LONG:
    return constant_long_instruction("OP_CONSTANT_LONG", chunk, offset);
  case OP_RETURN:
    return simple_instruction("OP_RETURN", offset);
  default:
    printf("Unknown opcode %d\n", instruction);
    return offset + 1;
  }
}

int constant_instruction(const char *name, Chunk *chunk, int offset) {
  uint8_t constant = chunk->code[offset + 1];
  printf("%-16s index %4d value '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 2;
}

int constant_long_instruction(const char *name, Chunk *chunk, int offset) {
  uint16_t constant = *(uint16_t *)&chunk->code[offset + 1];
  printf("%-16s index %4d value '", name, constant);
  print_value(chunk->constants.values[constant]);
  printf("'\n");
  return offset + 5;
}

int simple_instruction(const char *name, int offset) {
  printf("%-16s\n", name);
  return offset + 1;
}
