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
  MAX_ARGS = 4,
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

void op_ldst_arg_harness(void) {
  int32_t n_args = nondet_int32();
  int32_t idx = nondet_int32();

  __CPROVER_assume(n_args >= 1 && n_args <= MAX_ARGS);
  __CPROVER_assume(idx >= 0 && idx < n_args);

  aint stack[STACK_WORDS];
  aint arg_before[MAX_ARGS];
  aint stored = nondet_aint();
  aint *bp = &stack[BP_INDEX];
  aint *sp = &stack[SP_INDEX];
  insn code[5];
  int32_t target_slot = n_args - idx;

  bp[0] = n_args;
  for (int32_t i = 0; i < MAX_ARGS; i++) {
    arg_before[i] = nondet_aint();
    bp[i + 1] = arg_before[i];
  }
  sp[0] = stored;

  code[0].func = op_st_arg;
  code[1].num = idx;
  code[2].func = op_ld_arg;
  code[3].num = idx;
  code[4].func = stop_after_load;

  op_st_arg(&code[0], sp, bp);

  assert(stop_reached);
  assert(stop_ip_seen == &code[4]);
  assert(stop_sp_seen == sp - 1);
  assert(stop_bp_seen == bp);
  assert(bp[0] == n_args);
  assert(bp[target_slot] == stored);
  for (int32_t i = 0; i < n_args; i++) {
    if (i != target_slot - 1) {
      assert(bp[i + 1] == arg_before[i]);
    }
  }
  assert(stop_sp_seen[0] == stored);
  assert(stop_sp_seen[1] == stored);
}
