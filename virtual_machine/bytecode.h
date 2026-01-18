#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
  const uint8_t *code;
  int code_size;
  int entry_point;
  int globals_count;
  int *public_symbols;
  int public_symbols_count;
  const char *string_table;
  int string_table_size;
  /* Import information */
  char **imports;
  int import_count;
  /* Memory management */
  void *map_base;
  size_t map_size;
  /* Module name (derived from filename) */
  char *module_name;
} bytecode;

int read_i32(const uint8_t data[], int offset);
bytecode *load_bytecode(const char *filename);
void free_bytecode(bytecode *bc);

/* Helper to get public symbol name */
const char *get_public_name(bytecode *bc, int index);
/* Helper to get public symbol offset */
int get_public_offset(bytecode *bc, int index);

#endif
