#pragma once

#include "chunk.h"
#include "scanner.h"
#include "table.h"

typedef struct {
  // This tokens points to the name of the local variable in the source code
  lox_token name;
  // If this value is -1, it means that the variable has not yet been
  // initialized. Otherwise, this value is used by the compiler to check if the
  // variable is accessible in a given scope (lexical scoping).
  int depth;
  // If true, it means that this variable was created using `const`, and thus
  // cannot be modified.
  bool is_constant;
} lox_local;

DECLARE_LOX_ARRAY(lox_local, local_array);

typedef struct {
  lox_local_array locals;
  // Hash table that associates a global variable index with a value. If an
  // entry for an index exists, it means that the global variable associated
  // with the index has been created with `const`.
  lox_hash_table global_constants;
  // Arrays of break jumps that we have to patch.
  lox_int_array breaks;
  int continue_jump_to;
  int scope_depth;
  bool in_breakable;
  bool in_continueable;
} lox_compiler;

// Compiles code from a string, and writes the bytecode to chunk.
bool lox_compiler_compile(const char *source, lox_chunk *chunk);
