#pragma once

#include "chunk.h"
#include "table.h"

typedef struct {
  lox_chunk *chunk;
  uint8_t *ip;
  lox_value_array stack;
  // Every string created in lox is interned into this hash table. If the
  // strings table contains a key, it means that the given key is a string that
  // is currently interned.
  lox_hash_table strings;
  lox_hash_table global_indices;
  lox_value_array globals;
#ifndef NDEBUG
  // Reverse lookup table for getting global names from their index.
  lox_hash_table global_names;
#endif
  lox_object *objects;
} lox_vm;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} interpret_result;

void init_vm();
void free_vm();

interpret_result interpret(const char *source);
interpret_result run();

bool lox_is_falsey(lox_value value);
bool lox_values_equal(lox_value lhs, lox_value rhs);
