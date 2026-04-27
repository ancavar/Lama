#include <assert.h>
#include <stdint.h>

#include "symbols.h"
#include "symbols_proof_helpers.h"

int32_t nondet_int32_t(void);

void symbol_table_add_find_harness(void) {
  static const char name[] = "f";
  int32_t code_idx = nondet_int32_t();
  symbol_table *table = symbol_table_create();

  assert(table != NULL);
  assert(symbol_table_layout_is_valid(table));
  assert(table->len == 0);
  assert(table->cap == 0);
  assert(table->data == NULL);

  assert(symbol_table_add_function(table, name, code_idx));
  assert(symbol_table_layout_is_valid(table));
  assert(table->len == 1);

  resolved_symbol *found_fn = symbol_table_find_function(table, name);
  resolved_symbol *found_glo = symbol_table_find_global(table, name);

  assert(found_fn != NULL);
  assert(found_fn == &table->data[0]);
  assert(found_fn->name == name);
  assert(found_fn->is_function);
  assert(found_fn->idx == code_idx);
  assert(found_glo == NULL);

  symbol_table_destroy(table);
}
