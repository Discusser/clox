#include "vm.h"
#include "debug.h"
#include "memory.h"
#include "native/native.h"
#include "object.h"
#include "value.h"
#include "chunk.h"
#include "compiler.h"
#include <assert.h>
#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void free_objects();

static void reset_stack();
static lox_value *peek(int n);
static bool call_value(lox_value value, int arg_count);
static bool call_closure(lox_object_closure *closure, int arg_count);
static lox_object_upvalue *capture_upvalue(lox_value *local);
static void close_upvalues(lox_value *last);
static void define_method(lox_value name);
static bool invoke(lox_value name, int argc);
static bool invoke_from_class(lox_object_class *clazz, lox_value name,
                              int argc);
static bool bind_method(lox_object_class *clazz, lox_value name);
static void print_settings();

lox_vm vm;

void init_vm() {
#ifdef DEBUG_PRINT_SETTINGS
  print_settings();
#endif

  vm.bytes_allocated = 0;
  vm.next_gc = 1024 * 1024;
  lox_value_array_initialize(&vm.stack);
  lox_value_array_resize(&vm.stack, LOX_INITIAL_STACK_SIZE);
  lox_hash_table_init(&vm.strings);
  lox_hash_table_init(&vm.global_indices);
  lox_value_array_initialize(&vm.globals);
#ifndef NDEBUG
  lox_hash_table_init(&vm.global_names);
  lox_hash_table_init(&vm.local_names);
#endif
  vm.objects = NULL;
  vm.open_upvalues = NULL;
  vm.gray_capacity = 0;
  vm.gray_size = 0;
  vm.gray_stack = NULL;
  vm.frame_count = 0;
  vm.mark_value = true;
  vm.init_string = lox_value_from_object(
      (lox_object *)lox_object_string_new_copy("init", 4));
  reset_stack();

  define_natives(&vm);
}

void free_vm() {
  lox_value_array_free(&vm.stack);
  lox_hash_table_free(&vm.strings);
  lox_hash_table_free(&vm.global_indices);
  lox_value_array_free(&vm.globals);
#ifndef NDEBUG
  lox_hash_table_free(&vm.global_names);
  lox_hash_table_free(&vm.local_names);
#endif
  free_objects();
  free(vm.gray_stack);
}

static void free_objects() {
  lox_object *curr = vm.objects;
  while (curr != NULL) {
    lox_object *tmp = curr->next;
    lox_object_free(curr);
    curr = tmp;
  }
}

static void reset_stack() {
  vm.stack.size = 0;
  vm.frame_count = 0;
}

inline void push(lox_value value) { lox_value_array_push(&vm.stack, value); }

inline lox_value pop() { return lox_value_array_pop(&vm.stack); }

static inline lox_value *peek(int n) {
  return &vm.stack.values[vm.stack.size - 1 - n];
}

static inline lox_value peekv(int n) {
  return vm.stack.values[vm.stack.size - 1 - n];
}

static bool call_value(lox_value value, int arg_count) {
  if (lox_value_is_object(value)) {
    switch (value.as.object->type) {
    case OBJ_CLASS: {
      lox_object_class *clazz = (lox_object_class *)value.as.object;
      vm.stack.values[vm.stack.size - arg_count - 1] =
          lox_value_from_object((lox_object *)lox_object_instance_new(clazz));
      lox_value initializer;
      if (lox_hash_table_get(clazz->methods, vm.init_string, &initializer)) {
        return call_closure((lox_object_closure *)initializer.as.object,
                            arg_count);
      } else if (arg_count != 0) {
        runtime_error(
            "Expected 0 arguments to class initializer, found %i instead.",
            arg_count);
        return false;
      }
      return true;
    }
    case OBJ_CLOSURE: {
      return call_closure((lox_object_closure *)value.as.object, arg_count);
    }
    case OBJ_NATIVE: {
      lox_object_native *native = ((lox_object_native *)value.as.object);
      if (arg_count != native->arity) {
        runtime_error(
            "Native function '%s' expected %i arguments, found %i instead.",
            native->name, native->arity, arg_count);
        return false;
      }
      lox_value ret = native->function(arg_count, peek(arg_count - 1));
      vm.stack.size -= (arg_count + 1);
      push(ret);
      return true;
    }
    case OBJ_BOUND_METHOD: {
      lox_object_bound_method *bound =
          (lox_object_bound_method *)value.as.object;
      vm.stack.values[vm.stack.size - arg_count - 1] = bound->receiver;
      return call_closure(bound->method, arg_count);
    }
    default:
      break;
    }
  }

  runtime_error("Object is not callable.");
  return false;
}

