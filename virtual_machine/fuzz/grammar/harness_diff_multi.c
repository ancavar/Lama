#define _GNU_SOURCE
#include "../../vm.h"
#include <fcntl.h>
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// NOTE: need to paste absolute path to Driver.exe
#define DRIVER ""
#define MAX_OUT (64 * 1024)
#define MAX_UNITS 256

__AFL_COVERAGE();

static jmp_buf fuzz_jmp;
static int in_fuzz_iteration;

// catch exit() from the VM and longjmp back to the loop
void exit(int status) {
  if (in_fuzz_iteration) {
    longjmp(fuzz_jmp, status ? status : 1);
  }
  _exit(status);
}

static void run(const char **argv, const char *dir) {
  pid_t p = fork();
  if (p == 0) {
    if (dir) {
      chdir(dir);
    }
    int fd = open("/dev/null", O_WRONLY);
    dup2(fd, 1);
    dup2(fd, 2);
    close(fd);
    alarm(10);
    execv(argv[0], (char *const *)argv);
    _exit(EXIT_FAILURE);
  }
  int st;
  waitpid(p, &st, 0);
}

static int run_capture(const char *exe, const char *in, const char *out) {
  pid_t p = fork();
  if (p == 0) {
    int fi = open(in, O_RDONLY);
    dup2(fi, 0);
    close(fi);
    int fo = open(out, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(fo, 1);
    close(fo);
    int fe = open("/dev/null", O_WRONLY);
    dup2(fe, 2);
    close(fe);
    alarm(5);
    execl(exe, exe, NULL);
    _exit(EXIT_FAILURE);
  }
  int st;
  waitpid(p, &st, 0);
  return WIFEXITED(st) ? WEXITSTATUS(st) : -1;
}

static int slurp(const char *path, char *buf, int sz) {
  FILE *f = fopen(path, "r");
  if (!f) {
    return 0;
  }
  int n = fread(buf, 1, sz - 1, f);
  buf[n] = 0;
  fclose(f);
  return n;
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    return 0;
  }

  __AFL_COVERAGE_OFF();

  // read input
  FILE *f = fopen(argv[1], "rb");
  if (!f)
    return 0;
  fseek(f, 0, SEEK_END);
  long len = ftell(f);
  fseek(f, 0, SEEK_SET);
  char *buf = malloc(len + 1);
  fread(buf, 1, len, f);
  buf[len] = 0;
  fclose(f);

  // split delim
  char *units[MAX_UNITS];
  int unit_count = 0;
  char *cur = buf;
  while (unit_count < MAX_UNITS) {
    char *d = strstr(cur, "---DELIM---");
    if (!d) {
      units[unit_count++] = cur;
      break;
    }
    *d = 0;
    units[unit_count++] = cur;
    cur = d + strlen("---DELIM---");
    while (*cur == '\n' || *cur == '\r')
      cur++;
  }
  if (unit_count < 1) {
    free(buf);
    return 0;
  }

  char tmp[] = "/tmp/dm_XXXXXX";
  mkdtemp(tmp);
  char path[512];

  for (int i = 0; i < unit_count; i++) {
    if (i == unit_count - 1)
      snprintf(path, 512, "%s/Main.lama", tmp);
    else
      snprintf(path, 512, "%s/Unit%d.lama", tmp, i + 1);
    FILE *o = fopen(path, "w");
    fputs(units[i], o);
    fclose(o);
  }

  snprintf(path, 512, "%s/input.txt", tmp);
  f = fopen(path, "w");
  for (int i = 0; i <= 100; i++)
    fprintf(f, "%d\n", i);
  fclose(f);

  // compile
  for (int i = 0; i < unit_count - 1; i++) {
    char unit_file[32];
    snprintf(unit_file, 32, "Unit%d.lama", i + 1);
    const char *cmd[] = {DRIVER, "-I", ".", "-bc", unit_file, NULL};
    run(cmd, tmp);
  }

  const char *cmd_b[] = {DRIVER, "-I", ".", "-b", "Main.lama", NULL};
  run(cmd_b, tmp);

  snprintf(path, 512, "%s/Main.bc", tmp);
  if (access(path, F_OK))
    goto done;

  const char *cmd_o[] = {DRIVER,        "-I",        ".", "-o",
                         "main_native", "Main.lama", NULL};
  run(cmd_o, tmp);

  char exe[512];
  snprintf(exe, 512, "%s/main_native", tmp);
  if (access(exe, F_OK))
    goto done;

  char inp[512], rout[512], vout[512];
  snprintf(inp, 512, "%s/input.txt", tmp);
  snprintf(rout, 512, "%s/ref_out", tmp);
  snprintf(vout, 512, "%s/vm_out", tmp);
  int ref = run_capture(exe, inp, rout);

  int vm_ok = 0;
  fflush(stdout);
  int sv_out = dup(1), sv_in = dup(0);
  int fo = open(vout, O_WRONLY | O_CREAT | O_TRUNC, 0644);
  dup2(fo, 1);
  close(fo);
  int fi = open(inp, O_RDONLY);
  dup2(fi, 0);
  close(fi);

  __AFL_COVERAGE_ON();
  in_fuzz_iteration = 1;
  if (setjmp(fuzz_jmp) == 0) {
    const char *paths[] = {tmp};
    virtual_machine *vm = vm_create(path, paths, 1);
    if (vm) {
      vm_run(vm);
      vm_destroy(vm);
      vm_ok = 1;
    }
  }
  in_fuzz_iteration = 0;
  __AFL_COVERAGE_OFF();

  fflush(stdout);
  dup2(sv_out, 1);
  close(sv_out);
  dup2(sv_in, 0);
  close(sv_in);

  /* Compare */
  if (ref == 0 && !vm_ok)
    abort();
  if (ref == 0 && vm_ok) {
    char rb[MAX_OUT] = {0}, vb[MAX_OUT] = {0};
    slurp(rout, rb, MAX_OUT);
    slurp(vout, vb, MAX_OUT);
    if (strcmp(rb, vb)) {
      abort();
    }
  }

done: {
  char cmd[256];
  snprintf(cmd, 256, "rm -rf %s", tmp);
  system(cmd);
}
  free(buf);
  return 0;
}
