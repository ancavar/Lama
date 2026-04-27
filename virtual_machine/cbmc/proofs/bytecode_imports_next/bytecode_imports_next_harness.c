#include <assert.h>

#include "bytecode.h"

void bytecode_imports_next_harness(void) {
  static const char strings[] = "Std\0Math\0";
  static const uint8_t imports[] = {
      0x00, 0x00, 0x00, 0x00,
      0x63, 0x00, 0x00, 0x00,
  };
  bytecode bc = {
      .string_table = strings,
      .string_table_size = sizeof(strings),
      .imports = imports,
      .imports_len = 2,
  };
  bytecode_iterator iter;
  const char *name = NULL;

  bytecode_imports_init(&iter, &bc);

  assert(iter.len == 2);
  assert(iter.curr == 0);
  assert(bytecode_imports_next(&iter, &name));
  assert(iter.curr == 1);
  assert(name == strings);

  assert(!bytecode_imports_next(&iter, &name));
  assert(iter.curr == 1);
}
