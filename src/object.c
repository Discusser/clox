#include "object.h"
#include "chunk.h"
#include "memory.h"
#include "value.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern lox_vm vm;

#define OBJ_NEW(struct_type, object_type)                                      \
  ((struct_type *)lox_object_new(sizeof(struct_type), object_type))

void lox_print_object(lox_object *obj) {
  switch (obj->type) {
  case OBJ_STRING: {
    lox_object_string *str = ((lox_object_string *)obj);
    printf("%.*s", str->length, str->chars);
    break;
  }
  case OBJ_FUNCTION:
    lox_object_function_print((lox_object_function *)obj);
    break;
  case OBJ_NATIVE:
    printf("<native>");
    break;
  case OBJ_CLOSURE:
    lox_object_function_print(((lox_object_closure *)obj)->function);
    break;
  case OBJ_UPVALUE:
    lox_print_value(*((lox_object_upvalue *)obj)->location);
    break;
  case OBJ_CLASS:
    lox_object_class_print((lox_object_class *)obj);
    break;
  case OBJ_INSTANCE:
    lox_object_instance_print((lox_object_instance *)obj);
    break;
  case OBJ_BOUND_METHOD:
    lox_object_bound_method_print((lox_object_bound_method *)obj);
    break;
  }
}

lox_object *lox_object_new(size_t size, lox_object_type type) {
  lox_object *obj = ALLOC_SIZE(size);
  obj->type = type;
  obj->is_marked = !vm.mark_value;

  // Insert the newly created object into the list of objects stored in the VM
  // for garbage collection
  obj->next = vm.objects;
  vm.objects = obj;

#ifdef DEBUG_LOG_GC
  printf("%p allocate %zu for %d\n", obj, size, type);
#endif

  return obj;
}

void lox_object_free(lox_object *obj) {
#ifdef DEBUG_LOG_GC
  printf("%p free type %d\n", obj, obj->type);
#endif

  switch (obj->type) {
  case OBJ_STRING:
    lox_object_string_free((lox_object_string *)obj);
    break;
  case OBJ_FUNCTION:
    lox_object_function_free((lox_object_function *)obj);
    break;
  case OBJ_NATIVE:
    lox_object_native_free((lox_object_native *)obj);
    break;
  case OBJ_CLOSURE:
    lox_object_closure_free((lox_object_closure *)obj);
    break;
  case OBJ_UPVALUE:
    lox_object_upvalue_free((lox_object_upvalue *)obj);
    break;
  case OBJ_CLASS:
    lox_object_class_free((lox_object_class *)obj);
    break;
  case OBJ_INSTANCE:
    lox_object_instance_free((lox_object_instance *)obj);
    break;
  case OBJ_BOUND_METHOD:
    lox_object_bound_method_free((lox_object_bound_method *)obj);
    break;
  }
}

lox_object_type lox_object_get_type(lox_object *obj) { return obj->type; }

