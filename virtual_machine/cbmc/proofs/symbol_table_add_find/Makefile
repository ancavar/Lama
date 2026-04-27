HARNESS_FILE := symbol_table_add_find_harness
HARNESS_ENTRY := symbol_table_add_find_harness
PROOF_UID := symbol_table_add_find

PROJECT_SOURCES += $(SRCDIR)/symbols.c
PROJECT_SOURCES += $(SRCDIR)/memory.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
INCLUDES += -I$(SRCDIR)/cbmc/include
CBMCFLAGS += --reachability-slice
CBMC_DEFAULT_UNWIND := --unwind 4

include ../Makefile.common
