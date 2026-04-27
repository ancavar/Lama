#include <assert.h>
#include <stdint.h>

#include "reader.h"

uint8_t nondet_u8(void);
size_t nondet_size_t(void);

void reader_i32_harness(void) {
  uint8_t data[8];
  size_t size = nondet_size_t();
  size_t pos = nondet_size_t();
  byte_reader reader;

  data[0] = nondet_u8();
  data[1] = nondet_u8();
  data[2] = nondet_u8();
  data[3] = nondet_u8();
  data[4] = nondet_u8();
  data[5] = nondet_u8();
  data[6] = nondet_u8();
  data[7] = nondet_u8();

  __CPROVER_assume(size <= sizeof(data));
  __CPROVER_assume(pos <= size);
  __CPROVER_assume(size - pos >= 4);

  reader_init(&reader, data, size);
  reader.pos = pos;

  int32_t value = reader_i32(&reader);
  int32_t expected =
      (int32_t)(((uint32_t)data[0]) | ((uint32_t)data[1] << 8) |
                ((uint32_t)data[2] << 16) | ((uint32_t)data[3] << 24));

  assert(value == expected);
  assert(reader_pos(&reader) == pos + 4);
  assert(reader_eof(&reader) == (pos + 4 >= size));
}
