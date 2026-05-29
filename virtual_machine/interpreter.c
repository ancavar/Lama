/*
 * Core bytecode interpreter for the Lama VM.
 * Implements the fetch-decode-execute loop for all supported opcodes.
 * Manages the data stack, call stack, and interacts with the C runtime.
 */

#include "../runtime/gc.h"
#include "../runtime/runtime_common.h"
#include "bytecode.h"
#include "bytecode_merger.h"
#include "call_stack.h"
#include "ffi.h"
#include "module_manager.h"
#include "opcodes.h"
#include "stack.h"
#include "util.h"
#include "verifier.h"
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static aint *pending_closure = NULL;

extern void __init(void);
extern void set_args(aint argc, char *argv[]);
extern void *global_sysargs;

extern aint Lread(void);
extern aint Lwrite(aint n);
extern aint Ls__Infix_43(void *p, void *q);
extern aint Ls__Infix_45(void *p, void *q);
extern aint Ls__Infix_42(void *p, void *q);
extern aint Ls__Infix_47(void *p, void *q);
extern aint Ls__Infix_37(void *p, void *q);
extern aint Ls__Infix_60(void *p, void *q);
extern aint Ls__Infix_6061(void *p, void *q);
extern aint Ls__Infix_62(void *p, void *q);
extern aint Ls__Infix_6261(void *p, void *q);
extern aint Ls__Infix_6161(void *p, void *q);
extern aint Ls__Infix_3361(void *p, void *q);
extern aint Ls__Infix_3838(void *p, void *q);
extern aint Ls__Infix_3333(void *p, void *q);

extern aint Llength(void *p);
extern void *Lstring(aint *args);
extern aint LtagHash(char *s);
extern void *Barray(aint *args, aint bn);
extern void *Bsexp(aint *args, aint bn);
extern void *Bclosure(aint *args, aint bn);
extern void *Bstring(aint *args);
extern void *Belem(void *p, aint i);
extern void *Bsta(void *x, aint i, void *v);

extern aint Btag(void *d, aint t, aint n);
extern aint Barray_patt(void *d, aint n);
extern aint Bstring_patt(void *x, void *y);
extern aint Bclosure_tag_patt(void *x);
extern aint Bboxed_patt(void *x);
extern aint Bunboxed_patt(void *x);
extern aint Barray_tag_patt(void *x);
extern aint Bstring_tag_patt(void *x);
extern aint Bsexp_tag_patt(void *x);

/**
 * Retrieves a pointer to a local variable in the current stack frame.
 * Locals are stored below the arguments in the stack.
 * For closure calls, base points to closure, so args start at base-1.
 */
static inline aint *get_local(stack_t *stack, call_frame_t *frame, int idx) {
  int args_base = frame->closure ? frame->base - 1 : frame->base;
  return &stack->data[args_base - frame->n_args - idx];
}

/**
 * Retrieves a pointer to an argument in the current stack frame.
 * For closure calls, base points to closure, so args start at base-1.
 */
static inline aint *get_arg(stack_t *stack, call_frame_t *frame, int idx) {
  int args_base = frame->closure ? frame->base - 1 : frame->base;
  return &stack->data[args_base - idx];
}

/**
 * Retrieves a pointer to a variable stored in a closure's environment.
 */
static inline aint *get_closure_var(call_frame_t *frame, int idx) {
  data *closure_data = TO_DATA(*(frame->closure));
  aint *contents = (aint *)closure_data->contents;
  // +1 because contents[0] is the entry point
  return &contents[idx + 1];
}

static aint read_designation(stack_t *stack, call_frame_t *frame, aint *globals,
                             const uint8_t *code, int *ip_ptr) {
  uint8_t type_byte = code[(*ip_ptr)++];
  int idx = read_i32(code, *ip_ptr);
  *ip_ptr += 4;

  int designation_type = type_byte & 0xF;
  switch (designation_type) {
  case 0:
    return globals[idx];
  case 1:
    return *get_local(stack, frame, idx);
  case 2:
    return *get_arg(stack, frame, idx);
  case 3:
    return *get_closure_var(frame, idx);
  default:
    fprintf(stderr, "Unknown designation type: %d\n", designation_type);
    exit(1);
  }
}

