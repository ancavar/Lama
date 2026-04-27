#include <assert.h>
#include <stdint.h>

#include "symbols.h"
#include "symbols_proof_helpers.h"

int32_t nondet_int32_t(void);

void symbol_table_namespace_harness(void) {
  static const char name[] = "s";
  int32_t code_idx = nondet_int32_t();
  int32_t global_idx = nondet_int32_t();
  symbol_table *table = symbol_table_create();

  assert(table != NULL);
  assert(symbol_table_layout_is_valid(table));
  assert(symbol_table_add_function(table, name, code_idx));
  assert(symbol_table_add_global(table, name, global_idx));
  assert(symbol_table_layout_is_valid(table));
  assert(table->len == 2);

  resolved_symbol *found_fn = symbol_table_find_function(table, name);
  resolved_symbol *found_glo = symbol_table_find_global(table, name);

  assert(found_fn != NULL);
  assert(found_glo != NULL);
  assert(found_fn != found_glo);

  assert(found_fn == &table->data[0]);
  assert(found_fn->name == name);
  assert(found_fn->is_function);
  assert(found_fn->idx == code_idx);

  assert(found_glo == &table->data[1]);
  assert(found_glo->name == name);
  assert(!found_glo->is_function);
  assert(found_glo->idx == global_idx);

  symbol_table_destroy(table);
}
