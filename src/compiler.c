#include "compiler.h"
#include "memory.h"
#include "chunk.h"
#include "debug.h"
#include "object.h"
#include "scanner.h"
#include "table.h"
#include "value.h"
#include "vm.h"
#include <assert.h>
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

typedef void (*lox_parse_function)(bool can_assign);

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

DEFINE_LOX_ARRAY(lox_local, local_array);

void init_compiler(lox_compiler *curr);

static void consume();
static void consume_expected(lox_token_type type, const char *message);
static bool match(lox_token_type type);
static bool check(lox_token_type type);

static void synchronize();
static void error(const char *message);
static void error_at(lox_token *token, const char *message);
static void error_at_current(const char *message);
static int get_token_column(lox_token token);

static lox_chunk *current_chunk();
static void patch_jump(int location);
static void emit_jump_back(int location);
static int emit_jump(lox_op_code jump);
static void emit_byte(uint8_t byte);
static void emit_bytes2(uint8_t byte1, uint8_t byte2);
static void emit_short(uint16_t sh);
static uint16_t emit_constant(lox_value value);
static void emit_return();
static void end_compiler();

static uint16_t parse_variable(const char *error_message);
static uint16_t identifier_constant(lox_token *name, bool *is_cached);
static void define_variable(uint16_t index);
static void declare_variable(bool constant);
static void add_local(lox_token name, bool constant);
static bool are_identifiers_equal(lox_token *a, lox_token *b);
static int resolve_local(lox_compiler *compiler, lox_token *name);

static void begin_scope();
static void end_scope();

static void emit_break();
static void emit_continue();
static void patch_breaks();

static lox_parse_rule *get_parse_rule(lox_token_type type);
static void parse_precedence(lox_precedence precedence);
static void declaration();
static void variable_declaration();
static void statement();
static void break_statement();
static void continue_statement();
static void switch_statement();
static void switch_case();
static void default_case();
static void for_statement();
static void while_statement();
static void if_statement();
static void expression_statement();
static void print_statement();
static void block();
static void variable(bool can_assign);
static void named_variable(lox_token name, bool can_assign);
static void expression();
static void number(bool can_assign);
static void grouping(bool can_assign);
static void unary(bool can_assign);
static void binary(bool can_assign);
static void literal(bool can_assign);
static void string(bool can_assign);
static void compile_and(bool can_assign);
static void compile_or(bool can_assign);

extern lox_vm vm;

lox_parser parser;
lox_compiler *compiler;
lox_chunk *compiling_chunk;
const char *compiling_source;

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
    [TOKEN_PERCENTAGE] = {NULL, binary, PREC_FACTOR},
    [TOKEN_COLON] = {NULL, NULL, PREC_NONE},
    [TOKEN_BANG] = {unary, NULL, PREC_NONE},
    [TOKEN_BANG_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_EQUAL] = {NULL, NULL, PREC_NONE},
    [TOKEN_EQUAL_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_GREATER_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LESS] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_LESS_EQUAL] = {NULL, binary, PREC_EQUALITY},
    [TOKEN_IDENTIFIER] = {variable, NULL, PREC_NONE},
    [TOKEN_STRING] = {string, NULL, PREC_NONE},
    [TOKEN_NUMBER] = {number, NULL, PREC_NONE},
    [TOKEN_AND] = {NULL, compile_and, PREC_NONE},
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
    [TOKEN_CONST] = {NULL, NULL, PREC_NONE},
    [TOKEN_WHILE] = {NULL, NULL, PREC_NONE},
    [TOKEN_SWITCH] = {NULL, NULL, PREC_NONE},
    [TOKEN_CASE] = {NULL, NULL, PREC_NONE},
    [TOKEN_DEFAULT] = {NULL, NULL, PREC_NONE},
    [TOKEN_BREAK] = {NULL, NULL, PREC_NONE},
    [TOKEN_CONTINUE] = {NULL, NULL, PREC_NONE},
    [TOKEN_ERROR] = {NULL, NULL, PREC_NONE},
    [TOKEN_EOF] = {NULL, NULL, PREC_NONE},
};

