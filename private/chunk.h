#pragma once

#include "value.h"

// This enum contains all the opcodes for our virtual machine.
typedef enum {
  // Dummy opcode for invalid instructions.
  OP_INVALID,
  // Loads a constant onto the stack. Allows up to 2^8-1 different constants.
  // Parameters: index (1 byte)
  OP_CONSTANT,
  // Loads a constant onto the stack. Allows up to 2^16-1 different constants.
  // Parameters: index (2 bytes)
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
  OP_MODULO,
  // Prints the value at the top of the stack. Parameters: none
  OP_PRINT,
  // Pops the value off the top of the stack. This is mainly used by expression
  // statements, since their result is discarded. Parameters: none
  OP_POP,
  // Pops n values off the top of the stack. Parameters: count (2 bytes)
  OP_POPN,
  // Defines a new global variable with the value on the top of the stack,
  // allowing up to 2^8-1 globals. Parameters: index (1 byte)
  OP_DEFINE_GLOBAL,
  // Defines a new global variable with the value on the top of the stack,
  // allowing up to 2^16-1 globals. Parameters: index (2 bytes)
  OP_DEFINE_GLOBAL_LONG,
  // Pushes the value of the global with the given index onto the stack,
  // allowing up to 2^8-1 globals. Parameters: index (1 byte)
  OP_GET_GLOBAL,
  // Pushes the value of the global with the given index onto the stack,
  // allowing up to 2^16-1 globals. Parameters: index (2 bytes)
  OP_GET_GLOBAL_LONG,
  // Sets the value of the global with the given index to the value on top of
  // the stack, allowing up to 2^8-1 globals. Parameters: index (1 byte)
  OP_SET_GLOBAL,
  // Sets the value of the global with the given index to the value on top of
  // the stack, allowing up to 2^16-1 globals. Parameters: index (2 bytes)
  OP_SET_GLOBAL_LONG,
  // Pushes the local variable with the given index to the top of the stack.
  // Parameters: index (1 byte)
  OP_GET_LOCAL,
  // Sets the value of the local variable with the given index to the value at
  // the top of the stack. Parameters: index (1 byte)
  OP_SET_LOCAL,
  // Retrieves the value of the upvalue with the given index in the current
  // closure, and pushes it to the stack. Parameters: index (1 byte)
  OP_GET_UPVALUE,
  // Sets the value of the upvalue with the given index in the current closure.
  // This doesn't modify the stack. Parameters: index (1 byte)
  OP_SET_UPVALUE,
  // Closes the upvalue at the top of the stack, moving it to the heap. This
  // appears at the end of a closure's scopes, in order to make all the captured
  // variables persist as long as necessary.
  OP_CLOSE_UPVALUE,
  // Offsets the instruction pointer by the given amount, essentially performing
  // a jump, if the value on top of the stack is truthy. Parameters: offset (2
  // bytes)
  OP_JMP_TRUE,
  // Offsets the instruction pointer by the given amount, essentially performing
  // a jump, if the value on top of the stack is falsey. Parameters: offset (2
  // bytes)
  OP_JMP_FALSE,
  // Jumps unconditionally by a given offset. Parameters: offset (2 bytes)
  OP_JMP,
  // Jumps backwards unconditionally by a given offset. Parameters: offset (2
  // bytes)
  OP_JMP_BACK,
  // Pushes the value that is on top of the stack to the stack. The values are
  // identical if they are objects. If they are primitive types, changes on one
  // value will not necessarily reflect changes on the other. Parameters: none
  OP_DUP,
  // Calls the function on top of the stack. The stack, should contain the
  // function's parameters in reverse order, whose amount is specified by the
  // count parameter, and finally the function variable, stored as a
  // lox_object_function. This opcode doesn't actually pop any values from the
  // stack. Instead, the called function acts on the values already present on
  // the stack. Parameters: arg_count (1 byte)
  OP_CALL,
  // Creates a closure for a function. The first parameter is the function that
  // should be wrapped by the closure. Then each captured value : first a byte
  // to indicate whether it is a local or not, and then a short that holds the
  // index of the variable. The created closure is pushed onto the stack.
  // The (is_local, variable_index) part of the parameters is repeated
  // function->upvalue_count times. Parameters: function (2 bytes), (is_local (1
  // byte), variable_index (2 bytes)) (variable length)
  OP_CLOSURE,
  // Creates a new class with the name held in the constants table with the
  // given index. The class is pushed onto the stack. Parameters: index (1 byte)
  OP_CLASS,
  // Sets a property value on an instance. On top of the stack is the new value
  // of the property, and right under is the instance that shall be modified.
  // The index in the constant table to the property's name is passed as a
  // parameter. This pops the value and the instance off the stack, and pushes
  // the value on the stack, effectively shrinking the stack by one and
  // replacing the slot containing the instance with the value. Parameters:
  // index (1 byte).
  OP_SET_PROPERTY,
  // Gets a property value from the instance on top of the stack. The name of
  // the property is pointed to by a constant with a given index. This pops the
  // instance off the stack, and pushes the value onto the stack. Parameters:
  // index (1 byte)
  OP_GET_PROPERTY,
  OP_METHOD,
  OP_INVOKE,
  OP_INHERIT,
  OP_GET_SUPER,
  OP_SUPER_INVOKE,
  // Pops the value on top of the stack, pops the current frame, and pushes the
  // initial popped value (the return value) onto the stack. This pops the
  // current frame and goes to the previous frame. Parameters: none
  OP_RETURN,
} lox_op_code;

typedef struct lox_chunk {
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
// Adds a constant to the chunk. The constant's index is returned, which
// corresponds to its index in `chunk->constants`
int lox_chunk_add_constant(lox_chunk *chunk, lox_value value);

// Returns the 0-indexed line number of an instruction at the given offset.
int lox_chunk_get_offset_line(lox_chunk *chunk, int instruction_offset);
