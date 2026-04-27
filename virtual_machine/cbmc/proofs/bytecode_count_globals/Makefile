HARNESS_FILE := bytecode_count_globals_harness
HARNESS_ENTRY := bytecode_count_globals_harness
PROOF_UID := bytecode_count_globals

PROJECT_SOURCES += $(SRCDIR)/bytecode.c
PROJECT_SOURCES += $(SRCDIR)/memory.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
CBMCFLAGS += --reachability-slice

include ../Makefile.common
