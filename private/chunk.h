#pragma once

#include "common.h"
#include "value.h"

typedef enum {
  OP_CONSTANT,
  OP_CONSTANT_LONG,
  OP_RETURN,
} OpCode;

typedef struct {
  int size;
  int capacity;
  int last_line;
  int lines_size;
  int lines_capacity;
  uint8_t *code;
  // This array stores line numbers for each bytecode. To do this efficiently,
  // the data is stored using run-length encoding, since it is common to have
  // multiple bytecodes on one line. Consider the following bytecode:
  // LINE 1 OP_XXX\
  // LINE 1 OP_XXX\
  // LINE 1 OP_XXX\
  // LINE 3 OP_XXX\
  // LINE 4 OP_XXX\
  // LINE 4 OP_XXX\
  // `lines` will contain the following: {3, 0, 0, 2}
  // `lines[n]` is equal to the number of instructions on the line n+1.
  int *lines;
  lox_value_array constants;
} Chunk;

void initialize_chunk(Chunk *chunk);
void free_chunk(Chunk *chunk);
void write_to_chunk(Chunk *chunk, uint8_t byte, int line);
void write_array_to_chunk(Chunk *chunk, uint8_t *bytes, int size, int line);
void write_word_to_chunk(Chunk *chunk, uint32_t word, int line);
int add_constant(Chunk *chunk, lox_value value);

int get_line(Chunk *chunk, int instruction_offset);
void grow_lines(Chunk *chunk);
void grow_lines_to(Chunk *chunk, int min_size);
