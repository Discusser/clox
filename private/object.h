#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum { OBJ_STRING } lox_object_type;

// Forward reference lox_object so we can have it contain a pointer to another
// lox_object struct without any issues.
typedef struct lox_object lox_object;

// Base object struct. Every object in lox, that is every value that isn't a
// literal (number, boolean, nil), is represented by a child of lox_object.
struct lox_object {
  lox_object_type type;
  lox_object *next;
};

// String object used to represent lox strings.
typedef struct {
  lox_object object;
  char *chars;
  int length;
  uint32_t hash;
  // If this is set to true, then we can assume that the value of this
  // string is known as a compile-time, since it is a compile-time constant. A
  // constant string doesn't require any extra memory allocations since the
  // string itself is already contained in the source code.
  bool is_constant;
} lox_object_string;

// Helper function for printing a lox_object to standard output.
void lox_print_object(lox_object *obj);

// Allocates a new lox_object struct with the given size. This should only be
// called directly when creating a new child of lox_object.
lox_object *lox_object_new(size_t size, lox_object_type type);
// Frees a lox_object. This function dispatches to a child's free function
// according to the type.
void lox_object_free(lox_object *obj);
// Returns the type of the given lox_object.
lox_object_type lox_object_get_type(lox_object *obj);

// See the other constructors, and the LOX_OBJECT_STRING_FLAG_ macros in
// common.h for flags. If LOX_OBJECT_STRING_FLAG_COPY is set, `chars` is
// unmodified, and can be treated like a const pointer.
// LOX_OBJECT_STRING_FLAG_COPY and LOX_OBJECT_STRING_FLAG_CONSTANT are mutually
// exclusive, since a compile-time constant string should not be copied.
lox_object_string *lox_object_string_new(char *chars, int length, char flags);
// Creates a new lox_object_string without copying the chars pointer. If
// is_constant is true, the new object doesn't own the string. If it's false,
// the object takes ownership of the string.
lox_object_string *lox_object_string_new_consume(char *chars, int length,
                                                 bool is_constant);
// Creates a new lox_object_string by copying chars
lox_object_string *lox_object_string_new_copy(const char *chars, int length);
// Frees a lox_object_string.
void lox_object_string_free(lox_object_string *obj);
// Computes the hash for a given string (not necessarily null-terminated). This
// shouldn't be called manually, since lox_object_string_new_consume already
// calls it and stores the result in lox_object_string.hash
uint32_t lox_object_string_compute_hash(const char *chars, int length);
// Returns the length of the string contained in a lox_object_string
int lox_object_string_get_length(lox_object_string *obj);
// Returns the string contained in a lox_object_string
char *lox_object_string_get_chars(lox_object_string *obj);
// Returns whether or not the string contained in the lox_object_string is
// constant. If it is, the string is not owned by the object, and thus it is not
// responsible for freeing it. If it's not, the object is responsible for
// freeing the string.
bool lox_object_string_is_constant(lox_object_string *obj);
