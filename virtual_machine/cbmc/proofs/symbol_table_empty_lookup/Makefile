HARNESS_FILE := symbol_table_empty_lookup_harness
HARNESS_ENTRY := symbol_table_empty_lookup_harness
PROOF_UID := symbol_table_empty_lookup

PROJECT_SOURCES += $(SRCDIR)/symbols.c
PROJECT_SOURCES += $(SRCDIR)/memory.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
INCLUDES += -I$(SRCDIR)/cbmc/include
CBMCFLAGS += --reachability-slice
CBMC_DEFAULT_UNWIND := --unwind 2

include ../Makefile.common
