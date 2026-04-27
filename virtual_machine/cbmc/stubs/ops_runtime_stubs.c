#include <stddef.h>

#include "ffi.h"
#include "runtime/runtime_common.h"

size_t __gc_stack_top = 0;
size_t __gc_stack_bottom = 0;

aint Ls__Infix_43(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_45(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_42(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_47(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_37(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_60(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_6061(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_62(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_6261(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_6161(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_3361(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_3838(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

aint Ls__Infix_3333(aint p, aint q) {
  (void)p;
  (void)q;
  return BOX(0);
}

void *Barray(aint *args, aint bn) {
  (void)args;
  (void)bn;
  return NULL;
}

void *Bsexp(aint *args, aint bn) {
  (void)args;
  (void)bn;
  return NULL;
}

void *Bclosure(aint *args, aint bn) {
  (void)args;
  (void)bn;
  return NULL;
}

void *Bstring(aint *args) {
  (void)args;
  return NULL;
}

void *Belem(void *p, aint i) {
  (void)p;
  (void)i;
  return NULL;
}

void *Bsta(void *x, aint i, void *v) {
  (void)x;
  (void)i;
  (void)v;
  return NULL;
}

aint Btag(void *d, aint t, aint n) {
  (void)d;
  (void)t;
  (void)n;
  return BOX(0);
}

aint Barray_patt(aint d, aint n) {
  (void)d;
  (void)n;
  return BOX(0);
}

aint Bstring_patt(aint x, aint y) {
  (void)x;
  (void)y;
  return BOX(0);
}

aint Bclosure_tag_patt(aint x) {
  (void)x;
  return BOX(0);
}

aint Bboxed_patt(aint x) {
  (void)x;
  return BOX(0);
}

aint Bunboxed_patt(aint x) {
  (void)x;
  return BOX(0);
}

aint Barray_tag_patt(aint x) {
  (void)x;
  return BOX(0);
}

aint Bstring_tag_patt(aint x) {
  (void)x;
  return BOX(0);
}

aint Bsexp_tag_patt(aint x) {
  (void)x;
  return BOX(0);
}

void Bmatch_failure(aint v, const char *fname, aint line, aint col) {
  (void)v;
  (void)fname;
  (void)line;
  (void)col;
  __CPROVER_assume(0);
}

aint ffi_call_c(const ffi_resolved *res, aint *args, int n_args) {
  (void)res;
  (void)args;
  (void)n_args;
  return BOX(0);
}
