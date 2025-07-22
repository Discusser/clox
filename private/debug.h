#pragma once

#include "chunk.h"

// Disassembles a chunk. This prints every instruction in the chunk to standard
// output.
void lox_disassemble_chunk(lox_chunk *chunk, const char *name);
// Disassembles an instruction. This prints the instruction to standard output.
int lox_disassemble_instruction(lox_chunk *chunk, int offset);

lox_value lox_get_global_name(uint16_t global);
lox_value lox_get_local_name(uint16_t local);