static void run_internal(bytecode *bc, int entry_point, stack_t *stack,
                         call_stack_t *call_stack, aint *globals,
                         char **ffi_names, int ffi_len) {
  int ip = entry_point;
  int return_ip = -1;

  VM_TRACE_CALL("Entering run_internal at entry_point=%d\n", entry_point);

  while (ip < bc->code_size) {
    uint8_t opcode = bc->code[ip++];
    int l = opcode & 0xF;

    VM_TRACE_STACK(stack);

    switch (opcode) {
    case OP_CONST: {
      int n = read_i32(bc->code, ip);
      ip += 4;
      VM_DEBUG("CONST: %d\n", n);
      stack_push(stack, BOX(n));
      break;
    }
    case OP_BINOP_ADD:
    case OP_BINOP_SUB:
    case OP_BINOP_MUL:
    case OP_BINOP_DIV:
    case OP_BINOP_MOD:
    case OP_BINOP_EQ:
    case OP_BINOP_NE:
    case OP_BINOP_LT:
    case OP_BINOP_LE:
    case OP_BINOP_GT:
    case OP_BINOP_GE:
    case OP_BINOP_AND:
    case OP_BINOP_OR: {
      aint y = stack_pop(stack);
      aint x = stack_pop(stack);
      aint result;
      switch (l) {
      case 1: // +
        result = Ls__Infix_43((void *)x, (void *)y);
        break;
      case 2: // -
        result = Ls__Infix_45((void *)x, (void *)y);
        break;
      case 3: // *
        result = Ls__Infix_42((void *)x, (void *)y);
        break;
      case 4: // /
        if (UNBOX(y) == 0) {
          fprintf(stderr, "Division by zero\n");
          return;
        }
        result = Ls__Infix_47((void *)x, (void *)y);
        break;
      case 5: // %
        if (UNBOX(y) == 0) {
          fprintf(stderr, "Division by zero\n");
          return;
        }
        result = Ls__Infix_37((void *)x, (void *)y);
        break;
      case 6: // <
        result = Ls__Infix_60((void *)x, (void *)y);
        break;
      case 7: // <=
        result = Ls__Infix_6061((void *)x, (void *)y);
        break;
      case 8: // >
        result = Ls__Infix_62((void *)x, (void *)y);
        break;
      case 9: // >=
        result = Ls__Infix_6261((void *)x, (void *)y);
        break;
      case 10: // ==
        result = Ls__Infix_6161((void *)x, (void *)y);
        break;
      case 11: // !=
        result = Ls__Infix_3361((void *)x, (void *)y);
        break;
      case 12: // &&
        result = Ls__Infix_3838((void *)x, (void *)y);
        break;
      case 13: // !!
        result = Ls__Infix_3333((void *)x, (void *)y);
        break;
      }
      VM_DEBUG("BINOP %d: x=%ld, y=%ld -> result=%ld\n", l, UNBOX(x), UNBOX(y),
               UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_JMP: {
      int addr = read_i32(bc->code, ip);
      VM_DEBUG("JMP to 0x%08x\n", addr);
      ip = addr;
      break;
    }
    case OP_CJMP_Z: {
      int addr = read_i32(bc->code, ip);
      ip += 4;
      aint val = stack_pop(stack);
      VM_DEBUG("CJMP_Z val=%ld -> %s to 0x%08x\n", UNBOX(val),
               (UNBOX(val) == 0 ? "JUMP" : "SKIP"), addr);
      if (UNBOX(val) == 0) {
        ip = addr;
      }
      break;
    }
    case OP_CJMP_NZ: {
      int addr = read_i32(bc->code, ip);
      ip += 4;
      aint val = stack_pop(stack);
      VM_DEBUG("CJMP_NZ val=%ld -> %s to 0x%08x\n", UNBOX(val),
               (UNBOX(val) != 0 ? "JUMP" : "SKIP"), addr);
      if (UNBOX(val) != 0) {
        ip = addr;
      }
      break;
    }
    // TODO: unify ld and st
    case OP_LD: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      if (idx == 0) {
        globals[0] = (aint)global_sysargs;
      }
      aint val = globals[idx];
      VM_DEBUG("LD global[%d] = %ld\n", idx, val);
      stack_push(stack, val);
      break;
    }
    case OP_LD_LOC: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      call_frame_t *frame = call_stack_current(call_stack);
      aint val = *get_local(stack, frame, idx);
      VM_DEBUG("LD_LOC local[%d] = %ld\n", idx, val);
      stack_push(stack, val);
      break;
    }
    case OP_LD_ARG: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      call_frame_t *frame = call_stack_current(call_stack);
      aint val = *get_arg(stack, frame, idx);
      VM_DEBUG("LD_ARG arg[%d] = %ld\n", idx, val);
      stack_push(stack, val);
      break;
    }
    case OP_LD_CLO: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      call_frame_t *frame = call_stack_current(call_stack);
      aint val = *get_closure_var(frame, idx);
      VM_DEBUG("LD_CLO closure[%d] = %ld\n", idx, val);
      stack_push(stack, val);
      break;
    }
    case OP_ST: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      aint val = stack_pop(stack);
      VM_DEBUG("ST global[%d] = %ld\n", idx, val);
      globals[idx] = val;
      stack_push(stack, val);
      break;
    }
    case OP_ST_LOC: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      call_frame_t *frame = call_stack_current(call_stack);
      aint val = stack_pop(stack);
      VM_DEBUG("ST_LOC local[%d] = %ld\n", idx, val);
      *get_local(stack, frame, idx) = val;
      stack_push(stack, val);
      break;
    }
    case OP_ST_ARG: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      call_frame_t *frame = call_stack_current(call_stack);
      aint val = stack_pop(stack);
      VM_DEBUG("ST_ARG arg[%d] = %ld\n", idx, val);
      *get_arg(stack, frame, idx) = val;
      stack_push(stack, val);
      break;
    }
    case OP_ST_CLO: {
      int idx = read_i32(bc->code, ip);
      ip += 4;
      call_frame_t *frame = call_stack_current(call_stack);
      aint val = stack_pop(stack);
      VM_DEBUG("ST_CLO closure[%d] = %ld\n", idx, val);
      *get_closure_var(frame, idx) = val;
      stack_push(stack, val);
      break;
    }
    case OP_DROP:
      VM_DEBUG("DROP\n");
      stack_pop(stack);
      break;
    case OP_DUP:
      VM_DEBUG("DUP\n");
      stack_dup(stack);
      break;
    case OP_SWAP:
      VM_DEBUG("SWAP\n");
      stack_swap(stack);
      break;
    // TODO: possibly unify as well
    case OP_BEGIN: {
      int n_args = read_i32(bc->code, ip);
      ip += 4;
      int n_locals = read_i32(bc->code, ip);
      ip += 4;
      VM_TRACE_CALL("BEGIN n_args=%d n_locals=%d\n", n_args, n_locals);

      int base;
      aint *closure = NULL;

      // Check if we're being called through CALLC
      if (pending_closure != NULL) {
        // Stack: [... closure arg0 arg1 ... argN-1]
        // base points to closure so we clean it up on return
        base = (stack->sp - stack->data) + n_args + 1;
        closure = pending_closure;
        // Clear for next call
        pending_closure = NULL;
      } else {
        // base points to arg0
        base = (stack->sp - stack->data) + n_args;
      }

      // space for locals
      for (int i = 0; i < n_locals; i++) {
        stack_push(stack, 0);
      }

      call_stack_push(call_stack, return_ip, base, n_args, n_locals, closure);
      break;
    }
    case OP_BEGIN_CLOSURE: {
      int n_args = read_i32(bc->code, ip);
      ip += 4;
      int n_locals = read_i32(bc->code, ip);
      ip += 4;
      VM_TRACE_CALL("BEGIN_CLOSURE n_args=%d n_locals=%d\n", n_args, n_locals);

      // Stack: [... closure arg0 arg1 ... argN-1]
      // The closure is at sp + n_args + 1, arg0 is at sp + n_args
      // base points to the closure so we clean it up on return
      int base = (stack->sp - stack->data) + n_args + 1;
      aint *closure = pending_closure;
      // Clear for next call
      pending_closure = NULL;

      // space for locals
      for (int i = 0; i < n_locals; i++) {
        stack_push(stack, 0);
      }

      call_stack_push(call_stack, return_ip, base, n_args, n_locals, closure);
      break;
    }
    case OP_CLOSURE: {
      // addr:32 n_captured:32 [type:8 idx:32]...
      int addr = read_i32(bc->code, ip);
      ip += 4;
      int n_captured = read_i32(bc->code, ip);
      ip += 4;

      VM_DEBUG("CLOSURE addr=0x%08X n_captured=%d\n", addr, n_captured);

      aint args[n_captured + 1];
      args[0] = BOX(addr);

      for (int i = 0; i < n_captured; i++) {
        aint val = read_designation(stack, call_stack_current(call_stack),
                                    globals, bc->code, &ip);
        VM_DEBUG("Captured[%d] = %ld\n", i, val);
        args[i + 1] = val;
      }

      void *closure = Bclosure(args, BOX(n_captured));
      stack_push(stack, (aint)closure);
      break;
    }
    case OP_CALLC: {
      int n_args = read_i32(bc->code, ip);
      ip += 4;

      // Stack: [... closure arg0 arg1 ... argN-1]
      // sp points below argN-1. The closure is at sp + n_args + 1.

      aint *closure_ptr = stack->sp + n_args + 1;
      aint closure_val = *closure_ptr;

      aint entry = UNBOX(((aint *)closure_val)[0]);

      if (IS_FFI_CALL(entry)) {
        // FFI call
        int ffi_idx = FFI_INDEX(entry);
        if (ffi_idx < 0 || ffi_idx >= ffi_len) {
          fprintf(stderr, "Invalid FFI index: %d\n", ffi_idx);
          return;
        }
        const char *fn_name = ffi_names[ffi_idx];
        VM_TRACE_CALL("CALLC FFI '%s' n_args=%d\n", fn_name, n_args);

        aint ffi_args[n_args];
        for (int i = n_args - 1; i >= 0; i--) {
          ffi_args[i] = stack_pop(stack);
        }

        // Pop the closure
        stack_pop(stack);

        // FFI
        aint result = (aint)ffi_call_c(fn_name, ffi_args, n_args);
        stack_push(stack, result);
      } else {
        VM_TRACE_CALL("CALLC n_args=%d closure=0x%lx entry=0x%lx\n", n_args,
                      closure_val, entry);
        // Store pointer to closure location for BEGIN_CLOSURE to use
        pending_closure = closure_ptr;
        return_ip = ip;
        ip = entry;
      }
      break;
    }
    case OP_CALL: {
      int addr = read_i32(bc->code, ip);
      ip += 4;
      int n_args = read_i32(bc->code, ip);
      ip += 4;

      if (IS_FFI_CALL(addr)) {
        // FFI call
        // TODO: UNIFY WITH CALLC
        int ffi_idx = FFI_INDEX(addr);
        if (ffi_idx < 0 || ffi_idx >= ffi_len) {
          fprintf(stderr, "Invalid FFI index: %d\n", ffi_idx);
          return;
        }
        const char *fn_name = ffi_names[ffi_idx];
        VM_TRACE_CALL("CALL FFI '%s' n_args=%d\n", fn_name, n_args);

        aint args[n_args];
        for (int i = n_args - 1; i >= 0; i--) {
          args[i] = stack_pop(stack);
        }

        // FFI
        aint result = ffi_call_c(fn_name, args, n_args);
        stack_push(stack, result);
      } else {
        VM_TRACE_CALL("CALL addr=0x%08X\n", addr);
        return_ip = ip;
        ip = addr;
      }
      break;
    }
    case OP_RET:
    case OP_END: {
      call_frame_t frame = call_stack_pop(call_stack);

      // For closure calls, base points to closure, args start at base-1
      int args_base = frame.closure ? frame.base - 1 : frame.base;
      int current_top = stack->sp - stack->data;
      int returns_start = args_base - frame.n_args - frame.n_locals;
      int n_returns = returns_start - current_top;

      VM_TRACE_CALL("RET/END: n_returns=%d, return_ip=0x%08x\n", n_returns,
                    frame.return_ip);

      if (n_returns <= 0) {
        n_returns = 1;
      }
      for (int i = 0; i < n_returns; i++) {
        // TODO: make a stack function for this
        stack->data[frame.base - i] = stack->data[returns_start - i];
      }

      // sp points to empty slot below the return values
      stack->sp = stack->data + frame.base - n_returns;
      if (frame.return_ip < 0) {
        return;
      }
      ip = frame.return_ip;
      break;
    }

    case OP_READ: {
      aint val = Lread();
      VM_DEBUG("READ: %ld\n", UNBOX(val));
      stack_push(stack, val);
      break;
    }
    case OP_WRITE: {
      aint val = stack_pop(stack);
      VM_DEBUG("WRITE: %ld\n", UNBOX(val));
      stack_push(stack, Lwrite(val));
      break;
    }
    case OP_STRING: {
      // push string from string table onto stack
      int str_offset = read_i32(bc->code, ip);
      ip += 4;
      const char *src = read_string(bc, str_offset);
      VM_DEBUG("STRING: \"%s\"\n", src);
      void *str = Bstring((void *)&src);
      stack_push(stack, (aint)str);
      break;
    }
    case OP_ELEM: {
      // [top --> index, array] -> [element]
      aint idx = stack_pop(stack);
      aint arr = stack_pop(stack);
      void *elem = Belem((void *)arr, idx);
      VM_DEBUG("ELEM: arr=0x%lx, idx=%ld -> elem=0x%lx\n", arr, UNBOX(idx),
               (aint)elem);
      stack_push(stack, (aint)elem);
      break;
    }
    case OP_STA: {
      // TODO: support string (two operands)
      aint val = stack_pop(stack);
      aint idx = stack_pop(stack);
      aint arr = stack_pop(stack);
      VM_DEBUG("STA: arr=0x%lx, idx=%ld, val=0x%lx\n", arr, UNBOX(idx), val);
      Bsta((void *)arr, idx, (void *)val);
      stack_push(stack, val);
      break;
    }
    case OP_LENGTH: {
      aint val = stack_pop(stack);
      aint len = Llength((void *)val);
      VM_DEBUG("LENGTH: val=0x%lx -> len=%ld\n", val, UNBOX(len));
      stack_push(stack, len);
      break;
    }
    case OP_LSTRING: {
      aint val = stack_pop(stack);
      void *str = Lstring(&val);
      VM_DEBUG("LSTRING: val=%ld -> str=0x%lx\n", UNBOX(val), (aint)str);
      stack_push(stack, (aint)str);
      break;
    }
    case OP_BARRAY: {
      int n = read_i32(bc->code, ip);
      ip += 4;
      VM_DEBUG("BARRAY: n=%d\n", n);
      aint args[n];
      for (int i = n - 1; i >= 0; i--) {
        args[i] = stack_pop(stack);
      }
      void *arr = Barray(args, BOX(n));
      stack_push(stack, (aint)arr);
      break;
    }
    case OP_SEXP: {
      int tag_offset = read_i32(bc->code, ip);
      ip += 4;
      int n_fields = read_i32(bc->code, ip);
      ip += 4;
      const char *tag_str = read_string(bc, tag_offset);
      aint tag_hash = LtagHash((char *)tag_str);
      VM_DEBUG("SEXP: tag=\"%s\" (hash=0x%lx), n_fields=%d\n", tag_str,
               tag_hash, n_fields);
      aint args[n_fields + 1];
      for (int i = n_fields - 1; i >= 0; i--) {
        args[i] = stack_pop(stack);
      }
      args[n_fields] = tag_hash;

      void *s = Bsexp(args, BOX(n_fields + 1));
      stack_push(stack, (aint)s);
      break;
    }
    case OP_TAG: {
      int tag_offset = read_i32(bc->code, ip);
      ip += 4;
      int n_fields = read_i32(bc->code, ip);
      ip += 4;
      const char *tag_str = read_string(bc, tag_offset);
      aint tag_hash = LtagHash((char *)tag_str);
      aint val = stack_pop(stack);
      VM_DEBUG("TAG: val=0x%lx, tag=\"%s\" (hash=0x%lx), n_fields=%d\n", val,
               tag_str, tag_hash, n_fields);
      aint result = Btag((void *)val, tag_hash, BOX(n_fields));
      stack_push(stack, result);
      break;
    }
    case OP_ARRAY: {
      int n = read_i32(bc->code, ip);
      ip += 4;
      aint val = stack_pop(stack);
      VM_DEBUG("ARRAY pattern: val=0x%lx, n=%d\n", val, n);
      aint result = Barray_patt((void *)val, BOX(n));
      stack_push(stack, result);
      break;
    }
    case OP_FAIL: {
      int line = read_i32(bc->code, ip);
      ip += 4;
      int col = read_i32(bc->code, ip);
      ip += 4;
      VM_DEBUG("FAIL: line %d, col %d\n", line, col);
      fprintf(stderr, "Match failure at line %d, column %d\n", line, col);
      return;
    }
    case OP_PATT_STR_CMP: {
      aint y = stack_pop(stack);
      aint x = stack_pop(stack);
      aint result = Bstring_patt((void *)x, (void *)y);
      VM_DEBUG("PATT_STR_CMP: x=0x%lx, y=0x%lx -> %ld\n", x, y, UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_PATT_STRING: {
      aint val = stack_pop(stack);
      aint result = Bstring_tag_patt((void *)val);
      VM_DEBUG("PATT_STRING: val=0x%lx -> %ld\n", val, UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_PATT_ARRAY: {
      aint val = stack_pop(stack);
      aint result = Barray_tag_patt((void *)val);
      VM_DEBUG("PATT_ARRAY: val=0x%lx -> %ld\n", val, UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_PATT_SEXP: {
      aint val = stack_pop(stack);
      aint result = Bsexp_tag_patt((void *)val);
      VM_DEBUG("PATT_SEXP: val=0x%lx -> %ld\n", val, UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_PATT_BOXED: {
      aint val = stack_pop(stack);
      aint result = Bboxed_patt((void *)val);
      VM_DEBUG("PATT_BOXED: val=0x%lx -> %ld\n", val, UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_PATT_UNBOXED: {
      aint val = stack_pop(stack);
      aint result = Bunboxed_patt((void *)val);
      VM_DEBUG("PATT_UNBOXED: val=0x%lx -> %ld\n", val, UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_PATT_CLOSURE: {
      aint val = stack_pop(stack);
      aint result = Bclosure_tag_patt((void *)val);
      VM_DEBUG("PATT_CLOSURE: val=0x%lx -> %ld\n", val, UNBOX(result));
      stack_push(stack, result);
      break;
    }
    case OP_LINE: {
      int line = read_i32(bc->code, ip);
      ip += 4;
      VM_DEBUG("LINE %d\n", line);
      (void)line;
      break;
    }
    default:
      fprintf(stderr, "Not yet supported opcode 0x%02X at ip=0x%08x\n", opcode,
              ip - 1);
      return;
    }
  }
}

/**
 * Run merged bytecode with multiple main() entry points.
 */
void run_merged(merged_bytecode *merged, int argc, char *argv[]) {
  if (!merged || !merged->bc) {
    fprintf(stderr, "No merged bytecode to run\n");
    return;
  }

  stack_t stack;
  call_stack_t call_stack;
  stack_init(&stack);
  call_stack_init(&call_stack);

  // GC initialization
  __init();
  set_args(argc, argv);

  // Globals
  aint *globals = stack.data;
  for (int i = 0; i < merged->bc->globals_count; i++) {
    globals[i] = BOX(0);
  }
  if (merged->bc->globals_count > 0) {
    globals[0] = (aint)global_sysargs;
  }

  aint *sp_after_globals = stack.sp;

  VM_DEBUG("Globals count: %d, sp after globals: %ld\n",
           merged->bc->globals_count, sp_after_globals - stack.data);

  for (int i = 0; i < merged->main_count; i++) {
    int entry = merged->main_entries[i];
    VM_DEBUG("Executing main() #%d at offset %d\n", i + 1, entry);

    // Reset stack pointer (workaround)
    stack.sp = sp_after_globals;

    // Clear the call stack for each execution
    call_stack_init(&call_stack);

    // TODO: vm structure
    run_internal(merged->bc, entry, &stack, &call_stack, globals,
                 merged->ffi_names, merged->ffi_len);
  }
}

void run_modules(module_list *modules, int argc, char *argv[]) {
  merged_bytecode *merged = merge_modules(modules);
  if (!merged) {
    fprintf(stderr, "Failed to merge modules\n");
    return;
  }

  run_merged(merged, argc, argv);

  free_merged_bytecode(merged);
}

int main(int argc, char *argv[]) {
  if (argc < 2) {
    fprintf(stderr, "Usage: %s <bytecode.bc>\n", argv[0]);
    return 1;
  }

  module_list *modules = load_modules(argv[1], NULL);
  if (!modules) {
    fprintf(stderr, "Failed to load modules from '%s'\n", argv[1]);
    return 1;
  }

  run_modules(modules, argc - 1, argv + 1);

  free_modules(modules);
  return 0;
}
