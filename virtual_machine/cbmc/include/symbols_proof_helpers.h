#ifndef CBMC_SYMBOLS_PROOF_HELPERS_H
#define CBMC_SYMBOLS_PROOF_HELPERS_H

#include <stdbool.h>
#include <stddef.h>

#include "symbols.h"

struct symbol_table {
  resolved_symbol *data;
  size_t len;
  size_t cap;
};

static inline bool symbol_table_layout_is_valid(const symbol_table *table) {
  return table != NULL && table->len <= table->cap &&
         (table->cap == 0 ? table->data == NULL : table->data != NULL);
}

#endif
