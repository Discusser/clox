#pragma once

#include "common.h"
#include "table.h"
#include <stdio.h>

typedef struct {
  lox_object_closure *closure;
  uint8_t *ip;
  int slots_offset;
} lox_call_frame;

typedef struct {
  lox_call_frame frames[LOX_MAX_CALL_FRAMES];
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
  lox_hash_table local_names;
#endif
  lox_object *objects;
  lox_object_upvalue *open_upvalues;
  lox_object **gray_stack;
  int gray_capacity;
  int gray_size;
  ssize_t bytes_allocated;
  ssize_t next_gc;
  int frame_count;
  bool mark_value;
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

void runtime_error(const char *format, ...);

bool lox_is_falsey(lox_value value);
bool lox_values_equal(lox_value lhs, lox_value rhs);

void define_native(lox_vm *vm, const char *name, lox_native_function function,
                   int arity);

void push(lox_value value);
lox_value pop();
