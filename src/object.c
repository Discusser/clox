#include "object.h"
#include "memory.h"
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern lox_vm vm;

#define OBJ_NEW(struct_type, object_type)                                      \
  ((struct_type *)lox_object_new(sizeof(struct_type), object_type))

void print_object(lox_object *obj) {
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

lox_object_string *lox_object_string_new_consume(char *chars, int length,
                                                 bool is_constant) {
  lox_object_string *obj = OBJ_NEW(lox_object_string, OBJ_STRING);
  obj->chars = chars;
  obj->length = length;
  obj->is_constant = is_constant;
  return obj;
}

lox_object_string *lox_object_string_new_copy(const char *chars, int length) {
  char *heap_chars = ALLOC_ARRAY(char, length + 1);
  memcpy(heap_chars, chars, length);
  heap_chars[length] = '\0';
  return lox_object_string_new_consume(heap_chars, length, false);
}

void lox_object_string_free(lox_object_string *obj) {
  FREE_ARRAY(char, obj->chars, obj->length + 1);
  if (!obj->is_constant) {
    FREE(lox_object_string, obj);
  }
}

int lox_object_string_get_length(lox_object_string *obj) { return obj->length; }

char *lox_object_string_get_chars(lox_object_string *obj) { return obj->chars; }
