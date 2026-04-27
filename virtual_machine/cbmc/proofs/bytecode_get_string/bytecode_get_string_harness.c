#include <assert.h>

#include "bytecode.h"

void bytecode_get_string_harness(void) {
  static const char strings[] = "main\0glob\0";
  bytecode bc = {
      .string_table = strings,
      .string_table_size = sizeof(strings),
  };

  assert(bytecode_get_string(NULL, 0) == NULL);
  assert(bytecode_get_string(&bc, -1) == NULL);
  assert(bytecode_get_string(&bc, (int32_t)sizeof(strings)) == NULL);

  assert(bytecode_get_string(&bc, 0) == strings);
  assert(bytecode_get_string(&bc, 5) == strings + 5);
}
