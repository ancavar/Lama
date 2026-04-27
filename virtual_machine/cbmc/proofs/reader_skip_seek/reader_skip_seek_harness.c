#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "reader.h"

size_t nondet_size_t(void);

void reader_skip_seek_harness(void) {
  uint8_t data[8];
  size_t size = nondet_size_t();
  size_t pos = nondet_size_t();
  size_t skip = nondet_size_t();
  size_t seek = nondet_size_t();
  byte_reader reader;

  __CPROVER_assume(size <= sizeof(data));
  __CPROVER_assume(pos <= size);

  reader_init(&reader, data, size);
  reader.pos = pos;

  assert(reader_pos(&reader) == pos);
  assert(reader_eof(&reader) == (pos == size));

  reader_skip(&reader, skip);

  size_t expected_after_skip = skip >= size - pos ? size : pos + skip;
  assert(reader_pos(&reader) == expected_after_skip);
  assert(reader_eof(&reader) == (expected_after_skip == size));

  reader_seek(&reader, seek);

  size_t expected_after_seek = seek > size ? size : seek;
  assert(reader_pos(&reader) == expected_after_seek);
  assert(reader_eof(&reader) == (expected_after_seek == size));
}