bool lox_compiler_compile(const char *source, lox_chunk *chunk) {
  lox_compiler curr;
  init_compiler(&curr);
  init_scanner(source);
  compiling_chunk = chunk;
  compiling_source = source;

  parser.had_error = false;
  parser.panic_mode = false;

  consume();
  while (!match(TOKEN_EOF)) {
    declaration();
  }
  consume_expected(TOKEN_EOF, "Expected end of expression.");

  end_compiler();

  return !parser.had_error;
}

void init_compiler(lox_compiler *curr) {
  compiler = curr;
  lox_local_array_initialize(&compiler->locals);
  lox_hash_table_init(&compiler->global_constants);
  lox_int_array_initialize(&compiler->breaks);
  compiler->continue_jump_to = -1;
  compiler->scope_depth = 0;
  compiler->in_breakable = false;
  compiler->in_continueable = false;
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
  if (check(type)) {
    consume();
  } else {
    error_at_current(message);
  }
}

static bool match(lox_token_type type) {
  if (!check(type))
    return false;
  consume();
  return true;
}

static bool check(lox_token_type type) { return parser.current.type == type; }

static void synchronize() {
  parser.panic_mode = false;

  while (parser.current.type != TOKEN_EOF) {
    if (parser.previous.type == TOKEN_SEMICOLON)
      return;
    switch (parser.current.type) {
    case TOKEN_CLASS:
    case TOKEN_FUN:
    case TOKEN_VAR:
    case TOKEN_CONST:
    case TOKEN_FOR:
    case TOKEN_IF:
    case TOKEN_WHILE:
    case TOKEN_PRINT:
    case TOKEN_RETURN:
      return;
    default:;
    }

    consume();
  }
}

static void error(const char *message) { error_at(&parser.previous, message); }

static void error_at(lox_token *token, const char *message) {
  if (parser.panic_mode)
    return;
  parser.panic_mode = true;
  fprintf(stderr, "[line %d:%d] ERROR ", token->line, get_token_column(*token));
  if (token->type == TOKEN_EOF) {
    fprintf(stderr, "at end");
  } else if (token->type != TOKEN_ERROR) {
    fprintf(stderr, "at '%.*s'", token->length, token->start);
  }
  fprintf(stderr, ": %s\n", message);
  parser.had_error = true;
}

static void error_at_current(const char *message) {
  error_at(&parser.current, message);
}

static int get_token_column(lox_token token) {
  int line = 0;
  int col = 0;
  for (const char *p = compiling_source; *p != '\0'; p++) {
    if (*p == '\n') {
      line++;
      col = 0;
    } else {
      col++;
    }

    if (token.start == p)
      break;
  }

  return col;
}

static lox_chunk *current_chunk() { return compiling_chunk; }

static void patch_jump(int location) {
  int jump = current_chunk()->code.size - location - 2;

  if (jump > UINT16_MAX) {
    error("Too much code to jump over.");
  }

  current_chunk()->code.values[location] = (jump >> 8) & 0xff;
  current_chunk()->code.values[location + 1] = (jump) & 0xff;
}

static void emit_jump_back(int location) {
  emit_byte(OP_JMP_BACK);

  int offset = current_chunk()->code.size - location + 2;
  if (offset > UINT16_MAX) {
    error("Loop body too large.");
  }

  emit_byte((offset >> 8) & 0xff);
  emit_byte(offset & 0xff);
}

static int emit_jump(lox_op_code jump) {
  emit_byte(jump);
  emit_short(0);
  // Returns the offset of the jump's parameter in the bytecode.
  return current_chunk()->code.size - 2;
}

static void emit_byte(uint8_t byte) {
  lox_chunk_write(current_chunk(), byte, parser.previous.line);
}

static void emit_bytes2(uint8_t byte1, uint8_t byte2) {
  emit_byte(byte1);
  emit_byte(byte2);
}

static void emit_short(uint16_t sh) {
  emit_byte((sh >> 8) & 0xff);
  emit_byte(sh & 0xff);
}

static uint16_t emit_constant(lox_value value) {
  int constant = lox_chunk_add_constant(current_chunk(), value);
  if (constant <= UINT8_MAX) {
    emit_bytes2(OP_CONSTANT, constant);
  } else {
    emit_byte(OP_CONSTANT_LONG);
    emit_short(constant);
  }
  return constant;
}

