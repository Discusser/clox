#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

struct lox_settings {
  int array_minimum_capacity;
  int array_scale_factor;
  int gc_heap_grow_factor;
  float hash_table_load_factor;
  int initial_stack_size;
  int max_local_count;
};

extern struct lox_settings lox_settings;

#define LOX_ARRAY_MIN_CAPACITY lox_settings.array_minimum_capacity
#define LOX_ARRAY_SCALE_FACTOR lox_settings.array_scale_factor
#define LOX_GC_HEAP_GROW_FACTOR lox_settings.gc_heap_grow_factor
#define LOX_HASH_TABLE_LOAD_FACTOR lox_settings.hash_table_load_factor
#define LOX_INITIAL_STACK_SIZE lox_settings.initial_stack_size
#define LOX_MAX_CALL_FRAMES 64
#define LOX_MAX_LOCAL_COUNT lox_settings.max_local_count

#define LOX_OBJECT_STRING_FLAG_COPY 1
#define LOX_OBJECT_STRING_FLAG_CONSTANT 2

#if !defined(NDEBUG) && !defined(LOX_TEST)
// #define DEBUG_STRESS_GC
// #define DEBUG_LOG_GC
// #define DEBUG_LOG_GC_VERBOSE
// #define DEBUG_TRACE_EXECUTION
// #define DEBUG_PRINT_CODE
#define DEBUG_PRINT_SETTINGS
#endif
