#include <assert.h>
#include <stdbool.h>

#include "insn.h"
#include "ops.h"
#include "ops_proof_helpers.h"

aint nondet_aint(void);

static bool bad_reached = false;
static ops_vm_snapshot stop_snapshot;

static void stop_after_jump(DECL_STATE) {
  ops_snapshot_record(&stop_snapshot, STATE);
}

static void bad_path(DECL_STATE) {
  (void)ip;
  (void)sp;
  (void)bp;
  bad_reached = true;
}

void op_jmp_harness(void) {
  aint stack[4];
  aint *sp = &stack[2];
  aint *bp = &stack[3];
  insn code[4];

  stack[2] = nondet_aint();
  stack[3] = nondet_aint();
  ops_snapshot_reset(&stop_snapshot);
  bad_reached = false;

  code[0].func = op_jmp;
  code[1].target = &code[3];
  code[2].func = bad_path;
  code[3].func = stop_after_jump;

  op_jmp(&code[0], sp, bp);

  assert(stop_snapshot.reached);
  assert(!bad_reached);
  assert(stop_snapshot.ip_seen == &code[3]);
  assert(stop_snapshot.sp_seen == sp);
  assert(stop_snapshot.bp_seen == bp);
}