static void emit_return() { emit_byte(OP_RETURN); }

static void end_compiler() {
  lox_local_array_free(&compiler->locals);
  lox_hash_table_free(&compiler->global_constants);
  lox_int_array_free(&compiler->breaks);
  emit_return();
#ifdef DEBUG_PRINT_CODE
  if (!parser.had_error) {
    lox_disassemble_chunk(current_chunk(), "code");
  }
#endif
}

static uint16_t parse_variable(const char *error_message) {
  bool constant = parser.previous.type == TOKEN_CONST;
  consume_expected(TOKEN_IDENTIFIER, error_message);

  declare_variable(constant);
  if (compiler->scope_depth > 0)
    return 0;

  bool is_cached;
  uint16_t index = identifier_constant(&parser.previous, &is_cached);
  lox_value value = lox_value_from_number(index);
  if (lox_hash_table_has(&compiler->global_constants, value)) {
    error("Cannot redeclare const variable.");
  }
  if (is_cached && constant) {
    error("Cannot redeclare variable as const.");
  }

  if (constant)
    lox_hash_table_put(&compiler->global_constants, value,
                       lox_value_from_bool(constant));

  return index;
}

static uint16_t identifier_constant(lox_token *name, bool *is_cached) {
  lox_value key = lox_value_from_object(
      (lox_object *)lox_object_string_new_copy(name->start, name->length));
  lox_value global;
  if (lox_hash_table_get(&vm.global_indices, key, &global)) {
    if (is_cached)
      *is_cached = true;

    // NOTE: We probably have to free key here since the entry already exists,
    // unless the GC should take care of it
    assert(global.type == VAL_NUMBER);
    return global.as.number;
  }

  if (is_cached)
    *is_cached = false;

  uint16_t index = vm.globals.size;
  lox_value_array_push(&vm.globals, lox_value_from_empty());

  lox_value num = lox_value_from_number(index);
  lox_hash_table_put(&vm.global_indices, key, num);
#ifndef NDEBUG
  lox_hash_table_put(&vm.global_names, num, key);
#endif
  return index;
}

static void define_variable(uint16_t index) {
  // If this variable is not global, don't emit any OP_DEFINE_GLOBAL opcodes.
  if (compiler->scope_depth > 0) {
    // Now that we've parsed the variable's initializer, we can update the depth
    // to mark it as initialized.
    compiler->locals.values[compiler->locals.size - 1].depth =
        compiler->scope_depth;
    return;
  }

  if (index <= UINT8_MAX) {
    emit_bytes2(OP_DEFINE_GLOBAL, index);
  } else {
    emit_byte(OP_DEFINE_GLOBAL_LONG);
    emit_short(index);
  }
}

static void declare_variable(bool constant) {
  if (compiler->scope_depth == 0) {
    return;
  }

  lox_token *name = &parser.previous;

  for (int i = compiler->locals.size - 1; i >= 0; i--) {
    lox_local local = compiler->locals.values[i];
    if (local.depth != -1 && local.depth < compiler->scope_depth)
      break;
    // For local variables, we allow shadowing variables in inner scopes, but
    // not in the same scope. This is allowed
    // {
    //  var a = 3;
    //  {
    //    var a = 2;
    //  }
    // }
    // This is not allowed
    // {
    //  var a = 3;
    //  var a = 2;
    // }
    if (local.depth > compiler->scope_depth)
      continue;

    if (are_identifiers_equal(&local.name, name)) {
      char *str;
      if (asprintf(&str,
                   "Cannot redeclare variable with name '%.*s' in local scope",
                   name->length, name->start) != -1) {
        error(str);
        free(str);
      } else {
        fprintf(stderr,
                "An error occurred while trying to print an error message.");
      }
      return;
    }
  }

  add_local(*name, constant);
}

