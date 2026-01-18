#include <libgen.h>
#include <stdlib.h>
#include <string.h>

char *strndup(char *str, int chars) {
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
