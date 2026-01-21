/*
 * Module manager implementation for Lama VM.
 */

#include "module_manager.h"
#include "util.h"
#include <libgen.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void module_list_init(module_list *list) { da_init(list->modules); }

static void module_list_free(module_list *list) {
  for (size_t i = 0; i < list->modules.len; i++) {
    if (list->modules.data[i]) {
      if (list->modules.data[i]->bc) {
        free_bytecode(list->modules.data[i]->bc);
      }
      free(list->modules.data[i]->name);
      free(list->modules.data[i]);
    }
  }
  da_free(list->modules);
}

// TODO: not very efficient but okay for now
module *find_module(module_list *list, const char *name) {
  for (size_t i = 0; i < list->modules.len; i++) {
    if (list->modules.data[i] &&
        strcmp(list->modules.data[i]->name, name) == 0) {
      return list->modules.data[i];
    }
  }
  return NULL;
}

static module *create_module(module_list *list, const char *name) {
  module *mod = malloc(sizeof(module));
  if (!mod) {
    perror("malloc");
    return NULL;
  }

  mod->name = strdup(name);
  mod->bc = NULL;
  da_append(list->modules, mod);
  return mod;
}

/*
 * Build the path to a module's .bc file.
 */
static char *build_module_path(const char *module_name,
                               const char *search_path) {
  char *path = malloc(MAX_PATH_LEN);
  if (!path) {
    return NULL;
  }

  if (search_path && strlen(search_path) > 0) {
    snprintf(path, MAX_PATH_LEN, "%s/%s.bc", search_path, module_name);
  } else {
    snprintf(path, MAX_PATH_LEN, "%s.bc", module_name);
  }

  return path;
}

static char *get_directory(const char *filepath) {
  if (!filepath)
    return NULL;

  char *path_copy = strdup(filepath);
  if (!path_copy)
    return NULL;

  char *dir = dirname(path_copy);
  char *result = strdup(dir);

  free(path_copy);

  return result ? result : strdup(".");
}

/*
 * Check if a string looks like a file path (contains '/' or ends with '.bc')
 */
static int is_filepath(const char *str) {
  return strchr(str, '/') != NULL ||
         (strlen(str) > 3 && strcmp(str + strlen(str) - 3, ".bc") == 0);
}

/*
 * Load modules recursively.
 */
static module *load_module(module_list *list, const char *s,
                           const char *search_path) {
  char *filepath = NULL;
  char *module_name = NULL;
  char *derived_search_path = NULL;

  // Determine if we're loading by path or by name
  if (is_filepath(s)) {
    filepath = strdup(s);
    module_name = extract_module_name(s);
  } else {
    filepath = build_module_path(s, search_path);
    module_name = strdup(s);
  }

  if (!filepath) {
    fprintf(stderr, "Failed to build path for '%s'\n", s);
    free(module_name);
    return NULL;
  }

  if (!module_name) {
    fprintf(stderr, "Failed to derive module name for '%s'\n", s);
    free(filepath);
    return NULL;
  }

  // Check if this module is already loaded
  module *existing = find_module(list, module_name);
  if (existing) {
    free(filepath);
    free(module_name);
    return existing;
  }

  bytecode *bc = load_bytecode(filepath);
  if (!bc) {
    fprintf(stderr, "Failed to load bytecode from '%s'\n", filepath);
    free(filepath);
    free(module_name);
    return NULL;
  }

  // Create new module entry
  module *mod = create_module(list, module_name);
  free(module_name);

  if (!mod) {
    free_bytecode(bc);
    free(filepath);
    return NULL;
  }

  mod->bc = bc;

  // Determine search path for dependencies
  // TODO: -I
  if (search_path) {
    derived_search_path = strdup(search_path);
  } else {
    derived_search_path = get_directory(filepath);
  }
  free(filepath);

  // Recursively load dependencies
  for (int i = 0; i < bc->import_count; i++) {
    const char *import_name = bc->imports[i];

    // Skip since we already have it (as runtime.a)
    if (strcmp(import_name, "Std") == 0) {
      continue;
    }

    module *dep = load_module(list, import_name, derived_search_path);
    if (!dep) {
      fprintf(stderr, "Failed to load dependency '%s' for module '%s'\n",
              import_name, mod->name);
      free(derived_search_path);
      return NULL;
    }
  }

  free(derived_search_path);
  return mod;
}

module_list *load_modules(const char *module_or_path, const char *search_path) {
  module_list *list = malloc(sizeof(module_list));
  if (!list) {
    perror("malloc");
    return NULL;
  }

  module_list_init(list);

  module *main_module = load_module(list, module_or_path, search_path);
  if (!main_module) {
    module_list_free(list);
    free(list);
    return NULL;
  }

  return list;
}

void free_modules(module_list *list) {
  if (!list) {
    return;
  }
  module_list_free(list);
  free(list);
}
