/*
 * Bytecode Merger for Lama VM.
 *
 * Merges multiple bytecode modules into a single bytecode structure with:
 * - All sections (string tables, code) concatenated and relocated
 * - All cross-references resolved
 * - List of all main() function entry points
 */

#ifndef BYTECODE_MERGER_H
#define BYTECODE_MERGER_H

#include "bytecode.h"
#include "module_manager.h"
#include <stddef.h>

/* Result of merging modules */
typedef struct {
  bytecode *bc;      // Merged bytecode
  int *main_entries; // main() entry point offsets
  int main_count;    // Number of main() entries
} merged_bytecode;

/*
 * Merge all modules in the list into a single bytecode file.
 *
 * Globals start at offset 1 (offset 0 reserved for sysargs/externs).
 * Duplicate public symbols (except main) are detected as errors.
 *
 * Caller is responsible for freeing with free_merged_bytecode().
 */
merged_bytecode *merge_modules(module_list *modules);

/*
 * Free all resources associated with merged bytecode.
 */
void free_merged_bytecode(merged_bytecode *merged);

#endif
