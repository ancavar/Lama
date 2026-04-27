#include <assert.h>

#include "insn.h"
#include "ops.h"
#include "ops_proof_helpers.h"

aint nondet_aint(void);

enum {
  STACK_WORDS = 16,
  STACK_BASE_INDEX = 8,
};

static ops_vm_snapshot stop_snapshot;

static void stop_after_dup(DECL_STATE) {
  ops_snapshot_record(&stop_snapshot, STATE);
}

void op_dup_harness(void) {
  aint stack[STACK_WORDS];
  aint top = nondet_aint();
  aint next = nondet_aint();
  aint *sp = &stack[STACK_BASE_INDEX];
  aint *bp = NULL;
  insn code[2];

  ops_snapshot_reset(&stop_snapshot);
  sp[0] = top;
  sp[1] = next;

  code[0].func = op_dup;
  code[1].func = stop_after_dup;

  op_dup(&code[0], sp, bp);

  assert(stop_snapshot.reached);
  assert(stop_snapshot.ip_seen == &code[1]);
  assert(stop_snapshot.sp_seen == sp - 1);
  assert(stop_snapshot.bp_seen == bp);
  assert(stop_snapshot.sp_seen[0] == top);
  assert(stop_snapshot.sp_seen[1] == top);
  assert(stop_snapshot.sp_seen[2] == next);
}
