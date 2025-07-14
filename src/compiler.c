#include "compiler.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "scanner.h"
#include "value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT, // =
  PREC_OR,         // or
  PREC_AND,        // and
  PREC_EQUALITY,   // == !=
  PREC_COMPARISON, // < > <= >=
  PREC_TERM,       // + -
  PREC_FACTOR,     // * /
  PREC_UNARY,      // ! -
  PREC_CALL,       // . ()
  PREC_PRIMARY
} lox_precedence;

typedef void (*lox_parse_function)();

typedef struct {
  lox_parse_function prefix;
  lox_parse_function infix;
  lox_precedence precedence;
} lox_parse_rule;

typedef struct {
  lox_token current;
  lox_token previous;
  bool had_error;
  bool panic_mode;
} lox_parser;

static void consume();
static void consume_expected(lox_token_type type, const char *message);

static void error(const char *message);
static void error_at(lox_token *token, const char *message);
static void error_at_current(const char *message);

static lox_chunk *current_chunk();
static void emit_byte(uint8_t byte);
static void emit_bytes2(uint8_t byte1, uint8_t byte2);
static void emit_word(int word);
static int emit_constant(lox_value value);
static void emit_return();
static void end_compiler();

static lox_parse_rule *get_parse_rule(lox_token_type type);
static void parse_precedence(lox_precedence precedence);
static void expression();
static void number();
static void grouping();
static void unary();
static void binary();
static void literal();
static void string();

lox_parser parser;
lox_chunk *compiling_chunk;
lox_parse_rule rules[] = {
    [TOKEN_LEFT_PAREN] = {grouping, NULL, PREC_NONE},
    [TOKEN_RIGHT_PAREN] = {NULL, NULL, PREC_NONE},
    [TOKEN_LEFT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_RIGHT_BRACE] = {NULL, NULL, PREC_NONE},
    [TOKEN_COMMA] = {NULL, NULL, PREC_NONE},
    [TOKEN_DOT] = {NULL, NULL, PREC_NONE},
    [TOKEN_MINUS] = {unary, binary, PREC_TERM},
    [TOKEN_PLUS] = {NULL, binary, PREC_TERM},
    [TOKEN_SEMICOLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_SLASH] = {NULL, binary, PREC_FACTOR},
    [TOKEN_STAR] = {NULL, binary, PREC_FACTOR},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LESS] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_IDENTIFIER] = {NULL, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, NULL, PREC_NONE},
    [TOKEN_CLASS] = {NULL, NULL, PREC_NONE},
    [TOKEN_ELSE] = {NULL, NULL, PREC_NONE},
    [TOKEN_FALSE] = {literal, NULL, PREC_NONE},
    [TOKEN_FOR] = {NULL, NULL, PREC_NONE},
    [TOKEN_FUN] = {NULL, NULL, PREC_NONE},
    [TOKEN_IF] = {NULL, NULL, PREC_NONE},
    [TOKEN_NIL] = {literal, NULL, PREC_NONE},
    [TOKEN_OR] = {NULL, NULL, PREC_NONE},
    [TOKEN_PRINT] = {NULL, NULL, PREC_NONE},
    [TOKEN_RETURN] = {NULL, NULL, PREC_NONE},
    [TOKEN_SUPER] = {NULL, NULL, PREC_NONE},
    [TOKEN_THIS] = {NULL, NULL, PREC_NONE},
    [TOKEN_TRUE] = {literal, NULL, PREC_NONE},
    [TOKEN_VAR] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

bool lox_compiler_compile(const char *source, lox_chunk *chunk) {
  init_scanner(source);
  compiling_chunk = chunk;

  parser.had_error = false;
  parser.panic_mode = false;

  consume();
  expression();
  consume_expected(TOKEN_EOF, "Expected end of expression.");

  end_compiler();

  return !parser.had_error;
}

static void consume() {
  parser.previous = parser.current;

  for (;;) {
    parser.current = scan_token();
    if (parser.current.type != TOKEN_ERROR)
      break;

    error_at_current(parser.current.start);
  }
}

static void consume_expected(lox_token_type type, const char *message) {
  if (parser.current.type == type) {
    consume();
  } else {
    error_at_current(message);
  }
}