static bool call_closure(lox_object_closure *closure, int arg_count) {
  lox_object_function *fun = closure->function;
  if (arg_count != fun->arity) {
    runtime_error("Function '%s' expected %i arguments, found %i instead.",
                  fun->name->chars, fun->arity, arg_count);
    return false;
  }

  if (vm.frame_count >= LOX_MAX_CALL_FRAMES) {
    runtime_error("Stack overflow. Cannot have more than %i call frames.",
                  LOX_MAX_CALL_FRAMES);
    return false;
  }

  lox_call_frame *frame = &vm.frames[vm.frame_count++];
  frame->closure = closure;
  frame->ip = fun->chunk.code.values;
  frame->slots_offset = vm.stack.size - arg_count - 1;
  return true;
}

static lox_object_upvalue *capture_upvalue(lox_value *local) {
  lox_object_upvalue *previous_upvalue = NULL;
  lox_object_upvalue *val = vm.open_upvalues;
  while (val != NULL && val->location > local) {
    previous_upvalue = val;
    val = val->next;
  }

  if (val != NULL && val->location == local)
    return val;

  lox_object_upvalue *upvalue = lox_object_upvalue_new(local);
  upvalue->next = val;
  if (previous_upvalue == NULL) {
    vm.open_upvalues = upvalue;
  } else {
    previous_upvalue->next = upvalue;
  }
  return upvalue;
}

static void close_upvalues(lox_value *last) {
  while (vm.open_upvalues != NULL && vm.open_upvalues->location >= last) {
    lox_object_upvalue *upvalue = vm.open_upvalues;
    upvalue->closed = *upvalue->location;
    upvalue->location = &upvalue->closed;
    vm.open_upvalues = upvalue->next;
  }
}

bool lox_is_falsey(lox_value value) {
  return value.type == VAL_NIL ||
         (value.type == VAL_BOOL && value.as.boolean == false);
}

bool lox_values_equal(lox_value lhs, lox_value rhs) {
  if (lhs.type != rhs.type)
    return false;
  switch (lhs.type) {
  case VAL_BOOL:
    return lhs.as.boolean == rhs.as.boolean;
  case VAL_NIL:
    return true;
  case VAL_NUMBER:
    return lhs.as.number == rhs.as.number;
  case VAL_OBJECT: {
    return lhs.as.object == rhs.as.object;
  }
  default:
    return false;
  }
}

interpret_result interpret(const char *source) {
  lox_object_function *function = lox_compiler_compile(source);
  if (function == NULL)
    return INTERPRET_COMPILE_ERROR;

  push(lox_value_from_object((lox_object *)function));
  lox_object_closure *closure = lox_object_closure_new(function);
  pop();
  push(lox_value_from_object((lox_object *)closure));
  call_closure(closure, 0);

  return run();
}

