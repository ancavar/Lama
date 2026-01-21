/*
 * Module loader and linker for Lama VM.
 *
 */

#ifndef MODULE_MANAGER_H
#define MODULE_MANAGER_H

#include "bytecode.h"
#include <stdbool.h>
#include <stddef.h>

/* Maximum length of a module search path */
#define MAX_PATH_LEN 1024

typedef struct module {
  char *name;
  bytecode *bc;
} module;

typedef struct module_list {
  struct {
    module **data;
    size_t len;
    size_t cap;
  } modules;
} module_list;

module_list *load_modules(const char *module_or_path, const char *search_path);
void free_modules(module_list *list);
module *find_module(module_list *list, const char *name);

#endif
