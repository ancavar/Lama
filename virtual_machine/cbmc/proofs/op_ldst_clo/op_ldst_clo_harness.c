#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "insn.h"
#include "ops.h"

aint nondet_aint(void);
int32_t nondet_int32(void);

enum {
  STACK_WORDS = 64,
  BP_INDEX = 24,
  SP_INDEX = 8,
  MAX_ARGS = 3,
  MAX_CAPTURES = 4,
};

static bool stop_reached = false;
static insn *stop_ip_seen = NULL;
static aint *stop_sp_seen = NULL;
static aint *stop_bp_seen = NULL;

static void stop_after_load(DECL_STATE) {
  stop_reached = true;
  stop_ip_seen = ip;
  stop_sp_seen = sp;
  stop_bp_seen = bp;
}

void op_ldst_clo_harness(void) {
  int32_t n_args = nondet_int32();
  int32_t idx = nondet_int32();

  __CPROVER_assume(n_args >= 0 && n_args <= MAX_ARGS);
  __CPROVER_assume(idx >= 0 && idx < MAX_CAPTURES);

  aint stack[STACK_WORDS];
  aint closure[MAX_CAPTURES + 1];
  aint stored = nondet_aint();
  aint *bp = &stack[BP_INDEX];
  aint *sp = &stack[SP_INDEX];
  insn code[5];

  bp[0] = n_args;
  bp[n_args + 1] = (aint)closure;
  closure[0] = nondet_aint();
  for (int32_t i = 0; i < MAX_CAPTURES; i++) {
    closure[i + 1] = nondet_aint();
  }
  sp[0] = stored;

  code[0].func = op_st_clo;
  code[1].num = idx;
  code[2].func = op_ld_clo;
  code[3].num = idx;
  code[4].func = stop_after_load;

  op_st_clo(&code[0], sp, bp);

  assert(stop_reached);
  assert(stop_ip_seen == &code[4]);
  assert(stop_sp_seen == sp - 1);
  assert(stop_bp_seen == bp);
  assert(closure[idx + 1] == stored);
  assert(stop_sp_seen[0] == stored);
  assert(stop_sp_seen[1] == stored);
}
