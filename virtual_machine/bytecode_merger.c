/*
 * Bytecode Merger for Lama VM.
 *
 */

#include "bytecode_merger.h"
#include "opcodes.h"
#include "util.h"
#include "verifier.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
  char *name;
  char *module_name; // Module that defined this symbol
  int final_value;   // Final offset (code) or index (global)
} symbol_entry;

// TODO: hashmap?
typedef struct {
  symbol_entry *data;
  size_t len;
  size_t cap;
} symbol_table;

typedef struct {
  int string_table_base; // Base offset into merged string table
  int code_base;         // Base offset into merged code section
  int global_base;       // Base index for globals (1-indexed, 0 reserved)
} module_relocation;

static void symbol_table_init(symbol_table *table) { da_init((*table)); }

static void symbol_table_free(symbol_table *table) {
  for (size_t i = 0; i < table->len; i++) {
    free(table->data[i].name);
    free(table->data[i].module_name);
  }
  da_free((*table));
}

static symbol_entry *symbol_table_find(symbol_table *table, const char *name) {
  for (size_t i = 0; i < table->len; i++) {
    if (strcmp(table->data[i].name, name) == 0) {
      return &table->data[i];
    }
  }
  return NULL;
}

static int symbol_table_add(symbol_table *table, const char *name,
                            const char *module_name, int final_value) {
  // Check for duplicates (except main)
  // TODO: sepearate structure for main fuctninos
  if (strcmp(name, "main") != 0) {
    symbol_entry *existing = symbol_table_find(table, name);
    if (existing) {
      fprintf(stderr,
              "Error: Duplicate public symbol '%s' in modules '%s' and '%s'\n",
              name, existing->module_name, module_name);
      return -1;
    }
  }

  symbol_entry entry;
  entry.name = strdup(name);
  entry.module_name = strdup(module_name);
  entry.final_value = final_value;

  da_append((*table), entry);

  return 0;
}

typedef struct {
  int total_string_table_size;
  int total_code_size;
  int total_globals_len;
  module_relocation *relocations;
  size_t module_len;
} section_sizes;

static section_sizes *calculate_section_sizes(module_list *modules) {
  section_sizes *sizes = malloc(sizeof(section_sizes));
  if (!sizes)
    return NULL;

  sizes->module_len = modules->modules.len;
  sizes->relocations = malloc(sizes->module_len * sizeof(module_relocation));
  if (!sizes->relocations) {
    free(sizes);
    return NULL;
  }

  // Start globals at 1 (0 reserved for sysargs/externs) TODO: potentially ugly
  // as well
  sizes->total_globals_len = 1;
  sizes->total_string_table_size = 0;
  sizes->total_code_size = 0;

  for (size_t i = 0; i < sizes->module_len; i++) {
    module *mod = modules->modules.data[i];
    bytecode *bc = mod->bc;

    sizes->relocations[i].string_table_base = sizes->total_string_table_size;
    sizes->relocations[i].code_base = sizes->total_code_size;
    sizes->relocations[i].global_base = sizes->total_globals_len;

    sizes->total_string_table_size += bc->string_table_size;
    sizes->total_code_size += bc->code_size;
    sizes->total_globals_len += bc->globals_count;

    VM_DEBUG("Module '%s': str_base=%d, code_base=%d, global_base=%d\n",
             mod->name, sizes->relocations[i].string_table_base,
             sizes->relocations[i].code_base,
             sizes->relocations[i].global_base);
  }

  VM_DEBUG("Total: string_table=%d, code=%d, globals=%d\n",
           sizes->total_string_table_size, sizes->total_code_size,
           sizes->total_globals_len);

  return sizes;
}

static void free_section_sizes(section_sizes *sizes) {
  if (sizes) {
    free(sizes->relocations);
    free(sizes);
  }
}

static int build_symbol_table(module_list *modules, section_sizes *sizes,
                              symbol_table *table) {
  symbol_table_init(table);

  for (size_t i = 0; i < modules->modules.len; i++) {
    module *mod = modules->modules.data[i];
    bytecode *bc = mod->bc;
    module_relocation *reloc = &sizes->relocations[i];

    // Process public symbols
    for (int j = 0; j < bc->public_symbols_count; j++) {
      int name_offset = bc->public_symbols[j * 2];
      int offset = bc->public_symbols[j * 2 + 1];
      const char *name = read_string(bc, name_offset);

      int final_offset;

      if (strncmp(name, "global_", 7) == 0) {
        // Global variable, 0-based index
        final_offset = offset + reloc->global_base;
      } else {
        // Function, code offset
        final_offset = offset + reloc->code_base;
      }

      if (symbol_table_add(table, name, mod->name, final_offset) < 0) {
        return -1;
      }

      VM_DEBUG("Defined symbol '%s' -> %d\n", name, final_offset);
    }
  }

  return 0;
}