static void error(const char *message) { error_at(&parser.previous, message); }

static void error_at(lox_token *token, const char *message) {
  if (parser.panic_mode)
    return;
  parser.panic_mode = true;
  fprintf(stderr, "[line %d] ERROR ", token->line);
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, "at end");
  } else if (token->type != TOKEN_ERROR) {
    fprintf(stderr, "at %.*s", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}

static lox_chunk *current_chunk() { return compiling_chunk; }

static void emit_byte(uint8_t byte) {
  lox_chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes2(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_word(int word) {
  uint8_t *ptr = (uint8_t *)&word;
  emit_byte(ptr[0]);
  emit_byte(ptr[1]);
  emit_byte(ptr[2]);
  emit_byte(ptr[3]);
}

static int emit_constant(lox_value value) {
  int constant = lox_chunk_add_constant(current_chunk(), value);
  if (constant <= UINT8_MAX) {
    emit_bytes2(OP_CONSTANT, constant);
  } else {
    emit_byte(OP_CONSTANT);
    emit_word(constant);
  }
  return constant;
}

static void emit_return() { emit_byte(OP_RETURN); }

static void end_compiler() {
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    lox_disassemble_chunk(current_chunk(), "code");
  }
#endif
}

static lox_parse_rule *get_parse_rule(lox_token_type type) {
  return &rules[type];
}

static void parse_precedence(lox_precedence precedence) {
  consume();
  lox_parse_function prefix_rule = get_parse_rule(parser.previous.type)->prefix;
  if (prefix_rule == NULL) {
    error("Expected expression");
    return;
  }

  prefix_rule();

  while (precedence <= get_parse_rule(parser.current.type)->precedence) {
    consume();
    lox_parse_function infix_rule = get_parse_rule(parser.previous.type)->infix;
    infix_rule();
  }
}

static void expression() { parse_precedence(PREC_ASSIGNMENT); }

static void number() {
  lox_value value = lox_value_from_number(strtod(parser.previous.start, NULL));
  emit_constant(value);
}

static void grouping() {
  expression();
  consume_expected(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void unary() {
  lox_token_type operator_type = parser.previous.type;

  parse_precedence(PREC_UNARY);

  switch (operator_type) {
  case TOKEN_MINUS:
    emit_byte(OP_NEGATE);
  case TOKEN_BANG:
    emit_byte(OP_NOT);
    break;
  default:
    return;
  }
}

static void binary() {
  lox_token_type operator_type = parser.previous.type;
  lox_parse_rule *rule = get_parse_rule(operator_type);
  parse_precedence(rule->precedence + 1);

  switch (operator_type) {
  case TOKEN_PLUS:
    emit_byte(OP_ADD);
    break;
  case TOKEN_MINUS:
    emit_byte(OP_SUBTRACT);
    break;
  case TOKEN_STAR:
    emit_byte(OP_MULTIPLY);
    break;
  case TOKEN_SLASH:
    emit_byte(OP_DIVIDE);
    break;
  case TOKEN_EQUAL_EQUAL:
    emit_byte(OP_EQ);
    break;
  case TOKEN_BANG_EQUAL:
    emit_byte(OP_NEQ);
    break;
  case TOKEN_GREATER:
    emit_byte(OP_GREATER);
    break;
  case TOKEN_GREATER_EQUAL:
    emit_byte(OP_GREATEREQ);
    break;
  case TOKEN_LESS:
    emit_byte(OP_LESS);
    break;
  case TOKEN_LESS_EQUAL:
    emit_byte(OP_LESSEQ);
    break;
  default:
    return;
  }
}

static void literal() {
  switch (parser.previous.type) {
  case TOKEN_TRUE:
    emit_byte(OP_TRUE);
    break;
  case TOKEN_FALSE:
    emit_byte(OP_FALSE);
    break;
  case TOKEN_NIL:
    emit_byte(OP_NIL);
    break;
  default:
    return;
  }
}

static void string() {
  lox_object_string *str = lox_object_string_new_consume(
      (char *)(parser.previous.start + 1), parser.previous.length - 2, true);
  emit_constant(lox_value_from_object((lox_object *)str));
}
