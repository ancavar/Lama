HARNESS_FILE := reader_skip_seek_harness
HARNESS_ENTRY := reader_skip_seek_harness
PROOF_UID := reader_skip_seek

PROJECT_SOURCES += $(SRCDIR)/cbmc/sources/reader_anchor.c

PROOF_SOURCES += $(PROOFDIR)/$(HARNESS_FILE).c

INCLUDES += -I$(SRCDIR)
CBMCFLAGS += --reachability-slice

include ../Makefile.common
