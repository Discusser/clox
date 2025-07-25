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
  bool is_captured;
} lox_local;

typedef struct {
  uint16_t index;
  bool is_local;
} lox_up_value;

typedef enum {
  TYPE_FUNCTION,
  TYPE_METHOD,
  TYPE_INITIALIZER,
  TYPE_SCRIPT
} lox_function_type;

DECLARE_LOX_ARRAY(lox_local, local_array);

typedef struct lox_compiler lox_compiler;
typedef struct lox_class_compiler lox_class_compiler;

typedef struct lox_compiler {
  lox_up_value upvalues[256];
  lox_compiler *enclosing;
  lox_object_function *function;
  lox_function_type function_type;
  lox_local_array locals;
  // Hash table that associates a global variable index with a value. If an
  // entry for an index exists, it means that the global variable associated
  // with the index has been created with `const`.
  lox_hash_table global_constants;
  // Arrays of break jumps that we have to patch.
  lox_int_array breaks;
  lox_int_array continues;
  int scope_depth;
  int continue_depth;
  int break_depth;
} lox_compiler;

typedef struct lox_class_compiler {
  lox_class_compiler *enclosing;
  bool hasSuperclass;
} lox_class_compiler;

// Compiles code from a string, and writes the bytecode to chunk.
lox_object_function *lox_compiler_compile(const char *source);

void lox_compiler_mark_roots();
