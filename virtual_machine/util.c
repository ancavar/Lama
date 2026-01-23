#include "util.h"
#include "bytecode.h"
#include <libgen.h>
#include <stdlib.h>
#include <string.h>

char *strndup(const char *str, int chars) {
  char *buffer;
  int n;

  buffer = (char *)malloc(chars + 1);
  if (buffer) {
    for (n = 0; ((n < chars) && (str[n] != 0)); n++)
      buffer[n] = str[n];
    buffer[n] = 0;
  }

  return buffer;
}

char *strdup(const char *src) { return strndup((char *)src, strlen(src)); }

/* Extract module name from filename (without path and extension .bc) */
char *extract_module_name(const char *filename) {
  char *path_copy = strdup(filename);
  char *base = basename(path_copy);

  char *dot = strrchr(base, '.');
  if (dot && strcmp(dot, ".bc") == 0) {
    *dot = '\0';
  }

  char *result = strdup(base);
  free(path_copy);
  return result;
}

int read_i32(const unsigned char data[], int offset) {
  return data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) |
         (data[offset + 3] << 24);
}

void write_i32(unsigned char *data, int offset, int value) {
  data[offset] = value & 0xFF;
  data[offset + 1] = (value >> 8) & 0xFF;
  data[offset + 2] = (value >> 16) & 0xFF;
  data[offset + 3] = (value >> 24) & 0xFF;
}

const char *read_string(const bytecode *bc, int index) {
  if (!bc)
    return NULL;
  return bc->string_table + index;
}
