#pragma once

#include "chunk.h"

// Compiles code from a string, and writes the bytecode to chunk.
bool lox_compiler_compile(const char *source, lox_chunk *chunk);
