#include <assert.h>
#include <stdbool.h>

#include "insn.h"
#include "ops_frame_layout.h"
#include "ops.h"

enum {
  STACK_WORDS = 16,
  STACK_BASE_INDEX = 8,
};

static bool probe_reached = false;
static insn *probe_ip_seen = NULL;
static aint *probe_sp_seen = NULL;
static aint *probe_bp_seen = NULL;

static void probe_after_callc(DECL_STATE) {
  probe_reached = true;
  probe_ip_seen = ip;
  probe_sp_seen = sp;
  probe_bp_seen = bp;
}

void op_callc_frame_harness(void) {
  aint stack[STACK_WORDS];
  aint closure[1];
  insn code[4];
  aint *initial_sp = &stack[STACK_BASE_INDEX];
  aint *initial_bp = NULL;

  code[0].func = op_callc;
  code[1].num = 1;
  code[2].func = op_eof;
  code[3].func = probe_after_callc;

  closure[0] = (aint)&code[3];
  initial_sp[0] = BOX(100);
  initial_sp[1] = (aint)closure;

  op_callc(&code[0], initial_sp, initial_bp);

  assert(probe_reached);
  assert(probe_ip_seen == &code[3]);
  assert(probe_sp_seen == initial_sp - 4);
  assert(probe_bp_seen == initial_sp - 1);

  assert(probe_bp_seen[0] == 1);
  assert(probe_bp_seen[1] == BOX(100));
  assert((aint *)probe_bp_seen[2] == closure);
  assert((aint *)probe_bp_seen[FRAME_SAVED_BP] == initial_bp);
  assert((insn *)probe_bp_seen[FRAME_SAVED_IP] == &code[2]);
  assert((aint *)probe_bp_seen[FRAME_SAVED_SP] == initial_sp + 2);
}
