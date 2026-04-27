HARNESS_FILE := op_dup_harness
HARNESS_ENTRY := op_dup_harness
PROOF_UID := op_dup

PROJECT_SOURCES += $(SRCDIR)/ops.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c
PROOF_SOURCES += $(SRCDIR)/cbmc/stubs/ops_runtime_stubs.c

INCLUDES += -I$(SRCDIR)
INCLUDES += -I$(SRCDIR)/cbmc/include
DEFINES += -D__CPROVER__

CBMCFLAGS += --reachability-slice
CBMC_DEFAULT_UNWIND := --unwind 2

include ../Makefile.common