static void relocate_internal_references(uint8_t *code_buffer, int size,
                                         module_relocation *reloc) {
  int ip = 0;
  while (ip < size) {
    decoded_instr_t instr;
    // TODO: insanely ugly and lazy
    int instr_size = decode_instr(code_buffer, ip, size, &instr);
    if (instr_size < 0)
      break;

    uint8_t opcode = code_buffer[ip];
    ip++;

    switch (opcode) {
    // String references
    case OP_STRING:
    case OP_SEXP:
    case OP_TAG: {
      int str_offset = read_i32(code_buffer, ip);
      write_i32(code_buffer, ip, str_offset + reloc->string_table_base);
      break;
    }

    // Global references
    case OP_LD:
    case OP_ST: {
      int idx = read_i32(code_buffer, ip);
      write_i32(code_buffer, ip, idx + reloc->global_base);
      break;
    }

    case OP_JMP:
    case OP_CJMP_Z:
    case OP_CJMP_NZ: {
      int target = read_i32(code_buffer, ip);
      write_i32(code_buffer, ip, target + reloc->code_base);
      break;
    }

    case OP_CALL:
    case OP_CLOSURE: {
      int target = read_i32(code_buffer, ip);
      write_i32(code_buffer, ip, target + reloc->code_base);
      break;
    }
    }

    // Move to next instruction (instr_size already inlcudes the opcode byte)
    ip += (instr_size - 1);
  }
}

static int apply_substitutions(module *mod, uint8_t *merged_code,
                               module_relocation *reloc,
                               symbol_table *symbols) {
  bytecode *bc = mod->bc;
  const uint8_t *pos = bc->subst_table;
  const uint8_t *end = pos + bc->subst_table_size;

  while (pos < end) {
    int offset = read_i32(pos, 0);
    pos += 4;
    int string_table_index = read_i32(pos, 0);
    pos += 4;

    // Get the function's name from the module's string table
    const char *name = read_string(bc, string_table_index);

    symbol_entry *sym = symbol_table_find(symbols, name);
    if (!sym) {
      fprintf(stderr, "Undefined reference to '%s' in module '%s'\n", name,
              mod->name);
      return -1;
    }

    // offset is relative to a module start, so adds reloc->code_base
    int absolute_offset = reloc->code_base + offset;
    write_i32(merged_code, absolute_offset, sym->final_value);

    VM_DEBUG("Patched '%s' at %d with %d\n", name, absolute_offset,
             sym->final_value);
  }
  return 0;
}

static void build_merged(uint8_t *merged_code, char *merged_string_table,
                         module_list *modules, section_sizes *sizes,
                         symbol_table *table) {
  // Copy string table
  for (size_t i = 0; i < modules->modules.len; i++) {
    module *mod = modules->modules.data[i];
    memcpy(merged_string_table + sizes->relocations[i].string_table_base,
           mod->bc->string_table, mod->bc->string_table_size);
  }

  // Copy code
  for (size_t i = 0; i < modules->modules.len; i++) {
    module *mod = modules->modules.data[i];
    memcpy(merged_code + sizes->relocations[i].code_base, mod->bc->code,
           mod->bc->code_size);
  }

  // Relocate and apply external substitutions
  for (size_t i = 0; i < modules->modules.len; i++) {
    module *mod = modules->modules.data[i];
    module_relocation *reloc = &sizes->relocations[i];
    uint8_t *mod_code_start = merged_code + reloc->code_base;

    relocate_internal_references(mod_code_start, mod->bc->code_size, reloc);

    apply_substitutions(mod, merged_code, reloc, table);
  }
}

static int collect_main_entries(symbol_table *table, int **entries,
                                int *count) {
  int capacity = 10;
  *entries = malloc(capacity * sizeof(int));
  *count = 0;

  for (size_t i = 0; i < table->len; i++) {
    if (strcmp(table->data[i].name, "main") != 0) {
      continue;
    }
    if (*count >= capacity) {
      capacity *= 2;
      *entries = realloc(*entries, capacity * sizeof(int));
    }
    (*entries)[(*count)++] = table->data[i].final_value;
  }

  return 0;
}

merged_bytecode *merge_modules(module_list *modules) {
  merged_bytecode *merged = malloc(sizeof(merged_bytecode));
  bytecode *bc = malloc(sizeof(bytecode));
  merged->bc = bc;

  section_sizes *sizes = calculate_section_sizes(modules);
  if (!sizes)
    return NULL;

  symbol_table symbols;
  if (build_symbol_table(modules, sizes, &symbols) < 0) {
    free_section_sizes(sizes);
    return NULL;
  }

  bc->code_size = sizes->total_code_size;
  uint8_t *merged_code = malloc(bc->code_size);
  bc->code = merged_code;

  bc->string_table_size = sizes->total_string_table_size;
  char *merged_string_table = malloc(bc->string_table_size);
  bc->string_table = merged_string_table;

  bc->globals_count = sizes->total_globals_len;

  build_merged(merged_code, merged_string_table, modules, sizes, &symbols);

  // TODO: topological order
  // actually maybe there is a better way
  collect_main_entries(&symbols, &merged->main_entries, &merged->main_count);

  free_section_sizes(sizes);
  symbol_table_free(&symbols);

  return merged;
}

void free_merged_bytecode(merged_bytecode *mb) {
  if (mb) {
    if (mb->bc) {
      free((void *)mb->bc->code);
      free((void *)mb->bc->string_table);
      free(mb->bc);
    }
    free(mb->main_entries);
    free(mb);
  }
}
