#include "chunk.h"
#include "memory.h"
#include <string.h>

void initialize_chunk(Chunk *chunk) {
  chunk->size = 0;
  chunk->capacity = 0;
  chunk->last_line = -1;
  chunk->lines_size = 0;
  chunk->lines_capacity = 0;
  chunk->code = NULL;
  chunk->lines = NULL;
  initialize_value_array(&chunk->constants);
}

void free_chunk(Chunk *chunk) {
  FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
  FREE_ARRAY(int, chunk->lines, chunk->lines_capacity);
  free_value_array(&chunk->constants);
  initialize_chunk(chunk);
}

void write_to_chunk(Chunk *chunk, uint8_t byte, int line) {
  write_array_to_chunk(chunk, &byte, 1, line);
}

void write_array_to_chunk(Chunk *chunk, uint8_t *bytes, int size, int line) {
  if (chunk->size + size > chunk->capacity) {
    int previous_capacity = chunk->capacity;
    chunk->capacity = GROW_CAPACITY(chunk->capacity);
    if (chunk->size + size > chunk->capacity)
      chunk->capacity = chunk->size + size;
    chunk->code =
        GROW_ARRAY(uint8_t, chunk->code, previous_capacity, chunk->capacity);
  }

  for (int i = 0; i < size; i++) {
    chunk->code[chunk->size++] = bytes[i];
  }

  if (chunk->last_line == line) {
    // Technically this shouldn't be possible, because if chunk->last_line is a
    // valid line number, then the lines array shouldn't be empty, but we never
    // know.
    if (chunk->lines_size == 0) {
      grow_lines_to(chunk, line);
      chunk->lines[line - 1] = 0;
    }
  } else if (chunk->lines_size >= chunk->lines_capacity) {
    grow_lines_to(chunk, line);
    chunk->lines[line - 1] = 0;
  }

  chunk->lines[line - 1] += size;

  if (line > chunk->lines_size) {
    chunk->lines_size = line;
  }
  chunk->last_line = line;
}

void write_word_to_chunk(Chunk *chunk, uint32_t word, int line) {
  write_array_to_chunk(chunk, (uint8_t *)&word, 4, line);
}

int add_constant(Chunk *chunk, lox_value value) {
  write_to_value_array(&chunk->constants, value);
  return chunk->constants.size - 1;
}

void grow_lines(Chunk *chunk) { grow_lines_to(chunk, chunk->lines_size + 1); }

void grow_lines_to(Chunk *chunk, int min_size) {
  int previous_capacity = chunk->lines_capacity;
  chunk->lines_capacity = GROW_CAPACITY(chunk->lines_capacity);
  if (min_size > chunk->lines_capacity)
    chunk->lines_capacity = min_size;
  chunk->lines =
      GROW_ARRAY(int, chunk->lines, previous_capacity, chunk->lines_capacity);
  memset(&chunk->lines[previous_capacity], '\0',
         (chunk->lines_capacity - previous_capacity) * sizeof(int));
}

int get_line(Chunk *chunk, int instruction_offset) {
  if (instruction_offset < 0)
    return -1;

  int offset = 0;
  for (int line = 0; line < chunk->lines_size; line++) {
    if (offset > instruction_offset)
      return line - 1;
    offset += chunk->lines[line];
  }

  if (offset >= instruction_offset)
    return chunk->lines_size - 1;
  return -1;
}
