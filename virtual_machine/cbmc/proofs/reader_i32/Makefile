HARNESS_FILE := reader_i32_harness
HARNESS_ENTRY := reader_i32_harness
PROOF_UID := reader_i32

PROJECT_SOURCES += $(SRCDIR)/cbmc/sources/reader_anchor.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
CBMCFLAGS += --reachability-slice

include ../Makefile.common
