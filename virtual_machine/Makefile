CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2
DEPFLAGS = -MMD
TARGET = interpreter.exe
SOURCES = interpreter.c bytecode.c stack.c call_stack.c opcodes.c verifier.c module_manager.c util.c
OBJECTS = $(SOURCES:.c=.o)

RUNTIME_DIR = ../runtime
RUNTIME_LIB = $(RUNTIME_DIR)/runtime.a

all: $(TARGET)

debug: CFLAGS += -DDEBUG_PRINT -g3 -Og
debug: $(TARGET)

$(TARGET): $(OBJECTS) $(RUNTIME_LIB)
	$(CC) -o $@ $^

$(RUNTIME_LIB):
	$(MAKE) -C $(RUNTIME_DIR)

%.o: %.c
	$(CC) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJECTS) $(TARGET) $(OBJECTS:.o=.d)
	$(MAKE) -C $(RUNTIME_DIR) clean

.PHONY: all debug clean
