#include <assert.h>
#include <stdint.h>

#include "symbols.h"
#include "symbols_proof_helpers.h"

int32_t nondet_int32_t(void);

void symbol_table_duplicate_harness(void) {
  static const char name[] = "d";
  int32_t first_idx = nondet_int32_t();
  int32_t second_idx = nondet_int32_t();
  symbol_table *table = symbol_table_create();

  assert(table != NULL);
  assert(symbol_table_layout_is_valid(table));
  assert(table->len == 0);
  assert(table->cap == 0);
  assert(table->data == NULL);

  assert(symbol_table_add_function(table, name, first_idx));
  assert(symbol_table_layout_is_valid(table));
  assert(table->len == 1);

  assert(!symbol_table_add_function(table, name, second_idx));
  assert(symbol_table_layout_is_valid(table));
  assert(table->len == 1);

  resolved_symbol *found_fn = symbol_table_find_function(table, name);
  resolved_symbol *found_glo = symbol_table_find_global(table, name);

  assert(found_fn != NULL);
  assert(found_fn == &table->data[0]);
  assert(found_fn->name == name);
  assert(found_fn->is_function);
  assert(found_fn->idx == first_idx);
  assert(found_glo == NULL);

  symbol_table_destroy(table);
}