interpret_result run() {
  lox_call_frame *frame = &vm.frames[vm.frame_count - 1];
  // We store this in a register because this variable is used very frequently
  // from many instructions. Since we're caching the value, we have to update it
  // every time the frame changes, that is, every time we push or pop a frame,
  // which usually means every time we call a function or method, or return from
  // one.
  register uint8_t *ip = frame->ip;

#define READ_BYTE() (*ip++)
#define READ_SHORT() (ip += 2, (uint16_t)(ip[-2] << 8 | ip[-1]))
#define READ_CONST()                                                           \
  (frame->closure->function->chunk.constants.values[READ_BYTE()])
#define READ_CONST_LONG()                                                      \
  (frame->closure->function->chunk.constants.values[READ_SHORT()])
#define RUNTIME_ERROR(fmt)                                                     \
  do {                                                                         \
    frame->ip = ip;                                                            \
    runtime_error(fmt);                                                        \
    return INTERPRET_RUNTIME_ERROR;                                            \
  } while (0)
#define RUNTIME_ERROR_ARGS(fmt, ...)                                           \
  do {                                                                         \
    frame->ip = ip;                                                            \
    runtime_error(fmt, __VA_ARGS__);                                           \
    return INTERPRET_RUNTIME_ERROR;                                            \
  } while (0)

#define BINARY_OP(make_value, op)                                              \
  do {                                                                         \
    lox_value rhs = peekv(0);                                                  \
    lox_value lhs = peekv(1);                                                  \
    if (lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER) {                    \
      vm.stack.values[vm.stack.size - 2] =                                     \
          make_value(lhs.as.number op rhs.as.number);                          \
      vm.stack.size--;                                                         \
    } else {                                                                   \
      RUNTIME_ERROR("Operands must be numbers for '" #op "'.");                \
    }                                                                          \
  } while (false)

  for (;;) {
#ifdef DEBUG_TRACE_EXECUTION
    printf("%-10s", "STACK");
    for (int i = 0; i < vm.stack.size; i++) {
      printf("[ ");
      lox_print_value(vm.stack.values[i]);
      printf(" ]");
    }
    printf("\n");
    lox_disassemble_instruction(
        &frame->closure->function->chunk,
        ip - frame->closure->function->chunk.code.values);
#endif

    uint8_t instruction;
    switch (instruction = READ_BYTE()) {
    case OP_RETURN: {
      close_upvalues(vm.stack.values + frame->slots_offset);
      vm.frame_count--;
      // This indicates the end of the program
      if (vm.frame_count == 0) {
        pop();
        return INTERPRET_OK;
      }
      vm.stack.values[frame->slots_offset] = vm.stack.values[vm.stack.size - 1];
      vm.stack.size = frame->slots_offset + 1;
      frame = &vm.frames[vm.frame_count - 1];
      ip = frame->ip;
      break;
    }
    case OP_NIL:
      push(lox_value_from_nil());
      break;
    case OP_TRUE:
      push(lox_value_from_bool(true));
      break;
    case OP_FALSE:
      push(lox_value_from_bool(false));
      break;
    case OP_EQ: {
      lox_value rhs = peekv(0);
      lox_value lhs = peekv(1);
      vm.stack.values[vm.stack.size - 2] =
          lox_value_from_bool(lox_values_equal(rhs, lhs));
      vm.stack.size--;
      break;
    }
    case OP_NEQ: {
      lox_value rhs = peekv(0);
      lox_value lhs = peekv(1);
      vm.stack.values[vm.stack.size - 2] =
          lox_value_from_bool(!lox_values_equal(rhs, lhs));
      vm.stack.size--;
      break;
    }
    case OP_GREATER:
      BINARY_OP(lox_value_from_bool, >);
      break;
    case OP_GREATEREQ:
      BINARY_OP(lox_value_from_bool, >=);
      break;
    case OP_LESS:
      BINARY_OP(lox_value_from_bool, <);
      break;
    case OP_LESSEQ:
      BINARY_OP(lox_value_from_bool, <=);
      break;
    case OP_NEGATE: {
      lox_value value = peekv(0);
      if (value.type == VAL_NUMBER) {
        vm.stack.values[vm.stack.size - 1] =
            lox_value_from_number(-value.as.number);
      } else {
        RUNTIME_ERROR("Operand must be a number.");
      }
      break;
    }
    case OP_NOT: {
      lox_value value = peekv(0);
      bool f = lox_is_falsey(value);
      vm.stack.values[vm.stack.size - 1] = lox_value_from_bool(f);
      break;
    }
    case OP_ADD: {
      lox_value rhs = peekv(0);
      lox_value lhs = peekv(1);
      if (lox_value_is_string(lhs) && lox_value_is_string(rhs)) {
        lox_object_string *rstr = (lox_object_string *)(rhs.as.object);
        lox_object_string *lstr = (lox_object_string *)(lhs.as.object);
        int length = lstr->length + rstr->length;
        char *chars = ALLOC_ARRAY(char, length + 1);
        memcpy(chars, lstr->chars, lstr->length);
        memcpy(chars + lstr->length, rstr->chars, rstr->length);
        chars[length] = '\0';
        vm.stack.values[vm.stack.size - 2] = lox_value_from_object(
            (lox_object *)lox_object_string_new_consume(chars, length, false));
        vm.stack.size--;
      } else if (lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER) {
        vm.stack.values[vm.stack.size - 2] =
            lox_value_from_number(lhs.as.number + rhs.as.number);
        vm.stack.size--;
      } else {
        RUNTIME_ERROR("Operands must be numbers or strings.");
      }
      break;
    }
    case OP_SUBTRACT:
      BINARY_OP(lox_value_from_number, -);
      break;
    case OP_MULTIPLY:
      BINARY_OP(lox_value_from_number, *);
      break;
    case OP_DIVIDE:
      do {
        lox_value rhs = peekv(0);
        lox_value lhs = peekv(1);
        if (lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER) {
          if (rhs.as.number == 0) {
            RUNTIME_ERROR("Cannot divide by zero.");
          }
          vm.stack.values[vm.stack.size - 2] =
              lox_value_from_number(lhs.as.number / rhs.as.number);
          vm.stack.size--;
        } else {
          RUNTIME_ERROR("Operands must be numbers.");
        }
      } while (0);
      break;
    case OP_MODULO:
      do {
        lox_value rhs = peekv(0);
        lox_value lhs = peekv(1);
        if (lhs.type == VAL_NUMBER && rhs.type == VAL_NUMBER) {
          vm.stack.values[vm.stack.size - 2] =
              lox_value_from_number(fmod(lhs.as.number, rhs.as.number));
          vm.stack.size--;
        } else {
          RUNTIME_ERROR("Operands must be numbers.");
        }
      } while (0);
      break;
    case OP_PRINT:
      lox_print_value(pop());
      printf("\n");
      break;
    case OP_POP:
      vm.stack.size--;
      break;
    case OP_POPN:
      vm.stack.size -= READ_SHORT();
      break;
    case OP_CONSTANT: {
      push(READ_CONST());
      break;
    }
    case OP_CONSTANT_LONG: {
      push(READ_CONST_LONG());
      break;
    }
    case OP_GET_GLOBAL_LONG:
    case OP_GET_GLOBAL: {
      int index = instruction == OP_GET_GLOBAL ? READ_BYTE() : READ_SHORT();
      assert(index < vm.globals.size);

      lox_value value = vm.globals.values[index];
      if (value.type == VAL_EMPTY) {
        RUNTIME_ERROR_ARGS(
            "Undefined variable '%s'.",
            ((lox_object_string *)lox_get_global_name(index).as.object)->chars);
      }

      push(value);
      break;
    }
    case OP_DEFINE_GLOBAL_LONG:
    case OP_DEFINE_GLOBAL: {
      vm.globals.values[instruction == OP_DEFINE_GLOBAL ? READ_BYTE()
                                                        : READ_SHORT()] = pop();
      break;
    }
    case OP_SET_GLOBAL_LONG:
    case OP_SET_GLOBAL: {
      int index = instruction == OP_SET_GLOBAL ? READ_BYTE() : READ_SHORT();
      assert(index < vm.globals.size);

      lox_value value = vm.globals.values[index];
      if (value.type == VAL_EMPTY) {
        RUNTIME_ERROR_ARGS(
            "Undefined variable '%s'.",
            ((lox_object_string *)lox_get_global_name(index).as.object)->chars);
      }

      // We don't pop the value because this is an expression, so it must return
      // a value, which is in this case the value we give to the global, so we
      // might aswell not touch the stack.
      vm.globals.values[index] = *peek(0);
      break;
    }
    case OP_GET_LOCAL: {
      uint8_t slot = READ_BYTE();
      push(vm.stack.values[slot + frame->slots_offset]);
      break;
    }
    case OP_SET_LOCAL: {
      uint8_t slot = READ_BYTE();
      vm.stack.values[slot + frame->slots_offset] = *peek(0);
      break;
    }
    case OP_GET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      push(*frame->closure->upvalues[slot]->location);
      break;
    }
    case OP_SET_UPVALUE: {
      uint8_t slot = READ_BYTE();
      // We don't want to change the pointer held by the current closure, since
      // that will disallow sharing the upvalue between closures. Instead, we
      // modify the value the pointer points to
      *frame->closure->upvalues[slot]->location = peekv(0);
      break;
    }
    case OP_CLOSE_UPVALUE: {
      close_upvalues(vm.stack.values + vm.stack.size - 1);
      vm.stack.size--;
      break;
    }
    case OP_JMP_TRUE: {
      uint16_t offset = READ_SHORT();
      if (!lox_is_falsey(peekv(0)))
        ip += offset;
      break;
    }
    case OP_JMP_FALSE: {
      uint16_t offset = READ_SHORT();
      if (lox_is_falsey(peekv(0)))
        ip += offset;
      break;
    }
    case OP_JMP: {
      uint16_t offset = READ_SHORT();
      ip += offset;
      break;
    }
    case OP_JMP_BACK: {
      uint16_t offset = READ_SHORT();
      ip -= offset;
      break;
    }
    case OP_DUP:
      push(peekv(0));
      break;
    case OP_CALL: {
      int arg_count = READ_BYTE();
      frame->ip = ip;
      lox_value value = *peek(arg_count);
      if (!call_value(value, arg_count)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      // If the function call was successful, update the call frame we read
      // from.
      frame = &vm.frames[vm.frame_count - 1];
      ip = frame->ip;
      break;
    }
    case OP_CLOSURE: {
      lox_object_function *fun =
          (lox_object_function *)(READ_CONST_LONG().as.object);
      lox_object_closure *closure = lox_object_closure_new(fun);
      for (int i = 0; i < closure->upvalue_count; i++) {
        uint8_t is_local = READ_BYTE();
        uint16_t index = READ_SHORT();
        if (is_local) {
          closure->upvalues[i] =
              capture_upvalue(vm.stack.values + frame->slots_offset + index);
        } else {
          closure->upvalues[i] = frame->closure->upvalues[index];
        }
      }
      push(lox_value_from_object((lox_object *)closure));
      break;
    }
    case OP_CLASS: {
      // NOTE: By doing this, we consider that classes are always globals, even
      // though they can be defined as locals (inside of a class or inside of a
      // function).
      lox_object_string *name = (lox_object_string *)READ_CONST().as.object;
      lox_object_class *clazz = lox_object_class_new(name);
      push(lox_value_from_object((lox_object *)clazz));
      break;
    }
    case OP_SET_PROPERTY: {
      lox_value top = peekv(1);
      if (!lox_value_is_instance(top)) {
        RUNTIME_ERROR("Cannot set property on object that isn't an instance.");
      }

      lox_object_instance *instance = (lox_object_instance *)top.as.object;
      lox_value name = READ_CONST();
      lox_value val = peekv(0);
      lox_hash_table_put(instance->fields, name, val);
      // Pop the value, then the instance, and then push the value
      vm.stack.size--;
      vm.stack.values[vm.stack.size - 1] = val;
      break;
    }
    case OP_GET_PROPERTY: {
      lox_value top = peekv(0);
      if (!lox_value_is_instance(top)) {
        RUNTIME_ERROR("Cannot get property on object that isn't an instance.");
      }

      lox_object_instance *instance = (lox_object_instance *)top.as.object;
      lox_value name = READ_CONST();
      lox_value val;

      // First we check if a field with the name exists, then if nothing was
      // found we look for a method.
      if (lox_hash_table_get(instance->fields, name, &val)) {
        // Pop the instance, and push the value
        vm.stack.values[vm.stack.size - 1] = val;
      } else {
        if (!bind_method(instance->clazz, name)) {
          return INTERPRET_RUNTIME_ERROR;
        }
      }

      break;
    }
    case OP_METHOD:
      define_method(READ_CONST());
      break;
    case OP_INVOKE: {
      lox_value name = READ_CONST();
      uint8_t argc = READ_BYTE();
      frame->ip = ip;
      if (!invoke(name, argc)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      ip = frame->ip;
      break;
    }
    case OP_INHERIT: {
      lox_value super = peekv(1);
      if (!lox_value_is_class(super)) {
        RUNTIME_ERROR("Cannot inherit from object that is not a class.");
        return INTERPRET_RUNTIME_ERROR;
      }
      lox_object_class *superclass = (lox_object_class *)super.as.object;
      lox_object_class *child = (lox_object_class *)peekv(0).as.object;
      lox_hash_table_copy_to(superclass->methods, child->methods);
      vm.stack.size--;
      break;
    }
    case OP_GET_SUPER: {
      lox_object_instance *inst_this =
          (lox_object_instance *)peekv(1).as.object;
      lox_object_class *class_super = (lox_object_class *)peekv(0).as.object;
      lox_value name = READ_CONST();
      lox_value val;

      if (!bind_method(class_super, name)) {
        return INTERPRET_RUNTIME_ERROR;
      }

      break;
    }
    case OP_SUPER_INVOKE: {
      lox_value name = READ_CONST();
      uint8_t argc = READ_BYTE();
      lox_object_class *class_super =
          (lox_object_class *)vm.stack.values[--vm.stack.size].as.object;

      frame->ip = ip;
      if (!invoke_from_class(class_super, name, argc)) {
        return INTERPRET_RUNTIME_ERROR;
      }
      frame = &vm.frames[vm.frame_count - 1];
      ip = frame->ip;
      break;
    }
    default:
      printf("Unknown instruction %i\n", instruction);
      return INTERPRET_RUNTIME_ERROR;
    }
  }

#undef RUNTIME_ERROR_ARGS
#undef RUNTIME_ERROR
#undef READ_CONST_LONG
#undef READ_CONST
#undef READ_SHORT
#undef READ_BYTE
#undef BINARY_OP
#undef CONST_OP
}

void runtime_error(const char *format, ...) {
  fprintf(stderr, "Runtime Error: ");
  va_list args;
  va_start(args, format);
  vfprintf(stderr, format, args);
  va_end(args);
  fputc('\n', stderr);

  fprintf(stderr, "Stacktrace:\n");
  for (int i = vm.frame_count - 1; i >= 0; i--) {
    lox_call_frame *frame = &vm.frames[i];
    lox_object_function *fun = frame->closure->function;
    size_t instruction = frame->ip - fun->chunk.code.values - 2;
    fprintf(stderr, "  line %d in ",
            lox_chunk_get_offset_line(&fun->chunk, instruction) + 1);
    if (fun->name == NULL) {
      fprintf(stderr, "script");
    } else {
      fprintf(stderr, "%s()", fun->name->chars);
    }
    fprintf(stderr, "\n");
  }

  reset_stack();
}

void define_native(lox_vm *vm, const char *name, lox_native_function function,
                   int arity) {
  push(lox_value_from_object(
      (lox_object *)lox_object_string_new_copy(name, strlen(name))));
  push(lox_value_from_object(
      (lox_object *)lox_object_native_new(name, function, arity)));

  lox_value key = vm->stack.values[0];
  lox_value value = vm->stack.values[1];
  uint16_t index = vm->globals.size;
  lox_value num = lox_value_from_number(index);
  lox_value_array_push(&vm->globals, value);
  lox_hash_table_put(&vm->global_indices, key, num);
#ifndef NDEBUG
  lox_hash_table_put(&vm->global_names, num, key);
#endif

  pop();
  pop();
}

static void define_method(lox_value name) {
  lox_value method = peekv(0);
  lox_object_class *clazz = (lox_object_class *)peekv(1).as.object;
  lox_hash_table_put(clazz->methods, name, method);
  // Pop the method off the stack
  vm.stack.size--;
}

static bool invoke(lox_value name, int argc) {
  lox_value receiver = peekv(argc);
  if (!lox_value_is_instance(receiver)) {
    runtime_error("Cannot invoke method on object that isn't an instance.");
    return false;
  }

  lox_object_instance *inst = (lox_object_instance *)receiver.as.object;
  lox_value value;
  if (lox_hash_table_get(inst->fields, name, &value)) {
    vm.stack.values[vm.stack.size - argc - 1] = value;
    call_value(value, argc);
  }

  return invoke_from_class(inst->clazz, name, argc);
}

static bool invoke_from_class(lox_object_class *clazz, lox_value name,
                              int argc) {
  lox_value method;
  if (!lox_hash_table_get(clazz->methods, name, &method)) {
    runtime_error("Undefined property '%s' in instance of '%s'.",
                  ((lox_object_string *)name.as.object)->chars,
                  clazz->name->chars);
    return false;
  }

  return call_closure((lox_object_closure *)method.as.object, argc);
}

static bool bind_method(lox_object_class *clazz, lox_value name) {
  lox_value method;
  if (!lox_hash_table_get(clazz->methods, name, &method)) {
    // This function expects an instance to be sitting on top of the stack, so
    // we can say instance in the error messages aswell despite methods being
    // defined in classes.
    runtime_error("Undefined property '%s' in instance of '%s'.",
                  ((lox_object_string *)name.as.object)->chars,
                  clazz->name->chars);
    return false;
  }

  lox_object_bound_method *bound = lox_object_bound_method_new(
      peekv(0), (lox_object_closure *)method.as.object);
  // Pop the instance off the top of the stack and replace it with the bound
  // method
  vm.stack.values[vm.stack.size - 1] =
      lox_value_from_object((lox_object *)bound);

  return true;
}

static void print_settings() {
#define SETI(name) printf("  " #name "=%i\n", lox_settings.name)
#define SETF(name) printf("  " #name "=%f\n", lox_settings.name)

  printf("Lox Settings:\n");
  SETI(array_minimum_capacity);
  SETI(array_scale_factor);
  SETI(gc_heap_grow_factor);
  SETF(hash_table_load_factor);
  SETI(initial_stack_size);
  SETI(max_local_count);

#undef SETF
#undef SETI
}
