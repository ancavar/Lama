#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "insn.h"
#include "ops.h"
#include "ops_proof_helpers.h"

aint nondet_aint(void);

enum {
  STACK_WORDS = 16,
  STACK_BASE_INDEX = 8,
};

static aint initial_top = 0;
static aint initial_next = 0;
static aint *initial_sp = NULL;
static aint *initial_bp = NULL;

static ops_vm_snapshot stop_snapshot;

static void probe_after_swap(DECL_STATE) {
  assert(sp == initial_sp);
  assert(bp == initial_bp);
  assert(sp[0] == initial_next);
  assert(sp[1] == initial_top);

  ip++;
  ip->func(STATE);
}

static void probe_after_dup(DECL_STATE) {
  assert(sp == initial_sp - 1);
  assert(bp == initial_bp);
  assert(sp[0] == initial_next);
  assert(sp[1] == initial_next);
  assert(sp[2] == initial_top);

  ip++;
  ip->func(STATE);
}

static void stop_after_drop(DECL_STATE) {
  ops_snapshot_record(&stop_snapshot, STATE);
}

void op_stack_ops_harness(void) {
  aint stack[STACK_WORDS];
  insn code[6];

  initial_top = nondet_aint();
  initial_next = nondet_aint();
  initial_sp = &stack[STACK_BASE_INDEX];
  initial_bp = NULL;
  ops_snapshot_reset(&stop_snapshot);

  initial_sp[0] = initial_top;
  initial_sp[1] = initial_next;

  code[0].func = op_swap;
  code[1].func = probe_after_swap;
  code[2].func = op_dup;
  code[3].func = probe_after_dup;
  code[4].func = op_drop;
  code[5].func = stop_after_drop;

  op_swap(&code[0], initial_sp, initial_bp);

  assert(stop_snapshot.reached);
  assert(stop_snapshot.ip_seen == &code[5]);
  assert(stop_snapshot.sp_seen == initial_sp);
  assert(stop_snapshot.bp_seen == initial_bp);
  assert(stop_snapshot.sp_seen[0] == initial_next);
  assert(stop_snapshot.sp_seen[1] == initial_top);
}
