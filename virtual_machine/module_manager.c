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
  bytecode *bc = NULL;
  module *mod = NULL;
  module *result = NULL;

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
    goto cleanup;
  }

  if (!module_name) {
    fprintf(stderr, "Failed to derive module name for '%s'\n", s);
    goto cleanup;
  }

  // Check if this module is already loaded
  module *existing = find_module(list, module_name);
  if (existing) {
    result = existing;
    goto cleanup;
  }

  bc = load_bytecode(filepath);
  if (!bc) {
    fprintf(stderr, "Failed to load bytecode from '%s'\n", filepath);
    goto cleanup;
  }

  // Create new module entry
  mod = create_module(list, module_name);
  if (!mod) {
    goto cleanup;
  }

  mod->bc = bc;
  bc = NULL;

  // Determine search path for dependencies
  // TODO: -I
  if (search_path) {
    derived_search_path = strdup(search_path);
  } else {
    derived_search_path = get_directory(filepath);
  }

  // Recursively load dependencies
  for (int i = 0; i < mod->bc->import_count; i++) {
    const char *import_name = mod->bc->imports[i];

    // Skip since we already have it (as runtime.a)
    if (strcmp(import_name, "Std") == 0) {
      continue;
    }

    module *dep = load_module(list, import_name, derived_search_path);
    if (!dep) {
      fprintf(stderr, "Failed to load dependency '%s' for module '%s'\n",
              import_name, mod->name);
      goto cleanup;
    }
  }

  result = mod;

cleanup:
  free(filepath);
  free(module_name);
  free(derived_search_path);
  if (bc) {
    free_bytecode(bc);
  }
  return result;
}
/*
 * Based on Kahn's algorithm.
 * TODO: remove since we preserve order during bytecode compilation.
 */
static bool topological_sort(module_list *list) {
  int n = list->modules.len;

  int *indegree = calloc(n, sizeof(int));
  module **result = malloc(n * sizeof(module *));
  int *queue = malloc(n * sizeof(int));

  if (!indegree || !result || !queue) {
    goto fail;
  }

  // Compute indegrees
  for (int i = 0; i < n; i++) {
    module *mod = list->modules.data[i];

    for (int j = 0; j < mod->bc->import_count; j++) {
      const char *import_name = mod->bc->imports[j];

      if (strcmp(import_name, "Std") == 0) {
        continue;
      }

      int dep_idx = -1;
      for (int k = 0; k < n; k++) {
        if (strcmp(list->modules.data[k]->name, import_name) == 0) {
          dep_idx = k;
          break;
        }
      }

      if (dep_idx < 0) {
        fprintf(stderr, "Unknown module '%s' imported by '%s'\n", import_name,
                mod->name);
        goto fail;
      }

      // Edge: dep -> mod
      indegree[i]++;
    }
  }

  int hd = 0;
  int tl = 0;

  for (int i = 0; i < n; i++) {
    if (indegree[i] == 0) {
      queue[tl++] = i;
    }
  }

  int result_idx = 0;

  while (hd < tl) {
    int idx = queue[hd++];
    module *mod = list->modules.data[idx];

    result[result_idx++] = mod;

    // TODO: might actually need a hashmap after all
    for (int i = 0; i < n; i++) {
      module *dep = list->modules.data[i];

      for (int j = 0; j < dep->bc->import_count; j++) {
        if (strcmp(dep->bc->imports[j], mod->name) == 0) {
          if (--indegree[i] == 0) {
            queue[tl++] = i;
          }
        }
      }
    }
  }

  if (result_idx != n) {
    fprintf(stderr, "Circular dependency detected\n");
    goto fail;
  }
  memcpy(list->modules.data, result, n * sizeof(module *));
  free(indegree);
  free(queue);
  free(result);
  return true;

fail:
  free(indegree);
  free(queue);
  free(result);
  return false;
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

  if (!topological_sort(list)) {
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
