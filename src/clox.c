#include "memory.h"
#include <argp.h>
#include "vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>

struct lox_settings lox_settings;

const char *argp_program_version = LOX_PROGRAM_VERSION;
const struct argp_option long_options[] = {
    {"array_minimum_capacity", required_argument, 0, 0}, {0, 0, 0, 0}};
// static struct argp argp = {long_options, parse_opt}

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

  // Collect any remaining garbage, especially if the program exited
  // prematurely.
  collect_garbage();

  if (result == INTERPRET_COMPILE_ERROR)
    exit(EX_DATAERR);
  if (result == INTERPRET_RUNTIME_ERROR)
    exit(EX_SOFTWARE);
}

static void parse_opts(int argc, char *const *argv) {}

static void apply_default_settings() {
  lox_settings.array_minimum_capacity = 8;
  lox_settings.array_scale_factor = 2;
  lox_settings.gc_heap_grow_factor = 2;
  lox_settings.hash_table_load_factor = 0.75;
  lox_settings.initial_stack_size = 256;
  lox_settings.max_local_count = 256;
}

int main(int argc, char *const *argv) {
  apply_default_settings();
  parse_opts(argc, argv);

  init_vm();

  if (optind == argc) {
    repl();
  } else if (optind == argc - 1) {
    run_file(argv[optind]);
  }

  free_vm();

  return 0;
}
