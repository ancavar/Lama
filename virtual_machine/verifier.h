#ifndef VERIFIER_H
#define VERIFIER_H

#include "bytecode.h"
#include <stdbool.h>

typedef struct {
  int ip;   // Address of this instruction
  int size; // Total size of opcode + operands in bytes (useful for jumping to
            // next ip)
  uint8_t opcode;  // The opcode byte
  int jump_target; // Target address for all jumps (-1 if no jump)
} decoded_instr_t;
int decode_instr(const uint8_t *code, int ip, int code_size,
                 decoded_instr_t *instr);
bool verify_bytecode(bytecode *bc);

#endif
