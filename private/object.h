#pragma once

#include "value.h"

typedef enum { OBJ_STRING } lox_object_type;

struct lox_object {
  lox_object_type type;
  lox_object *next;
};

struct lox_object_string {
  lox_object object;
  char *chars;
  int length;
  bool is_constant;
};

void print_object(lox_object *obj);

// Allocates a new lox_object struct with the given size. This should only be
// called directly when creating a new child of lox_object.
lox_object *lox_object_new(size_t size, lox_object_type type);
void lox_object_free(lox_object *obj);
lox_object_type lox_object_get_type(lox_object *obj);

// Creates a new lox_object_string without copying the chars pointer. If
// is_constant is true, the new object doesn't own the string. If it's false,
// the object takes ownership of the string.
lox_object_string *lox_object_string_new_consume(char *chars, int length,
                                                 bool is_constant);
// Creates a new lox_object_string by copying chars
lox_object_string *lox_object_string_new_copy(const char *chars, int length);
void lox_object_string_free(lox_object_string *obj);
int lox_object_string_get_length(lox_object_string *obj);
char *lox_object_string_get_chars(lox_object_string *obj);
