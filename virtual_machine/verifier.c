#include "verifier.h"
#include "bytecode.h"
#include "opcodes.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VERIFY_ERROR(ip, msg, ...)                                             \
  do {                                                                         \
    fprintf(stderr, "Verification failed at ip 0x%08X: " msg "\n", (ip),       \
            ##__VA_ARGS__);                                                    \
  } while (0);

typedef struct {
  int ip;   // Address of this instruction
  int size; // Total size of opcode + operands in bytes (useful for jumping to
            // next ip)
  uint8_t opcode;  // The opcode byte
  int jump_target; // Target address for all jumps (-1 if no jump)
} decoded_instr_t;

static int compute_closure_size(const uint8_t *code, int ip, int code_size) {
  int n_captured = read_i32(code, ip + 5);
  int total_size = 1 + 4 + 4 + n_captured * 5;

  if (ip + total_size >= code_size)
    return -1;

  return total_size;
}

static int decode_instr(const uint8_t *code, int ip, int code_size,
                        decoded_instr_t *instr) {
  if (ip >= code_size)
    return -1;

  memset(instr, 0, sizeof(*instr));
  instr->ip = ip;
  instr->jump_target = -1;
  instr->opcode = code[ip];

  uint8_t opcode = instr->opcode;
  // Position after upcode
  int pos = ip + 1;

  switch (opcode) {
  // No operands
  case OP_BINOP_ADD:
  case OP_BINOP_SUB:
  case OP_BINOP_MUL:
  case OP_BINOP_DIV:
  case OP_BINOP_MOD:
  case OP_BINOP_LT:
  case OP_BINOP_LE:
  case OP_BINOP_GT:
  case OP_BINOP_GE:
  case OP_BINOP_EQ:
  case OP_BINOP_NE:
  case OP_BINOP_AND:
  case OP_BINOP_OR:
  case OP_DROP:
  case OP_DUP:
  case OP_SWAP:
  case OP_ELEM:
  case OP_STA:
  case OP_LENGTH:
  case OP_LSTRING:
  case OP_READ:
  case OP_WRITE:
  case OP_PATT_STR_CMP:
  case OP_PATT_STRING:
  case OP_PATT_ARRAY:
  case OP_PATT_SEXP:
  case OP_PATT_BOXED:
  case OP_PATT_UNBOXED:
  case OP_PATT_CLOSURE:
    instr->size = 1;
    break;

  // One operand
  case OP_CONST:
  case OP_STRING:
  case OP_LD:
  case OP_LD_LOC:
  case OP_LD_ARG:
  case OP_LD_CLO:
  case OP_ST:
  case OP_ST_LOC:
  case OP_ST_ARG:
  case OP_ST_CLO:
  case OP_LINE:
  case OP_ARRAY:
  case OP_BARRAY:
  case OP_CALLC:
    if (pos + 4 > code_size)
      return -1;
    instr->size = 5;
    break;

  case OP_JMP:
    if (pos + 4 > code_size)
      return -1;
    instr->jump_target = read_i32(code, pos);
    instr->size = 5;
    break;

  case OP_CJMP_Z:
  case OP_CJMP_NZ:
    if (pos + 4 > code_size)
      return -1;
    instr->jump_target = read_i32(code, pos);
    instr->size = 5;
    break;

  // Two operands
  case OP_BEGIN:
  case OP_BEGIN_CLOSURE:
    if (pos + 8 > code_size)
      return -1;
    instr->size = 9;
    break;

  case OP_CALL:
    if (pos + 8 > code_size)
      return -1;
    instr->jump_target = read_i32(code, pos); // call target
    instr->size = 9;
    break;

  case OP_CLOSURE: {
    int closure_size = compute_closure_size(code, ip, code_size);
    if (closure_size < 0)
      return -1;
    instr->jump_target = read_i32(code, pos); // closure entry point
    instr->size = closure_size;
    break;
  }

  case OP_SEXP:
    if (pos + 8 > code_size)
      return -1;
    instr->size = 9;
    break;

  case OP_TAG:
    if (pos + 8 > code_size)
      return -1;
    instr->size = 9;
    break;

  case OP_FAIL:
    if (pos + 8 > code_size)
      return -1;
    instr->size = 9;
    break;

  case OP_RET:
  case OP_END:
    instr->size = 1;
    break;

  case OP_HALT:
    instr->size = 1;
    break;

  default:
    return -1; // Unknown opcode
  }

  return instr->size;
}

/**
 * Insturciton map for checking valid jump boundaries
 */
typedef struct {
  decoded_instr_t *instrs; // array of decoded instructions
  int *ip_to_index; // map from ip to instruction index (-1 if not a boundary)
  int len;
  int cap;
  int code_size;
} instr_map_t;

static void instr_map_destroy(instr_map_t *map) {
  if (!map)
    return;
  free(map->instrs);
  free(map->ip_to_index);
  free(map);
}

static instr_map_t *instr_map_create(int code_size) {
  instr_map_t *map = malloc(sizeof(instr_map_t));
  if (!map)
    return NULL;

  map->code_size = code_size;
  map->cap = code_size / 2 + 1;
  map->len = 0;

  map->instrs = malloc(map->cap * sizeof(decoded_instr_t));
  map->ip_to_index = malloc(map->code_size * sizeof(int));

  if (!map->instrs || !map->ip_to_index) {
    instr_map_destroy(map);
    return NULL;
  }

  memset(map->ip_to_index, -1, code_size * sizeof(int));

  return map;
}

static int instr_map_add(instr_map_t *map, const decoded_instr_t *instr) {
  int ip = instr->ip;

  if (map->len >= map->cap) {
    int new_cap = map->cap + 2;
    decoded_instr_t *new_instrs =
        realloc(map->instrs, new_cap * sizeof(decoded_instr_t));
    if (!new_instrs)
      return -1;
    map->instrs = new_instrs;
    map->cap = new_cap;
  }

  int idx = map->len++;
  map->instrs[idx] = *instr;
  map->ip_to_index[ip] = idx;

  return idx;
}

static bool is_valid_instr_boundary(const instr_map_t *map, int ip) {
  if (ip < 0 || ip >= map->code_size)
    return false;
  return map->ip_to_index[ip] >= 0;
}

/**
 * Decoding pass (building an instruciton map)
 */
static instr_map_t *decode_all_instructions(bytecode *bc) {
  instr_map_t *map = instr_map_create(bc->code_size);
  if (!map) {
    fprintf(stderr, "Failed to allocate instruction map\n");
    return NULL;
  }

  int ip = 0;
  while (ip < bc->code_size) {
    decoded_instr_t instr;
    int size = decode_instr(bc->code, ip, bc->code_size, &instr);

    if (size < 0) {
      VERIFY_ERROR(ip, "Failed to decode instruction (opcode 0x%02X)",
                   bc->code[ip]);
      instr_map_destroy(map);
      return NULL;
    }

    if (instr_map_add(map, &instr) < 0) {
      fprintf(stderr, "Failed to add instruction to map\n");
      instr_map_destroy(map);
      return NULL;
    }

    ip += size;
  }

  return map;
}

/**
 * Validate jump targets
 */
static bool validate_jump_targets(instr_map_t *map) {
  for (int i = 0; i < map->len; i++) {
    decoded_instr_t *instr = &map->instrs[i];
    if (instr->jump_target >= 0) {
      if (!is_valid_instr_boundary(map, instr->jump_target)) {
        VERIFY_ERROR(instr->ip,
                     "Jump target 0x%08X is not a valid instruction boundary",
                     instr->jump_target);
        return false;
      }
    }
  }
  return true;
}

/**
 * Verify bytecode by validating jump targets
 */
bool verify_bytecode(bytecode *bc) {
  instr_map_t *map = decode_all_instructions(bc);
  if (!map) {
    return false;
  }

  if (!validate_jump_targets(map)) {
    instr_map_destroy(map);
    return false;
  }

  instr_map_destroy(map);
  return true;
}
