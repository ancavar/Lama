#include <assert.h>

#include "symbols.h"
#include "symbols_proof_helpers.h"

void symbol_table_empty_lookup_harness(void) {
  static const char name[] = "m";
  symbol_table *table = symbol_table_create();

  assert(table != NULL);
  assert(symbol_table_layout_is_valid(table));
  assert(table->len == 0);
  assert(symbol_table_find_function(table, name) == NULL);
  assert(symbol_table_find_global(table, name) == NULL);
}
