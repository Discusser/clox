#pragma once

#include "common.h"
#include "value.h"

// This enum contains all the opcodes for our virtual machine.
typedef enum {
  // Dummy opcode for invalid instructions.
  OP_INVALID,
  // Loads a constant onto the stack. Allows up to 2^8-1 different constants.
  // Parameters: index (1 byte)
  OP_CONSTANT,
  // Loads a constant onto the stack. Allows up to 2^32-1 different constants.
  // Parameters: index (4 bytes)
  OP_CONSTANT_LONG,
  // Pushes the literal 'nil' onto the stack. Parameters: none
  OP_NIL,
  // Pushes the literal 'true' onto the stack. Parameters: none
  OP_TRUE,
  // Pushes the literal 'false' onto the stack. Parameters: none
  OP_FALSE,
  // Checks if the two values on top of the stack are equal. Pops the operands
  // and pushes true or false according to the result. Parameters: none
  OP_EQ,
  // Checks if the two values on top of the stack are not equal. Pops the
  // operands
  // and pushes true or false according to the result. Parameters: none
  OP_NEQ,
  // Checks if the 2nd element of the stack is greater than the 1st element (the
  // top). Pops the operands and pushes true or false according to the result.
  // Parameters: none
  OP_GREATER,
  // Checks if the 2nd element of the stack is greater than or equal to the 1st
  // element (the
  // top). Pops the operands and pushes true or false according to the result.
  // Parameters: none
  OP_GREATEREQ,
  // Checks if the 2nd element of the stack is less than the 1st element (the
  // top). Pops the operands and pushes true or false according to the result.
  // Parameters: none
  OP_LESS,
  // Checks if the 2nd element of the stack is less than or equal to the 1st
  // element (the top). Pops the operands and pushes true or false according to
  // the result. Parameters: none
  OP_LESSEQ,
  // Negates the number at the top of the stack. Parameters: none
  OP_NEGATE,
  // Negates the value at the top of the stack. Parameters: none
  OP_NOT,
  // Binary operators that act on the two variables on top of the stack.
  // Parameters: none
  OP_ADD,
  OP_SUBTRACT,
  OP_MULTIPLY,
  OP_DIVIDE,
  // Prints the value at the top of the stack. Parameters: none
  OP_PRINT,
  // Pops the value off the top of the stack. This is mainly used by expression
  // statements, since their result is discarded. Parameters: none
  OP_POP,
  // Defines a new global variable with the value on the top of the stack,
  // allowing up to 2^8-1 globals. Parameters: index (1 byte)
  OP_DEFINE_GLOBAL,
  // Defines a new global variable with the value on the top of the stack,
  // allowing up to 2^32-1 globals. Parameters: index (4 bytes)
  OP_DEFINE_GLOBAL_LONG,
  // Pushes the value of the global with the given index onto the stack,
  // allowing up to 2^8-1 globals. Parameters: index (1 byte)
  OP_GET_GLOBAL,
  // Pushes the value of the global with the given index onto the stack,
  // allowing up to 2^32-1 globals. Parameters: index (4 bytes)
  OP_GET_GLOBAL_LONG,
  // Sets the value of the global with the given index to the value on top of
  // the stack, allowing up to 2^8-1 globals. Parameters: index (1 byte)
  OP_SET_GLOBAL,
  // Sets the value of the global with the given index to the value on top of
  // the stack, allowing up to 2^32-1 globals. Parameters: index (4 bytes)
  OP_SET_GLOBAL_LONG,
  // Returns. Parameters: none
  OP_RETURN,
} lox_op_code;

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
} lox_chunk;

// Initializes all the fields of a chunk.
void lox_chunk_initialize(lox_chunk *chunk);
// Frees a chunk from memory.
void lox_chunk_free(lox_chunk *chunk);
// Adds a byte at a given line to the chunk. `line` is expected to be greater
// than `chunk->last_line`, or else errors may occur.
void lox_chunk_write(lox_chunk *chunk, uint8_t byte, int line);
// Adds an array of bytes at a given line to the chunk. See also
// `write_to_chunk`
void lox_chunk_write_array(lox_chunk *chunk, uint8_t *bytes, int size,
                           int line);
// Adds an integer at a given line to the chunk. The integer is converted to a
// `uint8_t *` and written via `write_array_to_chunk`
void lox_chunk_write_word(lox_chunk *chunk, uint32_t word, int line);
// Adds a constant to the chunk. The constant's index is returned, which
// corresponds to its index in `chunk->constants`
int lox_chunk_add_constant(lox_chunk *chunk, lox_value value);

// Returns the 0-indexed line number of an instruction at the given offset.
int lox_chunk_get_offset_line(lox_chunk *chunk, int instruction_offset);
