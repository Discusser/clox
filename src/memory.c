#include "memory.h"
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include "compiler.h"
#include "vm.h"

extern lox_vm vm;

static void trace_references();
static void blacken_object(lox_object *obj);
static void sweep();

void *lox_reallocate(void *ptr, ssize_t old_size, ssize_t new_size) {
#ifdef DEBUG_LOG_GC_VERBOSE
  ssize_t prev = vm.bytes_allocated;
#endif
  vm.bytes_allocated += new_size - old_size;
#ifdef DEBUG_LOG_GC_VERBOSE
  // I have no idea why, but for some reason vm.bytes_allocated doesn't reach
  // zero at the end of the program. I'm not sure if this is an actual bug or if
  // there's just something that isn't being counted.
  printf("Allocated %zd bytes : %zd -> %zd (%zd -> %zd)\n", new_size - old_size,
         prev, vm.bytes_allocated, old_size, new_size);
#endif

  if (new_size > old_size) {
#ifdef DEBUG_STRESS_GC
    collect_garbage();
#endif

    if (vm.bytes_allocated > vm.next_gc) {
      collect_garbage();
    }
  }

  if (new_size == 0) {
    free(ptr);
    return NULL;
  }

  void *result = realloc(ptr, new_size);
  if (result == NULL)
    exit(1);
  return result;
}

int lox_grow_capacity(int capacity) {
  return capacity < LOX_ARRAY_MIN_CAPACITY ? LOX_ARRAY_MIN_CAPACITY
                                           : capacity * LOX_ARRAY_SCALE_FACTOR;
}

void collect_garbage() {
#ifdef DEBUG_LOG_GC
  printf("-- GC BEGIN\n");
  size_t before = vm.bytes_allocated;
#endif

  mark_roots();
  trace_references();
  lox_hash_table_remove_white(&vm.strings);
  sweep();

  vm.next_gc = vm.bytes_allocated * LOX_GC_HEAP_GROW_FACTOR;

#ifdef DEBUG_LOG_GC
  printf("-- GC END\n");
  printf("   Collected %zu bytes (from %zu to %zu) next at %zu\n",
         before - vm.bytes_allocated, before, vm.bytes_allocated, vm.next_gc);
#endif
}

void mark_roots() {
  for (int i = 0; i < vm.stack.size; i++) {
    mark_value(vm.stack.values[i]);
  }

  for (int i = 0; i < vm.frame_count; i++) {
    mark_object((lox_object *)vm.frames[i].closure);
  }

  for (lox_object_upvalue *upvalue = vm.open_upvalues; upvalue != NULL;
       upvalue = upvalue->next) {
    mark_object((lox_object *)upvalue);
  }

  mark_table(&vm.global_indices);
  mark_value_array(&vm.globals);

  lox_compiler_mark_roots();
}

void mark_value(lox_value value) {
  if (value.type == VAL_OBJECT) {
    mark_object(value.as.object);
  }
}

void mark_object(lox_object *obj) {
  if (obj == NULL || obj->is_marked)
    return;
  obj->is_marked = true;

  if (vm.gray_capacity < vm.gray_size + 1) {
    vm.gray_capacity = GROW_CAPACITY(vm.gray_capacity);
    // We use realloc instead of lox_reallocate because we are inside the
    // garbage collector now, and we don't want an allocation performed in
    // order to collect garbage to trigger another garbage collection.
    vm.gray_stack = (lox_object **)realloc(vm.gray_stack, sizeof(lox_object *) *
                                                              vm.gray_capacity);

    if (vm.gray_stack == NULL) {
      runtime_error("An error occurred while allocating memory for the garbage "
                    "collector.");
      exit(1);
    }
  }

  vm.gray_stack[vm.gray_size++] = obj;

#ifdef DEBUG_LOG_GC
  printf("%p mark ", obj);
  lox_print_object(obj);
  printf("\n");
#endif
}

void mark_table(lox_hash_table *table) {
  for (int i = 0; i < table->capacity; i++) {
    lox_hash_table_entry *entry = &table->entries[i];
    if (entry->key.type != VAL_EMPTY) {
      mark_value(entry->key);
      mark_value(entry->value);
    }
  }
}

static void trace_references() {
  while (vm.gray_size > 0) {
    lox_object *obj = vm.gray_stack[--vm.gray_size];
    blacken_object(obj);
  }
}

static void blacken_object(lox_object *obj) {
#ifdef DEBUG_LOG_GC
  printf("%p blacken ", obj);
  lox_print_object(obj);
  printf("\n");
#endif

  // PERF: Instead of adding OBJ_NATIVE to the gray stack,
  // simply ignore them in mark_object
  switch (obj->type) {
  case OBJ_NATIVE:
  case OBJ_STRING:
    break;
  case OBJ_FUNCTION: {
    lox_object_function *fun = (lox_object_function *)obj;
    mark_object((lox_object *)fun->name);
    mark_value_array(&fun->chunk.constants);
    break;
  }
  case OBJ_CLOSURE: {
    lox_object_closure *closure = (lox_object_closure *)obj;
    mark_object((lox_object *)closure->function);
    for (int i = 0; i < closure->upvalue_count; i++) {
      mark_object((lox_object *)closure->upvalues[i]);
    }
    break;
  }
  case OBJ_UPVALUE:
    mark_value(((lox_object_upvalue *)obj)->closed);
    break;
  }
}

void mark_value_array(lox_value_array *array) {
  for (int i = 0; i < array->size; i++) {
    if (array->values[i].type != VAL_EMPTY) {
      mark_value(array->values[i]);
    }
  }
}

static void sweep() {
  lox_object *previous = NULL;
  lox_object *object = vm.objects;
  while (object != NULL) {
    if (object->is_marked) {
      object->is_marked = false;
      previous = object;
      object = object->next;
    } else {
      lox_object *unreached = object;
      object = object->next;
      if (previous != NULL) {
        previous->next = object;
      } else {
        vm.objects = object;
      }

      lox_object_free(unreached);
    }
  }
}