lox_object_string *lox_object_string_new(char *chars, int length, char flags) {
  bool is_constant = (flags & LOX_OBJECT_STRING_FLAG_CONSTANT) ==
                     LOX_OBJECT_STRING_FLAG_CONSTANT;
  bool should_copy =
      (flags & LOX_OBJECT_STRING_FLAG_COPY) == LOX_OBJECT_STRING_FLAG_COPY;

  uint32_t hash = lox_object_string_compute_hash(chars, length);

  // If the string is interned, no point in allocating new memory.
  lox_object_string *interned =
      lox_hash_table_find_string(&vm.strings, chars, length, hash);
  if (interned != NULL) {
    // We only free the passed characters if the string is interned and we own
    // the characters, that is if we aren't copying the string, and the string
    // is not constant.
    if (!should_copy && !is_constant) {
      FREE_ARRAY(char, chars, length + 1);
    }
    return interned;
  }

  // If LOX_OBJECT_STRING_FLAG_COPY is set, we copy the characters that have
  // been provided.
  char *heap_chars = NULL;
  if (should_copy) {
    heap_chars = ALLOC_ARRAY(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
  } else {
    heap_chars = chars;
  }

  // Create the new string
  lox_object_string *obj = OBJ_NEW(lox_object_string, OBJ_STRING);
  obj->chars = heap_chars;
  obj->length = length;
  obj->is_constant = is_constant;
  obj->hash = hash;

  // Intern the newly-created string
  push(lox_value_from_object((lox_object *)obj));
  lox_hash_table_put(&vm.strings, lox_value_from_object((lox_object *)obj),
                     lox_value_from_nil());
  pop();

  return obj;
}

lox_object_string *lox_object_string_new_consume(char *chars, int length,
                                                 bool is_constant) {
  char flags = 0;
  if (is_constant)
    flags = LOX_OBJECT_STRING_FLAG_CONSTANT;
  return lox_object_string_new(chars, length, flags);
}

lox_object_string *lox_object_string_new_copy(const char *chars, int length) {
  char flags = LOX_OBJECT_STRING_FLAG_COPY;
  return lox_object_string_new((char *)chars, length, flags);
}

void lox_object_string_free(lox_object_string *obj) {
  FREE_ARRAY(char, obj->chars, obj->length + 1);
  if (!obj->is_constant) {
    FREE(lox_object_string, obj);
  }
}

uint32_t lox_object_string_compute_hash(const char *chars, int length) {
  // Implementation of the FNV-1a algorithm. Constant values are taken from
  // http://www.isthe.com/chongo/tech/comp/fnv/#FNV-param
  uint32_t hash = 2166136261;
  for (int i = 0; i < length; i++) {
    hash = hash ^ chars[i];
    hash = hash * 16777619;
  }
  return hash;
}

int lox_object_string_get_length(lox_object_string *obj) { return obj->length; }

char *lox_object_string_get_chars(lox_object_string *obj) { return obj->chars; }

bool lox_object_string_is_constant(lox_object_string *obj) {
  return obj->is_constant;
}

lox_object_function *lox_object_function_new() {
  lox_object_function *obj = OBJ_NEW(lox_object_function, OBJ_FUNCTION);
  lox_chunk_initialize(&obj->chunk);
  obj->name = NULL;
  obj->arity = 0;
  obj->upvalue_count = 0;
  return obj;
}

void lox_object_function_free(lox_object_function *obj) {
  lox_chunk_free(&obj->chunk);
  FREE(lox_object_function, obj);
}

void lox_object_function_print(lox_object_function *obj) {
  if (obj->name == NULL) {
    printf("<script>");
    return;
  }
  lox_object_function *fun = ((lox_object_function *)obj);
  printf("<fn ");
  lox_print_object((lox_object *)fun->name);
  printf(">");
}

lox_object_native *lox_object_native_new(const char *name,
                                         lox_native_function function,
                                         int arity) {
  lox_object_native *obj = OBJ_NEW(lox_object_native, OBJ_NATIVE);
  obj->name = name;
  obj->function = function;
  obj->arity = arity;
  return obj;
}

void lox_object_native_free(lox_object_native *obj) {
  FREE(lox_native_function, obj);
}

lox_object_closure *lox_object_closure_new(lox_object_function *function) {
  lox_object_closure *obj = OBJ_NEW(lox_object_closure, OBJ_CLOSURE);
  obj->function = function;
  obj->upvalues = ALLOC_ARRAY(lox_object_upvalue *, function->upvalue_count);
  obj->upvalue_count = function->upvalue_count;
  memset(obj->upvalues, '\0',
         function->upvalue_count * sizeof(lox_object_upvalue *));
  return obj;
}

void lox_object_closure_free(lox_object_closure *obj) {
  FREE_ARRAY(lox_object_upvalue *, obj->upvalues, obj->upvalue_count);
  FREE(lox_object_closure, obj);
}

lox_object_upvalue *lox_object_upvalue_new(lox_value *slot) {
  lox_object_upvalue *obj = OBJ_NEW(lox_object_upvalue, OBJ_UPVALUE);
  obj->location = slot;
  obj->next = NULL;
  obj->closed = lox_value_from_nil();
  return obj;
}

void lox_object_upvalue_free(lox_object_upvalue *obj) {
  FREE(lox_object_upvalue, obj);
}

lox_object_class *lox_object_class_new(lox_object_string *name) {
  lox_object_class *obj = OBJ_NEW(lox_object_class, OBJ_CLASS);
  obj->name = name;
  obj->methods = ALLOC_TYPE(lox_hash_table);
  lox_hash_table_init(obj->methods);
  return obj;
}

void lox_object_class_print(lox_object_class *obj) {
  printf("<class ");
  lox_print_object((lox_object *)obj->name);
  printf(">");
}

void lox_object_class_free(lox_object_class *obj) {
  lox_hash_table_free(obj->methods);
  FREE(lox_hash_table, obj->methods);
  FREE(lox_object_class, obj);
}

lox_object_instance *lox_object_instance_new(lox_object_class *clazz) {
  lox_object_instance *obj = OBJ_NEW(lox_object_instance, OBJ_INSTANCE);
  obj->clazz = clazz;
  obj->fields = ALLOC_TYPE(lox_hash_table);
  lox_hash_table_init(obj->fields);
  return obj;
}

void lox_object_instance_print(lox_object_instance *obj) {
  printf("<instance ");
  lox_print_object((lox_object *)obj->clazz->name);
  printf(">");
}

void lox_object_instance_free(lox_object_instance *obj) {
  lox_hash_table_free(obj->fields);
  FREE(lox_hash_table, obj->fields);
  FREE(lox_object_instance, obj);
}

lox_object_bound_method *
lox_object_bound_method_new(lox_value receiver, lox_object_closure *method) {
  lox_object_bound_method *obj =
      OBJ_NEW(lox_object_bound_method, OBJ_BOUND_METHOD);
  obj->receiver = receiver;
  obj->method = method;
  return obj;
}

void lox_object_bound_method_print(lox_object_bound_method *obj) {
  lox_print_object((lox_object *)obj->method);
}

void lox_object_bound_method_free(lox_object_bound_method *obj) {
  FREE(lox_object_bound_method, obj);
}
