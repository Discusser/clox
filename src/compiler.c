#include "compiler.h"
#include "scanner.h"
#include <stdio.h>

void compile(const char *source) {
  init_scanner(source);

  int line = -1;
  for (;;) {
    lox_token token = scan_token();

    printf("LINE %4d ", token.line);
    line = token.line;

    printf("TYPE %2d '%.*s'\n", token.type, token.length, token.start);

    if (token.type == TOKEN_EOF)
      break;
  }
}
