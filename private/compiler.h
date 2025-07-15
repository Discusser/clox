#pragma once

#include "chunk.h"
#include "scanner.h"
#include "table.h"

typedef struct {
  lox_token name;
  // If this value is -1, it means that the variable has not yet been
  // initialized
  int depth;
  bool is_constant;
} lox_local;

typedef struct {
  lox_local locals[LOX_MAX_LOCAL_COUNT];
  lox_hash_table global_constants;
  int local_count;
  int scope_depth;
} lox_compiler;

// Compiles code from a string, and writes the bytecode to chunk.
bool lox_compiler_compile(const char *source, lox_chunk *chunk);
