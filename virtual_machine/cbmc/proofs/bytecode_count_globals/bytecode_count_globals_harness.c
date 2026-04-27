#include <assert.h>

#include "bytecode.h"

void bytecode_count_globals_harness(void) {
  bytecode bc0 = {.globals_count = 0};
  bytecode bc1 = {.globals_count = 3};
  bytecode bc2 = {.globals_count = 7};
  bytecode *units[] = {&bc0, &bc1, &bc2};

  assert(bytecode_count_globals(units, 0) == 0);
  assert(bytecode_count_globals(units, 1) == 0);
  assert(bytecode_count_globals(units, 2) == 3);
  assert(bytecode_count_globals(units, 3) == 10);
}
