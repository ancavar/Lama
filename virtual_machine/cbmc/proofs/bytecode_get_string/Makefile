HARNESS_FILE := bytecode_get_string_harness
HARNESS_ENTRY := bytecode_get_string_harness
PROOF_UID := bytecode_get_string

PROJECT_SOURCES += $(SRCDIR)/bytecode.c
PROJECT_SOURCES += $(SRCDIR)/memory.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
CBMCFLAGS += --reachability-slice

include ../Makefile.common
