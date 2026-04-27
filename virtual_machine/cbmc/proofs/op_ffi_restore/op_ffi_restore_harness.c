#include <assert.h>
#include <stdbool.h>

#include "ffi.h"
#include "insn.h"
#include "ops_frame_layout.h"
#include "ops.h"

enum {
  STACK_WORDS = 64,
  BP_INDEX = 24,
  CALLER_SP_INDEX = 40,
  SAVED_BP_INDEX = 50,
};

static bool stop_reached = false;
static insn *stop_ip_seen = NULL;
static aint *stop_sp_seen = NULL;
static aint *stop_bp_seen = NULL;

static void stop_after_return(DECL_STATE) {
  stop_reached = true;
  stop_ip_seen = ip;
  stop_sp_seen = sp;
  stop_bp_seen = bp;
}

void op_ffi_restore_harness(void) {
  aint stack[STACK_WORDS];
  aint *bp = &stack[BP_INDEX];
  aint *caller_sp = &stack[CALLER_SP_INDEX];
  aint *saved_bp = &stack[SAVED_BP_INDEX];
  insn code[3];
  ffi_resolved res = {0};

  res.kind = FFI_REGULAR;
  res.fixed_args = 0;

  code[0].func = op_ffi_call;
  code[1].ptr = &res;
  code[2].func = stop_after_return;

  bp[0] = 0;
  bp[FRAME_SAVED_BP] = (aint)saved_bp;
  bp[FRAME_SAVED_IP] = (aint)&code[2];
  bp[FRAME_SAVED_SP] = (aint)caller_sp;

  op_ffi_call(&code[0], bp + 1, bp);

  assert(stop_reached);
  assert(stop_ip_seen == &code[2]);
  assert(stop_sp_seen == caller_sp - 1);
  assert(stop_bp_seen == saved_bp);
  assert(*stop_sp_seen == BOX(0));
}