static void add_local(lox_token name, bool constant) {
  if (compiler->locals.size > LOX_MAX_LOCAL_COUNT) {
    // NOTE: Since compiler->locals is a dynamic array, we probably don't need
    // this error message. It's important to note however that local variable
    // lookup is O(n) which means that having too many locals is not good,
    // unlike global variables that are stored in a hash table.
    char *str;
    if (asprintf(&str, "Exceeded maximum local variable count of %i",
                 LOX_MAX_LOCAL_COUNT) != -1) {
      error(str);
      free(str);
    } else {
      fprintf(stderr,
              "An error occurred while trying to print an error message.");
    }
    return;
  }

#ifndef NDEBUG
  lox_hash_table_put(
      &vm.local_names, lox_value_from_number(compiler->locals.size),
      lox_value_from_object(
          (lox_object *)lox_object_string_new_copy(name.start, name.length)));
#endif
  lox_local_array_push(&compiler->locals, (lox_local){name, -1, constant});
}

static bool are_identifiers_equal(lox_token *a, lox_token *b) {
  return a->length == b->length && memcmp(a->start, b->start, a->length) == 0;
}

static int resolve_local(lox_compiler *compiler, lox_token *name) {
  for (int i = compiler->locals.size - 1; i >= 0; i--) {
    if (are_identifiers_equal(&compiler->locals.values[i].name, name)) {
      if (compiler->locals.values[i].depth == -1) {
        error("Cannot use variable before it is initialized.");
      }
      return i;
    }
  }

  return -1;
}

static void begin_scope() { compiler->scope_depth++; }

static void end_scope() {
  compiler->scope_depth--;

  while (compiler->locals.size > 0 &&
         compiler->locals.values[compiler->locals.size - 1].depth >
             compiler->scope_depth) {
    // PERF: We can optimize this using a OP_POPN opcode, which takes 1
    // argument (uint8_t), the number of successive pops.
    emit_byte(OP_POP);
    lox_local_array_pop(&compiler->locals);
  }
}

static void emit_break() {
  lox_int_array_push(&compiler->breaks, emit_jump(OP_JMP));
  if (!compiler->in_breakable) {
    error("Cannot have 'break' statement outside of loop or switch statement.");
  }
}

static void emit_continue() {
  if (compiler->continue_jump_to == -1) {
    error("Invalid location for 'continue' to jump to.");
    return;
  }
  emit_jump_back(compiler->continue_jump_to);
  if (!compiler->in_continueable) {
    error("Cannot have 'continue' statement outside of loop.");
  }
}

static void patch_breaks() {
  for (int i = 0; i < compiler->breaks.size; i++) {
    patch_jump(compiler->breaks.values[i]);
  }
  lox_int_array_clear(&compiler->breaks);
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

  bool can_assign = precedence <= PREC_ASSIGNMENT;
  prefix_rule(can_assign);

  while (precedence <= get_parse_rule(parser.current.type)->precedence) {
    consume();
    lox_parse_function infix_rule = get_parse_rule(parser.previous.type)->infix;
    infix_rule(can_assign);
  }

  if (can_assign && match(TOKEN_EQUAL)) {
    error("Invalid assignment target.");
  }
}

static void declaration() {
  if (match(TOKEN_VAR) || match(TOKEN_CONST)) {
    variable_declaration();
  } else {
    statement();
  }

  if (parser.panic_mode) {
    synchronize();
  }
}

static void variable_declaration() {
  uint16_t global = parse_variable("Expected variable name.");

  if (match(TOKEN_EQUAL)) {
    expression();
  } else {
    emit_byte(OP_NIL);
  }

  consume_expected(TOKEN_SEMICOLON, "Expected ';' after variable declaration.");

  define_variable(global);
}

static void statement() {
  if (match(TOKEN_PRINT)) {
    print_statement();
  } else if (match(TOKEN_BREAK)) {
    break_statement();
  } else if (match(TOKEN_CONTINUE)) {
    continue_statement();
  } else if (match(TOKEN_SWITCH)) {
    switch_statement();
  } else if (match(TOKEN_FOR)) {
    for_statement();
  } else if (match(TOKEN_WHILE)) {
    while_statement();
  } else if (match(TOKEN_IF)) {
    if_statement();
  } else if (match(TOKEN_LEFT_BRACE)) {
    begin_scope();
    block();
    end_scope();
  } else {
    expression_statement();
  }
}

static void break_statement() {
  emit_break();
  consume_expected(TOKEN_SEMICOLON, "Expected ';' after break.");
}

static void continue_statement() {
  emit_continue();
  consume_expected(TOKEN_SEMICOLON, "Expected ';' after continue.");
}

