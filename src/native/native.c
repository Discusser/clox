#include "native/native.h"
#include <time.h>

void define_natives(lox_vm *vm) {
  define_native(vm, "clock", clock_native, 0);
  define_native(vm, "hasProperty", hasProperty_native, 2);
  define_native(vm, "getProperty", getProperty_native, 2);
  define_native(vm, "setProperty", setProperty_native, 3);
  define_native(vm, "removeProperty", removeProperty_native, 2);
}

lox_value clock_native(int argc, lox_value *argv) {
  return lox_value_from_number((double)clock() / CLOCKS_PER_SEC);
}

lox_value hasProperty_native(int argc, lox_value *argv) {
  if (argc != 2 || !lox_value_is_instance(argv[0]) ||
      !lox_value_is_string(argv[1]))
    return lox_value_from_bool(false);

  lox_object_instance *inst = (lox_object_instance *)argv[0].as.object;
  lox_value name = argv[1];
  return lox_value_from_bool(lox_hash_table_has(inst->fields, name));
}

lox_value getProperty_native(int argc, lox_value *argv) {
  if (argc != 2 || !lox_value_is_instance(argv[0]) ||
      !lox_value_is_string(argv[1]))
    return lox_value_from_nil();

  lox_object_instance *inst = (lox_object_instance *)argv[0].as.object;
  lox_value name = argv[1];
  lox_value val;
  if (!lox_hash_table_get(inst->fields, name, &val)) {
    return lox_value_from_nil();
  }
  return val;
}

lox_value setProperty_native(int argc, lox_value *argv) {
  if (argc != 3 || !lox_value_is_instance(argv[0]) ||
      !lox_value_is_string(argv[1]))
    return lox_value_from_nil();

  lox_object_instance *inst = (lox_object_instance *)argv[0].as.object;
  lox_value name = argv[1];
  lox_value val = argv[2];
  lox_hash_table_put(inst->fields, name, val);
  return argv[2];
}

lox_value removeProperty_native(int argc, lox_value *argv) {
  if (argc != 2 || !lox_value_is_instance(argv[0]) ||
      !lox_value_is_string(argv[1]))
    return lox_value_from_bool(false);

  lox_object_instance *inst = (lox_object_instance *)argv[0].as.object;
  lox_value name = argv[1];
  return lox_value_from_bool(lox_hash_table_remove(inst->fields, name));
}
