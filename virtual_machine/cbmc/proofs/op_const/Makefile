HARNESS_FILE := op_const_harness
HARNESS_ENTRY := op_const_harness
PROOF_UID := op_const

PROJECT_SOURCES += $(SRCDIR)/ops.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c
PROOF_SOURCES += $(SRCDIR)/cbmc/stubs/ops_runtime_stubs.c

INCLUDES += -I$(SRCDIR)
DEFINES += -D__CPROVER__

CBMCFLAGS += --reachability-slice
CBMC_DEFAULT_UNWIND := --unwind 2

include ../Makefile.common
