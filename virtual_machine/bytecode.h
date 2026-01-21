#ifndef BYTECODE_H
#define BYTECODE_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
  const uint8_t *code;
  int code_size;       // Size of code section
  int entry_point;     // Main function
  int globals_count;   // Global count
  int *public_symbols; // Public functions and global variables
  int public_symbols_count;
  const char *string_table;
  int string_table_size;
  char **imports; // Imported modules
  int import_count;
  const uint8_t *subst_table; // Substitution table for imported symbols
  int subst_table_size;
  void *map_base; // Directly mapping a file into memory
  size_t map_size;
  char *module_name; // Module name derived from filename (e.g. "Lib" for
                     // /stdlib/Lib.bc);
} bytecode;

int read_i32(const uint8_t data[], int offset);
bytecode *load_bytecode(const char *filename);
void free_bytecode(bytecode *bc);

#endif
