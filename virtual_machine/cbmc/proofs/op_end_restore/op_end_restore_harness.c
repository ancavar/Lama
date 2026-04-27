#include <assert.h>
#include <stdbool.h>

#include "insn.h"
#include "ops_frame_layout.h"
#include "ops.h"
#include "ops_proof_helpers.h"

aint nondet_aint(void);

enum {
  STACK_WORDS = 80,
  BP_INDEX = 32,
  SP_INDEX = 16,
  CALLER_SP_INDEX = 56,
  SAVED_BP_INDEX = 70,
};

static ops_vm_snapshot stop_snapshot;

static void stop_after_return(DECL_STATE) {
  ops_snapshot_record(&stop_snapshot, STATE);
}

void op_end_restore_harness(void) {
  aint stack[STACK_WORDS];
  aint ret_val = nondet_aint();
  aint *bp = &stack[BP_INDEX];
  aint *sp = &stack[SP_INDEX];
  aint *caller_sp = &stack[CALLER_SP_INDEX];
  aint *saved_bp = &stack[SAVED_BP_INDEX];
  insn code[2];
  ops_snapshot_reset(&stop_snapshot);

  code[0].func = op_end;
  code[1].func = stop_after_return;

  sp[0] = ret_val;
  bp[FRAME_SAVED_BP] = (aint)saved_bp;
  bp[FRAME_SAVED_IP] = (aint)&code[1];
  bp[FRAME_SAVED_SP] = (aint)caller_sp;

  op_end(&code[0], sp, bp);

  assert(stop_snapshot.reached);
  assert(stop_snapshot.ip_seen == &code[1]);
  assert(stop_snapshot.sp_seen == caller_sp - 1);
  assert(stop_snapshot.bp_seen == saved_bp);
  assert(*stop_snapshot.sp_seen == ret_val);
}
