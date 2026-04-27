#ifndef CBMC_OPS_PROOF_HELPERS_H
#define CBMC_OPS_PROOF_HELPERS_H

#include <stdbool.h>

#include "insn.h"

typedef struct {
  bool reached;
  insn *ip_seen;
  aint *sp_seen;
  aint *bp_seen;
} ops_vm_snapshot;

static inline void ops_snapshot_reset(ops_vm_snapshot *snapshot) {
  snapshot->reached = false;
  snapshot->ip_seen = NULL;
  snapshot->sp_seen = NULL;
  snapshot->bp_seen = NULL;
}

static inline void ops_snapshot_record(ops_vm_snapshot *snapshot, DECL_STATE) {
  snapshot->reached = true;
  snapshot->ip_seen = ip;
  snapshot->sp_seen = sp;
  snapshot->bp_seen = bp;
}

#endif
