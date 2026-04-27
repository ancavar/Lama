#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "insn.h"
#include "ops_frame_layout.h"
#include "ops.h"

aint nondet_aint(void);
int32_t nondet_int32(void);

enum {
  STACK_WORDS = 80,
  BP_INDEX = 48,
  SP_INDEX = 16,
  MAX_LOCALS = 4,
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

void op_ldst_loc_harness(void) {
  int32_t idx = nondet_int32();

  __CPROVER_assume(idx >= 0 && idx < MAX_LOCALS);

  aint stack[STACK_WORDS];
  aint locals_before[MAX_LOCALS];
  aint stored = nondet_aint();
  aint *bp = &stack[BP_INDEX];
  aint *sp = &stack[SP_INDEX];
  insn code[5];

  for (int32_t i = 0; i < MAX_LOCALS; i++) {
    locals_before[i] = nondet_aint();
    bp[FRAME_LOCALS - i] = locals_before[i];
  }
  sp[0] = stored;

  code[0].func = op_st_loc;
  code[1].num = idx;
  code[2].func = op_ld_loc;
  code[3].num = idx;
  code[4].func = stop_after_load;

  op_st_loc(&code[0], sp, bp);

  assert(stop_reached);
  assert(stop_ip_seen == &code[4]);
  assert(stop_sp_seen == sp - 1);
  assert(stop_bp_seen == bp);
  assert(bp[FRAME_LOCALS - idx] == stored);
  for (int32_t i = 0; i < MAX_LOCALS; i++) {
    if (i != idx) {
      assert(bp[FRAME_LOCALS - i] == locals_before[i]);
    }
  }
  assert(stop_sp_seen[0] == stored);
  assert(stop_sp_seen[1] == stored);
}
