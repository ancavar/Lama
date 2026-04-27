#include <assert.h>
#include <stdint.h>

#include "reader.h"

uint8_t nondet_u8(void);

void reader_u8_harness(void) {
  uint8_t data[1];
  byte_reader reader;

  data[0] = nondet_u8();

  reader_init(&reader, data, sizeof(data));

  assert(reader_pos(&reader) == 0);
  assert(!reader_eof(&reader));

  uint8_t value = reader_u8(&reader);

  assert(value == data[0]);
  assert(reader_pos(&reader) == sizeof(data));
  assert(reader_eof(&reader));
}
