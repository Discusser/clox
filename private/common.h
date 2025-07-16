#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define LOX_MAX_LOCAL_COUNT UINT8_MAX + 1
#define LOX_ARRAY_MIN_CAPACITY 8
#define LOX_ARRAY_SCALE_FACTOR 2
#define LOX_HASH_TABLE_LOAD_FACTOR 0.75

#define LOX_OBJECT_STRING_FLAG_COPY 1
#define LOX_OBJECT_STRING_FLAG_CONSTANT 2
