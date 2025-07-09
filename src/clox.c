#include "chunk.h"
#include "debug.h"

int main(int argc, const char **argv) {
  Chunk chunk;
  initialize_chunk(&chunk);
  int constant = add_constant(&chunk, 1.2);
  int long_const = add_constant(&chunk, 62);
  write_to_chunk(&chunk, OP_CONSTANT, 123);
  write_to_chunk(&chunk, constant, 123);
  write_to_chunk(&chunk, OP_CONSTANT_LONG, 124);
  write_word_to_chunk(&chunk, long_const, 124);
  write_to_chunk(&chunk, OP_RETURN, 124);
  disassemble_chunk(&chunk, "test");
  free_chunk(&chunk);

  return 0;
}
