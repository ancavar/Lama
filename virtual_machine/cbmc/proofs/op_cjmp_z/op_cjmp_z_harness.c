#include <assert.h>
#include <stdbool.h>

#include "insn.h"
#include "ops.h"

bool nondet_bool(void);
aint nondet_aint(void);

static bool target_reached = false;
static bool fallthrough_reached = false;
static aint *target_sp_seen = NULL;
static aint *fallthrough_sp_seen = NULL;
static aint *target_bp_seen = NULL;
static aint *fallthrough_bp_seen = NULL;
static insn *target_ip_seen = NULL;
static insn *fallthrough_ip_seen = NULL;

static void stop_target(DECL_STATE) {
  target_reached = true;
  target_ip_seen = ip;
  target_sp_seen = sp;
  target_bp_seen = bp;
}

static void stop_fallthrough(DECL_STATE) {
  fallthrough_reached = true;
  fallthrough_ip_seen = ip;
  fallthrough_sp_seen = sp;
  fallthrough_bp_seen = bp;
}

void op_cjmp_z_harness(void) {
  bool take_branch = nondet_bool();
  aint stack[8];
  aint *sp = &stack[3];
  aint *bp = &stack[6];
  insn code[5];

  stack[3] = take_branch ? BOX(0) : BOX(1);
  stack[6] = nondet_aint();

  code[0].func = op_cjmp_z;
  code[1].target = &code[4];
  code[2].func = stop_fallthrough;
  code[3].func = stop_fallthrough;
  code[4].func = stop_target;

  op_cjmp_z(&code[0], sp, bp);

  if (take_branch) {
    assert(target_reached);
    assert(!fallthrough_reached);
    assert(target_ip_seen == &code[4]);
    assert(target_sp_seen == sp + 1);
    assert(target_bp_seen == bp);
  } else {
    assert(!target_reached);
    assert(fallthrough_reached);
    assert(fallthrough_ip_seen == &code[2]);
    assert(fallthrough_sp_seen == sp + 1);
    assert(fallthrough_bp_seen == bp);
  }
}
