#include <assert.h>
#include <stdbool.h>

#include "insn.h"
#include "ops.h"
#include "ops_proof_helpers.h"

aint nondet_aint(void);

enum {
  STACK_WORDS = 16,
  STACK_BASE_INDEX = 8,
};

static ops_vm_snapshot stop_snapshot;

static void stop_after_load(DECL_STATE) {
  ops_snapshot_record(&stop_snapshot, STATE);
}

void op_ldst_glo_harness(void) {
  aint stack[STACK_WORDS];
  aint global = nondet_aint();
  aint stored = nondet_aint();
  aint *initial_sp = &stack[STACK_BASE_INDEX];
  aint *initial_bp = NULL;
  insn code[5];
  ops_snapshot_reset(&stop_snapshot);

  initial_sp[0] = stored;

  code[0].func = op_st_glo;
  code[1].global_ptr = &global;
  code[2].func = op_ld_glo;
  code[3].global_ptr = &global;
  code[4].func = stop_after_load;

  op_st_glo(&code[0], initial_sp, initial_bp);

  assert(stop_snapshot.reached);
  assert(stop_snapshot.ip_seen == &code[4]);
  assert(stop_snapshot.sp_seen == initial_sp - 1);
  assert(stop_snapshot.bp_seen == initial_bp);
  assert(global == stored);
  assert(stop_snapshot.sp_seen[0] == stored);
  assert(stop_snapshot.sp_seen[1] == stored);
}
