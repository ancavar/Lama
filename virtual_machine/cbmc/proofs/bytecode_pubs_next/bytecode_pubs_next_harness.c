#include <assert.h>

#include "bytecode.h"

void bytecode_pubs_next_harness(void) {
  static const char strings[] = "main\0glob\0";
  static const uint8_t pubs[] = {
      0x00, 0x00, 0x00, 0x00,
      0x2A, 0x00, 0x00, 0x00,
      PUB_FLAG_FUNCTION,
      0x63, 0x00, 0x00, 0x00,
      0x07, 0x00, 0x00, 0x00,
      PUB_FLAG_GLOBAL,
  };
  bytecode bc = {
      .string_table = strings,
      .string_table_size = sizeof(strings),
      .pubs = pubs,
      .pubs_len = 2,
  };
  bytecode_iterator iter;
  public_symbol out = {0};

  bytecode_pubs_init(&iter, &bc);

  assert(iter.len == 2);
  assert(iter.curr == 0);
  assert(bytecode_pubs_next(&iter, &out));
  assert(iter.curr == 1);
  assert(out.name == strings);
  assert(out.code_offset == 42);
  assert(out.flag == PUB_FLAG_FUNCTION);

  assert(!bytecode_pubs_next(&iter, &out));
  assert(iter.curr == 1);
}
