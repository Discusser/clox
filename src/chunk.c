#include "chunk.h"
#include "memory.h"
#include <string.h>

void initialize_chunk(Chunk *chunk) {
  chunk->last_line = -1;
  byte_array_initialize(&chunk->code);
  int_array_initialize(&chunk->lines);
  value_array_initialize(&chunk->constants);
}

void free_chunk(Chunk *chunk) {
  byte_array_free(&chunk->code);
  int_array_free(&chunk->lines);
  value_array_free(&chunk->constants);
  initialize_chunk(chunk);
}

void write_to_chunk(Chunk *chunk, uint8_t byte, int line) {
  write_array_to_chunk(chunk, &byte, 1, line);
}

void write_array_to_chunk(Chunk *chunk, uint8_t *bytes, int size, int line) {
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

void write_word_to_chunk(Chunk *chunk, uint32_t word, int line) {
  write_array_to_chunk(chunk, (uint8_t *)&word, 4, line);
}

int add_constant(Chunk *chunk, lox_value value) {
  value_array_push(&chunk->constants, value);
  return chunk->constants.size - 1;
}

int get_line(Chunk *chunk, int instruction_offset) {
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
