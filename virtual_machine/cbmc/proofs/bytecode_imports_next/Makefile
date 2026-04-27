HARNESS_FILE := bytecode_imports_next_harness
HARNESS_ENTRY := bytecode_imports_next_harness
PROOF_UID := bytecode_imports_next

PROJECT_SOURCES += $(SRCDIR)/bytecode.c
PROJECT_SOURCES += $(SRCDIR)/memory.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
CBMCFLAGS += --reachability-slice

include ../Makefile.common
