#include "object.h"
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
  }
  }
}

lox_object *lox_object_new(size_t size, lox_object_type type) {
  lox_object *obj = ALLOC_SIZE(size);
  obj->type = type;

  // Insert the newly created object into the list of objects stored in the VM
  // for garbage collection
  obj->next = vm.objects;
  vm.objects = obj;

  return obj;
}

void lox_object_free(lox_object *obj) {
  switch (obj->type) {
  case OBJ_STRING:
    lox_object_string_free((lox_object_string *)obj);
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
  lox_hash_table_put(&vm.strings, lox_value_from_object((lox_object *)obj),
                     lox_value_from_nil());

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
