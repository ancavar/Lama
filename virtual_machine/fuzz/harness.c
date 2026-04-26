
#include "../vm.h"

#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


static jmp_buf fuzz_jmp;
static int in_fuzz_iteration;

// catch exit() from the VM and longjmp back to the loop
void exit(int status) {
  if (in_fuzz_iteration) {
    longjmp(fuzz_jmp, status ? status : 1);
  }
  _exit(status);
}

int main(int argc, char *argv[]) {
  const char *bc_file = argv[1];
  const char *search_dir = "/dev/null";
  const char *paths[] = {search_dir};
  char *fake_argv[] = {"lama", NULL};

  while (__AFL_LOOP(10000)) {
    in_fuzz_iteration = 1;
    if (setjmp(fuzz_jmp) == 0) {
      virtual_machine *vm = vm_create(bc_file, paths, 1);
      if (vm) {
        vm_set_args(vm, 1, fake_argv);
        vm_run(vm);
        vm_destroy(vm);
      }
    }
    in_fuzz_iteration = 0;
  }

  return 0;
}
