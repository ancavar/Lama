HARNESS_FILE := symbol_table_duplicate_harness
HARNESS_ENTRY := symbol_table_duplicate_harness
PROOF_UID := symbol_table_duplicate

PROJECT_SOURCES += $(SRCDIR)/symbols.c
PROJECT_SOURCES += $(SRCDIR)/memory.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
INCLUDES += -I$(SRCDIR)/cbmc/include
CBMCFLAGS += --reachability-slice
REMOVE_FUNCTION_BODY += fprintf
CBMC_DEFAULT_UNWIND := --unwind 4

include ../Makefile.common
