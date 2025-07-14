#pragma once

// This header file should only provide functions if NDEBUG is not defined, that
// is if we are in 'debug mode'.
#ifndef NDEBUG

#include "chunk.h"

// Disassembles a chunk. This prints every instruction in the chunk to standard
// output.
void lox_disassemble_chunk(lox_chunk *chunk, const char *name);
// Disassembles an instruction. This prints the instruction to standard output.
int lox_disassemble_instruction(lox_chunk *chunk, int offset);

#endif
