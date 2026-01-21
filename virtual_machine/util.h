#ifndef UTIL_H
#define UTIL_H
/*
 * Dynamic array macros
 */
// TODO: exit(1) might be too harsh
#define da_append(xs, x)                                                       \
  do {                                                                         \
    if (xs.len >= xs.cap) {                                                    \
      xs.cap = xs.cap == 0 ? 256 : xs.cap * 2;                                 \
      xs.data = realloc(xs.data, xs.cap * sizeof(*xs.data));                   \
      if (!xs.data) {                                                          \
        perror("realloc");                                                     \
        exit(1);                                                               \
      }                                                                        \
    }                                                                          \
    xs.data[xs.len++] = x;                                                     \
  } while (0)

#define da_init(xs)                                                            \
  do {                                                                         \
    xs.data = NULL;                                                            \
    xs.len = 0;                                                                \
    xs.cap = 0;                                                                \
  } while (0)

#define da_free(xs)                                                            \
  do {                                                                         \
    free(xs.data);                                                             \
    xs.data = NULL;                                                            \
    xs.len = 0;                                                                \
    xs.cap = 0;                                                                \
  } while (0)

char *strndup(const char *str, int chars);
char *strdup(const char *src);
char *extract_module_name(const char *filename);

/* Read a 32-bit little-endian integer */
int read_i32(const unsigned char data[], int offset);
/* Write a 32-bit little-endian integer */
void write_i32(unsigned char *data, int offset, int value);

// TODO: quite ugly
#ifdef DEBUG_PRINT
#define STACK_PEEK_SIZE 5
#define VM_DEBUG(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define VM_TRACE_STACK(stack)                                                  \
  do {                                                                         \
    long sp_idx = (stack)->sp - (stack)->data;                                 \
    fprintf(stderr, "  stack [sp=%p, idx=%ld]: ", (stack)->sp, sp_idx);        \
    for (int i = 1; i <= STACK_PEEK_SIZE; i++) {                               \
      if (sp_idx + i < STACK_SIZE) {                                           \
        fprintf(stderr, "%ld ", (long)(stack)->data[sp_idx + i]);              \
      }                                                                        \
    }                                                                          \
    fprintf(stderr, "\n");                                                     \
  } while (0)
#define VM_TRACE_CALL(fmt, ...) fprintf(stderr, "[CALL] " fmt, ##__VA_ARGS__)
#define VM_ASSERT(cond, msg)                                                   \
  do {                                                                         \
    if (!(cond)) {                                                             \
      fprintf(stderr, "Assert failed: %s at %s:%d\n", msg, __FILE__,           \
              __LINE__);                                                       \
      exit(1);                                                                 \
    }                                                                          \
  } while (0)
#else
#define VM_DEBUG(fmt, ...)
#define VM_TRACE_STACK(stack)
#define VM_TRACE_CALL(fmt, ...)
#define VM_ASSERT(cond, msg)
#endif

#endif
