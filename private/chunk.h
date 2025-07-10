#pragma once

#include "common.h"
#include "value.h"

// This enum contains all the opcodes for our virtual machine.
typedef enum {
  // Loads a constant onto the stack. Allows up to 2^8-1 different constants.
  // Parameters: index (1 byte)
  OP_CONSTANT,
  // Loads a constant onto the stack. Allows up to 2^32-1 different constants.
  // Parameters: index (4 bytes)
  OP_CONSTANT_LONG,
  // Negates the value at the top of the stack. Parameters: none
  OP_NEGATE,
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  // Returns. Parameters: none
  OP_RETURN,
} op_code;

typedef struct {
  // The line number of the previous byte that was written to the chunk. This is
  // used to store the lines using run-length encoding in `lines`.
  int last_line;
  // This array contains the actual bytecode in the chunk. Each element is
  // either an opcode, or one of its parameters.
  lox_byte_array code;
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
  lox_int_array lines;
  lox_value_array constants;
} Chunk;

// Initializes all the fields of a chunk.
void initialize_chunk(Chunk *chunk);
// Frees a chunk from memory.
void free_chunk(Chunk *chunk);
// Adds a byte at a given line to the chunk. `line` is expected to be greater
// than `chunk->last_line`, or else errors may occur.
void write_to_chunk(Chunk *chunk, uint8_t byte, int line);
// Adds an array of bytes at a given line to the chunk. See also
// `write_to_chunk`
void write_array_to_chunk(Chunk *chunk, uint8_t *bytes, int size, int line);
// Adds an integer at a given line to the chunk. The integer is converted to a
// `uint8_t *` and written via `write_array_to_chunk`
void write_word_to_chunk(Chunk *chunk, uint32_t word, int line);
// Adds a constant to the chunk. The constant's index is returned, which
// corresponds to its index in `chunk->constants`
int add_constant(Chunk *chunk, lox_value value);

// Returns the line number of an instruction at the given offset.
int get_line(Chunk *chunk, int instruction_offset);