static void switch_statement() {
  bool prevb = compiler->in_breakable;
  compiler->in_breakable = true;

  consume_expected(TOKEN_LEFT_PAREN, "Expected '(' after switch.");
  expression();
  consume_expected(TOKEN_RIGHT_PAREN, "Expected ')' after switch expression.");
  consume_expected(TOKEN_LEFT_BRACE, "Expected '{' after switch expression.");

  int default_start = -1;
  bool is_default_last = false;
  while (!check(TOKEN_RIGHT_BRACE)) {
    if (match(TOKEN_DEFAULT)) {
      if (default_start != -1)
        error("Cannot have more than one default case in switch.");

      default_start = current_chunk()->code.size;
      default_case();
      is_default_last = true;
    } else if (match(TOKEN_CASE)) {
      switch_case();
      is_default_last = false;
    } else {
      // This is supposed to be unreachable, as default_case and switch_case
      // look for either '}', 'case', or 'default' in order to stop the current
      // case, so we can't really write a test for this error.
      error("Expected 'default' or 'case' in switch statement.");
    }
  }
  if (default_start != -1 && !is_default_last) {
    emit_jump_back(default_start);
  }

  patch_breaks();
  emit_byte(OP_POP);
  consume_expected(TOKEN_RIGHT_BRACE, "Expected '}' after switch cases.");

  compiler->in_breakable = prevb;
}

static void switch_case() {
  emit_byte(OP_DUP);
  expression();
  consume_expected(TOKEN_COLON, "Expected ':' after case label.");
  emit_byte(OP_EQ);
  int jump = emit_jump(OP_JMP_FALSE);
  while (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) &&
         !check(TOKEN_RIGHT_BRACE)) {
    statement();
  }
  emit_byte(OP_POP);
  emit_break();
  patch_jump(jump);
  emit_byte(OP_POP);
}

static void default_case() {
  emit_byte(OP_DUP);
  consume_expected(TOKEN_COLON, "Expected ':' after case label.");
  while (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) &&
         !check(TOKEN_RIGHT_BRACE)) {
    statement();
  }
  emit_break();
}

static void for_statement() {
  bool prevb = compiler->in_breakable;
  bool prevc = compiler->in_continueable;
  int prevjmp = compiler->continue_jump_to;
  compiler->in_breakable = true;
  compiler->in_continueable = true;

  begin_scope();
  consume_expected(TOKEN_LEFT_PAREN, "Expected '(' after for.");
  // Initializer
  if (match(TOKEN_SEMICOLON)) {
  } else if (match(TOKEN_VAR)) {
    variable_declaration();
  } else {
    expression_statement();
  }

  int loop_start = current_chunk()->code.size;
  int exit_jump = -1;
  // Condition
  if (!match(TOKEN_SEMICOLON)) {
    expression();
    consume_expected(TOKEN_SEMICOLON, "Expected ';' after loop condition.");

    exit_jump = emit_jump(OP_JMP_FALSE);
    emit_byte(OP_POP);
  }
  // Increment
  if (!match(TOKEN_RIGHT_PAREN)) {
    int increment_jump = emit_jump(OP_JMP);
    int increment_start = current_chunk()->code.size;
    expression();
    consume_expected(TOKEN_RIGHT_PAREN, "Expected ')' after for loop clauses.");

    emit_byte(OP_POP);
    emit_jump_back(loop_start);
    loop_start = increment_start;
    patch_jump(increment_jump);
  }

  compiler->continue_jump_to = loop_start;
  statement();
  emit_jump_back(loop_start);

  if (exit_jump != -1) {
    patch_jump(exit_jump);
    emit_byte(OP_POP);
  }

  patch_breaks();
  end_scope();

  compiler->in_breakable = prevb;
  compiler->in_continueable = prevc;
  compiler->continue_jump_to = prevjmp;
}

