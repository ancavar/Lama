#include <assert.h>
#include <stdbool.h>
#include <stdint.h>

#include "insn.h"
#include "ops.h"

int32_t nondet_int32(void);

enum {
  STACK_WORDS = 16,
  STACK_BASE_INDEX = 8,
};

static bool stop_reached = false;
static insn *stop_ip_seen = NULL;
static aint *stop_sp_seen = NULL;

static void stop_after_const(DECL_STATE) {
  (void)bp;
  stop_reached = true;
  stop_ip_seen = ip;
  stop_sp_seen = sp;
}

void op_const_harness(void) {
  int32_t value = nondet_int32();

  aint stack[STACK_WORDS];
  insn code[3];

  code[0].func = op_const;
  code[1].num = value;
  code[2].func = stop_after_const;

  aint *initial_sp = &stack[STACK_BASE_INDEX];

  op_const(&code[0], initial_sp, NULL);

  assert(stop_reached);
  assert(stop_ip_seen == &code[2]);
  assert(stop_sp_seen == initial_sp - 1);
  assert(*stop_sp_seen == BOX(value));
}
