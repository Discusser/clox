#include "scanner.h"
#include <ctype.h>
#include <string.h>

typedef struct {
  const char *start;
  const char *current;
  int line;
} lox_scanner;

static lox_token consume_string();
static lox_token consume_number();
static lox_token consume_identifier();

static lox_token_type identifier_type();
static lox_token_type check_keyword(int start, int length, const char *rest,
                                    lox_token_type type);

static bool is_at_end();
static bool is_identifier_char(char c);

static char consume();
static char consume_expect(lox_token_type type, const char *message);
static void consume_extra();
static bool match(char c);

static char peek();
static char peek_next();

lox_scanner scanner;

void init_scanner(const char *source) {
  scanner.start = source;
  scanner.current = source;
  scanner.line = 1;
}

lox_token scan_token() {
  consume_extra();

  scanner.start = scanner.current;

  if (is_at_end())
    return make_token(TOKEN_EOF);

  char c = consume();

  if (isdigit(c))
    return consume_number();
  if (is_identifier_char(c)) {
    return consume_identifier();
  }

  switch (c) {
  case '(':
    return make_token(TOKEN_LEFT_PAREN);
  case ')':
    return make_token(TOKEN_RIGHT_PAREN);
  case '{':
    return make_token(TOKEN_LEFT_BRACE);
  case '}':
    return make_token(TOKEN_RIGHT_BRACE);
  case ';':
    return make_token(TOKEN_SEMICOLON);
  case ',':
    return make_token(TOKEN_COMMA);
  case '.':
    return make_token(TOKEN_DOT);
  case '-':
    return make_token(TOKEN_MINUS);
  case '+':
    return make_token(TOKEN_PLUS);
  case '/':
    return make_token(TOKEN_SLASH);
  case '*':
    return make_token(TOKEN_STAR);
  case '!':
    return make_token(match('=') ? TOKEN_BANG_EQUAL : TOKEN_BANG);
  case '=':
    return make_token(match('=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL);
  case '<':
    return make_token(match('=') ? TOKEN_LESS_EQUAL : TOKEN_LESS);
  case '>':
    return make_token(match('=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER);
  case '"':
    return consume_string();
  }

  return error_token("Unexpected character.");
}

lox_token make_token(lox_token_type type) {
  lox_token token;
  token.type = type;
  token.start = scanner.start;
  token.length = scanner.current - scanner.start;
  token.line = scanner.line;
  return token;
}

lox_token error_token(const char *message) {
  lox_token token;
  token.type = TOKEN_ERROR;
  token.start = message;
  token.length = strlen(message);
  token.line = scanner.line;
  return token;
}

lox_token consume_string() {
  while (peek() != '"' && !is_at_end()) {
    if (peek() == '\n')
      scanner.line++;
    consume();
  }

  if (is_at_end())
    return error_token("Unterminated string.");

  if (consume() != '"') {
    return error_token("Expected '\"' to terminate string.");
  }

  return make_token(TOKEN_STRING);
}

lox_token consume_number() {
  while (isdigit(peek()))
    consume();
  if (peek() == '.' && isdigit(peek_next())) {
    do {
      consume();
    } while (isdigit(peek()));
  }

  return make_token(TOKEN_NUMBER);
}

lox_token consume_identifier() {
  while (is_identifier_char(peek()) || isdigit(peek())) {
    consume();
  }

  return make_token(identifier_type());
}

lox_token_type identifier_type() {
  switch (*scanner.start) {
  case 'a':
    return check_keyword(1, 2, "nd", TOKEN_AND);
  case 'c':
    return check_keyword(1, 4, "lass", TOKEN_CLASS);
  case 'e':
    return check_keyword(1, 3, "lse", TOKEN_ELSE);
  case 'f':
    if (scanner.current - scanner.start > 1) {
      switch (*(scanner.start + 1)) {
      case 'a':
        return check_keyword(2, 3, "lse", TOKEN_FALSE);
      case 'o':
        return check_keyword(2, 1, "r", TOKEN_FOR);
      case 'u':
        return check_keyword(2, 1, "n", TOKEN_FUN);
      }
    }
  case 'i':
    return check_keyword(1, 1, "f", TOKEN_IF);
  case 'n':
    return check_keyword(1, 2, "il", TOKEN_NIL);
  case 'o':
    return check_keyword(1, 1, "r", TOKEN_OR);
  case 'p':
    return check_keyword(1, 4, "rint", TOKEN_PRINT);
  case 'r':
    return check_keyword(1, 5, "eturn", TOKEN_RETURN);
  case 's':
    return check_keyword(1, 4, "uper", TOKEN_SUPER);
  case 't':
    if (scanner.current - scanner.start > 1) {
      switch (scanner.start[1]) {
      case 'h':
        return check_keyword(2, 2, "is", TOKEN_THIS);
      case 'r':
        return check_keyword(2, 2, "ue", TOKEN_TRUE);
      }
    }
    break;
  case 'v':
    return check_keyword(1, 2, "ar", TOKEN_VAR);
  case 'w':
    return check_keyword(1, 4, "hile", TOKEN_WHILE);
  }

  return TOKEN_IDENTIFIER;
}

lox_token_type check_keyword(int start, int length, const char *rest,
                             lox_token_type type) {
  if (scanner.current - scanner.start == start + length &&
      memcmp(scanner.start + start, rest, length) == 0) {
    return type;
  }

  return TOKEN_IDENTIFIER;
}

bool is_at_end() { return peek() == '\0'; }

char consume() { return *scanner.current++; }

char consume_expect(lox_token_type type, const char *message) {}

void consume_extra() {
  for (;;) {
    char c = peek();
    switch (c) {
    case '\n':
      scanner.line++;
      consume();
      break;
    case ' ':
    case '\r':
    case '\t':
      consume();
      break;
    case '/':
      if (peek_next() == '/') {
        while (peek() != '\n' && !is_at_end())
          consume();
      } else {
        return;
      }
      break;
    default:
      return;
    }
  }
}

char peek() { return *scanner.current; }

char peek_next() {
  if (is_at_end())
    return '\0';
  return *(scanner.current + 1);
}

bool match(char c) {
  if (is_at_end() || peek() != c)
    return false;
  consume();
  return true;
}

bool is_identifier_char(char c) { return isalpha(c) || c == '_'; }
