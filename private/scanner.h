#pragma once

#include <stdbool.h>

typedef enum {
  // Single-character tokens.
  TOKEN_LEFT_PAREN,
  TOKEN_RIGHT_PAREN,
  TOKEN_LEFT_BRACE,
  TOKEN_RIGHT_BRACE,
  TOKEN_COMMA,
  TOKEN_DOT,
  TOKEN_MINUS,
  TOKEN_PLUS,
  TOKEN_SEMICOLON,
  TOKEN_SLASH,
  TOKEN_STAR,
  TOKEN_COLON,
  TOKEN_PERCENTAGE,
  // One or two character tokens.
  TOKEN_BANG,
  TOKEN_BANG_EQUAL,
  TOKEN_EQUAL,
  TOKEN_EQUAL_EQUAL,
  TOKEN_GREATER,
  TOKEN_GREATER_EQUAL,
  TOKEN_LESS,
  TOKEN_LESS_EQUAL,
  // Literals.
  TOKEN_IDENTIFIER,
  TOKEN_STRING,
  TOKEN_NUMBER,
  // Keywords.
  TOKEN_AND,
  TOKEN_CLASS,
  TOKEN_ELSE,
  TOKEN_FALSE,
  TOKEN_FOR,
  TOKEN_FUN,
  TOKEN_IF,
  TOKEN_NIL,
  TOKEN_OR,
  TOKEN_PRINT,
  TOKEN_RETURN,
  TOKEN_SUPER,
  TOKEN_THIS,
  TOKEN_TRUE,
  TOKEN_VAR,
  TOKEN_CONST,
  TOKEN_WHILE,
  TOKEN_SWITCH,
  TOKEN_CASE,
  TOKEN_DEFAULT,
  TOKEN_BREAK,
  TOKEN_CONTINUE,

  TOKEN_ERROR,
  TOKEN_EOF
} lox_token_type;

typedef struct {
  lox_token_type type;
  const char *start;
  int length;
  int line;
} lox_token;

void init_scanner(const char *source);

lox_token scan_token();
lox_token make_token(lox_token_type type);
lox_token error_token(const char *message);
