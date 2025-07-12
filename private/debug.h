#pragma once

#ifndef NDEBUG

#include "chunk.h"

// Disassembles a chunk. This prints every instruction in the chunk to standard
// output.
void disassemble_chunk(lox_chunk *chunk, const char *name);
// Disassembles an instruction. This prints the instruction to standard output.
int disassemble_instruction(lox_chunk *chunk, int offset);

// Helper functions for printing specific instructions.
int constant_instruction(const char *name, lox_chunk *chunk, int offset);
int constant_long_instruction(const char *name, lox_chunk *chunk, int offset);
int simple_instruction(const char *name, int offset);

#endif
