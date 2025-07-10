#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

static void repl() {
  char line[1024];
  for (;;) {
    printf("> ");
    if (!fgets(line, sizeof(line), stdin)) {
      printf("\n");
      break;
    }
    interpret(line);
  }
}

static char *read_file(const char *path) {
  FILE *file = fopen(path, "rb");
  if (file == NULL) {
    fprintf(stderr, "Failed to open file at \"%s\"\n", path);
    exit(EX_IOERR);
  }

  fseek(file, 0, SEEK_END);
  size_t file_size = ftell(file);
  rewind(file);

  char *buf = malloc(sizeof(char) * (file_size + 1));
  if (buf == NULL) {
    fprintf(stderr, "Could not allocate buffer to store file at \"%s\"\n",
            path);
    exit(EX_IOERR);
  }

  size_t bytes_read = fread(buf, sizeof(char), file_size, file);
  if (bytes_read < file_size) {
    fprintf(stderr, "Could not read file at \"%s\"\n", path);
    exit(EX_IOERR);
  }
  buf[bytes_read] = '\0';

  fclose(file);

  return buf;
}

static void run_file(const char *path) {
  char *source = read_file(path);
  interpret_result result = interpret(source);
  free(source);

  if (result == INTERPRET_COMPILE_ERROR)
    exit(EX_DATAERR);
  if (result == INTERPRET_RUNTIME_ERROR)
    exit(EX_SOFTWARE);
}

int main(int argc, const char **argv) {
  init_vm();

  if (argc == 1) {
    repl();
  } else if (argc == 2) {
    run_file(argv[1]);
  } else {
    fprintf(stderr, "Usage: %s [path_to_file]\n", argv[0]);
    exit(EX_USAGE);
  }

  free_vm();

  return 0;
}
