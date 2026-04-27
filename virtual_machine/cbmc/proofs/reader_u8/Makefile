HARNESS_FILE := reader_u8_harness
HARNESS_ENTRY := reader_u8_harness
PROOF_UID := reader_u8

PROJECT_SOURCES += $(SRCDIR)/cbmc/sources/reader_anchor.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
CBMCFLAGS += --reachability-slice

include ../Makefile.common
