#pragma once

#include "chunk.h"

#define STACK_INITIAL_SIZE 256

typedef struct {
  Chunk *chunk;
  uint8_t *ip;
  lox_value_array stack;
} vm;

typedef enum {
  INTERPRET_OK,
  INTERPRET_COMPILE_ERROR,
  INTERPRET_RUNTIME_ERROR
} interpret_result;

void init_vm();
void free_vm();

void reset_stack();
void push(lox_value value);
lox_value pop();

interpret_result interpret(Chunk *chunk);
interpret_result run();

uint8_t read_byte();
uint32_t read_word();
lox_value read_constant();
lox_value read_constant_long();