static void while_statement() {
  bool prevb = compiler->in_breakable;
  bool prevc = compiler->in_continueable;
  compiler->in_breakable = true;
  compiler->in_continueable = true;

  consume_expected(TOKEN_LEFT_PAREN, "Expected '(' after while");

  // The loop starts over here because we have to re-evaluate the expression on
  // every iteration in case it changes.
  int loop_start = current_chunk()->code.size;
  expression();

  consume_expected(TOKEN_RIGHT_PAREN, "Expected ')' after expression");

  int end_jump = emit_jump(OP_JMP_FALSE);
  emit_byte(OP_POP);
  statement();
  emit_jump_back(loop_start);

  patch_jump(end_jump);
  emit_byte(OP_POP);

  compiler->in_breakable = prevb;
  compiler->in_continueable = prevc;
}

static void if_statement() {
  consume_expected(TOKEN_LEFT_PAREN, "Expected '(' after if");
  expression();
  consume_expected(TOKEN_RIGHT_PAREN, "Expected ')' after expression");

  int jump = emit_jump(OP_JMP_FALSE);
  emit_byte(OP_POP);
  statement();
  int end = emit_jump(OP_JMP);

  patch_jump(jump);
  emit_byte(OP_POP);
  if (match(TOKEN_ELSE))
    statement();

  patch_jump(end);
}

static void expression_statement() {
  expression();
  consume_expected(TOKEN_SEMICOLON, "Expected ';' after expression");
  emit_byte(OP_POP);
}

static void print_statement() {
  expression();
  consume_expected(TOKEN_SEMICOLON, "Expected ';' after expression");
  emit_byte(OP_PRINT);
}

static void block() {
  while (!check(TOKEN_RIGHT_BRACE) && !check(TOKEN_EOF)) {
    declaration();
  }

  consume_expected(TOKEN_RIGHT_BRACE, "Expected '}' after block.");
}

static void variable(bool can_assign) {
  named_variable(parser.previous, can_assign);
}

static void named_variable(lox_token name, bool can_assign) {
  uint8_t get, set;
  bool is_local = false;
  int arg = resolve_local(compiler, &name);
  if (arg != -1) {
    is_local = true;
  } else {
    arg = identifier_constant(&name, NULL);
  }

  bool is_const = false;
  if (is_local)
    is_const = compiler->locals.values[arg].is_constant;
  else
    is_const = lox_hash_table_has(&compiler->global_constants,
                                  lox_value_from_number(arg));

  if (can_assign && match(TOKEN_EQUAL)) {
    if (is_const) {
      error("Cannot re-assign const variable.");
    }

    expression();
    if (arg <= UINT8_MAX) {
      emit_bytes2(is_local ? OP_SET_LOCAL : OP_SET_GLOBAL, (uint8_t)arg);
    } else {
      emit_byte(OP_SET_GLOBAL_LONG);
      emit_short(arg);
    }
  } else {
    if (arg <= UINT8_MAX) {
      emit_bytes2(is_local ? OP_GET_LOCAL : OP_GET_GLOBAL, (uint8_t)arg);
    } else {
      emit_byte(OP_GET_GLOBAL_LONG);
      emit_short(arg);
    }
  }
}

static void expression() { parse_precedence(PREC_ASSIGNMENT); }

static void number(bool can_assign) {
  lox_value value = lox_value_from_number(strtod(parser.previous.start, NULL));
  emit_constant(value);
}

static void grouping(bool can_assign) {
  expression();
  consume_expected(TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
}

static void unary(bool can_assign) {
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

static void binary(bool can_assign) {
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
  case TOKEN_PERCENTAGE:
    emit_byte(OP_MODULO);
    break;
  default:
    return;
  }
}

static void literal(bool can_assign) {
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

static void string(bool can_assign) {
  lox_object_string *str = lox_object_string_new_copy(
      (char *)(parser.previous.start + 1), parser.previous.length - 2);
  emit_constant(lox_value_from_object((lox_object *)str));
}

static void compile_and(bool can_assign) {
  // a and b is equivalent to
  // if (a) {
  //  return b;
  // } else {
  //  return false;
  // }
  int jump = emit_jump(OP_JMP_FALSE);
  emit_byte(OP_POP);
  parse_precedence(PREC_AND);
  patch_jump(jump);
}

static void compile_or(bool can_assign) {
  // a or b is equivalent to
  // if (a) {
  //  return a;
  // } else {
  //  return b;
  // }
  int jump = emit_jump(OP_JMP_TRUE);
  emit_byte(OP_POP);
  parse_precedence(PREC_OR);
  patch_jump(jump);
}
