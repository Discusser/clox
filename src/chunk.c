#include "chunk.h"
#include "memory.h"
#include <string.h>

void lox_chunk_initialize(lox_chunk *chunk) {
  chunk->last_line = -1;
  byte_array_initialize(&chunk->code);
  int_array_initialize(&chunk->lines);
  value_array_initialize(&chunk->constants);
}

void lox_chunk_free(lox_chunk *chunk) {
  byte_array_free(&chunk->code);
  int_array_free(&chunk->lines);
  value_array_free(&chunk->constants);
  lox_chunk_initialize(chunk);
}

void lox_chunk_write(lox_chunk *chunk, uint8_t byte, int line) {
  lox_chunk_write_array(chunk, &byte, 1, line);
}

void lox_chunk_write_array(lox_chunk *chunk, uint8_t *bytes, int size,
                           int line) {
  for (int i = 0; i < size; i++) {
    byte_array_push(&chunk->code, bytes[i]);
  }

  if (chunk->last_line == line) {
    // Technically this shouldn't be possible, because if chunk->last_line is a
    // valid line number, then the lines array shouldn't be empty, but we never
    // know.
    if (chunk->lines.size == 0) {
      int_array_grow_to(&chunk->lines, line, true);
      chunk->lines.values[line - 1] = 0;
    }
  } else if (chunk->lines.size >= chunk->lines.capacity) {
    int_array_grow_to(&chunk->lines, line, true);
    chunk->lines.values[line - 1] = 0;
  }

  chunk->lines.values[line - 1] += size;

  if (line > chunk->lines.size) {
    chunk->lines.size = line;
  }
  chunk->last_line = line;
}

int lox_chunk_add_constant(lox_chunk *chunk, lox_value value) {
  value_array_push(&chunk->constants, value);
  return chunk->constants.size - 1;
}

int lox_chunk_get_offset_line(lox_chunk *chunk, int instruction_offset) {
  if (instruction_offset < 0)
    return -1;

  int offset = 0;
  for (int line = 0; line < chunk->lines.size; line++) {
    if (offset > instruction_offset)
      return line - 1;
    offset += chunk->lines.values[line];
  }

  if (offset >= instruction_offset)
    return chunk->lines.size - 1;
  return -1;
}
