#include "chunk.h"
#include "debug.h"
#include "vm.h"

int main(int argc, const char **argv) {
  init_vm();

  Chunk chunk;
  initialize_chunk(&chunk);
  int a = add_constant(&chunk, 4);
  int b = add_constant(&chunk, 3);
  int c = add_constant(&chunk, 2);
  write_to_chunk(&chunk, OP_CONSTANT_LONG, 123);
  write_word_to_chunk(&chunk, a, 123);
  write_to_chunk(&chunk, OP_CONSTANT_LONG, 123);
  write_word_to_chunk(&chunk, b, 123);
  write_to_chunk(&chunk, OP_NEGATE, 123);
  write_to_chunk(&chunk, OP_MULTIPLY, 123);
  write_to_chunk(&chunk, OP_RETURN, 123);
  interpret(&chunk);

  free_chunk(&chunk);
  free_vm();

  return 0;
}
